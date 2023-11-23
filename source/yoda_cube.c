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
#define VTX_BUFFER_SIZE 1024*sizeof(f32);
GXRModeObj *rmode;
static void *xfb[2] = {NULL, NULL}; //external framebuffer
static float *vb;

static u32 curr_fb = 0;
//static unsigned int do_copy = false;
GXTexObj texObj; //texture object
TPLFile tpl; //tpl file


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


s16 scuffed_cube[] ATTRIBUTE_ALIGN(32) = {
	// x y z
	-60,  60, -60, 	// 0
	 60,  20, -60, 	// 1
	 60,  60,  10, 	// 2
	-60,  60,  60, 	// 3
	 60, -30, -60, 	// 4
	 60, -60,  60, 	// 5
	-60, -60,  60,  // 6
	-60, -60, -60,  // 7
};

u8 pyramid[] ATTRIBUTE_ALIGN(32) = {
    -60,  60, -60, 	// 0
	 60,  20, -60, 	// 1
	 60,  60,  10, 	// 2
	-60,  60,  60, 	// 3
	 60, -30, -60, 	// 4
};

s16 triangle[] ATTRIBUTE_ALIGN(32) = {
    -30, -30, 0,
    30, -30, 0,
    0, 30, 0,
};

s16 *cubeSwitch[2] = {cube, scuffed_cube};

u8 colors[] ATTRIBUTE_ALIGN(32) = {
	// r, g, b, a
	100,  10, 100, 255, // 0 purple
	240,   0,   0, 255,	// 1 red
	255, 100, 255, 255,	// 2 pink
	255, 255,   0, 255, // 3 yellow
	 10, 120,  40, 255, // 4 green
	  0,  20, 100, 255  // 5 blue
};

//use this struct
typedef struct _Shape {
    guVector pos; //location in worldspace
    guVector scale; //scale of object
    guVector axis; //rotation axis
    float rot; //rotation in worldspace
    int vertc; //number of verts
    s16* verts; //pointer to array of verts
}Shape;

void translateShape(Shape *shape) {
    for (int i = 0; i < shape->vertc; i+=3) {
        shape->verts[i+0] = shape->verts[i+0] + shape->pos.x;
        shape->verts[i+1] = shape->verts[i+1] + shape->pos.y;
        shape->verts[i+2] = shape->verts[i+2] + shape->pos.z;
    }
}

void scaleShape(Shape *shape) {
    for (int i = 0; i < shape->vertc; i+=3) {
        shape->verts[i+0] = shape->verts[i+0] * shape->scale.x;
        shape->verts[i+1] = shape->verts[i+1] * shape->scale.y;
        shape->verts[i+2] = shape->verts[i+2] * shape->scale.z;
    }
}

void scaleArray(s16 *va, int c, int scale) {
    for (int i = 0; i < c; i++) {
        va[i] = va[i] * scale;
    }
}

void translateArray(s16 *va, int c, guVector vec) {
    for (int i = 0; i < c; i+=3) {
        va[i] += vec.x;
        va[i+1] += vec.y;
        va[i+2] += vec.z;
    }
}

//make a vertex array from two shapes
s16 *vtxcat(Shape *s1, Shape *s2) {
    size_t size = (s1->vertc + s2->vertc) * sizeof(guVector);
    s16 *va = aligned_alloc(size, 32);
    memset(va, 0, size);
    memcpy(va, s1->verts, s1->vertc * sizeof(guVector));
    
    //memcpy(va + s1->vertc, s2->verts, s2->vertc * sizeof(s16)*3);
    return va;
}

