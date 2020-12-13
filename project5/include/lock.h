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

struct Node {
	int tableID;
	int64_t recordID;
	void* tail;
	void* head;
	int acqiredCount;

	struct Node* hnext;
}typedef Node;

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
void trx_insert_lock(Trx* trx, lock_t* lock);
void trx_delete_duplicate_lock(Trx* trx);
Trx* trx_find(int trxID);
void trx_delete(int trxID);

/* APIs for lock table */
int init_lock_table(void);
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode);
int lock_release(lock_t* lock_obj);
void lock_engrave(lock_t* lock_obj, pagenum_t page_num, char* value);
int terminate_lock_table(void);
int lock_check_lock(void);
int lock_release_abort(lock_t* lock_obj);
<<<<<<< HEAD
int lock_detection(lock_t* lock, int trxID);
void lock_visited_initialize(void);

=======
>>>>>>> parent of 438d965... deadlock detection and abort finished

/* APIs for hash */
int hashKey(int tableID, int64_t recordID);
Node* setHash(int tableID, int64_t recordID);
Node* getHash(int tableID, int64_t recordID);
void deleteHash(int tableID, int64_t recordID);


#endif /* __LOCK_H__ */
