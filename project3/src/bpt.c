/*
 *  bpt.c  
 */
#define Version "1.14"

/*  bpt:  B+ Tree Disk-Based Implementation
*/

#include "bpt.h"
#include "buf.h"

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
int leafOrder = DEFAULT_LEAF_ORDER;
int internalOrder = DEFAULT_INTERNAL_ORDER;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
pagenum_t queue[MAX_NODE_NUMBER];

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES

int initDB(int bufferNum){
    return buf_initialize(bufferNum);
}

/* Open table from path's datafile.
 */
int openTable( char* pathname ){
    int tableID;
    HeaderPage headerPage;

    tableID = buf_open_table(pathname,OPEN_EXIST);
    if (tableID==-1){
        tableID = buf_open_table(pathname,OPEN_NEW);
        if (tableID==-1){
            printf("error: creating the new datafile\n");
            return -1;
        }
        buf_get_page(tableID,HEADER_PAGE_NUMBER,(page_t*)&headerPage);
        headerPage.freePageNumber = 0;
        headerPage.rootPageNumber = 0;
        headerPage.numberOfPage = 1;
        buf_set_page(tableID,HEADER_PAGE_NUMBER,(page_t*)&headerPage);
    }
    return tableID;
}

int closeTable(int tableID){
    return buf_close_table(tableID);
}

int shutdownDB(void){
    return buf_terminate();
}
/* Finds and returns the record to which
 * a key refers.
 */
// closed pin function;
int find (int tableID, int64_t key, char * returnValue){
    // Table is not opend yet.
    if (!buf_table_is_open(tableID)){
        return -1;
    }

    LeafPage leafNode;
    pagenum_t leafPageNum;
    leafPageNum = findLeaf(tableID,key,&leafNode);
    if(leafPageNum==0){
        buf_unpin_page(tableID, leafPageNum);    
        return -2;
    }
    
    // BS
    for (int i=0;i<leafNode.numberOfKeys;i++){
        if (leafNode.records[i].key == key){
            memcpy(returnValue,leafNode.records[i].value,VALUE_SIZE);
            buf_unpin_page(tableID, leafPageNum);
            return 0;
        }
    }
    buf_unpin_page(tableID, leafPageNum);
    return -1;
}

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
// unclosed pin function;
pagenum_t findLeaf(int tableID, int64_t key, LeafPage* retLeafNode){
    
    HeaderPage headerPage;
    buf_get_page(tableID,HEADER_PAGE_NUMBER,(page_t*)&headerPage);
    pagenum_t rootPageNum = headerPage.rootPageNumber; 
    if (rootPageNum == 0) {
        return 0;
    }
    buf_unpin_page(tableID, HEADER_PAGE_NUMBER);

    pagenum_t nextPageNum = rootPageNum;
    pagenum_t pastPageNum = nextPageNum; 
    InternalPage node;
    buf_get_page(tableID, rootPageNum, (page_t*)&node);

    while (!node.isLeaf) {
        // BS
        int i = 0;
        while (i < node.numberOfKeys) {
            if (key >= node.recordIDs[i].key) i++;
            else break;
        }
        
        nextPageNum = pageNumOf(&node,i);
        buf_unpin_page(tableID, pastPageNum);
        pastPageNum = nextPageNum;
        buf_get_page(tableID, nextPageNum, (page_t*)&node);
    }
    
    memcpy(retLeafNode, &node, PAGE_SIZE);

    return nextPageNum;
}

/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
int insert (int tableID, int64_t key, char * value){
    // Table is not opend yet.
    if (!buf_table_is_open(tableID)){
        return -1;
    }

    /* The current implementation ignores
     * duplicates.
     */
    
    char valueFound[120];
    if (find(tableID, key, valueFound) == 0){
        return -1;
    }   
    
    /* Case: the tree does not exist yet.
     * Start a new tree.
     */
    
    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    if (headerPage.rootPageNumber == 0) {
        buf_unpin_page(tableID, HEADER_PAGE_NUMBER);
        startNewTree(tableID, key, value);
        return 0;
    }
    buf_unpin_page(tableID, HEADER_PAGE_NUMBER);

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    LeafPage leafNode;
    pagenum_t leafPageNum;
    leafPageNum = findLeaf(tableID, key, &leafNode);

    /* Case: leaf has room for key and pointer.
     */

    if (leafNode.numberOfKeys < leafOrder - 1) {
        insertIntoLeaf(tableID, &leafNode, leafPageNum, key, value);
        return 0;
    }
    
    /* Case:  leaf must be split.
     */
    
    insertIntoLeafAfterSplitting(tableID, &leafNode, leafPageNum, key, value);
    return 0;
}

