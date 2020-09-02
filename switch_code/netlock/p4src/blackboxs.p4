blackbox stateful_alu get_left_bound_alu {
    reg: left_bound_register;

    output_value: register_lo;
    output_dst  : meta.left;
}

blackbox stateful_alu get_right_bound_alu {
    reg: right_bound_register;

    output_value: register_lo;
    output_dst  : meta.right;
}

blackbox stateful_alu get_tail_alu {
    reg: tail_register;

    output_value: register_lo;
    output_dst  : meta.tail;
}

blackbox stateful_alu get_head_alu {
    reg: head_register;

    output_value: register_lo;
    output_dst  : meta.head;
}

blackbox stateful_alu get_tid_alu {
    reg: tid_array_register;

    output_value: register_lo;
    output_dst  : current_node_meta.tid;
   // output_dst  : nlk_hdr.tid;
}

blackbox stateful_alu get_mode_alu {
    reg: mode_array_register;

    output_value: register_lo;
    output_dst  : current_node_meta.mode;
   // output_dst  : nlk_hdr.mode;
}

blackbox stateful_alu get_ip_alu {
    reg: ip_array_register;

    output_value: register_lo;
    output_dst  : current_node_meta.ip_address;
}

blackbox stateful_alu get_client_id_alu {
    reg: client_id_array_register;

    output_value: register_lo;
    output_dst  : current_node_meta.client_id;
}

blackbox stateful_alu get_timestamp_hi_alu {
    reg: timestamp_hi_array_register;

    output_value: register_lo;
    output_dst  : current_node_meta.timestamp_hi;
}

blackbox stateful_alu get_timestamp_lo_alu {
    reg: timestamp_lo_array_register;

    output_value: register_lo;
    output_dst  : current_node_meta.timestamp_lo;
}

blackbox stateful_alu dec_empty_slots_alu {
    reg: slots_two_sides_register;

    condition_lo: register_lo > 0;
    //condition_hi: (register_hi > 0) or (meta.queue_size_op == SHRINK);
    // ** (register_hi > 0) or (meta.queue_size_op == SHRINK)
    condition_hi: register_hi + meta.queue_size_op > 0;

    update_lo_1_predicate: (condition_lo) and (not condition_hi);
    update_lo_1_value    : register_lo - 1;
    update_lo_2_predicate: (not condition_lo) or (condition_hi);
    update_lo_2_value    : register_lo;

    update_hi_1_predicate: (condition_lo) and (not condition_hi);
    update_hi_1_value    : register_hi;
    update_hi_2_predicate: (not condition_lo) or (condition_hi);
    update_hi_2_value    : register_hi + 1;

    output_value: alu_hi;
    output_dst  : meta.length_in_server;
}

blackbox stateful_alu push_back_alu {
    reg: slots_two_sides_register;

    update_lo_1_value    : register_lo - 1;
    update_hi_1_value    : register_hi - 1;
}

blackbox stateful_alu inc_length_in_server_alu {
    reg: slots_two_sides_register;

    update_hi_1_value    : register_hi + 1;

    output_value: alu_hi;
    output_dst  : meta.length_in_server;
}
/*
blackbox stateful_alu inc_empty_slots_alu {
    reg: empty_slots_register;

    update_lo_1_value: register_lo + 1;
}*/

/*
blackbox stateful_alu inc_empty_slots_alu {
    reg: slots_two_sides_register;

    // ** empty_slot < (right-left + 1)
    condition_lo: register_lo < meta.size_of_queue + 1;
    condition_hi: register_hi < 0;
    //condition_lo: register_lo < 1;

    update_lo_1_predicate: condition_lo;     // ** queue not empty, switch pop
    update_lo_1_value    : register_lo + 1;
    update_lo_2_predicate: not condition_lo; // ** queue empty, forward to server
    update_lo_2_value    : register_lo;

    update_hi_1_predicate: condition_lo;     // ** queue not empty, switch pop
    update_hi_1_value    : register_hi;
    update_hi_2_predicate: not condition_lo; // ** queue empty, forward to server
    // ** temporally set to 0, then set by the packet from the server (will push more packets to the queue)
    update_hi_2_value    : register_hi - 1;
    //update_hi_2_value    : register_hi - 1;

    output_value: register_lo;
    output_dst  : meta.queue_not_empty;
}
*/
blackbox stateful_alu inc_empty_slots_alu {
    reg: slots_two_sides_register;

    condition_hi: register_hi > 0;
    // ** empty_slot 
    update_lo_1_value   : register_lo + 1;
    update_hi_1_value   : register_hi;

    //output_value: register_lo;
    //output_dst  : meta.empty_slots_before_pop;
    output_predicate: condition_hi;
    output_value: register_lo;
    output_dst  : meta.empty_slots_before_pop;
}

blackbox stateful_alu modify_empty_slots_alu {
    reg: slots_two_sides_register;

    update_lo_1_value: meta.empty_slots + 1;
    update_hi_1_value: register_hi;
}

