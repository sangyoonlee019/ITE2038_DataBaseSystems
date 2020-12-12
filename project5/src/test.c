#include "db.h"
#include "bpt.h"
#include "buf.h"
#include "lock.h"
#include "file.h"

#define OPNUM 4
#define TRHEAD_NUM 3
#define MAXNUM 10

int commit = 0;
int abort_find = 0;
int abort_update = 0;

void* trx_func(void* argv){
    int trx_id = trx_begin();

    int target;
    char value[120];
    
    for(int i = 0; i < OPNUM; i++){
        target = rand() % (MAXNUM) + 1;
        //if (rand() % 50 == 0){
            //target = -1;
        //}

        if (rand() % 2 == 0){
            printf("trx %d try find key: %d\n",trx_id,target);
            if (db_find(1, target, value, trx_id) == -1){
                printf("trx %d's find is aborted at key: %d ==================================================\n\n",trx_id,target);
                abort_find++;
                pthread_exit(NULL);
            }
            printf("trx %d find success key: %d \n\n",trx_id,target);
        }
        // else{
        //     char update_value[120];
        //     sprintf(update_value, "trx%d", trx_id);
        //     printf("trx %d try update key: %d\n",trx_id,target);
        //     if (db_update(1, target, update_value, trx_id) == -1){
        //         printf("trx %d's update is aborted at key: %d ==================================================\n\n",trx_id,target);
        //         abort_update++;
        //         pthread_exit(NULL);
        //     }
        //     printf("trx %d update success key: %d \n\n",trx_id,target);
        // }
    }

    printf("trx %d commit : %d\n\n",trx_id,trx_commit(trx_id));
    commit++;
}



int main(int argc, char* argv[]){
    srand(time(NULL));
    int table_id = 1;

    init_db(10);

    open_table("sample_10000.db");
    char value[120];

    /*
    for(int i = 0; i < MAXNUM; i++){
        sprintf(value, "key : %d", i);
        db_insert(table_id, i, value);
        cout << value << endl;
    }
    print_tree(table_id);
    */

    pthread_t thread_id[TRHEAD_NUM];

    for(int i = 0; i < TRHEAD_NUM; i++){
        pthread_create(&thread_id[i], 0, trx_func, NULL);
    }
    
    for(int i = 0; i < TRHEAD_NUM; i++){
        pthread_join(thread_id[i], NULL);
    }

    printf("\ncommit num   : %d\n",commit);
    printf("abort_find   : %d\n",abort_find);
    printf("abort_update : %d\n",abort_update);

 
    return 0;
}