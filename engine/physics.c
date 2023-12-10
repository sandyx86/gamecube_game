#pragma once

#include <gccore.h>
#include <math.h>
#include <stdbool.h>
#include "renderer.c"
#include "player.c"

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

    float *newbox = malloc(sizeof(box));
    memcpy(newbox, box, sizeof(box));

    obj->bbox = newbox;
}

void ungenerateBoundingBox(Object *obj) {
    free(obj->bbox);
}

void generateCube(float m) {
    float box[] = {
	    m,	m,	-m,
	    m,	-m,	-m,
	    m,	m,	m,
	    m,	-m,	m,
	    -m,	m,	-m,
	    -m,	-m,	-m,
	    -m,	m,	m,
	    -m,	-m,	m,
    };
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

    GX_SetCopyClear((GXColor){r, 0x00, b, 0xFF}, GX_MAX_Z24);
    
    return x && z;
}
