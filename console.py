import sys, os, time, subprocess, random
import paramiko
import threading
from config import *
from multiprocessing import Process
LOCK_NUM = 1000000
CLIENT_NUM = 2
CONTENTION_DEGREE = 1
SYNCHRONOUS = "s"
ASYNCHRONOUS = "a"
MICROBENCHMARK_SHARED = "s"
MICROBENCHMARK_EXCLUSIVE = "x"
ZIPFBENCHAMRK = 'z'
UNIFORMBENCHMARK = 'u'
TPCCBENCHMARK = 't'
TPCC_UNIFORMBENCHMARK = 'v'

INTERVAL = 5
WPKTS_SEND_LIMIT_MS = 10000
NUM_RCV_CORES = 6
MEM_BIN_PACK = "bin"
MEM_RAND_WEIGHT = "r_weight"
MEM_RAND_12 = "r_12"
MEM_RAND_200 = "r_20"
MAX_SLOT_NUM = 130000


def prRed(skk): print("\033[91m {}\033[00m" .format(skk))

class NetLockConsole(object):
    def to_hostname(self, client_name):
        return id_to_hostname_dict[client_name]

    def to_username(self, client_name):
        return id_to_username_dict[client_name] 

    def to_passwd(self, client_name):
        return id_to_passwd_dict[client_name]

    def __init__(self, client_names, client_names_2, server_names, switch_name):
        self.num_of_cores_ls = 8
        self.batch_size = 1
        self.timeout_slot = 400
        self.server_node_num = 1
        self.client_node_num = 10
        self.warehouse = 10
        self.think_time = 0
        self.task_id = 's'
        self.slot_num = MAX_SLOT_NUM
        
        self.memory_manage = MEM_BIN_PACK
        self.lock_num = LOCK_NUM
        self.client_num = CLIENT_NUM
        self.contention_degree = CONTENTION_DEGREE
        self.data_transfer_mode = ASYNCHRONOUS
        self.benchmark = ZIPFBENCHAMRK
        # self.benchmark = MICROBENCHMARK_EXCLUSIVE
        self.n_rcv_cores = NUM_RCV_CORES
        self.wpkts_send_limit_ms_client = WPKTS_SEND_LIMIT_MS
        self.wpkts_send_limit_ms_server = WPKTS_SEND_LIMIT_MS


        self.interval = INTERVAL
        self.mem_dict = {MEM_BIN_PACK:"b", MEM_RAND_WEIGHT:"w", MEM_RAND_12:"1", MEM_RAND_200:"2"}
        self.program_name = "netlock"
        self.netchain_name = "netchain"
        self.server_names = server_names
        self.client_names = client_names
        self.client_names_2 = client_names_2
        self.switch_name  = switch_name
        
        self.passwd = {}
        for client_name in client_names:
            self.passwd[client_name] = id_to_passwd_dict[client_name]

        for client_name in client_names_2:
            self.passwd[client_name] = id_to_passwd_dict[client_name]

        for client_name in server_names:
            self.passwd[client_name] = id_to_passwd_dict[client_name]

        self.switch_pw = id_to_passwd_dict[switch_name]

        self.clients = []
        for client_name in client_names:
            client = paramiko.SSHClient()
            # client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            client.load_system_host_keys()
            client.connect(hostname = self.to_hostname(client_name), username = self.to_username(client_name), password = self.passwd[client_name])
            self.clients.append((client_name, client))

        self.clients_2 = []
        for client_name in client_names_2:
            client = paramiko.SSHClient()
            # client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            client.load_system_host_keys()
            client.connect(hostname = self.to_hostname(client_name), username = self.to_username(client_name), password = self.passwd[client_name])
            self.clients_2.append((client_name, client))

        self.servers = []
        for server_name in server_names:
            client = paramiko.SSHClient()
            # client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            client.load_system_host_keys()
            client.connect(hostname = self.to_hostname(server_name), username = self.to_username(server_name), password = self.passwd[server_name])
            ##chan = client.get_transport().open_session()
            ##chan.get_pty()
            self.servers.append((server_name, client))
            


        switch = paramiko.SSHClient()
        switch.load_system_host_keys()
        switch.connect(hostname = self.to_hostname(self.switch_name), username = self.to_username(switch_name), password = self.switch_pw)
        self.switch = (self.switch_name, switch)

        self.local_home_dir = local_home_dir
        self.local_main_dir = self.local_home_dir + "NetLock/"
        self.local_p4_dir   = self.local_home_dir + "NetLock/switch_code/netlock/p4src/"
        self.local_ptf_dir  = self.local_home_dir + "NetLock/switch_code/netlock/controller_init/"
        self.local_res_dir  = self.local_home_dir + "NetLock/results/"
        self.local_host_dir = self.local_home_dir + "NetLock/dpdk_code/"
        self.local_client_dir = self.local_home_dir + "NetLock/dpdk_code/client_code/"
        self.local_server_dir = self.local_home_dir + "NetLock/dpdk_code/lock_server_code/"
        self.local_trace_dir  = self.local_home_dir + "NetLock/traces/"
        self.local_netchain_dir = self.local_home_dir + "NetLock/switch_code/netchain/"
        self.local_netchain_p4_dir = self.local_home_dir + "NetLock/switch_code/netchain/p4src/"
        self.local_netchain_ptf_dir = self.local_home_dir + "NetLock/switch_code/netchain/controller/"

        self.remote_server_home_dir = remote_server_home_dir
        self.remote_server_main_dir = self.remote_server_home_dir + "NetLock/"
        self.remote_server_host_dir = self.remote_server_home_dir + "NetLock/dpdk_code/"
        self.remote_server_client_dir = self.remote_server_home_dir + "NetLock/dpdk_code/client_code"
        self.remote_server_server_dir = self.remote_server_home_dir + "NetLock/dpdk_code/lock_server_code"
        self.remote_server_log_dir  = self.remote_server_home_dir + "NetLock/logs/"
        self.remote_server_res_dir  = self.remote_server_home_dir + "NetLock/results/"
        self.remote_server_trace_dir = self.remote_server_home_dir + "NetLock/traces/"
        self.remote_server_ptf_dir = self.remote_server_home_dir + "NetLock/switch_code/netlock/controller_init/"
        self.remote_server_netchain_client_dir = self.remote_server_client_dir
        # self.remote_server_netchain_dir = self.remote_server_home_dir + "NetLock/netchain/"

        self.remote_switch_home_dir = remote_switch_home_dir
        self.remote_switch_main_dir = self.remote_switch_home_dir + "NetLock/"
        self.remote_switch_p4_dir   = self.remote_switch_home_dir + "NetLock/switch_code/netlock/p4src/"
        self.remote_switch_ptf_dir  = self.remote_switch_home_dir + "NetLock/switch_code/netlock/controller_init/"
        self.remote_switch_log_dir  = self.remote_switch_home_dir + "NetLock/logs/"
        self.remote_switch_netchain_p4_dir = self.remote_switch_home_dir + "NetLock/switch_code/netchain/p4src/"
        self.remote_switch_netchain_ptf_dir = self.remote_switch_home_dir + "NetLock/switch_code/netchain/controller/"

        self.remote_switch_sde_dir  = remote_switch_sde_dir
        
        print "========init completed========"


    # ********************************
    # fundamental functions
    # ********************************

    def exe(self, client, cmd, with_print=False):
        (client_name, client_shell) = client
        stdin, stdout, stderr = client_shell.exec_command(cmd)
        # print client_name
        if with_print:
            # stdout.read()
            print client_name, ":", stdout.read(), stderr.read()

    def sudo_exe(self, client, cmd, with_print=False):
        (client_name, client_shell) = client
        cmdheader = "echo '%s' | sudo -S " %(self.passwd[client_name])
        cmd = cmdheader + cmd
        stdin, stdout, stderr = client_shell.exec_command(cmd)
        if with_print:
            # stdout.read()
            print client_name, ":", stdout.read(), stderr.read()
            stdout.flush()
            stderr.flush()

    def kill_host(self):
        for client in self.clients:
            self.sudo_exe(client, "pkill client")
        for server in self.servers:
            self.sudo_exe(server, "pkill server")

    def kill_switch(self):
        self.exe(self.switch, "ps -ef | grep switchd | grep -v grep | " \
            "awk '{print $2}' | xargs kill -9")
        self.exe(self.switch, "ps -ef | grep run_p4_test | grep -v grep | " \
            "awk '{print $2}' | xargs kill -9")
        self.exe(self.switch, "ps -ef | grep tofino | grep -v grep | " \
            "awk '{print $2}' | xargs kill -9")

    def kill_all(self):
        self.kill_host()
        self.kill_switch()
    
    def init_sync_host(self):
        for client in self.client_names + self.client_names_2 + self.server_names:
            cmd = "scp -r %s %s@%s:%s" % (self.local_main_dir, self.to_username(client), self.to_hostname(client), self.remote_server_home_dir)
            print cmd
            subprocess.call(cmd, shell = True)

    def init_sync_switch(self):
        cmd = "scp -r %s %s@%s:%s" % (self.local_main_dir, self.to_username(self.switch_name), self.to_hostname(self.switch_name), self.remote_switch_home_dir)
        print cmd
        subprocess.call(cmd, shell = True)

    def sync_host(self):
        for client in self.client_names:
            cmd = "rsync -r %s %s@%s:%s" % (self.local_client_dir, self.to_username(client), self.to_hostname(client), self.remote_server_client_dir)
            print cmd
            subprocess.call(cmd, shell = True)
            # cmd = "rsync -r %s %s@%s:%s" % (self.local_netchain_dir, self.to_username(client), self.to_hostname(client), self.remote_server_netchain_dir)
            # print cmd
            # subprocess.call(cmd, shell = True)
            cmd = "rsync -r %s %s@%s:%s" % (self.local_ptf_dir, self.to_username(client), self.to_hostname(client), self.remote_server_ptf_dir)
            print cmd
            subprocess.call(cmd, shell = True)
        for client in self.client_names_2:
            cmd = "rsync -r %s %s@%s:%s" % (self.local_client_dir, self.to_username(client), self.to_hostname(client), self.remote_server_client_dir)
            print cmd
            subprocess.call(cmd, shell = True)
            # cmd = "rsync -r %s %s@%s:%s" % (self.local_netchain_dir, self.to_username(client), self.to_hostname(client), self.remote_server_netchain_dir)
            # print cmd
            # subprocess.call(cmd, shell = True)
            cmd = "rsync -r %s %s@%s:%s" % (self.local_ptf_dir, self.to_username(client), self.to_hostname(client), self.remote_server_ptf_dir)
            print cmd
            subprocess.call(cmd, shell = True)
        for server in self.server_names:
            cmd = "rsync -r %s %s@%s:%s" % (self.local_server_dir, self.to_username(server), self.to_hostname(server), self.remote_server_server_dir)
            print cmd
            subprocess.call(cmd, shell = True)
            # cmd = "rsync -r %s %s@%s:%s" % (self.local_netchain_dir, self.to_username(server), self.to_hostname(server), self.remote_server_netchain_dir)
            # print cmd
            # subprocess.call(cmd, shell = True)
            cmd = "rsync -r %s %s@%s:%s" % (self.local_ptf_dir, self.to_username(server), self.to_hostname(server), self.remote_server_ptf_dir)
            print cmd
            subprocess.call(cmd, shell = True)
        return

    def sync_trace(self):
        for client in self.client_names:
            cmd = "rsync -r %s %s@%s:%s" % (self.local_trace_dir, self.to_username(client), self.to_hostname(client), self.remote_server_trace_dir)
            print cmd
            subprocess.call(cmd, shell = True)
        return

    def sync_switch(self):
        cmd = "scp -r %s %s@%s:%s" % (self.local_netchain_dir, self.to_username(self.switch_name), self.to_hostname(self.switch_name), self.remote_switch_main_dir)
        print cmd
        subprocess.call(cmd, shell = True)
        cmd = "scp -r %s %s@%s:%s" % (self.local_p4_dir, self.to_username(self.switch_name), self.to_hostname(self.switch_name), self.remote_switch_main_dir)
        print cmd
        subprocess.call(cmd, shell = True)
        cmd = "scp -r %s %s@%s:%s" % (self.local_ptf_dir, self.to_username(self.switch_name), self.to_hostname(self.switch_name), self.remote_switch_main_dir)
        print cmd
        subprocess.call(cmd, shell = True)
        return

    def sync_all(self):
        self.sync_switch()
        self.sync_host()
        return

    def compile_host(self):
        # dpdk_dir = self.remote_server_client_dir
        cmd_client = "source ~/.bash_profile;cd %s; make > %s/client_compile.log 2>&1 &" % (self.remote_server_client_dir, self.remote_server_log_dir)
        cmd_server = "source ~/.bash_profile;cd %s; make > %s/server_compile.log 2>&1 &" % (self.remote_server_server_dir, self.remote_server_log_dir)
        for client in self.clients + self.clients_2 + self.servers:
            print "%s compile client: %s" % (client[0], cmd_client)
            self.exe(client, cmd_client, True)
            print "%s compile server: %s" % (client[0], cmd_server)
            self.exe(client, cmd_server, True)
        return

    def compile_netchain(self):
        sde_dir = self.remote_switch_sde_dir
        p4_build = sde_dir + "p4_build.sh"
        p4_program = self.remote_switch_netchain_p4_dir + self.netchain_name + ".p4"
        cmd = "cd %s;source ./set_sde.bash;%s %s > %s/netchain_compile.log 2>&1 &" % (sde_dir, p4_build,
            p4_program, self.remote_switch_log_dir)
        print cmd
        self.exe(self.switch, cmd, True)
        return

    def compile_switch(self):
        sde_dir = self.remote_switch_sde_dir
        p4_build = sde_dir + "p4_build.sh"
        p4_program = self.remote_switch_p4_dir + self.program_name + ".p4"
        cmd = "cd %s;source ./set_sde.bash;%s %s > %s/p4_compile.log 2>&1 &" % (sde_dir, p4_build,
            p4_program, self.remote_switch_log_dir)
        print cmd
        self.exe(self.switch, cmd, True)
        return

    def compile_all(self):
        self.compile_host()
        self.compile_switch()
        return

    def run_client(self):
        dpdk_dir = self.remote_server_client_dir
        for client in self.clients:
            client_id = client[0].strip("netx")
            cmd = "cd %s;source ~/.bash_profile; echo '%s' | sudo -S %s/build/client --lcores 0@0,1@1,2@2,3@3,4@4,5@5,6@6,7@7 -- -m%s -s%s -w%d -i%s -n%s -b%s -r%s -c%s -z90 -e%s -k%s -o%s -l%d -a%s -T%d -C%d -S%d -O%d -g%d -N%d" %\
                 (self.remote_server_client_dir, self.passwd[client[0]], dpdk_dir, self.data_transfer_mode, self.wpkts_send_limit_ms_client, self.warehouse, self.interval, client_id, self.benchmark, self.n_rcv_cores, self.contention_degree, 
                  self.client_num, self.lock_num, self.mem_dict[self.memory_manage], self.slot_num, self.task_id, self.think_time, self.client_node_num, self.server_node_num, self.timeout_slot, self.batch_size, self.num_of_cores_ls) + " > %s/client_run_even_%s.log 2>&1 &" % (self.remote_server_res_dir, client_id)
            
            print "%s run client_dpdk: %s" % (client[0], cmd)
            self.exe(client, cmd, True)
        return

    def run_client_2(self):
        dpdk_dir = self.remote_server_client_dir
        for client in self.clients_2:
            client_id = client[0].strip("netx")
            cmd = "cd %s;source ~/.bash_profile; echo '%s' | sudo -S %s/build/client --lcores 0@0,1@1,2@2,3@3,4@4,5@5,6@6,7@7 -- -m%s -s%s -w%d -i%s -n%s -b%s -r%s -c%s -z90 -e%s -k%s -o%s -l%d -a%s -T%d -C%d -S%d -O%d -g%d -N%d" %\
                 (self.remote_server_client_dir, self.passwd[client[0]], dpdk_dir, self.data_transfer_mode, self.wpkts_send_limit_ms_client, self.warehouse, self.interval, client_id, self.benchmark, self.n_rcv_cores, self.contention_degree, 
                  self.client_num, self.lock_num, self.mem_dict[self.memory_manage], self.slot_num, self.task_id, self.think_time, self.client_node_num, self.server_node_num, self.timeout_slot, self.batch_size, self.num_of_cores_ls) + " > %s/client_run_even_%s.log 2>&1 &" % (self.remote_server_res_dir, client_id)
            print "%s run client_dpdk: %s" % (client[0], cmd)
            self.exe(client, cmd, True)
        return

    def run_server(self):
        dpdk_dir = self.remote_server_server_dir
        core_spec = ["0@0", "0@0", "0@0,1@1", "0@0,1@1,2@2", "0@0,1@1,2@2,3@3", "0@0,1@1,2@2,3@3,4@4", "0@0,1@1,2@2,3@3,4@4,5@5", "0@0,1@1,2@2,3@3,4@4,5@5,6@6", "0@0,1@1,2@2,3@3,4@4,5@5,6@6,7@7"]
        for server in self.servers:
            server_id = server[0].strip("netx")
            cmd = "cd %s;source ~/.bash_profile; echo '%s' | sudo -S %s/build/server --lcores %s -- -m%s -tf -s%s -w%d -i%s -n%s -b%s -r8 -c%s -z90 -e%s -k%s -o%s -l%d -a%s -T%d -C%d -S%d -O%d -g%d -N%d -P%s> %s/server_run_%d.log 2>&1 &" % (self.remote_server_server_dir, self.passwd[server[0]], dpdk_dir, core_spec[self.num_of_cores_ls], self.data_transfer_mode, 10000, self.warehouse, self.interval, server_id, self.benchmark, self.contention_degree, self.client_num, self.lock_num, self.mem_dict[self.memory_manage], self.slot_num, self.task_id, self.think_time, self.client_node_num, self.server_node_num, self.timeout_slot, self.batch_size, self.num_of_cores_ls, self.remote_server_main_dir, self.remote_server_res_dir, self.num_of_cores_ls)
            print "%s run server_dpdk: %s" % (server[0], cmd)
            self.exe(server, cmd, True)
        return

    def run_host(self):
        self.run_server()
        self.run_client()

    def run_netlock(self):
        sde_dir = self.remote_switch_sde_dir
        ## run switch
        run_netlockd = sde_dir + "run_switchd.sh"
        cmd = "cd %s;source ./set_sde.bash;%s -p %s > %s/run_switchd.log 2>&1 &" % (sde_dir, run_netlockd,
            self.program_name, self.remote_switch_log_dir)
        print cmd
        self.exe(self.switch, cmd, True)

        ## run ptf_test 
        run_ptf_test = sde_dir + "run_p4_tests.sh"
        ports_map = self.remote_switch_ptf_dir + "ports.json"
        target_mode = "hw"
        cmd = "cd %s;source ./set_sde.bash;%s -t %s -p %s -f %s --target %s --test-params=\"bm=\'%s\';lk=\'%s\';memn=\'%s\';slot=\'%s\';client_node_num=\'%d\';warehouse=\'%d\';server_node_num=\'%d\';task_id=\'%s\';batch_size=\'%d\';main_dir=\'%s\'\" > %s/run_ptf_test.log 2>&1 &" % (sde_dir, run_ptf_test,
            self.remote_switch_ptf_dir, self.program_name, ports_map, target_mode, self.benchmark, self.lock_num, self.memory_manage, self.slot_num, self.client_node_num, self.warehouse, self.server_node_num, self.task_id, self.batch_size, self.remote_switch_main_dir, self.remote_switch_log_dir)
        print cmd
        self.exe(self.switch, cmd, True)
        return

    def run_all(self):
        self.kill_all()
        time.sleep(10)
        self.run_netlock()
        time.sleep(180)
        self.run_server()
        time.sleep(10)
        self.run_client()
        time.sleep(30)
        self.grab_result()
        return

    def setup_dpdk(self):
        dpdk_dir = self.remote_server_client_dir
        cmd_eth_down = "ifconfig enp5s0f0 down > %s/eth_down.log 2>&1 &" % (self.remote_server_log_dir)
        

        for client in self.clients:
            cmd_setup_dpdk = "export passwd=%s;source ~/.bash_profile;echo $RTE_SDK;sh %s/tools.sh setup_dpdk > %s/setup_dpdk.log 2>&1 &" % (self.passwd[client[0]], dpdk_dir, self.remote_server_log_dir)
            print "%s run eth_down: %s" % (client[0], cmd_eth_down)
            self.sudo_exe(client, cmd_eth_down, True)
            print "%s run setup_dpdk: %s" % (client[0], cmd_setup_dpdk)
            self.exe(client, cmd_setup_dpdk, True)

        for client in self.clients_2:
            cmd_setup_dpdk = "export passwd=%s;source ~/.bash_profile;echo $RTE_SDK;sh %s/tools.sh setup_dpdk > %s/setup_dpdk.log 2>&1 &" % (self.passwd[client[0]], dpdk_dir, self.remote_server_log_dir)
            print "%s run eth_down: %s" % (client[0], cmd_eth_down)
            self.sudo_exe(client, cmd_eth_down, True)
            print "%s run setup_dpdk: %s" % (client[0], cmd_setup_dpdk)
            self.exe(client, cmd_setup_dpdk, True)

        for server in self.servers:
            cmd_setup_dpdk = "export passwd=%s;source ~/.bash_profile;echo $RTE_SDK;sh %s/tools.sh setup_dpdk > %s/setup_dpdk.log 2>&1 &" % (self.passwd[server[0]], dpdk_dir, self.remote_server_log_dir)
            print "%s run eth_down: %s" % (server[0], cmd_eth_down)
            self.sudo_exe(server, cmd_eth_down, True)
            print "%s run setup_dpdk: %s" % (server[0], cmd_setup_dpdk)
            self.exe(server, cmd_setup_dpdk, True)

        return

    def reboot_host(self):
        cmd = "reboot > %s/reboot.log 2>&1 &" % (self.remote_server_log_dir)
        for client in self.clients:
            print "%s run eth_down: %s" % (client[0], cmd)
            self.sudo_exe(client, cmd, True)
        for client in self.clients_2:
            print "%s run eth_down: %s" % (client[0], cmd)
            self.sudo_exe(client, cmd, True)
        for server in self.servers:
            print "%s run eth_down: %s" % (server[0], cmd)
            self.sudo_exe(server, cmd, True)

    def grab_result(self):
        for client in self.client_names:
            cmd = "rsync -r %s@%s:%s %s" % (self.to_username(client), self.to_hostname(client), self.remote_server_res_dir, self.local_res_dir)
            print cmd
            subprocess.call(cmd, shell = True)
        for client in self.client_names_2:
            cmd = "rsync -r %s@%s:%s %s" % (self.to_username(client), self.to_hostname(client), self.remote_server_res_dir, self.local_res_dir)
            print cmd
            subprocess.call(cmd, shell = True)
        for server in self.server_names:
            cmd = "rsync -r %s@%s:%s %s" % (self.to_username(server), self.to_hostname(server), self.remote_server_res_dir, self.local_res_dir)
            print cmd
            subprocess.call(cmd, shell = True)
        return

    def clean_result(self):
        if ("NetLock" in self.remote_server_res_dir):
            cmd = "rm -rf  %s/* > /dev/null  2>&1 & " % (self.remote_server_res_dir)
            for client in self.clients:
                print "%s deleting the result files: %s" % (client[0], cmd)
                self.sudo_exe(client, cmd, True)
            for client in self.clients_2:
                print "%s deleting the result files: %s" % (client[0], cmd)
                self.sudo_exe(client, cmd, True)
            for server in self.servers:
                print "%s deleting the result files: %s" % (server[0], cmd)
                self.sudo_exe(server, cmd, True)
        return

    def run_netchain_switch(self):
        sde_dir = self.remote_switch_sde_dir
        ## run switch
        run_netlockd = sde_dir + "run_switchd.sh"
        cmd = "cd %s;source ./set_sde.bash;%s -p %s > %s/run_netchain_switchd.log 2>&1 &" % (sde_dir, run_netlockd,
            self.netchain_name, self.remote_switch_log_dir)
        print cmd
        self.exe(self.switch, cmd, True)

        ## run ptf_test 
        run_ptf_test = sde_dir + "run_p4_tests.sh"
        ports_map = self.remote_switch_netchain_ptf_dir + "ports.json"
        target_mode = "hw"
        cmd = "cd %s;source ./set_sde.bash;%s -t %s -p %s -f %s --target %s --test-params=\"bm=\'%s\';lk=\'%s\';memn=\'%s\';slot=\'%s\';client_node_num=\'%d\';warehouse=\'%d\';server_node_num=\'%d\';task_id=\'%s\';batch_size=\'%d\'\" > %s/run_netchain_ptf_test.log 2>&1 &" % (sde_dir, run_ptf_test,
            self.remote_switch_netchain_ptf_dir, self.netchain_name, ports_map, target_mode, self.benchmark, self.lock_num, self.memory_manage, self.slot_num, self.client_node_num, self.warehouse, self.server_node_num, self.task_id, self.batch_size, self.remote_switch_log_dir)
        print cmd
        self.exe(self.switch, cmd, True)
        return

    # def run_netchain_server(self):
    #     dpdk_dir = self.remote_server_netchain_client_dir
    #     for server in self.servers:
    #         server_id = server[0].strip("netx")
    #         cmd = "cd %s;source ~/.bash_profile; echo '%s' | sudo -S %s/build/client --lcores 0@0,1@1,2@2,3@3,4@4,5@5,6@6,7@7 -- -m%s -tf -s%s -w%d -i%s -n%s -b%s -r8 -c%s -z90 -e%s -k%s -o%s -l%d -a%s -T%d -C%d -S%d -O%d -g%d> %s/netchain_server_run.log 2>&1 &" %\
    #             (self.remote_server_netchain_client_dir, self.passwd[server[0]], dpdk_dir, self.data_transfer_mode, self.wpkts_send_limit_ms_server, self.warehouse, self.interval, server_id, self.benchmark, self.contention_degree, self.client_num, self.lock_num, self.mem_dict[self.memory_manage], 
    #              self.slot_num, self.task_id, self.think_time, self.client_node_num, self.server_node_num, self.timeout_slot, self.batch_size, self.remote_server_log_dir)
    #         print "%s run server_dpdk: %s" % (server[0], cmd)
    #         self.exe(server, cmd, True)
    #     return

    def compile_netchain_host(self):
        dpdk_dir = self.remote_server_netchain_client_dir
        cmd = "source ~/.bash_profile;cd %s; make > %s/netchain_compile.log 2>&1 &" % (dpdk_dir, self.remote_server_log_dir)
        for client in self.clients:
            print "%s compile client_dpdk: %s" % (client[0], cmd)
            self.exe(client, cmd, True)
        for client in self.clients_2:
            print "%s compile client_dpdk: %s" % (client[0], cmd)
            self.exe(client, cmd, True)
        for server in self.servers:
            print "%s compile server_dpdk: %s" % (server[0], cmd)
            self.exe(server, cmd, True)
        return

    def run_netchain_client(self):
        dpdk_dir = self.remote_server_netchain_client_dir
        for client in self.clients:
            client_id = client[0].strip("netx")
            cmd = "cd %s;source ~/.bash_profile; echo '%s' | sudo -S %s/build/client --lcores 0@0,1@1,2@2,3@3,4@4,5@5,6@6,7@7 -- -m%s -s%s -w%d -i%s -n%s -b%s -r%s -c%s -z90 -e%s -k%s -o%s -l%d -a%s -T%d -C%d -S%d -O%d -g%d" %\
                 (self.remote_server_netchain_client_dir, self.passwd[client[0]], dpdk_dir, self.data_transfer_mode, self.wpkts_send_limit_ms_client, self.warehouse, self.interval, client_id, self.benchmark, self.n_rcv_cores, self.contention_degree, 
                  self.client_num, self.lock_num, self.mem_dict[self.memory_manage], self.slot_num, self.task_id, self.think_time, self.client_node_num, self.server_node_num, self.timeout_slot, self.batch_size) + " > %s/netchain_run_even_%s.log 2>&1 &" % (self.remote_server_res_dir, client_id)
            print "%s run client_dpdk: %s" % (client[0], cmd)
            self.exe(client, cmd, True)
        return    

    ## fig. 8(a)
    def micro_bm_s(self):
        self.task_id = 's'
        self.benchmark = MICROBENCHMARK_SHARED
        self.lock_num = 12
        self.client_node_num = 12
        self.server_node_num = 0
        self.client_num = 2
        self.n_rcv_cores = 6
        send_limit_list = [5000]
        for i in send_limit_list:
            self.wpkts_send_limit_ms_client = i
            self.run_all()

    ## fig. 8(b)
    def micro_bm_x(self):
        self.task_id = 'x'
        self.benchmark = MICROBENCHMARK_EXCLUSIVE
        self.lock_num = 55000
        send_limit_list = [5000]
        self.client_node_num = 12
        self.server_node_num = 0
        self.client_num = 2
        self.n_rcv_cores = 6
        self.data_transfer_mode = SYNCHRONOUS
        for i in send_limit_list:
            self.wpkts_send_limit_ms_client = i
            self.run_all()

    ## fig. 8(c)(d)
    def micro_bm_cont(self):
        self.server_node_num = 0
        self.client_node_num = 12

        self.task_id = 'c'
        self.benchmark = MICROBENCHMARK_EXCLUSIVE
        self.wpkts_send_limit_ms_client = 5000
        self.contention_degree = 12
        self.client_num = 2
        self.n_rcv_cores = 6
        self.data_transfer_mode = SYNCHRONOUS
        
        lk_num_list = [500, 1000, 1500, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000]
        for i in lk_num_list:
            self.lock_num = i
            self.run_all()
        return

    ## fig. 13(a)
    def mem_management(self):
        self.think_time = 0

        self.task_id = 'm'
        self.benchmark = TPCCBENCHMARK
        self.wpkts_send_limit_ms_client = 10000

        self.client_num = 4
        self.n_rcv_cores = 4
        
        self.data_transfer_mode = ASYNCHRONOUS
        self.client_node_num = 10
        self.server_node_num = 2
        self.warehouse = 10
        self.interval = 10
        self.lock_num = 700000 * self.warehouse
        self.slot_num = 130000
        self.timeout_slot = 600
        for sn in [130000]:
            self.timeout_slot = 4000 - sn / 100
            self.slot_num = sn
            for mm in [MEM_BIN_PACK, MEM_RAND_WEIGHT]:
                self.memory_manage = mm
                if (self.memory_manage == MEM_RAND_WEIGHT):
                    self.wpkts_send_limit_ms_client = 3000
                self.kill_all()
                time.sleep(10)
                self.run_netlock()
                time.sleep(180)
                self.run_server()
                self.run_client()
                time.sleep(45)
                self.grab_result()
        return

    ## fig. 14(b)
    def mem_size(self):
        self.task_id = 'S'
        self.think_time = 0
        self.client_num = 4
        self.n_rcv_cores = 4
        self.data_transfer_mode = ASYNCHRONOUS

        self.client_node_num = 10
        self.server_node_num = 2
        self.warehouse = 10

        self.interval = 15
        self.wpkts_send_limit_ms_client = 1000
        self.client_num = 4
        self.lock_num = 700000 * self.warehouse
        self.memory_manage = MEM_BIN_PACK

        # for bm in [ZIPFBENCHAMRK]:
        for bm in [TPCC_UNIFORMBENCHMARK]:
            for sn in [2000, 3000, 4000, 5000, 10000, 20000]:
                self.slot_num = sn
                self.benchmark = bm
                self.kill_all()
                time.sleep(5)
                self.run_netlock()
                time.sleep(180)
                self.run_server()
                self.run_client()
                time.sleep(50)
                self.grab_result()
        return

    ## fig. 14(a)
    def run_think_time(self):
        self.task_id = 't'
        self.wpkts_send_limit_ms_client = 10000
        self.client_num = 4
        self.n_rcv_cores = 4
        self.benchmark = TPCCBENCHMARK
        self.contention_degree = 12
        self.memory_manage = MEM_BIN_PACK
        self.slot_num = 130000
        self.data_transfer_mode = ASYNCHRONOUS
        self.client_node_num = 10
        self.server_node_num = 2
        self.warehouse = 10
        self.interval = 8
        self.lock_num = 700000 * self.warehouse

        send_rate_dic = {0: 10000, 1: 10000, 10:10000, 100:10000, 1000:5000, 10000: 1500, 100000: 1500}
        percent_dic = {4000: 0.3716185416666667, 6000: 0.3976164583333333, 90000: 0.597170625, 130000: 0.6281872916666666, 70000: 0.576940625, 8000: 0.41656604166666666, 100000: 0.6059195833333333, 80000: 0.5875347916666667, 120000: 0.6212935416666666, 30000: 0.5105391666666667, 2000: 0.3292158333333333, 50000: 0.549965625, 40000: 0.5325160416666667, 110000: 0.6140197916666666, 20000: 0.4805391666666667, 18000: 0.473036875, 60000: 0.5643233333333333, 16000: 0.46444895833333333, 14000: 0.45505979166666666, 12000: 0.44424604166666665, 10000: 0.43155}
        
        self.timeout_slot = 400
       
        for sn in [0, 400, 800, 1200, 1600, 2000, 2400, 2800, 3200, 3600, 4000]:
            self.slot_num = sn
            if (sn < 0):
                self.timeout_slot = 4000 - 6 * sn 
            else:
                self.timeout_slot = 400
            for tt in [0, 5, 10, 100]:
                self.think_time = tt
                self.kill_all()
                time.sleep(5)
                self.run_netlock()
                time.sleep(100)
                self.run_server()
                self.run_client()
                time.sleep(45)
                self.grab_result()
        return

    
    # fig. 10
    def run_tpcc(self):
        self.task_id = 'p'
        self.benchmark = TPCCBENCHMARK
        self.client_num = 4
        self.n_rcv_cores = 4
        self.memory_manage = MEM_BIN_PACK
        self.server_node_num = 2
        self.client_node_num = 10
        
        self.wpkts_send_limit_ms_client = 10000
        self.think_time = 0
        self.timeout_slot = 2000
        for wh in [1, 10]:
            self.warehouse = wh
            self.lock_num = 700000 * self.warehouse
            self.kill_all()
            time.sleep(5)
            self.run_netlock()
            time.sleep(180)
            self.run_server()
            time.sleep(1)
            self.run_client()
            time.sleep(30)
            self.grab_result()
        return

    # fig. 11
    def run_tpcc_multiple_server(self):
        self.task_id = 'q'
        self.benchmark = TPCCBENCHMARK
        self.client_num = 4
        self.n_rcv_cores = 4
        self.memory_manage = MEM_BIN_PACK
        self.server_node_num = 4
        self.client_node_num = 6
        
        self.wpkts_send_limit_ms_client = 10000
        self.think_time = 0
        for wh in [1, 10]:
            self.warehouse = wh
            self.lock_num = 700000 * self.warehouse
            self.kill_all()
            time.sleep(5)
            self.run_netlock()
            time.sleep(180)
            self.run_server()
            time.sleep(1)
            self.run_client()
            time.sleep(30)
            self.grab_result()
        return

    # Not shown
    def granularity(self):
        self.task_id = 'g'
        self.benchmark = TPCCBENCHMARK
        self.client_num = 4
        self.n_rcv_cores = 4
        self.memory_manage = MEM_BIN_PACK
        self.server_node_num = 1
        self.client_node_num = 10
        self.slot_num = 100000
        self.wpkts_send_limit_ms_client = 10000
        self.think_time = 0
        self.warehouse = 10
        self.lock_num = 700000 * self.warehouse
        self.interval = 10

        for to in [200, 300, 400]:
            self.timeout_slot = to
            for batch_size in [1, 10, 50, 100, 500, 1000, 2000]:
                self.batch_size = batch_size
                self.kill_all()
                time.sleep(5)
                self.run_netlock()
                time.sleep(180)
                self.run_server()
                self.run_client()
                time.sleep(50)
                self.grab_result()
        return

    # fig. 15
    def failover(self):
        self.task_id = 'f'
        self.benchmark = TPCCBENCHMARK
        self.client_num = 4
        self.n_rcv_cores = 4
        self.memory_manage = MEM_BIN_PACK
        self.server_node_num = 2
        self.client_node_num = 10
        
        self.wpkts_send_limit_ms_client = 10000
        self.think_time = 0
        self.interval = 50
        for wh in [10]:
            self.warehouse = wh
            self.lock_num = 700000 * self.warehouse
            self.kill_all()
            time.sleep(5)
            self.run_netlock()
            time.sleep(180)
            self.run_server()
            self.run_client()
            time.sleep(30)
            self.grab_result()
        return

    # fig. 12(a)
    def priority(self):
        self.task_id = 'r'
        self.benchmark = TPCCBENCHMARK
        self.client_num = 4
        self.n_rcv_cores = 4
        self.memory_manage = MEM_BIN_PACK
        self.server_node_num = 2
        self.client_node_num = 10
        
        self.wpkts_send_limit_ms_client = 10000
        self.think_time = 0
        self.timeout_slot = 1000
        self.interval = 18
        for wh in [10]:
            self.warehouse = wh
            self.lock_num = 700000 * self.warehouse
            self.kill_all()
            time.sleep(5)
            self.run_netlock()
            time.sleep(180)
            self.run_server()
            self.run_client_2()
            time.sleep(20)
            self.run_client()
            time.sleep(65)
            self.grab_result()
        return

    # fig. 10
    def netchain(self):
        # self.batch_size = 10
        self.task_id = 'p'
        self.benchmark = TPCCBENCHMARK
        self.client_num = 2
        self.n_rcv_cores = 6
        self.memory_manage = MEM_BIN_PACK
        self.server_node_num = 0
        self.client_node_num = 10
        
        self.wpkts_send_limit_ms_client = 10000
        self.think_time = 0
        self.timeout_slot = 1000
        for wh in [1, 10]:
            self.warehouse = wh
            self.lock_num = 700000 * self.warehouse
            self.kill_all()
            time.sleep(5)
            self.run_netchain_switch()
            time.sleep(50)
            # self.run_netchain_server()
            self.run_netchain_client()
            time.sleep(50)
            self.grab_result()
        return

    # fig. 11
    def netchain_ms(self):
        # self.batch_size = 10
        self.task_id = 'q'
        self.benchmark = TPCCBENCHMARK
        self.client_num = 2
        self.n_rcv_cores = 6
        self.memory_manage = MEM_BIN_PACK
        self.server_node_num = 0
        self.client_node_num = 6
        
        self.wpkts_send_limit_ms_client = 10000
        self.think_time = 0
        self.timeout_slot = 1000
        for wh in [1, 10]:
            self.warehouse = wh
            self.lock_num = 700000 * self.warehouse
            self.kill_all()
            time.sleep(5)
            self.run_netchain_switch()
            time.sleep(50)
            # self.run_netchain_server()
            self.run_netchain_client()
            time.sleep(50)
            self.grab_result()
        return

    ## fig. 9
    def micro_bm_s_only_server(self):
        self.task_id = 'e'
        self.benchmark = MICROBENCHMARK_SHARED
        self.lock_num = 160
        self.client_node_num = 10
        self.server_node_num = 1
        self.client_num = 2
        self.n_rcv_cores = 6
        self.wpkts_send_limit_ms_client = 1000
        for noc in [8]:
            self.num_of_cores_ls = noc
            self.kill_all()
            time.sleep(5)
            self.run_netlock()
            time.sleep(50)
            self.run_server()
            self.run_client()
            time.sleep(30)
            self.grab_result()

    ## fig. 9
    def micro_bm_x_only_server(self):
        self.task_id = 'e'
        self.benchmark = MICROBENCHMARK_EXCLUSIVE
        self.lock_num = 55000
        self.client_node_num = 10
        self.server_node_num = 1
        self.client_num = 2
        self.n_rcv_cores = 6
        self.data_transfer_mode = SYNCHRONOUS
        self.wpkts_send_limit_ms_client = 500
        for noc in [8]:
            self.num_of_cores_ls = noc
            self.kill_all()
            time.sleep(5)
            self.run_netlock()
            time.sleep(50)
            self.run_server()
            self.run_client()
            time.sleep(30)
            self.grab_result()

    # fig. 9
    def micro_bm_cont_only_server(self):
        self.server_node_num = 1
        self.client_node_num = 10

        self.task_id = 'e'
        self.benchmark = MICROBENCHMARK_EXCLUSIVE
        self.wpkts_send_limit_ms_client = 10000
        self.contention_degree = 12
        self.client_num = 2
        self.n_rcv_cores = 6
        self.data_transfer_mode = SYNCHRONOUS
        lk_num_list = [10000]
        self.wpkts_send_limit_ms_client = 500
        for i in lk_num_list:
            self.lock_num = i
            for noc in [2,4,6,8]:
                self.num_of_cores_ls = noc
                self.kill_all()
                time.sleep(5)
                self.run_netlock()
                time.sleep(55)
                self.run_server()
                self.run_client()
                time.sleep(30)
                self.grab_result()
        return

