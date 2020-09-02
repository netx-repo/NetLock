import pd_base_tests
import pdb
import time
import sys

from collections import OrderedDict
from ptf import config
from ptf.testutils import *
from ptf.thriftutils import *

import os

from pal_rpc.ttypes import *

from netlock.p4_pd_rpc.ttypes import *
from mirror_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *

from pkt_pd_rpc.ttypes import *

from config import *

MAX_SLOTS_NUM = 130000
MEM_BIN_PACK = "bin"
MEM_RAND_WEIGHT = "r_weight"
MEM_RAND_12 = "r_12"
MEM_RAND_200 = "r_20"

UDP_DSTPORT = 8888

port_ip_dic = {188: 0x0a010001 , 184: 0x0a010002 , 180: 0x0a010003 , 176: 0x0a010004 ,
               172: 0x0a010005 , 168: 0x0a010006 , 164: 0x0a010007 , 160: 0x0a010008 ,
               156: 0x0a010009 , 152: 0x0a01000a , 148: 0x0a01000b , 144: 0x0a01000c}

tot_num_lks = 0
slots_v_list = []
left_bound_list = []
dev_id = 0
if test_param_get("arch") == "Tofino":
  print "TYPE Tofino"
  sys.stdout.flush()
  MIR_SESS_COUNT = 1024
  MAX_SID_NORM = 1015
  MAX_SID_COAL = 1023
  BASE_SID_NORM = 1
  BASE_SID_COAL = 1016
elif test_param_get("arch") == "Tofino2":
  print "TYPE Tofino2"
  sys.stdout.flush()
  MIR_SESS_COUNT = 256
  MAX_SID_NORM = 255
  MAX_SID_COAL = 255
  BASE_SID_NORM = 0
  BASE_SID_COAL = 0
else:
  print "TYPE NONE"
  print test_param_get("arch")
  sys.stdout.flush()

ports = [188]

mirror_ids = []

dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

def setup_random(seed_val=0):
    if 0 == seed_val:
        seed_val = int(time.time())
    print
    print "Seed is:", seed_val
    sys.stdout.flush()
    random.seed(seed_val)

def make_port(pipe, local_port):
    assert(pipe >= 0 and pipe < 4)
    assert(local_port >= 0 and local_port < 72)
    return (pipe << 7) | local_port

def port_to_pipe(port):
    local_port = port & 0x7F
    assert(local_port < 72)
    pipe = (port >> 7) & 0x3
    assert(port == ((pipe << 7) | local_port))
    return pipe

def port_to_pipe_local_port(port):
    return port & 0x7F

swports = []
swports_by_pipe = {}
for device, port, ifname in config["interfaces"]:
    if port == 0: continue
    if port == 64: continue
    pipe = port_to_pipe(port)
    print device, port, pipe, ifname
    print int(test_param_get('num_pipes'))
    if pipe not in swports_by_pipe:
        swports_by_pipe[pipe] = []
    if pipe in range(int(test_param_get('num_pipes'))):
        swports.append(port)
        swports.sort()
        swports_by_pipe[pipe].append(port)
        swports_by_pipe[pipe].sort()

if swports == []:
    for pipe in range(int(test_param_get('num_pipes'))):
        for port in range(1):
            swports.append( make_port(pipe,port) )
cpu_port = 64
#cpu_port = 192
print "Using ports:", swports
sys.stdout.flush()

def mirror_session(mir_type, mir_dir, sid, egr_port=0, egr_port_v=False,
                   egr_port_queue=0, packet_color=0, mcast_grp_a=0,
                   mcast_grp_a_v=False, mcast_grp_b=0, mcast_grp_b_v=False,
                   max_pkt_len=1024, level1_mcast_hash=0, level2_mcast_hash=0,
                   mcast_l1_xid=0, mcast_l2_xid=0, mcast_rid=0, cos=0, c2c=0, extract_len=0, timeout=0,
                   int_hdr=[], hdr_len=0):
    return MirrorSessionInfo_t(mir_type,
                             mir_dir,
                             sid,
                             egr_port,
                             egr_port_v,
                             egr_port_queue,
                             packet_color,
                             mcast_grp_a,
                             mcast_grp_a_v,
                             mcast_grp_b,
                             mcast_grp_b_v,
                             max_pkt_len,
                             level1_mcast_hash,
                             level2_mcast_hash,
                             mcast_l1_xid,
                             mcast_l2_xid,
                             mcast_rid,
                             cos,
                             c2c,
                             extract_len,
                             timeout,
                             int_hdr,
                             hdr_len)

