#ifndef NETCACHE_UTIL_H
#define NETCACHE_UTIL_H
/*
 * constants
 */
#define NETLOCK_MODE
#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

#define FAILURE_ROLLBACK        0
#define FAILURE_STOP            1
#define FAILURE_NORMAL          2

#define MEM_BIN_PACK            'b'
#define MEM_RAND_WEIGHT         'w'
#define MEM_RAND_12             '1'
#define MEM_RAND_200            '2'

#define PRIMARY_BACKUP          1
#define SECONDARY_BACKUP        2
#define FAILURE_NOTIFICATION    3

#define MAX_CLIENT_NUM          8

#define SYNCHRONOUS             's'
#define ASYNCHRONOUS            'a'
#define SAMPLE_SIZE             10000

#define RING_SIZE               16384
#define MAX_PKTS_BURST          32

#define MICROBENCHMARK_SHARED   's'
#define MICROBENCHMARK_EXCLUSIVE 'x'
#define NORMALBENCHMARK         'n'
#define ZIPFBENCHAMRK           'z'
#define UNIFORMBENCHMARK        'u'
#define TPCC_UNIFORMBENCHMARK   'v'
#define TPCCBENCHMARK           't'

#define ROLE_CLIENT             'c'
#define ROLE_SERVER_POOL        's'

#define NOT_RECIRCULATE         0x00
#define ACQUIRE_LOCK            0x00
#define RELEASE_LOCK            0x01
#define PUSH_BACK_LOCK          0x02
#define GRANT_LOCK_FROM_SERVER  0x03
#define REJECT_LOCK_ACQUIRE     0x04

#define SHARED_LOCK             0x00
#define EXCLUSIVE_LOCK          0x01

#define MAX_TXN_NUM             1000000

#define MAX_LOCK_NUM            7010000
#define MAX_LOCK_NUM_IN_SWITCH  60000
#define LEN_ONE_SEGMENT         100

#define NC_MAX_PAYLOAD_SIZE     1500
#define NC_NB_MBUF              8191
#define NC_MBUF_SIZE            (2048+sizeof(struct rte_mbuf)+RTE_PKTMBUF_HEADROOM)
#define NC_MBUF_CACHE_SIZE      32
#define NC_MAX_BURST_SIZE       32
#define NC_MAX_LCORES           32
#define NC_NB_RXD               128 // RX descriptors
#define NC_NB_TXD               512 // TX descriptors
#define NC_RX_QUEUE_PER_LCORE   4
#define NC_TX_QUEUE_PER_LCORE   4
#define NC_DRAIN_US             10

#define TYPE_GET_REQUEST            0x00
#define TYPE_PUT_REQUEST            0x01
#define TYPE_GET_RESPONSE           0x02
#define TYPE_PUT_RESPONSE           0x03
#define TYPE_GET_RESPONSE_B         0x04
#define TYPE_PUT_RESPONSE_B         0x05
#define TYPE_WRITE_CACHED           0x12
#define TYPE_DELETE_CACHED          0x13
#define TYPE_CACHE_UPDATE           0x14
#define TYPE_CACHE_UPDATE_RESPONSE  0x15
#define TYPE_HOT_READ               0x16
#define TYPE_RESET_COUNTER          0x20

#define NODE_NUM                128
#define IP_SRC                  "10.1.0.17"
#define CLIENT_SLAVE_IP         "10.1.0.13"
#define IP_DST                  "10.1.0.15"
#define CLIENT_PORT             8900
#define SERVICE_PORT            8900
#define KEY_SPACE_SIZE          1000000
#define PER_NODE_KEY_SPACE_SIZE (KEY_SPACE_SIZE / NODE_NUM)
#define BIN_SIZE                100
#define BIN_RANGE               10
#define BIN_MAX                 (BIN_SIZE * BIN_RANGE)

#define TRACE_LENGTH            (1000*1000*10)
#define SIMULATION_LENGTH       (1000*1000*10)
#define VALUE_SIZE              16
#define CACHE_SIZE              10000

#define CHANGE_SIZE             200
#define CHANGE_INTERVAL         10
#define CHANGE_PATTERN          0