// INSERTION

/* First insertion:
 * start a new tree.
 */
void startNewTree(int tableID, int64_t key, char* value){
    HeaderPage headerPage;
    

    pagenum_t freePageNum;
    freePageNum = buf_alloc_page(tableID);
    LeafPage rootNode;
    rootNode.parentPageNumber = 0;
    rootNode.isLeaf = true;
    rootNode.numberOfKeys = 1;
    rootNode.rightSiblingPageNumber = 0;
    rootNode.records[0].key = key;
    memcpy(rootNode.records[0].value, value, VALUE_SIZE);
    buf_set_page(tableID, freePageNum, (page_t*)&rootNode);

    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    headerPage.rootPageNumber = freePageNum;
    buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
}

/* Inserts a new record
 * into a leaf.
 * Returns the altered leaf.
 */
// closed pin function
void insertIntoLeaf(int tableID, LeafPage* leafNode, pagenum_t leafPageNum, int64_t key, char* value){
    // BS
    if (debug) printf(". . .insertIntoLeaf %llu\n",leafPageNum);
    int insertionPoint = 0;
    while (insertionPoint < leafNode->numberOfKeys && leafNode->records[insertionPoint].key < key)
        insertionPoint++;

    for (int i = leafNode->numberOfKeys; i > insertionPoint; i--) {
        leafNode->records[i].key = leafNode->records[i-1].key;
        memcpy(leafNode->records[i].value, leafNode->records[i-1].value, VALUE_SIZE);
    }
    leafNode->records[insertionPoint].key = key;
    memcpy(leafNode->records[insertionPoint].value, value, VALUE_SIZE);
    leafNode->numberOfKeys++;
    buf_set_page(tableID ,leafPageNum, (page_t*)leafNode);
}

/* Inserts a new record
 * into a leaf so as to exceed
 * the tree's order, causing the leaf node to be split
 * in half.
 */
