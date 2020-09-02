import pd_base_tests
import pdb
import time
import sys
from collections import OrderedDict
from ptf import config
from ptf.testutils import *
from ptf.thriftutils import *

from netlock.p4_pd_rpc.ttypes import *
from mirror_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *

from config import *

dev_id = 0
if test_param_get("arch") == "tofino":
  MIR_SESS_COUNT = 1024
  MAX_SID_NORM = 1015
  MAX_SID_COAL = 1023
  BASE_SID_NORM = 1
  BASE_SID_COAL = 1016
elif test_param_get("arch") == "tofino2":
  MIR_SESS_COUNT = 256
  MAX_SID_NORM = 255
  MAX_SID_COAL = 255
  BASE_SID_NORM = 0
  BASE_SID_COAL = 0

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
    if port == 64: continue
    pipe = port_to_pipe(port)
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
cpu_port = swports[-1]
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

def init_tables(test, sess_hdl, dev_tgt):
    test.entry_hdls_ipv4 = []
    test.entry_hdls_ipv4_2 = []
    test.entry_acquire_lock_table = []
    # add entries for ipv4 routing

    test.client.ipv4_route_set_default_action__drop(sess_hdl, dev_tgt)
    for i in range(200):
        match_spec = netlock_ipv4_route_match_spec_t(i)
        action_spec = netlock_set_egress_action_spec_t(1)
        entry_hdl = test.client.ipv4_route_table_add_with_set_egress(
            sess_hdl, dev_tgt, match_spec, action_spec)
        test.entry_hdls_ipv4.append(entry_hdl)

    
    test.client.ipv4_route_2_set_default_action__drop(sess_hdl, dev_tgt)
    for i in range(200):
        match_spec = netlock_ipv4_route_2_match_spec_t(i)
        action_spec = netlock_set_egress_action_spec_t(1)
        entry_hdl = test.client.ipv4_route_2_table_add_with_set_egress_2(
            sess_hdl, dev_tgt, match_spec, action_spec)
        test.entry_hdls_ipv4_2.append(entry_hdl)
    
    match_spec = netlock_acquire_lock_table_match_spec_t(SHARED_LOCK)
    entry_hdl = test.client.acquire_lock_table_table_add_with_acquire_shared_lock_action(
            sess_hdl, dev_tgt, match_spec)
    test.entry_acquire_lock_table.append(entry_hdl)

    match_spec = netlock_acquire_lock_table_match_spec_t(EXCLUSIVE_LOCK)
    entry_hdl = test.client.acquire_lock_table_table_add_with_acquire_exclusive_lock_action(
            sess_hdl, dev_tgt, match_spec)
    test.entry_acquire_lock_table.append(entry_hdl)

    zero_v = netlock_shared_and_exclusive_count_register_value_t(0, 0)
    for i in range(200):
        test.client.register_write_left_bound_register(sess_hdl, dev_tgt, i, 5*(i-1) + 1)
        test.client.register_write_right_bound_register(sess_hdl, dev_tgt, i, 5*i)
        test.client.register_write_head_register(sess_hdl, dev_tgt, i, 5*(i-1) + 1)
        test.client.register_write_tail_register(sess_hdl, dev_tgt, i, 5*(i-1) + 1)
        test.client.register_write_shared_and_exclusive_count_register(sess_hdl, dev_tgt, i, zero_v)
        test.client.register_write_queue_size_op_register(sess_hdl, dev_tgt, i, 0)
        test.client.register_write_empty_slots_register(sess_hdl, dev_tgt, i, 5)


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

    if (test.entry_acquire_lock_table):
        print "Deleting %d entries" % len(test.entry_acquire_lock_table)
        for entry_hdl in test.entry_acquire_lock_table:
            status = test.client.acquire_lock_table_table_delete(
                sess_hdl, dev_id, entry_hdl)
    print "closing session"
    status = test.conn_mgr.client_cleanup(sess_hdl)

