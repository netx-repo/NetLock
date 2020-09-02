import os, sys, subprocess

MAX_LOCK_NUM = 7000000

def print_usage():
    print "Usage:"
    print "  ./statistic.py dirname client_num warehouse_num"
    return

def do_map(dirname, client_num, warehouse):
    print "dirname:", dirname
    cmd_list = "ls -l " + dirname + " | grep trace | awk -F [' ']+ '{print $9}'"
    output_list = subprocess.check_output(cmd_list, shell=True).strip('\n')
    file_list = output_list.split('\n')
    lock_num = 50000
    print file_list
    count = [0 for i in range(lock_num)]
    for filename in file_list:
        fullname = dirname.split('\\')[0] + dirname.split('\\')[1] + "/" + filename
        print fullname
        fin = open(fullname)
        fin.readline()
        fin.readline()
        while True:
            line = fin.readline()
            if not line:
                break
            words = [x.strip() for x in line.split(',')]
            txn_id = int(words[0])
            action_type = int(words[1])
            target_lm_id = int(words[2])
            target_obj_idx = int(words[3])
            lock_type = int(words[4])
            target_obj_idx = target_obj_idx + 1
            if (lock_type == 7) or (lock_type == 8):
                lock_type = 1
            elif (lock_type == 9):
                lock_type = 2
            lock_type = lock_type - 1

            if (target_obj_idx <= 20):
                lock_id = target_obj_idx
            else:
                lock_id = 20 + (target_obj_idx - 20) / 150
            count[lock_id] += 1
    l_count = []
    for i in range(lock_num + 1):
        l_count.append((i, count[i]))
    l_count.sort(key = (lambda element:element[1]), reverse = True)
    fout = open("stat/tpcc_incast_"+ client_num +"_w_" + warehouse +"_domap.stat", "w")
    for i in range(lock_num + 1):
        if l_count[i][1] > 0:
            fout.write(str(l_count[i][0])+','+str(l_count[i][1])+'\n')
    fout.close()

def no_map(dirname, client_num, warehouse):
    print "dirname:", dirname
    cmd_list = "ls -l " + dirname + " | grep trace | awk -F [' ']+ '{print $9}'"
    output_list = subprocess.check_output(cmd_list, shell=True).strip('\n')
    file_list = output_list.split('\n')
    print file_list
    lock_num = MAX_LOCK_NUM
    count = [0 for i in range(lock_num+1)]
    for filename in file_list:
        num_file = int(filename.strip('.csv').strip('trace_'))
        if num_file > 64+40:
            continue
        fullname = dirname.split('\\')[0] + dirname.split('\\')[1] + "/" + filename
        print fullname
        fin = open(fullname)
        fin.readline()
        fin.readline()
        while True:
            line = fin.readline()
            if not line:
                break
            words = [x.strip() for x in line.split(',')]
            if (len(words) <= 4):
                continue
            txn_id = int(words[0])
            action_type = int(words[1])
            if (action_type == 1):
                continue
            target_lm_id = int(words[2])
            target_obj_idx = int(words[3])
            lock_type = int(words[4])
            target_obj_idx = target_obj_idx + 1
            if (lock_type == 7) or (lock_type == 8):
                lock_type = 1
            elif (lock_type == 9):
                lock_type = 2
            lock_type = lock_type - 1
            lock_id = target_obj_idx
            count[lock_id] += 1
    l_count = []
    for i in range(lock_num + 1):
        l_count.append((i, count[i]))
    l_count.sort(key = (lambda element:element[1]), reverse = True)
    fout = open("stat/tpcc_multiserver_"+ client_num +"_w_" + warehouse +"_nomap.stat", "w")
    for i in range(lock_num + 1):
        if l_count[i][1] > 0:
            fout.write(str(l_count[i][0])+','+str(l_count[i][1])+'\n')
    fout.close()

def max_lock_freq_per_txn(dirname, client_num, warehouse):
    print "dirname:", dirname
    cmd_list = "ls -l " + dirname + " | grep trace | awk -F [' ']+ '{print $9}'"
    output_list = subprocess.check_output(cmd_list, shell=True).strip('\n')
    file_list = output_list.split('\n')
    print file_list
    lock_num = MAX_LOCK_NUM
    old_txn_id = -1
    count = {}
    max_c = 0
    for filename in file_list:
        fullname = dirname.split('\\')[0] + dirname.split('\\')[1] + "/" + filename
        print fullname
        fin = open(fullname)
        fin.readline()
        fin.readline()
        while True:
            line = fin.readline()
            if not line:
                break
            words = [x.strip() for x in line.split(',')]
            txn_id = int(words[0])


            action_type = int(words[1])
            if action_type == 1:
                continue
            if (txn_id != old_txn_id):
                if (len(count) != 0):
                    for i in count:
                        if (count[i] > max_c):
                            max_c = count[i]
                            max_lock = i
                            max_txn = old_txn_id
                            count_log = count
                    # if (max(count.values()) > max_c):
                    #     max_c = max(count.values())
                    #     max_lock = max(count, key=count.get)
                    #     max_txn = old_txn_id
                    #     count_log = count
                old_txn_id = txn_id
                count = {}
            target_lm_id = int(words[2])
            target_obj_idx = int(words[3])
            lock_type = int(words[4])
            target_obj_idx = target_obj_idx + 1
            if (lock_type == 7) or (lock_type == 8):
                lock_type = 1
            elif (lock_type == 9):
                lock_type = 2
            lock_type = lock_type - 1
            lock_id = target_obj_idx
            if (lock_id not in count):
                count[lock_id] = 1
            else:
                count[lock_id] += 1
        print max_txn, ":", max_lock, ":", max_c
        print count_log

    print "max count:", max_c

def main():
    if (len(sys.argv) <= 3):
        print_usage()
        sys.exit(-1)
    dirname = sys.argv[1]
    client_num = sys.argv[2]
    warehouse = sys.argv[3]
    # max_lock_freq_per_txn(dirname, client_num, warehouse)
    no_map(dirname, client_num, warehouse)
    

if __name__ == '__main__':
    main()