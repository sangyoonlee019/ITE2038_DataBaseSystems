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
#include "file.h"

#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

#define DEFAULT_TABLE_ID 0

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

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct record {
    int value;
} record;

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next; // Used for queue.
} node;



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
extern int order;
extern int leafOrder;
extern int indexOrder;
extern int tableID;
extern int dataFile;
extern HeaderPage headerPage;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
extern pagenum_t queue[MAX_NODE_NUMBER];

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
extern bool verbose_output;


// FUNCTION PROTOTYPES.

// Project2

int open_table(char* pathname);
int close_table(void);
int db_insert (int64_t key, char * value);
int db_find (int64_t key, char * ret_val);
int db_delete (int64_t key);


// New find functuin
char* findValue(int64_t key);
pagenum_t findLeaf(int64_t key, LeafPage* leafNode);

// New insert function
void insertRecord(int64_t key);
void startNewTree(int64_t key, char* value);
void insertIntoLeaf(LeafPage* leafNode, pagenum_t leafPageNum, int64_t key, char* value);
void insertIntoLeafAfterSplitting(LeafPage* leafNode, pagenum_t leafPageNum, int64_t key, char* value);
void insertIntoParent(NodePage* leftNode, pagenum_t leftPageNum, int64_t key, NodePage* rightNode, pagenum_t rightPageNum);
void insertIntoNewRoot(NodePage* leftNode, pagenum_t leftPageNum, int64_t key, NodePage* rightNode, pagenum_t rightPageNum);
int getLeftIndex(InternalPage* parentNode, pagenum_t leftPageNum);
void insertIntoInternal(InternalPage* parentNode, pagenum_t parentPageNum, int leftIndex, int64_t key, pagenum_t rightPageNum);
void insertIntoInternalAfterSplitting(InternalPage* parentNode, pagenum_t parentPageNum, int leftIndex, int64_t key, pagenum_t rightPageNum);

// New untility function
void printTree(void);
int pathToRoot(NodePage* node);
void printPage(pagenum_t pageNum);
void printNode(NodePage* node, pagenum_t nodePageNum);

// New delete function
void deleteEntry(NodePage* node, pagenum_t pageNum, int64_t key);
void removeEntryFromNode(NodePage* node, pagenum_t pageNum, int64_t key);
void adjustRoot(NodePage* rootNode, pagenum_t rootPageNum);
int getNeighborIndex(NodePage* node, pagenum_t pageNum, InternalPage* parentNode);
void coalesceNodes(NodePage* node, pagenum_t nodePageNum, NodePage* neighborNode, 
        pagenum_t neighborPageNum, InternalPage* parentNode, int neighborIndex, int64_t primeKey);
void redistributeInternalNodes(InternalPage* internalNode, pagenum_t internalPageNum, InternalPage* neighborNode, 
        pagenum_t neighborPageNum, InternalPage* parentNode, int neighborIndex, int primeKeyIndex, int64_t primeKey);

// Output and utility.

void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void usage_3( void );
void enqueue( node * new_node );
node * dequeue( void );
int height( node * root );
int path_to_root( node * root, node * child );
void print_leaves( node * root );
void print_tree( node * root );
void find_and_print(node * root, int key, bool verbose); 
void find_and_print_range(node * root, int range1, int range2, bool verbose); 
int find_range( node * root, int key_start, int key_end, bool verbose,
        int returned_keys[], void * returned_pointers[]); 
node * find_leaf( node * root, int key, bool verbose );
record * find( node * root, int key, bool verbose );
int cut( int length );

// Insertion.

record * make_record(int value);
node * make_node( void );
node * make_leaf( void );
int get_left_index(node * parent, node * left);
node * insert_into_leaf( node * leaf, int key, record * pointer );
node * insert_into_leaf_after_splitting(node * root, node * leaf, int key,
                                        record * pointer);
node * insert_into_node(node * root, node * parent, 
        int left_index, int key, node * right);
node * insert_into_node_after_splitting(node * root, node * parent,
                                        int left_index,
        int key, node * right);
node * insert_into_parent(node * root, node * left, int key, node * right);
node * insert_into_new_root(node * left, int key, node * right);
node * start_new_tree(int key, record * pointer);
node * insert( node * root, int key, int value );

// Deletion.

int get_neighbor_index( node * n );
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
                      int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
                          int neighbor_index,
        int k_prime_index, int k_prime);
node * delete_entry( node * root, node * n, int key, void * pointer );
node * delete( node * root, int key );

void destroy_tree_nodes(node * root);
node * destroy_tree(node * root);

#endif /* __BPT_H__*/