def print_usage():
    prRed("Usage")
    prRed("  console.py sync_(host, trace, switch, all)")
    prRed("  console.py compile_(host, switch, all)")
    prRed("  console.py run_(client, server, netlock, host, all)")
    prRed("  console.py setup_dpdk")
    prRed("  console.py reboot_host")
    prRed("  console.py grab_result")
    prRed("  console.py clean_result")
    prRed("  console.py benchmark (e.g. micro_bm_s, micro_bm_x, micro_bm_cont, mem_man, mem_size, think_time, run_tpcc, run_tpcc_ms)")
    prRed("  console.py failover")
    prRed("  console.py compile_netchain")
    sys.exit()

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        print_usage()
        sys.exit(0)


    print(client_id_t1)
    print(server_id)
    nl_console = NetLockConsole(client_id_t1, client_id_t2, server_id, switch_id)
    

    if sys.argv[1] == "sync_host":
        nl_console.sync_host()
    elif sys.argv[1] == "sync_trace":
        nl_console.sync_trace()
    elif sys.argv[1] == "init_sync_host":
        nl_console.init_sync_host()
    elif sys.argv[1] == "init_sync_switch":
        nl_console.init_sync_switch()
    elif sys.argv[1] == "sync_switch":
        nl_console.sync_switch()
    elif sys.argv[1] == "sync_all":
        nl_console.sync_all()
    elif sys.argv[1] == "compile_host":
        nl_console.compile_host()
    elif sys.argv[1] == "compile_switch":
        nl_console.compile_switch()
    elif sys.argv[1] == "compile_all":
        nl_console.compile_all()
    elif sys.argv[1] == "run_client":
        nl_console.run_client()
    elif sys.argv[1] == "run_server":
        nl_console.run_server()
    elif sys.argv[1] == "run_netlock":
        nl_console.run_netlock()
    elif sys.argv[1] == "run_all":
        nl_console.run_all()
    elif sys.argv[1] == "kill_host":
        nl_console.kill_host()
    elif sys.argv[1] == "kill_switch":
        nl_console.kill_switch()
    elif sys.argv[1] == "kill_all":
        nl_console.kill_all()
    elif sys.argv[1] == "setup_dpdk":
        nl_console.setup_dpdk()
    elif sys.argv[1] == "reboot_host":
        nl_console.reboot_host()
    elif sys.argv[1] == "grab_result":
        nl_console.grab_result()
    elif sys.argv[1] == "clean_result":
        nl_console.clean_result()
    elif sys.argv[1] == "micro_bm_s":
        nl_console.micro_bm_s()
    elif sys.argv[1] == "micro_bm_x":
        nl_console.micro_bm_x()
    elif sys.argv[1] == "micro_bm_cont":
        nl_console.micro_bm_cont()
    elif sys.argv[1] == "run_host":
        nl_console.run_host()
    elif sys.argv[1] == "mem_man":
        nl_console.mem_management()
    elif sys.argv[1] == "mem_size":
        nl_console.mem_size()
    elif sys.argv[1] == "think_time":
        nl_console.run_think_time()
    elif sys.argv[1] == "run_tpcc":
        nl_console.run_tpcc()
    elif sys.argv[1] == "run_tpcc_ms":
        nl_console.run_tpcc_multiple_server()
    elif sys.argv[1] == "granularity":
        nl_console.granularity()
    elif sys.argv[1] == "priority":
        nl_console.priority()
    elif sys.argv[1] == "failover":
        nl_console.failover()
    elif sys.argv[1] == "compile_netchain":
        nl_console.compile_netchain()
    elif sys.argv[1] == "compile_netchain_host":
        nl_console.compile_netchain_host()
    elif sys.argv[1] == "netchain":
        nl_console.netchain()
    elif sys.argv[1] == "netchain_ms":
        nl_console.netchain_ms()
    elif sys.argv[1] == "run_tpcc_only_server":
        nl_console.run_tpcc_incast_diff_cores_only_server()
    elif sys.argv[1] == "run_tpcc_add":
        nl_console.run_tpcc_incast_diff_cores()
    elif sys.argv[1] == "micro_bm_s_only_server":
        nl_console.micro_bm_s_only_server()
    elif sys.argv[1] == "micro_bm_cont_only_server":
        nl_console.micro_bm_cont_only_server()
    elif sys.argv[1] == "micro_bm_x_only_server":
        nl_console.micro_bm_x_only_server()
    else:
        print_usage()