#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/queue.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdbool.h>


#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_ethdev.h>

#include <unistd.h>

#include "util.h"
#include "lock_queue.h"
#include "think_queue.h"
#include "txn_queue.h"
#include "new_order.h"

#define COLLECT_LATENCY
#define DEBUG
#undef DEBUG


#include "zipf.h"

/*
 * constants
 */

#define EXTENSIVE_LOCKS_NORMAL_BENCHMARK 20
#define OBJ_PER_LOCK_NORMAL_BENCHMARK 150
#define EXTENSIVE_LOCKS_MICRO_BENCHMARK_SHARED 55000
#define OBJ_PER_LOCK_MICRO_BENCHMARK_SHARED 1
#define EXTENSIVE_LOCKS_MICRO_BENCHMARK_EXCLUSIVE 55000
#define OBJ_PER_LOCK_MICRO_BENCHMARK_EXCLUSIVE 1
#define EXTENSIVE_LOCKS_TPCC 7000000
#define OBJ_PER_LOCK_TPCC 1

#define MIN_LOSS_RATE           0.01
#define MAX_LOSS_RATE           0.05
#define PKTS_SEND_LIMIT_MIN_MS  300
#define PKTS_SEND_LIMIT_MAX_MS  2500
#define PKTS_SEND_RESTART_MS    300
#define NUM_LCORES              32

/*
 * custom types
 */
int timeout_slot = 400;
txn_queue_list txn_queues[MAX_CLIENT_NUM][MAX_TXN_NUM];

