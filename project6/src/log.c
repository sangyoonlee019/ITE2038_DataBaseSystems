#include "file.h"
#include "log.h"
#include "lock.h"
#include "bpt.h"

pthread_mutex_t log_buffer_manager_latch = PTHREAD_MUTEX_INITIALIZER;
char* logBuffer;
lsn_t offset;
lsn_t lastOffset;
lsn_t flushOffset;
int logmsgFile;

int logFile;
int logmsgFile;
int loser[MAX_TRX_SIZE];
lsn_t loserLastLSN[MAX_TRX_SIZE];

// External API
int log_initialize(int flag,int log_num,char* logPath, char* logmsgPath){
    printf("%s,%s \n",logPath,logmsgPath);
    pthread_mutex_lock(&log_buffer_manager_latch);
    
    if (file_open_log(logPath,OPEN_EXIST)==-1){
        if (file_open_log(logPath,OPEN_NEW)==-1){
            printf("error: creating the new logFile\n");
            return -1;
        }
    }

    if (file_open_logmsg(logmsgPath,OPEN_EXIST)==-1){
        if (file_open_logmsg(logmsgPath,OPEN_NEW)==-1){
            printf("error: creating the new logmsgFile\n");
            return -1;
        }
    }
    offset = 0;
    lastOffset = 0;
    logBuffer = (char*)malloc(sizeof(char)*MAX_LOG_SIZE);
    
    int ret = log_load(flag,log_num);
    pthread_mutex_unlock(&log_buffer_manager_latch);
    return ret;
}

int log_write_log(Log* log){
    pthread_mutex_lock(&log_buffer_manager_latch);
    if (offset+log->logSize>MAX_LOG_SIZE){
        lseek(logFile, flushOffset, SEEK_SET);
        write(logFile, logBuffer, offset);
        offset = 0;
        flushOffset+=offset;
    }

    memcpy(logBuffer+offset,log,log->logSize);
    lastOffset = offset;
    offset+=log->logSize;
    pthread_mutex_unlock(&log_buffer_manager_latch);
    return 1;
}

int log_flush(void){
    printf("logFile: %d\n",logFile);
    pthread_mutex_lock(&log_buffer_manager_latch);
    lseek(logFile, flushOffset, SEEK_SET);
    write(logFile, logBuffer, offset);
    offset = 0;
    lastOffset = 0;
    flushOffset+=offset;
    pthread_mutex_unlock(&log_buffer_manager_latch);
    return 1;
}

void log_terminate(){
    file_close_log();
    file_close_logmsg();
}

lsn_t log_new(Log* log, int32_t trxID, lsn_t prevLSN, int32_t type){
    log->LSN = offset;
    log->prevLSN = prevLSN;
    log->logSize = LOG_BCR_SIZE;
    log->trxID = trxID;
    log->type = type;

    return offset;
}

lsn_t log_read_log(lsn_t LSN,Log* log){
    if (LSN<0) return 0;
    pthread_mutex_lock(&log_buffer_manager_latch);
    memcpy(log, logBuffer+LSN, LOG_BCR_SIZE);
    if(log->logSize==0)
        return 0;

    if (log->type==LT_UPDATE){
        memcpy(log, logBuffer+LSN, LOG_ULR_SIZE);
    }else if (log->type==LT_COMPENSATE){
        memcpy(log, logBuffer+LSN, LOG_CLR_SIZE);
    }
    pthread_mutex_unlock(&log_buffer_manager_latch);
    return LSN+log->logSize;
}

// Inner API
int log_load(int flag, int log_num){
    lseek(logFile, 0, SEEK_SET);
    read(logFile, logBuffer, MAX_LOG_SIZE);
    recovery(flag, log_num);
    return 0;
}


