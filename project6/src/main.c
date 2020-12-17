#include "db.h"
#include "bpt.h"
#include "buf.h"
#include "lock.h"
#include "file.h"
#include "log.h"


#define OPEN "open "
#define CLOSE "close "
#define INSERT "insert "
#define DELETE "delete "
#define FIND "find "
#define UPDATE "update "

int getInstruction(char *buf, int nbuf) {
  printf("> ");
  memset(buf, 0, nbuf);
  if(gets(buf) == NULL) // EOF
    return -1;
  return 0;
}

// MAIN
int main( int argc, char ** argv ) {

    int input, range2;
    char instruction[6+2+20+120];
    char license_part;

    printf("@2\n");
    init_db(5,0,0,"log","logmsg");
    printf("@3\n");
    int tid1 = open_table("DATA14");

    // printf("%lu\n",sizeof(Log));
    // Log beginLog;
    // beginLog.logSize = LOG_ULR_SIZE;
    // beginLog.type = LT_UPDATE;
    // log_write_log(&beginLog);
    
    // beginLog.logSize = LOG_BCR_SIZE;
    // beginLog.type = LT_BEGIN;
    // log_write_log(&beginLog);
    
    // beginLog.logSize = LOG_CLR_SIZE;
    // beginLog.type = LT_COMPENSATE;
    // log_write_log(&beginLog);
    
    // Log newLog;
    // int readOffset = 0;
    // readOffset = log_read_log(readOffset,&newLog);
    // print_log(&newLog);
    // readOffset = log_read_log(readOffset,&newLog);
    // print_log(&newLog);
    // readOffset = log_read_log(readOffset,&newLog);
    // print_log(&newLog);

    // log_flush();
    
    while(getInstruction(instruction, sizeof(instruction)) >= 0){
        int trx_id = trx_begin();
        // int trx_id =1;
        if(strncmp(OPEN,instruction,5)==0){
            char* path;
            path = instruction + 5;
            int tid = open_table(path);
            if (tid<=0){
                printf("error: open table failed!\n");
            }
            printf("open table! : %d\n",tid);
        }else if(strncmp(CLOSE,instruction,6)==0){
            int tableID;
            tableID = atoi(instruction + 6);
            if (close_table(tableID)<0){
                printf("error: close table%d failed!\n",tableID);
            }else{
                printf("close: table%d\n",tableID);
            }
        }else if(strncmp(INSERT,instruction,6)==0){
            int tid, key;
            char* remainder,value;
            remainder = instruction + 6;
            tid = atoi(strtok(remainder," "));
            key = atoi(strtok(NULL," "));
            if ((remainder = strtok(NULL," "))==NULL || db_insert(tid,key,remainder)<0){
                printf("error: insert failed!\n"); 
            }
            printTree(tid);
            printBufferArray();
        }else if(strncmp(FIND,instruction,5)==0){
            int tid, key;
            char* remainder;
            char foundValue[120];
            remainder = instruction + 5;
            tid = atoi(strtok(remainder," "));
            key = atoi(strtok(NULL," "));
            int retval;
            printf("trx %d try find key: %d\n",trx_id,key);
            if ((retval=db_find(tid,key,foundValue,trx_id))<0){
                printf("trx %d's find is aborted at key: %d ==================================================\n\n",trx_id,tid);
                printf("error %d: find failed!\n",retval);
            }else{
                printf("found: %s\n",foundValue);
                printf("trx %d find success key: %d \n\n",trx_id,tid);
            }
        }else if(strncmp(UPDATE,instruction,6)==0){
            int tid, key;
            char* remainder,value;
            remainder = instruction + 6;
            tid = atoi(strtok(remainder," "));
            key = atoi(strtok(NULL," "));
            printf("trx %d try update key: %d\n",trx_id,tid);
            if ((remainder = strtok(NULL," "))==NULL || db_update(tid,key,remainder,trx_id)<0){
                printf("trx %d's update is aborted at key: %d ==================================================\n\n",trx_id,tid);
                printf("error: insert failed!\n"); 
            }
            printf("trx %d update success key: %d \n\n",trx_id,tid);
            printTreeValue(tid);
            // printBufferArray();
        }else if(strncmp(DELETE,instruction,7)==0){
            int tid, key;
            char* remainder;
            char foundValue[120];
            remainder = instruction + 7;
            tid = atoi(strtok(remainder," "));
            key = atoi(strtok(NULL," "));
            if (db_delete(tid,key)<0){
                printf("error: delete failed!\n");
            }
            printTree(tid);
        }else{
            // usage_2();
        }
        printf("trx before commit\n");
        trx_commit(trx_id);
        printf("trx after commit\n");
        // check_lock();
    }
    shutdown_db();
    return EXIT_SUCCESS;
}
