//  #### tables
@pragma stage 1
table decode_table {
    actions {
        decode_action;
    }
    default_action: decode_action;
}

@pragma stage 1
table shrink_finish_table {
    actions {
        shrink_finish_action;
    }
    default_action: shrink_finish_action;
}

@pragma stage 0
table get_length_table {
    actions {
        get_length_action;
    }
    default_action: get_length_action;
}

@pragma stage 1
table set_shrink_table {
    actions {
        set_shrink_action;
    }
    default_action: set_shrink_action;
}



@pragma stage 1
table get_left_bound_table {
    actions {
        get_left_bound_action;
    }
    default_action: get_left_bound_action;
}

@pragma stage 1
table get_right_bound_table {
    actions {
        get_right_bound_action;
    }
    default_action: get_right_bound_action;
}

@pragma stage 1
table modify_left_table {
    actions {
        modify_left_action;
    }
    default_action: modify_left_action;
}

@pragma stage 1
table modify_right_table {
    actions {
        modify_right_action;
    }
    default_action: modify_right_action;
}


@pragma stage 4
table update_tail_table {
    actions {
        update_tail_action;
    }
    default_action: update_tail_action;
}

@pragma stage 3
table update_head_table {
    actions {
        update_head_action;
    }
    default_action: update_head_action;
}

@pragma stage 2
table get_recirc_info_table {
    actions {
        get_recirc_info_action;
    }
    default_action: get_recirc_info_action;
}

@pragma stage 4
table get_tail_table {
    actions {
        get_tail_action;
    }
    default_action: get_tail_action;
}

@pragma stage 2
table inc_empty_slots_table {
    actions {
        inc_empty_slots_action;
    }
    default_action: inc_empty_slots_action;
}

@pragma stage 2
table dec_empty_slots_table {
    reads {
        nlk_hdr.op: exact;
    }
    actions {
        dec_empty_slots_action;
        push_back_action;
    }
}

@pragma stage 2
table modify_empty_slots_table {
    actions {
        modify_empty_slots_action;
    }
    default_action: modify_empty_slots_action;
}

@pragma stage 3
table modify_head_table {
    actions {
        modify_head_action;
    }
    default_action: modify_head_action;
}

@pragma stage 4
table modify_tail_table {
    actions {
        modify_tail_action;
    }
    default_action: modify_tail_action;
}

@pragma stage 5
table forward_to_server_table {
    reads {
        nlk_hdr.lock: ternary;
    }
    actions {
        forward_to_server_action;
    }
    default_action: forward_to_server_action;
}

@pragma stage 6
table update_tid_array_table {
    actions {
        update_tid_array_action;
    }
    default_action: update_tid_array_action;
}

@pragma stage 6
table get_tid_table {
    actions {
        get_tid_action;
    }
    default_action: get_tid_action;
}

@pragma stage 5
table update_mode_array_table {
    actions {
        update_mode_array_action;
    }
    default_action: update_mode_array_action;
}

@pragma stage 5
table get_mode_table {
    actions {
        get_mode_action;
    }
    default_action: get_mode_action;
}

@pragma stage 5
table update_ip_array_table {
    actions {
        update_ip_array_action;
    }
    default_action: update_ip_array_action;
}

@pragma stage 5
table get_ip_table {
    actions {
        get_ip_action;
    }
    default_action: get_ip_action;
}

@pragma stage 3
table acquire_lock_table {
    reads {
        nlk_hdr.mode: exact;
    }
    actions {
        acquire_shared_lock_action;
        acquire_exclusive_lock_action;
    }
}

@pragma stage 3
table update_lock_table {
    actions {
        update_lock_action;
    }
    default_action: update_lock_action;
}

@pragma stage 8
table notify_tail_client_table {
    actions {
        notify_tail_client_action;
    }
    default_action: notify_tail_client_action;
}

/*
@pragma stage 7
table notify_tail_client_1_table {
    actions {
        notify_tail_client_1_action;
    }
    default_action: notify_tail_client_1_action;
}

@pragma stage 8
table notify_tail_client_2_table {
    actions {
        notify_tail_client_2_action;
    }
    default_action: notify_tail_client_2_action;
}
*/