int current_failure_status = 0;
int last_failure_status = 0;
int client_node_num = 8;
int server_node_num = 1;
uint32_t last_txn_idx[MAX_CLIENT_NUM] = {0};
uint32_t idx[MAX_CLIENT_NUM] = {0};
char deadlocked[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
char txn_finished[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint8_t failure_act[MAX_CLIENT_NUM];
int txn_s[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
int txn_r[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
int num_retries[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
int detect_failure[MAX_CLIENT_NUM] = {0};
uint64_t txn_refresh_time[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint64_t txn_refresh_time_failure[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint64_t txn_begin_time[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint64_t txn_finish_time[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};

char memn_filename[200], memory_management;
char dir_prefix[100];
uint32_t num_ex[MAX_LOCK_NUM] = {0};
uint32_t num_sh[MAX_LOCK_NUM] = {0};
char busy[MAX_CLIENT_NUM][MAX_LOCK_NUM] = {0};
uint8_t client_index_s[NUM_LCORES] = {0};
uint8_t client_index_e[NUM_LCORES] = {0};
uint32_t ip_dst_pton, ip_src_pton;
int extensive_locks = EXTENSIVE_LOCKS_NORMAL_BENCHMARK;
int obj_per_lock = OBJ_PER_LOCK_NORMAL_BENCHMARK;
int occupying_flag[MAX_LOCK_NUM] = {0};

struct latency_statistics {
    uint64_t max;
    uint64_t num;
    uint64_t total;
    uint64_t overflow;
    uint64_t bin[BIN_SIZE];
} __rte_cache_aligned;

/*
 * global variables
 */

// key-value workload generation

float rate_adjust_per_sec = 1;
uint32_t average_interval = 64;
uint32_t write_ratio = 0;
uint32_t wpkts_send_limit_ms = 10000;
uint32_t wpkts_send_limit_ms_rec = 20000;
// uint32_t wpkts_send_limit_ms = 100; CHANGE HERE
uint32_t zipf_alpha = 90;
uint64_t pkts_send_limit_ms = 20;

// destination ip address

char ip_list[][32] = {
    "10.1.0.1",
    "10.1.0.2",
    "10.1.0.3",
    "10.1.0.4",
    "10.1.0.5",
    "10.1.0.6",
    "10.1.0.7",
    "10.1.0.8",
    "10.1.0.9",
    "10.1.0.10",
    "10.1.0.11",
    "10.1.0.12",
    "10.1.0.100"
    };
char ip_src[32] = "10.1.0.3";
char ip_dst[32] = "10.1.0.4";
uint16_t port_read = 8880;
uint16_t port_write = 8888;
uint16_t port_probe = 9998;
uint32_t socket_number = 3;

// generate packets with zipf
struct zipf_gen_state *zipf_state;

// statistics
struct latency_statistics latency_stat_c[NC_MAX_LCORES];
struct latency_statistics latency_stat_b[NC_MAX_LCORES];
struct latency_statistics latency_stat_avg[NC_MAX_LCORES];

static uint64_t latency_samples_together[1000000] = {0};
uint64_t latency_samples_together_len = 0;
static uint64_t latency_samples[NC_MAX_LCORES][100000] = {0};
uint64_t latency_sample_num[NC_MAX_LCORES] = {0};
uint64_t latency_sample_interval = 1000;
uint64_t latency_sample_start = 0;

static uint32_t second = 0;
static uint64_t throughput_per_second[3600] = {0};

// adjust client rate
uint32_t adjust_start = 0;
uint64_t last_sent = 0;
uint64_t last_recv = 0;


// length of the queues in the switch
int len_in_switch[MAX_LOCK_NUM];
int count_secondary = 0;
int secondary_locks[MAX_LOCK_NUM];


// tpc-c

uint32_t random_C = 1;

uint32_t *(txn_id[MAX_CLIENT_NUM]);
uint32_t *(action_type[MAX_CLIENT_NUM]);
uint32_t *(target_lm_id[MAX_CLIENT_NUM]);
uint32_t *(target_obj_idx[MAX_CLIENT_NUM]);
uint32_t *(lock_type[MAX_CLIENT_NUM]);
uint32_t len[MAX_CLIENT_NUM];

#include "collect.h"
/*
 * functions for processing
 */

static void send_netlock_req(uint32_t lcore_id, uint8_t action_type, uint8_t lock_type, uint32_t lock_id, uint32_t txn_id, uint32_t ip_src_addr);

// print latency
static void print_latency(struct latency_statistics * latency_stat) {
    uint64_t max = 0;
    uint64_t num = 0;
    uint64_t total = 0;
    uint64_t overflow = 0;
    uint64_t bin[BIN_SIZE];
    memset(&bin, 0, sizeof(bin));

    uint32_t i, j;
    double average_latency = 0;
    if (latency_stat[0].num > 0) {
        average_latency = latency_stat[0].total / (double)latency_stat[0].num;
    }
    printf("\tcount: %"PRIu64"\t"
        "average latency: %.4f ms\t"
        "max latency: %.4f ms\t"
        "overflow: %"PRIu64"\n",
        latency_stat[0].num, average_latency / 1000.0, latency_stat[0].max / 1000.0, overflow);
}

// generate probe packet
static void generate_probe_pkt(uint32_t lcore_id, struct rte_mbuf *mbuf, uint32_t ip_src_addr, uint32_t ip_dst_addr) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    assert(mbuf != NULL);
    mbuf->next = NULL;
    mbuf->nb_segs = 1;
    mbuf->ol_flags = 0;
    mbuf->data_len = 0;
    mbuf->pkt_len = 0;

    struct ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    struct ipv4_hdr *ip = (struct ipv4_hdr *)((uint8_t*) eth
        + sizeof(struct ether_hdr));
    struct udp_hdr *udp = (struct udp_hdr *)((uint8_t*) ip
        + sizeof(struct ipv4_hdr));

    rte_memcpy(eth, header_template, sizeof(header_template));

    mbuf->data_len += sizeof(header_template);
    mbuf->pkt_len += sizeof(header_template);

    if (ip_src_addr != 0) 
        ip->src_addr = ip_src_addr;
    else 
        inet_pton(AF_INET, ip_src, &(ip->src_addr));

    if (ip_dst_addr != 0)
        ip->dst_addr = ip_dst_addr;
    else
        inet_pton(AF_INET, ip_dst, &(ip->dst_addr));

    udp->src_port = htons(port_probe - 1);
    udp->dst_port = htons(port_probe);

    ProbeHeader* probe_header = (ProbeHeader*) ((uint8_t*)eth + sizeof(header_template));
    probe_header->failure_status = (uint8_t) 0;

    mbuf->data_len += sizeof(ProbeHeader);
    mbuf->pkt_len += sizeof(ProbeHeader);
}

// generate write request packet
static void generate_write_request_pkt(uint32_t lcore_id, 
        struct rte_mbuf *mbuf, uint8_t action_type, uint8_t lock_type, uint32_t lock_id, 
        uint32_t txn_id, uint32_t ip_src_addr, uint32_t ip_dst_addr, uint64_t timestamp, uint8_t client_id) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    assert(mbuf != NULL);

    mbuf->next = NULL;
    mbuf->nb_segs = 1;
    mbuf->ol_flags = 0;
    mbuf->data_len = 0;
    mbuf->pkt_len = 0;

    // init packet header
    struct ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    struct ipv4_hdr *ip = (struct ipv4_hdr *)((uint8_t*) eth
        + sizeof(struct ether_hdr));
    struct udp_hdr *udp = (struct udp_hdr *)((uint8_t*) ip
        + sizeof(struct ipv4_hdr));
    rte_memcpy(eth, header_template, sizeof(header_template));
    mbuf->data_len += sizeof(header_template);
    mbuf->pkt_len += sizeof(header_template);
    
    if (ip_src_addr != 0) 
        ip->src_addr = ip_src_addr;
    else 
        inet_pton(AF_INET, ip_src, &(ip->src_addr));

    if (ip_dst_addr != 0)
        ip->dst_addr = ip_dst_addr;
    else
        inet_pton(AF_INET, ip_dst, &(ip->dst_addr));
    
    udp->src_port = htons(port_write + (uint32_t)txn_id % 128);
    udp->dst_port = htons(port_write);

    MessageHeader* message_header = (MessageHeader*) ((uint8_t*)eth + sizeof(header_template));
    message_header->recircFlag = (uint8_t) NOT_RECIRCULATE;

    message_header->op_type = (uint8_t) action_type;
    message_header->mode = (uint8_t) lock_type;
    message_header->txnID = htonl(txn_id);
    message_header->lockID = htonl(lock_id);

    if (timestamp == 0) {
        message_header->payload = rte_rdtsc();
    } else {
        message_header->payload = timestamp;
    }

    message_header->client_id = client_id;
    

    mbuf->data_len += sizeof(MessageHeader);
    mbuf->pkt_len += sizeof(MessageHeader);
}

static void compute_latency(struct latency_statistics *latency_stat,
    uint64_t latency) {
    latency_stat->num++;
    latency_stat->total += latency;
    if(latency_stat->max < latency) {
        latency_stat->max = latency;
    }
    if(latency < BIN_MAX) {
        latency_stat->bin[latency/BIN_RANGE]++;
    } else {
        latency_stat->overflow++;
    }
}



// ** tpc-c new orders
static void send_netlock_req(uint32_t lcore_id, uint8_t action_type, uint8_t lock_type, uint32_t lock_id, uint32_t txn_id, uint32_t ip_src_addr) {
    struct rte_mbuf *mbuf;
    mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
    generate_write_request_pkt(lcore_id, mbuf, action_type, lock_type, lock_id, txn_id, ip_src_addr, 0, 0, 0);
    enqueue_pkt(lcore_id, mbuf);
}

int cmpfunc (const void * a, const void * b) {
    if (*(uint64_t *) a > *(uint64_t *)b) {
        return 1;
    }
    else if (*(uint64_t *) a < *(uint64_t *)b) {
        return -1;
    }
    else { 
        return 0;
    }
}

// RX loop for test
queue_list *lockqueues;
static int32_t np_client_rx_loop(uint32_t lcore_id) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    printf("%lld entering RX loop (master loop) on lcore %u\n", (long long)time(NULL), lcore_id);

    uint64_t begin_sample = 0;
    int stop_statis = 0;
    struct rte_mbuf *mbuf;
    struct rte_mbuf *mbuf2;
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    uint32_t i, j, nb_rx;

    uint64_t cur_tsc = rte_rdtsc();
    uint64_t ms_tsc = rte_get_tsc_hz() / 1000;
    uint64_t next_ms_tsc = cur_tsc + ms_tsc;
    uint64_t last_drain_tq_tsc = cur_tsc;

    uint64_t update_tsc = rte_get_tsc_hz(); // in second
    uint64_t increment_time = rte_get_tsc_hz() * think_time / 1000000;
    uint64_t next_update_tsc = cur_tsc + update_tsc;
    uint64_t average_start_tsc = cur_tsc + update_tsc * average_interval;
    uint64_t average_end_tsc = cur_tsc + update_tsc * average_interval * 2;
    uint64_t exit_tsc = cur_tsc + update_tsc * average_interval * 20;
    
    // uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * 100;
    uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * 10;
    uint64_t next_drain_tsc = cur_tsc + drain_tsc;

    uint32_t pkts_send_ms = 0;
    
    think_queue_list *thinkqueue;

    while (1) {
        // read current time
        cur_tsc = rte_rdtsc();

        // print stats at master lcore
        if ((lcore_id == 0) && (update_tsc > 0)) {
            if (unlikely(cur_tsc > next_update_tsc)) {
                print_per_core_throughput();
                // printf("latency\n");
                // print_latency(latency_stat_c);
                //print_latency(latency_stat_b);
                next_update_tsc += update_tsc;
            }

            
            if (unlikely(average_start_tsc > 0 && cur_tsc > average_start_tsc)) {
                uint64_t total_tx = 0;
                uint64_t total_rx = 0;
                uint64_t total_rx_read = 0;
                uint64_t total_rx_write = 0;
                uint64_t total_dropped = 0;
                uint64_t total_txn = 0;
                
                for (i = 0; i < n_lcores; i++) {
                    tput_stat_avg[i].last_tx = tput_stat[i].tx;
                    tput_stat_avg[i].last_rx = tput_stat[i].rx;
                    tput_stat_avg[i].last_rx_read = tput_stat[i].rx_read;
                    tput_stat_avg[i].last_rx_write = tput_stat[i].rx_write;
                    tput_stat_avg[i].last_dropped = tput_stat[i].dropped;

                    tput_stat_avg[i].txn = tput_stat[i].txn;
                    
                    total_tx += tput_stat[i].tx;
                    total_rx += tput_stat[i].rx;
                    total_rx_read += tput_stat[i].rx_read;
                    total_rx_write += tput_stat[i].rx_write;
                    total_dropped += tput_stat[i].dropped;
                    total_txn += tput_stat[i].txn;

                    latency_stat_avg[i].total = latency_stat_c[i].total;
                    latency_stat_avg[i].num = latency_stat_c[i].num;
                }
                tput_stat_total.last_tx = total_tx;
                tput_stat_total.last_rx = total_rx;
                tput_stat_total.last_rx_read = total_rx_read;
                tput_stat_total.last_rx_write = total_rx_write;
                tput_stat_total.last_dropped = total_dropped;
                tput_stat_total.last_txn = total_txn;

                average_start_tsc = 0;
                latency_sample_start = 1;
                
            }

            if (unlikely(average_end_tsc > 0 && (cur_tsc > average_end_tsc) && (stop_statis == 0))) {
                
                uint64_t total_tx = 0;
                uint64_t total_rx = 0;
                uint64_t total_rx_read = 0;
                uint64_t total_rx_write = 0;
                uint64_t total_dropped = 0;
                uint64_t total_latency = 0;
                uint64_t total_latency_num = 0;
                uint64_t total_txn = 0;

                

                printf("Final! Average!\n");

                for (i = 0; i < n_lcores; i++) {
                    tput_stat_avg[i].tx = tput_stat[i].tx;
                    tput_stat_avg[i].rx = tput_stat[i].rx;
                    tput_stat_avg[i].rx_read = tput_stat[i].rx_read;
                    tput_stat_avg[i].rx_write = tput_stat[i].rx_write;
                    tput_stat_avg[i].dropped = tput_stat[i].dropped;
                    tput_stat_avg[i].txn = tput_stat[i].txn;

                    printf("Core %d\n", i);
                    printf("tx: %"PRIu64"\n", (tput_stat_avg[i].tx - tput_stat_avg[i].last_tx) / average_interval);
                    printf("rx: %"PRIu64"\n", (tput_stat_avg[i].rx - tput_stat_avg[i].last_rx) / average_interval);
                    printf("rx_read: %"PRIu64"\n", (tput_stat_avg[i].rx_read - tput_stat_avg[i].last_rx_read) / average_interval);
                    printf("rx_write: %"PRIu64"\n", (tput_stat_avg[i].rx_write - tput_stat_avg[i].last_rx_write) / average_interval);
                    printf("txn_rate: %"PRIu64"\n", (tput_stat_avg[i].txn - tput_stat_avg[i].last_txn) / average_interval);

                    total_tx += tput_stat[i].tx;
                    total_rx += tput_stat[i].rx;
                    total_rx_read += tput_stat[i].rx_read;
                    total_rx_write += tput_stat[i].rx_write;
                    total_dropped += tput_stat[i].dropped;
                    total_txn += tput_stat[i].txn;
                }
                tput_stat_total.tx = total_tx;
                tput_stat_total.rx = total_rx;
                tput_stat_total.rx_read = total_rx_read;
                tput_stat_total.rx_write = total_rx_write;
                tput_stat_total.dropped = total_dropped;
                tput_stat_total.txn = total_txn;

                printf("Total\n");
                printf("tx: %"PRIu64"\n", (tput_stat_total.tx - tput_stat_total.last_tx) / average_interval);
                printf("rx: %"PRIu64"\n", (tput_stat_total.rx - tput_stat_total.last_rx) / average_interval);
                printf("rx_read: %"PRIu64"\n", (tput_stat_total.rx_read - tput_stat_total.last_rx_read) / average_interval);
                printf("rx_read: %"PRIu64"\n", (tput_stat_total.rx_write - tput_stat_total.last_rx_write) / average_interval);
                printf("txn_rate: %"PRIu64"\n", (tput_stat_total.txn - tput_stat_total.last_txn) / average_interval);
                //printf("dropped: %"PRIu64"\n", (tput_stat_avg.dropped - tput_stat_avg.last_dropped) / average_interval);
                
                collect_results();
                stop_statis = 1;
            }
            if (unlikely(average_end_tsc > 0 && (cur_tsc > exit_tsc) && (stop_statis == 1))) {
                rte_exit(EXIT_SUCCESS, "Test Completed\n");
            }
        }

        // TX: send packets, drain TX queue
        
        if (unlikely(cur_tsc > next_drain_tsc)) {
            send_pkt_burst(lcore_id);
            next_drain_tsc = next_drain_tsc + drain_tsc;
        }

        // RX
        for (i = 0; i < lconf->n_rx_queue; i++) {
            nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], 
                   mbuf_burst, NC_MAX_BURST_SIZE);
            for (j = 0; j < nb_rx; j++) {
                
                mbuf = mbuf_burst[j];
                rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *));
                
                
                
                if (unlikely(begin_sample == 0))
                    begin_sample = 1;
                //process_packet_server(lcore_id, mbuf, lockqueues);
                struct ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
                struct ipv4_hdr *ip = (struct ipv4_hdr *)((uint8_t*) eth
                    + sizeof(struct ether_hdr));
                struct udp_hdr *udp = (struct udp_hdr *)((uint8_t*) ip
                    + sizeof(struct ipv4_hdr));
               

                // parse NetLock header
                MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + sizeof(header_template));
                uint32_t lock_id = ntohl(message_header->lockID);
                
                struct ether_addr *src_addr, *dst_addr;
                src_addr = &(eth->s_addr);
                dst_addr = &(eth->d_addr);
                uint8_t mem_role = dst_addr->addr_bytes[5];
                uint32_t ip_from_eth = src_addr->addr_bytes[2] | (src_addr->addr_bytes[3] << 8) | (src_addr->addr_bytes[4] << 16) | (src_addr->addr_bytes[5] << 24);
                
                if (mem_role == FAILURE_NOTIFICATION) {
                    // * empty the queues (init)
                    for (int index_secondary=0; index_secondary<count_secondary; index_secondary++) {
                        queue_init(&(lockqueues[secondary_locks[index_secondary]]));
                    }
                }
                else if (message_header->op_type == ACQUIRE_LOCK) {
                    if (mem_role == SECONDARY_BACKUP) {
                        if (enqueue(&(lockqueues[lock_id]), message_header->mode, message_header->txnID, ip_from_eth, message_header->payload, message_header->client_id) != 0) {
                            fprintf(stderr, "ENQUEUE ERROR.\n");
                        }
                        int len_in_server = get_queue_size(&(lockqueues[lock_id]));
                        
                    } else if (mem_role == PRIMARY_BACKUP) {
                        /*
                         *  if this queue is entirely in the server
                         */
                        if (enqueue(&(lockqueues[lock_id]), message_header->mode, message_header->txnID, ip_from_eth, message_header->payload, message_header->client_id) != 0) {
                            fprintf(stderr, "ENQUEUE ERROR.\n");
                        }
                        if (message_header->mode == SHARED_LOCK) {
                            if (num_ex[lock_id] == 0) {
                                DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_v:%d, cid:%d\n", lock_id, ntohl(message_header->txnID), ip_from_eth, message_header->client_id);
                                mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt(lcore_id, mbuf2, GRANT_LOCK_FROM_SERVER, message_header->mode, lock_id, 
                                                       ntohl(message_header->txnID), ip_from_eth, ip_from_eth, message_header->payload, message_header->client_id);
                                enqueue_pkt(lcore_id, mbuf2);
                                /*
                                 * Here, rx_read marks the rate server grants the lock,
                                 * rx_write marks the rate server pushs locks back to switch
                                 */
                                tput_stat[lcore_id].rx_read += 1;
                            }
                            num_sh[lock_id] ++;
                        } else if (message_header->mode == EXCLUSIVE_LOCK) {
                            if ((num_ex[lock_id] == 0) && (num_sh[lock_id] == 0)) {
                                DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_v:%d, cid:%d\n", lock_id, ntohl(message_header->txnID), ip_from_eth, message_header->client_id);
                                mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt(lcore_id, mbuf2, GRANT_LOCK_FROM_SERVER, message_header->mode, lock_id, 
                                                       ntohl(message_header->txnID), ip_from_eth, ip_from_eth, message_header->payload, message_header->client_id);
                                enqueue_pkt(lcore_id, mbuf2);
                                /*
                                 * Here, rx_read marks the rate server grants the lock,
                                 * rx_write marks the rate server pushs locks back to switch
                                 */
                                tput_stat[lcore_id].rx_read += 1;
                            }
                            num_ex[lock_id] ++;
                        }
                    }
                }
                else if (message_header->op_type == RELEASE_LOCK) {
                    uint8_t mode;
                    uint32_t txn_id;
                    uint32_t ip_src_v;
                    uint8_t cid;
                    uint64_t timestamp;
                    
                    if (mem_role == SECONDARY_BACKUP) {
                        /* 
                         * If the request is just buffered here 
                         */
                        int len_in_server = get_queue_size(&(lockqueues[lock_id]));
                        
                        
                        for (int i=0; i<min(len_in_server, len_in_switch[lock_id]); i++) {
                            dequeue(&(lockqueues[lock_id]), &mode, &txn_id, &ip_src_v, &timestamp, &cid);
                            // push some locks back
                            DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_v:%d, cid:%d\n", lock_id, txn_id, ip_src_v, cid);
                            mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt(lcore_id, mbuf2, PUSH_BACK_LOCK, mode, lock_id, txn_id, ip_src_v, 0, timestamp, cid);
                            enqueue_pkt(lcore_id, mbuf2);
                            /*
                             * Here, rx_read marks the rate server grants the lock,
                             * rx_write marks the rate server pushs locks back to switch
                             */
                            tput_stat[lcore_id].rx_write += 1;
                        }
                    } else if (mem_role == PRIMARY_BACKUP) {
                        /*
                         *  If the request is processed here
                         */
                        dequeue(&(lockqueues[lock_id]), &mode, &txn_id, &ip_src_v, &timestamp, &cid);
                        queue_node *ptr = (&(lockqueues[lock_id]))->head;
                        if (mode == SHARED_LOCK) {
                            num_sh[lock_id] --;
                            if ((ptr != NULL) && (ptr->mode == EXCLUSIVE_LOCK)) {
                                // notify the client
                                DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_v:%d, cid:%d\n", lock_id, ntohl(ptr->txnID), ptr->ip_src, ptr->client_id);
                                mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt(lcore_id, mbuf2, GRANT_LOCK_FROM_SERVER, ptr->mode, lock_id, ntohl(ptr->txnID),
                                                       ptr->ip_src, ptr->ip_src, ptr->timestamp, ptr->client_id);
                                enqueue_pkt(lcore_id, mbuf2);
                                /*
                                 * Here, rx_read marks the rate server grants the lock,
                                 * rx_write marks the rate server pushs locks back to switch
                                 */
                                tput_stat[lcore_id].rx_read += 1;
                            }
                        } else if (mode == EXCLUSIVE_LOCK) {
                            num_ex[lock_id] --;
                            if ((ptr != NULL) && (ptr->mode == EXCLUSIVE_LOCK)) {
                                // notify the client
                                DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_v:%d, cid:%d\n", lock_id, ntohl(ptr->txnID), ptr->ip_src, ptr->client_id);
                                mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt(lcore_id, mbuf2, GRANT_LOCK_FROM_SERVER, ptr->mode, lock_id, ntohl(ptr->txnID),
                                                       ptr->ip_src, ptr->ip_src, ptr->timestamp, ptr->client_id);
                                enqueue_pkt(lcore_id, mbuf2);
                                /*
                                 * Here, rx_read marks the rate server grants the lock,
                                 * rx_write marks the rate server pushs locks back to switch
                                 */
                                tput_stat[lcore_id].rx_read += 1;
                            }
                            else {
                                while ((ptr != NULL) && (ptr->mode == SHARED_LOCK)) {
                                    DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_v:%d, cid:%d\n", lock_id, ntohl(ptr->txnID), ptr->ip_src, ptr->client_id);
                                    mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                    generate_write_request_pkt(lcore_id, mbuf2, GRANT_LOCK_FROM_SERVER, ptr->mode, lock_id, ntohl(ptr->txnID),
                                                           ptr->ip_src, ptr->ip_src, ptr->timestamp, ptr->client_id);
                                    enqueue_pkt(lcore_id, mbuf2);
                                    ptr = ptr->next;
                                    /*
                                     * Here, rx_read marks the rate server grants the lock,
                                     * rx_write marks the rate server pushs locks back to switch
                                     */
                                    tput_stat[lcore_id].rx_read += 1;
                                }
                            }
                        }
                    }
                }
                tput_stat[lcore_id].rx += 1;
                    
                
                rte_pktmbuf_free(mbuf);
            }
        }
    }
    
    for (int i=0; i<MAX_LOCK_NUM; i++) {
        queue_clear(&(lockqueues[i]));
    }
    return 0;
}