#define CONTROLLER_IP           "10.201.124.31"
#define CONTROLLER_PORT         8890
#define CLIENT_MASTER_IP        "127.0.0.1"
#define CLIENT_MASTER_PORT      8891

#define RSS_HASH_KEY_LENGTH 40

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

int num_of_cores = 8;
int batch_size = 1;
uint32_t *batch_map = NULL;
int locks_each_txn[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint32_t warehouse = 1;
int machine_id = 0;
int contention_degree = 2;
int client_num = 1;
int lock_num = 1;
char task_id = 's';
uint64_t think_time = 0;
int slot_num = 130000;
char data_transfer_mode = ASYNCHRONOUS;
char benchmark = MICROBENCHMARK_SHARED;
char role = ROLE_CLIENT;

static uint8_t hash_key[RSS_HASH_KEY_LENGTH] = {
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, };
    
   /*
static uint8_t hash_key[RSS_HASH_KEY_LENGTH] = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, };
    */
static const struct rte_eth_conf port_conf_client = {
    .rxmode = {
        .mq_mode = ETH_MQ_RX_RSS,
        .max_rx_pkt_len = ETHER_MAX_LEN,
        .split_hdr_size = 0,
        .header_split   = 0, // Header Split disabled
        .hw_ip_checksum = 0, // IP checksum offload disabled
        .hw_vlan_filter = 0, // VLAN filtering disabled
        .jumbo_frame    = 0, // Jumbo Frame Support disabled
        .hw_strip_crc   = 0, // CRC stripped by hardware
    },
    
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = hash_key,
            .rss_hf = ETH_RSS_UDP,
        },
    },
    
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};

static const struct rte_eth_conf port_conf_server = {
    .rxmode = {
         .mq_mode = ETH_MQ_RX_RSS,
        .max_rx_pkt_len = ETHER_MAX_LEN,
        .split_hdr_size = 0,
        .header_split   = 0, // Header Split disabled
        .hw_ip_checksum = 0, // IP checksum offload disabled
        .hw_vlan_filter = 0, // VLAN filtering disabled
        .jumbo_frame    = 0, // Jumbo Frame Support disabled
        .hw_strip_crc   = 0, // CRC stripped by hardware
    },
    
    
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = hash_key,
            .rss_hf = ETH_RSS_UDP,
            //.rss_hf = ETH_RSS_PORT,
        },
    },
    
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};


/*
 * custom types
 */

typedef struct  ProbeHeader_ {
    uint8_t     failure_status;
    uint8_t     op_type;
    uint8_t     mode;
    uint8_t     client_id;
    uint32_t    txnID;
    uint32_t    lockID;
    uint64_t    payload;
} __attribute__((__packed__)) ProbeHeader;

typedef struct  MessageHeader_ {
    uint8_t     recircFlag;
    uint8_t     op_type;
    uint8_t     mode;
    uint8_t     client_id;
    uint32_t    txnID;
    uint32_t    lockID;
    uint64_t    payload;
} __attribute__((__packed__)) MessageHeader;

struct mbuf_table {
    uint32_t len;
    struct rte_mbuf *m_table[NC_MAX_BURST_SIZE];
};

struct lcore_configuration {
    uint32_t vid; // virtual core id
    uint32_t port; // one port
    uint32_t tx_queue_id; // one TX queue
    uint32_t n_rx_queue;  // number of RX queues
    uint32_t rx_queue_list[NC_RX_QUEUE_PER_LCORE]; // list of RX queues
    struct mbuf_table tx_mbufs; // mbufs to hold TX queue
} __rte_cache_aligned;

struct throughput_statistics {
    uint64_t tx;
    uint64_t rx;
    uint64_t rx_read;
    uint64_t rx_write;
    uint64_t txn;
    uint64_t dropped;
    uint64_t last_tx;
    uint64_t last_rx;
    uint64_t last_rx_read;
    uint64_t last_rx_write;
    uint64_t last_dropped;
    uint64_t last_txn;
} __rte_cache_aligned;

/*
 * global variables
 */

uint32_t num_worker = 9;

uint32_t enabled_port_mask = 1;
uint32_t enabled_ports[RTE_MAX_ETHPORTS];
uint32_t n_enabled_ports = 0;
uint32_t n_rx_queues = 0;
uint32_t n_lcores = 0;
uint32_t n_rcv_cores = 0;

