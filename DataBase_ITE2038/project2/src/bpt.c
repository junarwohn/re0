/*
 *  bpt.c  
 */
#define Version "1.14"
/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 
 *  3. Neither the name of the copyright holder nor the names of its 
 *  contributors may be used to endorse or promote products derived from this 
 *  software without specific prior written permission.
 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 
 *  Author:  Amittai Aviram 
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *  
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 *  Usage:  bpt [order]
 *  where order is an optional argument
 *  (integer MIN_ORDER <= order <= MAX_ORDER)
 *  defined as the maximal number of pointers in any node.
 *
 */

#include "bpt.h"
#include "file.h"

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
int order = DEFAULT_ORDER;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
node * queue = NULL;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = false;

// Get global variable of file descriptor of datafile.
extern FILE * fp_db;

// table count
int table_cnt = 0;

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES

/* Copyright and license notice to user at startup. 
 */
void license_notice( void ) {
    printf("bpt version %s -- Copyright (C) 2010  Amittai Aviram "
            "http://www.amittai.com\n", Version);
    printf("This program comes with ABSOLUTELY NO WARRANTY; for details "
            "type `show w'.\n"
            "This is free software, and you are welcome to redistribute it\n"
            "under certain conditions; type `show c' for details.\n\n");
}


/* Routine to print portion of GPL license to stdout.
 */
void print_license( int license_part ) {
    int start, end, line;
    FILE * fp;
    char buffer[0x100];

    switch(license_part) {
    case LICENSE_WARRANTEE:
        start = LICENSE_WARRANTEE_START;
        end = LICENSE_WARRANTEE_END;
        break;
    case LICENSE_CONDITIONS:
        start = LICENSE_CONDITIONS_START;
        end = LICENSE_CONDITIONS_END;
        break;
    default:
        return;
    }

    fp = fopen(LICENSE_FILE, "r");
    if (fp == NULL) {
        perror("print_license: fopen");
        exit(EXIT_FAILURE);
    }
    for (line = 0; line < start; line++)
        fgets(buffer, sizeof(buffer), fp);
    for ( ; line < end; line++) {
        fgets(buffer, sizeof(buffer), fp);
        printf("%s", buffer);
    }
    fclose(fp);
}


/* First message to the user.
 */