// main processing loop for client
static int32_t np_client_loop(__attribute__((unused)) void *arg) {
    uint32_t lcore_id = rte_lcore_id();
    // if ((lcore_id < n_rcv_cores) || (role == ROLE_SERVER_POOL)) {
    np_client_rx_loop(lcore_id);
    // }
    // else {
    //     np_client_tx_write_loop(lcore_id);
    // }
    return 0;
}

void get_queue_len_in_switch(int *len_in_switch) {
    /*
     * Memory management
     */
    if (memory_management == MEM_BIN_PACK) {
        if (benchmark == TPCC_UNIFORMBENCHMARK) {
            sprintf(memn_filename, "%s/switch_code/netlock/controller_init/tpcc/tpcc_incast_%d_w_%d_sl_%d_nomap.in", dir_prefix, client_node_num, warehouse, slot_num);
        }
    }
    else if (memory_management == MEM_RAND_WEIGHT) {
        sprintf(memn_filename, "%s/switch_code/netlock/controller_init/random_weight.in", dir_prefix);
    }
    else if (memory_management == MEM_RAND_12) {
        sprintf(memn_filename, "%s/switch_code/netlock/controller_init/random_uni_12.in", dir_prefix);
    }
    else if (memory_management == MEM_RAND_200) {
        sprintf(memn_filename, "%s/switch_code/netlock/controller_init/random_uni_200.in", dir_prefix);
    }

    /*
     * run_tpcc
     */
    if (benchmark == TPCCBENCHMARK) {
        // ** TODO: change the filename according to the workload
        if (memory_management == MEM_BIN_PACK) {
            if (task_id == 'p')
                sprintf(memn_filename, "%s/switch_code/netlock/controller_init/tpcc/tpcc_notablelock_incast_%d_w_%d_sl_%d_nomap.in", dir_prefix, client_node_num, warehouse, slot_num);
            else if (task_id == 'q')
                sprintf(memn_filename, "%s/switch_code/netlock/controller_init/tpcc/tpcc_notablelock_multiserver_%d_w_%d_sl_%d_nomap.in", dir_prefix, client_node_num, warehouse, slot_num);
            else if (task_id == 'e')
                sprintf(memn_filename, "%s/switch_code/netlock/controller_init/tpcc/empty.in", dir_prefix);
            else 
                sprintf(memn_filename, "%s/switch_code/netlock/controller_init/tpcc/tpcc_notablelock_incast_%d_w_%d_sl_%d_nomap.in", dir_prefix, client_node_num, warehouse, slot_num);
        }
        else {
            sprintf(memn_filename, "%s/switch_code/netlock/controller_init/tpcc/tpcc_notablelock_incast_random_sn_%d.in", dir_prefix, slot_num);
        }
    }

    for (int i=0; i<MAX_LOCK_NUM; i++) {
        len_in_switch[i] = 0;
    }
    if (benchmark == MICROBENCHMARK_SHARED) {
        for (int i; i<MAX_LOCK_NUM; i++) {
            len_in_switch[i] = 10000;
        }
    } else if (benchmark == MICROBENCHMARK_EXCLUSIVE) {
        for (int i; i<MAX_LOCK_NUM; i++) {
            len_in_switch[i] = 12;
        }
    } else if (benchmark == NORMALBENCHMARK) {
        for (int i; i<MAX_LOCK_NUM; i++) {
            if (i<=20) {
                len_in_switch[i] = 500;
            }
            else {
                len_in_switch[i] = 20;
            }
        }
    } else if ((benchmark == ZIPFBENCHAMRK) || (benchmark == UNIFORMBENCHMARK)) {
        FILE *fin, *fp;
        char command[200];
        int lk_idx, lk_length, line_c;
        
        sprintf(command, "wc -l %s | awk -F '[ ]'+ '{print $1}'", memn_filename);
        DEBUG_PRINT("command: %s\n", command);
        fp = popen(command, "r");
        if (fp == NULL) {
            fprintf(stderr, "Failed to run command\n");
            return;
        }
        if (fscanf(fp, "%d", &line_c) == EOF) {
            fprintf(stderr, "RUN SHELL ERROR\n");
            return;
        }
        fclose(fp);
        fin = fopen(memn_filename, "r");
        for (int i=0; i<line_c; i++) {
            if (fscanf(fin, "%d,%d\n", &lk_idx, &lk_length) == EOF) {
                fprintf(stderr, "FILE ERROR\n");
                return ;
            }
            len_in_switch[lk_idx+1] = lk_length;
        }

    } else if ((benchmark == TPCCBENCHMARK) || (benchmark == TPCC_UNIFORMBENCHMARK)) {
        FILE *fin, *fp;
        char command[200];
        int lk_idx, lk_length, line_c;
        
        sprintf(command, "wc -l %s | awk -F '[ ]'+ '{print $1}'", memn_filename);
        DEBUG_PRINT("command: %s\n", command);
        fp = popen(command, "r");
        if (fp == NULL) {
            fprintf(stderr, "Failed to run command\n");
            return;
        }
        if (fscanf(fp, "%d", &line_c) == EOF) {
            fprintf(stderr, "RUN SHELL ERROR\n");
            return;
        }
        fclose(fp);
        fin = fopen(memn_filename, "r");
        for (int i=0; i<line_c; i++) {
            if (fscanf(fin, "%d,%d\n", &lk_idx, &lk_length) == EOF) {
                // * record the lk_idx
                secondary_locks[count_secondary++] = lk_idx + 1;
                fprintf(stderr, "FILE ERROR\n");
                return ;
            }
            len_in_switch[lk_idx+1] = lk_length;
        }
    }
    fprintf(stderr, "Read layout.\n");
    return;
}

