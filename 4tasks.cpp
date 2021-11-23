#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/types.h>

// defining colours to make the sceduling more readable
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KWHT  "\x1B[37m"

// define the number of tasks
#define NTASKS 4

// define the numnber of mutexes
#define NMUTEXES 3

// defining indexes for waste_time() function
#define INNERLOOP 75	
#define OUTERLOOP 250

// defining the number of times a task will be scheduled
#define NEXEC 20

// function to waste time during tasks
void waste_time()
{
	double uno;
	for (int i = 0; i < OUTERLOOP; i++)
	{
		for (int j = 0; j < INNERLOOP; j++)
		{
			uno = rand()*rand()%10;
		}
	}
}

// mutex semaphores
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

// global variables for tasks
int T1T2; // Task1 shall write something into T1T2, Task 2 shall read from it.
int T1T4; // Task1 shall write something into T1T4, Task 4 shall read from it.
int T2T3; // Task2 shall write something into T2T3, Task 3 shall read from it.

/*
#######################################################
#													  #
#  defining critical sections for each task:          #
#  z_11 : critical section where J1 writes on T1T2    #
#  z_12 : critical section where J1 writes on T1T4    #
#  z_21 : critical section where J2 read from T1T2    #
#  z_22 : critical section where J2 writes on T2T3    #
#  z_31 : critical section where J3 reads from T2T3   #	  
#  z_41 : critical section where J4 reads from T1T4   #
#													  #
#  beta_1 = { z_11, z_12 }							  #
#  beta_2 = { z_21, z_22 }							  #	
#  beta_3 = { z_31 }								  #	
#  beta_4 = { z_41 }								  #
#													  #
#  S1 ( z_11, z_21 )		 		         		  #
#  S2 ( z_12, z_41 )				     			  #
#  S3 ( z_22, z_31 )							 	  #
#													  #
#  beta*_12 = { z_21 }								  #
#  beta*_13 = { 0 }									  #
#  beta*_14 = { z_41 }								  #
#  beta*_23 = { z_31 }							 	  #
#  beta*_24 = { 0 }									  #
#  beta*_34 = { 0 }									  #
#													  #
#  beta*_1 = { z_21, z_41 }							  #
#  beta*_2 = { z_31 }								  #
#  beta*_3 = { 0 }									  #
#  beta*_4 = { 0 }									  #
#													  #
#  Since there is only 1 critical section that can    #
#  block  he task Ji  we can assume that it will be   #
#  also the longest blocking section 				  #
#												      #	
#######################################################
*/

// defining variables to store the duration of each critical sections
struct timespec d_11 ;
struct timespec d_12 ;
struct timespec d_21 ;
struct timespec d_22 ;
struct timespec d_31 ;
struct timespec d_41 ;

// defining the duration of each critical section

// code of periodic tasks
void task1_code();
void task2_code();
void task3_code();
void task4_code();

// periodic tasks
void *task1( void *);
void *task2( void *);
void *task3( void *);
void *task4( void *);

// define an array for storing the period values

// T1 = 80ms
// T2 = 100ms
// T3 = 160ms
// T4 = 200ms
// set task periods in nanoseconds
long int periods[NTASKS] = {80000000, 100000000, 160000000, 200000000};

struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS];
pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];
int missed_deadlines[NTASKS];

// calculing the number of times each task has to be executed
long int executions_task1 = NEXEC;
long int executions_task2 = NEXEC;
long int executions_task3 = NEXEC;
long int executions_task4 = NEXEC;

