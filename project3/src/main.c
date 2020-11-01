#include "db.h"
#include "bpt.h"
#include "buf.h"
#include "file.h"


#define OPEN "open "
#define CLOSE "close "
#define INSERT "insert "
#define DELETE "delete "
#define FIND "find "

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

    
    init_db(20);
    while(getInstruction(instruction, sizeof(instruction)) >= 0){
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
            if ((retval=db_find(tid,key,foundValue))<0){
                printf("error %d: find failed!\n",retval);
            }else{
                printf("found: %s\n",foundValue);
            }
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
    }
    shutdown_db();
    return EXIT_SUCCESS;
}
