#include "db.h"
#include "bpt.h"
#include "file.h"


int init_db(int buf_num){
    return initDB(buf_num);
}

int open_table(char* pathname){
    return openTable(pathname);
}

int db_insert(int64_t key, char* value){
    return insert(key,value);
}

int db_find(int64_t key, char* ret_val){
    return find(key, ret_val);
}

int db_delete (int64_t key){
    return delete(key);
}

int close_table(){
    return closeTable();
}