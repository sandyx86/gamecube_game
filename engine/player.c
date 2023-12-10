#pragma once
#include <gccore.h>
#include <ogc/tpl.h>

#include "renderer.c"

//the players hitbox
float playercollision_vtx[] = {
	10.000620f,	20.031015f,	-10.000620f,
	10.000620f,	-20.031015f,	-10.000620f,
	10.000620f,	20.031015f,	10.000620f,
	10.000620f,	-20.031015f,	10.000620f,
	-10.000620f,	20.031015f,	-10.000620f,
	-10.000620f,	-20.031015f,	-10.000620f,
	-10.000620f,	20.031015f,	10.000620f,
	-10.000620f,	-20.031015f,	10.000620f,
};

float playercollision_tx[] = {
	0.625000,	0.500000,
	0.375000,	0.500000,
	0.625000,	0.750000,
	0.375000,	0.750000,
	0.875000,	0.500000,
	0.625000,	0.250000,
	0.125000,	0.500000,
	0.375000,	0.250000,
	0.875000,	0.750000,
	0.625000,	1.000000,
	0.625000,	0.000000,
	0.375000,	1.000000,
	0.375000,	0.000000,
	0.125000,	0.750000,
};

int playercollision_idx[] = {1, 5, 7, 3, 4, 3, 7, 8, 8, 7, 5, 6, 6, 2, 4, 8, 2, 1, 3, 4, 6, 5, 1, 2, };
int playercollision_tx_idx[] = {1, 5, 9, 3, 4, 3, 10, 12, 13, 11, 6, 8, 7, 2, 4, 14, 2, 1, 3, 4, 8, 6, 1, 2, };
#define playercollision_vtxcnt 8
#define playercollision_idxcnt 24
#define playercollision_tx_idxcnt 24
 
typedef guVector Vec3;

typedef struct _collisionmesh {
    Vec3 box[8];
} CollisionMesh;

typedef struct _player {
    Vec3 pos;
    Vec3 vel;
	float rot;
    float *collisionbox;
} Player;

Player player;

void initPlayer() {
    player.pos = (Vec3){0.0, 0.0, 0.0};
    player.collisionbox = playercollision_vtx;
}

//moves the player
void movePlayer() {
	player.pos.x += player.vel.x;
	player.pos.y += player.vel.y;
	player.pos.z += player.vel.z;
}

//bind player to camera
void movePlayerByCam(Camera *cam) {
	player.pos.x = cam->pos.x;
	player.pos.y = cam->pos.y;
	player.pos.z = cam->pos.z;
}

//reads controller and adds or removes velocity from player
void acceleratePlayer() {
	player.vel.x = PAD_StickX(0)/10.0;
	player.vel.z = PAD_StickY(0)/10.0;
	movePlayer();
}