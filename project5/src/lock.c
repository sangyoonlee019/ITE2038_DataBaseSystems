#include "lock.h"
#include "buf.h"
#include "file.h"
#include "db.h"

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
};

typedef struct lock_t lock_t;
Node* lockTable[MAX_HASH];
Trx* trxTable[MAX_HASH];


int currentTrxID = DEFAULT_TRX_ID;
int debug1 = 0;
pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trx_table_latch = PTHREAD_MUTEX_INITIALIZER;

/* APIs for transaction */
int 
lock_trx_begin(void){
	pthread_mutex_lock(&trx_table_latch);
	
	int newTrxID = currentTrxID;
	if (trx_new(newTrxID)==NULL){
		pthread_mutex_unlock(&trx_table_latch);
		printf("error: trx begin is failed!\n");
		return 0;
	}
	currentTrxID++;

	pthread_mutex_unlock(&trx_table_latch);
	return newTrxID;
}

int 
lock_trx_commit(int trxID){
	if(debug1) printf("trx_commit (lock: %d trx: %d)\n",lock_check_lock(),trx_check_lock());
	pthread_mutex_lock(&trx_table_latch);
	Trx* trx = trx_find(trxID);
	lock_t* lock = trx->head;
	if (lock ==NULL){
		pthread_mutex_unlock(&trx_table_latch);
		return -1;
	}

	lock_t* nextLock = lock->trx_next;
	
	lock_release(lock);
	while(nextLock!=NULL){
		lock = nextLock;
		nextLock = lock->trx_next;
		lock_release(lock);
	}
	
	trx_delete(trxID);
	pthread_mutex_unlock(&trx_table_latch);
	
	return trxID;
}

