#pragma once

float plane_vtx[] = {
	61.658394f,	0.430711f,	-61.658394f,
	61.658394f,	-0.430711f,	-61.658394f,
	61.658394f,	0.430711f,	61.658394f,
	61.658394f,	-0.430711f,	61.658394f,
	-61.658394f,	0.430711f,	-61.658394f,
	-61.658394f,	-0.430711f,	-61.658394f,
	-61.658394f,	0.430711f,	61.658394f,
	-61.658394f,	-0.430711f,	61.658394f,
};

float plane_tx[] = {
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
	0.375000,	0.000000,
	0.375000,	1.000000,
	0.125000,	0.750000,
};

int plane_idx[] = {5, 3, 1, 3, 8, 4, 7, 6, 8, 2, 8, 6, 1, 4, 2, 5, 2, 6, 5, 7, 3, 3, 7, 8, 7, 5, 6, 2, 4, 8, 1, 3, 4, 5, 1, 2, };
int plane_tx_idx[] = {5, 3, 1, 3, 13, 4, 11, 8, 12, 2, 14, 7, 1, 4, 2, 6, 2, 8, 5, 9, 3, 3, 10, 13, 11, 6, 8, 2, 4, 14, 1, 3, 4, 6, 1, 2, };
#define plane_vtxcnt 8
#define plane_idxcnt 36
#define plane_tx_idxcnt 36


//make chef add this part in for easy model build
typedef struct _plane {
	float *vtx;
	float *tx;
	int *idx;
	int *tx_idx;
	int vtxcnt;
	int idxcnt;
	int tx_idxcnt;
} planemdl;

planemdl base_plane = {
	.vtx = plane_vtx,
	.tx = plane_tx,
	.idx = plane_idx,
	.tx_idx = plane_tx_idx,
};
