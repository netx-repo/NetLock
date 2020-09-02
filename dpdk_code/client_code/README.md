Current path: `dpdk_code/client_code/`

Compile:
`make`

Run:
`sudo ./build/client -l 0,1,2,3,4,5,6,7 -- -N8 -C10 -T0 -l10000 -r4 -bs -n1 -s5000`


# Note:
- `-l`: denotes the list of thread IDs. 0 is used for receive thread, which also updates the sending rate and print statistics. For others, thread ID k will send requests to server netx{k}.
- `-N`: specify the number of cores used to run, should be kept consistent with `-l`.
- `-C`: specify the number of clients.
- `-T`: specify the think time for a transaction, default: 0.
- `-l`: specify the number of memory slots used in the switch.
- `-r`: specify the number of cores used for receiving. In this example, 4 cores are used for receiving: core 0,1,2,3 are used only for sending, core 4,5,6,7 are used for receiving.
- `-b`: specify the benchmark.
    - `-bs`: microbenchmark - shared locks
    - `-bx`: microbenchmark - exclusive locks
    - `-bt`: microbenchmakr - TPC-C workload
- `-n`: specify the node number. (To better organize the result/log files for different machines)
- `-s`: specify the max sending rate (pkts per ms).

Arguments not used in this example:
- `-o`: specify the memory management algorithm.
    - `-ob`: the optimal allocation
    - `-ow`: the randomized allocation
- `-k`: specify the number of total locks.
- `-c`: specify the contention level (only used for microbenchark).
- `-w`: specify the number of warehouses (only used for TPC-C workload).
- `-i`: specify the packet sampling interval for latency calculation.
- `-e`: specify the number of clients each server should emulate, default: the same as the number of sending cores.
- `-a`: specify the task_id (experiment).
    - `-as`: microbenchmark - shared locks
    - `-ax`: microbenchmark - exclusive locks with no contention
    - `-ac`: microbenchmark - exclusive locks with contention
    - `-am`: memory management
    - `-aS`: memory size
    - `-at`: think time
    - `-ap`: TPC-C workload with 10v2 setting
    - `-aq`: TPC-C workload with 6v6 setting
    - `-af`: failure recovery experiment
    - `-ar`: policy support - priority