#ifndef __BUF_H__
#define __BUF_H__

#include "file.h"

#define BUFFER_SIZE PAGE_SIZE + 3

#define DEFAULT_BUFFER_NUMBER 8
#define MAX_TABLE_ID 10

#define HEAD(x) ((x).next)
#define TAIL(x) ((x).prev)

struct LRUList_{
    struct LRUList_* next;
    struct LRUList_* prev;
}typedef LRUList_;

struct BufferControlBlock {
    int tableID;
    pagenum_t pageNumber;
    int32_t isDirty;
    int32_t isPinned;
    int32_t isUsed;
    LRUList_ LRU;
}typedef BufferControlBlock;

struct Buffer{
    page_t frame;
    BufferControlBlock controlBlock;
}typedef Buffer;

extern Buffer* bufferArray;
extern LRUList LRUHeader; 
extern int numberOfBuffer;
extern int bufferUse;

// Initialize buffer
int buf_initialize(int numberOfBuffer);
// Terminate buffer
int buf_terminate(void);

// Open datafile
int buf_open_table(char* pathname);
// Check file is open
int buf_table_is_open(void);
// Close datafile
int buf_close_table(void);

pagenum_t buf_alloc_header_page(tableid tableID);
// Allocate an on-disk page from the free page list
pagenum_t buf_alloc_page(tableid tableID);

int buf_alloc_buffer(void);

int buf_eviction(void);

// Free an on-disk page to the free page list
void buf_free_page(pagenum_t pagenum);
// Read an on-disk page into the in-memory page structure(dest)
void buf_get_page(tableid tableID, pagenum_t pagenum, page_t* dest);
// Write an in-memory page(src) to the on-disk page
void buf_set_page(tableid tableID, pagenum_t pagenum, const page_t* src);

#endif /* __BUF_H__*/