#ifndef __BUF_H__
#define __BUF_H__

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "file.h"

#define BUFFER_SIZE PAGE_SIZE + 3
#define DEFAULT_BUFFER_NUMBER 10

#define DEFAULT_TABLE_ID 1
#define MAX_TABLE_NUM 10

#define MAX_PATHNAME_LENGHT 20

#define UNUSED -1
#define LRU 0

#define HEAD(x) ((x).next)
#define TAIL(x) ((x).prev)

// struct LRUList_{
//     struct Buffer* next;
//     struct Buffer* prev;
// }typedef LRUList_;

struct BufferControlBlock {
    int tableID;
    pagenum_t pageNumber;
    int32_t isDirty;
    pthread_mutex_t page_latch;
    int32_t isUsed;
    struct BufferControlBlock* next;
    struct BufferControlBlock* prev;
}typedef BufferControlBlock;

struct Buffer{
    page_t frame;
    BufferControlBlock controlBlock;
}typedef Buffer;

extern Buffer* bufferArray;
// extern LRUList_ LRUList;
extern BufferControlBlock LRUList; 
extern int numberOfBuffer;
extern int bufferUse;
extern int currentTableID;
extern int tableList[MAX_TABLE_NUM+1];
extern char* history[MAX_TABLE_NUM+1];
extern int debug;

// Initialize buffer
int buf_initialize(int numberOfBuffer);
// Terminate buffer
int buf_terminate(void);
// find past tableID
int buf_find_tableID(char* pathname);
// Open datafile
int buf_open_table(char* pathname, int mode);
// Close datafile
int buf_close_table(int tableID);
// Check table is open
int buf_table_is_open(int tableID);
// Allocate an page from the free page list
pagenum_t buf_alloc_page(tableid tableID);
// Free page
void buf_free_page(tableid tableID, pagenum_t pageNum);
// Allocate an buffer
int buf_alloc_buffer(void);
// Find victim according to LRU policy
int buf_eviction(void);

// Read an on-disk page into the in-memory page structure(dest)
void buf_get_page(tableid tableID, pagenum_t pagenum, page_t* dest);
// Write an in-memory page(src) to the on-disk page
void buf_set_page(tableid tableID, pagenum_t pagenum, const page_t* src);

void buf_unpin_page(tableid tableID, pagenum_t pagenum);

int buf_find_page(tableid tableID, pagenum_t pagenum);

int buf_check_lock(void);

void printBufferArray(void);
void printLRUList(void);

#endif /* __BUF_H__*/