void insertIntoLeafAfterSplitting(int tableID, LeafPage* leafNode, pagenum_t leafPageNum, int64_t key, char* value){
    if (debug) printf(". . .insertIntoLeafAfterSplitting %llu\n",leafPageNum);
    int i, j;
    LeafPage newLeafNode;
    newLeafNode.isLeaf = true;
    newLeafNode.numberOfKeys  = 0;
    newLeafNode.rightSiblingPageNumber = leafPageNum;
    // BS
    int insertionIndex = 0;
    while (insertionIndex < leafOrder - 1 && leafNode->records[insertionIndex].key < key)
        insertionIndex++;

    int split = cut(leafOrder - 1);

    /* Case:  insertion_index behingd split.
     */
    if (insertionIndex >= split){
        
        // Insert to newLeafNode
        for (i = split, j = 0; i<leafOrder-1;i++, j++){
            if (i==insertionIndex){
                 j++;
            }             
            newLeafNode.records[j].key = leafNode->records[i].key;
            memcpy(newLeafNode.records[j].value, leafNode->records[i].value, VALUE_SIZE); 
            newLeafNode.numberOfKeys++;

            // Delete moved records in oldLeafNode
            leafNode->numberOfKeys--;      
        }

        // Insert new record
        newLeafNode.records[insertionIndex-split].key = key;
        memcpy(newLeafNode.records[insertionIndex-split].value, value, VALUE_SIZE);
        newLeafNode.numberOfKeys++;
    }else{
        /* Case:  split behind insertion_index.
         */
        // Insert to newLeafNode
        for (i = split-1, j = 0; i<leafOrder-1; i++, j++){
            newLeafNode.records[j].key = leafNode->records[i].key;
            memcpy(newLeafNode.records[j].value, leafNode->records[i].value, VALUE_SIZE); 
            newLeafNode.numberOfKeys++;

            // Delete moved records in oldLeafNode
            leafNode->numberOfKeys--;
        }
        
        // Shift records behind insertion_index in oldLeafNode
        for (i = split-1; i>insertionIndex; i--){
            leafNode->records[i].key = leafNode->records[i-1].key;
            memcpy(leafNode->records[i].value, leafNode->records[i-1].value, VALUE_SIZE);
        }
        
        // Insert new record to oldLeafNode.
        leafNode->records[insertionIndex].key = key;
        memcpy(leafNode->records[insertionIndex].value, value, VALUE_SIZE);
        leafNode->numberOfKeys++;

    }
    
    // newLeafNode next page pointer!
    
    pagenum_t newLeafPageNum = buf_alloc_page(tableID);
    newLeafNode.rightSiblingPageNumber = leafNode->rightSiblingPageNumber;
    leafNode->rightSiblingPageNumber = newLeafPageNum;

    int64_t newKey;
    newLeafNode.parentPageNumber = leafNode->parentPageNumber;
    newKey = newLeafNode.records[0].key;

    buf_set_page(tableID, leafPageNum,(page_t*)leafNode);
    buf_set_page(tableID, newLeafPageNum,(page_t*)&newLeafNode);
    insertIntoParent(tableID ,(NodePage*)leafNode, leafPageNum, newKey, (NodePage*)&newLeafNode, newLeafPageNum);
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
void insertIntoParent(int tableID, NodePage* leftNode, pagenum_t leftPageNum, int64_t key, NodePage* rightNode, pagenum_t rightPageNum){
    if (debug) printf(". . .insertIntoParent L:%llu R:%llu\n",leftPageNum,rightPageNum);
    int leftIndex;
    InternalPage parentNode;
    pagenum_t parentPageNum;

    /* Case: new root. */

    parentPageNum = leftNode->parentPageNumber;
    if (parentPageNum == 0){
        insertIntoNewRoot(tableID, leftNode, leftPageNum, key, rightNode, rightPageNum);
        return;
    }

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's pointer to the left 
     * node.
     */

    buf_get_page(tableID,leftNode->parentPageNumber,(page_t*)&parentNode);
    leftIndex = getLeftIndex(&parentNode, leftPageNum);


    /* Simple case: the new key fits into the node. 
     */

    if (parentNode.numberOfKeys < internalOrder - 1){
        insertIntoInternal(tableID, &parentNode, parentPageNum, leftIndex, key, rightPageNum);
        return;
    }

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */

    insertIntoInternalAfterSplitting(tableID, &parentNode, parentPageNum, leftIndex, key, rightPageNum);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insertIntoNewRoot(int tableID, NodePage* leftNode, pagenum_t leftPageNum, int64_t key, NodePage* rightNode, pagenum_t rightPageNum){
    InternalPage rootNode;
    pagenum_t rootPageNum;
    
    rootNode.parentPageNumber = 0;
    rootNode.isLeaf = false;
    rootNode.numberOfKeys = 1;
    rootNode.oneMorePageNumber = leftPageNum;
    rootNode.recordIDs[0].key = key;
    rootNode.recordIDs[0].pageNumber = rightPageNum;
    
    rootPageNum = buf_alloc_page(tableID);
    buf_set_page(tableID, rootPageNum, (page_t*)&rootNode);

    leftNode->parentPageNumber = rootPageNum;
    buf_set_page(tableID, leftPageNum, (page_t*)leftNode);
    rightNode->parentPageNumber = rootPageNum;
    buf_set_page(tableID, rightPageNum, (page_t*)rightNode);

    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    headerPage.rootPageNumber = rootPageNum;
    buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
}

/* Helper function used in insertIntoParent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */
int getLeftIndex(InternalPage* parentNode, pagenum_t leftPageNum){
    // BS
    int leftIndex = 0;
    while (leftIndex <= parentNode->numberOfKeys && 
            pageNumOf(parentNode, leftIndex) != leftPageNum)
        leftIndex++;
    return leftIndex;
}

/* Inserts a new key and page number to a internal node
 * which these can fit
 * without violating the B+ tree properties.
 */
// unclosed pin function - leftNode, rightNode
void insertIntoInternal(int tableID, InternalPage* parentNode, pagenum_t parentPageNum,
            int leftIndex, int64_t key, pagenum_t rightPageNum){
    if (debug) printf(". . .insertIntoInternal P:%llu R:%llu\n",parentPageNum,rightPageNum);
    for (int i = parentNode->numberOfKeys; i > leftIndex; i--) {
        setPageNum(parentNode, i+1, pageNumOf(parentNode, i));
        parentNode->recordIDs[i].key = parentNode->recordIDs[i-1].key; 
    }

    setPageNum(parentNode, leftIndex+1, rightPageNum);
    parentNode->recordIDs[leftIndex].key = key;
    parentNode->numberOfKeys++;

    buf_set_page(tableID, parentPageNum, (page_t*)parentNode);
}

/* Inserts a new key and value to a internal node,
 * causing the internal node's size to exceed
 * the order, and causing the internal node to split into two.
 */
// closed pin function
void insertIntoInternalAfterSplitting(int tableID, InternalPage* parentNode, pagenum_t parentPageNum,
         int leftIndex, int64_t key, pagenum_t rightPageNum){
    if (debug) printf(". . .insertIntoParent P:%llu R:%llu\n",parentPageNum,rightPageNum);
    int i, j;
    pagenum_t newInternalPageNum;
    int64_t * tempKeys;
    pagenum_t* tempPageNums;

    

    tempPageNums = malloc( (internalOrder + 1) * sizeof(pagenum_t) );
    if (tempPageNums == NULL) {
        perror("Temporary pageNumbers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    tempKeys = malloc( internalOrder * sizeof(int64_t) );
    if (tempKeys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < parentNode->numberOfKeys + 1; i++, j++) {
        if (j == leftIndex + 1) j++;
        tempPageNums[j] = pageNumOf(parentNode,i);
    }

    for (i = 0, j = 0; i < parentNode->numberOfKeys; i++, j++) {
        if (j == leftIndex) j++;
        tempKeys[j] = parentNode->recordIDs[i].key;
    }

    tempPageNums[leftIndex + 1] = rightPageNum;
    tempKeys[leftIndex] = key;


    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  
    int split = cut(internalOrder);
    InternalPage newInternalNode;
    newInternalNode.isLeaf = false;
    newInternalNode.numberOfKeys  = 0;
    newInternalPageNum = buf_alloc_page(tableID);
    newInternalNode.parentPageNumber = parentNode->parentPageNumber;

    parentNode->numberOfKeys = 0;
    for (i = 0; i < split - 1; i++) {
        setPageNum(parentNode,i,tempPageNums[i]);
        parentNode->recordIDs[i].key = tempKeys[i];
        parentNode->numberOfKeys++;
    }
    setPageNum(parentNode,i,tempPageNums[i]);
    int primeKey = tempKeys[split - 1];
    for (++i, j = 0; i < internalOrder; i++, j++) {
        setPageNum(&newInternalNode,j,tempPageNums[i]);
        newInternalNode.recordIDs[j].key = tempKeys[i];
        newInternalNode.numberOfKeys++;
    }
    setPageNum(&newInternalNode,j,tempPageNums[i]);
    free(tempPageNums);
    free(tempKeys);

    NodePage childNode;
    for (i = 0; i <= newInternalNode.numberOfKeys; i++) {
        buf_get_page(tableID, pageNumOf(&newInternalNode,i),(page_t*)&childNode);
        childNode.parentPageNumber = newInternalPageNum;
        buf_set_page(tableID, pageNumOf(&newInternalNode,i),(page_t*)&childNode);
    }

    buf_set_page(tableID, parentPageNum,(page_t*)parentNode);
    buf_set_page(tableID, newInternalPageNum,(page_t*)&newInternalNode);

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    insertIntoParent(tableID, (NodePage*)parentNode, parentPageNum, primeKey, (NodePage*)&newInternalNode, newInternalPageNum);
}

/* Master deletion function.
 */
int delete (int tableID, int64_t key){
    // Table is not opend yet.
    if (!buf_table_is_open(tableID)){
        return -1;
    }
 
    char valueFound[120];
    if (find(tableID, key, valueFound) == -1) {
        return -1;
    }
    LeafPage leafNode;
    pagenum_t leafPageNum;
    leafPageNum = findLeaf(tableID, key, &leafNode);
    deleteEntry(tableID, (NodePage*)&leafNode, leafPageNum, key);
    return 0;
}

// DELETION.

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
void deleteEntry(int tableID, NodePage* node, pagenum_t nodePageNum, int64_t key){
    // Remove key and pointer from node.
    removeEntryFromNode(tableID, node, nodePageNum, key);
    /* Case:  deletion from the root. 
     */
    
    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage); 
    if (nodePageNum == headerPage.rootPageNumber){ 
        buf_unpin_page(tableID, HEADER_PAGE_NUMBER);
        adjustRoot(tableID, node, nodePageNum);
        return;
    }

    buf_unpin_page(tableID, HEADER_PAGE_NUMBER);
    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    // min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (node->numberOfKeys > 0)
        return;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    InternalPage parentNode;
    buf_get_page(tableID, node->parentPageNumber, (page_t*)&parentNode);

    NodePage neighborNode;
    pagenum_t neighborPageNum;
    int neighborIndex; 
    neighborIndex = getNeighborIndex(node, nodePageNum, &parentNode);

    int64_t primeKey; 
    int primeKeyIndex;
    if (neighborIndex==RIGHT_NEIGHBOR){
        primeKeyIndex = 0;
        primeKey = parentNode.recordIDs[primeKeyIndex].key;
        neighborPageNum = pageNumOf(&parentNode,1);
        
        buf_get_page(tableID, neighborPageNum, (page_t*)&neighborNode);
    }else{
        primeKeyIndex = neighborIndex;
        primeKey = parentNode.recordIDs[primeKeyIndex].key;
        neighborPageNum = pageNumOf(&parentNode,neighborIndex);

        buf_get_page(tableID, neighborPageNum, (page_t*)&neighborNode);
    }

    if(node->isLeaf){
            
        coalesceNodes(tableID, node, nodePageNum, &neighborNode, neighborPageNum, &parentNode, neighborIndex, primeKey);
    }else{
        if (neighborNode.numberOfKeys < internalOrder -1){
            coalesceNodes(tableID, node, nodePageNum, &neighborNode, neighborPageNum, &parentNode, neighborIndex, primeKey);
        }else{
            redistributeInternalNodes(tableID, (InternalPage*)node, nodePageNum, (InternalPage*)&neighborNode, neighborPageNum, &parentNode, neighborIndex, primeKeyIndex, primeKey);
        }
    }
}

void removeEntryFromNode(int tableID, NodePage* node, pagenum_t nodePageNum, int64_t key){
    
    int i, num_pointers;

    if (node->isLeaf){
        LeafPage* leafNode = (LeafPage*)node; 
        // Remove the key&value and shift other keys&values accordingly.
        i = 0;
        while (leafNode->records[i].key != key)
            i++;
        for (++i; i < leafNode->numberOfKeys; i++){
            leafNode->records[i-1].key = leafNode->records[i].key;
            memcpy(leafNode->records[i-1].value,leafNode->records[i].value,VALUE_SIZE);
        }

        // One key fewer.
        leafNode->numberOfKeys--;
    }else{
        InternalPage* internalNode = (InternalPage*)node; 
        // Remove the key&value and shift other keys&values accordingly.
        i = 0;
        while (internalNode->recordIDs[i].key != key)
            i++;
        for (; i < internalNode->numberOfKeys-1; i++){
            internalNode->recordIDs[i].key = internalNode->recordIDs[i+1].key;
            setPageNum(internalNode,i+1,pageNumOf(internalNode,i+2));
        }
        // One key fewer.
        internalNode->numberOfKeys--;
    }
    buf_set_page(tableID, nodePageNum, (page_t*)node);
}

void adjustRoot(int tableID, NodePage* rootNode, pagenum_t rootPageNum){

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (rootNode->numberOfKeys > 0)
        return;

    /* Case: empty root. 
     */

    // If it is a leaf (has no children),
    // then the whole tree is empty.
    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage); 

    if(rootNode->isLeaf){
        headerPage.rootPageNumber = 0;
    }
    
    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    else{
        InternalPage* internalRootNode = (InternalPage*)rootNode; 
        headerPage.rootPageNumber = pageNumOf(internalRootNode, 0);

        NodePage node;
        buf_get_page(tableID, headerPage.rootPageNumber, (page_t*)&node);
        node.parentPageNumber = 0;
        buf_set_page(tableID, headerPage.rootPageNumber, (page_t*)&node);
    }

    buf_set_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);

    buf_free_page(tableID, rootPageNum); 
}

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int getNeighborIndex(NodePage* node, pagenum_t nodePageNum, InternalPage* parentNode){
    int i;
    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
    for (i = 0; i <= parentNode->numberOfKeys; i++)
        if (pageNumOf(parentNode, i) == nodePageNum)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    exit(EXIT_FAILURE);
}

/* Coalesces a node that has become
 * too small(0) after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
void coalesceNodes(int tableID, NodePage* node, pagenum_t nodePageNum, NodePage* neighborNode, 
        pagenum_t neighborPageNum, InternalPage* parentNode, int neighborIndex, int64_t primeKey){
    int i, j, neighbor_insertion_index, n_end;
    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighborIndex == RIGHT_NEIGHBOR) {
        NodePage* tempNode;
        pagenum_t tempPageNum;
        tempNode = node;
        tempPageNum = nodePageNum;
        node = neighborNode;
        nodePageNum = neighborPageNum;
        neighborNode = tempNode;
        neighborPageNum = tempPageNum;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    int neighborInsertionIndex = neighborNode->numberOfKeys;

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */

    if (node->isLeaf){
        LeafPage* leafNode = (LeafPage*)node;
        LeafPage* leafNeighborNode = (LeafPage*)neighborNode;

        for (i = neighborInsertionIndex, j = 0; j < leafNode->numberOfKeys; i++, j++) {
            leafNeighborNode->records[i].key = leafNode->records[i].key;
            memcpy(leafNeighborNode->records[i].value, leafNode->records[i].value, VALUE_SIZE);
            leafNeighborNode->numberOfKeys++;    
        }
        leafNeighborNode->rightSiblingPageNumber = leafNode->rightSiblingPageNumber;
    }

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    else{
        
        InternalPage* internalNode = (InternalPage*)node;
        InternalPage* internalNeighborNode = (InternalPage*)neighborNode;
        /* Append primeKey.
         */

        internalNeighborNode->recordIDs[neighborInsertionIndex].key = primeKey;
        internalNeighborNode->numberOfKeys++;
        for (i = neighborInsertionIndex + 1, j = 0; j < internalNode->numberOfKeys; i++, j++) {
            setPageNum(internalNeighborNode, i, pageNumOf(internalNode, j));
            internalNeighborNode->recordIDs[i].key = internalNode->recordIDs[j].key;
            internalNeighborNode->numberOfKeys++;
        }
        setPageNum(internalNeighborNode, i, pageNumOf(internalNode, j));

        /* All children must now point up to the same parent.
         */

        NodePage childNode;
        for (i = 0; i < internalNeighborNode->numberOfKeys+1; i++) {
            buf_get_page(tableID, pageNumOf(internalNeighborNode,i),(page_t*)&childNode);
            childNode.parentPageNumber = neighborPageNum;
            buf_set_page(tableID, pageNumOf(internalNeighborNode,i),(page_t*)&childNode);
        }
    }

    buf_set_page(tableID, neighborPageNum, (page_t*)neighborNode);
    deleteEntry(tableID, (NodePage*)parentNode, node->parentPageNumber, primeKey);
    buf_free_page(tableID, nodePageNum);
}

