#include "lock.h"
#include "buf.h"
#include "file.h"
#include "db.h"
#include "log.h"



Node* lockTable[MAX_HASH];
Trx* trxTable[MAX_HASH];

int logmsgFile;
int currentTrxID = DEFAULT_TRX_ID;
int debug1 = 1;
pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trx_table_latch = PTHREAD_MUTEX_INITIALIZER;

/* APIs for transaction */
int 
lock_trx_begin(void){
	pthread_mutex_lock(&trx_table_latch);
	
	int newTrxID = currentTrxID;
	Trx* newTrx = trx_new(newTrxID);
	if (newTrx==NULL){
		pthread_mutex_unlock(&trx_table_latch);
		printf("error: trx begin is failed!\n");
		return 0;
	}
	currentTrxID++;

	Log log;
	lsn_t offset;
	offset = log_new(&log, newTrxID, -1, LT_BEGIN); 	
	log_write_log(&log);
	newTrx->leastLSN = offset;	

	pthread_mutex_unlock(&trx_table_latch);
	return newTrxID;
}

int 
lock_trx_commit(int trxID){
	lsn_t prevLSN;

	if(debug1) printf("Trx%d: trx_commit (lock: %d trx: %d)\n",trxID,lock_check_lock(),trx_check_lock());
	pthread_mutex_lock(&trx_table_latch);
	Trx* trx = trx_find(trxID);
	if (trx == NULL){
		pthread_mutex_unlock(&trx_table_latch);
		return 0;
	}
	lock_t* lock = trx->head;
	if (lock ==NULL){
		pthread_mutex_unlock(&trx_table_latch);
		return -1;
	}

	lock_t* nextLock = lock->trx_next;
	
	lock_release(lock);
	while(nextLock){
		lock = nextLock;
		nextLock = lock->trx_next;
		lock_release(lock);
	}
	
	prevLSN = trx->leastLSN;
	trx_delete(trxID);

	Log log;
	log_new(&log, trxID, prevLSN, LT_COMMIT); 	
	log_write_log(&log);

	pthread_mutex_unlock(&trx_table_latch);
	if(debug1) printf("Trx%d: trx_commit end (lock: %d trx: %d)\n",trxID,lock_check_lock(),trx_check_lock());
	
	return trxID;
}

