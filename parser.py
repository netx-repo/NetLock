import sys, os, time, subprocess, random
import threading
from multiprocessing import Process

class Parser:
    def __init__(self, client_num, dirnames):
        self.client_num = client_num
        self.dirnames = dirnames
        return

    def parse_rx(self):
        res_tput = []
        for dirname in self.dirnames:
            # cmd_tput = "cat " + dirname + "res_rc* | grep 'rx:' | awk -F [' ']+ 'BEGIN{t=0}{t+=$2}END{print t/1000000}'"
            cmd_tput = "cat " + dirname + "res_rc* | grep 'rx:' | awk -F [' ']+ 'BEGIN{t=0}{t+=$2}END{print t}'"
            out_tput = subprocess.check_output(cmd_tput, shell=True).strip('\n')
            res_tput.append(float(out_tput))
            print dirname, out_tput
        print res_tput
        return

    def parse_average_latency(self):
        res_avg = []
        for dirname in self.dirnames:
            cmd_average = "cat " + dirname + "res_rc* | grep 'Average' | awk -F [' ']+ 'BEGIN{t=0}{t+=$3}END{print t*1000/"+str(self.client_num)+"}'"
            output_average = subprocess.check_output(cmd_average, shell=True).strip('\n')
            res_avg.append(float(output_average))
            print dirname, output_average
        print res_avg
        return

    def parse_median_latency(self):
        res_med = []
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            res_med.append(float(latency_list[len(latency_list) / 2]))
            print dirname, latency_list[len(latency_list) / 2]
        print res_med
        return

    def parse_99_latency(self):
        res_99 = []
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            res_99.append(float(latency_list[len(latency_list) * 99 / 100]))
            print dirname, latency_list[len(latency_list) * 99 / 100]
        print res_99
        return

    def parse_999_latency(self):
        res_999 = []
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            res_999.append(float(latency_list[len(latency_list) * 999 / 1000]))
            print dirname, latency_list[len(latency_list) * 999 / 1000]
        print res_999
        return

    def parse_txn_rx(self):
        res_tput = []
        for dirname in self.dirnames:
            # cmd_tput = "cat " + dirname + "res_rc* | grep 'txn' | awk -F [' ']+ 'BEGIN{t=0}{t+=$2}END{print t/1000000}'"
            cmd_tput = "cat " + dirname + "res_rc* | grep 'txn' | awk -F [' ']+ 'BEGIN{t=0}{t+=$2}END{print t}'"
            out_tput = subprocess.check_output(cmd_tput, shell=True).strip('\n')
            res_tput.append(float(out_tput))
            print dirname, out_tput
        print res_tput
        return

    def parse_avg_txn_latency(self):
        res_avg = []
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "txn_latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            res_avg.append(sum(latency_list) * 1.0 / len(latency_list))
            print dirname, sum(latency_list) * 1.0/ len(latency_list)
        print res_avg
        return

    def parse_99_txn_latency(self):
        res_99 = []
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "txn_latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            res_99.append(float(latency_list[len(latency_list) * 99 / 100]))
            print dirname, latency_list[len(latency_list) * 99 / 100]
        print res_99
        return
    
    def parse_999_txn_latency(self):
        res_999 = []
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "txn_latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            res_999.append(float(latency_list[len(latency_list) * 999 / 1000]))
            print dirname, latency_list[len(latency_list) * 999 / 1000]
        print res_999
        return

    def parse_latency_cdf(self):
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            # print latency_list
            max_latency = latency_list[len(latency_list) - 11]
            max_latency = 436
            count = [0 for i in range(5000)]
            tot = 0
            for i in range(len(latency_list)-50):
                count[latency_list[i]] += 1
                tot += 1
            for i in range(2, max_latency+1):
                count[i] = count[i] + count[i-1]
            print range(150+1)
            for i in range(1, max_latency+1):
                count[i] = count[i] * 1.0 / tot
            print count[0:150+1]
        return

    def parse_txn_latency_cdf(self):
        for dirname in self.dirnames:
            cmd_percent = "cat " + dirname + "txn_latency_rc*"
            out_percent = subprocess.check_output(cmd_percent, shell=True).strip('\n')
            latency_list = []
            for line in out_percent.split('\n'):
                latency_list.append(int(line))
            latency_list.sort()
            # print latency_list
            max_latency = latency_list[len(latency_list) - 11]
            max_latency = 4000
            count = [0 for i in range(max_latency + 1)]
            tot = 0
            for i in range(len(latency_list)-50):
                if (latency_list[i] < max_latency):
                    count[latency_list[i]] += 1
                    tot += 1
            for i in range(2, max_latency+1):
                count[i] = count[i] + count[i-1]
            print range(1000+1)
            for i in range(1, max_latency+1):
                count[i] = count[i] * 1.0 / tot
            print count[0:1000+1]
        return

    def priority(self):
        for dirname in self.dirnames:
            pr_rx_list = [[] for i in range(13)]
            pr_total_rx_1 = [0 for i in range(400)]
            for i in range(1, 6):
                filename = dirname + "client_run_" + str(i) + ".log"
                cmd_rx = "cat " + filename + " | grep 'rx_per' | awk -F ':' '{print $2}'"
                out_rx = subprocess.check_output(cmd_rx, shell = True).strip('\n')
                pr_rx_list[i] = []
                count = 0 
                for line in out_rx.split('\n'):
                    pr_rx_list[i].append(int(line))
                    pr_total_rx_1[count] = pr_total_rx_1[count] + pr_rx_list[i][count]
                    count += 1

            pr_total_rx_2 = [0 for i in range(400)]
            for i in range(6, 11):
                filename = dirname + "client_run_" + str(i) + ".log"
                cmd_rx = "cat " + filename + " | grep 'rx_per' | awk -F ':' '{print $2}'"
                out_rx = subprocess.check_output(cmd_rx, shell = True).strip('\n')
                pr_rx_list[i] = []
                count = 0 
                for line in out_rx.split('\n'):
                    pr_rx_list[i].append(int(line))
                    pr_total_rx_2[count] = pr_total_rx_2[count] + pr_rx_list[i][count]
                    count += 1

            nopr_rx_list = [[] for i in range(13)]
            nopr_total_rx_1 = [0 for i in range(400)]
            for i in range(1, 6):
                filename = dirname + "client_run_even_" + str(i) + ".log"
                cmd_rx = "cat " + filename + " | grep 'rx_per' | awk -F ':' '{print $2}'"
                out_rx = subprocess.check_output(cmd_rx, shell = True).strip('\n')
                nopr_rx_list[i] = []
                count = 0
                for line in out_rx.split('\n'):
                    nopr_rx_list[i].append(int(line))
                    nopr_total_rx_1[count] = nopr_total_rx_1[count] + nopr_rx_list[i][count]
                    count += 1

            nopr_total_rx_2 = [0 for i in range(400)]
            for i in range(6, 11):
                filename = dirname + "client_run_even_" + str(i) + ".log"
                cmd_rx = "cat " + filename + " | grep 'rx_per' | awk -F ':' '{print $2}'"
                out_rx = subprocess.check_output(cmd_rx, shell = True).strip('\n')
                nopr_rx_list[i] = []
                count = 0
                for line in out_rx.split('\n'):
                    nopr_rx_list[i].append(int(line))
                    nopr_total_rx_2[count] = nopr_total_rx_2[count] + nopr_rx_list[i][count]
                    count += 1
            print pr_total_rx_1
            print pr_total_rx_2
            print nopr_total_rx_1
            print nopr_total_rx_2

    def failure(self):
        for dirname in self.dirnames:
            failure_rx_list = [[] for i in range(13)]
            failure_rx_4 = [0 for i in range(4000)]
            failure_rx_5 = [0 for i in range(4000)]
            failure_rx_6 = [0 for i in range(4000)]
            failure_rx_7 = [0 for i in range(4000)]
            failure_total_rx = [0 for i in range(1000)]
            for i in range(1, 10):
                filename = dirname + "client_run_even_" + str(i) + ".log"
                cmd_rx_4 = "cat " + filename + " | grep 'core 4' | grep 'rx:' | awk -F [' ']+ '{print $4}' | awk -F ['\t'] '{print $1}'"
                out_rx_4 = subprocess.check_output(cmd_rx_4, shell = True).strip('\n')
                cmd_rx_5 = "cat " + filename + " | grep 'core 5' | grep 'rx:' | awk -F [' ']+ '{print $4}' | awk -F ['\t'] '{print $1}'"
                out_rx_5 = subprocess.check_output(cmd_rx_5, shell = True).strip('\n')
                cmd_rx_6 = "cat " + filename + " | grep 'core 6' | grep 'rx:' | awk -F [' ']+ '{print $4}' | awk -F ['\t'] '{print $1}'"
                out_rx_6 = subprocess.check_output(cmd_rx_6, shell = True).strip('\n')
                cmd_rx_7 = "cat " + filename + " | grep 'core 7' | grep 'rx:' | awk -F [' ']+ '{print $4}' | awk -F ['\t'] '{print $1}'"
                out_rx_7 = subprocess.check_output(cmd_rx_7, shell = True).strip('\n')
                failure_rx_list[i] = []
                count = 0
                for line in out_rx_4.split('\n'):
                    # failure_rx_list[i].append(int(line))
                    failure_total_rx[count] = failure_total_rx[count] + int(line) #failure_rx_list[i][count]
                    count += 1
                count = 0
                for line in out_rx_5.split('\n'):
                    # failure_rx_list[i].append(int(line))
                    failure_total_rx[count] = failure_total_rx[count] + int(line) #failure_rx_list[i][count]
                    count += 1
                count = 0
                for line in out_rx_6.split('\n'):
                    # failure_rx_list[i].append(int(line))
                    failure_total_rx[count] = failure_total_rx[count] + int(line) #failure_rx_list[i][count]
                    count += 1
                count = 0
                for line in out_rx_7.split('\n'):
                    # failure_rx_list[i].append(int(line))
                    failure_total_rx[count] = failure_total_rx[count] + int(line) #failure_rx_list[i][count]
                    count += 1
            print failure_total_rx
            diff = [0 for i in range(1000)]
            for i in range(1, len(failure_total_rx)):
                if failure_total_rx[i] > failure_total_rx[i-1]:
                    diff[i] = failure_total_rx[i] - failure_total_rx[i-1]
            diff[0] = failure_total_rx[0]
            print diff

