// tpc-c
#define DEBUG
#ifndef TPCC 
#define TPCC
#define ITEM_PER_WARE 100000
#define DIST_PER_WARE 10
#define CUST_PER_DIST 3000
#define ROLL_BACK 0

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "new_order.h"

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
// * C is a run-time constant randomly chosen within [0 .. A] that can be varied without altering performance. The same C value, per field (C_LAST, C_ID, and OL_I_ID), must be used by all emulated terminals.
// * NURand(A,x,y)=(((random(0,A)| random(x,y))+C)%(y-x+1))+x

// * 1: SHARED; 2: EXCLUSIVE; 8:TABLE_LOCK_SHARED; 9: TABLE_LOCK_EXCLUSIVE.

static uint32_t get_random(uint32_t x, uint32_t y) {
    return ((rand() % (y - x + 1)) + x);
}

static uint32_t NURand(uint32_t A, uint32_t x, uint32_t y, uint32_t C) {
    return ((((get_random(0,A) | get_random(x,y)) + C) % (y-x+1)) + x);
}

static void get_batch_map(uint32_t *map) {
    FILE *fin, *fp;
    char filename[200];
    char command[200];
    int line_c = 0, a,b;
    sprintf(filename, "../batch/batch_%d.in", batch_size);
    sprintf(command, "wc -l %s | awk -F '[ ]'+ '{print $1}'", filename);
    DEBUG_PRINT("command: %s\n", command);
    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run command\n");
        return;
    }
    if (fscanf(fp, "%d", &line_c) == EOF) {
        fprintf(stderr, "RUN SHELL ERROR\n");
        return;
    }
    fclose(fp);

    fin = fopen(filename, "r");
    for (int i=0; i<line_c; i++) {
        if (fscanf(fin, "%d,%d\n", &a, &b) == EOF) {
            fprintf(stderr, "READ MAP FILE ERROR\n");
            return;
        }
        map[a] = b;
    }
    fclose(fin);
    return;
}

#endif