/* semaphores.c */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>

#include "semaphores.h"

union senum /* Union required from semctl */
{
  int val;
  struct semid_ds *buff;
  unsigned short *array;
};

/* =================================================== */
int sem_create_set(int nsems, int val)
{
  int semid;
  if ((semid = semget(IPC_PRIVATE, nsems, IPC_CREAT | 0660 )) == -1)
  {
    perror("sem_create_set -> semget");
    exit(EXIT_FAILURE);
  }

  union senum arg;
  arg.val = val;
  for (int i = 0; i < nsems; ++i)
  {
    if (semctl(semid, i, SETVAL, arg) == -1)
    {
      perror("sem_create_set -> semctl");
      exit(EXIT_FAILURE);
    }
  }
  return semid;
}

/* =================================================== */
void sem_down(int semid, int nsem)
{
  struct sembuf sb;
  sb.sem_num = nsem;
  sb.sem_op = -1;
  sb.sem_flg = 0;

  if (semop(semid, &sb, 1) == -1)
  {
    perror("sem_down -> semop");
    exit(EXIT_FAILURE);
  }
}

/* =================================================== */
void sem_up(int semid, int nsem)
{
  struct sembuf sb;
  sb.sem_num = nsem;
  sb.sem_op = 1;
  sb.sem_flg = 0;

  if (semop(semid, &sb, 1) == -1)
  {
    perror("sem_up -> semop");
    exit(EXIT_FAILURE);
  }
}

/* =================================================== */
/* Deletes a set of semaphores */
void sem_delete(int semid)
{
  if (semctl(semid, 105, IPC_RMID) == -1) // 105 is ignored
  {
    perror("sem_delete -> semctl");
    exit(EXIT_FAILURE);
  }
}
/* =================================================== */