def print_usage():
    print "Usage:"
    print "  parser.py tput [micro_bm_s, micro_bm_x, micro_bm_cont, tpcc, tpcc_ms, mem_size, mem_man]"
    print "  parser.py txn_tput [tpcc, tpcc_ms]"
    print "  parser.py avg_latency [micro_bm_s, micro_bm_x, micro_bm_cont, tpcc, tpcc_ms]"
    print "  parser.py med_latency [micro_bm_s, micro_bm_x, micro_bm_cont, tpcc, tpcc_ms]"
    print "  parser.py 99_latency [micro_bm_s, micro_bm_x, micro_bm_cont, tpcc, tpcc_ms]"
    print "  parser.py 99.9_latency [micro_bm_s, micro_bm_x, micro_bm_cont, tpcc, tpcc_ms]"
    print "  parser.py txn_avg_latency [tpcc, tpcc_ms]"
    print "  parser.py txn_99_latency [tpcc, tpcc_ms]"
    print "  parser.py txn_99.9_latency [tpcc, tpcc_ms]"
    print "  parser.py cdf_latency [micro_bm_s, micro_bm_x, micro_bm_cont, tpcc, tpcc_ms]"

def main():
    if (len(sys.argv) <= 2):
        print_usage()
        sys.exit(0)
    else:
        dirnames = []
        client_num = 10
        if (sys.argv[2] == "micro_bm_s"):
            dirnames = ["results/shared/cn_2_lk_12/"]
            client_num = 12
        elif (sys.argv[2] == "micro_bm_x"):
            dirnames = ["results/exclusive/cn_2_lk_55000/"]
            client_num = 12
        elif (sys.argv[2] == "micro_bm_cont"):
            lk_num_list = [500, 1000, 1500, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000]
            for lk_num in lk_num_list:
                dirnames.append("results/contention/cn_2_lk_" + str(lk_num) + "/")
            client_num = 12
        elif (sys.argv[2] == "tpcc"):
            warehouse_list = [1, 10]
            for warehouse in warehouse_list:
                dirnames.append("results/tpcc/10v2/cn_4_wh_" + str(warehouse) + "/")
            client_num = 10
        elif (sys.argv[2] == "tpcc_ms"):
            warehouse_list = [1, 10]
            for warehouse in warehouse_list:
                dirnames.append("results/tpcc/6v6/cn_4_wh_" + str(warehouse) + "/")
            client_num = 6
        elif (sys.argv[2] == "mem_size"):
            mem_size_list = [2000, 3000, 4000, 5000, 10000, 20000]
            for mem_size in mem_size_list:
                dirnames.append("results/mem_size/tpcc_uniform/cn_4_sn_" + str(mem_size) + "/")
            client_num = 10
        elif (sys.argv[2] == "mem_man"):
            mem_man_list = ["random", "binpack"]
            for mem_man in mem_man_list:
                dirnames.append("results/mem_manage/" + mem_man + "/cn_4_lk_7000000_sn_130000" + "/")
            client_num = 10
        else:
            print_usage()
            sys.exit(0)

        par = Parser(client_num, dirnames)

        if (sys.argv[1] == "tput"):
            par.parse_rx()
        elif (sys.argv[1] == "avg_latency"):
            par.parse_average_latency()
        elif (sys.argv[1] == "med_latency"):
            par.parse_median_latency()
        elif (sys.argv[1] == "99_latency"):
            par.parse_99_latency()
        elif (sys.argv[1] == "99.9_latency"):
            par.parse_999_latency()
        elif (sys.argv[1] == "cdf_latency"):
            par.parse_latency_cdf()
        elif (sys.argv[1] == "txn_avg_latency"):
            par.parse_avg_txn_latency()
        elif (sys.argv[1] == "txn_99_latency"):
            par.parse_99_txn_latency()
        elif (sys.argv[1] == "txn_99.9_latency"):
            par.parse_999_txn_latency()
        elif (sys.argv[1] == "txn_tput"):
            par.parse_txn_rx()
        elif (sys.argv[1] == "txn_cdf_latency"):
            par.parse_txn_latency_cdf()
        elif (sys.argv[1] == "failure"):
            par.failure()
        else:
            print_usage()
        
        

if __name__ == '__main__':
    main()
