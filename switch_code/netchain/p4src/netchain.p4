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

header_type meta_t {
    fields {
        lock_id: 32;
        routed: 1;
        available: 8;
    }
}
metadata meta_t meta;

register lock_status_register {
    width: 32;
    instance_count: LENGTH_ARRAY;
}

action decode_action() {
    modify_field(meta.lock_id, nc_hdr.lock);
}

table decode_table {
    actions {
        decode_action;
    }
    default_action: decode_action;
}

action release_lock_action() {
    release_lock_alu.execute_stateful_alu(meta.lock_id);
}

table release_lock_table {
    actions {
        release_lock_action;
    }
    default_action: release_lock_action;
}

action acquire_lock_action() {
    acquire_lock_alu.execute_stateful_alu(meta.lock_id);
}

table acquire_lock_table {
    actions {
        acquire_lock_action;
    }
    default_action: acquire_lock_action;
}

action set_retry_action() {
    modify_field(nc_hdr.op, RETRY_LOCK);
}

table set_retry_table {
    actions {
        set_retry_action;
    }
    default_action: set_retry_action;
}

action reply_to_client_action() {
    modify_field(ipv4.dstAddr, ipv4.srcAddr);
}

table reply_to_client_table {
    actions {
        reply_to_client_action;
    }
    default_action: reply_to_client_action;
}

control ingress {
    if (valid(nc_hdr)) {
        apply(decode_table);
        if (nc_hdr.op == ACQUIRE_LOCK) {
            apply(acquire_lock_table);
            if (meta.available != 0) {
                // not available, tell client to retry
                apply(set_retry_table);
            }
        }
        else if (nc_hdr.op == RELEASE_LOCK) {
            apply(release_lock_table);
        }
        apply(reply_to_client_table);
    }
    if (valid(nc_hdr)) {
        apply(ipv4_route);
    }
}

control egress {
    // if ()
}