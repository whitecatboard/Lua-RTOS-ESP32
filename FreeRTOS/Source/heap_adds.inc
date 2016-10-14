/*
 * Whitecat, heap adds for whitecat, base implementation for Realloc and Calloc
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

void *pvPortRealloc(void *pv, size_t size) {
    void *result;
    size_t bsize;

    if (size == 0) {
        vPortFree(pv);
        return NULL;
    }

    uint8_t *puc = ( uint8_t * ) pv;
    BlockLink_t *pxLink;

    if( pv != NULL ) {
        puc -= xHeapStructSize;
        pxLink = ( void * ) puc;

        // Get block size
        bsize = pxLink->xBlockSize;
        bsize &= ~xBlockAllocatedBit;

        result = (void *) pvPortMalloc(size);
        if (!result)
            return NULL;

        if (size > bsize) {
            bcopy(pv, result, bsize);
        } else {
            bcopy(pv, result, size);
        }

        vPortFree(pv);
    } else {
        result = pvPortMalloc(size);
    }

    return result;
}

void *pvPortCalloc(size_t items, size_t size) {
   void *result;

   result = (void *) pvPortMalloc(items * size);
   if (!result)
       return NULL;

   bzero(result, items * size);

   return result;
}

UBaseType_t uxPortAllocated(void *pv) {
    uint8_t *puc = ( uint8_t * ) pv;
    BlockLink_t *pxLink;

    if( pv != NULL ) {
        puc -= xHeapStructSize;

        pxLink = ( void * ) puc;

        return (((pxLink->xBlockSize & xBlockAllocatedBit) != 0) && (pxLink->pxNextFreeBlock == NULL));
    }
    
    return 0;
}