struct rte_mempool *pktmbuf_pool = NULL;
struct ether_addr port_eth_addrs[RTE_MAX_ETHPORTS];
struct lcore_configuration lcore_conf[NC_MAX_LCORES];
struct throughput_statistics tput_stat[NC_MAX_LCORES];
struct throughput_statistics tput_stat_avg[NC_MAX_LCORES];
struct throughput_statistics tput_stat_total;

// uint64_t txn_rate_stat[NC_MAX_LCORES];

uint8_t header_template[
    sizeof(struct ether_hdr)
    + sizeof(struct ipv4_hdr)
    + sizeof(struct udp_hdr)];

/*
 * display memmory block
 */
void mem_display(const void *address, int len) {
    const unsigned char *p = address;
    DEBUG_PRINT("MEMORY DUMP\n");
    for (size_t i = 0; i < len; i++) {
        DEBUG_PRINT("%02hhx", p[i]);
        if (i % 2 == 1)
            DEBUG_PRINT(" ");
        if (i % 16 == 15)
            DEBUG_PRINT("\n");
    }
    if (len % 16 != 0)
        DEBUG_PRINT("\n");
}

/*
 * functions for generation
 */


static void print_burst(uint32_t lcore_id) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf **m_table = (struct rte_mbuf **)lconf->tx_mbufs.m_table;
    struct rte_mbuf *mbuf;
    for (int i=0; i< lconf->tx_mbufs.len; i++) {
        mbuf = m_table[i];
        struct ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
        struct ipv4_hdr *ip = (struct ipv4_hdr *)((uint8_t*) eth
            + sizeof(struct ether_hdr));
        struct udp_hdr *udp = (struct udp_hdr *)((uint8_t*) ip
            + sizeof(struct ipv4_hdr));
        MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + sizeof(header_template));
        uint8_t op_t = message_header->op_type;
        uint8_t mode = message_header->mode;
        uint32_t txn_id = ntohl(message_header->txnID);

        uint32_t lock_id = ntohl(message_header->lockID);
    }
}

// send packets, drain TX queue
static void send_pkt_burst(uint32_t lcore_id) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf **m_table = (struct rte_mbuf **)lconf->tx_mbufs.m_table;

    uint32_t n = lconf->tx_mbufs.len;

    uint32_t ret = rte_eth_tx_burst(
        lconf->port,
        lconf->tx_queue_id,
        m_table,
        lconf->tx_mbufs.len);
    tput_stat[lcore_id].tx += ret;
    if (unlikely(ret < n)) {
        tput_stat[lcore_id].dropped += (n - ret);
        do {
            rte_pktmbuf_free(m_table[ret]);
        } while (++ret < n);
    }
    lconf->tx_mbufs.len = 0;
}

// put packet into TX queue
static void enqueue_pkt(uint32_t lcore_id, struct rte_mbuf *mbuf) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    lconf->tx_mbufs.m_table[lconf->tx_mbufs.len++] = mbuf;

    // enough packets in TX queue
    if (unlikely(lconf->tx_mbufs.len == NC_MAX_BURST_SIZE)) {
    // if (unlikely(lconf->tx_mbufs.len == 1)) {
        send_pkt_burst(lcore_id);
    }
}

static void acquire_enqueue_pkt(uint32_t lcore_id, struct rte_mbuf *mbuf, int locks_per_txn) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    lconf->tx_mbufs.m_table[lconf->tx_mbufs.len++] = mbuf;

    // enough packets in TX queue
    // if (unlikely(lconf->tx_mbufs.len == NC_MAX_BURST_SIZE)) {
    if (unlikely(lconf->tx_mbufs.len == locks_per_txn)) {
        send_pkt_burst(lcore_id);
    }
}

/*
 * functions for initialization
 */

