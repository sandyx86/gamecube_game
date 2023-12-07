#pragma once
#include <gccore.h>
#include <ogc/tpl.h>

typedef guVector Vec3;

typedef struct vtx2 {
    f32 x,y,z;
} Vertex2;

typedef struct collisionmesh {
    Vertex2 box[8];
} CollisionMesh;

typedef struct player {
    Vec3 pos;
    Vec3 vel;
    CollisionMesh hitbox;
} Player;
