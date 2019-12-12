/* coordinator.c */

#include <errno.h>
#include <math.h> // log
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "semaphores.h"
#include "shared_mem.h"

#define FACTOR 10e4 /* Used to generate random microseconds */

struct entry_t
{
  short value;   /* Supposed data stored    */
  long reads;    /* Sum of read operations  */
  long writes;   /* Sum of write operations */
  long curr_rdr; /* Current readers num of the entry */
};

/* Sleeps for a random number of microseconds.        *
 * μsecs are generated using exponential distribution */
void sleep_exp_time(void)
{
  unsigned int usecs = (-log(1.0 * rand() / RAND_MAX) / FACTOR * 10e6);
  usleep(usecs);
}


int main(int argc, char const *argv[])
{
  if (argc != 5)
  {
    fprintf(stderr, ">ERROR: 4 arguments required.\n");
    exit(EXIT_FAILURE);
  }

  int total_entries = atoi(argv[1]);
  int peers = atoi(argv[2]);    /* Processes          */
  int loops = atoi(argv[3]);    /* Loops per process  */
  int ratio = atoi(argv[4]);    /* Readers per writer */

  printf("\nTotal Entries: %d\nPeers: %d\nLoops: %d\n\
Reader/Writer ratio: %d%%\n", total_entries, peers, loops, ratio);
  fflush(stdout);

  /* Create 2 sets of semaphores:  *
   * Counting ones for the readers *
   * Binary ones for the writers   */
  int rd_mutex = sem_create_set(total_entries, 1);
  int wrt_sem = sem_create_set(total_entries, 1);

  /* Create and attach the shared memory segment */
  int shmid = shm_create(total_entries * sizeof(struct entry_t));
  struct entry_t *array = (struct entry_t *) shm_attach(shmid);
  
  for (int i = 0; i < total_entries; ++i) /* Initialize entries */
  {
    array[i].value = rand() % 20;
    array[i].reads = array[i].writes = array[i].curr_rdr = 0;
  }
  
  printf("\n>Child process stats:\n");
  fflush(stdout);

  /* Spawn the desired number of processes */
  for (int i = 0; i < peers; ++i)
  {
    int pid = fork();

    if (pid == 0)   /* Child process */
    {
      srand(time(NULL) % getpid());

      long rd_sum  = 0;      /* Local counters to track read/write attempts */
      long wrt_sum = 0;
      long double ttime = 0; /* Time spent waiting for resources to be available */

      struct timespec start;   /* Starting/ending points to track the time */
      struct timespec end;

      /* Perform # loops for each process */
      for (int j = 0; j < loops; ++j)
      {
        int entr = rand() % total_entries; /* Choose an entry to operate on */
        int attr = rand() % 101;  /* Choose whether to read or write */
        
        if (attr <= ratio)  /* Reader */
        {
          ++rd_sum;

          /* Time the request for the resource was made */
          clock_gettime(CLOCK_MONOTONIC, &start);

          sem_down(rd_mutex, entr); /* Gains access to the resource */

          /* Time the request was approved */
          clock_gettime(CLOCK_MONOTONIC, &end);

          long sec = end.tv_sec - start.tv_sec; 
          long ns = end.tv_nsec - start.tv_nsec; 
          if (start.tv_nsec > end.tv_nsec) /* Clock underflow */
          {
            --sec; 
            ns += 1000000000; 
          }
          ttime += (long double)(sec * 10e9 + ns); /* Convert time to ns */

          ++array[entr].curr_rdr;   /* Keep track of total readers */

          if (array[entr].curr_rdr == 1) /* Block writers */
            sem_down(wrt_sem, entr);

          sem_up(rd_mutex, entr);
          
          ++array[entr].reads;
          sleep_exp_time(); /* Generate time using the resource */ 

          sem_down(rd_mutex, entr);
          
          --array[entr].curr_rdr;
          if (array[entr].curr_rdr == 0) /* Allow writers */
            sem_up(wrt_sem, entr);

          sem_up(rd_mutex, entr); /* Release the resource */
        }
        else /* Writer */
        {
          ++wrt_sum;

          /* Time the request for the resource was made */
          clock_gettime(CLOCK_MONOTONIC, &start);

          sem_down(wrt_sem, entr);  /* Gains access to the resource */

          /* Time the request was approved */
          clock_gettime(CLOCK_MONOTONIC, &end); 
          
          long sec = end.tv_sec - start.tv_sec; 
          long ns = end.tv_nsec - start.tv_nsec; 
          if (start.tv_nsec > end.tv_nsec)  /* Clock underflow  */
          {
            --sec; 
            ns += 1000000000; 
          }
          ttime += (long double)(sec * 10e9 + ns); /* Convert time to ns */

          ++array[entr].writes;
          sleep_exp_time(); /* Generate time using the resource */
          
          sem_up(wrt_sem, entr); /* Release the resource */
        }
      }
      /* Print process stats */
      printf("\033[0;36mPID:\033[0m %4d, \033[0;32mReads: \033[0m%-3ld, \033\
[0;31mWrites: \033[0m%-3ld, \033[0;33mAverage time: \033[0m%7.0Lf nsec\n",
            getpid(), rd_sum, wrt_sum, ttime/(rd_sum + wrt_sum));

      exit(EXIT_SUCCESS); /* Child exits */
    }
    else if (pid < 0)
    {
      perror("fork");
      exit(EXIT_FAILURE);
    }
  }

  /* Caller function executes from here */
  for (int i = 0; i < peers; ++i)
  {
    int status;
    if (wait(&status) == -1)  /* Wait for every child to terminate */
    {
      perror("wait");
      exit(EXIT_FAILURE);
    }                    /* Error check */
    if (status != EXIT_SUCCESS)
    {
      perror("Error in child process");
      exit(EXIT_FAILURE);
    }
  }
  /* Extract and output shared memory stats */
  long ent_reads  = 0; /* Total entries reads/writes */
  long ent_writes = 0;
  for (int i = 0; i < total_entries; ++i)
  {
    ent_reads  += array[i].reads;
    ent_writes += array[i].writes;
  }
  printf("\n>Caller process: shared memory stats\n");
  printf("Total \033[0;32mReads\033[0m : %3ld\n",   ent_reads);
  printf("Total \033[0;31mWrites\033[0m: %3ld\n\n", ent_writes);

  shm_detach(array);    /* Destroy shared mem segment */
  shm_clean(shmid);

  sem_delete(rd_mutex); /* Destroy semaphores */
  sem_delete(wrt_sem);

  exit(EXIT_SUCCESS);   /* Parent process exits */
}
