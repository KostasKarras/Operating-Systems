#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "p3180076-p3180154-pizza1.h"

//mutexes
pthread_mutex_t lockcooker,lockoven,screen;

//2 condition variables that we wil use to make wait και signal into order function
pthread_cond_t ovens,cookers;

//initialization numberOfCook and numberOfOven to know everytime how many exactly cookers and ovens are available
int numberOfCook = Ncook;
int numberOfOven = Noven;

unsigned int seed;

//pointer that will be used below to create a dynamic table to save the times
double *timer;

//variables to get summary results of average and maximum time
double sum,max = 0;

int main(int argc, char *argv[]){
	
	if(argc != 3){
		printf("WRONG! You have to give the number of customers AND the seed to generate random numbers.\n");
		exit(-1);
	}//check if the arguments are exactly 3(->file name -> number of customers -> seed) and if the are not 3, then the program exits.

	//Number of customers
	int Ncust = atoi(argv[1]);

	if(Ncust <= 0){
		printf("WRONG! Number of customers must be positive integer.\n");
		exit(-1);
	}//if number of customers is smaller or equal than zero then we print a message and the program exits

	seed = atoi(argv[2]);
	unsigned int seed1 = seed;

	//thread array with size equal to number of customers
	pthread_t threads[Ncust];

	//initialization of mutexes and condition variables
	pthread_mutex_init(&lockcooker,NULL);
	pthread_mutex_init(&lockoven,NULL);
	pthread_mutex_init(&screen,NULL);
	pthread_cond_init(&ovens,NULL);
	pthread_cond_init(&cookers,NULL);

	//int array with size equal to number of customers, in order to understand, when program is running, the hierarchy of the orders and pass it as an argument in the corresponding thread
	int id[Ncust];
	
	//dynamic array with the corresponding times
	timer = malloc(Ncust*sizeof(double));

	for(int i = 0;i < Ncust;i++){
		
		//first order is coming at the zero time, so it is no need to wait
		if(i != 0){
			
			//sleep for 1-5 seconds (Torderhigh = 5, Torderlow = 1 ==> random % Torderhigh + Torderlow = [1,5])
			sleep(rand_r(&seed1) % Torderhigh + Torderlow);
		}
		
		timer[i] = 0;
		
		//initialization of the id array with number = i + 1, so the number of the orders starts from 1 and no from 0
		id[i] = i+1;
		printf("Received the order: %d\n",i+1);
		
		//create threads with arguments to the corresponding field in the thread table, no attributes, the order function and id [i] for debugging
		pthread_create(&threads[i],NULL,order,&id[i]);
	}

	for (int i = 0;i < Ncust;i++){
		
		//wait until threads are done
		pthread_join(threads[i],NULL);
	}

	//This for loop is used to get summary results
	for (int i = 0;i < Ncust;i++){
		sum += timer[i];
		if(max < timer[i])
			max = timer[i];
	}

	//orders average completion time
	double lepta = (sum / Ncust);

	//lock screen to print the results
	pthread_mutex_lock(&screen);
	printf("The average completion order time is:%.2f λεπτά.\nThe maximum completion order time is:%.2f λεπτά\n",lepta,max);

	//unlock screen
	pthread_mutex_unlock(&screen);

	//release the used space
	free(timer);

	//destroy mutexes and condition variables
	pthread_mutex_destroy(&lockcooker);
	pthread_mutex_destroy(&lockoven);
	pthread_mutex_destroy(&screen);
	pthread_cond_destroy(&ovens);
	pthread_cond_destroy(&cookers);

	return 0;
}

void *order(void *x){
	
	//we take the time at the beginning and the end of the order
	struct timespec start,finish;

	//the respective order number
	int id = *(int*)x;

	//copy the seed to a local variable in order not to use mutex and add the id for greater randomness
	unsigned int seed2 = seed + id;

	//lock mutex for cookers
	pthread_mutex_lock(&lockcooker);
	
	//we start counting the time of the order (basically we take the time of the clock)
	clock_gettime(CLOCK_REALTIME,&start);

	//as long as no cooker is available it displays a message that it has not found cooker and is waiting
	while (numberOfCook == 0){
		printf("The order %d doesn't found manufacturer! She is blocked ...\n",id);
		pthread_cond_wait(&cookers,&lockcooker);
	}

	//reduce the global variable that counts the number of cooks
	numberOfCook--;

	//unlock mutex
	pthread_mutex_unlock(&lockcooker);

	//sleep until it is ready (that is, to prepare before they are put in the oven) (random% 5 + 1)
	sleep((rand_r(&seed2) % Norderhigh + Norderlow) * Tprep);

	//lock mutex for the oven
	pthread_mutex_lock(&lockoven);

	//while no oven is available displays a message that it did not find an oven available and is waiting
	while (numberOfOven == 0){
		printf("The manufacturer is waiting for an oven to be released. So order %d is indirectly blocked ...\n",id);
		pthread_cond_wait(&ovens,&lockoven);
	}

	//we reduce the global variable that counts the number of ovens
	numberOfOven--;

	//unlock mutex for the ovens
	pthread_mutex_unlock(&lockoven);

	sleep(Tbake);//sleep until it is ready (that is, to bake the pizzas)

	//lock mutex for the cookers
	pthread_mutex_lock(&lockcooker);

	//increase the number of cooks after the baking is over
	numberOfCook++;

	//we signal to an order which is blocked and waiting for a cooker to start her
	pthread_cond_signal(&cookers);

	//unlock mutex
	pthread_mutex_unlock(&lockcooker);

	//lock mutex for the ovens
	pthread_mutex_lock(&lockoven);

	//increase number of cookers
	numberOfOven++;

	//we signal to an order which is blocked and waiting for an oven to bake her
	pthread_cond_signal(&ovens);

	//unlock mutex
	pthread_mutex_unlock(&lockoven);

	//lock mutex for screen
	pthread_mutex_lock(&screen);

	//we stop counting the time of the order (basically we take the time of the clock)
	clock_gettime(CLOCK_REALTIME,&finish);

	//total order completion time
	double seconds = finish.tv_sec - start.tv_sec;

	//we assign this time to the timer table with index id-1 because in the main function we passed id + 1 as an argument
	timer[id-1] = seconds;

	//we display the order served as well as the time it took (we use +0.5 for rounding eg 2.3 -> 2 (correct), 2.8 -> 2 (incorrect) | 2.3 + 0.5 = 2.8 -> 2 (correct), 2.8 + 0.5 = 3.3 -> 3 (correct))
	printf("The order with number %d prepared in %d minutes.\n",id,(int)(seconds + 0.5));

	//unlock mutex for screen
	pthread_mutex_unlock(&screen);

	//we leave the thread (we do not return anything to main)
	pthread_exit(NULL);
}