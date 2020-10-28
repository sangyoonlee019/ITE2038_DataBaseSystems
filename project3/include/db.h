#ifndef __DB_H__
#define __DB_H__

#include<inttypes.h>

int open_table(char* pathname);
int close_table(void);
int db_insert (int64_t key, char * value);
int db_find (int64_t key, char * ret_val);
int db_delete (int64_t key);

#endif /* __DB_H__*/