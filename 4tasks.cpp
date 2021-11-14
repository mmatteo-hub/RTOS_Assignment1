#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/types.h>

// define the number of tasks
#define NTASKS 4

#define INNERLOOP 10
#define OUTERLOOP 2000

// global variables for tasks
int T1T2; // Task1 shall write something into T1T2, Task 2 shall read from it.
int T1T4; // Task1 shall write something into T1T4, Task 4 shall read from it.
int T2T3; // Task2 shall write something into T2T3, Task 3 shall read from it.

// code of periodic tasks
void task1_code();
void task2_code();
void task3_code();
void task4_code();

//periodic tasks
void *task1( void *);
void *task2( void *);
void *task3( void *);
void *task4( void *);

// define an array for storing the period values
long int periods[NTASKS];
struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS];
pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];
int missed_deadlines[NTASKS];

int main()
{
    // set task periods in nanoseconds
    // T1 = 80ms
    // T2 = 100ms
    // T3 = 160ms
    // T4 = 200ms
    periods[0] = 80000000;
    periods[1] = 100000000;
    periods[2] = 160000000;
    periods[3] = 200000000;

    // assign max and min priority in the task set
    struct sched_param priomax;
    priomax.sched_priority = sched_get_priority_max(SCHED_FIFO);
    struct sched_param priomin;
    priomin.sched_priority = sched_get_priority_min(SCHED_FIFO);

    // set the maximun priority to the current thread (need to be superuser)
    if(!getuid()) pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomax);

    // execute all tasks in standalone modality to measure the execution time
    int i;
    for(i = 0; i < NTASKS; i++)
    {
        struct timespec time_1, time_2;
        clock_gettime(CLOCK_REALTIME, &time_1);

        // need to execute each task more than one for computing the WCET periodic tasks
        if(!i) task1_code();
        if(i) task2_code();
        if(i == 2) task3_code();
        if(i == 3) task4_code();

        // compute the Worst Case Execution Time (in real time it should be repeated more times)
        WCET[i]= 1000000000*(time_2.tv_sec - time_1.tv_sec)+(time_2.tv_nsec-time_1.tv_nsec);
      	printf("\nWorst Case Execution Time %d=%f \n", i, WCET[i]);
        fflush(stdout);
    }

    // compute U
    double U = 0;
    for(int i = 0; i < NTASKS; i++)
    {
        U += WCET[i]/periods[i];
    }
    
    // compute Ulub by considering we do not have harmonic relationships between periods
    double Ulub = NTASKS*(pow(2.0,(1.0/NTASKS)) -1);

    // if there are harmonic relationships Ulub = 1;

    // Check the sufficient condition: if not satisfied, exit
    if(U > Ulub)
    {
        printf("\n U=%lf Ulub=%lf Non schedulable Task Set", U, Ulub);
      	fflush(stdout);
        return(-1);
    }
    else
    {
        printf("\n U=%lf Ulub=%lf Scheduable Task Set", U, Ulub);
        fflush(stdout);
        sleep(2);
    }

    // set minimun priority to the current task
    if (getuid() == 0) pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);

    // set the attributes to the tasks
    for(int i = 0; i < NTASKS; i++)
    {
        //initializa the attribute structure of task i
      	pthread_attr_init(&(attributes[i]));

		//set the attributes to tell the kernel that the priorities and policies are explicitly chosen,
		//not inherited from the main thread (pthread_attr_setinheritsched) 
      	pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);
      
		// set the attributes to set the SCHED_FIFO policy (pthread_attr_setschedpolicy)
		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

		//properly set the parameters to assign the priority inversely proportional 
		//to the period
      	parameters[i].sched_priority = priomin.sched_priority + NTASKS - i;

		//set the attributes and the parameters of the current thread (pthread_attr_setschedparam)
      	pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    }

    //delare the variable to contain the return values of pthread_create	
  	int iret[NTASKS];

    //declare variables to read the current time
    struct timespec time_1;
	clock_gettime(CLOCK_REALTIME, &time_1);

    for (int i = 0; i < NTASKS; i++)
    {
		long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
		//then we compute the end of the first period and beginning of the next one
		next_arrival_time[i].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[i].tv_sec= time_1.tv_sec + next_arrival_nanoseconds/1000000000;
       	missed_deadlines[i] = 0;
    }

    // create all threads(pthread_create)
  	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), task1, NULL);
  	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task2, NULL);
  	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task3, NULL);
	iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task4, NULL);

  	// join all threads (pthread_join)
  	pthread_join( thread_id[0], NULL);
  	pthread_join( thread_id[1], NULL);
  	pthread_join( thread_id[2], NULL);
	pthread_join( thread_id[3], NULL);

  	// set the next arrival time for each task. This is not the beginning of the first
	// period, but the end of the first period and beginning of the next one. 
  	for (int i = 0; i < NTASKS; i++)
    	{
      		printf ("\nMissed Deadlines Task %d=%d", i, missed_deadlines[i]);
		fflush(stdout);
    	}
  	exit(0);
}

