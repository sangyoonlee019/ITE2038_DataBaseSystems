#include "file.h"
#include "log.h"

pthread_mutex_t log_buffer_manager_latch = PTHREAD_MUTEX_INITIALIZER;
char* logBuffer;
int offset;
int flushOffset;
int logmsgFile;

int logFile;
int logmsgFile;


// External API
int log_initialize(char* logPath, char* logmsgPath){
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
    logBuffer = (char*)malloc(sizeof(char)*MAX_LOG_SIZE);
    pthread_mutex_unlock(&log_buffer_manager_latch);
    return log_load();
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
    flushOffset+=offset;
    pthread_mutex_unlock(&log_buffer_manager_latch);
    return 1;
}

void log_terminate(){
    file_close_log();
    file_close_logmsg();
}

// Inner API
int log_load(void){
    lseek(logFile, 0, SEEK_SET);
    read(logFile, logBuffer, MAX_LOG_SIZE);
    recovery();
    return 1;
}

lsn_t log_read_log(lsn_t LSN,Log* log){
    memcpy(log, logBuffer+LSN, LOG_BCR_SIZE);
    if(log->logSize==0)
        return 0;

    if (log->type==LT_UPDATE){
        memcpy(log, logBuffer+LSN, LOG_ULR_SIZE);
    }else if (log->type==LT_COMPENSATE){
        memcpy(log, logBuffer+LSN, LOG_CLR_SIZE);
    }
    offset = LSN+log->logSize;
    flushOffset+=log->logSize;
    return offset;
}

void recovery(void){
    Log log;
    while(log_read_log(offset, &log)){
        print_log(&log);
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