void usage_1( void ) {
    printf("B+ Tree of Order %d.\n", order);
    printf("Following Silberschatz, Korth, Sidarshan, Database Concepts, "
           "5th ed.\n\n"
           "To build a B+ tree of a different order, start again and enter "
           "the order\n"
           "as an integer argument:  bpt <order>  ");
    printf("(%d <= order <= %d).\n", MIN_ORDER, MAX_ORDER);
    printf("To start with input from a file of newline-delimited integers, \n"
           "start again and enter the order followed by the filename:\n"
           "bpt <order> <inputfile> .\n");
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


/* Helper function for printing the
 * tree out.  See print_tree.
 */
void enqueue( node * new_node ) {
    node * c;
    if (queue == NULL) {
        queue = new_node;
        queue->next = NULL;
    }
    else {
        c = queue;
        while(c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
        new_node->next = NULL;
    }
}


/* Helper function for printing the
 * tree out.  See print_tree.
 */
node * dequeue( void ) {
    node * n = queue;
    queue = queue->next;
    n->next = NULL;
    return n;
}


/* Prints the bottom row of keys
 * of the tree (with their respective
 * pointers, if the verbose_output flag is set.
 */
void print_leaves( node * root ) {
    int i;
    node * c = root;

    if (root == NULL) {
        printf("Empty tree.\n");
        return;
    }
    while (!c->is_leaf)
        c = c->pointers[0];
    while (true) {
        for (i = 0; i < c->num_keys; i++) {
            if (verbose_output)
                printf("%lx ", (unsigned long)c->pointers[i]);
            printf("%d ", c->keys[i]);
        }
        if (verbose_output)
            printf("%lx ", (unsigned long)c->pointers[order - 1]);
        if (c->pointers[order - 1] != NULL) {
            printf(" | ");
            c = c->pointers[order - 1];
        }
        else
            break;
    }
    printf("\n");
}


/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int height( node * root ) {
    int h = 0;
    node * c = root;
    while (!c->is_leaf) {
        c = c->pointers[0];
        h++;
    }
    return h;
}


/* Utility function to give the length in edges
 * of the path from any node to the root.
 */
int path_to_root( node * root, node * child ) {
    int length = 0;
    node * c = child;
    while (c != root) {
        c = c->parent;
        length++;
    }
    return length;
}


/* Prints the B+ tree in the command
 * line in level (rank) order, with the 
 * keys in each node and the '|' symbol
 * to separate nodes.
 * With the verbose_output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */
void print_tree( node * root ) {

    node * n = NULL;
    int i = 0;
    int rank = 0;
    int new_rank = 0;

    if (root == NULL) {
        printf("Empty tree.\n");
        return;
    }
    queue = NULL;
    enqueue(root);
    while( queue != NULL ) {
        n = dequeue();
        if (n->parent != NULL && n == n->parent->pointers[0]) {
            new_rank = path_to_root( root, n );
            if (new_rank != rank) {
                rank = new_rank;
                printf("\n");
            }
        }
        if (verbose_output) 
            printf("(%lx)", (unsigned long)n);
        for (i = 0; i < n->num_keys; i++) {
            if (verbose_output)
                printf("%lx ", (unsigned long)n->pointers[i]);
            printf("%d ", n->keys[i]);
        }
        if (!n->is_leaf)
            for (i = 0; i <= n->num_keys; i++)
                enqueue(n->pointers[i]);
        if (verbose_output) {
            if (n->is_leaf) 
                printf("%lx ", (unsigned long)n->pointers[order - 1]);
            else
                printf("%lx ", (unsigned long)n->pointers[n->num_keys]);
        }
        printf("| ");
    }
    printf("\n");
}

int open_table(char *pathname) {
    if ((fp_db = fopen(pathname, "r+")) != NULL)
        return table_cnt++; 
    return -1;
};

/* Finds the record under a given key and prints an
 * appropriate message to stdout.
 */
void find_and_print(node * root, int key, bool verbose) {
    Record * r = find(root, key, verbose);
    if (r == NULL)
        printf("Record not found under key %d.\n", key);
    else 
        printf("Record at %lx -- key %d, value %d.\n",
                (unsigned long)r, key, r->value);
}


/* Finds and prints the keys, pointers, and values within a range
 * of keys between key_start and key_end, including both bounds.
 */
void find_and_print_range( node * root, int key_start, int key_end,
        bool verbose ) {
    int i;
    int array_size = key_end - key_start + 1;
    int returned_keys[array_size];
    void * returned_pointers[array_size];
    int num_found = find_range( root, key_start, key_end, verbose,
            returned_keys, returned_pointers );
    if (!num_found)
        printf("None found.\n");
    else {
        for (i = 0; i < num_found; i++)
            printf("Key: %d   Location: %lx  Value: %d\n",
                    returned_keys[i],
                    (unsigned long)returned_pointers[i],
                    ((Record *)
                     returned_pointers[i])->value);
    }
}


/* Finds keys and their pointers, if present, in the range specified
 * by key_start and key_end, inclusive.  Places these in the arrays
 * returned_keys and returned_pointers, and returns the number of
 * entries found.
 */
int find_range( node * root, int key_start, int key_end, bool verbose,
        int returned_keys[], void * returned_pointers[]) {
    int i, num_found;
    num_found = 0;
    node * n = find_leaf( root, key_start, verbose );
    if (n == NULL) return 0;
    for (i = 0; i < n->num_keys && n->keys[i] < key_start; i++) ;
    if (i == n->num_keys) return 0;
    while (n != NULL) {
        for ( ; i < n->num_keys && n->keys[i] <= key_end; i++) {
            returned_keys[num_found] = n->keys[i];
            returned_pointers[num_found] = n->pointers[i];
            num_found++;
        }
        n = n->pointers[order - 1];
        i = 0;
    }
    return num_found;
}


/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
pagenum_t find_leaf(int key, bool verbose) {
    int i = 0;
    InternalPage c;    
    pagenum_t lpn;

    if (c == NULL) {
        perror("Header Page creation.");
        exit(EXIT_FAILURE);
    }

    // Set c as Root Page
    lpn = ((HeaderPage) c).rpn;
    file_read_page(lpn, &c);

    while (!c.is_leaf) {
        if (verbose) {
            printf("[");
            for (i = 0; i < c.kcnt - 1; i++)
                printf("%d ", c.records[i].key);
            printf("%d] ", c.records[i].key);
        }
        i = 0;
        while (i < c.kcnt) {
            if (key >= c.records[i].key) i++;
            else break;
        }
        if (verbose)
            printf("%d ->\n", i);
        lpn = c.records[i].key;
        file_read_page(lpn, &c);
    }

    if (verbose) {
        printf("Leaf [");
        for (i = 0; i < c.kcnt - 1; i++)
            printf("%d ", c.records[i].key);
        printf("%d] ->\n", c.records[i].key);
    }
    return lpn;
}


/* Finds and returns the record to which
 * a key refers.
 */
Record * find( node * root, int key, bool verbose ) {
    int i = 0;
    node * c = find_leaf( root, key, verbose );
    if (c == NULL) return NULL;
    for (i = 0; i < c->num_keys; i++)
        if (c->keys[i] == key) break;
    if (i == c->num_keys) 
        return NULL;
    else
        return (Record *)c->pointers[i];
}

//Record * find( node * root, int key, bool verbose ) {
int db_find(int64_t key, char *ret_val) {
    int i = 0;
    pagenum_t lpn = find_leaf(key, verbose);
    LeafPage c;
    file_read_page(lpn, &c);

    if (c.is_leaf != 1) return -1;
    for (i = 0; i < c.kcnt; i++)
        if (c.records[i].key == key) break;
    if (i == c.kcnt) 
        return -1;
    else {
        strcpy(ret_val, c.records[i].value);
        return 0;
    }

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


// INSERTION


/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
pagenum_t make_intl( void ) {

    pagenum_t new_ipn;
    InternalPage new_ip;
    new_ipn = file_alloc_page();
    file_read_page(new_ipn, &new_ip);
    new_ip.is_leaf = false;
    new_ip.kcnt = 0;
    new_ip.ppn = 0;
    new_ip.lspn = 0;
    file_write_page(new_ipn, &new_ip);
    return new_ipn;
}


/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
pagenum_t make_leaf( void ) {
    LeafPage lp;
    pagenum_t lpn = file_alloc_page();
    lp.ppn = 0;
    lp.is_leaf = true;
    lp.kcnt = 0;
    lp.rspn = 0;
    file_write_page(lpn, &lp);
    return lpn;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */
int get_left_index(pagenum_t ppn, pagenum_t left_pn) {

    int left_index = 0;
    InternalPage pp;
    file_read_page(ppn, &pn);
    // Cross the index
    if (pp.lspn == left_pn)
        return left_index;
    left_index++;
    while (left_index <= pp->kcnt && 
            pp.records[left_index - 1].pn != left_pn)
        left_index++;
    return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */

int insert_into_leaf(pagenum_t lpn, int key, char * value) {

    int i, insertion_point;
    LeafPage lp;
    file_read_page(lpn, &lp);
    insertion_point = 0;
    while (insertion_point < lp.kcnt && lp.records[insertion_point].key < key)
        insertion_point++;

    for (i = lp.kcnt; i > insertion_point; i--) {
        lp.records[i].key = leaf.records[i - 1].key;
        strcpy(lp.records[i].value, lp.records[i - 1].value);
    }
    lp.records[insertion_point].key = key;
    strcpy(lp.records[insertion_point].value, value);
    lp.kcnt++;
    file_write_page(lpn, &lp);
    return 0;
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
node * insert_into_leaf_after_splitting(pagenum_t lpn, int key, char * value) {

    pagenum_t new_lpn;
    LeafPage lp;
    LeafPage new_lp;
    Record * temp_records;
    Record null_record;
    int insertion_index, split, new_key, i, j;

    file_read_page(lpn, &lp);

    new_lpn = make_leaf();
    file_read_page(new_lpn, &new_lp);

    temp_records = (Records *)malloc(order * sizeof(Records));

    if (temp_records == NULL) {
        perror("Temporary records array.");
        exit(EXIT_FAILURE);
    }

    insertion_index = 0;
    while (insertion_index < order - 1 && lp.records[insertion_index].key < key)        
        insertion_index++;

    for (i = 0, j = 0; i < lp.kcnt; i++, j++) {
        if (j == insertion_index) j++;
        temp_records[i] = lp.records[i];
    }

    temp_records[insertion_index].key = key;
    strcpy(temp_records[insertion_index].value, value);

    lp.kcnt = 0;

    split = cut(order - 1);

    for (i = 0; i < split; i++) {
        lp.records[i] = temp_records[i];
        lp.kcnt++;
    }

    for (i = split, j = 0; i < order; i++, j++) {
        new_lp.records[j] = temp_records[i];
        new_lp.kcnt++;
    }

    free(temp_records);

    new_lp.rspn = lp.rspn;
    lp.rspn = new_lpn;

    for (i = lp.kcnt; i < order - 1; i++)
        lp.records[i] = null_record;
    for (i = new_lp.kcnt; i < order - 1; i++)
        new_lp.records[i] = null_record;

    new_lp->ppn = lp->ppn;
    new_key = new_lp->record[0].key;

    return insert_into_parent(lpn, new_key, new_lpn);
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
pagenum_t insert_into_intl(pagenum_t pn, int left_index, int key, pagenum_t right_pn);
//node * insert_into_node(node * root, node * n, 
//        int left_index, int key, node * right) {
    int i;
    InternalPage ip;
    file_read_page(pn, &ip);

    for (i = ip.kcnt; i > left_index; i--) {
        ip.records[i] = ip.records[i - 1];
    }
    ip.records[left_index].key = key;
    ip.records[left_index].pn = right_pn;
    ip.kcnt++;
    file_write_page(pn, &ip);
    return pn;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
pagenum_t insert_into_intl_after_splitting(pagenum_t ppn, int left_index, int key, pagenum_t right_pn) {

    int i, j, split, k_prime;
    InternalPage old_ip;
    pagenum_t new_ipn, child_pn;
    InternalPage new_ip, child_p;
    int * temp_keys;
    int * temp_pns;

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    temp_pns = malloc( (order + 1) * sizeof(int));
    if (temp_pointers == NULL) {
        perror("Temporary pointers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    temp_keys = malloc( order * sizeof(int) );
    if (temp_keys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    file_read_page(ppn, &old_ip);

    temp_pns[0] = old_ip.lspn;
    for (i = 0, j = 1; i < old_ip.kcnt + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pns[j] = old_ip.records[i].pn;
    }

    for (i = 0, j = 0; i < old_ip.kcnt; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_ip.records[i].key;
    }

    temp_pointers[left_index + 1] = right;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  
    split = cut(order);
    new_ipn = make_intl();
    file_read_page(new_ipn, &new_ip);
    old_ip.kcnt = 0;
    old_ip.lspn = temp_pns[0];
    old_ip.kcnt++;
    for (i = 0; i < split - 1; i++) {
        old_ip.records[i].key = temp_keys[i];
        old_ip.records[i].pn = temp_pns[i + 1];
        old_ip.kcnt++;
    }

    old_node->pointers[i] = temp_pointers[i];

    k_prime = temp_keys[split - 1];
    new_ip.lspn = temp_pointers[i];
    for (++i, j = 0; i < order; i++, j++) {
        new_ip.records[j].pn = temp_pns[i];
        new_ip.records[j].key = temp_keys[i];
        new_ip.kcnt++;
    }
    free(temp_pns);
    free(temp_keys);
    new_ip.ppn = old_ip.ppn;
    for (i = 0; i <= new_ip.kcnt; i++) {
        child_pn = new_ip.records[i].pn;
        file_read_page(child_pn, &child_p);
        child_p.ppn = new_ipn;
        file_write_page(child_pn, &child_p);
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    file_write_page(new_ipn, &new_ip);
    file_write_page(ppn, &old_ip);
    return insert_into_parent(old_ipn, k_prime, new_ipn);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
int insert_into_parent(pagenum_t left_pn, int key, pagenum_t right_pn);
//int insert_into_parent(node * left, int key, node * right) {

    int left_index;
    pagenum_t ppn;
    InternalPage pp;
    page_t left_p, right_p;
    file_read_page(left_pn, &left_p);
    ppn = left_p.ppn;

    /* Case: new root. */

    if (ppn == 0)
        return insert_into_new_root(left_pn, key, right_pn);

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's pointer to the left 
     * node.
     */

    left_index = get_left_index(ppn, left_pn);


    /* Simple case: the new key fits into the node. 
     */

    if (pp.kcnt < order - 1)
        //return insert_into_node(root, parent, left_index, key, right);
        return insert_into_intl(ppn, left_index, key, right_pn);

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */

    return insert_into_intl_after_splitting(ppn, left_index, key, right_pn);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
pagenum_t insert_into_new_root(pagenum_t left_pn, int key, pagenum_t right_pn) {

    pagenum_t rpn = make_node();
    InternalPage rp;
    LeafPage left_p, right_p;
    file_read_page(rpn, &rp);
    file_read_page(left_pn, &left_p);
    file_read_page(right_pn, &right_p);
    rp.lspn = left_pn;
    rp.records[0].key = key;
    rp.records[0].pn = right_pn;
    rp.kcnt++;
    rp.ppn = 0;
    left_p.ppn = rpn;
    right_p.ppn = rpn;
    file_write_page(rpn, &rp);
    file_write_page(left_pn, &left_p);
    file_write_page(right_pn, &right_p);
    return rpn;
}



/* First insertion:
 * start a new tree.
 */
pagenum_t start_new_tree(int key, char * value) {

    HeaderPage hp;
    LeafPage lp;
    pagenum_t lpn = make_leaf();
    file_read_page(0, &hp);
    file_read_page(lpn, &lp);
    hp.rpn = lpn;
    lp.records[0].key = key;
    strcpy(lp.records[0].value, value);
    lp.ppn = 0;
    lp.kcnt = 1;
    file_write_page(0, &hp);
    file_write_page(lpn, &lp);
    return lpn;
}


//node * insert( node * root, int key, int value ) {
int db_insert(int64_t key, char* value) {

    //Record * pointer;
    HeaderPage hp;
    pagenum_t lpn;
    LeafPage lp;
    
    char ret_val[120];

    /* Does not accept duplicated key. 
     * Ignore input. 
     */
    if (db_find(key, ret_val) == 0) {
        return 0;
    }
    /* Create a new record for the
     * value.
     */

    // Set file pointer at start
    file_read_page(0, &hp);

    /* Case: No page under header page.
     * Make New Page
     */
    if (hp.rpn == 0) 
        // care return value
        return start_new_tree(key, value);


    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    lpn = find_leaf(key, false);
    file_read_page(lpn, &lp);

    /* Case: leaf has room for key and pointer.
     */

    if (lp->kcnt < order - 1) {
        leaf = insert_into_leaf(lpn, key, value);
        return 0;
    }


    /* Case:  leaf must be split.
     */

    return insert_into_leaf_after_splitting(lpn, key, value);
}


// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( node * n ) {

    int i;

    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
    for (i = 0; i <= n->parent->num_keys; i++)
        if (n->parent->pointers[i] == n)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, int key, node * pointer) {

    int i, num_pointers;

    // Remove the key and shift other keys accordingly.
    i = 0;
    while (n->keys[i] != key)
        i++;
    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
    while (n->pointers[i] != pointer)
        i++;
    for (++i; i < num_pointers; i++)
        n->pointers[i - 1] = n->pointers[i];


    // One key fewer.
    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < order - 1; i++)
            n->pointers[i] = NULL;
    else
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;

    return n;
}


node * adjust_root(node * root) {

    node * new_root;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_keys > 0)
        return root;

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        new_root = root->pointers[0];
        new_root->parent = NULL;
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else
        new_root = NULL;

    free(root->keys);
    free(root->pointers);
    free(root);

    return new_root;
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!n->is_leaf) {

        /* Append k_prime.
         */

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;


        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor->pointers[i] = n->pointers[j];

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */

    else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
        neighbor->pointers[order - 1] = n->pointers[order - 1];
    }

    root = delete_entry(root, n->parent, k_prime, n);
    free(n->keys);
    free(n->pointers);
    free(n); 
    return root;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, 
        int k_prime_index, int k_prime) {  

    int i;
    node * tmp;

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!n->is_leaf) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {  
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!n->is_leaf)
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n->num_keys++;
    neighbor->num_keys--;

    return root;
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
node * delete_entry( node * root, node * n, int key, void * pointer ) {

    int min_keys;
    node * neighbor;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, pointer);

    /* Case:  deletion from the root. 
     */

    if (n == root) 
        return adjust_root(root);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    // Delayed Merge
    // if (n->num_keys >= min_keys)
    if (n->num_keys > 0)
        return root;

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

    neighbor_index = get_neighbor_index( n );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = n->parent->keys[k_prime_index];
    neighbor = neighbor_index == -1 ? n->parent->pointers[1] : 
        n->parent->pointers[neighbor_index];

    capacity = n->is_leaf ? order : order - 1;

    /* Coalescence. */

    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */

    else
        return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
 */
node * delete(node * root, int key) {

    node * key_leaf;
    Record * key_record;

    key_record = find(root, key, false);
    key_leaf = find_leaf(root, key, false);
    if (key_record != NULL && key_leaf != NULL) {
        root = delete_entry(root, key_leaf, key, key_record);
        free(key_record);
    }
    return root;
}


void destroy_tree_nodes(node * root) {
    int i;
    if (root->is_leaf)
        for (i = 0; i < root->num_keys; i++)
            free(root->pointers[i]);
    else
        for (i = 0; i < root->num_keys + 1; i++)
            destroy_tree_nodes(root->pointers[i]);
    free(root->pointers);
    free(root->keys);
    free(root);
}


node * destroy_tree(node * root) {
    destroy_tree_nodes(root);
    return NULL;
}