class NETLOCK_HDR(Packet):
    name = "NETLOCK_HDR"
    fields_desc = [
        XByteField("recirc_flag", 0),
        XByteField("op", 0),
        XByteField("mode", 0),
        XIntField("tid", 0),
        XIntField("lock", 0)
    ]

class ADM_HDR(Packet):
    name = "ADM_HDR"
    fields_desc = [
        XByteField("op", 0),
        XIntField("lock", 0),
        XIntField("new_left", 0),
        XIntField("new_right", 0)
    ]

def netlock_packet(pktlen=0,
            eth_dst='00:11:11:11:11:11',
            eth_src='00:22:22:22:22:22',
            ip_src='0.0.0.2',
            ip_dst='0.0.0.1',
            udp_sport=8000,
            udp_dport=LK_PORT,
            recirc_flag=0,
            op=0,
            mode=0,
            tid=0,
            lock=0):
    udp_pkt = simple_udp_packet(pktlen=0,
                                eth_dst=eth_dst,
                                eth_src=eth_src,
                                ip_dst=ip_dst,
                                ip_src=ip_src,
                                udp_sport=udp_sport,
                                udp_dport=udp_dport)

    return udp_pkt / NETLOCK_HDR(recirc_flag=recirc_flag, op=op, mode = mode, tid = tid, lock = lock)

def adm_packet(pktlen=0,
            eth_dst='00:11:11:11:11:11',
            eth_src='00:22:22:22:22:22',
            ip_src='0.0.0.2',
            ip_dst='0.0.0.1',
            udp_sport=8000,
            udp_dport=ADM_PORT,
            op=0,
            lock=0,
            new_left=0,
            new_right=0):
    udp_pkt = simple_udp_packet(pktlen=0,
                                eth_dst=eth_dst,
                                eth_src=eth_src,
                                ip_dst=ip_dst,
                                ip_src=ip_src,
                                udp_sport=udp_sport,
                                udp_dport=udp_dport)

    return udp_pkt / ADM_HDR(op=op, lock = lock, new_left = new_left, new_right = new_right)

def scapy_netlock_bindings():
    bind_layers(UDP, NETLOCK_HDR, dport=LK_PORT)
    bind_layers(UDP, ADM_HDR, dport=ADM_PORT)

def receive_packet(test, port_id, template):
    dev, port = port_to_tuple(port_id)
    (rcv_device, rcv_port, rcv_pkt, pkt_time) = dp_poll(test, dev, port, timeout=2)
    nrcv = template.__class__(rcv_pkt)
    return nrcv

def print_packet(test, port_id, template):
    receive_packet(test, port_id, template).show2()

def addPorts(test):
    test.pal.pal_port_add_all(dev_id, pal_port_speed_t.BF_SPEED_40G, pal_fec_type_t.BF_FEC_TYP_NONE)
    test.pal.pal_port_enable_all(dev_id)
    ports_not_up = True
    print "Waiting for ports to come up..."
    sys.stdout.flush()
    num_tries = 12
    i = 0
    while ports_not_up:
        ports_not_up = False
        for p in swports:
            x = test.pal.pal_port_oper_status_get(dev_id, p)
            if x == pal_oper_status_t.BF_PORT_DOWN:
                ports_not_up = True
                print "  port", p, "is down"
                sys.stdout.flush()
                time.sleep(3)
                break
        i = i + 1
        if i >= num_tries:
            break
    assert ports_not_up == False
    print "All ports up."
    sys.stdout.flush()
    return



