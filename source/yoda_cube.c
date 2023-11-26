#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <ogc/tpl.h>

#include "textures_tpl.h"
#include "textures.h"


#define DEFAULT_FIFO_SIZE (256*1024)
#define VTX_BUFFER_SIZE 10*sizeof(s16)*3*8 //10 cubes worth
GXRModeObj *rmode;
static float rotation = 0.0; //rotation for camera
static void *xfb[2] = {NULL, NULL}; //external framebuffer
typedef guVector Vec3;

typedef struct _Object {
    Vec3 pos;
    Vec3 scale;
    Vec3 axis; 
    float rot; //rotation
    int vc; //count of vertices
    int vi; //memory address for where it should be in the worldspace buffer
    s16 *va; //pointer to vertex array
}Object;


//worldspace
typedef struct _worldspace {
    s16 *worldspace; //final worldspace
    Object *objarray[32]; //hard coded 32 object max for now
    u32 index;
    u32 objcount;
    u32 vtxcount;
}Worldspace;

static u32 curr_fb = 0;
GXTexObj texObj; //texture object
TPLFile tpl; //tpl file

typedef struct _Camera {
    Vec3 pos;
    Vec3 up;
    Vec3 view;
    float *rot; //pointer to rotation
} Camera;

Camera newCamera(Vec3 pos, Vec3 up, Vec3 view) {
    return (Camera){
        {pos.x, pos.y, pos.z},
        {up.x, up.y, up.z},
        {view.x, view.y, view.z},
        &rotation //address of global camera rotation variable
    };
}

typedef struct _renderer {
    Mtx model, modelview;
    Camera *cam;
}Renderer;

void renRender(Renderer *ren, Mtx44 view) {
    guVector axis = {0, 1, 0};
    guMtxIdentity(ren->model);
    guMtxRotAxisDeg(view, &axis, rotation);
    guMtxTransApply(ren->model, ren->model, ren->cam->pos.x, ren->cam->pos.y, ren->cam->pos.z);
    guMtxConcat(view, ren->model, ren->modelview);
    GX_LoadPosMtxImm(ren->modelview, GX_PNMTX0);
}

//scales an object
void scaleArray(s16* va, size_t len, s16 scale) {
    for (int i = 0; i < len*3; i++) {
        va[i] = scale * va[i];
    }
}

void translateArray(s16 *va, int nvec, guVector vec) {
    for (int i = 0; i < nvec; i++) {
        va[i*3] += vec.x;
        va[(i*3)+1] += vec.y;
        va[(i*3)+2] += vec.z;
    }
}

//initialize worldspace buffer
void wsInit(Worldspace *ws, size_t s) {
    ws->worldspace = malloc(s);
    memset(ws->worldspace, 0, s);
    ws->objcount = 0;
    ws->vtxcount = 0;
    ws->index = 0;
}

//add an object to worldspace
void wsAppend(Worldspace *ws, Object *obj) {
    //make a new temporary array to avoid changing the original
    volatile s16 *new = malloc(3*obj->vc*sizeof(s16));
    memcpy(new, obj->va, 3*obj->vc*sizeof(s16));
    translateArray(new, obj->vc, obj->pos);
    scaleArray(new, obj->vc, obj->scale.x); //just scale by x for now cause its easier
    ws->objarray[ws->objcount] = &obj; //this might work idk
    //copy object to worldspace
    memcpy((int)ws->worldspace + (int)ws->index, new, 3*obj->vc*sizeof(s16));
    obj->vi = ws->index;
    ws->index += obj->vc*sizeof(s16)*3;
    ws->objcount++;
    ws->vtxcount += obj->vc*3;

    free(new);
}

//append an object to worldspace without scaling or translation
void wsRawAppend(Worldspace *ws, Object *obj) {
    s16 *new = malloc(obj->vc*sizeof(s16));
    memcpy(new, obj->va, obj->vc*sizeof(s16));
    memcpy(ws->worldspace + ws->index, new, obj->vc*sizeof(s16));
    ws->index += obj->vc*sizeof(s16);
    ws->objcount++;
    ws->vtxcount += obj->vc;
    free(new);
}


//original cube
s16 cube[] ATTRIBUTE_ALIGN(32) = {
	// x y z
	-1,  1, -1, 	// 0
	 1,  1, -1, 	// 1
	 1,  1,  1, 	// 2
	-1,  1,  1, 	// 3
	 1, -1, -1, 	// 4
	 1, -1,  1, 	// 5
	-1, -1,  1,  // 6
	-1, -1, -1,  // 7
};

u8 colors[] ATTRIBUTE_ALIGN(32) = {
	// r, g, b, a
	100,  10, 100, 255, // 0 purple
	240,   0,   0, 255,	// 1 red
	255, 100, 255, 255,	// 2 pink
	255, 255,   0, 255, // 3 yellow
	 10, 120,  40, 255, // 4 green
	  0,  20, 100, 255  // 5 blue
};



void drawModel(Mtx v, Camera *cam);
void drawFace(u8,u8,u8,u8);
void moveCamera(Camera *cam, int stickX, int stickY, float rotation);
void rotateCamera(Mtx, int);

