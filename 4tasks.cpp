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

// global variables for tasks
int T1T2; // Task1 shall write something into T1T2, Task 2 shall read from it.
int T1T4; // Task1 shall write something into T1T4, Task 4 shall read from it.
int T2T3; // Task2 shall write something into T2T3, Task 3 shall read from it.

// code of periodic casts
void task1_code();
void task2_code();
void task3_code();
void task4_code();

// define an array for storing the period values
long int periods[NTASKS];

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
}