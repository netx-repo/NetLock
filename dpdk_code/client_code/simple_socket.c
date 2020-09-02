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
uint32_t num_ex[MAX_LOCK_NUM] = {0};
uint32_t num_sh[MAX_LOCK_NUM] = {0};
char busy[MAX_CLIENT_NUM][MAX_LOCK_NUM] = {0};
// char busy[MAX_LOCK_NUM] = {0};
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

// TX loop for test, fixed write rate
static int32_t np_client_tx_write_loop(uint32_t lcore_id) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    printf("%lld entering TX loop for write on lcore %u\n", (long long)time(NULL), lcore_id);

    struct rte_mbuf *mbuf;
    struct rte_mbuf *mbuf2;

    uint64_t cur_tsc = rte_rdtsc();
    uint64_t ms_tsc = rte_get_tsc_hz() / 1000;
    uint64_t next_ms_tsc = cur_tsc + ms_tsc;
    // uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * 100;
    uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * 10;
    uint64_t next_drain_tsc = cur_tsc + drain_tsc;
    uint64_t pkts_send_ms = 0;
    uint8_t cc = 0;

    uint32_t ind = 0;
    
    uint8_t mode;
    uint32_t lock_id = 0;
    uint32_t ttxn_id;
    uint32_t last_txn = 0;
    uint8_t cid = client_index_s[lcore_id];
    
    uint64_t expire_time[2] = {rte_get_tsc_hz() * max(timeout_slot, think_time * 2) / 1000000, rte_get_tsc_hz() * 10 / 1000000};
    uint64_t failure_timeout = rte_get_tsc_hz() * 50000 / 1000000;
    uint64_t loss_timeout = rte_get_tsc_hz() * 5000 / 1000000;
    
    int expire_idx = 0;
    int probe_counter = 0;
    while (1) {
        // read current time
        cur_tsc = rte_rdtsc();

        // clean packet counters for each ms
        if (unlikely(cur_tsc > next_ms_tsc)) {
            pkts_send_ms = 0;
            next_ms_tsc += ms_tsc;
        }
        
        // TX: generate packet, put in TX queue
        if (pkts_send_ms < wpkts_send_limit_ms) {
            {
                /*
                 * send probe packets to get the status of switch
                 */

                if (task_id == 'f') {
                    probe_counter ++;
                    if (probe_counter > 15000000)
                    {
                        mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                        generate_probe_pkt(lcore_id, mbuf2, ip_src_pton, 0);
                        enqueue_pkt(lcore_id, mbuf2);
                        send_pkt_burst(lcore_id); 
                        probe_counter = 0;
                    }
                }                      

                for (uint8_t cid=client_index_s[lcore_id]; cid <= client_index_e[lcore_id]; cid ++ ) {
                    /*
                     *  When the lock server is not responding (for a certain time)...
                     *  roll back to the begining of the txn
                     */
                    if (failure_act[cid] == FAILURE_ROLLBACK) {
                        txn_refresh_time[cid][last_txn] = cur_tsc;
                        txn_refresh_time_failure[cid][last_txn] = cur_tsc;
                        txn_queue_clear(&(txn_queues[cid][last_txn]));
                        txn_s[cid][last_txn] = 0;
                        txn_r[cid][last_txn] = 0;
                        num_retries[cid][last_txn] = 0;
                        expire_idx = 0;
                        idx[cid] = last_txn_idx[cid];
                        failure_act[cid] = FAILURE_STOP;
                        continue;
                    }
                    if (failure_act[cid] == FAILURE_STOP) {
                        txn_refresh_time[cid][last_txn] = cur_tsc;
                        txn_refresh_time_failure[cid][last_txn] = cur_tsc;
                        txn_queue_clear(&(txn_queues[cid][last_txn]));
                        txn_s[cid][last_txn] = 0;
                        txn_r[cid][last_txn] = 0;
                        num_retries[cid][last_txn] = 0;
                        expire_idx = 0;
                        idx[cid] = last_txn_idx[cid];
                        continue;
                    }
                    /*
                     * lock on the object
                     */
                    if (unlikely(idx[cid] == len[cid])) {
                        // DEBUG_PRINT("SEND FINISHED\n");
                        idx[cid] = 0;
                    }
                    if (target_obj_idx[cid][idx[cid]] <= extensive_locks) {
                        lock_id = target_obj_idx[cid][idx[cid]];
                    }
                    else {
                        lock_id = extensive_locks + 1 + (target_obj_idx[cid][idx[cid]] - extensive_locks - 1) / obj_per_lock;
                    }
                    
                    if (likely(action_type[cid][idx[cid]] == ACQUIRE_LOCK)) {
                        if (data_transfer_mode == SYNCHRONOUS) {
                            if (busy[cid][lock_id] == 1) {
                                idx[cid] ++;
                                continue;
                            }
                        }
                        ttxn_id = txn_id[cid][idx[cid]];
                        if ((benchmark == TPCCBENCHMARK) || (benchmark == TPCC_UNIFORMBENCHMARK)) {
                            if (ttxn_id != last_txn) {

                                if (txn_r[cid][last_txn] == locks_each_txn[cid][last_txn]) {
                                    if (deadlocked[cid][last_txn] == 0) {
                                        if (think_time > 0)
                                            usleep(think_time);     // sleep think_time
                                    }
                                    int bound = txn_r[cid][last_txn];
                                    txn_r[cid][last_txn] = 0;
                                    txn_queue_node *ptr = (&(txn_queues[cid][last_txn]))->head;
                                    int qlen = txn_get_queue_size(&(txn_queues[cid][last_txn]));
                                    for (int index = 0; index < txn_s[cid][last_txn]; index ++) {
                                        if (ptr == NULL) {
                                            fprintf(stderr, "ptr NULL step1.1!\n");
                                        }
                                        ptr = ptr->next;
                                    }

                                    for (int index = txn_s[cid][last_txn]; index < bound; index++) {
                                        if (ptr == NULL) {
                                            fprintf(stderr, "ptr NULL step1.2!\n");
                                        }
                                        if (deadlocked[cid][last_txn] == 0) {
                                            if (ptr->op_type == GRANT_LOCK_FROM_SERVER) {
                                                /*
                                                 * rx_read records grant_from_server
                                                 */
                                                tput_stat[lcore_id].rx_read += 1;
                                            }
                                            else {
                                                /*
                                                 * rx_write records grant from switch
                                                 */
                                                tput_stat[lcore_id].rx_write += 1;
                                            }
                                        }
                                        if (deadlocked[cid][last_txn] == 0)
                                            DEBUG_PRINT("rcv_1 lock_id:%d, txn_id:%d, cid:%d\n", ptr->lock_id, last_txn, cid);
                                        else 
                                            DEBUG_PRINT("rcv_2 lock_id:%d, txn_id:%d, cid:%d\n", ptr->lock_id, last_txn, cid);
                                        mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                        generate_write_request_pkt(lcore_id, mbuf2, RELEASE_LOCK, ptr->mode, ptr->lock_id, last_txn, ip_src_pton, 0, 0, cid);
                                        enqueue_pkt(lcore_id, mbuf2);
                                        ptr = ptr->next;

                                    }
                                    txn_queue_clear(&(txn_queues[cid][last_txn]));
                                    txn_s[cid][last_txn] = 0;
                                    send_pkt_burst(lcore_id);
                                    txn_finished[cid][last_txn] = 1;

                                }
                                else if (txn_r[cid][last_txn] < locks_each_txn[cid][last_txn]){
                                    if (cur_tsc > txn_refresh_time[cid][last_txn] + expire_time[expire_idx]) {
                                        deadlocked[cid][last_txn] = 1;
                                        int bound = txn_r[cid][last_txn];
                                        txn_queue_node *ptr = (&(txn_queues[cid][last_txn]))->head;
                                        int qlen = txn_get_queue_size(&(txn_queues[cid][last_txn]));
                                        for (int index = 0; index < txn_s[cid][last_txn]; index ++) {
                                            if (ptr == NULL) {
                                                fprintf(stderr, "ptr NULL step2.1!\n");
                                            }
                                            ptr = ptr->next;
                                        }
                                        /*
                                         *  release the locks first, roll-back
                                         */
                                        if (bound > txn_s[cid][last_txn])
                                            num_retries[cid][last_txn] ++;
                                        for (int index = txn_s[cid][last_txn]; index < bound; index++) {
                                            if (ptr == NULL) {
                                                fprintf(stderr, "ptr NULL step2.2!\n");
                                            }
                                            
                                            mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                            generate_write_request_pkt(lcore_id, mbuf2, RELEASE_LOCK, ptr->mode, ptr->lock_id, last_txn, ip_src_pton, 0, 0, cid);
                                            enqueue_pkt(lcore_id, mbuf2);
                                            ptr = ptr->next;
                                        }
                                        txn_s[cid][last_txn] = bound;
                                        send_pkt_burst(lcore_id);
                                        txn_refresh_time[cid][last_txn] = cur_tsc;

                                        expire_idx = 1;

                                    }
                                    continue;
                                }
                                else {
                                    fprintf(stderr, "SOMETHING MUST BE WRONG!\n");
                                }
                                if (txn_finished[cid][last_txn] == 1) {
                                    expire_idx = 0;
                                    if (deadlocked[cid][last_txn] != 1) {
                                        tput_stat[lcore_id].txn ++;
                                        tput_stat[lcore_id].rx += locks_each_txn[cid][last_txn];
                                        txn_finish_time[cid][last_txn] = rte_rdtsc();
                                        txn_begin_time[cid][ttxn_id] = 0;
                                        txn_refresh_time[cid][ttxn_id] = 0;
                                        txn_refresh_time_failure[cid][ttxn_id] = 0;
                                        txn_finish_time[cid][ttxn_id] = 0;
                                        last_txn_idx[cid] = idx[cid];
                                        deadlocked[cid][last_txn] = 0;
                                        deadlocked[cid][ttxn_id] = 0;
                                    }
                                    else {
                                        idx[cid] = last_txn_idx[cid];
                                        deadlocked[cid][last_txn] = 0;
                                        txn_refresh_time[cid][ttxn_id] = 0;
                                        txn_refresh_time_failure[cid][ttxn_id] = 0;
                                        txn_finish_time[cid][last_txn] = 0;
                                        int r = rand() % 10;
                                        usleep(r * timeout_slot / 400 * 30);
                                        
                                        continue;
                                    }
                                }
                            }
                        }
                        if ((benchmark == TPCCBENCHMARK) || (benchmark == TPCC_UNIFORMBENCHMARK)) {
                            last_txn = ttxn_id;
                            
                            if (txn_begin_time[cid][last_txn] == 0) {
                                txn_begin_time[cid][last_txn] = rte_rdtsc();
                                txn_refresh_time[cid][ttxn_id] = txn_begin_time[cid][last_txn];
                                txn_refresh_time_failure[cid][ttxn_id] = txn_begin_time[cid][last_txn];
                            }
                            txn_finished[cid][last_txn] = 0;
                            txn_s[cid][last_txn] = 0;
                            num_retries[cid][last_txn] = 0;
                        }
                        
                        if (data_transfer_mode == SYNCHRONOUS)
                            busy[cid][lock_id] = 1;
                        DEBUG_PRINT("send lock_id:%d, txn_id:%d, cid:%d, len:%d\n", lock_id, txn_id[cid][idx[cid]], cid, locks_each_txn[cid][last_txn]);
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                        generate_write_request_pkt(lcore_id, mbuf, action_type[cid][idx[cid]], lock_type[cid][idx[cid]], lock_id, txn_id[cid][idx[cid]], 0, 0, 0, cid);
                        acquire_enqueue_pkt(lcore_id, mbuf, locks_each_txn[cid][last_txn]);
                        
                        idx[cid] ++;
                        pkts_send_ms ++;
                    } else {
                        idx[cid] ++;
                    }
                }
            }
        }
    }
    return 0;
}