/* Redistributes entries between two internal nodes when
 * one has become too small(1 page number) after deletion
 * but its neighbor is too big(internalOrder -1) to append the
 * small node's entries without exceeding the
 * order
 */
void redistributeInternalNodes(int tableID, InternalPage* internalNode, pagenum_t internalPageNum, InternalPage* neighborNode, 
        pagenum_t neighborPageNum, InternalPage* parentNode, int neighborIndex, int primeKeyIndex, int64_t primeKey){
    
    int i;

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    if(neighborIndex==RIGHT_NEIGHBOR) {  
        InternalPage* internalRightNode = neighborNode;
        
        // Move rightNegibor's first pageNum to node's 1st pageNum.
        // Add parent's primeKey(the key between node and neighbor) to node
        setPageNum(internalNode, 1, pageNumOf(internalRightNode, 0));
        internalNode->recordIDs[0].key = primeKey;
        internalNode->numberOfKeys++;
        
        // Chanege parent's primeKey to rightNeighbor's first key
        parentNode->recordIDs[primeKeyIndex].key = internalRightNode->recordIDs[0].key;

        // Shift rightNeighbor's RecordIDs and delete neighbor's last key;
        for (i = 0; i < internalRightNode->numberOfKeys - 1; i++) {
            setPageNum(internalRightNode, i , pageNumOf(internalRightNode, i + 1));
            internalRightNode->recordIDs[i].key = internalRightNode->recordIDs[i + 1].key;
        }
        setPageNum(internalRightNode, i , pageNumOf(internalRightNode, i + 1));
        internalRightNode->numberOfKeys--;

        // Update child's parentPageNumber from rightNeighborPageNum to nodePageNum  
        NodePage childNode;
        buf_get_page(tableID,pageNumOf(internalNode, 1), (page_t*)&childNode);
        childNode.parentPageNumber = internalPageNum;
        buf_set_page(tableID,pageNumOf(internalNode, 1), (page_t*)&childNode);
    }

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    else {
        InternalPage* internalLeftNode = neighborNode;
        
        // Shift existing pageNum from 0 to 1 and Move leftNegibor's last pageNum to node's first pageNum.
        // Add parent's primeKey(the key between neighbor and node) to node
        setPageNum(internalNode, 1, pageNumOf(internalNode, 0));
        setPageNum(internalNode, 0, pageNumOf(internalLeftNode, internalLeftNode->numberOfKeys));
        internalNode->recordIDs[0].key = primeKey;
        internalNode->numberOfKeys++;
        
        // Chanege parent's primeKey to leftNeighbor's last key
        parentNode->recordIDs[primeKeyIndex].key = internalLeftNode->recordIDs[internalLeftNode->numberOfKeys-1].key;

        // Delete neighbor's last key;
        internalLeftNode->numberOfKeys--;

        // Update child's parentPageNumber to node from leftNeighborNode 
        NodePage childNode;
        buf_get_page(tableID, pageNumOf(internalNode, 0), (page_t*)&childNode);
        childNode.parentPageNumber = internalPageNum;
        buf_set_page(tableID, pageNumOf(internalNode, 0), (page_t*)&childNode);
    }

    buf_set_page(tableID, internalPageNum, (page_t*)internalNode);
    buf_set_page(tableID, neighborPageNum, (page_t*)neighborNode);
    buf_set_page(tableID, internalNode->parentPageNumber, (page_t*)parentNode);
}

