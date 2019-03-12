/*
 * Kristina Kolibab
 * Grid locking assignment
 * Feb 22, 2019
 *
 * Compile Unix: gcc gridapp.c -lpthread -DUNIX
 * Compile Windows: clang.exe gridapp.c -DWINDOWS
 * Execute: ./a.out OR ./a.exe [# for NxN grid] [# of threads] [level of granularity]
 *
 * Locking data structures: pthread_mutex_lock / pthread_mutex_unlock 
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#ifdef WINDOWS
	#include <windows.h>
#endif 

#ifdef UNIX
	#include <pthread.h>
	#include <unistd.h>
#endif

#define MAXGRIDSIZE 	10
#define MAXTHREADS		1000
#define NO_SWAPS		20 

extern int errno;

typedef enum {GRID, ROW, CELL, NONE} grain_type;
int gridsize = 0;
int grid[MAXGRIDSIZE][MAXGRIDSIZE];
int threads_left = 0;

time_t start_t, end_t;

#ifdef UNIX
	/* Declare global locks and cell lock array in UNIX */
	pthread_mutex_t grid_lock;
	pthread_mutex_t cell_lock[MAXGRIDSIZE][MAXGRIDSIZE];
	pthread_mutex_t c_lock;
	pthread_mutex_t counter_lock;
#endif

#ifdef WINDOWS
	/* Declare global lock mutex and cell lock array in Windows */
	HANDLE ghMutex;
	HANDLE cell_lock_w[MAXGRIDSIZE][MAXGRIDSIZE];
#endif
 
/****** PRINTGRID ******/
int PrintGrid(int grid[MAXGRIDSIZE][MAXGRIDSIZE], int gridsize)
{
	int i;
	int j;
	
	for (i = 0; i < gridsize; i++)
	{
		for (j = 0; j < gridsize; j++)
			fprintf(stdout, "%d\t", grid[i][j]);
		fprintf(stdout, "\n");
	}
	return 0;
}

/****** INIT_GRID ******/
long InitGrid(int grid[MAXGRIDSIZE][MAXGRIDSIZE], int gridsize)
{
	int i;
	int j;
	long sum = 0;
	int temp = 0;

	srand( (unsigned int)time( NULL ) );

	for (i = 0; i < gridsize; i++)
		for (j = 0; j < gridsize; j++) {
			temp = rand() % 100;			
			grid[i][j] = temp;
			sum = sum + temp;
		}

	return sum;
}

/****** SUMGRID ******/
long SumGrid(int grid[MAXGRIDSIZE][MAXGRIDSIZE], int gridsize)
{
	int i;
	int j;
	long sum = 0;

	for (i = 0; i < gridsize; i++){
		for (j = 0; j < gridsize; j++) {
			sum = sum + grid[i][j];
		}
	}
	return sum;
}