class AcquireLockTest(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["netlock"])
        scapy_netlock_bindings()

    def runTest(self):
        print "========== acquire lock test =========="
        sess_hdl = self.conn_mgr.client_init()
        self.sids = []
        
        try:
            
            read_flags = netlock_register_flags_t(read_hw_sync = True)

            init_tables(self, sess_hdl, dev_tgt)
            self.conn_mgr.complete_operations(sess_hdl)
            
            ####    Test acquire shared lock    #### 
            for i in range(3):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 1)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 1) and (ip_hdr.dst == '0.0.0.' + str(i+10)):
                    print "pass [acquire shared lock +]"
                else:
                    print "fail [acquire shared lock +]"

            ####    Test acquire exclusive lock    ####
            for i in range(2):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+10), op = ACQUIRE_LOCK, mode = EXCLUSIVE_LOCK, tid = 1, lock = 3)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                
                if (i == 0):
                    if (kv_hdr.lock == 3) and (ip_hdr.dst == '0.0.0.' + str(i+10)):
                        print "pass [acquire exclusive lock +]"
                    else:
                        print "fail [acquire exclusive lock +]"
                else:
                    if (kv_hdr.lock == 3) and (ip_hdr.dst != '0.0.0.' + str(i+10)):
                        print "pass [acquire exclusive lock -]"
                    else:
                        print "fail [acquire exclusive lock -]"  

            ####    Test acquire shared lock (occupied)    ####
            for i in range(2):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 3)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 3) and (ip_hdr.dst != '0.0.0.' + str(i+10)):
                    print "pass [acquire shared lock -]"
                else:
                    print "fail [acquire shared lock -]" 

        finally:
            for sid in self.sids:
                self.mirror.mirror_session_disable(sess_hdl, Direction_e.PD_DIR_INGRESS, dev_tgt, sid)
            for sid in self.sids:
                self.mirror.mirror_session_delete(sess_hdl, dev_tgt, sid)
            clean_tables(self, sess_hdl, dev_id)
        return

