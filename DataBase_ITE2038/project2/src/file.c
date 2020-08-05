/*
 * =====================================================================================
 *
 *       Filename:  file.c
 *
 *    Description:  Following architecture of a DBMS,
 *                  this corresponds to Disk(File) Space Management.
 *
 *        Version:  1.0
 *        Created:  08/01/20 01:54:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joonho Wohn
 *   Organization:  
 *
 * =====================================================================================
 */
#include "file.h"

pagenum_t file_alloc_page(void) {
    HeaderPage hp;
    FreePage fp;
    pagenum_t fpn;
    file_read_page(0, &hp);
    // When Free Page exist
    if (hp.fpn != 0) {
        fpn = hp.fpn;
        file_read_page(fpn, &fp);
        hp.fpn = fp.nfpn;
        file_write_page(0, &hp);
    // When no Free Page left
    // Create new free page
    } else {
        fp.nfpn = 0;
        // Setting new free page number as number of pages
        fpn = hp.pcnt++;
        hp.fpn = fpn;
        file_write_page(fpn, &fp);
        file_write_page(0, &hp);
    }
    return fpn;
}

void file_free_page(pagenum_t pagenum);

void file_read_page(pagenum_t pagenum, page_t* dest) {
    fseek(fp_db, pagenum * sizeof(page_t), SEEK_SET);
    fread(dest, sizeof(page_t), 1, fp_db);
}

void file_write_page(pagenum_t pagenum, const page_t* src) {
    fseek(fp_db, pagenum * sizeof(page_t), SEEK_SET);
    fwrite(src, sizeof(page_t), 1, fp_db);
    fflush(fp_db);
}