// init header template
static void init_header_template(void) {
    memset(header_template, 0, sizeof(header_template));
    struct ether_hdr *eth = (struct ether_hdr *)header_template;
    struct ipv4_hdr *ip = (struct ipv4_hdr *)((uint8_t*) eth + sizeof(struct ether_hdr));
    struct udp_hdr *udp = (struct udp_hdr *)((uint8_t*)ip + sizeof(struct ipv4_hdr));
    uint32_t pkt_len = sizeof(header_template) + sizeof(MessageHeader);

    // eth header
    eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    struct ether_addr src_addr = {
        .addr_bytes = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}};
    struct ether_addr dst_addr = {
        .addr_bytes = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    //ether_addr_copy(&port_eth_addrs[0], &eth->s_addr);
    ether_addr_copy(&src_addr, &eth->s_addr);
    ether_addr_copy(&dst_addr, &eth->d_addr);

    // ip header
    char src_ip[] = IP_SRC;
    char dst_ip[] = IP_DST;
    int st1 = inet_pton(AF_INET, src_ip, &(ip->src_addr));
    int st2 = inet_pton(AF_INET, dst_ip, &(ip->dst_addr));
    if(st1 != 1 || st2 != 1) {
        fprintf(stderr, "inet_pton() failed.Error message: %s %s",
            strerror(st1), strerror(st2));
        exit(EXIT_FAILURE);
    }
    ip->total_length = rte_cpu_to_be_16(pkt_len - sizeof(struct ether_hdr));
    ip->version_ihl = 0x45;
    ip->type_of_service = 0;
    ip->packet_id = 0;
    ip->fragment_offset = 0;
    ip->time_to_live = 64;
    ip->next_proto_id = IPPROTO_UDP;
    uint32_t ip_cksum;
    uint16_t *ptr16 = (uint16_t *)ip;
    ip_cksum = 0;
    ip_cksum += ptr16[0]; ip_cksum += ptr16[1];
    ip_cksum += ptr16[2]; ip_cksum += ptr16[3];
    ip_cksum += ptr16[4];
    ip_cksum += ptr16[6]; ip_cksum += ptr16[7];
    ip_cksum += ptr16[8]; ip_cksum += ptr16[9];
    ip_cksum = ((ip_cksum & 0xffff0000) >> 16) + (ip_cksum & 0x0000ffff);
    if (ip_cksum > 65535) {
        ip_cksum -= 65535;
    }
    ip_cksum = (~ip_cksum) & 0x0000ffff;
    if (ip_cksum == 0) {
        ip_cksum = 0xffff;
    }
    ip->hdr_checksum = (uint16_t)ip_cksum;

    // udp header
    udp->src_port = htons(CLIENT_PORT);
    udp->dst_port = htons(SERVICE_PORT);
    udp->dgram_len = rte_cpu_to_be_16(pkt_len
        - sizeof(struct ether_hdr)
        - sizeof(struct ipv4_hdr));
    udp->dgram_cksum = 0;
}

// check link status
static void check_link_status(void) {
    const uint32_t check_interval_ms = 100;
    const uint32_t check_iterations = 90;
    uint32_t i, j;
    struct rte_eth_link link;
    for (i = 0; i < check_iterations; i++) {
        uint8_t all_ports_up = 1;
        for (j = 0; j < n_enabled_ports; j++) {
            uint32_t portid = enabled_ports[j];
            memset(&link, 0, sizeof(link));
            rte_eth_link_get_nowait(portid, &link);
            if (link.link_status) {
                printf("\tport %u link up - speed %u Mbps - %s\n",
                    portid,
                    link.link_speed,
                    (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
                        "full-duplex" : "half-duplex");
            } else {
                all_ports_up = 0;
            }
        }

        if (all_ports_up == 1) {
            printf("check link status finish: all ports are up\n");
            break;
        } else if (i == check_iterations - 1) {
            printf("check link status finish: not all ports are up\n");
        } else {
            rte_delay_ms(check_interval_ms);
        }
    }
}

// initialize all status
static void nc_init(void) {
    uint32_t i, j;

    // create mbuf pool
    printf("create mbuf pool\n");
    pktmbuf_pool = rte_mempool_create(
        "mbuf_pool",
        NC_NB_MBUF,
        NC_MBUF_SIZE,
        NC_MBUF_CACHE_SIZE,
        sizeof(struct rte_pktmbuf_pool_private),
        rte_pktmbuf_pool_init, NULL,
        rte_pktmbuf_init, NULL,
        rte_socket_id(),
        0);
    if (pktmbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "cannot init mbuf pool\n");
    }

    // determine available ports
    printf("create enabled ports\n");
    uint32_t n_total_ports = 0;
    n_total_ports = rte_eth_dev_count();

    if (n_total_ports == 0) {
      rte_exit(EXIT_FAILURE, "cannot detect ethernet ports\n");
    }
    if (n_total_ports > RTE_MAX_ETHPORTS) {
        n_total_ports = RTE_MAX_ETHPORTS;
    }

    // get info for each enabled port 
    struct rte_eth_dev_info dev_info;
    n_enabled_ports = 0;
    printf("\tports: ");
    
    for (i = 0; i < n_total_ports; i++) {
        if ((enabled_port_mask & (1 << i)) == 0) {
            continue;
        }
        enabled_ports[n_enabled_ports++] = i;
        rte_eth_dev_info_get(i, &dev_info);
        printf("%u ", i);
    }
    printf("\n");

    // find number of active lcores
    printf("create enabled cores\n\tcores: ");
    n_lcores = 0;
    for(i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            n_lcores++;
            printf("%u ",i);
        }
    }

    // ensure numbers are correct
    if (n_lcores % n_enabled_ports != 0) {
        rte_exit(EXIT_FAILURE,
            "number of cores (%u) must be multiple of ports (%u)\n",
            n_lcores, n_enabled_ports);
    }

    uint32_t rx_queues_per_lcore = NC_RX_QUEUE_PER_LCORE;

    uint32_t rx_queues_per_port = rx_queues_per_lcore * n_lcores / n_enabled_ports;

