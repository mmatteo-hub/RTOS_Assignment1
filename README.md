# RTOS_Assignment1

`Rate_Monotonic_1.cpp` is the referement file to do the assignment.

### Creating the executable file
In order to create the executable the user must be root so type `sudo su` and insert the `password` required; then create the executable by `g++ -pthread file.cpp -o file`

#### .gitignore
In the gitignore there are all files to be ignored.
Remember, when a new `.cpp` file is uploaded, after having compiled it, update the `.gitignore` file with the executable name choosen.

## Code used 
In the code used, `4tasks.cpp`, the protocol used is the PRIORITY CEILING (`PTHREAD_PRIO_PROTECT`) as in the code `4tasks_a.cpp`. In `4tasks.cpp` there are some deadlocks, in particular:
* between J1 and J3
* between J2 and J4

In the code those deadlocks are resolved, if we change the priority protocol used with the PRIORITY INHERITANCE one for istance, we can see the program will block due to this presence.

There is a function `waste_time()` defined as follows:
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
