header_type ethernet_t {
    fields {
        dstAddr:    48;
        srcAddr:    48;
        etherType:  16;
    }
}
header ethernet_t ethernet;

header_type ipv4_t {
    fields {
        version:        4;
        ihl:            4;
        diffserv:       8;
        totalLen:       16;
        identification: 16;
        flags:          3;
        fragOffset:     13;
        ttl:            8;
        protocol:       8;
        hdrChecksum:    16;
        srcAddr:        32;
        dstAddr:        32;
    }
}
header ipv4_t ipv4;

header_type tcp_t {
    fields {
        srcPort:    16;
        dstPort:    16;
        seqNo:      32;
        ackNo:      32;
        dataOffset: 4;
        res:        3;
        ecn:        3;
        ctrl:       6;
        window:     16;
        checksum:   16;
        urgentPtr:  16;
    }
}
header tcp_t tcp;

header_type udp_t {
    fields {
        srcPort:    16;
        dstPort:    16;
        pkt_length: 16;
        checksum:   16;
    }
}
header udp_t udp;

header_type nlk_hdr_t {
    fields {
        recirc_flag: 8;
        op: 8;
        mode: 8;
        client_id: 8;
        tid: 32;
        lock: 32;
        timestamp_lo: 32;
        timestamp_hi: 32;
    }

}
header nlk_hdr_t nlk_hdr;

header_type adm_hdr_t {
    fields {
        op: 8;
        lock: 32;
        new_left: 32;
        new_right:32;
    }
}
header adm_hdr_t adm_hdr;

header_type recirculate_hdr_t {
    fields {
        dequeued_mode: 8;
        cur_head: 32;
        cur_tail: 32;
    }
}
header recirculate_hdr_t recirculate_hdr;

header_type probe_hdr_t {
    fields {
        failure_status: 8;
        op: 8;
        mode: 8;
        client_id: 8;
        tid: 32;
        lock: 32;
        timestamp_lo: 32;
        timestamp_hi: 32;
    }
}
header probe_hdr_t probe_hdr;