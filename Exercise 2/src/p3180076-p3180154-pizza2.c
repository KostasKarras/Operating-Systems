#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "p3180076-p3180154-pizza2.h"

//mutexes
pthread_mutex_t lockcooker,lockoven,lockdeliverer,screen;

//3 condition variables that we wil use to make wait και signal into order function
pthread_cond_t ovens,cookers,deliverers;

//initialization numberOfCook, numberOfOven and numberOfDeliverer to know everytime how many exactly cookers, ovens and deliverers are available
int numberOfCook = Ncook;
int numberOfOven = Noven;
int numberOfDeliverer = Ndeliverer;

unsigned int seed;

//pointers that will be used below to create dynamic tables to save the times
double *timer,*timer2;

//variables to get summary results of average and maximum time of complete and cold of pizza
double sum,sum2,max,max2 = 0;

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
	pthread_mutex_init(&lockdeliverer,NULL);
	pthread_mutex_init(&screen,NULL);
	pthread_cond_init(&ovens,NULL);
	pthread_cond_init(&cookers,NULL);
	pthread_cond_init(&deliverers,NULL);

	//int array with size equal to number of customers, in order to understand, when program is running, the hierarchy of the orders and pass it as an argument in the corresponding thread
	int id[Ncust];

	//dynamic array with the corresponding times of the completion of the orders
	timer = malloc(Ncust*sizeof(double));

	//dynamic array with the corresponding times of the cold of the orders
	timer2 = malloc(Ncust*sizeof(double));

	for(int i = 0;i < Ncust;i++){
		
		//first order is coming at the zero time, so it is no need to wait
		if(i != 0){
			
			//sleep for 1-5 seconds (Torderhigh = 5, Torderlow = 1 ==> random % Torderhigh + Torderlow = [1,5])
			sleep(rand_r(&seed1) % Torderhigh + Torderlow);
		}
		
		timer[i] = 0;
		timer2[i] = 0;
		
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
		sum2 += timer2[i];
		if(max2 < timer2[i])
			max2 = timer2[i];
		if(max < timer[i])
			max = timer[i];
	}

	//orders average completion time
	double lepta = (sum / Ncust);

	//orders average cold time
	double lepta2 = (sum2 / Ncust);

	//lock screen to print the results
	pthread_mutex_lock(&screen);
	printf("The average completion order time is:%.2f λεπτά.\nThe maximum completion order time is:%.2f λεπτά\n",lepta,max);
	printf("The average cold order time is:%.2f λεπτά.\nThe maximum cold order time is:%.2f λεπτά\n",lepta2,max2);

	//unlock screen
	pthread_mutex_unlock(&screen);

	//release the used space
	free(timer);
	free(timer2);

	//destroy mutexes and condition variables
	pthread_mutex_destroy(&lockcooker);
	pthread_mutex_destroy(&lockoven);
	pthread_mutex_destroy(&lockdeliverer);
	pthread_mutex_destroy(&screen);
	pthread_cond_destroy(&ovens);
	pthread_cond_destroy(&cookers);
	pthread_cond_destroy(&deliverers);

	return 0;
}

