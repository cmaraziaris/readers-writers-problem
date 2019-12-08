/* shared_mem.c */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#include "shared_mem.h"

/* ============================================== */
int shm_create(size_t size)
{
  int sid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0660);

  if (sid == -1)
  {
    perror("shm_create -> shmget");
    exit(EXIT_FAILURE);
  }
  return sid;
}

/* ============================================== */
void *shm_attach(int shmid)
{
  void *p = shmat(shmid, NULL, 0);
  if (p == (void *) -1)
  {
    perror("shm_attach -> shmat");
    exit(EXIT_FAILURE);
  }
  return p;
}

/* ============================================== */
void shm_detach(void *addr)
{
  if (shmdt(addr) == -1)
  {
    perror("shm_detach -> shmdt");
    exit(EXIT_FAILURE);
  }
}

/* ============================================== */
void shm_clean(int shmid)
{
  if (shmctl(shmid, IPC_RMID, 0) == -1)
  {
    perror("shm_clean -> shmctl");
    exit(EXIT_FAILURE);
  }
}

/* ============================================== */