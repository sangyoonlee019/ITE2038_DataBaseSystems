#ifndef __LOG_H__
#define __LOG_H__

#include<inttypes.h>
#include <pthread.h>

#define RC_NORMAL 0
#define RC_REDO_CRASH 1
#define RC_UNDO_CRASH 2

int log_alnalisis(void);
int log_redo(void);
int log_undo(void);

#endif /* __LOG_H__*/