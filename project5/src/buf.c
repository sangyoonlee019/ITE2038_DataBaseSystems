
#include "file.h"
#include "buf.h"
#include "lock.h"

int currentTableID = DEFAULT_TABLE_ID;
int numberOfBuffer = DEFAULT_BUFFER_NUMBER;
int debug = 0;
int bufferUse;
int tableList[MAX_TABLE_NUM+1];
char* history[MAX_TABLE_NUM+1];

Buffer* bufferArray;
BufferControlBlock LRUList;

int buf_table_is_open(int tableID){
    if (tableList[tableID]==-1){
        return false;
    }
    return true;
}

// Initialize buffer
int buf_initialize(int num_buf){
    bufferArray = (Buffer*)malloc((sizeof(BufferControlBlock)+PAGE_SIZE)*num_buf);
    if (bufferArray == NULL) {
        return -1;
    }
    for (int i=0;i<numberOfBuffer;i++){
        bufferArray[i].controlBlock.isUsed = false;
        bufferArray[i].controlBlock.next = NULL;
        bufferArray[i].controlBlock.prev = NULL;
    }

    HEAD(LRUList) = &LRUList;
    TAIL(LRUList) = &LRUList;
    LRUList.tableID = LRU;
    numberOfBuffer = num_buf;
    bufferUse = 0;

    for (int i=1;i<=MAX_TABLE_NUM;i++){
        tableList[i] = UNUSED;
        history[i] = (char*)malloc(MAX_PATHNAME_LENGHT+1);
    }

    return 0;
}

// Terminate buffer
int buf_terminate(){
    for (int i=1;i<=MAX_TABLE_NUM;i++){
        int dataFile = tableList[i];
        if (dataFile!=UNUSED){
            // printf("!%d\n",dataFile);
            // printf("closing tid%d...\n",i);
            if (buf_close_table(i)<0)
                return -1;
        }
    }

    for (int i=1;i<=MAX_TABLE_NUM;i++){
        free(history[i]);
    }
    free(bufferArray);
    return 0;
}

int buf_find_tableID(char* pathname){
    for (int i=1;i<=MAX_TABLE_NUM;i++){
        if (strcmp(history[i],pathname)==0)
            return i;
    }
    return -1;
}

int buf_open_table(char* pathname, int mode){
    int fd;
    fd = file_open_table(pathname, mode);
    // printf("opening fd%d...\n",fd);

    if (fd==-1)
        return fd;

    int tableID = buf_find_tableID(pathname);
    if (tableID==-1){
        if (currentTableID>MAX_TABLE_NUM)
            return -1;
        
        tableID = currentTableID;
        currentTableID++;
        memcpy(history[tableID],pathname,MAX_PATHNAME_LENGHT);
    }
    tableList[tableID] = fd;
    
    return tableID;
}

int buf_close_table(int tableID){
    int dataFile;
    dataFile = tableList[tableID];
    // 해시 테이블여기도 써야할까?
    for (int i=0;i<numberOfBuffer;i++){
        if (bufferArray[i].controlBlock.isUsed && bufferArray[i].controlBlock.tableID==tableID ){
            file_write_page(dataFile, bufferArray[i].controlBlock.pageNumber, &bufferArray[i].frame);
            bufferArray[i].controlBlock.isUsed = false;
            bufferArray[i].controlBlock.prev->next = bufferArray[i].controlBlock.next;
            bufferArray[i].controlBlock.next->prev = bufferArray[i].controlBlock.prev;
            bufferUse--;
        }
    } 
    tableList[tableID]=UNUSED;
    // printf("closing fd%d...\n",dataFile);
    return file_close_table(dataFile);
}

pagenum_t buf_alloc_page(tableid tableID){
    
    pagenum_t freePageNum;
    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    
    if (headerPage.freePageNumber==0){
        freePageNum = headerPage.numberOfPage;
        int newBufferIdx = buf_alloc_buffer();
        Buffer* newBuffer = &bufferArray[newBufferIdx]; 
        newBuffer->controlBlock.tableID = tableID;
        newBuffer->controlBlock.pageNumber=freePageNum;
        newBuffer->controlBlock.isDirty=true;
        newBuffer->controlBlock.isUsed=true;        
        newBuffer->controlBlock.isPinned=true;

        newBuffer->controlBlock.next = HEAD(LRUList);
        newBuffer->controlBlock.prev = &LRUList;
        HEAD(LRUList)->prev = &newBuffer->controlBlock;
        HEAD(LRUList) = &newBuffer->controlBlock;

        headerPage.numberOfPage++;
        buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
        if (debug) printf("--------alloc page%llu-%d\n",freePageNum,tableID);
        return freePageNum;
    }

    FreePage freePage;
    freePageNum = headerPage.freePageNumber;
    buf_get_page(tableID, freePageNum, (page_t*)&freePage);
    headerPage.freePageNumber = freePage.nextFreePageNumber;
    buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    if (debug) printf("--------alloc page%llu-%d\n",freePageNum,tableID);
    return freePageNum;
}