static int32_t process_packet_server(uint32_t lcore_id, struct rte_mbuf *mbuf, queue_list *lockqueues) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];

    // parse packet header
    struct ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    struct ipv4_hdr *ip = (struct ipv4_hdr *)((uint8_t*) eth
        + sizeof(struct ether_hdr));
    struct udp_hdr *udp = (struct udp_hdr *)((uint8_t*) ip
        + sizeof(struct ipv4_hdr));

    // parse NetLock header
    MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + sizeof(header_template));
    uint32_t lock_id = ntohl(message_header->lockID);

    tput_stat[ntohl(ip->src_addr) - 167837696].rx += 1;
    if (message_header->op_type == ACQUIRE_LOCK) {
        tput_stat[ntohl(ip->src_addr) - 167837696].rx_read += 1;
    }
    else if (message_header->op_type == RELEASE_LOCK) {
        tput_stat[ntohl(ip->src_addr) - 167837696].rx_write += 1;
    }

    if (message_header->op_type == ACQUIRE_LOCK) {
        if (enqueue(&(lockqueues[lock_id]), message_header->mode, message_header->txnID, ip->src_addr, message_header->payload, message_header->client_id) != 0) {
            fprintf(stderr, "ENQUEUE ERROR.\n");
        }
    }
    else if (message_header->op_type == RELEASE_LOCK) {
        uint8_t mode;
        uint32_t txn_id;
        uint32_t ip_src;
        uint64_t timestamp;
        uint8_t cid;
        int len_in_server = get_queue_size(&(lockqueues[lock_id]));
        for (int i=0; i<min(len_in_server, len_in_switch[lock_id]); i++) {
            dequeue(&(lockqueues[lock_id]), &mode, &txn_id, &ip_src, &timestamp, &cid);
            send_netlock_req(lcore_id, PUSH_BACK_LOCK, mode, lock_id, txn_id, ip_src);
            send_pkt_burst(lcore_id);
        }
    }

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
    
    if (role == ROLE_CLIENT) {
        thinkqueue = (think_queue_list *) malloc(sizeof(think_queue_list));
        think_queue_init(thinkqueue);
    }

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

            // if (unlikely(average_start_tsc > 0 && (((role == ROLE_SERVER_POOL) &&(begin_sample > 0)) ||
            //             ((role == ROLE_CLIENT)&&(cur_tsc > average_start_tsc))))) {
            if (unlikely(average_start_tsc > 0 && cur_tsc > average_start_tsc)) {
                uint64_t total_tx = 0;
                uint64_t total_rx = 0;
                uint64_t total_rx_read = 0;
                // uint64_t total_rx_write = 0;
                // uint64_t total_dropped = 0;
                // uint64_t total_txn = 0;
                
                for (i = 0; i < n_lcores; i++) {
                    tput_stat_avg[i].last_tx = tput_stat[i].tx;
                    tput_stat_avg[i].last_rx = tput_stat[i].rx;
                    // tput_stat_avg[i].last_rx_read = tput_stat[i].rx_read;
                    // tput_stat_avg[i].last_rx_write = tput_stat[i].rx_write;
                    // tput_stat_avg[i].last_dropped = tput_stat[i].dropped;

                    tput_stat_avg[i].txn = tput_stat[i].txn;
                    
                    total_tx += tput_stat[i].tx;
                    total_rx += tput_stat[i].rx;
                    total_rx_read += tput_stat[i].rx_read;
                    // total_rx_write += tput_stat[i].rx_write;
                    // total_dropped += tput_stat[i].dropped;
                    // total_txn += tput_stat[i].txn;

                    // latency_stat_avg[i].total = latency_stat_c[i].total;
                    // latency_stat_avg[i].num = latency_stat_c[i].num;
                }
                tput_stat_total.last_tx = total_tx;
                tput_stat_total.last_rx = total_rx;
                tput_stat_total.last_rx_read = total_rx_read;
                // tput_stat_total.last_rx_write = total_rx_write;
                // tput_stat_total.last_dropped = total_dropped;
                // tput_stat_total.last_txn = total_txn;

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
                    // tput_stat_avg[i].rx_write = tput_stat[i].rx_write;
                    // tput_stat_avg[i].dropped = tput_stat[i].dropped;
                    // tput_stat_avg[i].txn = tput_stat[i].txn;

                    printf("Core %d\n", i);
                    printf("tx: %"PRIu64"\n", (tput_stat_avg[i].tx - tput_stat_avg[i].last_tx) / average_interval);
                    printf("rx: %"PRIu64"\n", (tput_stat_avg[i].rx - tput_stat_avg[i].last_rx) / average_interval);
                    printf("rx_read: %"PRIu64"\n", (tput_stat_avg[i].rx_read - tput_stat_avg[i].last_rx_read) / average_interval);
                    // printf("rx_write: %"PRIu64"\n", (tput_stat_avg[i].rx_write - tput_stat_avg[i].last_rx_write) / average_interval);
                    // printf("txn_rate: %"PRIu64"\n", (tput_stat_avg[i].txn - tput_stat_avg[i].last_txn) / average_interval);

                    total_tx += tput_stat[i].tx;
                    total_rx += tput_stat[i].rx;
                    total_rx_read += tput_stat[i].rx_read;
                    // total_rx_write += tput_stat[i].rx_write;
                    // total_dropped += tput_stat[i].dropped;
                    // total_txn += tput_stat[i].txn;
                }
                tput_stat_total.tx = total_tx;
                tput_stat_total.rx = total_rx;
                tput_stat_total.rx_read = total_rx_read;
                // tput_stat_total.rx_write = total_rx_write;
                // tput_stat_total.dropped = total_dropped;
                // tput_stat_total.txn = total_txn;

                printf("Total\n");
                printf("tx: %"PRIu64"\n", (tput_stat_total.tx - tput_stat_total.last_tx) / average_interval);
                printf("rx: %"PRIu64"\n", (tput_stat_total.rx - tput_stat_total.last_rx) / average_interval);
                printf("rx_read: %"PRIu64"\n", (tput_stat_total.rx_read - tput_stat_total.last_rx_read) / average_interval);
                // printf("rx_read: %"PRIu64"\n", (tput_stat_total.rx_write - tput_stat_total.last_rx_write) / average_interval);
                // printf("txn_rate: %"PRIu64"\n", (tput_stat_total.txn - tput_stat_total.last_txn) / average_interval);
                //printf("dropped: %"PRIu64"\n", (tput_stat_avg.dropped - tput_stat_avg.last_dropped) / average_interval);
                
                uint64_t latency_total=0, num_total=0;
                // for (i=0;i<n_rcv_cores;i++) {
                //     latency_total = latency_total + latency_stat_c[i].total - latency_stat_avg[i].total;
                //     num_total = num_total + latency_stat_c[i].num - latency_stat_avg[i].num;
                // }
                // printf("Average latency: %.4f ms\n", (latency_total - num_total) / (double) (num_total) / 1000.0);
                // printf("Average latency: %.4f ms\n", (latency_total) / (double) (num_total) / 1000.0);

                fflush(stdout);

                

                // Sort latency_samples
                for (int lc = 0; lc < n_rcv_cores; lc++) {
                    for (i = 0; i < latency_sample_num[lc]; ++i) {
                        latency_samples_together[latency_samples_together_len] = latency_samples[lc][i];
                        latency_samples_together_len ++;
                    }
                }
                for (i = 0; i < latency_samples_together_len; i++) {
                    uint64_t lat = latency_samples_together[i];
                    j = i;
                    while (j > 0 && latency_samples_together[j - 1] > lat) {
                        latency_samples_together[j] = latency_samples_together[j - 1];
                        j --;
                    }
                    latency_samples_together[j] = lat;
                }

                // ** quicksort
                //qsort(latency_samples_together, latency_samples_together_len, sizeof(uint64_t), cmpfunc);

                // Print median
                printf("Median latency: %.4f ms\n", latency_samples_together[latency_samples_together_len / 2] / 1000.0);
                fflush(stdout);

                
                
                char filename_res[200], filename_lt[200], filename_txn_lt[200];
                char dirname_1[200], dirname_2[200], dirname_3[200];
                FILE *fout;
                char cmd[100];
                /*
                 * fig 1. ../results/shared/cn%dlk%d/ 's'
                 * fig 2. ../results/exclusive/cn%dlk%d/ 'x'
                 * fig 3,4. ../results/contention/cn%dlk%d/ 'c'
                 * fig 5,6. ../results/mem_manage/(binpack, random)/cn%dlk%d/ 'm'
                 * fig 7. ../results/mem_size/(zipf, uniform)/cn%dlk%d/ 'S'
                 * fig 8. ../results/think_time/cn%dlk%d/ 't'
                 */
                if (task_id == 's') {
                    sprintf(dirname_1, "../results/shared");
                    sprintf(dirname_2, "../results/shared/cn_%d_lk_%d", client_num, lock_num);
                    sprintf(dirname_3, "../results/shared/cn_%d_lk_%d", client_num, lock_num);
                    sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                } else if (task_id == 'x') {
                    sprintf(dirname_1, "../results/exclusive");
                    sprintf(dirname_2, "../results/exclusive/cn_%d_lk_%d", client_num, lock_num);
                    sprintf(dirname_3, "../results/exclusive/cn_%d_lk_%d", client_num, lock_num);
                    sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                } else if (task_id == 'c') {
                    sprintf(dirname_1, "../results/contention");
                    sprintf(dirname_2, "../results/contention/cn_%d_lk_%d", client_num, lock_num);
                    sprintf(dirname_3, "../results/contention/cn_%d_lk_%d", client_num, lock_num);
                    sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                } else if (task_id == 'm') {
                    sprintf(dirname_1, "../results/mem_manage");
                    if (memory_management == MEM_BIN_PACK) {
                        sprintf(dirname_2, "../results/mem_manage/binpack");
                        sprintf(dirname_3, "../results/mem_manage/binpack/cn_%d_lk_%d_sn_%d", client_num, lock_num, slot_num);
                    }
                    else if (memory_management == MEM_RAND_WEIGHT) {
                        sprintf(dirname_2, "../results/mem_manage/random");
                        sprintf(dirname_3, "../results/mem_manage/random/cn_%d_lk_%d_sn_%d", client_num, lock_num, slot_num);
                    }
                    sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_ln%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, lock_num);
                    sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                } else if (task_id == 'S') {
                    sprintf(dirname_1, "../results/mem_size");
                    if (benchmark == ZIPFBENCHAMRK) {
                        sprintf(dirname_2, "../results/mem_size/zipf");
                        sprintf(dirname_3, "../results/mem_size/zipf/cn_%d_sn_%d", client_num, slot_num);
                    }
                    else if (benchmark == UNIFORMBENCHMARK) {
                        sprintf(dirname_2, "../results/mem_size/uniform");
                        sprintf(dirname_3, "../results/mem_size/uniform/cn_%d_sn_%d", client_num, slot_num);
                    }
                    else if (benchmark == TPCC_UNIFORMBENCHMARK) {
                        sprintf(dirname_2, "../results/mem_size/tpcc_uniform");
                        sprintf(dirname_3, "../results/mem_size/tpcc_uniform/cn_%d_sn_%d", client_num, slot_num);
                    }
                    sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
                } else if (task_id == 't') {
                    sprintf(dirname_1, "../results/think_time");
                    sprintf(dirname_2, "../results/think_time/cn_%d_sn_%d_tt_%lu", client_num, slot_num, think_time);
                    sprintf(dirname_3, "../results/think_time/cn_%d_sn_%d_tt_%lu", client_num, slot_num, think_time);
                    sprintf(filename_res, "%s/res_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_i%d_s%d_%d_cn%d_sn%d.out", dirname_3, role, benchmark, average_interval, wpkts_send_limit_ms, machine_id, client_num, slot_num);
                    sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                } else if (task_id == 'p') {
                    sprintf(dirname_1, "../results/tpcc");
                    sprintf(dirname_2, "../results/tpcc/incast");
                    sprintf(dirname_3, "../results/tpcc/incast/cn_%d_wh_%d", client_num, warehouse);
                    sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                    sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                } else if (task_id == 'q') {
                    sprintf(dirname_1, "../results/tpcc");
                    sprintf(dirname_2, "../results/tpcc/multi_server/");
                    sprintf(dirname_3, "../results/tpcc/multi_server/cn_%d_wh_%d", client_num, warehouse);
                    sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                    sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_wh%d.out", dirname_3, role, benchmark, machine_id, client_num, warehouse);
                } else if (task_id == 'g') {
                    sprintf(dirname_1, "../results/granularity");
                    sprintf(dirname_2, "../results/granularity/sn_%d_bs_%d_to_%d/", slot_num, batch_size, timeout_slot);
                    sprintf(dirname_3, "../results/granularity/sn_%d_bs_%d_to_%d/", slot_num, batch_size, timeout_slot);
                    sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
                    sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
                } else if (task_id == 'f') {
                    sprintf(dirname_1, "../results/failover");
                    sprintf(dirname_2, "../results/failover");
                    sprintf(dirname_3, "../results/failover");
                    sprintf(filename_res, "%s/res_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
                    sprintf(filename_lt, "%s/latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
                    sprintf(filename_txn_lt, "%s/txn_latency_r%c_b%c_%d_cn%d_sn_%d_bs_%d.out", dirname_3, role, benchmark, machine_id, client_num, slot_num, batch_size);
                } else if (task_id == 'e') {
                    if (client_node_num == 10) {
                        sprintf(dirname_1, "../results/camera_ready_incast");
                        sprintf(dirname_2, "../results/camera_ready_incast/empty");
                        sprintf(dirname_3, "../results/camera_ready_incast/empty/core_%d_wh%d", num_of_cores, warehouse);
                        sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
                    }
                    else {
                        sprintf(dirname_1, "../results/camera_ready_multicast");
                        sprintf(dirname_2, "../results/camera_ready_multicast/empty");
                        sprintf(dirname_3, "../results/camera_ready_multicast/empty/core_%d_wh%d", num_of_cores, warehouse);
                        sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
                    }
                }
                else if ((task_id == '2') || (task_id == '3')) {
                    if (client_node_num == 10) {
                        sprintf(dirname_1, "../results/camera_ready_incast");
                        sprintf(dirname_2, "../results/camera_ready_incast/netlock");
                        sprintf(dirname_3, "../results/camera_ready_incast/netlock/core_%d_wh%d", num_of_cores, warehouse);
                        sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
                    }
                    else {
                        sprintf(dirname_1, "../results/camera_ready_multicast");
                        sprintf(dirname_2, "../results/camera_ready_multicast/netlock");
                        sprintf(dirname_3, "../results/camera_ready_multicast/netlock/core_%d_wh%d", num_of_cores, warehouse);
                        sprintf(filename_res, "%s/res_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_lt, "%s/latency_r%c_%d.out", dirname_3, role, machine_id);
                        sprintf(filename_txn_lt, "%s/txn_latency_r%c_%d.out", dirname_3, role, machine_id);
                    }
                }
                else {
                    fprintf(stderr, "TASK ID WRONG!");
                }
                DEBUG_PRINT("%s\n", dirname_1);
                DEBUG_PRINT("%s\n", dirname_2);
                DEBUG_PRINT("%s\n", dirname_3);
                DEBUG_PRINT("%s\n", filename_res);
                DEBUG_PRINT("%s\n", filename_lt);

                sprintf(cmd, "mkdir -p %s", dirname_1);
                int sysRet = system(cmd);
                if (sysRet != 0) {
                    fprintf(stderr, "Execute command ERROR!\n");
                }
                sprintf(cmd, "mkdir -p %s", dirname_2);
                sysRet = system(cmd);
                if (sysRet != 0) {
                    fprintf(stderr, "Execute command ERROR!\n");
                }
                
                sprintf(cmd, "mkdir -p %s", dirname_3);
                sysRet = system(cmd);
                if (sysRet != 0) {
                    fprintf(stderr, "Execute command ERROR!\n");
                }
                
                fout = fopen(filename_res, "w");

                fprintf(fout, "Total\n");
                fprintf(fout, "tx: %"PRIu64"\n", (tput_stat_total.tx - tput_stat_total.last_tx) / average_interval);
                fprintf(fout, "rx: %"PRIu64"\n", (tput_stat_total.rx - tput_stat_total.last_rx) / average_interval);
                fprintf(fout, "rx_read: %"PRIu64"\n", (tput_stat_total.rx_read - tput_stat_total.last_rx_read) / average_interval);
                fprintf(fout, "rx_write: %"PRIu64"\n", (tput_stat_total.rx_write - tput_stat_total.last_rx_write) / average_interval);
                fprintf(fout, "txn_rate: %"PRIu64"\n", (tput_stat_total.txn - tput_stat_total.last_txn) / average_interval);
                
                fprintf(fout, "Average latency: %.4f ms\n", (latency_total) / (double) (num_total) / 1000.0);
                fprintf(fout, "Median latency: %.4f ms\n", latency_samples_together[latency_samples_together_len / 2] / 1000.0);
                fprintf(fout, "99 percent tail latency: %.4f ms\n", latency_samples_together[latency_samples_together_len * 99/100 - 1] / 1000.0);
                fprintf(fout, "99.9 percent tail latency: %.4f ms\n", latency_samples_together[latency_samples_together_len * 999/1000 - 1] / 1000.0);
                fflush(fout);
                fclose(fout);

#ifdef COLLECT_LATENCY
                
                fout = fopen(filename_lt, "w");
                for (i = 0; i < latency_samples_together_len; i++) {
                    uint64_t lat = latency_samples_together[i];
                    fprintf(fout, "%lu\n", lat);
                }
                fflush(fout);
                fclose(fout);


                if ((task_id == 'p') || (task_id == 'q') || (benchmark == TPCCBENCHMARK) || (benchmark == TPCC_UNIFORMBENCHMARK)) {
                    fout = fopen(filename_txn_lt, "w");
                    for (i=0; i<client_num; i++) {
                        for (int j=0; j<MAX_TXN_NUM; j++) {
                            if ((txn_begin_time[i][j] != 0) && (txn_finish_time[i][j] != 0)) {
                                uint64_t lat = timediff_in_us(txn_finish_time[i][j], txn_begin_time[i][j]);
                                fprintf(fout, "%lu\n", lat);
                            }
                        }
                    }
                    fflush(fout);
                    fclose(fout);
                }
#endif
                stop_statis = 1;
            }
            if (unlikely(average_end_tsc > 0 && (cur_tsc > exit_tsc) && (stop_statis == 1))) {
                rte_exit(EXIT_SUCCESS, "Test Completed\n");
            }
        }

        // TX: send packets, drain TX queue
        if ((benchmark != TPCCBENCHMARK) && (benchmark != TPCC_UNIFORMBENCHMARK)) {
            if (unlikely ((think_time > 0) && (role == ROLE_CLIENT) && (cur_tsc > last_drain_tq_tsc + increment_time))) {
                think_queue_node *ptr = (thinkqueue)->head;
                uint8_t mode;
                uint32_t txn_id;
                uint32_t lock_id;
                uint8_t cid;
                uint8_t opt;
                while (ptr != NULL) {
                    ptr = ptr->next;
                    think_dequeue(thinkqueue, &mode, &lock_id, &txn_id, &cid);
                    // * TODO: txn_enqueue TPCCBENCHAMRK
                    
                    mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                    generate_write_request_pkt(lcore_id, mbuf2, RELEASE_LOCK, mode, lock_id, txn_id, ip_src_pton, 0, 0, cid);
                    enqueue_pkt(lcore_id, mbuf2);
                    if (data_transfer_mode == SYNCHRONOUS)
                        busy[cid][lock_id] = 0;
                }
                last_drain_tq_tsc = cur_tsc;
            }
        }
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
                
                if (role == ROLE_CLIENT) {
                    
                    struct ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
                    struct ipv4_hdr *ip = (struct ipv4_hdr *)((uint8_t*) eth
                        + sizeof(struct ether_hdr));
                    struct udp_hdr *udp = (struct udp_hdr *)((uint8_t*) ip
                        + sizeof(struct ipv4_hdr));
                    
                    if (ntohs(udp->dst_port) == port_probe) {
                        ProbeHeader* probe_header = (ProbeHeader*) ((uint8_t *) eth + sizeof(header_template));
                        current_failure_status = (uint8_t) probe_header->failure_status;
                        
                        if ((current_failure_status == 1) && (last_failure_status == 0)) {
                            memset(failure_act, FAILURE_ROLLBACK, client_num);
                        }
                        else if ((current_failure_status == 0) && (last_failure_status == 1)) {
                            fprintf(stderr, "Become normal\n");
                            memset(failure_act, FAILURE_NORMAL, client_num);
                        }
                        last_failure_status = current_failure_status;
                        rte_pktmbuf_free(mbuf);
                        continue;
                    }
                    if (failure_act[client_index_s[lcore_id]] != FAILURE_NORMAL) {
                        
                        rte_pktmbuf_free(mbuf);
                        continue;
                    }
                    
                    // parse NetLock header
                    MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + sizeof(header_template));
                    uint8_t mode = (uint8_t) message_header->mode;
                    uint8_t opt = (uint8_t) message_header->op_type;
                    uint32_t txn_id = ntohl(message_header->txnID);
                    uint32_t lock_id = ntohl(message_header->lockID);
                    uint8_t cid = (uint8_t) (message_header->client_id);
                    
                    if (message_header->op_type == REJECT_LOCK_ACQUIRE) {
                        /*
                         * send the request again ...
                         */
                        mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                        generate_write_request_pkt(lcore_id, mbuf2, ACQUIRE_LOCK, mode, lock_id, txn_id, ip_src_pton, 0, 0, cid);
                        enqueue_pkt(lcore_id, mbuf2);
                    }
                    else {
                        uint64_t latency = timediff_in_us(rte_rdtsc(), message_header->payload);
                        compute_latency(&latency_stat_c[lcore_id], latency);
                        if (latency_sample_start == 1) {
                            if (latency_stat_c[lcore_id].num % latency_sample_interval == 0) {
                                latency_samples[lcore_id][latency_sample_num[lcore_id]] = latency;
                                latency_sample_num[lcore_id]++;
                            }
                        }
                        

                        if (think_time == 0) {
                            if ((benchmark == TPCCBENCHMARK) || (benchmark == TPCC_UNIFORMBENCHMARK)) {
                                txn_enqueue(&(txn_queues[cid][txn_id]), mode, opt, lock_id);
                                txn_r[cid][txn_id] ++;
                                // ** receiving reply means failure is recovered
                                detect_failure[cid] = 0;
                            }
                            else {
                                mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt(lcore_id, mbuf2, RELEASE_LOCK, mode, lock_id, txn_id, ip_src_pton, 0, 0, cid);
                                enqueue_pkt(lcore_id, mbuf2);
                                if (data_transfer_mode == SYNCHRONOUS)
                                    busy[cid][lock_id] = 0;
                            }
                        }
                        else {
                            if ((benchmark == TPCCBENCHMARK) || (benchmark == TPCC_UNIFORMBENCHMARK)) {
                                txn_enqueue(&(txn_queues[cid][txn_id]), mode, opt, lock_id);
                                txn_r[cid][txn_id] ++;
                                // ** receiving reply means failure is recovered
                                detect_failure[cid] = 0;
                            }
                            else {
                                uint64_t grant_time = rte_rdtsc();
                                if (think_enqueue(thinkqueue, mode, lock_id, txn_id, cid, grant_time) != 0) {
                                    fprintf(stderr, "ENQUEUE ERROR.\n");
                                }
                                
                                
                                if (unlikely(cur_tsc > last_drain_tq_tsc + increment_time)) {
                                    think_queue_node *ptr = (thinkqueue)->head;
                                    while (ptr != NULL) {
                                        ptr = ptr->next;
                                        think_dequeue(thinkqueue, &mode, &lock_id, &txn_id, &cid);
                                        // TODO: txn_enqueue for TPCCBENCHMARK
                                        
                                        mbuf2 = rte_pktmbuf_alloc(pktmbuf_pool);
                                        generate_write_request_pkt(lcore_id, mbuf2, RELEASE_LOCK, mode, lock_id, txn_id, ip_src_pton, 0, 0, cid);
                                        enqueue_pkt(lcore_id, mbuf2);
                                        if (data_transfer_mode == SYNCHRONOUS)
                                            busy[cid][lock_id] = 0;
                                        
                                    }
                                    last_drain_tq_tsc = cur_tsc;
                                }
                            }
                        }

                        if ((benchmark != TPCCBENCHMARK) && (benchmark != TPCC_UNIFORMBENCHMARK)) {
                            tput_stat[lcore_id].rx += 1;
                            if ((message_header->op_type == ACQUIRE_LOCK) || (message_header->op_type == PUSH_BACK_LOCK) || (message_header->op_type == GRANT_LOCK_FROM_SERVER)) {
                                tput_stat[lcore_id].rx_read += 1;
                            }
                            else if (message_header->op_type == RELEASE_LOCK) {
                                tput_stat[lcore_id].rx_write += 1;
                            }
                        }
                    }
                }
                else {
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
                             *  if this queue is entirely in the server
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
                    
                }
                rte_pktmbuf_free(mbuf);
            }
        }
    }
    if (role == ROLE_SERVER_POOL) {
        for (int i=0; i<MAX_LOCK_NUM; i++) {
            queue_clear(&(lockqueues[i]));
        }
    }
    return 0;
}