@pragma stage 8
table i2e_mirror_table {
    reads {
        ipv4.dstAddr: exact;
    }
    actions {
        i2e_mirror_action;
    }
}



//  ##

@pragma stage 8
table metahead_plus_1_table {
    actions {
        metahead_plus_1_action;
    }
    default_action: metahead_plus_1_action;
}
@pragma stage 8
table metahead_plus_2_table {
    actions {
        metahead_plus_2_action;
    }
    default_action: metahead_plus_2_action;
}

// ##

// ## resubmit
@pragma stage 9
table mark_to_resubmit_table {
    actions {
        mark_to_resubmit_action;
    }
    default_action: mark_to_resubmit_action;
}


@pragma stage 9
table mark_to_resubmit_2_table {
    actions {
        mark_to_resubmit_2_action;
    }
    default_action: mark_to_resubmit_2_action;
}

table resubmit_table {
    actions {
        resubmit_action;
    }
    default_action: resubmit_action;
}

@pragma stage 9
table notify_controller_table {
    actions {
        notify_controller_action;
    }
    default_action: notify_controller_action;
}

@pragma stage 6
table notify_head_client_table {
    actions {
        notify_head_client_action;
    }
    default_action: notify_head_client_action;
}

@pragma stage 10
table drop_packet_table {
    actions {
        _drop;
    }
    default_action: _drop;
}



@pragma stage 2
table get_size_of_queue_table {
    actions {
        get_size_of_queue_action;
    }
    default_action: get_size_of_queue_action;
}

@pragma stage 7
table update_timestamp_hi_array_table {
    actions {
        update_timestamp_hi_array_action;
    }
    default_action: update_timestamp_hi_array_action;
}

@pragma stage 8
table update_timestamp_lo_array_table {
    actions {
        update_timestamp_lo_array_action;
    }
    default_action: update_timestamp_lo_array_action;
}

@pragma stage 7
table get_timestamp_hi_table {
    actions {
        get_timestamp_hi_action;
    }
    default_action: get_timestamp_hi_action;
}

@pragma stage 8
table get_timestamp_lo_table {
    actions {
        get_timestamp_lo_action;
    }
    default_action: get_timestamp_lo_action;
}

@pragma stage 6
table update_client_id_array_table {
    actions {
        update_client_id_array_action;
    }
    default_action: update_client_id_array_action;
}

@pragma stage 6
table get_client_id_table {
    actions {
        get_client_id_action;
    }
    default_action: get_client_id_action;
}

// table set_as_secondary_table {
//     actions {
//         set_as_secondary_action;
//     }
//     default_action: set_as_secondary_action;
// }

// table set_as_primary_table {
//     actions {
//         set_as_primary_action;
//     }
//     default_action: set_as_primary_action;
// }
table set_tag_table {
    reads {
        meta.failure_status: exact;
        meta.lock_exist: exact;
    }
    actions {
        set_as_primary_action;
        set_as_secondary_action;
        set_as_failure_notification_action;
    }
}

@pragma stage 0
table check_lock_exist_table {
    reads {
        nlk_hdr.lock: exact;
    }
    actions {
        check_lock_exist_action;
    }
    size: NUM_LOCKS;
}

table change_mode_table {
    reads {
        current_node_meta.tid: ternary;
    }
    actions {
        change_mode_act;
    }
    default_action: change_mode_act;
}

table fix_src_port_table {
    reads {
        nlk_hdr.lock: ternary;
    }
    actions {
        fix_src_port_action;
    }
    //default_action:  fix_src_port_action;
}

@pragma stage 0
table check_failure_status_table {
    actions {
        check_failure_status_action;
    }
    default_action: check_failure_status_action;
}

@pragma stage 0
table check_failure_status_2_table {
    actions {
        check_failure_status_action;
    }
    default_action: check_failure_status_action;
}

table set_failure_status_table {
    actions {
        set_failure_status_action;
    }
    default_action: set_failure_status_action;
}