int main()
{
    // assign max and min priority in the task set
    struct sched_param priomax;
    priomax.sched_priority = sched_get_priority_max(SCHED_OTHER);
    struct sched_param priomin;
    priomin.sched_priority = sched_get_priority_min(SCHED_OTHER);

	printf("%sTask 1: T1 = %ld ms\n%sTask 2: T2 = %ld ms\n%sTask 3: T3 = %ld ms\n%sTask 4: T4 = %ld ms\n", KRED, periods[0]/1000000, KBLU, periods[1]/1000000, KGRN, periods[2]/1000000, KYEL, periods[3]/1000000);
	fflush(stdout);

    // set the maximun priority to the current thread (need to be superuser)
    if(!getuid()) pthread_setschedparam(pthread_self(),SCHED_OTHER,&priomax);

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

		clock_gettime(CLOCK_REALTIME, &time_2);

        // compute the Worst Case Execution Time (in real time it should be repeated more times)
        WCET[i]= 1000000000*(time_2.tv_sec - time_1.tv_sec)+(time_2.tv_nsec-time_1.tv_nsec);
      	printf("\n%sWorst Case Execution Time %d = %f ms\n\n", KNRM, i, WCET[i]/1000000);
        fflush(stdout);
    }

	printf("%sCheck now the sufficient condition for the schedulability.\n", KWHT); fflush(stdout);

    // compute U and Ulub at each iteration
    double U = 0;
	double Ulub = 0;
	for(int i = 1; i <= NTASKS; i++)
	{
		switch(i)
		{
			case 1: // J1
				// since J1 has two possible critical section, z_21 and z_41, it has to be determined the longest
				// in order to determine the longest blocking section
				struct timespec B;
				if((d_21.tv_sec*1000000000 + d_21.tv_nsec) > (d_41.tv_sec*1000000000 + d_41.tv_nsec)) B = d_21;
				else B = d_41;
				U += (WCET[i-1]/periods[i-1] + (B.tv_sec*1000000000 + B.tv_nsec)/periods[i-1]);
				Ulub = i*(pow(2.0,1.0/i)-1.0);
				(U <= Ulub) ? printf("%sSufficient condition satisfied. %sU = %f, Ulub = %f\n", KGRN, KNRM, U, Ulub) : printf("%sSufficient condition not satisfied! %sU = %f, Ulub = %f\n", KRED, KNRM, U, Ulub);
				fflush(stdout);
				break;

			case 2: // J2
				U += (WCET[i-1]/periods[i-1] + WCET[i-2]/periods[i-2] + (d_31.tv_sec*1000000000 + d_31.tv_nsec)/periods[i-1]);
				Ulub = i*(pow(2.0,1.0/i)-1.0);
				(U <= Ulub) ? printf("%sSufficient condition satisfied. %sU = %f, Ulub = %f\n", KGRN, KNRM, U, Ulub) : printf("%sSufficient condition not satisfied! %sU = %f, Ulub = %f\n", KRED, KNRM, U, Ulub);
				fflush(stdout);
				break;

			case 3: // J3
				U += (WCET[i-1]/periods[i-1] + WCET[i-2]/periods[i-2] + WCET[i-3]/periods[i-3]);
				Ulub = i*(pow(2.0,1.0/i)-1.0);
				(U <= Ulub) ? printf("%sSufficient condition satisfied. %sU = %f, Ulub = %f\n", KGRN, KNRM, U, Ulub) : printf("%sSufficient condition not satisfied! %sU = %f, Ulub = %f\n", KRED, KNRM, U, Ulub);
				fflush(stdout);
				break;

			case 4: // J4
				U += (WCET[i-1]/periods[i-1] + WCET[i-2]/periods[i-2] + WCET[i-3]/periods[i-3] + WCET[i-4]/periods[i-4]);
				Ulub = i*(pow(2.0,1.0/i)-1.0);
				(U <= Ulub) ? printf("%sSufficient condition satisfied. %sU = %f, Ulub = %f\n", KGRN, KNRM, U, Ulub) : printf("%sSufficient condition not satisfied! %sU = %f, Ulub = %f\n", KRED, KNRM, U, Ulub);
				fflush(stdout);
				break;
		}
	}

    // Check the sufficient condition: if not satisfied, exit
    if(U > Ulub)
    {
        return(-1);
    }
    else
    {
        printf("\n %sSCHEDULING:\n", KNRM);
        fflush(stdout);
        sleep(2);
    }

    // set minimun priority to the current task
    if (getuid() == 0) pthread_setschedparam(pthread_self(),SCHED_OTHER,&priomin);

    // set the attributes to the tasks
    for(int i = 0; i < NTASKS; i++)
    {
        // initializa the attribute structure of task i
      	pthread_attr_init(&(attributes[i]));

		// set the attributes to tell the kernel that the priorities and policies are explicitly chosen,
		// not inherited from the main thread (pthread_attr_setinheritsched) 
      	pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);
      
		// set the attributes to set the SCHED_FIFO policy (pthread_attr_setschedpolicy)
		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_OTHER);

		// properly set the parameters to assign the priority inversely proportional 
		// to the period
      	parameters[i].sched_priority = priomin.sched_priority + NTASKS - i;

		// set the attributes and the parameters of the current thread (pthread_attr_setschedparam)
      	pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    }

	// attributes for semaphores
	pthread_mutexattr_t mymutexattr;
	pthread_mutexattr_init(&mymutexattr);
	pthread_mutexattr_setprotocol(&mymutexattr, PTHREAD_PRIO_PROTECT);

	// Initialization of semaphores according to tasks period
	pthread_mutexattr_setprioceiling(&mymutexattr, parameters[0].sched_priority);
	pthread_mutex_init(&mutex1, &mymutexattr);

	pthread_mutexattr_setprioceiling(&mymutexattr, parameters[0].sched_priority);
	pthread_mutex_init(&mutex2, &mymutexattr);

	pthread_mutexattr_setprioceiling(&mymutexattr, parameters[1].sched_priority);
	pthread_mutex_init(&mutex3, &mymutexattr);

    // delare the variable to contain the return values of pthread_create	
  	int iret[NTASKS];

    //declare variables to read the current time
    struct timespec time_1;
	clock_gettime(CLOCK_REALTIME, &time_1);

    for (int i = 0; i < NTASKS; i++)
    {
		long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
		// then we compute the end of the first period and beginning of the next one
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

	printf("\n"); fflush(stdout);

  	// set the next arrival time for each task. This is not the beginning of the first
	// period, but the end of the first period and beginning of the next one. 
  	for (int i = 0; i < NTASKS; i++)
    	{
      		printf ("%sMissed Deadlines Task %d = %d\n", KNRM, i+1, missed_deadlines[i]);
			fflush(stdout);
    	}

	// destructor
	pthread_mutexattr_destroy(&mymutexattr);
  	exit(0);
}