// initialization
static void custom_init(void) {
    // initialize per-lcore stats
    memset(&tput_stat, 0, sizeof(tput_stat));

    // initialize zipf
    zipf_state = malloc(sizeof(struct zipf_gen_state));
    zipf_init(zipf_state, KEY_SPACE_SIZE, zipf_alpha * 0.01, 2011);

    printf("finish initialization\n");
    printf("==============================\n");
}


/*
 * functions for parsing arguments
 */

static void nc_parse_args_help(void) {
    printf("simple_socket [EAL options] --\n"
           "  -s pkts_send_limit_ms\n"
           "  -w warehouse_num \n"
           "  -n machine_id\n"
           "  -b benchmark (microbenchmark:'s','x'; tpccbenchmark:'t')\n"
           "  -r num_rcv_cores\n"
           "  -N number of cores used to run, should be consistent with EAL options\n"
           "  -C the number of clients\n"
           "  -T think time for a transaction, default: 0\n"
           "  -l the number of memory slots used in the switch\n"
           "  -o memory management algorithm ('b': optimal allocation; 'w': random allocation)\n"
           "  -k the number of total locks\n"
           "  -c contention level (microbenchmark only)\n"
           "  -i packet sampling interval for latency calculation\n"
           "  -a task_id\n");
}


