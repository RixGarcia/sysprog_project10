#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include "binary_sem.h"
#include "semun.h"

// block of shared memory
struct shmseg
{
  int counter;
  int board[3][3];
};

// i think win functions need to be only for player 1
// return 1 - row win not found
// return 0 - row win found
int rowWin(struct shmseg *smap)
{
  int i;
  
  for(i = 0; i < 3; i++)
    {
      // row win found
      if(smap->board[i][0] == 1 && smap->board[i][0] == smap->board[i][1] && smap->board[i][1] == smap->board[i][2])
	{
	  return 0;
	}
    }
  // row win not found
  return 1;
}

int columnWin(struct shmseg *smap)
{
  int i;

  for(i = 0; i < 3; i++)
    {
      // column win found
      if(smap->board[0][i] == 1 && smap->board[0][i] == smap->board[1][i] && smap->board[1][i] == smap->board[2][i])
	{
	  return 0;
	}
    }
  // column win not found
  return 1;
}

int diagonalWin(struct shmseg *smap)
{
  // top left to bottom right diagonal win
  if(smap->board[0][0] == 1 && smap->board[0][0] == smap->board [1][1] && smap->board[1][1] == smap->board[2][2])
    {
      return 0;
    }
  
  // bottom left to top right diagonal win
  if(smap->board[2][0] == 1 && smap->board[2][0] == smap->board[1][1] && smap->board[1][1] == smap->board[0][2])
    {
      return 0;
    }
  
  return 1;
}

int rowBlock(struct shmseg *smap)
{
  int i;

  for(i = 0; i < 3; i++)
    {
      if(smap->board[i][0] == 1 && smap->board[i][1] == 1)
	{
	  // right block
	}
      if(smap->board[i][0] == 1 && smap->board[i][2] == 1)
	{
	  // middle block
	}
      if (smap->board[i][1] == 1 && smap->board[i][2] == 1)
	{
	  // left block
	}
    }
}

int columnBlock(struct shmseg *smap)
{
    int i;

  for(i = 0; i < 3; i++)
    {
      if(smap->board[0][i] == 1 && smap->board[1][i] == 1)
	{
	  // bottom block
	}
      if(smap->board[0][i] == 1 && smap->board[2][i] == 1)
	{
	  // middle block
	}
      if (smap->board[1][i] == 1 && smap->board[2][i] == 1)
	{
	  // top block
	}
    }
}

int diagonalBlock(struct shmseg *smap)
{
  // top left to bottom right block
  if(smap->board[0][0] == 'X' && smap->board[1][1] == 'X')
    {
      // bottom right block
    }
  if(smap->board[0][0] == 'X' && smap->board[2][2] == 'X')
    {
      // center block
    }
  if(smap->board[1][1] == 'X' && smap->board[2][2] == 'X')
    {
      // top left block
    }

  // top right to bottom left block
  if(smap->board[0][2] == 'X' && smap->board[1][1] == 'X')
    {
      // bottom left block
    }
  if(smap->board[0][2] == 'X' && smap->board[2][0] == 'X')
    {
      // center block
    }
  if(smap->board[1][1] == 'X' && smap->board[2][0] == 'X')
    {
      // top right block
    }
}

void printBoard(struct shmseg *smap)
{
    int iteration = 6;
    int a = 0;
    int b = 0;
    for(int i = 1; i <= iteration; i++)
    {
        
        if (i % 2 != 0 )
        {
            printf("  %c | %c  | %c ", smap->board[a][0],smap->board[a][1],smap->board[a][2]);
            a++;
            printf("This is what 'a' is: %d", a);
            printf("This is what 'b' is: %d", b);
        }
        else if (i == 6)
        {
            printf("\n");
        }
        else
        {
            printf("\n---|---|---\n");
        }
    }
}

// function provided by Mr. Knight in guided exercise 11
// checks if an error occured, if one has prints error message
int checkError(int e, const char *str)
{
  if(e == -1)
    {
      if(errno == EINTR) return e;
      perror(str);
      exit(EXIT_FAILURE);
    }
  return e;
}

int main(int argc, char *argv[])
{
  struct shmseg *smap;
  int fd;
  int num1, num2;
  int semid, shmid;
  key_t semK, shmK;

  // 1 - checks to see if FIFO exists - if equal to -1 mkfifo has failed
  if(mkfifo("xoSync", S_IRWXU) == -1)
    {
      // checks to see if the FIFO doesn't already exists
      if(errno != EEXIST)
	{
	  perror("mkfifo producer");
	  exit(EXIT_FAILURE);
	}
    }

  // 2 - open FIFO xoSync for read
  checkError(fd = open("xoSync", O_RDONLY), "open FIFO");
  
  // 3- reading string from the FIFO
  checkError(read(fd, &num1, sizeof(num1)), "read num");
  checkError(read(fd, &num2, sizeof(num2)), "read num");

  // 5 - close FIFO
  close(fd);

  // 6 - Generate System V keys with ftok
  // first number uses for shared memory
  shmK = ftok("xoSync", num1);
  // second number used for semaphores
  semK = ftok("xoSync", num2);

  // 7 - retrieve the shared memory and the semaphore set create by player 1
  checkError(semid = semget(semK, 0, 0), "semget");
  checkError(shmid = shmget(shmK, 0, 0), "shmget");

  printf("DEBUG passed shared mem retrieval\n");

  // 8 - Attach the shared memory segment
  smap = shmat(shmid, NULL, 0);
  if(smap == (void *) -1)
    {
      checkError(-1, "shmat");
    }
  
  // 9 - Enter the game play loop
  while(1)
    {
      printf("DEBUG entering game loop\n");

      // 1 - reserve player 2's semaphore
      checkError(reserveSem(semid, 1), "reserveSem");

      // 2 - display the state of the game board
      printBoard(smap);
      
      // 3 - if the turn counter is -1, exit the loop
      if (smap->counter == -1)
	{
	  exit(EXIT_SUCCESS);
	}
	  
      // 4 - make players 2 move
      // logic goes here

      // first O move - random placement

      // if all the checking functions return false - randomly place O
      // if one function returns true - block X

      // 5 - display the state of the game board
      checkError(write(STDOUT_FILENO, "Player 2 (O) \n", 14), "write");
      // display board now
      printBoard(smap);
      
      // 6 - increment the game turn by 1
      smap->counter++;
      // 7 - release player 1's semaphore
      checkError(releaseSem(semid, 0), "releaseSem");
    }
  
  // 10 - Open the FIFO xoSync for read
  checkError(fd = open("xoSync", O_RDONLY), "open consumer");
  
  // 11 - Close the FIFO
  close(fd);
  
  // 12 - Detach the segment of shared memory
  checkError(shmdt(smap), "shmdt");

  exit(EXIT_SUCCESS);
}