int trx_abort(int trxID){
	if (debug1) printf("trx%d abort called!\n",trxID);
	// pthread_mutex_lock(&trx_table_latch);
	Trx* trx = trx_find(trxID);
	
	if (trx==NULL){
		// pthread_mutex_unlock(&trx_table_latch);
		return -1;
	} 
	
	lock_t* lock = trx->head;
	if (lock == NULL){
		// pthread_mutex_unlock(&trx_table_latch);
		return trxID;
	} 
	
	
	lock_t* nextLock = lock->trx_next;
	if (lock->lock_mode==LM_EXCLUSIVE){
		LeafPage leafNode;
		buf_get_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
		
		for (int i=0;i<leafNode.numberOfKeys;i++){
			if (leafNode.records[i].key == lock->sentinal->recordID){
				memcpy(leafNode.records[i].value,history,VALUE_SIZE);
				buf_set_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
				break;
			}
		}
		
	}
	
	lock_release_abort(lock);

	while(nextLock!=NULL){
		lock = nextLock;
		nextLock = lock->trx_next;
		if (lock->lock_mode==LM_EXCLUSIVE){
			LeafPage leafNode;
			buf_get_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
			for (int i=0;i<leafNode.numberOfKeys;i++){
				if (leafNode.records[i].key == lock->sentinal->recordID){
					memcpy(leafNode.records[i].value,history,VALUE_SIZE);
					buf_set_page(lock->sentinal->tableID, lock->page_num, (page_t*)&leafNode);
					break;
				}
			}
		}
		lock_release_abort(lock);
	}
	
	trx_delete(trxID);
	// pthread_mutex_unlock(&trx_table_latch);
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

void trx_insert_lock(Trx* trx, lock_t* lock){
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

lock_t*
lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode)
{	
	if(debug1 && lock_mode==LM_SHARED) printf("lock_acqire: S%lld (lock: %d trx: %d)\n",key,lock_check_lock(),trx_check_lock());
	if(debug1 && lock_mode==LM_EXCLUSIVE) printf("lock_acqire: X%lld (lock: %d trx: %d)\n",key,lock_check_lock(),trx_check_lock());
	pthread_mutex_lock(&trx_table_latch);
	pthread_mutex_lock(&lock_table_latch);
	
	Node* node = getHash(table_id,key); 
	lock_t* lock = (lock_t*)malloc(sizeof(lock_t)); 

	if (node==NULL){
		node = setHash(table_id, key);

		lock->prev=NULL;
		lock->next=NULL;
		lock->sentinal=node;
		lock->lock_mode = lock_mode;
		lock->owner_trx_id = trx_id;
		lock->state = LS_ACQIRED;
		lock->trx_next = NULL;

		node->tail = lock;
		node->head = lock;
		node->acqiredCount++;

		Trx* trx = trx_find(trx_id);
		if (trx==NULL){
			printf("error: trx_find\n");
			pthread_mutex_unlock(&trx_table_latch);
			pthread_mutex_unlock(&lock_table_latch);
			return NULL;
		} 
		trx_insert_lock(trx,lock);
		pthread_mutex_unlock(&trx_table_latch);
	}else{
		lock->prev=NULL;
		lock->next=NULL;
		lock->sentinal=node;
		lock->lock_mode = lock_mode;
		lock->owner_trx_id = trx_id;
		lock->trx_next = NULL;

		if(node->head==NULL){
			lock->state = LS_ACQIRED;

			node->tail = lock;
			node->head = lock;
			node->acqiredCount++;
	
			Trx* trx = trx_find(trx_id);
			
			if (trx==NULL){
				printf("error: trx_find\n");
				pthread_mutex_unlock(&trx_table_latch);
				pthread_mutex_unlock(&lock_table_latch);
				return NULL;
			} 
			trx_insert_lock(trx,lock);
			pthread_mutex_unlock(&trx_table_latch);
		}else{
			int conflict = 0;
			lock_t* clock = node->head;
			lock_t* lastLock = node->tail;
			while(clock){
				if(clock->owner_trx_id==trx_id){
					if (clock->lock_mode==LM_SHARED && lock_mode==LM_EXCLUSIVE){
						if(clock==node->head && clock==node->tail){
							clock->lock_mode = LM_EXCLUSIVE;
							pthread_mutex_unlock(&trx_table_latch);
							pthread_mutex_unlock(&lock_table_latch);
							return clock;
						}else{
							conflict = 1;
							break;
						}
					}else{
						pthread_mutex_unlock(&trx_table_latch);
						pthread_mutex_unlock(&lock_table_latch);
						return clock;
					}
				}else if(clock == node->tail){
					if(clock->lock_mode==LM_SHARED && lock_mode==LM_SHARED){
						if(clock->state==LS_ACQIRED){
							conflict=0;
						}else{
							conflict=1;
						}
					}else{
						conflict=1;
					}
				}
				clock = clock->next;
			}	
			
			int deadLock = 0;
			if (conflict){
				// deadLock detection
				if(deadLock){
					trx_abort(trx_id);
					pthread_mutex_unlock(&trx_table_latch);
					pthread_mutex_unlock(&lock_table_latch);
					return NULL;
				}else{
					lock->state = LS_WAITING;
					lastLock = node->tail; 
					lastLock->next = lock;
					lock->prev = lastLock;
					node->tail = lock;

					Trx* trx = trx_find(trx_id);
					if (trx==NULL){
						printf("error: trx_find\n");;
						pthread_mutex_unlock(&trx_table_latch);
						pthread_mutex_unlock(&lock_table_latch);
						return NULL;
					} 
					trx_insert_lock(trx,lock);
					pthread_mutex_unlock(&trx_table_latch);
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
				
				Trx* trx = trx_find(trx_id);
				
				if (trx==NULL){
					printf("~~~~~~~~~~~~~~\n");
					pthread_mutex_unlock(&trx_table_latch);
					pthread_mutex_unlock(&lock_table_latch);
					return NULL;
				} 
				trx_insert_lock(trx,lock);
				pthread_mutex_unlock(&trx_table_latch);
			}
		}
	}	
	
	pthread_mutex_unlock(&lock_table_latch);
	if (debug1) printf("lock_acquire end (lock: %d trx: %d)\n",lock_check_lock(),trx_check_lock());
	return lock;
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





