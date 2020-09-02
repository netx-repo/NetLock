#include <tofino/constants.p4>
#if __TARGET_TOFINO__ == 2
#include <tofino2/intrinsic_metadata.p4>
#else
#include <tofino/intrinsic_metadata.p4>
#endif
#include <tofino/primitives.p4>
#include <tofino/stateful_alu_blackbox.p4>

#include "includes/defines.p4"
#include "includes/headers.p4"
#include "includes/parser.p4"
#include "routing.p4"
#include "blackboxs.p4"
#include "actions.p4"
#include "tables.p4"

//  #### metas
field_list nl_i2e_mirror_info {
    current_node_meta.mode;
    current_node_meta.tid;
    current_node_meta.clone_md;
    current_node_meta.ip_address;
    current_node_meta.timestamp_hi;
    current_node_meta.timestamp_lo;
    current_node_meta.client_id;
}

header_type meta_t {
    fields {
        new_left:       32;
        new_right:      32;
        head:           32;
        tail:           32;
        queue_size_op:  2;
        do_resubmit:    1;
        routed:         1;
        lock_exist:     1;
        recirc_flag:    2;
        dequeued_mode:  8;
        recirced:       2;
        locked:         32;
        left:           32;
        right:          32;
        src_ip:         32;
        dst_ip:         32;
        empty_slots:    32;
        length_in_server:32;
        size_of_queue:  32;
        empty_slots_before_pop:32;
        ts_hi:          32;
        ts_lo:          32;
        lock_id:        32;
        under_threshold: 1;
        tenant_id:      16;
        tenant_threshold:32;
        failure_status: 8;
    }
}
metadata meta_t meta;

header_type node_meta_t {
    fields {
        clone_md: 8;
        mode: 8;
        client_id: 8;
        tid: 32;
        ip_address: 32;
        timestamp_lo: 32;
        timestamp_hi: 32;
    }
}
metadata node_meta_t current_node_meta;

header_type resubmit_meta_t {
    fields {
        dequeued_mode: 8;
        resubmit_flag: 2;
    }
}
metadata resubmit_meta_t resubmit_meta;

field_list redirect_FL {
    resubmit_meta;
    meta.head;
}

header_type i2e_metadata_t {
    fields {
        ingress_tstamp    : 32;
        mirror_session_id : 16;
    }
}
metadata i2e_metadata_t i2e_metadata;

field_list clone_FL {
    i2e_metadata.ingress_tstamp;
    i2e_metadata.mirror_session_id;
}

//  #### registers
register head_register {
    width: 32;
    instance_count: NUM_LOCKS;
}

register tail_register {
    width: 32;
    instance_count: NUM_LOCKS;
}

register slots_two_sides_register {
    width: 64;
    instance_count: NUM_LOCKS;
}

register left_bound_register {
    width: 32;
    instance_count: NUM_LOCKS;
}

register right_bound_register {
    width: 32;
    instance_count: NUM_LOCKS;
}

register shared_and_exclusive_count_register {
    width: 64;
    instance_count: NUM_LOCKS;
}

register mode_array_register {
    width: 8;
    instance_count: LENGTH_ARRAY;
}

register tid_array_register {
    width: 32;
    instance_count: LENGTH_ARRAY;
}

register ip_array_register {
    width: 32;
    instance_count: LENGTH_ARRAY;
}

register client_id_array_register {
    width: 8;
    instance_count: LENGTH_ARRAY;
}

register timestamp_hi_array_register {
    width: 32;
    instance_count: LENGTH_ARRAY;
}

register timestamp_lo_array_register {
    width: 32;
    instance_count: LENGTH_ARRAY;
}

register queue_size_op_register {
    width: 1;
    instance_count: NUM_LOCKS;
}

register tenant_acq_counter_register {
    width: 64;
    instance_count: NUM_TENANTS;
}

register failure_status_register {
    width: 8;
    instance_count: 1;
}

//  #### control flows