int trx_abort(int trxID){
	if(debug1) printf("Trx%d: trx_abort (lock: %d trx: %d buf: %d)\n",trxID,lock_check_lock(),trx_check_lock(),buf_check_lock());
	pthread_mutex_lock(&trx_table_latch);
	Trx* trx = trx_find(trxID);
	if (trx==NULL){
		printf("trx_abort error: trx is NULL\n");
		return -1;
	} 
	
	// Compensation
	Log clog;
	lsn_t retLSN = log_read_log(trx->leastLSN, &clog);
	while(retLSN && clog.type==LT_UPDATE){
		Log newLog;
		log_new(&newLog,trxID,trx->leastLSN, LT_COMPENSATE);
		// New Log Info
		newLog.tableID = clog.tableID;
		newLog.pageNumber = clog.pageNumber;
		newLog.offset = clog.offset;
		newLog.dataLength = clog.dataLength;
		memcpy(newLog.newImage,clog.oldImage,VALUE_SIZE);
		memcpy(newLog.oldImage,clog.newImage,VALUE_SIZE);

		trx->leastLSN = newLog.LSN; 
		log_write_log(&newLog);

		// Rollback
		LeafPage leafNode;
		buf_get_page(clog.tableID, clog.pageNumber, (page_t*)&leafNode);
		memcpy(&leafNode+clog.offset,clog.oldImage,VALUE_SIZE);
		buf_set_page(clog.tableID, clog.pageNumber, (page_t*)&leafNode);

		log_read_log(clog.prevLSN, &clog);
	}

	// // printf("@1\n");
	// lock_t* lock = trx->head;
	// if (lock ==NULL){
	// 	return -1;
	// }
	// // printf("@2\n");
	// lock_t* nextLock = lock->trx_next;
	// if (lock->lock_mode == LM_EXCLUSIVE){
	// 	// Roll Back
	// 	LeafPage leafNode;
	// 	// printf("buf: %d\n",buf_check_lock());
	// 	buf_get_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
	// 	for (int i=0;i<leafNode.numberOfKeys;i++){
	// 		if (leafNode.records[i].key == lock->sentinal->recordID){
	// 			// printf("history1\n");
	// 			memcpy(leafNode.records[i].value,lock->history,VALUE_SIZE);
				// buf_set_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
	// 			// printf("history1-end\n");
	// 			break;
	// 		}
	// 	}
	// 	buf_unpin_page(lock->sentinal->tableID, lock->page_num);
	// 	// printf("bufend: %d\n",buf_check_lock());
	// }
	// // printf("@3\n");
	// lock_release(lock);
	// // printf("@4\n");
	// while(nextLock){
	// 	lock = nextLock;
	// 	if (lock->lock_mode == LM_EXCLUSIVE){
	// 		// Roll Back
	// 		LeafPage leafNode;
	// 		// printf("buf: %d\n",buf_check_lock());
	// 		buf_get_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
	// 		for (int i=0;i<leafNode.numberOfKeys;i++){
	// 			if (leafNode.records[i].key == lock->sentinal->recordID){
	// 				// printf("history2\n");
	// 				memcpy(leafNode.records[i].value,lock->history,VALUE_SIZE);
	// 				buf_set_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
	// 				// printf("history2-end\n");
	// 				break;
	// 			}
	// 		}
	// 		buf_unpin_page(lock->sentinal->tableID, lock->page_num);
	// 		// printf("bufend: %d\n",buf_check_lock());
	// 	}

	// 	nextLock = lock->trx_next;
	// 	lock_release(lock);
	// }
	// printf("abort_end!\n");

	

	Log rlog;
	log_new(&rlog, trxID, trx->leastLSN, LT_ROLLBACK); 	
	log_write_log(&rlog);
	// printf("abort_end2!\n");
	trx_delete(trxID);
	log_flush();
	pthread_mutex_unlock(&trx_table_latch);
	if(debug1) printf("Trx%d: trx_abort end (lock: %d trx: %d)\n",trxID,lock_check_lock(),trx_check_lock());
	return trxID;
}

/* APIs for transaction table */
int 
trx_hash(int trxID){
	return trxID%MAX_HASH;
}

Trx* 
trx_new(int trxID)
{	
	Trx* newTrx = (Trx*)malloc(sizeof(Trx));
	if (newTrx==NULL){
		return NULL;
	}

	newTrx->trxID = trxID;
	pthread_mutex_init(&newTrx->trx_latch,NULL);
	newTrx->head = NULL;
	newTrx->next = NULL; 

    int hash_key = trx_hash(trxID);
    if (trxTable[hash_key] == NULL)
    {
        trxTable[hash_key] = newTrx;
    }
    else
    {
        newTrx->next = trxTable[hash_key];
        trxTable[hash_key] = newTrx;
	}

	return newTrx;
}

Trx* 
trx_find(int trxID)
{
    int hash_key = trx_hash(trxID);
    if (trxTable[hash_key] == NULL)
        return NULL;
 
    if (trxTable[hash_key]->trxID == trxID)
    {
        return trxTable[hash_key];
    }
    else
    {
        Trx* trx = trxTable[hash_key];
        while (trx->next)
        {
            if (trx->next->trxID == trxID)
            {
                return trx->next;
            }
            trx = trx->next;
        }
    }
    return  NULL;
}

lsn_t trx_leastLSN(int trxID, lsn_t LSN){
	lsn_t prevLSN;
	// pthread_mutex_lock(&trx_table_latch);
	Trx* trx = trx_find(trxID);
	prevLSN = trx->leastLSN;
	trx->leastLSN = LSN;
	// pthread_mutex_unlock(&trx_table_latch);
	return prevLSN;
}

void trx_insert_lock(int trx_id, lock_t* lock, int trx_lock){
	Trx* trx = trx_find(trx_id);
	if (trx==NULL){
		printf("error: trx_find\n");
	} 
	
	lock_t* currentLock = trx->head;
	if (currentLock==NULL){
		trx->head = lock;
	}else{
		while(currentLock->trx_next!=NULL){
			currentLock=currentLock->trx_next;
		}
		currentLock->trx_next = lock;
	}

}

