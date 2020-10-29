
#include "file.h"
#include "buf.h"

Buffer* bufferArray;
LRUList_ LRUList;
int numberOfBuffer;
int bufferUse;

// Initialize buffer
int buf_initialize(int num_buf){
    bufferArray = (Buffer*)malloc((sizeof(BufferControlBlock)+PAGE_SIZE)*num_buf);
    if (bufferArray == NULL) {
        printf("buffr initailize fail!\n");
        return -1;
    }
    for (int i=0;i<numberOfBuffer;i++){
        bufferArray[i].controlBlock.isUsed = false;
        bufferArray[i].controlBlock.LRU.next = NULL;
        bufferArray[i].controlBlock.LRU.prev = NULL;
    }

    HEAD(LRUList) = NULL;
    TAIL(LRUList) = NULL;
    numberOfBuffer = num_buf;
    bufferUse = 0;
    return 0;
}
// Terminate buffer
int buf_terminate(void){
    free(bufferArray);
    return 0;
}

int buf_open_table(char* pathname){
    
}

pagenum_t buf_alloc_header_page(tableid tableID);

// bpt.c가 쓸 함수
pagenum_t buf_alloc_page(tableid tableID){
    pagenum_t freePageNum;
    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    // 아예 새 페이지 만들어야하는경우
    if (headerPage.freePageNumber==0){
        freePageNum = headerPage.numberOfPage;
        
        int newBufferIdx = buf_alloc_buffer();
        bufferArray[newBufferIdx].controlBlock.tableID = tableID;
        bufferArray[newBufferIdx].controlBlock.isDirty=true;
        bufferArray[newBufferIdx].controlBlock.isUsed=true;        
        //LRU관련 필드 채우는거 추가해야행
        bufferArray[newBufferIdx].controlBlock.isPinned=true;

        headerPage.numberOfPage++;
        buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
        return freePageNum;
    }

    FreePage freePage;
    freePageNum = headerPage.freePageNumber;
    buf_get_page(tableID, freePageNum, (page_t*)&freePage);
    headerPage.freePageNumber = freePage.nextFreePageNumber;
    buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    return freePageNum;
}

int buf_eviction(){
    return 1;
}


int buf_alloc_buffer(){
    int freeBufferIndex;
    if (bufferUse < numberOfBuffer){
        // freebuffer찾는 다른 알고리즘 만들어야하나?
        for (int i=0;i<numberOfBuffer;i++){
            if (bufferArray[i].controlBlock.isUsed==false){
                freeBufferIndex = i;
                break;
            }
        }    
    }else{
        freeBufferIndex = buf_eviction();
    }

    bufferUse++;
    return freeBufferIndex;
}

// void buf_find_page();

// Get needed page from bufferArray
// LRU는 모르겠다ㅋㅋㅋ
void buf_get_page(tableid tableID, pagenum_t pageNum, page_t* dest){
    // buf_find만들어야하나??
    // 1. buffer에 해당 페이지 있는경우
    for (int i=0;i<numberOfBuffer;i++){
        if (bufferArray[i].controlBlock.tableID==tableID && pageNum==bufferArray[i].controlBlock.pageNumber){
            bufferArray[i].controlBlock.isPinned = true;
            memcpy(dest, bufferArray[i].frame, PAGE_SIZE);
            return;      
        }
    } 
    
    // 2. 버퍼에 해당페이지 없는 경우 - 디스크에서 가져와 할당
    int newBufferIdx;
    newBufferIdx = buf_alloc_buffer();
    file_read_page(tableID, pageNum, dest);
    memcpy(bufferArray[newBufferIdx].frame, dest, PAGE_SIZE);
    bufferArray[newBufferIdx].controlBlock.tableID = tableID;
    bufferArray[newBufferIdx].controlBlock.pageNumber = pageNum;
    bufferArray[newBufferIdx].controlBlock.isDirty=false;
    bufferArray[newBufferIdx].controlBlock.isUsed=true;
    
    //LRU관련 필드 채우는거 추가해야행
    bufferArray[newBufferIdx].controlBlock.isPinned=true;
}

// Set page to bufferArray 
// isDirty 필드 해주어ㅑ함
// isPinned false로 바꿔줘야함
void buf_set_page(tableid tableID, pagenum_t pageNum, const page_t* src){
    //buf_free_page 만들어야하나?
    //buf_find도 만들어야 하나 이거 할떄마다 오버헤드먹네시밤ㅠㅠ
    int setBufferIdx;
    for (int i=0;i<numberOfBuffer;i++){
        if (bufferArray[i].controlBlock.tableID==tableID && pageNum==bufferArray[i].controlBlock.pageNumber){
            setBufferIdx = i;
            break;      
        }
    } 

    memcpy(bufferArray[setBufferIdx].frame, src, PAGE_SIZE);
    bufferArray[setBufferIdx].controlBlock.isDirty = true;
    bufferArray[setBufferIdx].controlBlock.isUsed = false;
    bufferArray[setBufferIdx].controlBlock.isPinned = false;
}

