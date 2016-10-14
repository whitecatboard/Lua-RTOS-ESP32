/*------------------------------------------------------------------------*/
/* Sample code of OS dependent controls for FatFs                         */
/* (C)ChaN, 2014                                                          */
/*------------------------------------------------------------------------*/

#include <time.h>
#include <stdlib.h>
#include <sys/fat/ff.h>

DWORD get_fattime (void) {
    struct tm *info;
    time_t now;
    
    // Get current time
    now = time(NULL);
    info = localtime(&now);

    return (((info->tm_year + 1900 - 1980) & 0b1111111) << 25) |
           (((info->tm_mon + 1) & 0b1111) << 21) | 
           (((info->tm_mday) & 0b11111) << 16) | 
           ((info->tm_hour & 0b11111) << 11) | 
           ((info->tm_min & 0b11111) << 5) | 
           (((info->tm_sec / 2) & 0b11111) << 0);
}

#if _FS_REENTRANT
/*------------------------------------------------------------------------*/
/* Create a Synchronization Object										  */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to create a new
/  synchronization object, such as semaphore and mutex. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_cre_syncobj (	/* !=0:Function succeeded, ==0:Could not create due to any error */
	BYTE vol,			/* Corresponding logical drive being processed */
	_SYNC_t *sobj		/* Pointer to return the created sync object */
)
{
    mtx_init(sobj,NULL,NULL,0);

	return 1;
}



/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_cre_syncobj function. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_del_syncobj (	/* !=0:Function succeeded, ==0:Could not delete due to any error */
	_SYNC_t sobj		/* Sync object tied to the logical drive to be deleted */
)
{
    mtx_destroy(&sobj);

	return 1;
}



/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_req_grant (	/* 1:Got a grant to access the volume, 0:Could not get a grant */
	_SYNC_t sobj	/* Sync object to wait */
)
{
    mtx_lock(&sobj);
    
	return 1;
}



/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
*/

void ff_rel_grant (
	_SYNC_t sobj	/* Sync object to be signaled */
)
{
    mtx_unlock(&sobj);
}

#endif




#if _USE_LFN == 3	/* LFN with a working buffer on the heap */
/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/
/* If a NULL is returned, the file function fails with FR_NOT_ENOUGH_CORE.
*/

void* ff_memalloc (	/* Returns pointer to the allocated memory block */
	UINT msize		/* Number of bytes to allocate */
)
{
	return malloc(msize);	/* Allocate a new memory block with POSIX API */
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree (
	void* mblock	/* Pointer to the memory block to free */
)
{
	free(mblock);	/* Discard the memory block with POSIX API */
}

#endif
