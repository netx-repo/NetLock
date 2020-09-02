import os,sys
import csv
lib_path = os.path.abspath(os.path.join('../../client'))
sys.path.append(lib_path)
from config import *
import random
from random import randint

class MicroBenchmark:
    def __init__(self, lock_type = SHARED_LOCK, max_lock_id = 100000, server_number = 10, threads_per_server = 2):
        self.lock_per_server = max_lock_id / server_number
        if (lock_type == SHARED_LOCK):
            self.lock_type = 1
        elif (lock_type == EXCLUSIVE_LOCK):
            self.lock_type = 2
        self.max_lock_id = max_lock_id
        self.server_number = server_number
        self.threads_per_server = threads_per_server
        #for i in range(1, server_number + 1):

def main():
    micro_benchmark = MicroBenchmark(SHARED_LOCK, 120000, 12)
    for i in range(1, micro_benchmark.server_number + 1):
        with open('shared/micro_bm_s'+str(i)+'.csv', mode='w') as output_file:
            csv_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
            csv_writer.writerow(['** on machine #'+str(i)])
            csv_writer.writerow(["** txn_id", "action", "target_lm_id", "target_obj_idx", "lock_type"])
            for j in range(1, micro_benchmark.lock_per_server + 1):
                txn_id = j % 1000
                action = ACQUIRE_LOCK
                target_lm_id = 2
                #lock_id = (i-1) * micro_benchmark.lock_per_server + j
                lock_id = i - 1
                lock_type = micro_benchmark.lock_type
                csv_writer.writerow([txn_id, action, target_lm_id, lock_id, lock_type])
    
    ## exclusive locks contention on client itself (between threads) hold by switch (2)
    micro_benchmark = MicroBenchmark(EXCLUSIVE_LOCK, 54000, 12)
    for i in range(1, micro_benchmark.server_number + 1):
        with open('ex_old/micro_bm_x'+str(i)+'.csv', mode='w') as output_file:
            csv_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
            csv_writer.writerow(['** on machine #'+str(i)])
            csv_writer.writerow(["** txn_id", "action", "target_lm_id", "target_obj_idx", "lock_type"])
            for j in range(1, micro_benchmark.lock_per_server + 1):
                txn_id = j % 1000
                action = ACQUIRE_LOCK
                target_lm_id = 2
                lock_id = (i-1) * micro_benchmark.lock_per_server + j
                lock_type = micro_benchmark.lock_type
                csv_writer.writerow([txn_id, action, target_lm_id, lock_id, lock_type])
    
    ## exclusive locks contention on clients (not on threads) hold by switch (2)
    micro_benchmark = MicroBenchmark(EXCLUSIVE_LOCK, 54000, 12)
    for i in range(1, micro_benchmark.server_number + 1):
        for l in range(1, 3):
            with open('ex_old/micro_bm_x'+str(i)+"_lc"+str(l+5)+'.csv', mode='w') as output_file:
                csv_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
                csv_writer.writerow(['** on machine #'+str(i)+' contention_degree: 2'])
                csv_writer.writerow(["** txn_id", "action", "target_lm_id", "target_obj_idx", "lock_type"])
                for j in range((l-1) * micro_benchmark.lock_per_server + 1, l * micro_benchmark.lock_per_server + 1):
                    txn_id = j % 1000
                    action = ACQUIRE_LOCK
                    target_lm_id = 2
                    lock_id = ((i-1) * micro_benchmark.lock_per_server + j + 55000 - 1) % 55000 + 1
                    lock_type = micro_benchmark.lock_type
                    csv_writer.writerow([txn_id, action, target_lm_id, lock_id, lock_type])
    
    ## exclusive locks contention on clients (not on threads) can't hold by switch
    for contention_degree in range(1,7):
        micro_benchmark = MicroBenchmark(EXCLUSIVE_LOCK, 54000, 12)
        for i in range(1, micro_benchmark.server_number + 1):
            for l in range(1, 3):
                with open('contention/queue_size_2/micro_bm_x'+str(i)+"_cd"+str(contention_degree)+"_lc"+str(l+5)+'.csv', mode='w') as output_file:
                    csv_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
                    csv_writer.writerow(['** on machine #'+str(i)+' contention_degree: '+str(contention_degree)])
                    csv_writer.writerow(["** txn_id", "action", "target_lm_id", "target_obj_idx", "lock_type"])
                    for j in range((l-1) * micro_benchmark.lock_per_server * contention_degree / micro_benchmark.threads_per_server + 1,
                                    l * micro_benchmark.lock_per_server * contention_degree / micro_benchmark.threads_per_server + 1):
                        txn_id = j % 1000
                        action = ACQUIRE_LOCK
                        target_lm_id = 2
                        lock_id = ((i-1) * micro_benchmark.lock_per_server + j + 55000 - 1) % 55000 + 1
                        lock_type = micro_benchmark.lock_type
                        csv_writer.writerow([txn_id, action, target_lm_id, lock_id, lock_type])
    return
    ## exclusive locks, test different contention (decide by number clients*threads, switch can hold)
    client_num = 1200
    # lock_nums = [1, 2, 3, 6, 10, 12, 20, 24, 30, 40, 50, 60, 70, 80, 90, 100, 120, 150, 200, 250, 300]
    # lock_nums = [150, 200, 250, 300, 350]
    lock_nums = []
    server_num = 12
    micro_benchmark = MicroBenchmark(EXCLUSIVE_LOCK, 55000, server_num, client_num / server_num)
    for lk in lock_nums:
        for i in range(1, micro_benchmark.server_number + 1):
            for j in range(0, micro_benchmark.threads_per_server):
                with open('contention/lk'+str(lk)+'/micro_bm_x'+str(i)+"_t"+str(j)+"_lk"+str(lk)+".csv", mode = 'w') as output_file:
                    csv_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
                    csv_writer.writerow(['** on machine #'+str(i)+' client: '+str(j)])
                    csv_writer.writerow(["** txn_id", "action", "target_lm_id", "target_obj_idx", "lock_type"])
                    for l in range(1200):
                        lock_id = randint(0, lk - 1)
                        txn_id = l % 1000
                        action = ACQUIRE_LOCK
                        target_lm_id = 2
                        lock_type = micro_benchmark.lock_type
                        csv_writer.writerow([txn_id, action, target_lm_id, lock_id, lock_type])

    ## exclusive locks, test different contention (decide by number of locks, switch can hold)
    client_num = 24
    # lock_nums = [1, 2, 3, 6, 10, 12, 20, 24, 30, 40, 50, 60, 70, 80, 90, 100, 120, 150, 200, 250, 300]
    # lock_nums = [150, 200, 250, 300, 350]
    # lock_nums = [2, 10, 100, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 8000, 10000]
    lock_nums = [2, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000, 9500, 10000]
    server_num = 12
    micro_benchmark = MicroBenchmark(EXCLUSIVE_LOCK, 55000, server_num, client_num / server_num)
    for lk in lock_nums:
        for i in range(1, micro_benchmark.server_number + 1):
            lk_list = range(lk)
            random.shuffle(lk_list)
            os.system("mkdir -p contention_shuffle; mkdir -p contention_shuffle/lk"+str(lk))
            for j in range(0, micro_benchmark.threads_per_server):
                ## cache miss
                shard_list = lk_list[j*lk / micro_benchmark.threads_per_server:(j+1)*lk / micro_benchmark.threads_per_server]
                shard_list.sort()
                with open('contention_shuffle/lk'+str(lk)+'/micro_bm_x'+str(i)+"_t"+str(j)+"_lk"+str(lk)+".csv", mode = 'w') as output_file:
                    csv_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
                    csv_writer.writerow(['** on machine #'+str(i)+' client: '+str(j)])
                    csv_writer.writerow(["** txn_id", "action", "target_lm_id", "target_obj_idx", "lock_type"])
                    for l in range(lk / micro_benchmark.threads_per_server):
                        # lock_id = j * lk / micro_benchmark.threads_per_server + lk_list[l + j * j * lk / micro_benchmark.threads_per_server]
                        lock_id = lk_list[l + j * lk / micro_benchmark.threads_per_server]
                        #lock_id = shard_list[l]
                        txn_id = l % 1000
                        action = ACQUIRE_LOCK
                        target_lm_id = 2
                        lock_type = micro_benchmark.lock_type
                        csv_writer.writerow([txn_id, action, target_lm_id, lock_id, lock_type])



if __name__ == '__main__':
    main()