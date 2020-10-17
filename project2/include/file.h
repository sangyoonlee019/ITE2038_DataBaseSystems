#ifndef __FILE_H__
#define __FILE_H__

#ifndef O_DIRECT
#define O_DIRECT	00040000	/* direct disk access hint */
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define OPEN_EXIST 0
#define OPEN_NEW 1

#define false 0
#define true 1

#define DEFAULT_LEAF_ORDER 32
#define DEFAULT_INTERNAL_ORDER 249

#define VALUE_SIZE 120
#define PAGE_SIZE 4096
#define PAGE_HEADER_SIZE 128

#define HEADER_PAGE_NUMBER 0

typedef uint64_t pagenum_t; 
struct page_t {
    char pageSize[PAGE_SIZE];
}typedef page_t;

struct Record{
    int64_t key;
    char value[120];
}typedef Record;

struct RecordID{
    int64_t key;
    pagenum_t pageNumber;
}typedef RecordID;

#pragma pack(push,8)
struct HeaderPage{
    pagenum_t freePageNumber;
    pagenum_t rootPageNumber;
    pagenum_t numberOfPage;
    char reserved[PAGE_SIZE - 24];
}typedef HeaderPage;
#pragma pack(pop)

#pragma pack(push,8)
struct FreePage{
    pagenum_t nextFreePageNumber;
    char notUsed[PAGE_SIZE - 8];
}typedef FreePage;
#pragma pack(pop)

#pragma pack(push,4)
struct LeafPage{
    pagenum_t parentPageNumber;
    int32_t isLeaf;
    int32_t numberOfKeys;
    char reserved[PAGE_HEADER_SIZE - 24];
    pagenum_t rightSiblingPageNumber;
    Record records[DEFAULT_LEAF_ORDER-1];
}typedef LeafPage;
#pragma pack(pop)

#pragma pack(push,4)
struct InternalPage{
    pagenum_t parentPageNumber;
    int32_t isLeaf;
    int32_t numberOfKeys;
    char reserved[PAGE_HEADER_SIZE - 24];
    pagenum_t oneMorePageNumber;
    RecordID recordIDs[DEFAULT_INTERNAL_ORDER-1];
}typedef InternalPage;
#pragma pack(pop)

#pragma pack(push,4)
struct NodePage{
    pagenum_t parentPageNumber;
    int32_t isLeaf;
    int32_t numberOfKeys;
    char reserved[PAGE_HEADER_SIZE - 24];
    pagenum_t specialPageNumber;
    char notUsed[PAGE_SIZE-PAGE_HEADER_SIZE];
}typedef NodePage;
#pragma pack(pop)


// Get ith page number in recordID arr in InternalPage.
pagenum_t pageNumOf(InternalPage* page, int i);
// Set ith page number in recordID arr in InternalPage.
void setPageNum(InternalPage* page, int i, pagenum_t pageNum);


void file();
// Open datafile
int file_open_table(char* pathname, int mode);
// Check file is open
int file_table_is_open(void);
// Close datafile
int file_close_table();
// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();
// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src);

#endif /* __FILE_H__*/