void 
trx_delete(int trxID)
{
    int hash_key = trx_hash(trxID);
    if (trxTable[hash_key] == NULL)
        return;
 
    Trx* delTrx = NULL;
    if (trxTable[hash_key]->trxID == trxID)
    {
        delTrx = trxTable[hash_key];
        trxTable[hash_key] = trxTable[hash_key]->next;
    }
    else
    {
        Trx* trx = trxTable[hash_key];
    	Trx* next = trx->next;
        while (next)
        {
			if (next->trxID == trxID)
            {
                trx->next = next->next;
                delTrx = next;
                break;
            }
            trx = next;
            next = trx->next; 
        }
    }
    free(delTrx); 
}

int trx_check_lock(){
	if (pthread_mutex_trylock(&trx_table_latch)!=0){
		return 1;
	}else{
		pthread_mutex_unlock(&trx_table_latch);
		return 0;
	}
}

int lock_check_lock(){
	if (pthread_mutex_trylock(&lock_table_latch)!=0){
		return 1;
	}else{
		pthread_mutex_unlock(&lock_table_latch);
		return 0;
	}
}

void trx_print_lock(int trxID){	
	Trx* trx = trx_find(trxID);
	printf("trx%d: ",trxID);
	lock_t* lock = trx->head;
	while(lock){
		if (lock->lock_mode==LM_SHARED) printf("- S%lld",lock->sentinal->recordID);
		else printf("- X%lld",lock->sentinal->recordID);
		printf("(%d) ",lock->state);
		lock = lock->trx_next;
	}
	printf("\n");
	
}

int
init_lock_table()
{
	return 0;
}

