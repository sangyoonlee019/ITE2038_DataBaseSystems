#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "buf.h"
#include "file.h"

#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// #########################수정해야함 min max
// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

#define MAX_NODE_NUMBER 10000000

#define RIGHT_NEIGHBOR -1

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int leafOrder;
extern int indexOrder;
extern int tableNum;
// extern int dataFile;
// extern HeaderPage headerPage;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
extern pagenum_t queue[MAX_NODE_NUMBER];

// FUNCTION PROTOTYPES.

// New find functuin
int find(int tableID, int64_t key, char* returnValue);
pagenum_t findLeaf(int tableID, int64_t key, LeafPage* leafNode);

// New insert function
int insert(int tableID, int64_t key, char* value);
void startNewTree(int tableID, int64_t key, char* value);
void insertIntoLeaf(int tableID, LeafPage* leafNode, pagenum_t leafPageNum, int64_t key, char* value);
void insertIntoLeafAfterSplitting(int tableID, LeafPage* leafNode, pagenum_t leafPageNum, int64_t key, char* value);
void insertIntoParent(int tableID, NodePage* leftNode, pagenum_t leftPageNum, int64_t key, NodePage* rightNode, pagenum_t rightPageNum);
void insertIntoNewRoot(int tableID, NodePage* leftNode, pagenum_t leftPageNum, int64_t key, NodePage* rightNode, pagenum_t rightPageNum);
int getLeftIndex(InternalPage* parentNode, pagenum_t leftPageNum);
void insertIntoInternal(int tableID, InternalPage* parentNode, pagenum_t parentPageNum, int leftIndex, int64_t key, pagenum_t rightPageNum);
void insertIntoInternalAfterSplitting(int tableID, InternalPage* parentNode, pagenum_t parentPageNum, int leftIndex, int64_t key, pagenum_t rightPageNum);

// New untility function
int initDB(int bufferNum);
int openTable(char* pathname);
int closeTable(int tableID);
int shutdownDB(void);
void printTree(int tableID);
int pathToRoot(int tableID, NodePage* node);
void printPage(int tableID, pagenum_t pageNum);
void printNode(NodePage* node, pagenum_t nodePageNum);
void usage_1( void );
void usage_2( void );
void usage_3( void );
int cut( int length );

// New delete function
int delete(int tableID, int64_t key);
void deleteEntry(int tableID, NodePage* node, pagenum_t pageNum, int64_t key);
void removeEntryFromNode(int tableID, NodePage* node, pagenum_t pageNum, int64_t key);
void adjustRoot(int tableID, NodePage* rootNode, pagenum_t rootPageNum);
int getNeighborIndex(NodePage* node, pagenum_t pageNum, InternalPage* parentNode);
void coalesceNodes(int tableID, NodePage* node, pagenum_t nodePageNum, NodePage* neighborNode, 
        pagenum_t neighborPageNum, InternalPage* parentNode, int neighborIndex, int64_t primeKey);
void redistributeInternalNodes(int tableID, InternalPage* internalNode, pagenum_t internalPageNum, InternalPage* neighborNode, 
        pagenum_t neighborPageNum, InternalPage* parentNode, int neighborIndex, int primeKeyIndex, int64_t primeKey);



#endif /* __BPT_H__*/
