blackbox stateful_alu acquire_lock_alu {
    reg: lock_status_register;

    update_lo_1_value: 1;

    output_value: register_lo;
    output_dst  : meta.available;
}

blackbox stateful_alu release_lock_alu {
    reg: lock_status_register;

    update_lo_1_value: 0;
}

// blackbox stateful_alu acquire_lock_alu {
//     reg: lock_status_register;

//     update_lo_1_value: nc_hdr.txn_id;

//     output_value: register_lo;
//     output_dst  : meta.available;
// }

// blackbox stateful_alu release_lock_alu {
//     reg: lock_status_register;

//     condition_lo: register_lo == nc_hdr.txn_id;

//     update_hi_1_predicate: condition_lo;
//     update_lo_1_value: 0;
// }