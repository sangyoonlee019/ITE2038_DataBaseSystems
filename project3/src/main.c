#include "db.h"
#include "bpt.h"
#include "buf.h"
#include "file.h"


#define OPEN "open "
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

    printf("@LRUList: %lu BCB: %lu Buffer: %lu\n",sizeof(LRUList),sizeof(BufferControlBlock),sizeof(Buffer));

    // open_table("sample_10000.db");
    while(getInstruction(instruction, sizeof(instruction)) >= 0){
        if(strncmp(OPEN,instruction,5)==0){
            char* path;
            path = instruction + 5;
            if (open_table(path)<=0){
                printf("error: open table failed!\n");
            }
        }else if(strncmp(INSERT,instruction,6)==0){
            int key;
            char* remainder,value;
            remainder = instruction + 6;
            key = atoi(strtok(remainder," "));
            if ((remainder = strtok(NULL," "))==NULL || db_insert(key,remainder)<0){
                printf("error: insert failed!\n"); 
            }
            printTree();
        }else if(strncmp(FIND,instruction,5)==0){
            int key;
            char foundValue[120];
            key = atoi(instruction + 5);
            int retval;
            if ((retval=db_find(key,foundValue))<0){
                printf("error %d: find failed!\n",retval);
            }else{
                printf("found: %s\n",foundValue);
            }
        }else if(strncmp(DELETE,instruction,7)==0){
            int key;
            key = atoi(instruction + 7);
            printf("delete inst!\n");
            if (db_delete(key)<0){
                printf("error: delete failed!\n");
            }
            printTree();
        }else{
            // usage_2();
        }
    }
    close_table();
    return EXIT_SUCCESS;
}
