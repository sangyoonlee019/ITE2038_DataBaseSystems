#ifndef __DB_H__
#define __DB_H__

#include<inttypes.h>

int init_db(int buf_num);
int open_table(char* pathname);
int close_table(int table_id);
int db_insert (int table_id, int64_t key, char * value);
int db_find (int table_id, int64_t key, char * ret_val);
int db_delete (int table_id, int64_t key);
int shutdown_db(void);

#endif /* __DB_H__*/