// Foreach task defined, the scope written concerns the locking and unlocking of different semaphores
// which regulate the writing and reading from the globla variables declared.
// It is used P(Sn) to indicate that a task has joined a critical section while V(Sn) is printed
// when a task goes out from a critical section after hving released the semaphore.

// Every task has some for loops to make them loose some time during the execution and make the output more readable

// application specific task_1 code
void task1_code()
{
	struct timespec time_1;
	struct timespec time_2;

	// print the id of the current task
  	printf(" %s1[ ", KRED); fflush(stdout);
	
	// semaphore added to create a deadlock between J1 and J3
	pthread_mutex_lock(&mutex3);
	waste_time();

	// take the time when the critical section starts
	clock_gettime(CLOCK_REALTIME, &time_1);

	// take the semaphore
	pthread_mutex_lock(&mutex1);

	// write on the variable by adding 1 each time
	T1T2 += 1;
	printf(" %swriting on T1T2 ", KRED); fflush(stdout);

	for(int i=0; i < 3; i++)
	{
		waste_time();
	}
	// released the semaphore
	pthread_mutex_unlock(&mutex1);

	// get the time when the critical section ends
	clock_gettime(CLOCK_REALTIME, &time_2);

	waste_time();

	// releases the semaphore 
	pthread_mutex_unlock(&mutex3);

	// store the value of the first critical section
	d_11.tv_sec = (time_2.tv_sec - time_1.tv_sec);
	d_11.tv_nsec = (time_2.tv_nsec - time_1.tv_nsec);

	// take the time when the critical section starts
	clock_gettime(CLOCK_REALTIME, &time_1);
	
	// take the semaphore
	pthread_mutex_lock(&mutex2);

	waste_time();

	// executing the fuction waste_time() 2 times
	for(int k = 0; k < 2; k++)
	{
		waste_time();
	}

	// write on the variable by adding 1 each time
	T1T4 += 2;
	printf(" %swriting on T1T4 ", KRED); fflush(stdout);

	// release the semaphore
	pthread_mutex_unlock(&mutex2);

	// take the time when the critical section ends
	clock_gettime(CLOCK_REALTIME, &time_2);
	
	// store the value of the first critical section
	d_12.tv_sec = (time_2.tv_sec - time_1.tv_sec);
	d_12.tv_nsec = (time_2.tv_nsec - time_1.tv_nsec);

  	// print the id of the current task
  	printf(" %s]1 ", KRED); fflush(stdout);
}

