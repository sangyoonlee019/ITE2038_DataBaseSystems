
#include "file.h"

HeaderPage headerPage;
int dataFile = -1;


// Get ith page number in recordID arr in InternalPage.
pagenum_t pageNumOf(InternalPage* page, int i){
    if (i==0){
        return page->oneMorePageNumber;
    }
    return page->recordIDs[i-1].pageNumber;
}

void setPageNum(InternalPage* page, int i, pagenum_t pageNum){
    if (i==0){
        page->oneMorePageNumber = pageNum;
    }else{
        page->recordIDs[i-1].pageNumber = pageNum;
    }
}

void file(){
    
}
// Open datafile
int file_open_table(char* pathname, int mode){
    int dataFile;
    switch (mode){
    case OPEN_EXIST:
        dataFile = open(pathname, O_RDWR|O_DIRECT|O_SYNC, 0644);
        if (dataFile==-1) return -1;
        break;
    case OPEN_NEW:
        dataFile = open(pathname, O_RDWR|O_CREAT|O_DIRECT|O_SYNC, 0644);
        if (dataFile==-1) return -1;
        break;
    default:
        printf("error: file_open_table mode is wrong\n");
        break;
    }
    return dataFile;
}

// int file_table_is_open(){
//     if (dataFile==-1){
//         return false;
//     }
//     return true;
// }

// Close datafile
int file_close_table(int dataFile){
    if (close(dataFile)==-1){
        printf("error: closing datafile\n");
        return -1;
    }
    return 0;
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int dataFile, pagenum_t pagenum, page_t* dest){ 
    lseek(dataFile, PAGE_SIZE*pagenum, SEEK_SET);
    read(dataFile, dest, PAGE_SIZE);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int dataFile, pagenum_t pagenum, const page_t* src){ 
    lseek(dataFile, PAGE_SIZE*pagenum, SEEK_SET);
    write(dataFile, src, PAGE_SIZE);
}