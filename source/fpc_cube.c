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
GXRModeObj *rmode;
static void *xfb[2] = {NULL, NULL}; //external framebuffer
static u32 curr_fb = 0;
//static unsigned int do_copy = false;

GXTexObj texObj; //texture object
TPLFile tpl; //tpl file

s16 cube[] ATTRIBUTE_ALIGN(32) = {
	// x y z
	-30,  30, -30, 	// 0
	 30,  30, -30, 	// 1
	 30,  30,  30, 	// 2
	-30,  30,  30, 	// 3
	 30, -30, -30, 	// 4
	 30, -30,  30, 	// 5
	-30, -30,  30,  // 6
	-30, -30, -30,  // 7
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

typedef struct _Camera {
    guVector pos;
    guVector up;
    guVector view;
} Camera;

void draw_cube(Mtx v, Camera *cam);

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

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    
    GX_SetArray(GX_VA_POS, cube, 3*sizeof(s16)); //put the cube in pos memory
	GX_SetArray(GX_VA_CLR0, colors, 4*sizeof(u8)); //put the colors in color memory

    DCFlushRange(cube,sizeof(cube));
	DCFlushRange(colors,sizeof(colors));
    
    GX_SetNumChans(1); //set number of color channels that are output to TEV stages
    GX_SetNumTexGens(0); //set number of texture coordinates that are available in TEV
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    Mtx44 view; //view matrix
    Camera cam = {
        {0.0, 0.0, 0.0}, //camera pos
        {0.0, 1.0, 0.0}, //determines which direction is up
        {0.0, 0.0, -1.0} //looking down -z axis
    };

    static float rotation = 0.0; //theta

    //game loop
    while(true) {
        guLookAt(view, &cam.pos, &cam.up, &cam.view);

        //guLookAt(view, &cam, &up, &look);
        GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
        GX_InvVtxCache();
        GX_InvalidateTexAll();
        PAD_ScanPads();
        int stickX = PAD_StickX(0);
        int stickY = PAD_StickY(0);
        rotation += PAD_SubStickX(0)/20.0;

        moveCamera(&cam, stickX, stickY, rotation);
        draw_cube(view, &cam); //draws a cube in the view matrix

        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(xfb[curr_fb], GX_TRUE);

        GX_DrawDone();
        VIDEO_SetNextFramebuffer(xfb[curr_fb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }
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

void draw_cube(Mtx v, Camera *cam) {
    Mtx model, modelview, xorz;
    static guVector axis = {0, 1, 0};
    static float rotation = 0.0;
    
    rotation += PAD_SubStickX(0)/20.0;

    guMtxIdentity(model); //sets value to 1.0 if i == j in matrix
    guMtxRotAxisDeg(v, &axis, rotation); //rotate view around y axis
    guMtxTransApply(model, model, cam->pos.x, cam->pos.y, cam->pos.z); //translate world relative to camera
    guMtxConcat(v, model, modelview);

    //load the modelview matrix into matrix memory
    GX_LoadPosMtxImm(modelview, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
    //maybe make an array of draw functions that get called here
        draw_quad(0, 3, 2, 1, 0);
		draw_quad(0, 7, 6, 3, 1);
		draw_quad(0, 1, 4, 7, 2);
		draw_quad(1, 2, 5, 4, 3);
		draw_quad(2, 3, 6, 5, 4);
		draw_quad(4, 7, 6, 5, 5);
    GX_End();
}