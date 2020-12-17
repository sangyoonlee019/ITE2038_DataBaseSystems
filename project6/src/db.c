#include "db.h"
#include "bpt.h"
#include "lock.h"
#include "file.h"


int init_db(int buf_num, int flag, int log_num, char* log_path, char* logmsg_path){
    return initDB(buf_num, flag, log_num, log_path, logmsg_path);
}

// int init_db(int buf_num){
//     return initDB(buf_num);
// }

int open_table(char* pathname){
    return openTable(pathname);
}

int db_insert(int table_id, int64_t key, char* value){
    return insert(table_id, key,value);
}

int db_find(int table_id, int64_t key, char* ret_val, int trx_id){
    return find_new(table_id, key, ret_val, trx_id);
}

int db_update (int table_id, int64_t key, char* values, int trx_id){
    return update(table_id, key, values, trx_id);
}

int db_delete (int table_id, int64_t key){
    return delete(table_id, key);
}

int close_table(int table_id){
    return closeTable(table_id);
}

int shutdown_db(){
    return shutdownDB();    
}

int trx_begin(void){
    return lock_trx_begin();
}

int trx_commit(int trx_id){
    return lock_trx_commit(trx_id);
}