#ifdef NETLOCK_MODE
    if (role == ROLE_CLIENT)
        rx_queues_per_port = rx_queues_per_lcore * n_rcv_cores / n_enabled_ports;
#endif

    uint32_t tx_queues_per_port = n_lcores / n_enabled_ports;

    if (rx_queues_per_port < rx_queues_per_lcore) {
        rte_exit(EXIT_FAILURE,
            "rx_queues_per_port (%u) must be >= rx_queues_per_lcore (%u)\n",
            rx_queues_per_port, rx_queues_per_lcore);
    }

    // assign each lcore some RX queues and a port
    printf("set up %d RX queues per port and %d TX queues per port\n",
        rx_queues_per_port, tx_queues_per_port);
    uint32_t portid_offset = 0;
    uint32_t rx_queue_id = 0;
    uint32_t tx_queue_id = 0;
    uint32_t vid = 0;
    for (i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            lcore_conf[i].vid = vid++;
#ifdef NETLOCK_MODE
            if ((i >= n_rcv_cores) && (role == ROLE_CLIENT))
                lcore_conf[i].n_rx_queue = 0;
            else
#endif
                lcore_conf[i].n_rx_queue = rx_queues_per_lcore;
            for (j = 0; j < lcore_conf[i].n_rx_queue; j++) {
                lcore_conf[i].rx_queue_list[j] = rx_queue_id++;
            }
            lcore_conf[i].port = enabled_ports[portid_offset];
            lcore_conf[i].tx_queue_id = tx_queue_id++;
            if (tx_queue_id % tx_queues_per_port == 0) {
                portid_offset++;
                rx_queue_id = 0;
                tx_queue_id = 0;
            }
        }
    }
#ifdef NETLOCK_MODE
    for (i=0;i<NC_MAX_LCORES;i++) {

        DEBUG_PRINT("l_core_%d, n_rx:%d\n", i, lcore_conf[i].n_rx_queue);
        DEBUG_PRINT("l_core_%d, tx_q_id:%d\n",i, lcore_conf[i].tx_queue_id);
        for (j = 0; j < lcore_conf[i].n_rx_queue; j++) {
            DEBUG_PRINT("rx_q[%d]:%d\n", j, lcore_conf[i].rx_queue_list[j]);
        }
    }
    DEBUG_PRINT("rx_queues_per_port:%d\n", rx_queues_per_port);