blackbox stateful_alu update_tail_alu {
    reg: tail_register;

    condition_lo: register_lo == meta.right;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value    : meta.left;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value    : register_lo + 1;

    output_value: register_lo;
    output_dst  : meta.tail;
}

blackbox stateful_alu update_head_alu {
    reg: head_register;

    condition_lo: register_lo == meta.right;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value    : meta.left;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value    : register_lo + 1;

    output_value: register_lo;
    output_dst  : meta.head;
}

blackbox stateful_alu update_mode_array_alu {
    reg: mode_array_register;

    update_lo_1_value: nlk_hdr.mode;
}

blackbox stateful_alu update_ip_array_alu {
    reg: ip_array_register;

    update_lo_1_value: ipv4.srcAddr;
}

blackbox stateful_alu update_client_id_array_alu {
    reg: client_id_array_register;

    update_lo_1_value: nlk_hdr.client_id;
}

blackbox stateful_alu update_tid_array_alu {
    reg: tid_array_register;

    update_lo_1_value: nlk_hdr.tid;
}

blackbox stateful_alu update_timestamp_hi_array_alu {
    reg: timestamp_hi_array_register;

    // update_lo_1_value: nlk_hdr.timestamp;
    update_lo_1_value: meta.ts_hi;
}

blackbox stateful_alu update_timestamp_lo_array_alu {
    reg: timestamp_lo_array_register;

    // update_lo_1_value: nlk_hdr.timestamp;
    update_lo_1_value: meta.ts_lo;
}

blackbox stateful_alu acquire_shared_lock_alu {
    reg: shared_and_exclusive_count_register;

    condition_lo: register_lo > 0;
    //condition_hi: 1 == 0;

    update_lo_1_value    : register_lo;

    update_hi_1_predicate: condition_lo;
    update_hi_1_value    : register_hi + 1;
    update_hi_2_predicate: not condition_lo;
    update_hi_2_value    : register_hi + 1;

   // output_value: combined_predicate;
   // output_value: register_lo;
    output_predicate : condition_lo;
    output_value: alu_hi;
    output_dst  : meta.locked;
}

blackbox stateful_alu acquire_exclusive_lock_alu {
    reg: shared_and_exclusive_count_register;

    condition_lo: register_lo > 0;
    condition_hi: register_hi > 0;

    update_lo_1_predicate: (not condition_lo) and (not condition_hi);
    update_lo_1_value    : register_lo + 1;
    update_lo_2_predicate: condition_lo or condition_hi;
    update_lo_2_value    : register_lo + 1;
    //update_lo_2_value    : register_lo;

    update_hi_1_value    : register_hi;


   // output_value: combined_predicate;
   // output_value: register_lo;
    output_predicate : condition_lo or condition_hi;
    output_value: alu_lo;
    output_dst  : meta.locked;
}

blackbox stateful_alu update_lock_alu {
    reg: shared_and_exclusive_count_register;

   // condition_lo: current_node_meta.mode == EXCLUSIVE_LOCK;
   // condition_hi: current_node_meta.mode == SHARED_LOCK;
    condition_lo: nlk_hdr.mode == EXCLUSIVE_LOCK;
    condition_hi: nlk_hdr.mode == SHARED_LOCK;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value    : register_lo - 1;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value    : register_lo;

    update_hi_1_predicate: condition_hi;
    update_hi_1_value    : register_hi - 1;
    update_hi_2_predicate: not condition_hi;
    update_hi_2_value    : register_hi;
}

blackbox stateful_alu modify_left_alu {
    reg: left_bound_register;

    update_lo_1_value: adm_hdr.new_left;

    output_value: alu_lo;
    output_dst: meta.left;
}

blackbox stateful_alu modify_right_alu {
    reg: right_bound_register;

    update_lo_1_value: adm_hdr.new_right;
}

blackbox stateful_alu modify_head_alu {
    reg: head_register;

    update_lo_1_value: meta.left;
}

blackbox stateful_alu modify_tail_alu {
    reg: tail_register;

    update_lo_1_value: meta.left;
}

blackbox stateful_alu get_queue_size_op_alu {
    reg: queue_size_op_register;

    output_value: register_lo;
    output_dst  : meta.queue_size_op;
}

blackbox stateful_alu set_queue_size_op_alu {
    reg: queue_size_op_register;

    update_lo_1_value   : set_bit;
}

blackbox stateful_alu clr_queue_size_op_alu {
    reg: queue_size_op_register;

    update_lo_1_value   : clr_bit;
}

blackbox stateful_alu tenant_count_alu {
    reg: tenant_acq_counter_register;

    condition_lo: register_lo < meta.tenant_threshold;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value    : register_lo + 1;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value    : register_lo;

    update_hi_1_predicate: condition_lo;
    update_hi_1_value    : 1;
    update_hi_2_predicate: not condition_hi;
    update_hi_2_value    : 0;

    output_value: alu_hi;
    output_dst  : meta.under_threshold;
}

blackbox stateful_alu check_failure_status_alu {
    reg: failure_status_register;

    output_value: register_lo;
    output_dst  : meta.failure_status;
}