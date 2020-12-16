#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include "file.h"

typedef struct lock_t lock_t;

#define MAX_HASH 20

#define DEFAULT_TRX_ID 1
#define LM_SHARED 0
#define LM_EXCLUSIVE 1

#define LS_ACQIRED 1
#define LS_WAITING 0

#define ACQUIRED 0
#define NEED_TO_WAIT 1
#define DEADLOCK 2
#define UNDIFINED -1

struct Node {
	int tableID;
	int64_t recordID;
	void* tail;
	void* head;
	int acqiredCount;

	struct Node* hnext;
}typedef Node;

struct lock_t {
	struct lock_t* prev;
	struct lock_t* next;
	Node* sentinal;
	pthread_cond_t cond;
	int lock_mode;
	struct lock_t* trx_next;
	int owner_trx_id;
	int state;
	pagenum_t page_num;
	char history[120];
	int visited;
}typedef lock_t;

struct Trx {
	int trxID;
	lock_t* head;

	struct Trx* next;
}typedef Trx;


/* APIs for transaction */
int lock_trx_begin(void);
int lock_trx_commit(int trxID);
int trx_abort(int trxID);
int trx_check_lock(void);
void trx_print_lock(int trxID);

/* APIs for transaction table */
int trx_hash(int trxID);
Trx* trx_new(int trxID);
void trx_insert_lock(int trx_id, lock_t* lock);
Trx* trx_find(int trxID);
void trx_delete(int trxID);

/* APIs for lock table */
int init_lock_table(void);
int lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode,lock_t* ret_lock);
int lock_release(lock_t* lock_obj);
void lock_engrave(lock_t* lock_obj, pagenum_t page_num, char* value);
int terminate_lock_table(void);
int lock_check_lock(void);
int lock_release_abort(lock_t* lock_obj);
void lock_visited_initialize(void);
int lock_detection(lock_t* lock, int trxID);

/* APIs for hash */
int hashKey(int tableID, int64_t recordID);
Node* setHash(int tableID, int64_t recordID);
Node* getHash(int tableID, int64_t recordID);
void deleteHash(int tableID, int64_t recordID);


#endif /* __LOCK_H__ */