int main(void) {
    //int32_t xfbHeight;
    void *gp_fifo = NULL; //CPU to GP FIFO buffer for the GP's command processor
    VIDEO_Init();
    PAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb[curr_fb]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync(); //wait for next vertical retrace (cap fps to refresh rate)
    
    //gp_fifo must be aligned to a 32 byte boundary
    gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
    if (gp_fifo == NULL) {
        return 0; //alignment failed
    }

    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE); //set all the fifo buffer to 0

    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    GX_SetCopyClear((GXColor){0,128,128,0xFF}, GX_MAX_Z24);

    //viewport
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight/(f32)rmode->efbHeight);

    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight==2*rmode->xfbHeight)?true:false));

    if (rmode->aa) {
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR); //enables antialiasing
    } else {
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    }

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(xfb[curr_fb], true); //copy efb to xfb and clear color and z buffer
    GX_SetDispCopyGamma(GX_GM_1_0); //gamma correction applied during efb to xfb copy
    
    Mtx44 perspective; //empty 4x4 matrix
    guPerspective(perspective, 90, 1.33F, 10.0F, 1000.0F); //creates the matrix
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE); //load matrix into register

    //set vertex descriptors for position color and texture
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    TPL_OpenTPLFromMemory(&tpl, (void *)textures_tpl, textures_tpl_size);
    TPL_GetTexture(&tpl, gordon, &texObj); //load tpl into a texture object
    GX_LoadTexObj(&texObj, GX_TEXMAP0); //load texture object into register

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

	GX_SetArray(GX_VA_CLR0, colors, 4*sizeof(u8)); //put the colors in color memory
    GX_SetArray(GX_VA_TEX0, (void *)textures_tpl, 3*sizeof(f32));

    DCFlushRange(cube, 65536);
	DCFlushRange(colors, 65536);
    DCFlushRange((void *)textures_tpl, 65536);
    
    GX_SetNumChans(1); //set number of color channels that are output to TEV stages
    GX_SetNumTexGens(1); //set number of texture coordinates that are available in TEV
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_DECAL);

    Mtx44 view; //view matrix

    static int a_press = 4;

    Worldspace worldspace;
    wsInit(&worldspace, 1024);

    Camera cam = newCamera((Vec3){0.0, 0.0, 0.0}, (Vec3){0.0, 1.0, 0.0}, (Vec3){0.0, 0.0, -1.0});
    Object obj, poop;
    obj.pos = (Vec3){0.0, 0.0, 0.0};
    obj.va = cube;
    obj.vc = 8;
    obj.scale = (Vec3){1, 10, 10};

    poop.pos = (Vec3){5.0, 7.0, 2.0};
    poop.va = cube;
    poop.vc = 8;
    poop.scale = (Vec3){10, 0, 0};

    scaleArray(cube, 24, 10);
    wsAppend(&worldspace, &obj);
    wsAppend(&worldspace, &poop);
    poop.scale = (Vec3){23, 0, 0};
    wsAppend(&worldspace, &poop);
    obj.pos = (Vec3){0.0, 9.0, 4.0};
    wsAppend(&worldspace, &obj);
    wsAppend(&worldspace, &poop);

    Renderer ren;
    ren.cam = &cam;

    //game loop
    while(true) {
        GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
        GX_InvVtxCache();
        GX_InvalidateTexAll();

        PAD_ScanPads();
        int stickX = PAD_StickX(0);
        int stickY = PAD_StickY(0);
        rotation += PAD_SubStickX(0)/20.0;

        //press A to switch models
        if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {
            a_press++;
        }

        if (PAD_ButtonsDown(0) & PAD_BUTTON_B) {
            a_press--;
        }
        
        guLookAt(view, &cam.pos, &cam.up, &cam.view);
        moveCamera(&cam, stickX, stickY, rotation);

        GX_SetArray(GX_VA_POS, worldspace.worldspace, 3*sizeof(s16));

        renRender(&ren, view);

            GX_Begin(GX_QUADS, GX_VTXFMT0, worldspace.vtxcount);
            for (int i = 0; i < worldspace.objcount; i++) {
                drawQuad(0+i*8, 3+i*8, 2+i*8, 1+i*8, 0); //top face
		        drawQuad(0+i*8, 7+i*8, 6+i*8, 3+i*8, 1); //left face
		        drawQuad(0+i*8, 1+i*8, 4+i*8, 7+i*8, 2); //back face
		        drawQuad(1+i*8, 2+i*8, 5+i*8, 4+i*8, 3); //right face
		        drawQuad(2+i*8, 3+i*8, 6+i*8, 5+i*8, 4); //front face
		        drawQuad(4+i*8, 7+i*8, 6+i*8, 5+i*8, 5); //bottom face
            }
            GX_End();


        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(xfb[curr_fb], GX_TRUE);

        GX_DrawDone();
        VIDEO_SetNextFramebuffer(xfb[curr_fb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }
}

void drawVert(u8 pos, u8 c, f32 s, f32 t) {
    GX_Position1x8(pos);
	GX_Color1x8(c);
	GX_TexCoord2f32(s, t);
}

void drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c) {
    drawVert(v0, 0, 0.0, 0.0);
    drawVert(v1, 0, 1.0, 0.0);
    drawVert(v2, 0, 1.0, 1.0);
    drawVert(v3, 0, 0.0, 1.0);
}

void moveCamera(Camera *cam, int stickX, int stickY, float rotation) {
    //cameras view is {0.0, 0.0, -1.0};
    cam->pos.x += -PAD_StickX(0)*cosf(DegToRad(rotation))/20.0 - PAD_StickY(0)*sinf(DegToRad(rotation))/20.0;
    cam->pos.z += -PAD_StickX(0)*sinf(DegToRad(rotation))/20.0 + PAD_StickY(0)*cosf(DegToRad(rotation))/20.0;
}