def init_tables(test, sess_hdl, dev_tgt):
    global tot_num_lks
    global slots_v_list
    test.entry_hdls_ipv4 = []
    test.entry_hdls_ipv4_2 = []
    test.entry_acquire_lock_table = []
    test.entry_ethernet_set_mac = []
    test.entry_dec_empty_slots_table = []
    test.entry_fix_src_port_table = []
    test.entry_check_lock_exist_table = []
    test.entry_set_tag_table = []
    test.entry_change_mode_table = []
    test.entry_forward_to_server_table = []
    test.entry_get_tenant_inf_table = []
    ipv4_table_address_list = [0x0a010001, 0x0a010002, 0x0a010003, 0x0a010004, 0x0a010005,
        0x0a010006, 0x0a010007, 0x0a010008, 0x0a010009, 0x0a01000a, 0x0a01000b, 0x0a01000c, 0x01010101]
    ipv4_table_port_list = [188, 184, 180, 176, 172, 168, 164, 160, 156, 152, 148, 144, 320]
    tgt_tenant = [1,2,3, 4,5,6, 7,8,9, 10,11,0, 1]
    ethernet_set_mac_src = ["\xa8\x2b\xb5\xde\x92\x2e", 
                            "\xa8\x2b\xb5\xde\x92\x32",
                            "\xa8\x2b\xb5\xde\x92\x36",
                            "\xa8\x2b\xb5\xde\x92\x3a",
                            "\xa8\x2b\xb5\xde\x92\x3e",
                            "\xa8\x2b\xb5\xde\x92\x42",
                            "\xa8\x2b\xb5\xde\x92\x46",
                            "\xa8\x2b\xb5\xde\x92\x4a",
                            "\xa8\x2b\xb5\xde\x92\x4e",
                            "\xa8\x2b\xb5\xde\x92\x52",
                            "\xa8\x2b\xb5\xde\x92\x56",
                            "\xa8\x2b\xb5\xde\x92\x5a"]
    ethernet_set_mac_dst = ["\x3c\xfd\xfe\xab\xde\xd8",
                            "\x3c\xfd\xfe\xa6\xeb\x10",
                            "\x3c\xfd\xfe\xaa\x5d\x00",
                            "\x3c\xfd\xfe\xaa\x46\x68",
                            "\x3c\xfd\xfe\xab\xde\xf0",
                            "\x3c\xfd\xfe\xab\xdf\x90",
                            "\x3c\xfd\xfe\xab\xe0\x50",
                            "\x3c\xfd\xfe\xab\xd9\xf0",
                            "\xd0\x94\x66\x3b\x12\x37",
                            "\xd0\x94\x66\x84\x9f\x19",
                            "\xd0\x94\x66\x84\x9f\xa9",
                            "\xd0\x94\x66\x84\x54\x81"]
    # fix_src_port = [9000, 9001, 9002, 9003, 9004, 9005, 9006, 9007]
    fix_src_port = []
    for i in range(256):
        fix_src_port.append(9000 + i)
    udp_src_port_list = []
    for i in range(128):
        udp_src_port_list.append(UDP_DSTPORT + i)

    # add entries for ipv4 routing

    test.client.ipv4_route_set_default_action__drop(sess_hdl, dev_tgt)
    for i in range(len(ipv4_table_address_list)):
        match_spec = netlock_ipv4_route_match_spec_t(ipv4_table_address_list[i])
        action_spec = netlock_set_egress_action_spec_t(ipv4_table_port_list[i])
        entry_hdl = test.client.ipv4_route_table_add_with_set_egress(
            sess_hdl, dev_tgt, match_spec, action_spec)
        test.entry_hdls_ipv4.append(entry_hdl)

    test.client.ipv4_route_2_set_default_action__drop(sess_hdl, dev_tgt)
    for i in range(len(ipv4_table_address_list)):
        match_spec = netlock_ipv4_route_2_match_spec_t(ipv4_table_address_list[i])
        action_spec = netlock_set_egress_action_spec_t(ipv4_table_port_list[i])
        entry_hdl = test.client.ipv4_route_2_table_add_with_set_egress_2(
            sess_hdl, dev_tgt, match_spec, action_spec)
        test.entry_hdls_ipv4_2.append(entry_hdl)

    ## Add multiple servers
    server_node_num = int(test_param_get('server_node_num'))

    # add entries for other tables
    priority_0 = 1
    for i in range(server_node_num):
        match_spec = netlock_forward_to_server_table_match_spec_t(i, server_node_num - 1)
        action_spec = netlock_forward_to_server_action_action_spec_t(ipv4_table_address_list[11 - i])
        entry_hdl = test.client.forward_to_server_table_table_add_with_forward_to_server_action(sess_hdl, dev_tgt, match_spec, priority_0, action_spec)
        test.entry_forward_to_server_table.append(entry_hdl)
    
    for i in range(len(ipv4_table_address_list)):
        match_spec = netlock_get_tenant_inf_table_match_spec_t(ipv4_table_address_list[i])
        action_spec = netlock_get_tenant_inf_action_action_spec_t(tgt_tenant[i], 500000000)
        entry_hdl = test.client.get_tenant_inf_table_table_add_with_get_tenant_inf_action(
            sess_hdl, dev_tgt, match_spec, action_spec)
        test.entry_get_tenant_inf_table.append(entry_hdl)

    match_spec = netlock_acquire_lock_table_match_spec_t(SHARED_LOCK)
    entry_hdl = test.client.acquire_lock_table_table_add_with_acquire_shared_lock_action(
            sess_hdl, dev_tgt, match_spec)
    test.entry_acquire_lock_table.append(entry_hdl)

    match_spec = netlock_acquire_lock_table_match_spec_t(EXCLUSIVE_LOCK)
    entry_hdl = test.client.acquire_lock_table_table_add_with_acquire_exclusive_lock_action(
            sess_hdl, dev_tgt, match_spec)
    test.entry_acquire_lock_table.append(entry_hdl)

    match_spec_0 = netlock_dec_empty_slots_table_match_spec_t(0)  # normal acquire
    match_spec_1 = netlock_dec_empty_slots_table_match_spec_t(2)  # server push back
    entry_hdl_0 = test.client.dec_empty_slots_table_table_add_with_dec_empty_slots_action(
        sess_hdl, dev_tgt, match_spec_0)
    entry_hdl_1 = test.client.dec_empty_slots_table_table_add_with_push_back_action(
        sess_hdl, dev_tgt, match_spec_1)
    test.entry_dec_empty_slots_table.append(entry_hdl_0)
    test.entry_dec_empty_slots_table.append(entry_hdl_1)

    priority_0 = 1
    for i in range(len(fix_src_port)):
        match_spec = netlock_fix_src_port_table_match_spec_t(i, len(fix_src_port) - 1)
        action_spec = netlock_fix_src_port_action_action_spec_t(fix_src_port[i])
        entry_hdl = test.client.fix_src_port_table_table_add_with_fix_src_port_action(
            sess_hdl, dev_tgt, match_spec, priority_0, action_spec)
        test.entry_fix_src_port_table.append(entry_hdl)

    for i in range(len(udp_src_port_list)):
        match_spec = netlock_change_mode_table_match_spec_t(i, len(udp_src_port_list) - 1)
        action_spec = netlock_change_mode_act_action_spec_t(udp_src_port_list[i])
        entry_hdl = test.client.change_mode_table_table_add_with_change_mode_act(
            sess_hdl, dev_tgt, match_spec, priority_0, action_spec)
        test.entry_change_mode_table.append(entry_hdl)

    match_spec_0_0 = netlock_set_tag_table_match_spec_t(0, 0)
    match_spec_0_1 = netlock_set_tag_table_match_spec_t(0, 1)
    match_spec_1_0 = netlock_set_tag_table_match_spec_t(1, 0)
    match_spec_1_1 = netlock_set_tag_table_match_spec_t(1, 1)
    entry_hdl_0 = test.client.set_tag_table_table_add_with_set_as_primary_action(
            sess_hdl, dev_tgt, match_spec_0_0)
    entry_hdl_1 = test.client.set_tag_table_table_add_with_set_as_secondary_action(
            sess_hdl, dev_tgt, match_spec_0_1)
    entry_hdl_2 = test.client.set_tag_table_table_add_with_set_as_primary_action(
            sess_hdl, dev_tgt, match_spec_1_0)
    entry_hdl_3 = test.client.set_tag_table_table_add_with_set_as_failure_notification_action(
            sess_hdl, dev_tgt, match_spec_1_1)
    test.entry_set_tag_table.append(entry_hdl_0)
    test.entry_set_tag_table.append(entry_hdl_1)
    test.entry_set_tag_table.append(entry_hdl_2)
    test.entry_set_tag_table.append(entry_hdl_3)

    zero_v = netlock_shared_and_exclusive_count_register_value_t(0, 0)
    tot_lk = int(test_param_get('lk'))
    hmap = [0 for i in range(tot_lk + 1)]
    
    if (test_param_get('slot') != None):
        slot_num = int(test_param_get('slot'))
    else:
        slot_num = MAX_SLOTS_NUM

    hash_v = 0
    task_id = test_param_get('task_id')
    if (test_param_get('bm') == 'x') and (task_id != 'e'):
        #### microbenchmark exclusive lock low contention
        tot_num_lks = tot_lk
        qs = slot_num / tot_lk
        slots_v = netlock_slots_two_sides_register_value_t(0, qs)
        for i in range(1, tot_lk + 1):
            slots_v_list.append(slots_v)
            test.client.register_write_left_bound_register(sess_hdl, dev_tgt, i, qs*(i-1) + 1)
            left_bound_list.append(qs*(i-1) + 1)
            test.client.register_write_right_bound_register(sess_hdl, dev_tgt, i, qs*i)
            test.client.register_write_head_register(sess_hdl, dev_tgt, i, qs*(i-1) + 1)
            test.client.register_write_tail_register(sess_hdl, dev_tgt, i, qs*(i-1) + 1)
            test.client.register_write_shared_and_exclusive_count_register(sess_hdl, dev_tgt, i, zero_v)
            test.client.register_write_queue_size_op_register(sess_hdl, dev_tgt, i, 0)
            test.client.register_write_slots_two_sides_register(sess_hdl, dev_tgt, i, slots_v)
            #### CHANGE according to memory management
            match_spec = netlock_check_lock_exist_table_match_spec_t(i)
            action_spec = netlock_check_lock_exist_action_action_spec_t(i)
            entry_hdl = test.client.check_lock_exist_table_table_add_with_check_lock_exist_action(
                sess_hdl, dev_tgt, match_spec, action_spec)
            test.entry_check_lock_exist_table.append(entry_hdl)
    elif (test_param_get('bm') == 's') and (task_id != 'e'):
        #### microbenchmark shared lock
        tot_num_lks = tot_lk
        qs = slot_num / tot_lk
        slots_v_qs = netlock_slots_two_sides_register_value_t(0, qs)
        for i in range(1, tot_lk + 1):
            slots_v_list.append(slots_v_qs)
            test.client.register_write_left_bound_register(sess_hdl, dev_tgt, i, qs*(i-1) + 1)
            left_bound_list.append(qs*(i-1) + 1)
            test.client.register_write_right_bound_register(sess_hdl, dev_tgt, i, qs*i)
            test.client.register_write_head_register(sess_hdl, dev_tgt, i, qs*(i-1) + 1)
            test.client.register_write_tail_register(sess_hdl, dev_tgt, i, qs*(i-1) + 1)
            test.client.register_write_shared_and_exclusive_count_register(sess_hdl, dev_tgt, i, zero_v)
            test.client.register_write_queue_size_op_register(sess_hdl, dev_tgt, i, 0)
            test.client.register_write_slots_two_sides_register(sess_hdl, dev_tgt, i, slots_v_qs)
            match_spec = netlock_check_lock_exist_table_match_spec_t(i)
            action_spec = netlock_check_lock_exist_action_action_spec_t(i)
            entry_hdl = test.client.check_lock_exist_table_table_add_with_check_lock_exist_action(
                sess_hdl, dev_tgt, match_spec, action_spec)
            test.entry_check_lock_exist_table.append(entry_hdl)
    elif ((test_param_get('bm') == 't') or (test_param_get('bm') == 'v')):
        #### TPCC benchmark 
        if (test_param_get('slot') != None):
            slot_num = int(test_param_get('slot'))
        else:
            slot_num = MAX_SLOTS_NUM
        client_node_num = test_param_get('client_node_num')
        warehouse = test_param_get('warehouse')
        task_id = test_param_get('task_id')
        batch_size = test_param_get('batch_size')
        main_dir = test_param_get('main_dir')
        if (test_param_get('memn') == MEM_BIN_PACK):
            if (task_id == 'p') or (task_id == '2'):
                filename_suffix = "tpcc_notablelock_incast_"+client_node_num+"_w_"+warehouse + "_sl_" + str(slot_num) + "_nomap.in"
            elif (task_id == 'q') or (task_id == '3'):
                filename_suffix = "tpcc_notablelock_multiserver_"+client_node_num+"_w_"+warehouse + "_sl_" + str(slot_num) + "_nomap.in"
            elif (task_id == 'g'):
                filename_suffix = "tpcc_notablelock_incast_"+client_node_num+"_w_"+warehouse + "_sl_" + str(slot_num) + "_map_" + batch_size + ".in"
            elif (task_id == 'e'):
                filename_suffix = "empty.in"
            else:
                filename_suffix = "tpcc_notablelock_incast_"+client_node_num+"_w_"+warehouse + "_sl_" + str(slot_num) + "_nomap.in"
        else:
            filename_suffix = "tpcc_notablelock_incast_random_sn_" + str(slot_num) + ".in"
        # filename = "/home/zhuolong/exp/netlock-code/controller_init/tpcc/" + filename_suffix

        filename = main_dir + "/switch_code/netlock/controller_init/tpcc/" + filename_suffix
        print "Input filename:",filename
        if (filename != "null"):
            fin = open(filename)
            start_bound = 0
            while True:
                line = fin.readline()
                if not line:
                    break
                words = [x.strip() for x in line.split(',')]
                lk = int(words[0]) + 1
                hash_v += 1
                hmap[lk] = hash_v
                lk_num = int(words[1])
                slots_v = netlock_slots_two_sides_register_value_t(0, lk_num)
                slots_v_list.append(slots_v)
                test.client.register_write_left_bound_register(sess_hdl, dev_tgt, hash_v, start_bound + 1)
                left_bound_list.append(start_bound + 1)
                test.client.register_write_right_bound_register(sess_hdl, dev_tgt, hash_v, start_bound + lk_num)
                test.client.register_write_head_register(sess_hdl, dev_tgt, hash_v, start_bound + 1)
                test.client.register_write_tail_register(sess_hdl, dev_tgt, hash_v, start_bound + 1)
                test.client.register_write_shared_and_exclusive_count_register(sess_hdl, dev_tgt, hash_v, zero_v)
                test.client.register_write_queue_size_op_register(sess_hdl, dev_tgt, hash_v, 0)
                test.client.register_write_slots_two_sides_register(sess_hdl, dev_tgt, hash_v, slots_v)
                match_spec = netlock_check_lock_exist_table_match_spec_t(lk)
                action_spec = netlock_check_lock_exist_action_action_spec_t(hash_v)
                entry_hdl = test.client.check_lock_exist_table_table_add_with_check_lock_exist_action(
                    sess_hdl, dev_tgt, match_spec, action_spec)
                test.entry_check_lock_exist_table.append(entry_hdl)
                start_bound = start_bound + lk_num
            tot_num_lks = hash_v

