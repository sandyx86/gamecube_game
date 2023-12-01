#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <ogc/tpl.h>

#include "../other/renderer.c"
#include "textures_tpl.h"
#include "textures.h"

#include "../other/rounded.c"
#include "../other/weird.c"

#define DEFAULT_FIFO_SIZE (256 * 1024)
#define VTX_BUFFER_SIZE 10 * sizeof(s16) * 3 * 8 // 10 cubes worth
static void *xfb[2] = {NULL, NULL}; // external framebuffer
GXRModeObj *rmode;

/*
// original cube
s16 cube[] ATTRIBUTE_ALIGN(32) = {
    // x y z
    -1, 1, -1,  // 0
    1, 1, -1,   // 1
    1, 1, 1,    // 2
    -1, 1, 1,   // 3
    1, -1, -1,  // 4
    1, -1, 1,   // 5
    -1, -1, 1,  // 6
    -1, -1, -1, // 7
};
*/

u8 colors[] ATTRIBUTE_ALIGN(32) = {
    // r, g, b, a
    100, 10, 100, 255,  // 0 purple
    240, 0, 0, 255,     // 1 red
    255, 100, 255, 255, // 2 pink
    255, 255, 0, 255,   // 3 yellow
    10, 120, 40, 255,   // 4 green
    0, 20, 100, 255     // 5 blue
};

void drawVert(u8 pos, u8 c, f32 s, f32 t);
void drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c);
void drawTriangle(f32 v0, f32 v1, f32 v2, u8 c);
void drawTriangles(f32 *va, int *idx, f32* tx, int *tx_idx, int vc);

int main(void) {
    // int32_t xfbHeight;
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
    VIDEO_WaitVSync(); // wait for next vertical retrace (cap fps to refresh rate)

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
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR); // enables antialiasing
    }
    else {
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
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
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    TPL_OpenTPLFromMemory(&tpl, (void *)textures_tpl, textures_tpl_size);
    TPL_GetTexture(&tpl, gordon, &texObj); // load tpl into a texture object
    GX_LoadTexObj(&texObj, GX_TEXMAP0);    // load texture object into register

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    
    //can change this to GX_F32
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    //GX_SetArray(GX_VA_CLR0, colors, 4 * sizeof(u8)); // put the colors in color memory
    GX_SetArray(GX_VA_TEX0, (void *)textures_tpl, 3 * sizeof(s16));

    DCFlushRange(rounded_vtx, 65536);
    DCFlushRange((void *)textures_tpl, 65536);
    //DCFlushRange(rounded_tx_idx, 65536);

    GX_SetNumChans(1);   // set number of color channels that are output to TEV stages
    GX_SetNumTexGens(1); // set number of texture coordinates that are available in TEV
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_DECAL);
    //GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	//GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    Mtx44 view; // view matrix
    static int a_press = 4;

    Worldspace worldspace;
    wsInit(&worldspace, 1024);

    Camera cam = newCamera((Vec3){0.0, 0.0, 0.0}, (Vec3){0.0, 1.0, 0.0}, (Vec3){0.0, 0.0, -1.0});
    Object bigbox;
    bigbox.pos = (Vec3){0.0, 0.0, 0.0};
    bigbox.scale = (Vec3){100.0, 0.0, 0.0};
    bigbox.vc = 24;
    bigbox.va = rounded_vtx;
    wsAppend(&worldspace, &bigbox);
    

    Renderer ren;
    ren.cam = &cam;

    // game loop
    while (true) {
        GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
        GX_InvVtxCache();
        GX_InvalidateTexAll();
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_DECAL);

        PAD_ScanPads();
        int stickX = PAD_StickX(0);
        int stickY = PAD_StickY(0);
        cam.rot += PAD_SubStickX(0) / 20.0;

        // press A to inflate cube
        if (PAD_ButtonsDown(0) & PAD_BUTTON_A)
        {
            bigbox.scale.x += 9.0;
        }

        //press B to deflate cube
        if (PAD_ButtonsDown(0) & PAD_BUTTON_B)
        {
            bigbox.scale.x -= 9.0;
        }

        wsUpdate(&worldspace);

        //renderer stuff
        guLookAt(view, &cam.pos, &cam.up, &cam.view);
        moveCamera(&cam, stickX, stickY);

        //GX_SetArray(GX_VA_POS, worldspace.worldspace, 3 * sizeof(s16));

        rDraw(&ren, view, &cam);

        GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 132);
        for (int i = 0; i < 3; i+=3) {
            //need a worldspace index, and vtxcount
            drawTriangles(worldspace.worldspace, rounded_idx, rounded_tx, rounded_tx_idx, 132);
        }
        GX_End();

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

void drawVert(u8 pos, u8 c, f32 s, f32 t) {
    GX_Position1x8(pos);
    GX_Color1x8(c);
    GX_TexCoord2f32(s, t);
}

void drawTriangles(f32 *va, int *idx, f32* tx, int *tx_idx, int vc) {
        for (int j = 0; j < vc; j++) {
            drawPoint(va[((idx[j]-1)*3)+0], va[((idx[j]-1)*3)+1], va[((idx[j]-1)*3)+2], tx[((tx_idx[j]-1)*2)], tx[((tx_idx[j]-1)*2)+1], idx[j]/4);
        }
}

//idk why textures stop working
void drawPoint(f32 x, f32 y, f32 z, f32 s, f32 t, u8 c) {
    GX_Position3f32(x, y, z);
    GX_TexCoord2f32(s, t);
    GX_Color4u8(x, y, z, 255);
}

void drawQuad2(u8 v0, u8 v1, u8 v2, int idx) {
    drawVert(v0, 0, 0.0, 0.0);
    drawVert(v1, 0, 1.0, 0.0);
    drawVert(v2, 0, 1.0, 1.0);
}

void drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c) {
    drawVert(v0, 0, 0.0, 0.0);
    drawVert(v1, 0, 1.0, 0.0);
    drawVert(v2, 0, 1.0, 1.0);
    drawVert(v3, 0, 0.0, 1.0);
}