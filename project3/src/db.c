#include "db.h"
#include "bpt.h"
#include "file.h"


int init_db(int buf_num){
    return initDB(buf_num);
}

int open_table(char* pathname){
    return openTable(pathname);
}

int db_insert(int table_id, int64_t key, char* value){
    return insert(table_id, key,value);
}

int db_find(int table_id, int64_t key, char* ret_val){
    return find(table_id, key, ret_val);
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