static int nc_parse_args(int argc, char **argv) {
    int opt, num;
    double fnum;
    while ((opt = getopt(argc, argv, "m:p:w:s:i:c:t:n:b:r:z:e:k:o:l:a:T:C:S:O:g:N:P:")) != -1) {
        switch (opt) {
        case 'N':
            num = atoi(optarg);
            if (num > 0) {
                /*
                 * number of cores
                 */
                num_of_cores = num;
            } else {
                nc_parse_args_help();
                return -1;
            } 
            break;
        case 'g':
            num = atoi(optarg);
            if (num > 0) {
                /*
                 * batch size
                 */
                batch_size = num;
            } else {
                nc_parse_args_help();
                return -1;
            } 
            break;
        case 'O':
            num = atoi(optarg);
            if (num >= 0) {
                /*
                 * number of server nodes
                 */
                timeout_slot = num;
            } else {
                nc_parse_args_help();
                return -1;
            } 
            break;
        case 'S':
            num = atoi(optarg);
            if (num >= 0) {
                /*
                 * number of server nodes
                 */
                server_node_num = num;
            } else {
                nc_parse_args_help();
                return -1;
            } 
            break;
        case 'C':
            num = atoi(optarg);
            if (num >= 0) {
                /*
                 * number of client nodes
                 */
                client_node_num = num;
            } else {
                nc_parse_args_help();
                return -1;
            } 
            break;
        case 'T':
            num = atoi(optarg);
            if (num >= 0) {
                /*
                 * think time (ms)
                 */
                think_time = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'a':
            task_id = optarg[0];
            DEBUG_PRINT("task:%c\n", task_id);
            break;
        case 'l':
            num = atoi(optarg);
            if (num >= 0) {
                /*
                 * num of slots in the switch
                 */
                slot_num = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'o':
            if (optarg[0] == 'b') {
                memory_management = MEM_BIN_PACK;
            } else if (optarg[0] == 'w') {
                memory_management = MEM_RAND_WEIGHT;
            } else if (optarg[0] == '1') {
                memory_management = MEM_RAND_12;
            } else if (optarg[0] == '2') {
                memory_management = MEM_RAND_200;
            }
            break;
        case 'k':
            num = atoi(optarg);
            if (num > 0) {
                /*
                 * num of total locks
                 */
                lock_num = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'e':
            num = atoi(optarg);
            if (num > 0) {
                /*
                 * num of client each server emulates
                 */
                client_num = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 't':
            role = ROLE_SERVER_POOL;
            break;
        case 'm':
            // printf("optarg: %s\n",optarg);
            if (optarg[0] == 's') {
                data_transfer_mode = SYNCHRONOUS;
            } else if (optarg[0] == 'a') {
                data_transfer_mode = ASYNCHRONOUS;
            }
            break;
        case 's':
            num = atoi(optarg);
            wpkts_send_limit_ms = num;
            break;
        case 'w':
            num = atoi(optarg);
            if (num > 0) {
                warehouse = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'i':
            num = atoi(optarg);
            average_interval = num;
            break;
        case 'p':
            num = atoi(optarg);
            if (num > 0) {
                enabled_port_mask = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'c':
            num = atoi(optarg);
            if (num > 0) {
                contention_degree = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'n':
            num = atoi(optarg);
            if (num > 0) {
                machine_id = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'b':
            if (optarg[0] == 's') {
                benchmark = MICROBENCHMARK_SHARED;
                extensive_locks = EXTENSIVE_LOCKS_MICRO_BENCHMARK_SHARED;
                obj_per_lock = OBJ_PER_LOCK_MICRO_BENCHMARK_SHARED;
                printf("micro_benchmark shared lock\n");
            } else if (optarg[0] == 'x') {
                benchmark = MICROBENCHMARK_EXCLUSIVE;
                extensive_locks = EXTENSIVE_LOCKS_MICRO_BENCHMARK_EXCLUSIVE;
                obj_per_lock = OBJ_PER_LOCK_MICRO_BENCHMARK_EXCLUSIVE;
                printf("micro_benchmark exclusive lock\n");
            } else if (optarg[0] == 'n') {
                benchmark = NORMALBENCHMARK;
                extensive_locks = EXTENSIVE_LOCKS_NORMAL_BENCHMARK;
                obj_per_lock = OBJ_PER_LOCK_NORMAL_BENCHMARK;
                printf("normal_benchmark\n");
            } else if (optarg[0] == 'z') {
                benchmark = ZIPFBENCHAMRK;
                extensive_locks = EXTENSIVE_LOCKS_MICRO_BENCHMARK_EXCLUSIVE;
                obj_per_lock = OBJ_PER_LOCK_MICRO_BENCHMARK_EXCLUSIVE;
                printf("zipf_benchmark\n");
            } else if (optarg[0] == 'u') {
                benchmark = UNIFORMBENCHMARK;
                extensive_locks = EXTENSIVE_LOCKS_MICRO_BENCHMARK_EXCLUSIVE;
                obj_per_lock = OBJ_PER_LOCK_MICRO_BENCHMARK_EXCLUSIVE;
                printf("uniform_benchmark\n");
            } else if (optarg[0] == 't') {
                benchmark = TPCCBENCHMARK;

                /*
                 * Not mapping several lock obj to one lock in switch
                 */ 
                extensive_locks = EXTENSIVE_LOCKS_TPCC;
                obj_per_lock = OBJ_PER_LOCK_TPCC;
            } else if (optarg[0] == 'v') {
                benchmark = TPCC_UNIFORMBENCHMARK;
                extensive_locks = EXTENSIVE_LOCKS_TPCC;
                obj_per_lock = OBJ_PER_LOCK_TPCC;
            }
            else {
                nc_parse_args_help();
                return -1;
            }
            
            break;
        case 'r':
            num = atoi(optarg);
            if (num > 0) {
                n_rcv_cores = num;
            } else {
                nc_parse_args_help();
                return -1;
            }
            break;
        case 'z':
            num = atoi(optarg);
            zipf_alpha = num;
            break;
        case 'P':
            strcpy(dir_prefix, optarg);
            fprintf(stderr, "prefix:%s\n", dir_prefix);
            break;
        default:
            nc_parse_args_help();
            return -1;
        }
    }
    latency_sample_interval = max(1, wpkts_send_limit_ms * average_interval * 2 * 1000 / SAMPLE_SIZE / n_rcv_cores);
    printf("rate_adjust_per_sec: %f", rate_adjust_per_sec);
    printf("parsed arguments: speed: %"PRIu32
        ", enabled_port_mask: %"PRIu32
        ", write_ratio: %"PRIu32
        ", avg_intv: %"PRIu32
        ", zipf_alpha: %"PRIu32
        " \n", wpkts_send_limit_ms, enabled_port_mask, write_ratio, average_interval,zipf_alpha);
    return 1;
}

/*
 * main function
 */

int main(int argc, char **argv) {

    int ret;
    uint32_t lcore_id;
    srand(time(NULL));
    random_C = get_random(1, 130000);
    // parse default arguments
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }
    argc -= ret;
    argv += ret;

    // parse netcache arguments
    ret = nc_parse_args(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid netlock arguments\n");
    }
    // init
    nc_init();
    custom_init();

    get_queue_len_in_switch(len_in_switch);
    
    memcpy(ip_src, ip_list[machine_id - 1], 32);
    memcpy(ip_dst, ip_list[12], 32);
    for (int i=0;i<=11;i++) {
        DEBUG_PRINT("ip_list[%d]:%s\n", i, ip_list[i]);
    }
    
    DEBUG_PRINT("ip_src:%s\nip_dst:%s\n", ip_src, ip_dst);
    inet_pton(AF_INET, ip_src, &(ip_src_pton));
    inet_pton(AF_INET, ip_dst, &(ip_dst_pton));
    memset(failure_act, FAILURE_NORMAL, client_num);

    if (task_id == 'g') {
        batch_map = (uint32_t *) malloc(sizeof(uint32_t) * (MAX_LOCK_NUM + 1));
        get_batch_map(batch_map);
    }
    
    lockqueues = (queue_list *) malloc(sizeof(queue_list) * (MAX_LOCK_NUM + 1));
    for (int i=0; i<MAX_LOCK_NUM; i++) {
        queue_init(&(lockqueues[i]));
    }
    
    
    // launch main loop in every lcore
    rte_eal_mp_remote_launch(np_client_loop, NULL, CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            ret = -1;
            break;
        }
    }

    return 0;
}
