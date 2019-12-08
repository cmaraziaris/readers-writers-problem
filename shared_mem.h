/* shared_mem.h */

#include <sys/types.h> // size_t

/* Creates a shared mem segment with `size` bytes *
 * Returns an identifier to this segment.         */
int   shm_create(size_t size);


/* Deallocates space allocated for a shared mem segment *
 * `shmid` is returned by shm_create()                  */
void  shm_clean(int shmid);


/* Attaches shared mem segment with id `shmid` to a process. *
 * Returns a pointer to the allocated space.                 */
void* shm_attach(int shmid);


/* Detaches a shared mem segment from a process *
 * `addr` is returned by shm_attach()           */
void  shm_detach(void *addr);