/* Prints the B+ tree in the command
 * line in level (rank) order, with the 
 * keys in each node and the '|' symbol
 * to separate nodes.
 */
void printTree(int tableID){
    int i;
    int front = 0;
    int back = 0;
    int rank = 0;
    int newRank = 0;

    HeaderPage headerPage;
    buf_get_page(tableID, HEADER_PAGE_NUMBER, (page_t*)&headerPage);
    if (headerPage.rootPageNumber == 0) {
		printf("Empty tree.\n");
        buf_unpin_page(tableID, HEADER_PAGE_NUMBER);
        return;
    }
    
    queue[back]=headerPage.rootPageNumber;
    buf_unpin_page(tableID, HEADER_PAGE_NUMBER);
    back++;
    while (front < back) {
        pagenum_t nodePageNum = queue[front];
        front++;

        NodePage node;
        buf_get_page(tableID, nodePageNum, (page_t*)&node);
        if(node.parentPageNumber != 0 ){
            InternalPage parentNode;
            buf_get_page(tableID, node.parentPageNumber, (page_t*)&parentNode);
            if(pageNumOf(&parentNode, 0) == nodePageNum){
                newRank = pathToRoot(tableID ,&node);
                if (newRank != rank){
                    rank = newRank;
                    printf("\n");
                } 
            }
            buf_unpin_page(tableID,node.parentPageNumber);
        }

        if(node.isLeaf){
            LeafPage* leafNode = (LeafPage*)&node;
            printf("(%llu) ",nodePageNum);
            for (i = 0;i<leafNode->numberOfKeys;i++){
                printf("%lld ",leafNode->records[i].key);
            }
        }else{
            InternalPage* internalNode = (InternalPage*)&node;
            printf("(%llu) ",nodePageNum);
            for (i = 0;i<internalNode->numberOfKeys;i++){
                printf("%lld ",internalNode->recordIDs[i].key);
                queue[back]=pageNumOf(internalNode,i);
                back++;
            }
            queue[back]=pageNumOf(internalNode,i);
            back++;
        }
        printf("| ");
        buf_unpin_page(tableID, nodePageNum);
    }
    printf("\n");
}

