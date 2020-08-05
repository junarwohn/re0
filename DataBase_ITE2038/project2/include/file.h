#ifndef __FILE_H__
#define __FILE_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

extern FILE * fp_db;

/* Subelement of Page */

typedef uint64_t pagenum_t;

typedef struct _page_t {
    char rsvd[4096];
} page_t;


typedef struct _hdr_page {
    union {
        struct {
            int fpn;        // Free Page Number
            int rpn;        // Root Page Number
            int pcnt;       // Page Count (Number of Page). Modified in file layer
        };
        page_t rsvd;
    };
} HeaderPage;

typedef struct _free_page {
    union {
        int nfpn;   // Next Free Page Number
        page_t rsvd;
    };
} FreePage;

typedef struct _intl_page {
    union {
        struct {
            int ppn;              // Next Free Page Number or Parent Page Number
            bool is_leaf;
            unsigned char kcnt;     // Key Count (Number of Keys).
        };
        char rsvd[120];
    };
    int lspn;      // Left Most Sibling Page Number
    struct {
        int key;
        int pn;
    } records[248];
} InternalPage;

typedef struct _leaf_page {
    union {
        struct {
            int ppn;              // Next Free Page Number or Parent Page Number
            bool is_leaf;
            unsigned char kcnt;     // Key Count (Number of Keys). 
        };
        char rsvd[120];
    };
    int rspn;      // Right Sibling Page Number
    struct {
        int key;
        char value[120];
    } records[31];
} LeafPage;

FILE * fp_db;

pagenum_t file_alloc_page(void);

void file_free_page(pagenum_t pagenum);

void file_read_page(pagenum_t pagenum, page_t* dest);

void file_write_page(pagenum_t pagenum, const page_t* src);

#endif /* __FILE_H__*/