void *order(void *x){

	//we take the time at the beginning and the end of the order
	struct timespec start,finish,startfreezing;

	//the respective order number
	int id = *(int*)x;

	//copy the seed to a local variable in order not to use mutex and add the id for greater randomness(for the cooker)
	unsigned int seed2 = seed + id;

	//copy the seed to a local variable in order not to use mutex and add the id for greater randomness(for the deliverer)
	unsigned int seed3 = seed - id;

	//lock mutex for cookers
	pthread_mutex_lock(&lockcooker);

	//we start counting the time of the order (basically we take the time of the clock)
	clock_gettime(CLOCK_REALTIME,&start);

	//as long as no cooker is available it displays a message that it has not found cooker and is waiting
	while (numberOfCook == 0){
		printf("The order %d doesn't found manufacturer! She is blocked ...\n",id);
		pthread_cond_wait(&cookers,&lockcooker);
	}

	//when it comes out of while it means that a cooker has been released and therefore the order is being served
	printf("The order with number: %d served.\n",id);

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
		printf("The cooker is waiting for an oven to be released. So the order with number %d is indirectly blocked ... \ n",id);
		pthread_cond_wait(&ovens,&lockoven);
	}

	//when it comes out of while it means that an oven has been released and therefore the order enters the oven
	printf("The order with number %d enters the oven.\n",id);

	//we reduce the global variable that counts the number of ovens
	numberOfOven--;

	//unlock mutex for the ovens
	pthread_mutex_unlock(&lockoven);

	//lock mutex for the cookers
	pthread_mutex_lock(&lockcooker);

	//increase the number of cooks after the baking is over
	numberOfCook++;

	//we signal to an order which is blocked and waiting for a cooker to start her
	pthread_cond_signal(&cookers);

	//unlock mutex
	pthread_mutex_unlock(&lockcooker);

	//sleep until pizzas are done
	sleep(Tbake);

	//we take the time from the moment the baking is over and the pizzas start to cool
	clock_gettime(CLOCK_REALTIME,&startfreezing);

	//lock mutex for deliverers
	pthread_mutex_lock(&lockdeliverer);

	//while no deliverer is available displays a message that it did not find a deliverer available and is waiting
	while(numberOfDeliverer == 0){
		printf("The order with number %d cools in the oven. A deliverer is expected to pick it up.\n",id);
		pthread_cond_wait(&deliverers,&lockdeliverer);
	}

	//when it comes out of while it means that a deliverer has been released and therefore the order is on his hands
	//we reduce the global variable that counts the number of deliverers
	numberOfDeliverer--;

	//unlock mutex for deliverers
	pthread_mutex_unlock(&lockdeliverer);

	//lock mutex for ovens
	pthread_mutex_lock(&lockoven);

	//increase the number of ovens
	numberOfOven++;

	//we signal to an order which is blocked and waiting for a cooker to start her
	pthread_cond_signal(&ovens);

	//unlock mutex
	pthread_mutex_unlock(&lockoven);

	//sleep randomly for min = 5 to max = 15 and store the same time for the deliverer's come back to pizzaria
	unsigned int deliverer_return = sleep(rand_r(&seed3) % (Thigh - Tlow + 1) + Tlow);

	//lock mutex for screen
	pthread_mutex_lock(&screen);

	//we stop counting the time of the order (basically we take the time of the clock)
	clock_gettime(CLOCK_REALTIME,&finish);

	//total order completion time
	double seconds = finish.tv_sec - start.tv_sec;

	//total order cold time
	double seconds2 = finish.tv_sec - startfreezing.tv_sec;

	//we assign this time to the timer tables with index id-1 because in the main function we passed id + 1 as an argument
	timer[id-1] = seconds;
	timer2[id-1] = seconds2;

	//we display the order served as well as the time it took (we use +0.5 for rounding eg 2.3 -> 2 (correct), 2.8 -> 2 (incorrect) | 2.3 + 0.5 = 2.8 -> 2 (correct), 2.8 + 0.5 = 3.3 -> 3 (correct))
	printf("The order with number %d delivered into %d minutes and colds for %d minutes.\nDeliverer is coming back.\n",id,(int)(seconds + 0.5),(int)(seconds2 + 0.5));

	//unlock mutex for screen
	pthread_mutex_unlock(&screen);
	
	sleep(deliverer_return);//sleep for time equal with the time to deliver the pizza from pizzaria to the house

	//lock mutex for deliverers
	pthread_mutex_lock(&lockdeliverer);

	//increase number of deliverers(after come back to the pizzaria)
	numberOfDeliverer++;

	//signal to an order which is blocked and waiting for deliverer
	pthread_cond_signal(&deliverers);

	//unlock mutex
	pthread_mutex_unlock(&lockdeliverer);

	//we leave the thread (we do not return anything to main)
	pthread_exit(NULL);
}