/* Utility function to give the length in edges
 * of the path from any node to the root.
 */
int pathToRoot(int tableID, NodePage* node){
    int length = 0;
    NodePage child;
    pagenum_t parentPageNum, pastPageNum;
    parentPageNum = node->parentPageNumber;
    pastPageNum = parentPageNum;
    while(parentPageNum != 0){
        buf_get_page(tableID,parentPageNum,(page_t*)&child);
        parentPageNum = child.parentPageNumber;
        buf_unpin_page(tableID, pastPageNum);
        pastPageNum = parentPageNum;
        length++;
    }
    return length;
}

void printPage(tableid tableID, pagenum_t pageNum){
    NodePage node;
    int i;
    buf_get_page(tableID, pageNum, (page_t*)&node);
    
    if (pageNum == HEADER_PAGE_NUMBER){
        HeaderPage* page = (HeaderPage*)&node;
        printf("HeaderPage %llu:\n",pageNum);
        printf("#freePage - %llu\n",page->freePageNumber);
        printf("#rootPage - %llu\n",page->rootPageNumber);
        printf("#pages %llu:\n\n",page->numberOfPage);
        buf_unpin_page(tableID, pageNum);
        return;
    }

    if (node.isLeaf){
        printf("LeafPage %llu:\n",pageNum);
        printf("#parentPage - %llu\n",node.parentPageNumber);
        printf("#keys %d:\n",node.numberOfKeys);
        
        
        LeafPage* leafNode = (LeafPage*)&node;
        printf("#rightSibling %llu:\n",leafNode->rightSiblingPageNumber);
        printf("records - \n");
        for (i=0;i<node.numberOfKeys;i++){
            printf("( %lld, %s ) ",leafNode->records[i].key,leafNode->records[i].value);
        }
        printf("\n\n");
        buf_unpin_page(tableID, pageNum);
        return;
    }
    printf("InternalPage %llu:\n",pageNum);
    printf("#parentPage - %llu\n",node.parentPageNumber);
    printf("#keys %d:\n",node.numberOfKeys);
    
    InternalPage* leafNode = (InternalPage*)&node;
    printf("recordIDs - \n");
    for (i=0;i<node.numberOfKeys;i++){
        printf("(%llu) %lld  ",pageNumOf(leafNode,i),leafNode->recordIDs[i].key);
    }
    printf("(%llu)",pageNumOf(leafNode,i));
    printf("\n\n");
    buf_unpin_page(tableID, pageNum);
}