/****** DO_SWAPS ******/
void* do_swaps(void* args)
{
	/* Declare local variables */
	int i, row1, column1, cell1, row2, column2, cell2;
	int temp;
	int j;
	int row_unlock_type;
	grain_type* gran_type = (grain_type*)args;

/* Incr in UNIX */
#ifdef UNIX
	pthread_mutex_lock(&counter_lock); 
	threads_left++;
	pthread_mutex_unlock(&counter_lock); 
#endif 

/* Incr in Windows */
#ifdef WINDOWS
	WaitForSingleObject(ghMutex, INFINITE);
	threads_left++;
	ReleaseMutex(ghMutex);
#endif


	for(i=0; i<NO_SWAPS; i++)
	{
		row1 = rand() % gridsize;
		column1 = rand() % gridsize;	
		row2 = rand() % gridsize;
		column2 = rand() % gridsize;

		/****** ROW ******/
		if (*gran_type == ROW)
		{
			if(row1 == row2 && column1 == column2){
				/* If cells are the same, skip to avoid deadlock */ 
				continue;
			} else if (row1 == row2){
				/* If rows are the same, lock the columns instead */
				row_unlock_type = 1; /* randomly chosen value */
#ifdef UNIX
				/* Locking this critical section */
				pthread_mutex_lock(&c_lock);
				for(j = 0; j < gridsize; j++){
					pthread_mutex_lock(&cell_lock[j][column1]);
					pthread_mutex_lock(&cell_lock[j][column2]);
				}
				pthread_mutex_unlock(&c_lock);
#endif

#ifdef WINDOWS
				/* Locking this critical section */
				WaitForSingleObject(ghMutex, INFINITE);
				for(j = 0; j < gridsize; j++){
					WaitForSingleObject(cell_lock_w[j][column1], INFINITE);
					WaitForSingleObject(cell_lock_w[j][column2], INFINITE);
				}
				ReleaseMutex(ghMutex);
#endif
			} else {
				/* rows and columns are NOT the same, proceed as normal */
				row_unlock_type = 0;
#ifdef UNIX
				/* Locking this critical section */
				pthread_mutex_lock(&c_lock);
				for(j = 0; j < gridsize; j++){
					pthread_mutex_lock(&cell_lock[row1][j]);
					pthread_mutex_lock(&cell_lock[row2][j]);
				}
				pthread_mutex_unlock(&c_lock);
#endif

#ifdef WINDOWS
				/* Locking this critical section */
				WaitForSingleObject(ghMutex, INFINITE);
				for(j = 0; j < gridsize; j++){
					WaitForSingleObject(cell_lock_w[row1][j], INFINITE);
					WaitForSingleObject(cell_lock_w[row2][j], INFINITE);
				}
				ReleaseMutex(ghMutex);
#endif
			}
		} 

		/****** CELL ******/
		else if (*gran_type == CELL)
		{
			if(row1 == row2 && column1 == column2){
				/* If cells are the same, skip to avoid deadlock */ 
				continue;
			} else {
#ifdef UNIX
			/* Locking this critical section */
			pthread_mutex_lock(&c_lock);
			pthread_mutex_lock(&cell_lock[row1][column1]);
			pthread_mutex_lock(&cell_lock[row2][column2]);		
			pthread_mutex_unlock(&c_lock);
#endif 

#ifdef WINDOWS
			/* Locking this critical section */
			WaitForSingleObject(ghMutex, INFINITE);
			WaitForSingleObject(cell_lock_w[row1][column1], INFINITE);
			WaitForSingleObject(cell_lock_w[row2][column2], INFINITE);
			ReleaseMutex(ghMutex);
#endif
			}
		}

		/****** GRID ******/
		else if (*gran_type == GRID)
		{
#ifdef UNIX
			pthread_mutex_lock(&grid_lock);
#endif

#ifdef WINDOWS
			WaitForSingleObject(ghMutex, INFINITE);
#endif
		}

		/* Swapping happens here */
		temp = grid[row1][column1];
#ifdef UNIX
		sleep(1);
#endif

#ifdef WINDOWS
		Sleep(1000);
#endif
		grid[row1][column1]=grid[row2][column2];
		grid[row2][column2]=temp;

		/****** ROW ******/
		if (*gran_type == ROW)
		{
			if(row_unlock_type){ /* if 1/true, unlock columns */
#ifdef UNIX
				for(j = 0; j < gridsize; j++){
					pthread_mutex_unlock(&cell_lock[j][column1]);
					pthread_mutex_unlock(&cell_lock[j][column2]);
				}
#endif 
				 
#ifdef WINDOWS
				for(j = 0; j < gridsize; j++){
					ReleaseMutex(cell_lock_w[j][column1]);
					ReleaseMutex(cell_lock_w[j][column2]);
				}
#endif

			} else { /* else, unlock rows */
#ifdef UNIX
				for(j = 0; j < gridsize; j++){
					pthread_mutex_unlock(&cell_lock[row1][j]);
					pthread_mutex_unlock(&cell_lock[row2][j]);
				}
#endif

#ifdef WINDOWS
				for(j = 0; j < gridsize; j++){
					ReleaseMutex(cell_lock_w[row1][j]);
					ReleaseMutex(cell_lock_w[row2][j]);
				}
#endif

			}
		} 
		/****** CELL ******/
		else if (*gran_type == CELL)
		{
#ifdef UNIX
			pthread_mutex_unlock(&cell_lock[row2][column2]);
			pthread_mutex_unlock(&cell_lock[row1][column1]);
#endif

#ifdef WINDOWS
			ReleaseMutex(cell_lock_w[row1][column1]);
			ReleaseMutex(cell_lock_w[row2][column2]);
#endif

		}
		/****** GRID ******/
		else if (*gran_type == GRID)
		{
#ifdef UNIX
			pthread_mutex_unlock(&grid_lock);
#endif

#ifdef WINDOWS
			ReleaseMutex(ghMutex);
#endif
		}
	}

/* Decr in UNIX */
#ifdef UNIX
	pthread_mutex_lock(&counter_lock); 
	threads_left--;
	if (threads_left == 0){  
	  time(&end_t);         
	}
	pthread_mutex_unlock(&counter_lock);
#endif

/* Decr in Windows */
#ifdef WINDOWS
	WaitForSingleObject(ghMutex, INFINITE);
	threads_left--;
	if(threads_left == 0){
		time(&end_t);
	}
	ReleaseMutex(ghMutex);
#endif

	return NULL;
}	