// application specific task_1 code
void task1_code()
{
	//print the id of the current task
  	printf(" 1[ "); fflush(stdout);

	//this double loop with random computation is only required to waste time
	int i,j;
	double uno;
  	for (i = 0; i < OUTERLOOP; i++)
    	{
      		for (j = 0; j < INNERLOOP; j++)
		{
			uno = rand()*rand()%10;
    		}
  	}

  	// when the random variable uno=0, then aperiodic task 5 must
  	// be executed
 
  	// when the random variable uno=1, then aperiodic task 5 must
  	// be executed
  
  	//print the id of the current task
  	printf(" ]1 "); fflush(stdout);
}

//thread code for task_1 (used only for temporization)
void *task1( void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

   	//execute the task one hundred times... it should be an infinite loop (too dangerous)
  	int i=0;
  	for (i=0; i < 100; i++)
    {
      		// execute application specific code
		task1_code();

		// it would be nice to check if we missed a deadline here... why don't
		// you try by yourself?

		// sleep until the end of the current period (which is also the start of the
		// new one
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);

		// the thread is ready and can compute the end of the current period for
		// the next iteration
 		
		long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
		next_arrival_time[0].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[0].tv_sec= next_arrival_time[0].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}

void task2_code()
{
	//print the id of the current task
  	printf(" 2[ "); fflush(stdout);
	int i,j;
	double uno;
  	for (i = 0; i < OUTERLOOP; i++)
    {
    	for (j = 0; j < INNERLOOP; j++)
	    {
		    uno = rand()*rand()%10;
		}
    }
	//print the id of the current task
  	printf(" ]2 "); fflush(stdout);
}


void *task2( void *ptr )
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	int i=0;
  	for (i=0; i < 100; i++)
    {
        task2_code();

        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
        next_arrival_time[1].tv_nsec= next_arrival_nanoseconds%1000000000;
        next_arrival_time[1].tv_sec= next_arrival_time[1].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}

void task3_code()
{
	//print the id of the current task
  	printf(" 3[ "); fflush(stdout);
	int i,j;
	double uno;
  	for (i = 0; i < OUTERLOOP; i++)
    {
        for (j = 0; j < INNERLOOP; j++)
        {		
            uno = rand()*rand()%10;
        }
    }
	//print the id of the current task
  	printf(" ]3 "); fflush(stdout);
}

void *task3( void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	int i=0;
  	for (i=0; i < 100; i++)
    {
        task3_code();

        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
        next_arrival_time[2].tv_nsec= next_arrival_nanoseconds%1000000000;
        next_arrival_time[2].tv_sec= next_arrival_time[2].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}

void task4_code()
{
	//print the id of the current task
  	printf(" 4[ "); fflush(stdout);
	int i,j;
	double uno;
  	for (i = 0; i < OUTERLOOP; i++)
    {
        for (j = 0; j < INNERLOOP; j++)
        {		
            uno = rand()*rand()%10;
        }
    }
	//print the id of the current task
  	printf(" ]4 "); fflush(stdout);
}

void *task4( void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	int i=0;
  	for (i=0; i < 100; i++)
    {
        task4_code();

        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[3], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[3].tv_nsec + periods[3];
        next_arrival_time[3].tv_nsec= next_arrival_nanoseconds%1000000000;
        next_arrival_time[3].tv_sec= next_arrival_time[3].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}