#endif
    // initialize each port
    for (portid_offset = 0; portid_offset < n_enabled_ports; portid_offset++) {
        uint32_t portid = enabled_ports[portid_offset];
        int32_t ret;
        if (role == ROLE_CLIENT) {
            ret = rte_eth_dev_configure(portid, rx_queues_per_port,
                tx_queues_per_port, &port_conf_client);
        }
        else {
            ret = rte_eth_dev_configure(portid, rx_queues_per_port,
                tx_queues_per_port, &port_conf_server);
        }
        if (ret < 0) {
            rte_exit(EXIT_FAILURE, "cannot configure device: err=%d, port=%u\n",
               ret, portid);
        }
        rte_eth_macaddr_get(portid, &port_eth_addrs[portid]);

        // initialize RX queues
        for (i = 0; i < rx_queues_per_port; i++) {
            ret = rte_eth_rx_queue_setup(portid, i, NC_NB_RXD,
                rte_eth_dev_socket_id(portid), NULL, pktmbuf_pool);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                    "rte_eth_rx_queue_setup: err=%d, port=%u\n", ret, portid);
            }
         }

        // initialize TX queues
        for (i = 0; i < tx_queues_per_port; i++) {
            ret = rte_eth_tx_queue_setup(portid, i, NC_NB_TXD,
                rte_eth_dev_socket_id(portid), NULL);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                    "rte_eth_tx_queue_setup: err=%d, port=%u\n", ret, portid);
            }
        }

        // start device
        ret = rte_eth_dev_start(portid);
        if (ret < 0) {
            rte_exit(EXIT_FAILURE,
                "rte_eth_dev_start: err=%d, port=%u\n", ret, portid);
        }

        rte_eth_promiscuous_enable(portid);

        char mac_buf[ETHER_ADDR_FMT_SIZE];
        ether_format_addr(mac_buf, ETHER_ADDR_FMT_SIZE, &port_eth_addrs[portid]);
        printf("initiaze queues and start port %u, MAC address:%s\n",
           portid, mac_buf);
    }

    if (!n_enabled_ports) {
        rte_exit(EXIT_FAILURE, "all available ports are disabled. Please set portmask.\n");
    }
    check_link_status();
    init_header_template();
}

/*
 * functions for print
 */

// print current time
static void print_time(void) {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("%s\n", buffer);
}

