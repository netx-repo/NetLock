//  #### actions
action decode_action() {
    // get_queue_size_op_alu.execute_stateful_alu(nlk_hdr.lock);
    get_queue_size_op_alu.execute_stateful_alu(meta.lock_id);
    modify_field(meta.ts_hi, nlk_hdr.timestamp_hi);
    modify_field(meta.ts_lo, nlk_hdr.timestamp_lo);
    // shift_right(meta.ts_hi, nlk_hdr.timestamp, 32);
    // bit_and(meta.ts_lo, nlk_hdr.timestamp, 4294967295);
}

action forward_to_server_action(server_ip) {
    //  ## redirect the packet to server  ##
    //modify_field(nlk_hdr.op, FORWARD_LOCK);
    // modify_field(ipv4.srcAddr, DEDICATED_SERVER_IP);
    // modify_field(ipv4.dstAddr, DEDICATED_SERVER_IP);
    modify_field(ipv4.srcAddr, server_ip);
    modify_field(ipv4.dstAddr, server_ip);
}

action get_left_bound_action() {
    // get_left_bound_alu.execute_stateful_alu(nlk_hdr.lock);
    get_left_bound_alu.execute_stateful_alu(meta.lock_id);
}

action get_right_bound_action() {
    // get_right_bound_alu.execute_stateful_alu(nlk_hdr.lock);
    get_right_bound_alu.execute_stateful_alu(meta.lock_id);
}

action update_tail_action() {
    // update_tail_alu.execute_stateful_alu(nlk_hdr.lock);
    update_tail_alu.execute_stateful_alu(meta.lock_id);
}

action update_mode_array_action() {
    update_mode_array_alu.execute_stateful_alu(meta.tail);
}

action update_ip_array_action() {
    update_ip_array_alu.execute_stateful_alu(meta.tail);
}

action update_tid_array_action() {
    update_tid_array_alu.execute_stateful_alu(meta.tail);
}

action acquire_shared_lock_action() {
    // acquire_shared_lock_alu.execute_stateful_alu(nlk_hdr.lock);
    acquire_shared_lock_alu.execute_stateful_alu(meta.lock_id);
}

action acquire_exclusive_lock_action() {
    // acquire_exclusive_lock_alu.execute_stateful_alu(nlk_hdr.lock);
    acquire_exclusive_lock_alu.execute_stateful_alu(meta.lock_id);
}

action notify_tail_client_action() {
    modify_field(ipv4.dstAddr, ipv4.srcAddr);
   // modify_field(udp.dstPort, REPLY_PORT);
}

action notify_tail_client_1_action() {
    /*
     * Not using
     */
    //  ## notify the client that has obtained the lock  ##
    modify_field(meta.src_ip, ipv4.srcAddr);
    modify_field(meta.dst_ip, ipv4.dstAddr);
    modify_field(udp.dstPort, REPLY_PORT);
   // modify_field(udp.dstPort, LK_PORT);
}

action notify_tail_client_2_action() {
    /*
     * Not using
     */
    modify_field(ipv4.srcAddr, meta.dst_ip);
    modify_field(ipv4.dstAddr, meta.src_ip);
}

action i2e_mirror_action(mirror_id) {
    //  ## notify the client that has obtained the lock  ##
    modify_field(current_node_meta.clone_md, 1);
    
#if __TARGET_TOFINO__ == 2
    modify_field(ig_intr_md_for_mb.mirror_hash, 2);
    modify_field(ig_intr_md_for_mb.mirror_multicast_ctrl, 0);
    modify_field(ig_intr_md_for_mb.mirror_io_select, 0);
#endif
    clone_ingress_pkt_to_egress(mirror_id, nl_i2e_mirror_info);
}


// ## 

action update_head_action() {
    // update_head_alu.execute_stateful_alu(nlk_hdr.lock);
    update_head_alu.execute_stateful_alu(meta.lock_id);
}

action get_tail_action() {
    // get_tail_alu.execute_stateful_alu(nlk_hdr.lock);
    get_tail_alu.execute_stateful_alu(meta.lock_id);
}

action get_head_action() {
    // get_head_alu.execute_stateful_alu(nlk_hdr.lock);
    get_head_alu.execute_stateful_alu(meta.lock_id);
}

action metahead_plus_1_action() {
    modify_field(meta.head, meta.left);
}

action metahead_plus_2_action() {
    add_to_field(meta.head, 1);
}

action get_tid_action() {
    get_tid_alu.execute_stateful_alu(meta.head);
}

action get_mode_action() {
    get_mode_alu.execute_stateful_alu(meta.head);
}

action get_ip_action() {
    get_ip_alu.execute_stateful_alu(meta.head);
}

action update_lock_action() {
    // update_lock_alu.execute_stateful_alu(nlk_hdr.lock);
    update_lock_alu.execute_stateful_alu(meta.lock_id);
}

action notify_controller_action() {
    // ## notify the controller when the queue is empty  ####
    modify_field(ig_intr_md_for_tm.copy_to_cpu, 1);
    _drop();
   // modify_field(ipv4.dstAddr, CONTROLLER_IP);
}



// ## modify_bound
action modify_left_action() {
    modify_left_alu.execute_stateful_alu(adm_hdr.lock);
}

action modify_right_action() {
    modify_right_alu.execute_stateful_alu(adm_hdr.lock);
}

action modify_head_action() {
    modify_head_alu.execute_stateful_alu(adm_hdr.lock);
}