def clean_tables(test, sess_hdl, dev_id):
    if (test.entry_hdls_ipv4):
        print "Deleting %d entries" % len(test.entry_hdls_ipv4)
        for entry_hdl in test.entry_hdls_ipv4:
            status = test.client.ipv4_route_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_hdls_ipv4_2):
        print "Deleting %d entries" % len(test.entry_hdls_ipv4_2)
        for entry_hdl in test.entry_hdls_ipv4_2:
            status = test.client.ipv4_route_2_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_ethernet_set_mac):
        print "Deleting %d entries" % len(test.entry_ethernet_set_mac)
        for entry_hdl in test.entry_ethernet_set_mac:
            status = test.client.ethernet_set_mac_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_acquire_lock_table):
        print "Deleting %d entries" % len(test.entry_acquire_lock_table)
        for entry_hdl in test.entry_acquire_lock_table:
            status = test.client.acquire_lock_table_table_delete(
                sess_hdl, dev_id, entry_hdl)
    if (test.entry_dec_empty_slots_table):
        print "Deleting %d entries" % len(test.entry_dec_empty_slots_table)
        for entry_hdl in test.entry_dec_empty_slots_table:
            status = test.client.dec_empty_slots_table_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_fix_src_port_table):
        print "Deleting %d entries" % len(test.entry_fix_src_port_table)
        for entry_hdl in test.entry_fix_src_port_table:
            status = test.client.fix_src_port_table_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_check_lock_exist_table):
        print "Deleting %d entries" % len(test.entry_check_lock_exist_table)
        for entry_hdl in test.entry_check_lock_exist_table:
            status = test.client.check_lock_exist_table_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_set_tag_table):
        print "Deleting %d entries" % len(test.entry_set_tag_table)
        for entry_hdl in test.entry_set_tag_table:
            status = test.client.set_tag_table_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_change_mode_table):
        print "Deleting %d entries" % len(test.entry_change_mode_table)
        for entry_hdl in test.entry_change_mode_table:
            status = test.client.change_mode_table_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_forward_to_server_table):
        print "Deleting %d entries" % len(test.entry_forward_to_server_table)
        for entry_hdl in test.entry_forward_to_server_table:
            status = test.client.forward_to_server_table_table_delete(
                sess_hdl, dev_id, entry_hdl)

    if (test.entry_get_tenant_inf_table):
        print "Deleting %d entries" % len(test.entry_get_tenant_inf_table)
        for entry_hdl in test.entry_get_tenant_inf_table:
            status = test.client.get_tenant_inf_table_table_delete(
                sess_hdl, dev_id, entry_hdl)

    print "closing session"
    status = test.conn_mgr.client_cleanup(sess_hdl)