// print packet
/*static void print_packet(struct rte_mbuf *mbuf) {
    // print length
    printf("packet: pkt_len:%"PRIu32" data_len:%"PRIu16"\n", mbuf->pkt_len, mbuf->data_len);

    // print eth
    struct ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    char mac_buf[ETHER_ADDR_FMT_SIZE];
    ether_format_addr(mac_buf, ETHER_ADDR_FMT_SIZE, &eth->s_addr);
    printf("\teth: src_mac:%s", mac_buf);
    ether_format_addr(mac_buf, ETHER_ADDR_FMT_SIZE, &eth->d_addr);
    printf(" dst_mac:%s\n", mac_buf);

    // print ip
    struct ipv4_hdr* ip = (struct ipv4_hdr*) ((uint8_t*)eth + sizeof(struct ether_hdr));
    char ip_buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip->src_addr, ip_buf, INET_ADDRSTRLEN);
    printf("\tip: src_ip:%s", ip_buf);
    inet_ntop(AF_INET, &ip->dst_addr, ip_buf, INET_ADDRSTRLEN);
    printf(" dst_ip:%s\n", ip_buf);

    // print payload
    MessageHeader* message_header = (MessageHeader*) ((uint8_t*)eth + sizeof(header_template));
    if (message_header->type == TYPE_GET_REQUEST) {
        printf("\tmessage type: %"PRIu8"\tseq: %"PRIu64"\tkey: %"PRIu64"\n",
            message_header->type, message_header->seq,
            rte_be_to_cpu_64(message_header->key));
    } else if (message_header->type == TYPE_PUT_REQUEST
        || message_header->type == TYPE_GET_RESPONSE_B
        || message_header->type == TYPE_GET_RESPONSE_C
        || message_header->type == TYPE_PUT_RESPONSE_B) {
        
        uint8_t* value = (uint8_t *) eth
            + sizeof(header_template) + sizeof(MessageHeader);
        uint32_t i;

        printf("\tmessage type: %"PRIu8"\tseq: %"PRIu64"\tkey: %"PRIu64
            "\tvalue: ",
            message_header->type, message_header->seq,
            rte_be_to_cpu_64(message_header->key));
        for (i = 0; i < VALUE_SIZE; i++) {
            printf("%"PRIu64" ", *((uint64_t*)(value + i * 8)));
        }
        printf("\n");
    } else if (message_header->type == TYPE_WRITE_CACHED) {
        uint8_t* bitmap = (uint8_t *) eth
            + sizeof(header_template) + sizeof(MessageHeader);
        uint8_t* value = bitmap + 1;
        uint32_t i;

        printf("\tmessage type: %"PRIu8"\tseq: %"PRIu64"\tkey: %"PRIu64
            "\tbitmap:%"PRIu8"\tvalue: ",
            message_header->type, message_header->seq,
            rte_be_to_cpu_64(message_header->key),
            *bitmap);
        for (i = 0; i < VALUE_SIZE; i++) {
            printf("%"PRIu64" ", *((uint64_t*)(value + i * 8)));
        }
        printf("\n");
    } else if (message_header->type == TYPE_CACHE_UPDATE
        || message_header->type == TYPE_CACHE_UPDATE_RESPONSE
        || message_header->type == TYPE_PUT_RESPONSE_C) {
        uint8_t* bitmap1 = (uint8_t *) eth
            + sizeof(header_template) + sizeof(MessageHeader);
        uint8_t* value1 = bitmap1 + 1;
        uint8_t* bitmap2 = value1 + VALUE_SIZE / 2 * 8;
        uint8_t* value2 = bitmap2 + 1;
        uint32_t i;

        printf("\tmessage type: %"PRIu8"\tseq: %"PRIu64"\tkey: %"PRIu64
            "\tbitmap1:%"PRIu8"\tvalue1: ",
            message_header->type, message_header->seq,
            rte_be_to_cpu_64(message_header->key),
            *bitmap1);
        for (i = 0; i < VALUE_SIZE/2; i++) {
            printf("%"PRIu64" ", *((uint64_t*)(value1 + i * 8)));
        }
        printf("\tbitmap2:%"PRIu8"\tvalue2: ", *bitmap2);
        for (i = 0; i < VALUE_SIZE/2; i++) {
            printf("%"PRIu64" ", *((uint64_t*)(value2 + i * 8)));
        }
        printf("\n");
    } else if (message_header->type == TYPE_HOT_READ) {
        uint16_t* value = (uint16_t *) eth
            + sizeof(header_template) + sizeof(MessageHeader);
        printf("\tmessage type: %"PRIu8"\tseq: %"PRIu64"\tkey: %"PRIu64
            "\tv0: %"PRIu16"\tv1: %"PRIu16"\tv2: %"PRIu16"\tv3: %"PRIu16
            "\n",
            message_header->type, message_header->seq,
            rte_be_to_cpu_64(message_header->key),
            *value, *(value+2), *(value+4), *(value+6));
    } else if (message_header->type == TYPE_RESET_COUNTER) {
        printf("\tmessage type: %"PRIu8"\tseq: %"PRIu64"\tkey: %"PRIu64"\n",
            message_header->type, message_header->seq,
            rte_be_to_cpu_64(message_header->key));
    }

    uint8_t* value = (uint8_t *) eth
        + sizeof(header_template) + sizeof(MessageHeader);
    uint32_t i;
    for (i = 0; i < VALUE_SIZE; i++) {
        printf("%"PRIu64" ", *((uint64_t*)(value + i * 8)));
    }
    printf("\n");
}*/