void recovery(int flag, int log_num){
    Log log;
    int logCount = 0;
    int readOffset;
    printf("offsetStart %llu\n",offset);
    
    // Analysis Pass
    readOffset = 0;
    while((readOffset = log_read_log(readOffset, &log))){
        printf("printingLog...\n");
        print_log(&log);
        if(log.type==LT_BEGIN){
            loser[log.trxID] = 1;      
            loserLastLSN[log.trxID] = log.LSN;                  
        }else if(log.type==LT_COMMIT || log.type==LT_ROLLBACK){
            loser[log.trxID] = 0;
        }else {
            if(loser[log.trxID]){
                loserLastLSN[log.trxID] = log.LSN;
            }
        }
    }

    // Redo pass
    readOffset = 0;
    while((readOffset = log_read_log(readOffset, &log))){
        if(log.type==LT_UPDATE || log.type==LT_COMPENSATE){
            pagenum_t pageNum = log.pageNumber;
            char pathname[15];
            sprintf(pathname,"DATA%d",log.tableID);
            openTable(pathname);
            LeafPage leafNode;
            buf_get_page(log.tableID, log.pageNumber, (page_t*)&leafNode);
            if(leafNode.pageLSN<log.LSN){
                memcpy(&leafNode+log.offset,log.oldImage,VALUE_SIZE);
                buf_set_page(log.tableID, log.pageNumber, (page_t*)&leafNode);
            }else{
                buf_unpin_page(log.tableID, log.pageNumber);
            }
            logCount++;
            if (flag==REDO_CRASH && logCount==log_num) return; 
        }
    }

    // Undo pass
    logCount = 0;
    for (int i=0;i<MAX_TRX_SIZE;i++){
        if (loser[i]==1){
            Trx* trx = trx_new(i);
            trx->leastLSN = loserLastLSN[i];
        }
    }

    while(1){
        int noLooser = 1;
        lsn_t maxLastLSN = 0;
        int lastTrxID;
        for (int i=0;i<MAX_TRX_SIZE;i++){
            if (loser[i]==1){
                // if (loserLastLSN[i]!=-1){
                noLooser = 0;
                if (maxLastLSN<loserLastLSN[i]){
                    maxLastLSN=loserLastLSN[i];
                    lastTrxID = i;
                }
                // } 
            }
        }
        if (noLooser) break;

        Log log;
        log_read_log(maxLastLSN, &log);
        if(log.type==LT_UPDATE || log.type==LT_COMPENSATE){
            pagenum_t pageNum = log.pageNumber;
            char pathname[15];
            sprintf(pathname,"DATA%d",log.tableID);
            openTable(pathname);
            LeafPage leafNode;
            buf_get_page(log.tableID, log.pageNumber, (page_t*)&leafNode);
            if(leafNode.pageLSN>=log.LSN){
                Log newLog;
                log_new(&newLog, log.trxID, log.LSN, LT_COMPENSATE);
                newLog.tableID = log.tableID;
                newLog.pageNumber = log.pageNumber;
                newLog.offset = log.offset;
                newLog.dataLength = log.dataLength;
                memcpy(newLog.newImage,log.oldImage,VALUE_SIZE);
                memcpy(newLog.oldImage,log.newImage,VALUE_SIZE);
                // 라스트 시퀀스 바꿔주기
                log_write_log(&newLog);

                memcpy(&leafNode+log.offset,log.oldImage,VALUE_SIZE);
                leafNode.pageLSN = newLog.LSN;
                buf_set_page(log.tableID, log.pageNumber, (page_t*)&leafNode);
            }
            loserLastLSN[log.trxID] = log.prevLSN;
        }else if(log.type==LT_BEGIN){
            Log newLog;
            log_new(&newLog, log.trxID, log.LSN, LT_ROLLBACK);
            log_write_log(&newLog);
            loser[log.trxID] = 0;
        }
        logCount++;
        if (flag==UNDO_CRASH && logCount==log_num) return; 
    }
}


void print_log(Log* log){
    if (log->type==LT_UPDATE){
        printf("UPDATE %d\n",log->logSize);
    }else if (log->type==LT_COMPENSATE){
        printf("COMPENSATE %d\n",log->logSize);
    }else{
        printf("BCR %d\n",log->logSize);
    }
}
