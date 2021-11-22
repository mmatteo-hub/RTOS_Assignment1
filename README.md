# RTOS_Assignment1

`Rate_Monotonic_1.cpp` is the referement file to do the assignment.

### .gitignore
In the gitignore there are the files to be ignored.
Remember, when a new `.cpp` file is uploaded, if it has to be compiled so compute `g++ -pthread file.cpp -o file` and then update the `.gitignore` file

### Creating the executable file
In order to create the executable the user must be root so type `sudo su` and insert the `password` required; then create the executable by `g++ -pthread file.cpp -o file`

## Code used 
In the code used, `4tasks.cpp` the protocol used is the PRIORITY INHERITANCE (`PTHREAD_PRIO_PROTECT`) whereas in the code `4tasks_deadlock.cpp` it is used the PRIORITY INHERITANCE protocol (`PTHREAD_PRIO_INHERIT`).

Ther is a function `waste_time()` defined as follows:
```C++
#define INNERLOOP 50
#define OUTERLOOP 250

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
```
and is used to waste time inside the scope of each tasks.
