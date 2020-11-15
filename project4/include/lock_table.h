#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct lock_t lock_t;

#define MAX_HASH 10

struct Node {
	int tableID;
	int64_t recordID;
	void* tail;
	void* head;
	
	struct Node* hnext;
}typedef Node;

/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(int table_id, int64_t key);
int lock_release(lock_t* lock_obj);
int terminate_lock_table(void);

/* APIs for hash */
int hashKey(int tableID, int64_t recordID);
Node* setHash(int tableID, int64_t recordID);
Node* getHash(int tableID, int64_t recordID);
void deleteHash(int tableID, int64_t recordID);


#endif /* __LOCK_TABLE_H__ */
