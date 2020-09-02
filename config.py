import sys
# local_home_dir = "/Users/zyu/Dropbox/code/p4/"
local_home_dir = "/home/username/zhuolong/ae/"
remote_server_home_dir = "/home/username/zhuolong/test/"
remote_switch_home_dir = "/home/zhuolong/test/"
remote_switch_sde_dir  = "/home/zhuolong/bf-sde-8.2.2/"

id_to_usernamename_dict = {"netx1": "username", "netx2": "username", "netx3": "username",
     "netx4": "username", "netx5": "username", "netx6": "username",
     "netx7": "username", "netx8": "username", "netx9": "username",
     "netx10": "username", "netx11": "username", "netx12": "username",
     "netxy": "username"}
id_to_passwd_dict = {"netx1": "yourpasswd", "netx2": "yourpasswd", "netx3": "yourpasswd",
     "netx4": "yourpasswd", "netx5": "yourpasswd", "netx6": "yourpasswd",
     "netx7": "yourpasswd", "netx8": "yourpasswd", "netx9": "yourpasswd",
     "netx10": "yourpasswd", "netx11": "yourpasswd", "netx12": "yourpasswd",
     "netxy": "yourpasswd"}

id_to_hostname_dict = {"netx1": "netx1.cs.jhu.edu", "netx2": "netx2.cs.jhu.edu", 
     "netx3": "netx3.cs.jhu.edu", "netx4": "netx4.cs.jhu.edu",
     "netx5": "netx5.cs.jhu.edu", "netx6": "netx6.cs.jhu.edu",
     "netx7": "netx7.cs.jhu.edu", "netx8": "netx8.cs.jhu.edu",
     "netx9": "netx9.cs.jhu.edu", "netx10": "netx10.cs.jhu.edu",
     "netx11": "netx11.cs.jhu.edu", "netx12": "netx12.cs.jhu.edu",
     "netxy": "netxy.cs.jhu.edu"}

client_id_t1 = []
client_id_t2 = []
server_id = []

def conf_12_clients():
     global client_id_t1, client_id_t2, server_id
     # tenant 1's client id. Normally we only need this.
     client_id_t1 = ["netx1", "netx2", "netx3", "netx4", "netx5", "netx6", "netx7", "netx8", "netx9", "netx10", "netx11", "netx12"]
     # tenant 2's client id. It's for the two tenant situation.
     client_id_t2 = []
     # Lock server's id
     server_id = []
     print("Clients:", client_id_t1)
     print("Servers:", server_id)

def conf_10_clients_2_servers():
     global client_id_t1, client_id_t2, server_id
     # tenant 1's client id. Normally we only need this.
     client_id_t1 = ["netx1", "netx2", "netx3", "netx4", "netx5", "netx6", "netx7", "netx8", "netx9", "netx10"]
     # tenant 2's client id. It's for the two tenant situation.
     client_id_t2 = []
     # Lock server's id
     server_id = ["netx11", "netx12"]
     print("Clients:", client_id_t1)
     print("Servers:", server_id)

def conf_6_clients_6_servers():
     global client_id_t1, client_id_t2, server_id
     # tenant 1's client id. Normally we only need this.
     client_id_t1 = ["netx1", "netx2", "netx3", "netx4", "netx5", "netx6"]
     # tenant 2's client id. It's for the two tenant situation.
     client_id_t2 = []
     # Lock server's id
     server_id = ["netx7", "netx8", "netx9", "netx10", "netx11", "netx12"]
     print("Clients:", client_id_t1)
     print("Servers:", server_id)

## use different func to set the clients/servers for different experiments
# conf_12_clients()
if (sys.argv[1] in ["micro_bm_s", "micro_bm_x", "micro_bm_cont"]):
   conf_12_clients()
elif (sys.argv[1] in ["run_tpcc_ms"]):
   conf_6_clients_6_servers()
else:
   conf_10_clients_2_servers()
switch_id = "netxy"