void printNode(NodePage* node, pagenum_t nodePageNum){
    int i;
    if (nodePageNum == HEADER_PAGE_NUMBER){
        HeaderPage* page = (HeaderPage*)node;
        printf("HeaderPage %llu:\n",nodePageNum);
        printf("#freePage - %llu\n",page->freePageNumber);
        printf("#rootPage - %llu\n",page->rootPageNumber);
        printf("#pages %llu:\n",page->numberOfPage);
        return;
    }
    if (node->isLeaf){
        printf("LeafPage %llu:\n",nodePageNum);
        printf("#parentPage - %llu\n",node->parentPageNumber);
        printf("#keys %d:\n",node->numberOfKeys);
        
        
        LeafPage* leafNode = (LeafPage*)node;
        printf("#rightSibling %llu:\n",leafNode->rightSiblingPageNumber);
        printf("records - \n");
        for (i=0;i<node->numberOfKeys;i++){
            printf("( %lld, %s ) ",leafNode->records[i].key,leafNode->records[i].value);
        }
        printf("\n");
        return;
    }
    printf("InternalPage %llu:\n",nodePageNum);
        printf("#parentPage - %llu\n",node->parentPageNumber);
        printf("#keys %d:\n",node->numberOfKeys);
        
        InternalPage* leafNode = (InternalPage*)node;
        printf("recordIDs - \n");
        for (i=0;i<node->numberOfKeys;i++){
            printf("(%llu) %lld  ",pageNumOf(leafNode,i),leafNode->recordIDs[i].key);
        }
        printf("(%llu)",pageNumOf(leafNode,i));
        printf("\n");
}

/* First message to the user.
 */
void usage_1( void ) {
    printf("B+ Tree of Order leaf: %d, internal: %d.\n", leafOrder, internalOrder);
}


/* Second message to the user.
 */
void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\tp <k> -- Print the path from the root to key k and its associated "
           "value.\n"
    "\tr <k1> <k2> -- Print the keys and values found in the range "
            "[<k1>, <k2>\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tx -- Destroy the whole tree.  Start again with an empty tree of the "
           "same order.\n"
    "\tt -- Print the B+ tree.\n"
    "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
    "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
           "leaves.\n"
    "\tq -- Quit. (Or use Ctl-D.)\n"
    "\t? -- Print this help message.\n");
}

/* Brief usage note.
 */
void usage_3( void ) {
    printf("Usage: ./bpt [<order>]\n");
    printf("\twhere %d <= order <= %d .\n", MIN_ORDER, MAX_ORDER);
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}