#include "db.h"

#define OPEN "open "
#define INSERT "insert "
#define DELETE "delete "
#define FIND "find "

int getInstruction(char *buf, int nbuf) {
  printf("> ");
  memset(buf, 0, nbuf);
  ;
  if(gets(buf) < 0) // EOF
    return -1;
  return 0;
}

// MAIN
int main( int argc, char ** argv ) {

    char * input_file;
    FILE * fp;
    node * root;
    int input, range2;
    char instruction[6+2+20+120];
    char license_part;

    root = NULL;
    verbose_output = false;

    // if (argc > 1) {
    //     order = atoi(argv[1]);
    //     if (order < MIN_ORDER || order > MAX_ORDER) {
    //         fprintf(stderr, "Invalid order: %d .\n\n", order);
    //         usage_3();
    //         exit(EXIT_FAILURE);
    //     }
    // }

    license_notice();
    usage_1();  
    usage_2();

    // if (argc > 2) {
    //     input_file = argv[2];
    //     fp = fopen(input_file, "r");
    //     if (fp == NULL) {
    //         perror("Failure  open input file.");
    //         exit(EXIT_FAILURE);
    //     }
    //     while (!feof(fp)) {
    //         fscanf(fp, "%d\n", &input);
    //         root = insert(root, input, input);
    //     }
    //     fclose(fp);
    //     print_tree(root);
    // }

    // printf("> ");
    // while (scanf("%c", &instruction) >= 0) {
    //     switch (instruction) {
    //     case 'd':
    //         scanf("%d", &input);
    //         root = delete(root, input);
    //         print_tree(root);
    //         break;
    //     case 'i':
    //         scanf("%d", &input);
    //         root = insert(root, input, input);
    //         print_tree(root);
    //         break;
    //     case 'f':
    //     case 'p':
    //         scanf("%d", &input);
    //         find_and_print(root, input, instruction == 'p');
    //         break;
    //     case 'r':
    //         scanf("%d %d", &input, &range2);
    //         if (input > range2) {
    //             int tmp = range2;
    //             range2 = input;
    //             input = tmp;
    //         }
    //         find_and_print_range(root, input, range2, instruction == 'p');
    //         break;
    //     case 'l':
    //         print_leaves(root);
    //         break;
    //     case 'q':
    //         while (getchar() != (int)'\n');
    //         return EXIT_SUCCESS;
    //         break;
    //     case 't':
    //         print_tree(root);
    //         break;
    //     case 'v':
    //         verbose_output = !verbose_output;
    //         break;
    //     case 'x':
    //         if (root)
    //             root = destroy_tree(root);
    //         print_tree(root);
    //         break;
    //     default:
    //         usage_2();
    //         break;
    //     }
    //     while (getchar() != (int)'\n');
    //     printf("> ");
    // }
    printf("\n");

    order = DEFAULT_LEAF_ORDER;
    while(getInstruction(instruction, sizeof(instruction)) >= 0){
        if(strncmp(OPEN,instruction,5)==0){
            char* path;
            printf("open inst!\n");
            path = instruction + 5;
            if (open_table(path)<0){
                printf("error: open table failed!\n");
            }
        }else if(strncmp(INSERT,instruction,6)==0){
            int key;
            char* remainder,value;
            remainder = instruction + 6;
            printf("insert inst!\n");
            key = atoi(strtok(remainder," "));
            if ((remainder = strtok(NULL," "))==NULL || db_insert(key,remainder)<0){
                printf("error: insert failed!\n"); 
            }
        }else if(strncmp(FIND,instruction,5)==0){
            int key;
            char foundValue[120];
            key = atoi(instruction + 5);
            printf("find inst!\n");
            if (db_find(key,foundValue)<0){
                printf("error: find failed!\n");
            }
            printf("found: %s\n",foundValue);
        }else if(strncmp(DELETE,instruction,7)==0){
            int key;
            key = atoi(instruction + 7);
            printf("delete inst!\n");
            if (db_delete(key)<0){
                printf("error: delete failed!\n");
            }
        }else{
            usage_2();
        }
    }

    return EXIT_SUCCESS;
}