action modify_tail_action() {
    modify_tail_alu.execute_stateful_alu(adm_hdr.lock);
}

// ## set_shrink
action set_shrink_action() {
    set_queue_size_op_alu.execute_stateful_alu(adm_hdr.lock);
}

action shrink_finish_action() {
    clr_queue_size_op_alu.execute_stateful_alu(adm_hdr.lock);
}

// ## resubmit
action mark_to_resubmit_action() {
   // modify_field(resubmit_meta.resubmit_flag, 1);
   // modify_field(resubmit_meta.dequeued_mode, current_node_meta.mode);
   // modify_field(meta.do_resubmit, 1);

    modify_field(nlk_hdr.recirc_flag, 1);
    add_header(recirculate_hdr);
    modify_field(recirculate_hdr.cur_tail, meta.tail);
    modify_field(recirculate_hdr.cur_head, meta.head);
    modify_field(recirculate_hdr.dequeued_mode, current_node_meta.mode);
    modify_field(meta.do_resubmit, 1);
}

action resubmit_action() {
   // resubmit(redirect_FL);
    
    recirculate(68);
    modify_field(meta.recirced, 1);
}

action mark_to_resubmit_2_action() {
    modify_field(nlk_hdr.recirc_flag, 2);
    modify_field(recirculate_hdr.cur_tail, meta.tail);
    modify_field(recirculate_hdr.cur_head, meta.head);
    modify_field(meta.do_resubmit, 1);
}

action notify_head_client_action() {
    //modify_field(nlk_hdr.mode, current_node_meta.mode);
    //modify_field(ipv4.srcAddr, DEDICATED_SERVER_IP);
    modify_field(ipv4.dstAddr, current_node_meta.ip_address);
}

action get_recirc_info_action() {
    modify_field(meta.tail, recirculate_hdr.cur_tail);
    modify_field(meta.head, recirculate_hdr.cur_head);
    modify_field(meta.recirc_flag, nlk_hdr.recirc_flag);
    modify_field(meta.dequeued_mode, recirculate_hdr.dequeued_mode);
    //modify_field(meta.queue_not_empty, 1);
}

action inc_empty_slots_action() {
    // inc_empty_slots_alu.execute_stateful_alu(nlk_hdr.lock);
    inc_empty_slots_alu.execute_stateful_alu(meta.lock_id);
}

action dec_empty_slots_action() {
    // dec_empty_slots_alu.execute_stateful_alu(nlk_hdr.lock);
    dec_empty_slots_alu.execute_stateful_alu(meta.lock_id);
}

action push_back_action() {
    // push_back_alu.execute_stateful_alu(nlk_hdr.lock);
    push_back_alu.execute_stateful_alu(meta.lock_id);
}

action inc_length_in_server_action() {
    // inc_length_in_server_alu.execute_stateful_alu(nlk_hdr.lock);
    inc_length_in_server_alu.execute_stateful_alu(meta.lock_id);
}

action modify_empty_slots_action() {
    modify_empty_slots_alu.execute_stateful_alu(adm_hdr.lock);
}

action get_length_action() {
    subtract(meta.empty_slots, adm_hdr.new_right, adm_hdr.new_left);
}



action get_size_of_queue_action() {
    subtract(meta.size_of_queue, meta.right, meta.left);
}

action update_timestamp_hi_array_action() {
    update_timestamp_hi_array_alu.execute_stateful_alu(meta.tail);
}

action update_timestamp_lo_array_action() {
    update_timestamp_lo_array_alu.execute_stateful_alu(meta.tail);
}

action get_timestamp_hi_action() {
    get_timestamp_hi_alu.execute_stateful_alu(meta.head);
}

action get_timestamp_lo_action() {
    get_timestamp_lo_alu.execute_stateful_alu(meta.head);
}

action update_client_id_array_action() {
    update_client_id_array_alu.execute_stateful_alu(meta.tail);
}

action get_client_id_action() {
    get_client_id_alu.execute_stateful_alu(meta.head);
}

action set_as_primary_action() {
    modify_field(ethernet.dstAddr, PRIMARY_BACKUP);
}

action set_as_secondary_action() {
    modify_field(ethernet.dstAddr, SECONDARY_BACKUP);
}

action check_lock_exist_action(index) {
    modify_field(meta.lock_exist, 1);
    modify_field(meta.lock_id, index);
}

action change_mode_act(udp_src_port) {
    modify_field(nlk_hdr.mode, current_node_meta.mode);
    modify_field(nlk_hdr.tid, current_node_meta.tid);
    modify_field(nlk_hdr.timestamp_lo, current_node_meta.timestamp_lo);
    modify_field(nlk_hdr.timestamp_hi, current_node_meta.timestamp_hi);
    modify_field(nlk_hdr.client_id, current_node_meta.client_id);
    modify_field(ipv4.dstAddr, current_node_meta.ip_address);
    modify_field(ipv4.srcAddr, current_node_meta.ip_address);
    modify_field(udp.srcPort, udp_src_port);
    modify_field(udp.dstPort, LK_PORT);
}

action fix_src_port_action(fix_port) {
    modify_field(ethernet.srcAddr, ipv4.srcAddr);
    modify_field(udp.srcPort, fix_port);    
}

action set_as_failure_notification_action() {
    modify_field(ethernet.dstAddr, FAILURE_NOTIFICATION);
}

action check_failure_status_action() {
    check_failure_status_alu.execute_stateful_alu(0);
}

action set_failure_status_action() {
    modify_field(probe_hdr.failure_status, meta.failure_status);
}
