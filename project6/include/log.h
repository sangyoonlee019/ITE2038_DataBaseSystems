#ifndef __LOG_H__
#define __LOG_H__

#include "file.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOG_SIZE 500000
#define MAX_TRX_SIZE 500000

#define LOG_BCR_SIZE 28
#define LOG_ULR_SIZE 288
#define LOG_CLR_SIZE 296

#define LT_BEGIN 0
#define LT_UPDATE 1
#define LT_COMMIT 2
#define LT_ROLLBACK 3
#define LT_COMPENSATE 4

#define REDO_CRASH 1
#define UNDO_CRASH 2

struct UpdateLog{
    char size[LOG_ULR_SIZE];
}typedef UpdateLog;

struct CompensateLog{
    char size[LOG_CLR_SIZE];
}typedef CompensateLog;

struct BCRLog{
    char size[LOG_BCR_SIZE];
}typedef BCRLog;

#pragma pack(push,4)
struct Log {
    int32_t logSize;
    lsn_t LSN;
    lsn_t prevLSN;
    int32_t trxID;
    int32_t type;
    int32_t tableID;
    pagenum_t pageNumber;
    int32_t offset;
    int32_t dataLength;
    char oldImage[120];
    char newImage[120];
    lsn_t nextUndoLSN;
}typedef Log;
#pragma pack(pop)

int log_initialize(int flag,int log_num,char* logPath, char* logmsgPath);
int log_flush(void);
int log_write_log(Log* log);
lsn_t log_read_log(lsn_t LSN,Log* log);
void recovery(int flag, int log_num);
int log_load(int flag, int log_num);
void log_terminate(void);
void print_log(Log* log);
lsn_t log_new(Log* log, int32_t trxID, lsn_t prevLSN, int32_t type);

#endif /* __LOG_H__*/