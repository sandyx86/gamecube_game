#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <ogc/tpl.h>

typedef guVector Vec3;

typedef struct vtx {
    f32 x,y,z,u,v;
} Vertex;

typedef struct tri {
    Vertex vtx[3];
} Triangle;

typedef struct quad {
    Vertex vtx[4];
} Quadrangle;

typedef struct mdl {
    Triangle *triptr;
    int tricnt;
} Model;

typedef struct _obj {
    Model *mdl;
    Vec3 pos;
    Vec3 scale;
    GXTexObj *txt;
} Object;

Vertex buildVertex(f32 *vtx, f32 *tx, int *idx, int *tx_idx, int n) {
    Vertex v;
    v.x = vtx[0+(idx[n]-1)*3];
    v.y = vtx[1+(idx[n]-1)*3];
    v.z = vtx[2+(idx[n]-1)*3];
    v.u = tx[0+(tx_idx[n]-1)*2];
    v.v = tx[1+(tx_idx[n]-1)*2];
    return v;
}

Triangle buildTriangle(f32 *vtx, f32 *tx, int *idx, int *tx_idx, int n) {
    Triangle tri;
    //tri.vtx = malloc(3*sizeof(Vertex));
    tri.vtx[0] = buildVertex(vtx, tx, idx, tx_idx, n*3);
    tri.vtx[1] = buildVertex(vtx, tx, idx, tx_idx, n*3 + 1);
    tri.vtx[2] = buildVertex(vtx, tx, idx, tx_idx, n*3 + 2);
    return tri;
}

Model buildModel(f32 *vtx, f32 *tx, int *idx, int *tx_idx, int idxcnt) {
    Model mdl;
    mdl.tricnt = idxcnt/3;
    mdl.triptr = malloc(mdl.tricnt * sizeof(Triangle));
    
    for (int i = 0; i < mdl.tricnt; i++) {
        mdl.triptr[i] = buildTriangle(vtx, tx, idx, tx_idx, i);
    }
    return mdl;
}

void drawTriangle(Triangle *tri) {
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GX_Position3f32(tri->vtx[0].x, tri->vtx[0].y, tri->vtx[0].z);
    GX_TexCoord2f32(tri->vtx[0].u, -tri->vtx[0].v);
    //GX_Color4u8(tri->vtx[0].x, tri->vtx[0].y, tri->vtx[0].z, 255);
    GX_Position3f32(tri->vtx[1].x, tri->vtx[1].y, tri->vtx[1].z);
    GX_TexCoord2f32(tri->vtx[1].u, -tri->vtx[1].v);
    //GX_Color4u8(tri->vtx[1].x, tri->vtx[1].y, tri->vtx[1].z, 255);
    GX_Position3f32(tri->vtx[2].x, tri->vtx[2].y, tri->vtx[2].z);
    GX_TexCoord2f32(tri->vtx[2].u, -tri->vtx[2].v);
    //GX_Color4u8(tri->vtx[2].x, tri->vtx[2].y, tri->vtx[2].z, 255);

    GX_End();
}

void translateTri(Triangle *tri, Vec3 vec) {
    tri->vtx[0].x += vec.x;
    tri->vtx[0].y += vec.y;
    tri->vtx[0].z += vec.z;

    tri->vtx[1].x += vec.x;
    tri->vtx[1].y += vec.y;
    tri->vtx[1].z += vec.z;

    tri->vtx[2].x += vec.x;
    tri->vtx[2].y += vec.y;
    tri->vtx[2].z += vec.z;
}


void translateModel(Model *mdl, Vec3 vec) {
    for (int i = 0; i < mdl->tricnt; i++) {
        translateTri(&mdl->triptr[i], vec);
    }
}

void drawModel(Model *mdl) {
    //Triangle *ptr = mdl->triptr;
    for (int i = 0; i < mdl->tricnt; i++) {
        drawTriangle(&mdl->triptr[i]);
    }
}

void drawObject(Object *obj) {
    //GX_LoadTexObj(obj->txt, GX_TEXMAP0);
    //translateModel(obj->mdl, obj->pos);
    drawModel(obj->mdl);
}

//worldspace
typedef struct _worldspace {
    f32 *worldspace; //final worldspace

    u32 index;
    u32 objcount;
    u32 vtxcount;
} Worldspace;

static u32 curr_fb = 0;
GXTexObj texObj; //texture object
TPLFile tpl; //tpl file

typedef struct _camera {
    Vec3 pos;
    Vec3 up;
    Vec3 view;
    float rot; //pointer to rotation
} Camera;

Camera newCamera(Vec3 pos, Vec3 up, Vec3 view) {
    return (Camera){
        {pos.x, pos.y, pos.z},
        {up.x, up.y, up.z},
        {view.x, view.y, view.z},
        0.0,
    };
}

void moveCamera(Camera *cam, int stickX, int stickY) {
    //currently no look up or down
    cam->pos.x += -PAD_StickX(0)*cosf(DegToRad(cam->rot))/20.0 - PAD_StickY(0)*sinf(DegToRad(cam->rot))/20.0;
    cam->pos.z += -PAD_StickX(0)*sinf(DegToRad(cam->rot))/20.0 + PAD_StickY(0)*cosf(DegToRad(cam->rot))/20.0;
}

typedef struct _renderer {
    Mtx model, modelview;
    Mtx44 view;
    Camera *cam;
}Renderer;

//load the modelview matrix into matrix memory
void LoadModelView(Renderer *ren, Mtx44 view, Camera *cam) {
    guVector axis = {0, 1, 0};
    guMtxIdentity(ren->model);
    guMtxRotAxisDeg(view, &axis, cam->rot);
    guMtxTransApply(ren->model, ren->model, ren->cam->pos.x, ren->cam->pos.y, ren->cam->pos.z);
    guMtxConcat(view, ren->model, ren->modelview);
    GX_LoadPosMtxImm(ren->modelview, GX_PNMTX0);
}

//index buffer

//initialize worldspace buffer
void wsInit(Worldspace *ws, size_t s) {
    ws->worldspace = malloc(s);
    memset(ws->worldspace, 0, s);
    ws->objcount = 0;
    ws->vtxcount = 0;
    ws->index = 0;
}

/*
    method GX_DISABLE, GX_DIRECT, GX_INDEX8, GX_INDEX16
    dimensions GX_TEX_S, GX_TEX_ST
    size GX_S8, GX_S16, GX_F32
    texgens
*/
void enableTextures(int method, int dimensions, int size, int texgens) {
    GX_SetVtxDesc(GX_VA_TEX0, method);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, dimensions, size, 0);
    GX_SetNumTexGens(texgens);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
}

void enableColors(int method, int dimensions, int size, int texgens) {
    GX_SetVtxDesc(GX_VA_CLR0, method);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, dimensions, size, 0);
    GX_SetNumTexGens(texgens);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
}