// thread code for task_1 (used only for temporization)
void *task1( void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

   	//execute the task one hundred times... it should be an infinite loop (too dangerous)
  	int i=0;
  	for (i=0; i < executions_task1; i++)
    {
      	// execute application specific code
		task1_code();

		struct timespec time_1;
		clock_gettime(CLOCK_REALTIME, &time_1);
		if(1000000000*time_1.tv_sec +time_1.tv_nsec >  1000000000*next_arrival_time[0].tv_sec + next_arrival_time[0].tv_nsec) missed_deadlines[0] += 1;
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
	struct timespec time_1;
	struct timespec time_2;

	// print the id of the current task
  	printf(" %s2[ ", KBLU); fflush(stdout);
	
  	// waste time
	waste_time();

	// take the semaphore
	pthread_mutex_lock(&mutex1);

	// take the time when the critical section starts
	clock_gettime(CLOCK_REALTIME, &time_1);

	printf(" %sTaken S1 by J2 ", KWHT); fflush(stdout);
	
	// print to know the program is inside the critical section
	printf(" %sP(S1) ", KBLU); fflush(stdout);
	
	// waste time
	waste_time();
	
	// write on the variable by adding 1 each time
	printf(" %sread T1T2 = %d ", KBLU, T1T2); fflush(stdout);
	
	// releases the semaphore
	pthread_mutex_unlock(&mutex1);

	// take the time when the critical section ends
	clock_gettime(CLOCK_REALTIME, &time_2);

	printf(" %sReleased S1 by J2 ", KWHT); fflush(stdout);
	
	// store the value of the first critical section
	d_21.tv_sec = (time_2.tv_sec - time_1.tv_sec);
	d_21.tv_nsec = (time_2.tv_nsec - time_1.tv_nsec);
	
	// print to know the program is out the critical section
	printf(" %sV(S1) ", KBLU); fflush(stdout);

  	// waste time
	waste_time();

	// take the semaphore
	pthread_mutex_lock(&mutex3);

	// take the time when the critical section starts
	clock_gettime(CLOCK_REALTIME, &time_1);

	printf(" %sTaken S3 by J2 ", KWHT); fflush(stdout);
	// waste time
	waste_time();

	// semaphore to create a deadlock
	pthread_mutex_lock(&mutex2);
	printf(" %sTaken S2 by J2 ", KWHT); fflush(stdout);
	
	// print to know the program is inside the critical section
	printf(" %sP(S2) ", KBLU); fflush(stdout);

	// waste time
	waste_time();
	// write on the variable by adding 3 each time
	T2T3 *= 2;
	
	// release the semaphore
	pthread_mutex_unlock(&mutex2);

	// take the time when the critical section ends
	clock_gettime(CLOCK_REALTIME, &time_2);

	printf(" %sReleased S2 by J2 ", KWHT); fflush(stdout);
	
	// reading from the variable
	printf(" %s... writing on T2T3 ... ", KBLU); fflush(stdout);
	
	// waste time
	waste_time();
	waste_time();
	
	// releases the semaphore
	pthread_mutex_unlock(&mutex3);
	printf(" %sReleased S3 by J2 ", KWHT); fflush(stdout);
	
	// store the value of the first critical section
	d_22.tv_sec = (time_2.tv_sec - time_1.tv_sec);
	d_22.tv_nsec = (time_2.tv_nsec - time_1.tv_nsec);
	
	// print to know the program is out the critical section
	printf(" %sV(S2) ", KBLU); fflush(stdout);

  	// print the id of the current task
  	printf(" %s]2 ", KBLU); fflush(stdout);
}


void *task2( void *ptr )
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	int i=0;
  	for (i=0; i < executions_task2; i++)
    {
        task2_code();

		struct timespec time_1;
		clock_gettime(CLOCK_REALTIME, &time_1);
		if(1000000000*time_1.tv_sec +time_1.tv_nsec >  1000000000*next_arrival_time[1].tv_sec + next_arrival_time[1].tv_nsec) missed_deadlines[1] += 1;

        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
        next_arrival_time[1].tv_nsec= next_arrival_nanoseconds%1000000000;
        next_arrival_time[1].tv_sec= next_arrival_time[1].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}

void task3_code()
{
	struct timespec time_1;
	struct timespec time_2;

	// print the id of the current task
  	printf(" %s3[ ", KGRN); fflush(stdout);

	// take the time when the critical section starts
	clock_gettime(CLOCK_REALTIME, &time_1);

	// take the semaphore
	pthread_mutex_lock(&mutex3);
	
	// executing the fuction waste_time() 2 times
	for(int i=0; i < 2; i++)
	{
		waste_time();
	}

	// semaphore added to create a deadlock between J1 and J3
	pthread_mutex_lock(&mutex1);

	// write on the variable by adding 1 each time
	printf(" %sread T2T3 = %d ", KGRN, T2T3); fflush(stdout);
	
	// waste time
	waste_time();

	// release the semaphore
	pthread_mutex_unlock(&mutex1);
	waste_time();
	
	// release the semaphore
	pthread_mutex_unlock(&mutex3);

	// take the time when the critical section ends
	clock_gettime(CLOCK_REALTIME, &time_2);
	
	// store the value of the first critical section
	d_31.tv_sec = (time_2.tv_sec - time_1.tv_sec);
	d_31.tv_nsec = (time_2.tv_nsec - time_1.tv_nsec);
	
	// print the id of the current task
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
  	for (i=0; i < executions_task3; i++)
    {
        task3_code();

		struct timespec time_1;
		clock_gettime(CLOCK_REALTIME, &time_1);
		if(1000000000*time_1.tv_sec +time_1.tv_nsec >  1000000000*next_arrival_time[2].tv_sec + next_arrival_time[2].tv_nsec) missed_deadlines[2] += 1;

        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
        next_arrival_time[2].tv_nsec= next_arrival_nanoseconds%1000000000;
        next_arrival_time[2].tv_sec= next_arrival_time[2].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}

void task4_code()
{
	struct timespec time_1;
	struct timespec time_2;

	// print the id of the current task
  	printf(" %s4[ ", KYEL); fflush(stdout);

	// take the time when the critical section starts
	clock_gettime(CLOCK_REALTIME, &time_1);

	// take the semaphore
	pthread_mutex_lock(&mutex2);
	printf(" %sTaken S2 by J4", KWHT); fflush(stdout);
	
	// waste time
	waste_time();
	
	// semahpore to create deadlock
	pthread_mutex_lock(&mutex3);
	printf(" %sTaken S3 by J4", KWHT); fflush(stdout);

	// print to know the program is inside the critical section
	printf(" %sP(S2) ", KYEL); fflush(stdout);

	// write on the variable by adding 1 each time
	printf(" %sread T1T4 = %d" , KYEL, T1T4); fflush(stdout);
	
	// releases the semaphore
	pthread_mutex_unlock(&mutex3);
	printf(" %sReleased S3 by J4", KWHT); fflush(stdout);
	
	// waste time
	waste_time();
	waste_time();
	
	// release the semaphore
	pthread_mutex_unlock(&mutex2);

	// take the time when the critical section starts
	clock_gettime(CLOCK_REALTIME, &time_1);
	
	printf(" %sReleased S2 by J4", KWHT); fflush(stdout);

	// waste time
	waste_time();
	
	// store the value of the first critical section
	d_41.tv_sec = (time_2.tv_sec - time_1.tv_sec);
	d_41.tv_nsec = (time_2.tv_nsec - time_1.tv_nsec);
	
	// print to know the program is out the critical section
	printf(" %sV(S2) ", KYEL); fflush(stdout);

  	// print the id of the current task
  	printf(" %s]4 ", KYEL); fflush(stdout);
}

void *task4( void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	int i=0;
  	for (i=0; i < executions_task4; i++)
    {
        task4_code();

		struct timespec time_1;
		clock_gettime(CLOCK_REALTIME, &time_1);
		if(1000000000*time_1.tv_sec +time_1.tv_nsec > 1000000000*next_arrival_time[3].tv_sec + next_arrival_time[3].tv_nsec) missed_deadlines[3] += 1;

        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[3], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[3].tv_nsec + periods[3];
        next_arrival_time[3].tv_nsec = next_arrival_nanoseconds%1000000000;
        next_arrival_time[3].tv_sec = next_arrival_time[3].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}
