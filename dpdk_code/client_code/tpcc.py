## tpcc - new order

## NURand(A,x,y)=(((random(0,A)| random(x,y))+C)%(y-x+1))+x

## w_id:    warehouse ID
## d_id:    district ID, which I think not useful for our current case
## c_id:    customer ID
## ol_cnt:  number of items in the order
## i_ids:   a list of item IDs
## i_id:    item ID
## rbk:     1% of the transactions chosen at random (error)

def gen_new_order(w_id, d_id, c_id, ol_cnt):
    i_ids = []
    for i in range(ol_cnt):
        i_ids = i_ids.append(NURand(8191, 1, 100000))

    #### warehouse table: get w_tax, (1 shared lock)
    lock_id = get_lock_id('warehouse_t', w_id)
    send_netlock_req(SHARED_LOCK, lock_id)

    #### district table: get d_tax, d_next_i_id++, (1 shared lock + 1 exclusive lock)
    lock_id_0 = get_lock_id('district_t', w_id, d_id, 0)
    send_netlock_req(SHARED_LOCK, lock_id_0)
    lock_id_1 = get_lock_id('district_t', w_id, d_id, 1)
    send_netlock_req(EXCLUSIVE_LOCK, lock_id_1)

    #### customer table: get c_discount, get c_last, get c_credit, (3 shared locks)
    for i in range(3):
        lock_id = get_lock_id('customer_t', w_id, d_id, c_id, i)
        send_netlock_req(SHARED_LOCK, lock_id)

    #### order table and new-order table: insert lines into the table, (2 exclusive locks or 2 exclusive table locks ? )
    lock_id_3 = get_lock_id('order_t')
    send_netlock_req(EXCLUSIVE_LOCK, lock_id_3)
    lock_id_4 = get_lock_id('order_t')
    send_netlock_req(EXCLUSIVE_LOCK, lock_id_4)

    #### ol_cnt numbers of items
    for i in range(ol_cnt):
        i_id = i_ids[i]
        #### item table: get i_price, i_name, i_data, (3 shared locks)
        for j in range(3):
            lock_id = get_lock_id('item_i', i_id, j)
            send_netlock_req(SHARED_LOCK, lock_id)

        #### stock table: update s_quantity, s_ytd, s_order_cnt/s_remote_cnt; get s_dist_xx, s_data (2 shared locks and 3 exclusive locks)
        for j in range(2):
            lock_id = get_lock_id('stock_t', i_id, w_id, j)
            send_netlock_req(SHARED_LOCK, lock_id)
        for j in range(3):
            lock_id = get_lock_id('stock_t', i_id, w_id, j+2)
            send_netlock_req(EXCLUSIVE_LOCK, lock_id)

        #### order-line table: insert line: ol_delivery_d, ol_number, pl_dist_info, (3 exclusive locks)
        for j in range(3):
            lock_id = get_lock_id('order_line_t', j)
            send_netlock_req(EXCLUSIVE_LOCK, lock_id)


