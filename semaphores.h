/* semaphores.h */

/* Creates a set of #nsems semaphores. *
 * Initializes them with value val.    *
 * Returns the semaphore set id.       */
int  sem_create_set(int nsems, int val);


/* Delete a semaphore set.                 *
 * `semid` is returned by sem_create_set() */
void sem_delete(int semid);


/* Decrease by 1 the set's `nsem` semaphore value */
void sem_down(int semid, int nsem);


/* Increase by 1 the set's `nsem` semaphore value */
void sem_up(int semid, int nsem);