class ReleaseLockTest(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["netlock"])
        scapy_netlock_bindings()

    def runTest(self):
        print "========== release lock test =========="
        sess_hdl = self.conn_mgr.client_init()
        self.sids = []
        
        try:
            sids = random.sample(xrange(BASE_SID_NORM, MAX_SID_NORM), len(swports))
            for port,sid in zip(swports[0:1], sids[0:1]):
                action_spec = netlock_i2e_mirror_action_action_spec_t(sid)
                result = self.client.i2e_mirror_table_set_default_action_i2e_mirror_action(
                    sess_hdl, dev_tgt, action_spec)
                info = mirror_session(MirrorType_e.PD_MIRROR_TYPE_NORM,
                                  Direction_e.PD_DIR_INGRESS,
                                  sid,
                                  port,
                                  True)
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

            ####    Test acquire exclusive lock    ####
            for i in range(1):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+10), op = ACQUIRE_LOCK, mode = EXCLUSIVE_LOCK, tid = 1, lock = 2)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                
                if (kv_hdr.lock == 2) and (ip_hdr.dst == '0.0.0.' + str(i+10)):
                    print "pass [acquire exclusive lock +]"
                else:
                    print "fail [acquire exclusive lock +]"

            ####    Test acquire shared lock (occupied)    ####
            for i in range(3):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 2)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 2) and (ip_hdr.dst != '0.0.0.' + str(i+1+10)):
                    print "pass [acquire exclusive lock -]"
                else:
                    print "fail [acquire exclusive lock -]"

            ####    Test release exclusive lock (notify following locks)    ####
            for i in range(1):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+10), op = RELEASE_LOCK, mode = EXCLUSIVE_LOCK, tid = 1, lock = 2)
                send_packet(self, swports[0], pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                
                if (kv_hdr.lock == 2):
                    print "pass [release exclusive lock]"
                else:
                    print "fail [release exclusive lock]"    

                pkt = receive_packet(self, swports[0], netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                
                if (kv_hdr.lock == 2):
                    print "pass [release exclusive lock (notify following locks)]"
                else:
                    print "fail [release exclusive lock (notify following locks)]"

                pkt = receive_packet(self, swports[0], netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                
                if (kv_hdr.lock == 2):
                    print "pass [release exclusive lock (notify following locks)]"
                else:
                    print "fail [release exclusive lock (notify following locks)]"  

                pkt = receive_packet(self, swports[0], netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                
                if (kv_hdr.lock == 2):
                    print "pass [release exclusive lock (notify following locks)]"
                else:
                    print "fail [release exclusive lock (notify following locks)]"    

        finally:
            for sid in self.sids:
                self.mirror.mirror_session_disable(sess_hdl, Direction_e.PD_DIR_INGRESS, dev_tgt, sid)
            for sid in self.sids:
                self.mirror.mirror_session_delete(sess_hdl, dev_tgt, sid)
            clean_tables(self, sess_hdl, dev_id)
        return


class ASetShrinkTest(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["netlock"])
        scapy_netlock_bindings()

    def runTest(self):
        print "========== set shrink test =========="
        sess_hdl = self.conn_mgr.client_init()
        self.sids = []
        
        try:
            

            read_flags = netlock_register_flags_t(read_hw_sync = True)

            init_tables(self, sess_hdl, dev_tgt)
            self.conn_mgr.complete_operations(sess_hdl)
            self.devport_mgr.devport_mgr_set_copy_to_cpu(dev_id, True, cpu_port)

            ####    Test acquire shared lock    ####
            for i in range(2):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4) and (ip_hdr.dst == '0.0.0.' + str(i+1+10)):
                    print "pass [acquire shared lock +]"
                else:
                    print "fail [acquire shared lock +]"

            ####    Test set shrink mode for a lock    ####
            for i in range(1):
                pkt = adm_packet(ip_src = '1.0.0.' + str(i+1+10), op = SHRINK, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, adm_packet())

                kv_hdr = pkt.getlayer(ADM_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4):
                    print "pass [set shrink]"
                else:
                    print "fail [set shrink]"

            ####    Test acquire shared lock after SHRINK is set (should forward to server)    ####
            for i in range(2,3):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4) and (ip_hdr.dst == '0.0.0.' + str(DEDICATED_SERVER_IP)):
                    print "pass [acquire shared lock (forward to server)]"
                else:
                    print "fail [acquire shared lock (forward to server)]"

            ####    Test release shared lock    ####
            for i in range(1):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = RELEASE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4):
                    print "pass [release shared lock]"
                else:
                    print "fail [release shared lock]"

            ####    Test release shared lock (notify controller when the lock-queue is empty)    ####
            for i in range(1):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = RELEASE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, cpu_port, netlock_packet())
                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                verify_no_other_packets(self)
                if (kv_hdr.lock == 4):
                    print "pass [release the last lock (notify controller)]"
                else:
                    print "fail [release the last lock (notify controller)]"

        finally:
            for sid in self.sids:
                self.mirror.mirror_session_disable(sess_hdl, Direction_e.PD_DIR_INGRESS, dev_tgt, sid)
            for sid in self.sids:
                self.mirror.mirror_session_delete(sess_hdl, dev_tgt, sid)
            clean_tables(self, sess_hdl, dev_id)
        return