int 
lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode,lock_t* lock)
{	
	if(debug1 && lock_mode==LM_SHARED) printf("Trx%d: lock_acqire: S%lld (lock: %d trx: %d)\n",trx_id,key,lock_check_lock(),trx_check_lock());
	if(debug1 && lock_mode==LM_EXCLUSIVE) printf("Trx%d: lock_acqire: X%lld (lock: %d trx: %d)\n",trx_id,key,lock_check_lock(),trx_check_lock());
	pthread_mutex_lock(&trx_table_latch);
	if(debug1 && lock_mode==LM_SHARED) printf("Trx%d: lock_acqire2: S%lld (lock: %d trx: %d)\n",trx_id,key,lock_check_lock(),trx_check_lock());
	if(debug1 && lock_mode==LM_EXCLUSIVE) printf("Trx%d: lock_acqire2: X%lld (lock: %d trx: %d)\n",trx_id,key,lock_check_lock(),trx_check_lock());
	pthread_mutex_lock(&lock_table_latch);
	if(debug1 && lock_mode==LM_SHARED) printf("Trx%d: lock_acqire3: S%lld (lock: %d trx: %d)\n",trx_id,key,lock_check_lock(),trx_check_lock());
	if(debug1 && lock_mode==LM_EXCLUSIVE) printf("Trx%d: lock_acqire3: X%lld (lock: %d trx: %d)\n",trx_id,key,lock_check_lock(),trx_check_lock());
	
	Node* node = getHash(table_id,key);  

	if (node==NULL){
		node = setHash(table_id, key);

		lock->prev=NULL;
		lock->next=NULL;
		lock->sentinal=node;
		lock->lock_mode = lock_mode;
		lock->owner_trx_id = trx_id;
		lock->state = LS_ACQIRED;
		lock->trx_next = NULL;
		lock->visited = 0;

		node->tail = lock;
		node->head = lock;
		node->acqiredCount++;

		trx_insert_lock(trx_id,lock,0);
		pthread_mutex_unlock(&trx_table_latch);
	}else{
		lock->prev=NULL;
		lock->next=NULL;
		lock->sentinal=node;
		lock->lock_mode = lock_mode;
		lock->owner_trx_id = trx_id;
		lock->trx_next = NULL;
		lock->visited = 0;

		if(node->head==NULL){
			lock->state = LS_ACQIRED;

			node->tail = lock;
			node->head = lock;
			node->acqiredCount++;
	
			trx_insert_lock(trx_id,lock,0);
			pthread_mutex_unlock(&trx_table_latch);
		}else{
			int conflict = 0;
			int deadLock = 0;
			lock_t* clock = node->head;
			lock_t* lastLock = node->tail;
			while(clock){
				if(clock->owner_trx_id==trx_id){
					if (clock->lock_mode==LM_SHARED && lock_mode==LM_EXCLUSIVE){
						if(clock==node->head && clock==node->tail){
							clock->lock_mode = LM_EXCLUSIVE;
							lock = clock;
							pthread_mutex_unlock(&trx_table_latch);
							pthread_mutex_unlock(&lock_table_latch);
							return ACQUIRED;
						}else{
							deadLock = 1;
							break;
						}
					}else{
						pthread_mutex_unlock(&trx_table_latch);
						pthread_mutex_unlock(&lock_table_latch);
						lock = clock;
						return ACQUIRED;
					}
				}else if(clock == node->tail){
					if(clock->lock_mode==LM_SHARED && lock_mode==LM_SHARED){
						if(clock->state==LS_ACQIRED){
							conflict=0;
						}else{
							// printf("something is wrong\n");
							conflict=1;
						}
					}else{
						// printf("something is wrong2: %d-%d\n",clock->lock_mode,lock->lock_mode);
						conflict=1;
					}
				}
				clock = clock->next;
			}	
			
			if (deadLock){
				if (debug1) printf("@deadLock is detected in trx %d!!!\n",trx_id);
				lock=NULL;
				pthread_mutex_unlock(&trx_table_latch);
				pthread_mutex_unlock(&lock_table_latch);
				return DEADLOCK;
			}else if (conflict){
				// printf("@\n");
				// deadLock detection
				// Step1
				if (lock_detection(node->tail,trx_id)>0){
					deadLock = 1;
				}
				lock_visited_initialize();
				if(deadLock){
					if (debug1) printf("@deadLock is detected in trx %d!!!\n",trx_id);
					lock=NULL;
					pthread_mutex_unlock(&trx_table_latch);
					pthread_mutex_unlock(&lock_table_latch);
					return DEADLOCK;
				}else{
					lock->state = LS_WAITING;
					lastLock = node->tail; 
					lastLock->next = lock;
					lock->prev = lastLock;
					node->tail = lock;

					trx_insert_lock(trx_id,lock,1);
					pthread_mutex_unlock(&trx_table_latch);
					// ?????? ????????? 
					// pthread_mutex_unlock(&lock_table_latch);
					// if (debug1) printf("lock_acquire: %d wait (lock: %d trx: %d)\n",trx_id,lock_check_lock(),trx_check_lock());
					// return NEED_TO_WAIT;
					pthread_cond_init(&lock->cond,NULL);
					pthread_cond_wait(&lock->cond,&lock_table_latch);
					
					lock->state = LS_ACQIRED;
					node->acqiredCount++;
				}
			}else{
				lock->state = LS_ACQIRED;
				lastLock = node->tail;
				lastLock->next = lock;
				lock->prev = lastLock;
				node->tail = lock;
				node->acqiredCount++;
				
				trx_insert_lock(trx_id,lock,0);
				pthread_mutex_unlock(&trx_table_latch);
			}
		}
	}	
	
	pthread_mutex_unlock(&lock_table_latch);
	if (debug1) printf("lock_acquire: %d end (lock: %d trx: %d)\n",trx_id,lock_check_lock(),trx_check_lock());
	return ACQUIRED;
}

void lock_wait(lock_t* lock_obj){
	// pthread_mutex_lock(&trx_table_latch);
	// pthread_mutex_lock(&lock_table_latch);
	printf("!!!!\n");
	pthread_cond_init(&lock_obj->cond,NULL);
	Trx* trx = trx_find(lock_obj->owner_trx_id);
	pthread_cond_wait(&lock_obj->cond,&trx->trx_latch);
	pthread_mutex_unlock(&trx->trx_latch);
	printf("wakeUp!!!\n");
	lock_obj->state = LS_ACQIRED;
	lock_obj->sentinal->acqiredCount++;

	// pthread_mutex_unlock(&trx_table_latch);
	// pthread_mutex_unlock(&lock_table_latch);
	if (debug1) printf("lock_acquire: %d end (lock: %d trx: %d)\n",lock_obj->owner_trx_id,lock_check_lock(),trx_check_lock());

}



