void collect_results() {
    char filename_res[200], filename_lt[200], filename_txn_lt[200];
    char dirname_1[200], dirname_2[200], dirname_3[200];
    FILE *fout;
    char cmd[100];
    int i, j;

    uint64_t latency_total=0, num_total=0;
    for (i=0;i<n_rcv_cores;i++) {
        latency_total = latency_total + latency_stat_c[i].total - latency_stat_avg[i].total;
        num_total = num_total + latency_stat_c[i].num - latency_stat_avg[i].num;
    }
    printf("Average latency: %.4f ms\n", (latency_total - num_total) / (double) (num_total) / 1000.0);
    printf("Average latency: %.4f ms\n", (latency_total) / (double) (num_total) / 1000.0);

    fflush(stdout);

    

    // Sort latency_samples
    for (int lc = 0; lc < n_rcv_cores; lc++) {
        for (i = 0; i < latency_sample_num[lc]; ++i) {
            latency_samples_together[latency_samples_together_len] = latency_samples[lc][i];
            latency_samples_together_len ++;
        }
    }
    for (i = 0; i < latency_samples_together_len; i++) {
        uint64_t lat = latency_samples_together[i];
        j = i;
        while (j > 0 && latency_samples_together[j - 1] > lat) {
            latency_samples_together[j] = latency_samples_together[j - 1];
            j --;
        }
        latency_samples_together[j] = lat;
    }

    // ** quicksort
    //qsort(latency_samples_together, latency_samples_together_len, sizeof(uint64_t), cmpfunc);
    
    // Print median
    printf("Median latency: %.4f ms\n", latency_samples_together[latency_samples_together_len / 2] / 1000.0);
    fflush(stdout);


    /*
     * fig 1. ../results/shared/cn%dlk%d/ 's'
     * fig 2. ../results/exclusive/cn%dlk%d/ 'x'
     * fig 3,4. ../results/contention/cn%dlk%d/ 'c'
     * fig 5,6. ../results/mem_manage/(binpack, random)/cn%dlk%d/ 'm'
     * fig 7. ../results/mem_size/(zipf, uniform)/cn%dlk%d/ 'S'
     * fig 8. ../results/think_time/cn%dlk%d/ 't'
     */
    if (task_id == 's') {
        sprintf(dirname_1, "../results/shared");
        sprintf(dirname_2, "../results/shared/cn_%d_lk_%d", client_num, lock_num);
        sprintf(dirname_3, "../results/shared/cn_%d_lk_%d", client_num, lock_num);
        sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
        sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
    } else if (task_id == 'x') {
        sprintf(dirname_1, "../results/exclusive");
        sprintf(dirname_2, "../results/exclusive/cn_%d_lk_%d", client_num, lock_num);
        sprintf(dirname_3, "../results/exclusive/cn_%d_lk_%d", client_num, lock_num);
        sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
        sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
    } else if (task_id == 'c') {
        sprintf(dirname_1, "../results/contention");
        sprintf(dirname_2, "../results/contention/cn_%d_lk_%d", client_num, lock_num);
        sprintf(dirname_3, "../results/contention/cn_%d_lk_%d", client_num, lock_num);
        sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
        sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
    } else if (task_id == 'm') {
        sprintf(dirname_1, "../results/mem_manage");
        if (memory_management == MEM_BIN_PACK) {
            sprintf(dirname_2, "../results/mem_manage/binpack");
            sprintf(dirname_3, "../results/mem_manage/binpack/cn_%d_lk_%d_sn_%d", client_num, lock_num, slot_num);
        }
        else if (memory_management == MEM_RAND_WEIGHT) {
            sprintf(dirname_2, "../results/mem_manage/random");
            sprintf(dirname_3, "../results/mem_manage/random/cn_%d_lk_%d_sn_%d", client_num, lock_num, slot_num);
        }
        sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
        sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
        sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
    } else if (task_id == 'S') {
        sprintf(dirname_1, "../results/mem_size");
        if (benchmark == ZIPFBENCHAMRK) {
            sprintf(dirname_2, "../results/mem_size/zipf");
            sprintf(dirname_3, "../results/mem_size/zipf/cn_%d_sn_%d", client_num, slot_num);
        }
        else if (benchmark == UNIFORMBENCHMARK) {
            sprintf(dirname_2, "../results/mem_size/uniform");
            sprintf(dirname_3, "../results/mem_size/uniform/cn_%d_sn_%d", client_num, slot_num);
        }
        else if (benchmark == TPCC_UNIFORMBENCHMARK) {
            sprintf(dirname_2, "../results/mem_size/tpcc_uniform");
            sprintf(dirname_3, "../results/mem_size/tpcc_uniform/cn_%d_sn_%d", client_num, slot_num);
        }
        sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
        sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
    } else if (task_id == 't') {
        sprintf(dirname_1, "../results/think_time");
        sprintf(dirname_2, "../results/think_time/cn_%d_sn_%d_tt_%lu", client_num, slot_num, think_time);
        sprintf(dirname_3, "../results/think_time/cn_%d_sn_%d_tt_%lu", client_num, slot_num, think_time);
        sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
        sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
        sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
    } else if (task_id == 'p') {
        sprintf(dirname_1, "../results/tpcc");
        sprintf(dirname_2, "../results/tpcc/10v2");
        sprintf(dirname_3, "../results/tpcc/10v2/cn_%d_wh_%d", client_num, warehouse);
        sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
        sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
        sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
    } else if (task_id == 'q') {
        sprintf(dirname_1, "../results/tpcc");
        sprintf(dirname_2, "../results/tpcc/6v6/");
        sprintf(dirname_3, "../results/tpcc/6v6/cn_%d_wh_%d", client_num, warehouse);
        sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
        sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
        sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
    } else if (task_id == 'g') {
        sprintf(dirname_1, "../results/granularity");
        sprintf(dirname_2, "../results/granularity/sn_%d_bs_%d_to_%d/", slot_num, batch_size, timeout_slot);
        sprintf(dirname_3, "../results/granularity/sn_%d_bs_%d_to_%d/", slot_num, batch_size, timeout_slot);
        sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
        sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
        sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
    } else if (task_id == 'f') {
        sprintf(dirname_1, "../results/failover");
        sprintf(dirname_2, "../results/failover");
        sprintf(dirname_3, "../results/failover");
        sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
        sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
        sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
    } else if (task_id == 'e') {
        if (client_node_num == 10) {
            sprintf(dirname_1, "../results/camera_ready_10v2");
            sprintf(dirname_2, "../results/camera_ready_10v2/empty");
            sprintf(dirname_3, "../results/camera_ready_10v2/empty/core_%d_wh%d", num_of_cores, warehouse);
            sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
        }
        else {
            sprintf(dirname_1, "../results/camera_ready_6v6");
            sprintf(dirname_2, "../results/camera_ready_6v6/empty");
            sprintf(dirname_3, "../results/camera_ready_6v6/empty/core_%d_wh%d", num_of_cores, warehouse);
            sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
        }
    }
    else if ((task_id == '2') || (task_id == '3')) {
        if (client_node_num == 10) {
            sprintf(dirname_1, "../results/camera_ready_10v2");
            sprintf(dirname_2, "../results/camera_ready_10v2/netlock");
            sprintf(dirname_3, "../results/camera_ready_10v2/netlock/core_%d_wh%d", num_of_cores, warehouse);
            sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
        }
        else {
            sprintf(dirname_1, "../results/camera_ready_6v6");
            sprintf(dirname_2, "../results/camera_ready_6v6/netlock");
            sprintf(dirname_3, "../results/camera_ready_6v6/netlock/core_%d_wh%d", num_of_cores, warehouse);
            sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
            sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
        }
    }
    else {
        fprintf(stderr, "TASK ID WRONG!");
    }
    DEBUG_PRINT("%s\n", dirname_1);
    DEBUG_PRINT("%s\n", dirname_2);
    DEBUG_PRINT("%s\n", dirname_3);
    DEBUG_PRINT("%s\n", filename_res);
    DEBUG_PRINT("%s\n", filename_lt);

    // sprintf(cmd, "mkdir -p ../results/contention");
    sprintf(cmd, "mkdir -p %s", dirname_1);
    int sysRet = system(cmd);
    if (sysRet != 0) {
        fprintf(stderr, "Execute command ERROR!\n");
    }
    // sprintf(cmd, "mkdir -p ../results/contention/cn%d", client_num);
    sprintf(cmd, "mkdir -p %s", dirname_2);
    sysRet = system(cmd);
    if (sysRet != 0) {
        fprintf(stderr, "Execute command ERROR!\n");
    }
    // sprintf(cmd, "mkdir -p ../results/contention/cn%d/lk%d", client_num, lock_num);
    sprintf(cmd, "mkdir -p %s", dirname_3);
    sysRet = system(cmd);
    if (sysRet != 0) {
        fprintf(stderr, "Execute command ERROR!\n");
    }
    
    fout = fopen(filename_res, "w");

    fprintf(fout, "Total\n");
    fprintf(fout, "tx: %"PRIu64"\n", (tput_stat_total.tx - tput_stat_total.last_tx) / average_interval);
    fprintf(fout, "rx: %"PRIu64"\n", (tput_stat_total.rx - tput_stat_total.last_rx) / average_interval);
    fprintf(fout, "rx_read: %"PRIu64"\n", (tput_stat_total.rx_read - tput_stat_total.last_rx_read) / average_interval);
    fprintf(fout, "rx_write: %"PRIu64"\n", (tput_stat_total.rx_write - tput_stat_total.last_rx_write) / average_interval);
    fprintf(fout, "txn_rate: %"PRIu64"\n", (tput_stat_total.txn - tput_stat_total.last_txn) / average_interval);
    //printf("dropped: %"PRIu64"\n", (tput_stat_avg.dropped - tput_stat_avg.last_dropped) / average_interval);
    //fprintf(fout, "Core 0 Average latency: %.4f ms\n", (latency_stat_c[0].total - latency_stat_avg[0].total) / (double) (latency_stat_c[0].num - latency_stat_avg[0].num) / 1000.0);
    // fprintf(fout, "Average latency: %.4f ms\n", (latency_total - num_total) / (double) (num_total) / 1000.0);
    fprintf(fout, "Average latency: %.4f ms\n", (latency_total) / (double) (num_total) / 1000.0);
    fprintf(fout, "Median latency: %.4f ms\n", latency_samples_together[latency_samples_together_len / 2] / 1000.0);
    fprintf(fout, "99 percent tail latency: %.4f ms\n", latency_samples_together[latency_samples_together_len * 99/100 - 1] / 1000.0);
    fprintf(fout, "99.9 percent tail latency: %.4f ms\n", latency_samples_together[latency_samples_together_len * 999/1000 - 1] / 1000.0);
    fflush(fout);
    fclose(fout);

#ifdef COLLECT_LATENCY
    fout = fopen(filename_lt, "w");
    for (i = 0; i < latency_samples_together_len; i++) {
        uint64_t lat = latency_samples_together[i];
        fprintf(fout, "%lu\n", lat);
    }
    fflush(fout);
    fclose(fout);


    if ((task_id == 'p') || (task_id == 'q') || (benchmark == TPCCBENCHMARK) || (benchmark == TPCC_UNIFORMBENCHMARK)) {
        fout = fopen(filename_txn_lt, "w");
        for (i=0; i<client_num; i++) {
            for (int j=0; j<MAX_TXN_NUM; j++) {
                if ((txn_begin_time[i][j] != 0) && (txn_finish_time[i][j] != 0)) {
                    uint64_t lat = timediff_in_us(txn_finish_time[i][j], txn_begin_time[i][j]);
                    fprintf(fout, "%lu\n", lat);
                }
            }
        }
        fflush(fout);
        fclose(fout);
    }
#endif
}