control acquire_lock {
    // Stage 2: [register_lo: empty_slots_register]
    // ** change the number of empty slots for normal requests and push-back requests
    apply(dec_empty_slots_table);

    if ((meta.length_in_server != 0) and (nlk_hdr.op == ACQUIRE_LOCK)) {
        // * Forward and buffer the request to the server
        // Stage 4
        // ** fix src_port for RSS
        apply(fix_src_port_table);

        apply(set_tag_table);
        // Stage 5
        // ** if the queue is shrinking or is full, forward the request to server
        apply(forward_to_server_table);

    }
    else {
        // Stage 3: [register_lo: num_exclusive_lock; register_hi: num_shared_lock]
        // ** num_shared_lock ++ or num_exclusive_lock ++; set meta.locked
        apply(acquire_lock_table);

        // Stage 4: [register_lo: tail_register]
        // ** tail ++
        apply(update_tail_table);

        // Stage 5: [register_lo: ip_array_register]
        // ** enqueue(src_ip)
        apply(update_ip_array_table);

        // Stage 5: [register_lo: mode_array_register]
        // ** enqueue(mode)
        apply(update_mode_array_table);

        // Stage 6: [register_lo: client_id_array_register]
        // ** enqueue(client_id)
        apply(update_client_id_array_table);

        // Stage 6: [register_lo: tid_array_register]
        // ** enqueue(tid)
        apply(update_tid_array_table);

        // Stage 7: [register_lo: timestamp_hi_array_register]
        // ** enqueue(timestamp_hi)
        apply(update_timestamp_hi_array_table);

        // Stage 8: [register_lo: timestamp_lo_array_register]
        // ** enqueue(timestamp_lo)
        apply(update_timestamp_lo_array_table);

        if (meta.locked == 0) {
            // ** notify the client, grant the lock
            apply(notify_tail_client_table);
        }
        else {
            // ** drop the packet if locked
            apply(drop_packet_table);
        }
        
    }
    
}

control release_lock {
    
    if (nlk_hdr.recirc_flag == 0) {
        // Stage 2: [register_lo: empty_slots_register]
        // ** num_of_empty_slots ++
        apply(inc_empty_slots_table);
        // Stage 3: [register_lo: head_register]
        // ** head ++
        if (meta.empty_slots_before_pop != meta.size_of_queue) {
            apply(update_head_table);
            // Stage 3: [register_lo: num_exclusive_lock; register_hi: num_shared_lock]
            // ** num_shared_lock -- or num_exclusive_lock --
            apply(update_lock_table);
        }   
    }
    else {
        // Stage 2
        // ** it is a resubmit packet
        apply(get_recirc_info_table);
    }

    if ((meta.empty_slots_before_pop == meta.size_of_queue) and (nlk_hdr.recirc_flag == 0)) {
        // * If the switch queue is empty, get some back from the server queue
        // Stage 4
        // ** fix src_port for RSS
        apply(fix_src_port_table);

        apply(set_tag_table);
        // Stage 5
        // ** forward release request to server if the queue is empty
        apply(forward_to_server_table);

    }
    else {

        // Stage 4: [register_lo: tail_register]
        // ** get tail
        if (nlk_hdr.recirc_flag == 0)
            apply(get_tail_table);

        // Stage 5: [register_lo: ip_array_register]
        // ** get head node inf (src_ip)
        apply(get_ip_table);

        // Stage 5: [register_lo: mode_array_register]
        // ** get head node inf (mode)
        apply(get_mode_table);

        // Stage 6: [register_lo: client_id_array_register]
        // ** get head node inf (client_id)
        apply(get_client_id_table);

        // Stage 6: [register_lo: tid_array_register]
        // ** get head node inf (tid)
        apply(get_tid_table);

        // Stage 6
        // ** change the ip_dst
        apply(notify_head_client_table);

        // Stage 7: [register_lo: timestamp_hi_array_register]
        // ** get timestamp_hi
        apply(get_timestamp_hi_table);

        // Stage 8: [register_lo: timestamp_lo_array_register]
        // ** get timestamp_lo
        apply(get_timestamp_lo_table);

        if (((meta.recirc_flag == 1) and ((meta.dequeued_mode == EXCLUSIVE_LOCK) or (current_node_meta.mode == EXCLUSIVE_LOCK))) 
            or ((meta.recirc_flag == 2) and (current_node_meta.mode == SHARED_LOCK))) {
            // Stage 7
            // ** modify the ipv4 address for the packet
            apply(ipv4_route_2);

            // Stage 7
            // ** mirror the packet, one to notify the client, one to go through resubmit procedure again
            apply(i2e_mirror_table);
        }

        if (((meta.recirc_flag == 1) and ((meta.dequeued_mode == EXCLUSIVE_LOCK) or (current_node_meta.mode == EXCLUSIVE_LOCK))) 
             or ((meta.recirc_flag == 2) and (current_node_meta.mode == SHARED_LOCK)) 
             or (meta.recirc_flag == 0)) {
            // Stage 8
            // ** meta.head ++ (point to the next item)
            if (meta.head == meta.right) {
                apply(metahead_plus_1_table);
            }
            else {
                apply(metahead_plus_2_table);
            }

            if (meta.head != meta.tail) {
                // Stage 9
                if (meta.recirc_flag == 0) {
                    apply(mark_to_resubmit_table);
                }
                else if (((meta.recirc_flag == 1) and (current_node_meta.mode == SHARED_LOCK))
                           or (meta.recirc_flag == 2)) {
                    // ** ESSS
                    apply(mark_to_resubmit_2_table);
                }
                if (meta.do_resubmit == 1) {
                    apply(resubmit_table);
                }
            }

        }


        // ** drop the original packet
        if (0 == meta.do_resubmit) {
            apply(drop_packet_table);
        }

    }
    
}