/****** MAIN ******/
int main(int argc, char **argv)
{
	int nthreads = 0;
#ifdef UNIX
	pthread_t threads[MAXTHREADS];
#endif
	grain_type rowGranularity = NONE;
	long initSum = 0, finalSum = 0;
	int i, j, k;

#ifdef UNIX
	/* Initialize UNIX locks */
	pthread_mutex_init(&grid_lock, NULL);
	pthread_mutex_init(&counter_lock, NULL);
	pthread_mutex_init(&c_lock, NULL);
	for(j = 0; j < MAXGRIDSIZE; j++){
		for(k = 0; k < MAXGRIDSIZE; k++){
			pthread_mutex_init(&cell_lock[j][k], NULL);
		}
	}
#endif

#ifdef WINDOWS
	/* Initialize Windows locks */
	ghMutex = CreateMutex(NULL, false, NULL);
	for(j = 0; j < MAXGRIDSIZE; j++){
		for(k = 0; k < MAXGRIDSIZE; k++){
			cell_lock_w[j][k] = CreateMutex(NULL, false, NULL);
		}
	}
#endif
	
	if (argc > 3)
	{
		/* Check for grid size */
		gridsize = atoi(argv[1]);					
		if (gridsize > MAXGRIDSIZE || gridsize < 1)
		{
			printf("Grid size must be between 1 and 10.\n");
			return(1);
		}
		/* Check for number of threads */
		nthreads = atoi(argv[2]);
		if (nthreads < 1 || nthreads > MAXTHREADS)
		{
			printf("Number of threads must be between 1 and 1000.");
			return(1);
		}
		/* Check for granularity */
		if (argv[3][1] == 'r' || argv[3][1] == 'R')
			rowGranularity = ROW;
		if (argv[3][1] == 'c' || argv[3][1] == 'C')
			rowGranularity = CELL;
		if (argv[3][1] == 'g' || argv[3][1] == 'G')
		  rowGranularity = GRID;
			
	}
	else
	{
		printf("Format:  gridapp gridSize numThreads -cell\n");
		printf("         gridapp gridSize numThreads -row\n");
		printf("         gridapp gridSize numThreads -grid\n");
		printf("         gridapp gridSize numThreads -none\n");
		return(1);
	}

	printf("Initial Grid:\n\n");
	initSum =  InitGrid(grid, gridsize);
	PrintGrid(grid, gridsize);
	printf("\nInitial Sum:  %ld\n", initSum);
	printf("Executing threads...\n");

	srand((unsigned int)time( NULL ) );
	
	time(&start_t);
#ifdef UNIX
	for (i = 0; i < nthreads; i++)
	{
		if (pthread_create(&(threads[i]), NULL, do_swaps, (void *)(&rowGranularity)) != 0)
		{
			perror("thread creation failed:");
			exit(-1);
		} 
	}

	for (i = 0; i < nthreads; i++)
		pthread_detach(threads[i]);
#endif

	while (1)
	{
#ifdef UNIX
		sleep(2);
#endif

#ifdef WINDOWS
		Sleep(2000);
#endif

		if (threads_left == 0)
		  {
		    fprintf(stdout, "\nFinal Grid:\n\n");
		    PrintGrid(grid, gridsize);
		    finalSum = SumGrid(grid, gridsize); 
		    fprintf(stdout, "\n\nFinal Sum:  %ld\n", finalSum);
		  
			if (initSum != finalSum){
		      fprintf(stdout,"DATA INTEGRITY VIOLATION!!!!!\n");
		    } else {
		      fprintf(stdout,"DATA INTEGRITY MAINTAINED!!!!!\n");
		    }
		    fprintf(stdout, "Secs elapsed:  %g\n", difftime(end_t, start_t));

		    exit(0);
		  }
	}		

/* Clean up UNIX stuff */
#ifdef UNIX
	pthread_mutex_destroy(&grid_lock);
	pthread_mutex_destroy(&counter_lock);
	pthread_mutex_destroy(&c_lock);
	for(j = 0; j < MAXGRIDSIZE; j++){
		for(k = 0; k < MAXGRIDSIZE; k++){
			pthread_mutex_destroy(&cell_lock[j][k]);
		}
	}
#endif

/* Clean up Windows stuff */
#ifdef WINDOWS
	CloseHandle(ghMutex);
	for(j = 0; j < MAXGRIDSIZE; j++){
		for(k = 0; k < MAXGRIDSIZE; k++){
			CloseHandle(cell_lock_w[j][k]);
		}
	}
#endif
	
	return(0);
}