def failure_sim(test, sess_hdl, dev_tgt):
    global tot_num_lks
    print "failover BEGIN."
    sys.stdout.flush()
    # set failure_status to failure (failure_status_register)
    test.client.register_write_failure_status_register(sess_hdl, dev_tgt, 0, 1)

    # set head,tail register 
    zero_v = netlock_shared_and_exclusive_count_register_value_t(0, 0)
    read_flags = netlock_register_flags_t(read_hw_sync = True)
    for i in range(1, tot_num_lks + 1):
        k_left = left_bound_list[i - 1]
        test.client.register_write_head_register(sess_hdl, dev_tgt, i, k_left)
        test.client.register_write_tail_register(sess_hdl, dev_tgt, i, k_left)
        test.client.register_write_shared_and_exclusive_count_register(sess_hdl, dev_tgt, i, zero_v)
        test.client.register_write_slots_two_sides_register(sess_hdl, dev_tgt, i, slots_v_list[i-1])

    # set failure_status to normal
    test.client.register_write_failure_status_register(sess_hdl, dev_tgt, 0, 0)
    return

class AcquireLockTest(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["netlock"])
        scapy_netlock_bindings()

    def runTest(self):
        #self.pkt.init()
        #sess_pkt = self.pkt.client_init()
        print "========== acquire lock test =========="
        sess_hdl = self.conn_mgr.client_init()
        self.sids = []
        
        try:
            if (test_param_get('target') == 'hw'):
                addPorts(self)
            else:
                print "test_param_get(target):", test_param_get('target')

            sids = random.sample(xrange(BASE_SID_NORM, MAX_SID_NORM), len(swports))
            for port,sid in zip(swports[0:len(swports)], sids[0:len(sids)]):
                ip_address = port_ip_dic[port]
                match_spec = netlock_i2e_mirror_table_match_spec_t(ip_address)                
                action_spec = netlock_i2e_mirror_action_action_spec_t(sid)
                result = self.client.i2e_mirror_table_table_add_with_i2e_mirror_action(sess_hdl,
                 dev_tgt, match_spec, action_spec)
                info = mirror_session(MirrorType_e.PD_MIRROR_TYPE_NORM,
                                  Direction_e.PD_DIR_INGRESS,
                                  sid,
                                  port,
                                  True)
                print "port:", port, "; sid:", sid 
                sys.stdout.flush()
                self.mirror.mirror_session_create(sess_hdl, dev_tgt, info)
                self.sids.append(sid)
            self.conn_mgr.complete_operations(sess_hdl)
            for sid in self.sids:
                self.mirror.mirror_session_enable(sess_hdl, Direction_e.PD_DIR_INGRESS, dev_tgt, sid)
            self.conn_mgr.complete_operations(sess_hdl)

            

            read_flags = netlock_register_flags_t(read_hw_sync = True)

            init_tables(self, sess_hdl, dev_tgt)
            self.conn_mgr.complete_operations(sess_hdl)
            self.devport_mgr.devport_mgr_set_copy_to_cpu(dev_id, True, cpu_port)
            print "INIT Finished."
            sys.stdout.flush()
            wait_time = 0
            while (True):
                if (test_param_get('task_id') == 'f'):
                    if (wait_time == 122):
                        failure_sim(self, sess_hdl, dev_tgt)
                        print "failover FINISHED."
                        sys.stdout.flush()
                    if (wait_time <= 122):
                        wait_time += 1
                count_0 = netlock_tenant_acq_counter_register_value_t(0, 0)
                for i in range(13):
                    self.client.register_write_tenant_acq_counter_register(sess_hdl, dev_tgt, i, count_0)
                time.sleep(1)
                
            self.conn_mgr.complete_operations(sess_hdl)
        finally:
            for sid in self.sids:
                self.mirror.mirror_session_disable(sess_hdl, Direction_e.PD_DIR_INGRESS, dev_tgt, sid)
            for sid in self.sids:
                self.mirror.mirror_session_delete(sess_hdl, dev_tgt, sid)
            clean_tables(self, sess_hdl, dev_id)