control decode {
    // Stage 1: [register_lo: queue_size_op_register]
    apply(decode_table);

    // Stage 1: [register_lo: left_bound_register]
    apply(get_left_bound_table);

    // Stage 1: [register_lo: right_bound_register]
    apply(get_right_bound_table);

    // Stage 2
    apply(get_size_of_queue_table);
}

control modify_bound {

    // Stage 0
    apply(get_length_table);

    // Stage 1: [register_lo: queue_size_op_register]
    apply(shrink_finish_table);

    // Stage 1: [register_lo: left_bound_register]
    apply(modify_left_table);

    // Stage 1: [register_lo: right_bound_register]
    apply(modify_right_table);

    // Stage 2: [register_lo: empty_slots_register]
    apply(modify_empty_slots_table);

    // Stage 3: [register_lo: head_bound_register]
    apply(modify_head_table);

    // Stage 3: [register_lo: tail_bound_register]
    apply(modify_tail_table);

}

action get_tenant_inf_action(tenant_id, tenant_threshold) {
    modify_field(meta.tenant_id, tenant_id);
    modify_field(meta.tenant_threshold, tenant_threshold);
}

@pragma stage 0
table get_tenant_inf_table {
    reads {
        ipv4.srcAddr : exact;
    }
    actions {
        get_tenant_inf_action;
    }
}

action tenant_count_action() {
    tenant_count_alu.execute_stateful_alu(meta.tenant_id);
}

@pragma stage 1
table tenant_count_table {
    actions {
        tenant_count_action;
    }
    default_action: tenant_count_action;
}

control check_threshold {
    apply(get_tenant_inf_table);
    apply(tenant_count_table);
}

action reject_request_action() {
    modify_field(ipv4.dstAddr, ipv4.srcAddr);
    modify_field(nlk_hdr.op, REJECT_LOCK_ACQUIRE);
}

table reject_request_table {
    actions {
        reject_request_action;
    }
    default_action: reject_request_action;
}

control reject_request {
    apply(reject_request_table);
}

control ingress {
    if (valid(probe_hdr)) {
        apply(check_failure_status_2_table);
        apply(set_failure_status_table);
        apply(notify_tail_client_table);
    }
    // else if (valid(adm_hdr)) {
    //     else if (adm_hdr.op == MODIFY) {
    //         modify_bound();
    //     }
    // }
    else if ((valid(nlk_hdr)) and ((nlk_hdr.op == ACQUIRE_LOCK) or (nlk_hdr.op == PUSH_BACK_LOCK) or (nlk_hdr.op == RELEASE_LOCK))) {
        apply(check_lock_exist_table);
        apply(check_failure_status_table);
        if (nlk_hdr.op == ACQUIRE_LOCK) {
            check_threshold();
        }
        
        if ((meta.failure_status != 1) and (meta.lock_exist == 1)) {
            /* The switch is working, and is responsible for this lock */
            decode();
            if ((nlk_hdr.op == ACQUIRE_LOCK) and (meta.under_threshold == 0)) {
                /* The client/tenant is sending too much */
                reject_request();
            }
            else if ((nlk_hdr.op == ACQUIRE_LOCK) or (nlk_hdr.op == PUSH_BACK_LOCK)) {
                /* Handle normal lock requests and requests pushed back from the server */
                acquire_lock();
            }
            else if (nlk_hdr.op == RELEASE_LOCK) {
                /* Release a lock */
                release_lock();
            }
        } else {
            if ((meta.failure_status != 1) and (nlk_hdr.op == ACQUIRE_LOCK) and (meta.under_threshold == 0)) {
                /* The client/tenant is sending too much */
                reject_request();
            }
            else {
                /* Fail recovering or Switch is not responsible for this lock */
                apply(fix_src_port_table);
                apply(set_tag_table);
                apply(forward_to_server_table);
            }
        }
    }
    
    if ((valid(nlk_hdr) or valid(probe_hdr)) and (meta.routed == 0) and (meta.recirced == 0)) {
        /* Not doing routing for other packets for this experiment*/
        apply(ipv4_route);
    }
}

control egress {
    if (current_node_meta.clone_md == 1) {
        /*
         * if the packet is a notification, put target informations into the packet
         */
        apply (change_mode_table);
    }
}