// Free an on-buffer page to the free page list
void buf_free_page(tableid tableID, pagenum_t pagenum){ 
    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);

    FreePage freePage;
    freePage.nextFreePageNumber = headerPage.freePageNumber;
    buf_set_page(tableID, pagenum, (page_t*)&freePage);

    headerPage.freePageNumber = pagenum;
    buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
}

int buf_eviction(){
    BufferControlBlock* victim;
    victim = TAIL(LRUList);
    while(victim->isPinned){
        victim = victim->prev;
    }    

    victim->prev->next = victim->next;
    victim->next->prev = victim->prev;

    int victimIdx;
    victimIdx = buf_find_page(victim->tableID,victim->pageNumber);
    bufferUse--;

    if (victim->isDirty){
        int dataFile;
        dataFile = tableList[victim->tableID];
        file_write_page(dataFile, victim->pageNumber, &bufferArray[victimIdx].frame);
    }

    return victimIdx;
}


int buf_alloc_buffer(){
    int freeBufferIndex;
    if (bufferUse < numberOfBuffer){
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


// Get needed page from bufferArray
void buf_get_page(tableid tableID, pagenum_t pageNum, page_t* dest){
    if (debug) printf("----------get page%llu-%d\n",pageNum,tableID);
    int dataFile;

    // 1. Buffer Array has a page

    int getBufferIdx;
    getBufferIdx = buf_find_page(tableID, pageNum);
    if (getBufferIdx!=-1){
        Buffer* buffer = &bufferArray[getBufferIdx];
        buffer->controlBlock.isPinned = true;
        memcpy(dest, &(buffer->frame), PAGE_SIZE);

        buffer->controlBlock.next->prev = buffer->controlBlock.prev;
        buffer->controlBlock.prev->next = buffer->controlBlock.next;

        buffer->controlBlock.next = HEAD(LRUList);
        buffer->controlBlock.prev = &LRUList;
        
        HEAD(LRUList)->prev = &buffer->controlBlock;
        HEAD(LRUList) = &buffer->controlBlock;            
        return;  
    }
    
    // 2. Buffer Array has not a page

    int newBufferIdx;
    newBufferIdx = buf_alloc_buffer();
    Buffer* newBuffer = &bufferArray[newBufferIdx];

    dataFile = tableList[tableID];
    file_read_page(dataFile, pageNum, dest);
    memcpy(&(newBuffer->frame), dest, PAGE_SIZE);

    newBuffer->controlBlock.tableID = tableID;
    newBuffer->controlBlock.pageNumber = pageNum;
    newBuffer->controlBlock.isDirty=false;
    newBuffer->controlBlock.isUsed=true;
    newBuffer->controlBlock.isPinned=true;

    newBuffer->controlBlock.next = HEAD(LRUList);
    newBuffer->controlBlock.prev = &LRUList;
    HEAD(LRUList)->prev = &(newBuffer->controlBlock);
    HEAD(LRUList) = &(newBuffer->controlBlock);
}

int buf_find_page(tableid tableID, pagenum_t pageNum){
    for (int i=0;i<numberOfBuffer;i++){
        if (bufferArray[i].controlBlock.tableID==tableID && pageNum==bufferArray[i].controlBlock.pageNumber){
            return i;
        }
    }
    return -1;
}

// Set page to bufferArray 
void buf_set_page(tableid tableID, pagenum_t pageNum, const page_t* src){
    if (debug) printf("----------set page%llu-%d\n",pageNum,tableID);
    int setBufferIdx;
    setBufferIdx = buf_find_page(tableID,pageNum);
    if (setBufferIdx==numberOfBuffer){
        printf("error: setting buffer!\n");
        return;
    }

    Buffer* setBuffer = &bufferArray[setBufferIdx];
    memcpy(&(setBuffer->frame), src, PAGE_SIZE);
    setBuffer->controlBlock.isDirty = true;
    setBuffer->controlBlock.isPinned = false;
}

void buf_unpin_page(tableid tableID, pagenum_t pageNum){
    if (debug) printf("--------unpin page%llu-%d\n",pageNum,tableID);
    int setBufferIdx;
    for (int i=0;i<numberOfBuffer;i++){
        if (bufferArray[i].controlBlock.tableID==tableID && pageNum==bufferArray[i].controlBlock.pageNumber){
            setBufferIdx = i;
            break;      
        }
    } 

    if (setBufferIdx==numberOfBuffer){
        printf("error: unpin mismatch!!\n");
    }else{
        bufferArray[setBufferIdx].controlBlock.isPinned = false;
    }
    
}

void printBufferArray(){
    for (int i=0;i<numberOfBuffer;i++){
        Buffer * buffer = &bufferArray[i];
        if (buffer->controlBlock.isUsed){
            if (buffer->controlBlock.isPinned)
                printf("#%llu.%d | ",buffer->controlBlock.pageNumber,buffer->controlBlock.tableID);
            else
                printf("%llu.%d | ",buffer->controlBlock.pageNumber,buffer->controlBlock.tableID);
        }else{
            printf("    | ");
        }
    }
    printf("\n");
}

void printLRUList(){
    BufferControlBlock* buf;
    buf = LRUList.next;
    printf("HEAD - ");
    while(buf!=&LRUList){
        printf("%llu - ",buf->pageNumber);
        buf = buf->next;
    }
    printf("TAIL\n");   
}