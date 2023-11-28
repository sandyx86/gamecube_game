#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <ogc/tpl.h>

typedef guVector Vec3;

typedef struct _Object Object;
typedef struct _Object {
    Object *next;
    Vec3 pos;
    Vec3 scale;
    Vec3 axis; 
    float rot; //rotation
    int vc; //count of vertices
    void *vi; //memory address for where it should be in the worldspace buffer
    int id;
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
    //cameras view is {0.0, 0.0, -1.0};
    cam->pos.x += -PAD_StickX(0)*cosf(DegToRad(cam->rot))/20.0 - PAD_StickY(0)*sinf(DegToRad(cam->rot))/20.0;
    cam->pos.z += -PAD_StickX(0)*sinf(DegToRad(cam->rot))/20.0 + PAD_StickY(0)*cosf(DegToRad(cam->rot))/20.0;
}

typedef struct _renderer {
    Mtx model, modelview;
    Camera *cam;
}Renderer;

void renRender(Renderer *ren, Mtx44 view, Camera *cam) {
    guVector axis = {0, 1, 0};
    guMtxIdentity(ren->model);
    guMtxRotAxisDeg(view, &axis, cam->rot);
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
    static Object *lastObj; //probably dont need this
    static int objId;
    if (lastObj != NULL) {
        lastObj->next = obj;
    }
    //make a new temporary array to avoid changing the original
    s16 *new = malloc(3*obj->vc*sizeof(s16));
    memcpy(new, obj->va, 3*obj->vc*sizeof(s16));
    translateArray(new, obj->vc, obj->pos);
    scaleArray(new, obj->vc, obj->scale.x); //just scale by x for now cause its easier
    ws->objarray[ws->objcount] = obj; //this might work idk
    
    //copy object to worldspace
    memcpy((void *)ws->worldspace + (int)ws->index, new, 3*obj->vc*sizeof(s16));
    obj->vi = (void *)ws->worldspace+ws->index;
    obj->id = objId++;
    ws->index += obj->vc*sizeof(s16)*3;
    ws->objcount++;
    ws->vtxcount += obj->vc*3;

    lastObj = obj;

    free(new);
}

void wsUpdate(Worldspace *ws) {
    Object *obj = ws->objarray[0]; //start with first object
    s16 *new = malloc(3*obj->vc*sizeof(s16)); //new temporary vertex array
    memcpy(new, obj->va, 3*obj->vc*sizeof(s16)); //copy original model to new array
    int index = 0;
    for (int i = 0; i < ws->objcount; i++) {
        obj = ws->objarray[i];
        translateArray(new, obj->vc, obj->pos); 
        scaleArray(new, obj->vc, obj->scale.x); //just scale by x for now cause its easier
        //ws->objarray[i] = obj;
        memcpy((void *)ws->worldspace + index, new, 3*obj->vc*sizeof(s16));
        index += obj->vc*sizeof(s16)*3;

    }
    free(new);
}

void *GetLastObj(Object *obj) {
    Object *poop;
    while (poop != NULL) {
        poop = poop->next;
    }
    return poop;
}

//calculate size of a whole bunch of objects so i can maybe remove them from worldspace
int calcSize(Object *obj) {
    //probably dont need this
    int size = 0;
    Object *poop = NULL;
    while (poop != NULL) {
        size += poop->vc*3*sizeof(s16);
        poop = poop->next;
    }
    return size;
}

//destroy the world
void wsClear(Worldspace *ws) {
    memset(ws->worldspace, 0, 1024);
    ws->objcount = 0;
    ws->vtxcount = 0;
    ws->index = 0;
}

void wsRemoveAllButFirstObject(Worldspace *ws) {
    Object *poop = ws->objarray[0]->next;
    while (poop != NULL) {
        memset(poop->vi, 0, poop->vc*3*sizeof(s16));
        ws->vtxcount -= poop->vc*3;
        poop = poop->next;
    }
    ws->objcount = 1;
}

void wsClear2(Worldspace *ws) {
    Object *poop = ws->objarray[0];
    while (poop != NULL) {
        memset(poop->vi, 0, poop->vc*3*sizeof(s16));
        ws->vtxcount -= poop->vc*3;
        poop = poop->next;
    }
    ws->objcount = 0;
}

void wsRemove(Worldspace *ws, Object *obj) {
    //memmove(obj->vi, obj->next->vi, calcSize(obj));
    memset(obj->vi, 0, obj->vc*8*sizeof(s16));
    ws->objarray[obj->id] = NULL;
    ws->vtxcount-=obj->vc*3;
    ws->objcount--;
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