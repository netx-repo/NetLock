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

static void get_traces(uint32_t lcore_id, uint32_t **txn_id, uint32_t **action_type, uint32_t **target_lm_id,
                uint32_t **target_obj_idx, uint32_t **lock_type, uint32_t* len) {
    FILE *fin, *fp;
    char filename[200];
    char buffer[200];
    char command[200];
    int line_c;

    // get filename
    if (benchmark == MICROBENCHMARK_SHARED) {
        sprintf(filename, "../../traces/microbenchmark/shared/micro_bm_s%d.csv", machine_id);
    }
    else if (benchmark == MICROBENCHMARK_EXCLUSIVE) {
        if (contention_degree == 1) // no contention
            sprintf(filename, "../../traces/microbenchmark/contention/queue_size_2/micro_bm_x%d_cd%d_lc%d.csv", machine_id, contention_degree, lcore_id+6);
        else if (contention_degree == 12) // contention
            sprintf(filename, "../../traces/microbenchmark/contention_shuffle/lk%d/micro_bm_x%d_t%d_lk%d.csv", lock_num, machine_id, lcore_id, lock_num); // client_id based
    } 
    else if (benchmark == TPCC_UNIFORMBENCHMARK) {
        // sprintf(filename, "../../tpcc_traces/1_24_2019/10v2_w%d_300s/trace_%d.csv", warehouse, (machine_id - 1) * (n_lcores - n_rcv_cores) + lcore_id + 9);
        // * change@June 26 2020 noTableLock (use less traces)
        sprintf(filename, "../../traces/tpcc_traces/10_14_2019/noTableLock_10v2_w%d_60s/trace_%d.csv", warehouse, (machine_id - 1) * (n_lcores - n_rcv_cores) + lcore_id + 9);
    }
    else if (benchmark == TPCCBENCHMARK) {
        // * TODO: change the filename
        if (task_id == 'p') {
            
            sprintf(filename, "../../traces/tpcc_traces/10_14_2019/noTableLock_10v2_w%d_60s/trace_%d.csv", warehouse, (machine_id - 1) * (n_lcores - n_rcv_cores) + lcore_id + 9);
        }
        else if (task_id == 'q') {
            
            sprintf(filename, "../../traces/tpcc_traces/10_14_2019/noTableLock_6v6_w%d_60s/trace_%d.csv", warehouse, (machine_id - 1) * (n_lcores - n_rcv_cores) + lcore_id + 25);
        }
        else {
            // sprintf(filename, "../../tpcc_traces/1_24_2019/10v2_w%d_300s/trace_%d.csv", warehouse, (machine_id - 1) * (n_lcores - n_rcv_cores) + lcore_id + 9);
            sprintf(filename, "../../traces/tpcc_traces/10_14_2019/noTableLock_10v2_w%d_60s/trace_%d.csv", warehouse, (machine_id - 1) * (n_lcores - n_rcv_cores) + lcore_id + 9);
        }
    }

    // get the line numbers
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
    line_c = line_c - 2;

    // get the traces
    fin = fopen(filename, "r");
    if (fgets(buffer, 100, fin) == NULL) {
        fprintf(stderr, "FILE ERROR\n");
        return;
    }
    if (fgets(buffer, 100, fin) == NULL) {
        fprintf(stderr, "FILE ERROR\n");
        return ;
    }

    (*len) = 0;
    (*txn_id) = (uint32_t *) malloc(sizeof(uint32_t) * (line_c + 1));
    (*action_type) = (uint32_t *) malloc(sizeof(uint32_t) * (line_c + 1));
    (*target_lm_id) = (uint32_t *) malloc(sizeof(uint32_t) * (line_c + 1));
    (*target_obj_idx) = (uint32_t *) malloc(sizeof(uint32_t) * (line_c + 1));
    (*lock_type) = (uint32_t *) malloc(sizeof(uint32_t) * (line_c + 1));
    int count_a = 0;
    int count_r = 0;

   
    uint32_t c_txn_id, c_action_type, c_target_lm_id, c_target_obj_idx, c_lock_type;
    int same_lock_flag;
    int backward_i;
    int j;
    for (int i=0; i<line_c; i++) {
        
        same_lock_flag = 0;
        if (fscanf(fin, "%d,%d,%d,%d,%d\n", &c_txn_id, &c_action_type, &c_target_lm_id,
               &c_target_obj_idx, &c_lock_type) == EOF) {
            fprintf(stderr, "FILE ERROR\n");
            return ;
        }
        if (c_action_type == 1) {
            continue;
        }
        if (benchmark == TPCC_UNIFORMBENCHMARK) {
            c_target_obj_idx = (rand() % 7000000) + 1;
        }
        else {
            if (task_id == 'g')
                c_target_obj_idx = batch_map[c_target_obj_idx];
            c_target_obj_idx ++;
        }
        
        backward_i = (*len) - 1;
        while ((backward_i >= 0) && ((*txn_id)[backward_i] == c_txn_id)) {
            if ((*target_obj_idx)[backward_i] == c_target_obj_idx) {
                if (task_id == 'g') {
                    if ((c_lock_type == 2) || (c_lock_type == 9))
                        (*lock_type)[backward_i] = 1;
                }
                same_lock_flag = 1;
                break;
            }
            backward_i --;
        }
        if (same_lock_flag == 1)
            continue;
        j = (*len);
        (*txn_id)[j] = c_txn_id;
        (*action_type)[j] = c_action_type;
        (*target_lm_id)[j] = c_target_lm_id;
        (*target_obj_idx)[j] = c_target_obj_idx;

        (*lock_type)[j] = c_lock_type;


        
        // (*target_obj_idx)[i] ++;

        if (((*lock_type)[j] == 7) || ((*lock_type)[j] == 8) || ((*lock_type)[j] == 1)) {
            (*lock_type)[j] = 0;
        }
        else if (((*lock_type)[j] == 9) || ((*lock_type)[j] == 2)) {
            (*lock_type)[j] = 1;
        }

        if ((*action_type)[j] == 0) {
            locks_each_txn[lcore_id][(*txn_id)[j]] ++;
            count_a ++;
        }
        else if ((*action_type)[j] == 1) {
            count_r ++;
        }
        (*len) ++;

    }

    fclose(fin);
    fprintf(stderr, "count_a: %d, count_r: %d\n", count_a, count_r);
}

#endif