#ifndef __DB_H__
#define __DB_H__

#include<inttypes.h>

// int init_db(int buf_num);
int init_db(int buf_num, int flag, int log_num, char* log_path, char* logmsg_path);
int open_table(char* pathname);
int close_table(int table_id);
int db_insert (int table_id, int64_t key, char * value);
int db_find (int table_id, int64_t key, char * ret_val, int trx_id);
int db_update (int table_id, int64_t key, char* values, int trx_id);
int db_delete (int table_id, int64_t key);
int shutdown_db(void);
int trx_begin(void);
int trx_commit(int trx_id);
void check_lock(void);

#endif /* __DB_H__*/