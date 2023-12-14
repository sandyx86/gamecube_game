#pragma once

#include <gccore.h>
#include <math.h>
#include <stdbool.h>
#include "renderer.c"
#include "player.c"

Vec3 GetMax(float *bbox);
Vec3 GetMin(float *bbox);
float *generateCube(float x, float y, float z);

//int bbox_idx[] = {5, 3, 1, 3, 8, 4, 7, 6, 8, 2, 8, 6, 1, 4, 2, 5, 2, 6, 5, 7, 3, 3, 7, 8, 7, 5, 6, 2, 4, 8, 1, 3, 4, 5, 1, 2, };
int bbox_idx[] = {0, 3, 2, 1, 0, 7, 6, 3, 0, 1, 4, 7, 1, 2, 5, 4, 2, 3, 6, 5, 4, 7, 6, 5};

void generateBoundingBox(Object *obj) {
    float highest_x = 0.0;
    float highest_y = 0.0;
    float highest_z = 0.0;

    for (int i = 0; i < obj->mdl->tricnt; i += 1) {
        if (fabs(obj->mdl->triptr->vtx[i].x) > highest_x)
            highest_x = fabs(obj->mdl->triptr->vtx[i].x);
        
        if (fabs(obj->mdl->triptr->vtx[i].y) > highest_y)
            highest_y = fabs(obj->mdl->triptr->vtx[i].y);
        
        if (fabs(obj->mdl->triptr->vtx[i].z) > highest_z)
            highest_z = fabs(obj->mdl->triptr->vtx[i].z);
    }

    float box[24] = {
	    highest_x,	highest_y,	-highest_z,
	    highest_x,	-highest_y,	-highest_z,
	    highest_x,	highest_y,	highest_z,
	    highest_x,	-highest_y,	highest_z,
	    -highest_x,	highest_y,	-highest_z,
	    -highest_x,	-highest_y,	-highest_z,
	    -highest_x,	highest_y,	highest_z,
	    -highest_x,	-highest_y,	highest_z,
    };
    //01234567
    //02461357
    //

    float *newbox = malloc(sizeof(box));
    memcpy(newbox, box, sizeof(box));

    obj->bbox = newbox;
}

float *sGenerateBoundingBox(SObject *obj) {
    float highest_x = 0.0;
    float highest_y = 0.0;
    float highest_z = 0.0;

    float x, y, z;

    for (int i = 0; i < obj->vtxcnt; i += 1) {
        x = obj->va[0+(obj->idx[i]-1)*3];
        y = obj->va[01+(obj->idx[i]-1)*3];
        z = obj->va[02+(obj->idx[i]-1)*3];
        
        if (fabs(x) > highest_x)
            highest_x = fabs(x);
        
        if (fabs(y) > highest_y)
            highest_y = fabs(y);
        
        if (fabs(z) > highest_z)
            highest_z = fabs(z);
    }

    return generateCube(highest_x, highest_y, highest_z);
}

void ungenerateBoundingBox(Object *obj) {
    free(obj->bbox);
}

float *generateCube(float x, float y, float z) {
    float *g;
    float box[] = {
	    x,	y,	-z,
	    x,	-y,	-z,
	    x,	y,	z,
	    x,	-y,	z,
	    -x,	y,	-z,
	    -x,	-y,	-z,
	    -x,	y,	z,
	    -x,	-y,	z,
    };

    g = malloc(sizeof(box));
    memcpy(g, box, sizeof(box));
    return g;
}

void freeCube(float *p) {
    free(p);
}

Vec3 VectorMidpoint(Vec3 a, Vec3 b) {
    return (Vec3){((a.x+b.x)/2), ((a.y+b.y)/2), ((a.z+b.z)/2)};
}

//get max of bounding box
Vec3 GetMax(float *bbox) {
    Vec3 max = {0.0,0.0,0.0};

    //assume there are 24 floating points
    for (int i = 0; i < 24; i += 3) {
        if (bbox[i] > max.x) {
            max.x = bbox[i];
        }

        if (bbox[i+1] > max.y) {
            max.y = bbox[i+1];
        }

        if (bbox[i+2] > max.z) {
            max.z = bbox[i+2];
        }
    }

    return max;
}

Vec3 GetMin(float *bbox) {
    Vec3 min = {0.0,0.0,0.0};

    //assume there are 24 floating points
    for (int i = 0; i < 24; i += 3) {
        if (bbox[i] < min.x) {
            min.x = bbox[i];
        }

        if (bbox[i+1] < min.y) {
            min.y = bbox[i+1];
        }

        if (bbox[i+2] < min.z) {
            min.z = bbox[i+2];
        }
    }

    return min;
}

Vec3* makeVec(float *bbox) {
    static Vec3 vecs[8];
    for (int i = 0; i < 8; i++) {
        vecs[i].x = bbox[i*3];
        vecs[i].y = bbox[1+(i*3)];
        vecs[i].z = bbox[2+(i*3)];
    }
    return vecs;
}

void drawBoundingBox(float *bbox) {
    //GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
    Vec3 *a = makeVec(bbox);
    //GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    //GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
    //GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_U8, 0);

    GX_Begin(GX_LINES, vtxfmt, 128);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            GX_Position3f32(a[i].x, a[i].y, a[i].z);
            GX_TexCoord2f32(0.0, 0.0);
            //GX_Color4u8(0xff, 0x0, 0x0, 0xff);
            
            GX_Position3f32(a[j].x, a[j].y, a[j].z);
            GX_TexCoord2f32(0.0, 0.0);
            //GX_Color4u8(0xff, 0x0, 0x0, 0xff);
        }
    }

    //GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_End();
}  

/*
    "If you're able to draw a line between two separate polygons,
        then they do not collide." - Separating Axis Theorem, 2023
*///check collision between player and object
bool checkCollision(Player *player, Object *obj) {
    //Vec3 mp = VectorMidpoint(player->pos, obj->pos);
    Vec3 player_max = GetMax(player->collisionbox);
    Vec3 player_min = GetMin(player->collisionbox);
    Vec3 obj_max = GetMax(obj->bbox);
    Vec3 obj_min = GetMin(obj->bbox);

    bool x, y, z;
    int r, g, b;

    x = false;
    z = false;
    if ((player_max.x + player->pos.x >= obj_min.x + obj->pos.x) && (player_min.x + player->pos.x <= obj_max.x + obj->pos.x)) {
        //collision
        r = 0x55;
        //GX_SetCopyClear((GXColor){0x88, 0x00, 0x00, 0xFF}, GX_MAX_Z24);
        x = true;
    } else {r = 0;}
    
    if ((player_max.z + player->pos.z >= obj_min.z + obj->pos.z) && (player_min.z + player->pos.z <= obj_max.z + obj->pos.z)) {
        //collision
        //GX_SetCopyClear((GXColor){r++, g++, b++, 0xFF}, GX_MAX_Z24);
        b = 0x55;
        z = true;
    } else {b = 0;}

    //GX_SetCopyClear((GXColor){r, 0x00, b, 0xFF}, GX_MAX_Z24);

    if (x && z) {
        drawBoundingBox(obj->bbox);
    }
    
    return x && z;
}