class ModifyBoundTest(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["netlock"])
        scapy_netlock_bindings()

    def runTest(self):
        print "========== modify bound test =========="
        sess_hdl = self.conn_mgr.client_init()
        self.sids = []
        
        try:
            read_flags = netlock_register_flags_t(read_hw_sync = True)

            init_tables(self, sess_hdl, dev_tgt)
            self.conn_mgr.complete_operations(sess_hdl)
            self.devport_mgr.devport_mgr_set_copy_to_cpu(dev_id, True, cpu_port)

            ####    Test acquire shared lock    ####
            for i in range(2):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4) and (ip_hdr.dst == '0.0.0.' + str(i+1+10)):
                    print "pass [acquire shared lock +]"
                else:
                    print "fail [acquire shared lock +]"

            ####    Test set SHRINK for a lock    ####
            for i in range(1):
                pkt = adm_packet(ip_src = '1.0.0.' + str(i+1+10), op = SHRINK, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, adm_packet())

                kv_hdr = pkt.getlayer(ADM_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4):
                    print "pass [set shrink]"
                else:
                    print "fail [set shrink]"

            ####    Test acquire shared lock    ####
            for i in range(2,3):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4) and (ip_hdr.dst == '0.0.0.' + str(DEDICATED_SERVER_IP)):
                    print "pass [acquire shared lock (forward to server)]"
                else:
                    print "fail [acquire shared lock (forward to server)]"

            ####    Test release shared lock    ####
            for i in range(1):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = RELEASE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (kv_hdr.lock == 4):
                    print "pass [release shared lock]"
                else:
                    print "fail [release shared lock]"

            ####    Test release shared lock (notify the controller when the lock-queue is empty)    ####
            for i in range(1):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = RELEASE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, cpu_port, netlock_packet())
                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                verify_no_other_packets(self)
                if (kv_hdr.lock == 4):
                    print "pass [release the last lock (notify controller)]"
                else:
                    print "fail [release the last lock (notify controller)]"

            ####    Test modify the bound when the queue is empty    ####
            for i in range(1):
                pkt = adm_packet(ip_src = '1.0.0.' + str(i+1+10), op = MODIFY, lock = 4, new_left = 17, new_right = 19)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, adm_packet())
                n_left = self.client.register_read_left_bound_register(sess_hdl, dev_tgt, 4, read_flags)
                n_right = self.client.register_read_right_bound_register(sess_hdl, dev_tgt, 4, read_flags)
                n_head = self.client.register_read_head_register(sess_hdl, dev_tgt, 4, read_flags)
                n_tail = self.client.register_read_tail_register(sess_hdl, dev_tgt, 4, read_flags)
                ## the value of register_read
                if (n_left[0] == 17) and (n_right[0] == 19):
                    print "pass [modify the bound]"
                else:
                    print "fail [modify the bound]"

            ####    Test acquire shared lock after the bound is modified    ####
            ####    Test acquire shared lock (forward to server when the queue is full)    ####
            for i in range(4):
                pkt = netlock_packet(ip_src = '0.0.0.' + str(i+1+10), op = ACQUIRE_LOCK, mode = SHARED_LOCK, tid = 1, lock = 4)
                send_packet(self, 0, pkt)
                pkt = receive_packet(self, 1, netlock_packet())

                kv_hdr = pkt.getlayer(NETLOCK_HDR)
                ip_hdr = pkt.getlayer(IP)
                if (i<3):
                    if (kv_hdr.lock == 4) and (ip_hdr.dst == '0.0.0.' + str(i+1+10)):
                        print "pass [acquire shared lock +]"
                    else:
                        print "fail [acquire shared lock +]"
                else:
                    if (kv_hdr.lock == 4) and (ip_hdr.dst == '0.0.0.' + str(DEDICATED_SERVER_IP)):
                        print "pass [acquire shared lock (forward to server)]"
                    else:
                        print "fail [acquire shared lock (forward to server)]"

        finally:
            for sid in self.sids:
                self.mirror.mirror_session_disable(sess_hdl, Direction_e.PD_DIR_INGRESS, dev_tgt, sid)
            for sid in self.sids:
                self.mirror.mirror_session_delete(sess_hdl, dev_tgt, sid)
            clean_tables(self, sess_hdl, dev_id)
        return