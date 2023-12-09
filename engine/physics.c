#pragma once

#include <gccore.h>
#include <math.h>

/*
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

    obj->bbox = box;
}
*/

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