int
lock_release(lock_t* lock_obj)
{	
	if(debug1 && lock_obj->lock_mode==LM_SHARED) printf("lock_release: S%lld (lock: %d trx: %d)\n",lock_obj->sentinal->recordID,lock_check_lock(),trx_check_lock());
	if(debug1 && lock_obj->lock_mode==LM_EXCLUSIVE) printf("lock_release: X%lld\n",lock_obj->sentinal->recordID);
	pthread_mutex_lock(&lock_table_latch);
	Node* node  = lock_obj->sentinal;
	lock_t* headLock = (lock_t*)node->head;
	lock_t* lastLock = (lock_t*)node->tail;

	if (lock_obj==headLock && lock_obj==lastLock){
		node->head = NULL;
		node->tail = NULL;
		node->acqiredCount--;
	}else if (lock_obj==headLock){
		// S or X
		node->head = lock_obj->next;
		lock_obj->next->prev = NULL;
		node->acqiredCount--;
		
		if(node->acqiredCount==0){
			if (lock_obj->next->lock_mode==LM_SHARED){
				lock_t* slock = lock_obj->next;
				while(slock && slock->lock_mode==LM_SHARED){
					// Trx* trx = trx_find(lock_obj->owner_trx_id);
					// pthread_mutex_lock(&trx->trx_latch);
					pthread_cond_signal(&(slock->cond));
					// pthread_mutex_unlock(&trx->trx_latch);

					slock = slock->next;
				}
			}else{
				// Trx* trx = trx_find(lock_obj->next->owner_trx_id);
				// pthread_mutex_lock(&trx->trx_latch);
				pthread_cond_signal(&(lock_obj->next->cond));
				// pthread_mutex_unlock(&trx->trx_latch);
			}
		}
		
	}else if (lock_obj==lastLock){
		// S
		node->tail = lock_obj->prev;
		lock_obj->prev->next = NULL;
		node->acqiredCount--;

	}else{
		// S
		lock_obj->prev->next = lock_obj->next;
		lock_obj->next->prev = lock_obj->prev;
		node->acqiredCount--;

	}
	free(lock_obj);
	
	pthread_mutex_unlock(&lock_table_latch);
	
	if (debug1) printf("lock_release end (lock: %d trx: %d)\n",lock_check_lock(),trx_check_lock());
	return 0;
}

int
lock_release_abort(lock_t* lock_obj)
{	
	// if(lock_obj->lock_mode==LM_SHARED) printf("lock_release_abort: S%lld (lock: %d trx: %d)\n",lock_obj->sentinal->recordID,lock_check_lock(),trx_check_lock());
	// if(lock_obj->lock_mode==LM_EXCLUSIVE) printf("lock_release_abort: X%lld\n",lock_obj->sentinal->recordID);
	// pthread_mutex_lock(&lock_table_latch);
	Node* node  = lock_obj->sentinal;
	lock_t* headLock = (lock_t*)node->head;
	lock_t* lastLock = (lock_t*)node->tail;

	if (lock_obj==headLock && lock_obj==lastLock){
		// S or X
		node->head = NULL;
		node->tail = NULL;
		node->acqiredCount--;
	}else if (lock_obj==headLock){
		// S or X
		node->head = lock_obj->next;
		lock_obj->next->prev = NULL;
		node->acqiredCount--;
		
		if(node->acqiredCount==0){
			if (lock_obj->next->lock_mode==LM_SHARED){
				lock_t* slock = lock_obj->next;
				while(slock && slock->lock_mode==LM_SHARED){
					pthread_cond_signal(&(slock->cond));
					slock = slock->next;
				}
			}else{
				pthread_cond_signal(&(lock_obj->next->cond));
			}
		}
	}else if (lock_obj==lastLock){
		// S
		node->tail = lock_obj->prev;
		lock_obj->prev->next = NULL;
		node->acqiredCount--;

	}else{
		// S
		lock_obj->prev->next = lock_obj->next;
		lock_obj->next->prev = lock_obj->prev;
		node->acqiredCount--;

	}

	free(lock_obj);
	// pthread_mutex_unlock(&lock_table_latch);
	// printf("lock_release_abort end (lock: %d trx: %d)\n",lock_check_lock(),trx_check_lock());
	return 0;
}

