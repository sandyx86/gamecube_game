#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <ogc/tpl.h>

#include "textures_tpl.h"
#include "textures.h"

#include <renderer.c>
#include <input_handler.c>
#include <physics.c>
#include <player.c>

#include "../models/cylinder.c"
#include <../models/plane.c>

#define DEFAULT_FIFO_SIZE (256 * 1024)
#define VTX_BUFFER_SIZE 10 * sizeof(s16) * 3 * 8 // 10 tests worth
static void *xfb[2] = {NULL, NULL}; // external framebuffer
GXRModeObj *rmode;

int color;
void changeColor();
void moveCam();
void swapFunc();
void *parameters[20];
Camera cam;
Renderer ren;
Mtx44 view; // view matrix

int main(void) {
    void *gp_fifo = NULL; // CPU to GP FIFO buffer for the GP's command processor
    VIDEO_Init();
    PAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb[curr_fb]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    //VIDEO_WaitVSync(); // wait for next vertical retrace (cap fps to refresh rate)

    // gp_fifo must be aligned to a 32 byte boundary
    gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
    if (gp_fifo == NULL) {
        return 0; // alignment failed
    }

    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE); // set all the fifo buffer to 0

    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    GX_SetCopyClear((GXColor){0, 128, 128, 0xFF}, GX_MAX_Z24);

    // viewport
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);

    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? true : false));

    if (rmode->aa) {
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_NEAR); // enables antialiasing
    }
    else {
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_NEAR);
    }

    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(xfb[curr_fb], true); // copy efb to xfb and clear color and z buffer
    GX_SetDispCopyGamma(GX_GM_1_0);  // gamma correction applied during efb to xfb copy

    Mtx44 perspective;                                     // empty 4x4 matrix
    guPerspective(perspective, 90, 1.33F, 10.0F, 10000.0F); // creates the matrix
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);     // load matrix into register

    // set vertex descriptors for position color and texture
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    TPL_OpenTPLFromMemory(&tpl, (void *)textures_tpl, textures_tpl_size);
    TPL_GetTexture(&tpl, gordon, &texObj); // load tpl into a texture object
    //GX_InitTexObj(&texObj, textures_tpl, 32, 32, GX_TF_RGBA8, GX_REPEAT, GX_REPEAT, GX_FALSE);
    GX_InitTexObjWrapMode(&texObj, GX_REPEAT, GX_REPEAT);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);    // load texture object into register

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);

    GX_SetArray(GX_VA_TEX0, (void *)textures_tpl, 3 * sizeof(f32));

    DCFlushRange(cylinder_vtx, 65536);
    DCFlushRange((void *)textures_tpl, 65536);

    GX_SetNumChans(1);   // set number of color channels that are output to TEV stages
    GX_SetNumTexGens(3); // set number of texture coordinates that are available in TEV
    //GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOp(GX_TEVSTAGE0, GX_DECAL);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

    cam = newCamera((Vec3){0.0, 0.0, 0.0}, (Vec3){0.0, 1.0, 0.0}, (Vec3){0.0, 0.0, -1.0});
    ren.cam = &cam;
    
    //Model the_model = buildModel(tri_vtx, tri_tx, tri_idx, tri_tx_idx, tri_idxcnt);
    Model the_model = buildModel(cylinder_vtx, cylinder_tx, cylinder_idx, cylinder_tx_idx, cylinder_idxcnt);
    Model flat = buildModel(plane_vtx, plane_tx, plane_idx, plane_tx_idx, plane_idxcnt);
    Object the_object;
    the_object.mdl = &the_model;
    the_object.pos = (Vec3){0.0, 0.0, 0.0};

    Worldspace ws;
    wsInit(&ws, 1 << 16);
    
    wsAppend(&ws, &the_object);
    the_object.pos = (Vec3){0.0, 24.0, 0.0};
    wsAppend(&ws, &the_object);
    the_object.pos = (Vec3){13.0, 0.0, 0.0};
    wsAppend(&ws, &the_object);

    translateModel(&flat, (Vec3){0.0, -10.0, 0.0});

    imap.parameter;
    initHandler();
    assignFunction(PAD_BUTTON_B, swapFunc);
    
    // game loop
    while (true) {
        GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
        GX_InvVtxCache();
        GX_InvalidateTexAll();

        PAD_ScanPads();

        moveCam();
        handleInput();
        
        drawModel((Model *)&ws);
        drawModel(&flat);
        drawObject(&the_object);
     
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(xfb[curr_fb], GX_TRUE);
        GX_Flush();
        GX_DrawDone();
        VIDEO_SetNextFramebuffer(xfb[curr_fb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }
}

void changeColor() {
    static int c;
    GX_SetCopyClear((GXColor){0.0, c++, PAD_SubStickX(0), 0xFF}, GX_MAX_Z24);
}

void moveCam() {
    guLookAt(view, &cam.pos, &cam.up, &cam.view);
    int stickX = PAD_StickX(0);
    int stickY = PAD_StickY(0);
    cam.rot += PAD_SubStickX(0) / 20.0;
    cam.pos.x += -stickX*cosf(DegToRad(cam.rot))/20.0 - stickY*sinf(DegToRad(cam.rot))/20.0;
    cam.pos.z += -stickX*sinf(DegToRad(cam.rot))/20.0 + stickY*cosf(DegToRad(cam.rot))/20.0;
    LoadModelView(&ren, view);
}

void swapFunc() {
    if (imap.buttonA != changeColor) {
        assignFunction(PAD_BUTTON_A, changeColor);
    } else {
        assignFunction(PAD_BUTTON_A, moveCam);
    }
}