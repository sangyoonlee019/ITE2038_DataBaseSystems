#include <lock_table.h>

struct lock_t {
	struct lock_t* prev;
	struct lock_t* next;
	Node* sentinal;
	pthread_cond_t cond;
};

typedef struct lock_t lock_t;
Node* hashTable[MAX_HASH];

pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;

int
init_lock_table()
{
	return 0;
}

lock_t*
lock_acquire(int table_id, int64_t key)
{	
	pthread_mutex_lock(&lock_table_latch);
	Node* node = getHash(table_id,key); 
	lock_t* lock = (lock_t*)malloc(sizeof(lock_t)); 

	if (node==NULL){
		node = setHash(table_id, key);

		lock->prev=NULL;
		lock->next=NULL;
		lock->sentinal=node;

		node->tail = lock;
		node->head = lock;
	}else{
		lock->prev=NULL;
		lock->next=NULL;
		lock->sentinal=node;
		
		if(node->head==NULL){
			node->tail = lock;
			node->head = lock;
		}else{
			pthread_cond_init(&lock->cond,NULL);
			
			lock_t* lastLock = (lock_t*)node->tail;
			lastLock->next = lock;
			lock->prev = lastLock;
			node->tail = lock;

			pthread_cond_wait(&lock->cond,&lock_table_latch);
		}
	}	

	pthread_mutex_unlock(&lock_table_latch);
	return lock;
}

int
lock_release(lock_t* lock_obj)
{	
	int r = pthread_mutex_lock(&lock_table_latch);
	printf("lock: %d",r);
	Node* node  = lock_obj->sentinal;
	lock_t* lastLock = (lock_t*)node->tail;
	if (lastLock==lock_obj){
		node->head = NULL;
		node->tail = NULL;
	}else{
		node->head = lock_obj->next;
		lock_obj->next->prev = NULL;
		pthread_cond_signal(&(lock_obj->next->cond));
	}
	free(lock_obj);
	r = pthread_mutex_unlock(&lock_table_latch);
	printf("unlock: %d",r);
	return 0;
}




int
terminate_lock_table(){
	for (int i=0;i<MAX_HASH;i++){
		if (hashTable[i]!=NULL)
			free(hashTable[i]);
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
    node->hnext = NULL;

    int hash_key = hashKey(tableID,recordID);
    if (hashTable[hash_key] == NULL)
    {
        hashTable[hash_key] = node;
    }
    else
    {
        node->hnext = hashTable[hash_key];
        hashTable[hash_key] = node;
    }

	return node;
}

Node* getHash(int tableID, int64_t recordID)
{
    int hash_key = hashKey(tableID,recordID);
    if (hashTable[hash_key] == NULL)
        return NULL;
 
    if (hashTable[hash_key]->tableID == tableID &&
		hashTable[hash_key]->recordID == recordID)
    {
        return hashTable[hash_key];
    }
    else
    {
        Node* node = hashTable[hash_key];
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
    if (hashTable[hash_key] == NULL)
        return;
 
    Node* delNode = NULL;
    if (hashTable[hash_key]->tableID == tableID &&
		hashTable[hash_key]->recordID == recordID)
    {
        delNode = hashTable[hash_key];
        hashTable[hash_key] = hashTable[hash_key]->hnext;
    }
    else
    {
        Node* node = hashTable[hash_key];
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