typedef struct _Camera {
    guVector pos;
    guVector up;
    guVector view;
} Camera;

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
    GX_SetCopyClear((GXColor){0,0,0,0xFF}, GX_MAX_Z24);

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

    //setting up the vertex descriptor
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);


    scaleArray(cube, 8*3, 30);
    
    translateArray(cube, 8*3, (guVector){10, 0, 0}); //move cube
    s16 *array = aligned_alloc(16 * sizeof(s16)*3, 64);
    memcpy(array, cube, 8 * sizeof(s16)*3); //move cube into vertex array
    //scaleArray(array, 8*3, 30); //make the shapes larger
    translateArray(cube, 8*3, (guVector){-150, 0, 0}); //move cube back
    memcpy(array + 3*8, cube, 8 * sizeof(s16)*3);
    

    TPL_OpenTPLFromMemory(&tpl, (void *)textures_tpl, textures_tpl_size);
    TPL_GetTexture(&tpl, yoda, &texObj);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetArray(GX_VA_POS, array, 3*sizeof(s16)); //put the cube in pos memory
	GX_SetArray(GX_VA_CLR0, colors, 4*sizeof(u8)); //put the colors in color memory
    GX_SetArray(GX_VA_TEX0, (void *)textures_tpl, textures_tpl_size);
    
    //GX_SetArray(GX_VA_POS, cube, 3*sizeof(s16));

    DCFlushRange(array, 65536);
	DCFlushRange(colors, 65536);
    DCFlushRange((void *)textures_tpl, 65536);
    
    GX_SetNumChans(1); //set number of color channels that are output to TEV stages
    GX_SetNumTexGens(1); //set number of texture coordinates that are available in TEV
    //GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_DECAL);;

    Mtx44 view; //view matrix
    Camera cam = {
        {0.0, 0.0, 0.0}, //camera pos
        {0.0, 1.0, 0.0}, //determines which direction is up
        {0.0, 0.0, -1.0} //looking down -z axis
    };

    static float rotation = 0.0;
    static int scale = 1;
    int swap = 0;

    //game loop
    while(true) {
        guLookAt(view, &cam.pos, &cam.up, &cam.view);
;
        GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
        GX_InvVtxCache();
        GX_InvalidateTexAll();

        PAD_ScanPads();
        int stickX = PAD_StickX(0);
        int stickY = PAD_StickY(0);
        rotation += PAD_SubStickX(0)/20.0;

        //press A to switch models
        if (PAD_ButtonsDown(0) & PAD_BUTTON_RIGHT) {
            //scaleArray(array, 8*3, 2);
            //translateArray(array, 8*3, (guVector){1, 0, 0});
        }

        if (PAD_ButtonsDown(0) & PAD_BUTTON_LEFT) {
            //scaleArray(array, 8*3, 0.5F);
            //translateArray(array, 8*3, (guVector){-1, 0, 0});
        }

        //GX_SetArray(GX_VA_POS, big_array, 3*sizeof(s16));

        moveCamera(&cam, stickX, stickY, rotation);
        drawModel(view, &cam); //draws the world

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

void draw_quad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c) {
    GX_Position1x8(v0);
    GX_Color1x8(c);
    GX_Position1x8(v1);
    GX_Color1x8(c);
    GX_Position1x8(v2);
    GX_Color1x8(c);
    GX_Position1x8(v3);
    GX_Color1x8(c);
}

void moveCamera(Camera *cam, int stickX, int stickY, float rotation) {
    //cameras view is {0.0, 0.0, -1.0};
    cam->pos.x += -PAD_StickX(0)*cosf(DegToRad(rotation))/20.0 - PAD_StickY(0)*sinf(DegToRad(rotation))/20.0;
    cam->pos.z += -PAD_StickX(0)*sinf(DegToRad(rotation))/20.0 + PAD_StickY(0)*cosf(DegToRad(rotation))/20.0;
}

void drawModel(Mtx v, Camera *cam) {
    Mtx model, modelview;
    static guVector axis = {0, 1, 0};
    static float rotation = 0.0;
    
    rotation += PAD_SubStickX(0)/20.0;

    guMtxIdentity(model); //sets value to 1.0 if i == j in matrix
    guMtxRotAxisDeg(v, &axis, rotation); //rotate view around y axis
    guMtxTransApply(model, model, cam->pos.x, cam->pos.y, cam->pos.z); //translate world relative to camera
    guMtxConcat(v, model, modelview);
    //guMtxConcat(model, modelview, modelview);

    //load the modelview matrix into matrix memory
    GX_LoadPosMtxImm(modelview, GX_PNMTX0);

    //will draw whatever is in POS_VA
    GX_Begin(GX_QUADS, GX_VTXFMT0, 48);
    //maybe make an array of draw functions that get called here
        /*
        draw_quad(0, 3, 2, 1, 0); //top face
		draw_quad(0, 7, 6, 3, 1); //left face
		draw_quad(0, 1, 4, 7, 2); //back face
		draw_quad(1, 2, 5, 4, 3); //right face
		draw_quad(2, 3, 6, 5, 4); //front face
		draw_quad(4, 7, 6, 5, 5); //bottom face
        */
        
        drawQuad(0, 3, 2, 1, 0); //top face
		drawQuad(0, 7, 6, 3, 1); //left face
		drawQuad(0, 1, 4, 7, 2); //back face
		drawQuad(1, 2, 5, 4, 3); //right face
		drawQuad(2, 3, 6, 5, 4); //front face
		drawQuad(4, 7, 6, 5, 5); //bottom face
        
        
        drawQuad(0+8, 3+8, 2+8, 1+8, 0); //top face
		drawQuad(0+8, 7+8, 6+8, 3+8, 1); //left face
		drawQuad(0+8, 1+8, 4+8, 7+8, 2); //back face
		drawQuad(1+8, 2+8, 5+8, 4+8, 3); //right face
		drawQuad(2+8, 3+8, 6+8, 5+8, 4); //front face
		drawQuad(4+8, 7+8, 6+8, 5+8, 5); //bottom face
        

    GX_End();
}

//draw a single triangle
void drawFace(u8 x, u8 y, u8 z, u8 c) {
    GX_Position1x8(x);
    GX_Color1x8(c);
    GX_Position1x8(y);
    GX_Color1x8(c);
    GX_Position1x8(z);
    GX_Color1x8(c);
}