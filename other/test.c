#pragma once

float test_vtx[] = {
	1.000000f,	1.000000f,	-1.000000f,
	1.000000f,	-1.000000f,	-1.000000f,
	1.000000f,	1.000000f,	1.000000f,
	1.000000f,	-1.000000f,	1.000000f,
	-1.000000f,	1.000000f,	-1.000000f,
	-1.000000f,	-1.000000f,	-1.000000f,
	-1.000000f,	1.000000f,	1.000000f,
	-1.000000f,	-1.000000f,	1.000000f,
};

float test_tx[] = {
	0.994537,	0.004466,
	0.005463,	0.004466,
	0.994537,	0.993541,
	0.005463,	0.993541,
	1.983612,	0.004466,
	0.994537,	-0.984608,
	-0.983612,	0.004466,
	0.005463,	-0.984608,
	1.983612,	0.993541,
	0.994537,	1.982615,
	0.994537,	-1.973683,
	0.005463,	-1.973683,
	0.005463,	1.982615,
	-0.983612,	0.993541,
};

int test_idx[] = {5, 3, 1, 3, 8, 4, 7, 6, 8, 2, 8, 6, 1, 4, 2, 5, 2, 6, 5, 7, 3, 3, 7, 8, 7, 5, 6, 2, 4, 8, 1, 3, 4, 5, 1, 2, };
int test_tx_idx[] = {5, 3, 1, 3, 13, 4, 11, 8, 12, 2, 14, 7, 1, 4, 2, 6, 2, 8, 5, 9, 3, 3, 10, 13, 11, 6, 8, 2, 4, 14, 1, 3, 4, 6, 1, 2, };
#define test_vtxcnt 8
#define test_idxcnt 36
#define test_tx_idxcnt 36