// main processing loop for client
static int32_t np_client_loop(__attribute__((unused)) void *arg) {
    uint32_t lcore_id = rte_lcore_id();
    if ((lcore_id < n_rcv_cores) || (role == ROLE_SERVER_POOL)) {
        np_client_rx_loop(lcore_id);
    }
    else {
        np_client_tx_write_loop(lcore_id);
    }
    return 0;
}

void get_queue_len_in_switch(int *len_in_switch) {
    if (memory_management == MEM_BIN_PACK) {
        if (benchmark == ZIPFBENCHAMRK)
            sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/zipf-9/binpack_sl%d.in", slot_num);
        else if (benchmark == UNIFORMBENCHMARK) {
            sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/uniform/binpack_sl%d.in", slot_num);
        
        }
        else if (benchmark == TPCC_UNIFORMBENCHMARK) {
            sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/tpcc/tpcc_incast_%d_w_%d_sl_%d_nomap.in", client_node_num, warehouse, slot_num);
        }
    }
    else if (memory_management == MEM_RAND_WEIGHT) {
        strcpy(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/random_weight.in");
    }
    else if (memory_management == MEM_RAND_12) {
        strcpy(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/random_uni_12.in");
    }
    else if (memory_management == MEM_RAND_200) {
        strcpy(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/random_uni_200.in");
    }
    if (benchmark == TPCCBENCHMARK) {
        // ** TODO: change the filename according to the workload
        if (memory_management == MEM_BIN_PACK) {
            if (task_id == 'p')
                sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/tpcc/tpcc_notablelock_incast_%d_w_%d_sl_%d_nomap.in", client_node_num, warehouse, slot_num);
            else if (task_id == 'q')
                sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/tpcc/tpcc_notablelock_multiserver_%d_w_%d_sl_%d_nomap.in", client_node_num, warehouse, slot_num);
            else if (task_id == 'e')
                sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/tpcc/empty.in");
            else 
                sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/tpcc/tpcc_notablelock_incast_%d_w_%d_sl_%d_nomap.in", client_node_num, warehouse, slot_num);
        }
        else {
            sprintf(memn_filename, "/home/user/zhuolong/exp/netlock-code/controller_init/tpcc/tpcc_notablelock_incast_random_sn_%d.in", slot_num);
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
           "  -i average_interval\n"
           "  -p port_mask (>0)\n"
           "  -c warehouse_num (contention)\n"
           "  -n machine_id\n"
           "  -b benchmark (microbenchmark:'s','x'; normalbenchmark:'n')\n"
           "  -r num_rcv_cores\n"
           "  -tf (server as server pool)\n");
}


static int nc_parse_args(int argc, char **argv) {
    int opt, num;
    double fnum;
    while ((opt = getopt(argc, argv, "m:p:w:s:i:c:t:n:b:r:z:e:k:o:l:a:T:C:S:O:g:N:")) != -1) {
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

    for (int i=n_rcv_cores; i<n_lcores; i++) {
        int k = i - n_rcv_cores;
        client_index_s[i] = (uint8_t) k * client_num / (n_lcores - n_rcv_cores);
        client_index_e[i] = (uint8_t) (k+1) * client_num / (n_lcores - n_rcv_cores) - 1;
        for (int j=client_index_s[i]; j<=client_index_e[i]; j++) {
            get_traces(j, &(txn_id[j]), &(action_type[j]),
                    &(target_lm_id[j]), &(target_obj_idx[j]), &(lock_type[j]), &(len[j]));
        }
    }

    if (role == ROLE_SERVER_POOL) {
        lockqueues = (queue_list *) malloc(sizeof(queue_list) * (MAX_LOCK_NUM + 1));
        for (int i=0; i<MAX_LOCK_NUM; i++) {
            queue_init(&(lockqueues[i]));
        }
    }
    else if (role == ROLE_CLIENT) {
        for (int i=0; i<MAX_CLIENT_NUM; i++)
            for (int j=0; j<MAX_TXN_NUM; j++)
                txn_queue_init(&(txn_queues[i][j]));
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