// print per-core throughput
static void print_per_core_throughput(void) {
    // time is in second
    printf("%lld\nthroughput\n", (long long)time(NULL));
    uint32_t i;
    uint64_t total_tx = 0;
    uint64_t total_rx = 0;
    uint64_t total_dropped = 0;
    static uint64_t last_tx = 0;
    static uint64_t last_rx = 0;
    for (i = n_rcv_cores; i < n_lcores; i++) {
        // total_tx += tput_stat[i].tx;
        total_rx += tput_stat[i].rx;
    }
    // fprintf(f_pr, "%lu\n", total_rx - last_rx);
    printf("rx_pers:%lu\n", total_rx - last_rx);
    // fprintf(f_pr, "%lu,%lu,%lu,%lu\n", tput_stat[4].rx, tput_stat[5].rx, tput_stat[6].rx, tput_stat[7].rx);
    // fflush(f_pr);
    // fprintf(f_pr, "%lu\n", total_rx - last_rx);
    
    // fflush(f_pr, "%lu,%lu,%lu,%lu", );
    // last_tx = total_tx;
    fflush(stdout);
    last_rx = total_rx;
    
    //for (i = 0; i < n_lcores; i++) {
    for (i = 0; i < n_lcores; i++) {
        printf("\tcore %"PRIu32"\t"
            "tx: %"PRIu64"\t"
            "rx: %"PRIu64"\t"
            "rx_read: %"PRIu64"\t"
            "rx_write: %"PRIu64"\t"
            "dropped: %"PRIu64"\n",
            i, tput_stat[i].tx,
            tput_stat[i].rx,
            tput_stat[i].rx_read,
            tput_stat[i].rx_write,
            tput_stat[i].dropped);

        /*
        total_tx += tput_stat[i].tx - tput_stat[i].last_tx;
        total_rx += tput_stat[i].rx - tput_stat[i].last_rx;
        total_dropped += tput_stat[i].dropped - tput_stat[i].last_dropped;
        tput_stat[i].last_tx = tput_stat[i].tx;
        tput_stat[i].last_rx = tput_stat[i].rx;
        tput_stat[i].last_rx_read = tput_stat[i].rx_read;
        tput_stat[i].last_rx_write = tput_stat[i].rx_write;
        tput_stat[i].last_dropped = tput_stat[i].dropped;*/
        /*
        printf("\tcore %"PRIu32"\t"
            "tx: %"PRIu64"\t"
            "rx: %"PRIu64"\t"
            "rx_read: %"PRIu64"\t"
            "rx_write: %"PRIu64"\t"
            "dropped: %"PRIu64"\n",
            i, tput_stat[i].tx - tput_stat[i].last_tx,
            tput_stat[i].rx - tput_stat[i].last_rx,
            tput_stat[i].rx_read - tput_stat[i].last_rx_read,
            tput_stat[i].rx_write - tput_stat[i].last_rx_write,
            tput_stat[i].dropped - tput_stat[i].last_dropped);
        total_tx += tput_stat[i].tx - tput_stat[i].last_tx;
        total_rx += tput_stat[i].rx - tput_stat[i].last_rx;
        total_dropped += tput_stat[i].dropped - tput_stat[i].last_dropped;
        tput_stat[i].last_tx = tput_stat[i].tx;
        tput_stat[i].last_rx = tput_stat[i].rx;
        tput_stat[i].last_rx_read = tput_stat[i].rx_read;
        tput_stat[i].last_rx_write = tput_stat[i].rx_write;
        tput_stat[i].last_dropped = tput_stat[i].dropped;
        */
    }
    printf("\ttotal\ttx: %"PRIu64"\t"
        "rx: %"PRIu64"\t"
        "dropped: %"PRIu64"\n",
        total_tx, total_rx, total_dropped);
    fflush(stdout);
}

// print throughput
static void print_throughput(void) {
    // time is in second
    printf("%lld\nthroughput ", (long long)time(NULL));
    uint32_t i;
    uint64_t total_tx = 0;
    uint64_t total_rx = 0;
    uint64_t total_dropped = 0;
    for (i = 0; i < n_lcores; i++) {
        total_tx += tput_stat[i].tx - tput_stat[i].last_tx;
        total_rx += tput_stat[i].rx - tput_stat[i].last_rx;
        total_dropped += tput_stat[i].dropped - tput_stat[i].last_dropped;
        tput_stat[i].last_tx = tput_stat[i].tx;
        tput_stat[i].last_rx = tput_stat[i].rx;
        tput_stat[i].last_dropped = tput_stat[i].dropped;
    }
    printf("tx: %"PRIu64"\t"
        "rx: %"PRIu64"\t"
        "dropped: %"PRIu64"\n",
        total_tx, total_rx, total_dropped);
}


/*
 * misc
 */

static uint64_t timediff_in_us(uint64_t new_t, uint64_t old_t) {
    return (new_t - old_t) * 1000000UL / rte_get_tsc_hz();
}

#endif //NETCACHE_UTIL_H