int lock_detection(lock_t* clock, int trxID){
	if(clock->visited) return 0;
	int detection = 0;
	
	// Step 1
	while(clock){
		// Step 2
		clock->visited = 1;
		if(clock->owner_trx_id == trxID){
			detection = 1;
			return detection;
		}

		Trx* trx = trx_find(clock->owner_trx_id);
		lock_t* tlock = trx->head;
		while(tlock){
			if (tlock->state == LS_WAITING){
				lock_t* slock = tlock->prev;
				if (tlock->lock_mode == LM_SHARED){
					while (slock && slock->lock_mode == LM_SHARED){
						// slock->visited = 1;
						slock = slock->prev;
					}
				}
				if (slock) detection+=lock_detection(slock,trxID);
				// Option for efficiency
				if(detection) return detection;
			}

			tlock = tlock->trx_next;
		}

		clock = clock->prev;
	}
	return detection;
}

void lock_visited_initialize(void){
	for (int i=0; i<MAX_HASH;i++){
		Node* cnode = lockTable[i];
		while(cnode){
			lock_t* clock = cnode->head;
			while(clock){
				clock->visited = 0;
				clock = clock->next;
			}
			cnode = cnode->hnext;
		}
	}
}

void 
lock_engrave(lock_t* lock_obj, pagenum_t page_num, char* value){
	pthread_mutex_lock(&lock_table_latch);
	lock_obj->page_num = page_num;
	memcpy(lock_obj->history,value,VALUE_SIZE);
	pthread_mutex_unlock(&lock_table_latch);
}


int
terminate_lock_table(){
	for (int i=0;i<MAX_HASH;i++){
		if (lockTable[i]!=NULL)
			free(lockTable[i]);
	}
 	return 0;
}

/* APIs for hash */
int hashKey(int tableID, int64_t recordID){
	return (tableID+recordID)%MAX_HASH;
}

Node* setHash(int tableID, int64_t recordID)
{	
	Node* node = (Node*)malloc(sizeof(Node));
    node->tableID = tableID;
	node->recordID = recordID;
	node->tail = NULL;
	node->head = NULL;
	node->acqiredCount = 0;
    node->hnext = NULL;

    int hash_key = hashKey(tableID,recordID);
    if (lockTable[hash_key] == NULL)
    {
        lockTable[hash_key] = node;
    }
    else
    {
        node->hnext = lockTable[hash_key];
        lockTable[hash_key] = node;
    }

	return node;
}

Node* getHash(int tableID, int64_t recordID)
{
    int hash_key = hashKey(tableID,recordID);
    if (lockTable[hash_key] == NULL)
        return NULL;
 
    if (lockTable[hash_key]->tableID == tableID &&
		lockTable[hash_key]->recordID == recordID)
    {
        return lockTable[hash_key];
    }
    else
    {
        Node* node = lockTable[hash_key];
        while (node->hnext)
        {
            if (node->hnext->tableID == tableID &&
				node->hnext->recordID == recordID)
            {
                return node->hnext;
            }
            node = node->hnext;
        }
    }
    return  NULL;
}


void deleteHash(int tableID, int64_t recordID)
{
    int hash_key = hashKey(tableID,recordID);
    if (lockTable[hash_key] == NULL)
        return;
 
    Node* delNode = NULL;
    if (lockTable[hash_key]->tableID == tableID &&
		lockTable[hash_key]->recordID == recordID)
    {
        delNode = lockTable[hash_key];
        lockTable[hash_key] = lockTable[hash_key]->hnext;
    }
    else
    {
        Node* node = lockTable[hash_key];
        Node* next = node->hnext;
        while (next)
        {
			if (next->tableID == tableID &&
				next->recordID == recordID)
            {
                node->hnext = next->hnext;
                delNode = next;
                break;
            }
            node = next;
            next = node->hnext; 
        }
    }
    free(delNode); 
}





