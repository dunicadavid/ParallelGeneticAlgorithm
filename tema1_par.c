#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "genetic_algorithm.h"

int main(int argc, char *argv[])
{	

	int nr_of_threads = 0;

	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// first power of 2 after object_count
	int power_of_two = 1;
	
	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &nr_of_threads, argc, argv)) {
		return 0;
	}
	
	//optain power_of_two
	for(; power_of_two < object_count ; power_of_two*=2);

	individual *current_generation = (individual*) calloc(power_of_two, sizeof(individual));
	individual *next_generation = (individual*) calloc(power_of_two, sizeof(individual));
	individual *generation_copy = (individual*) calloc(power_of_two, sizeof(individual));

	pthread_barrier_t barrier;

	pthread_t th[nr_of_threads];


	pthread_barrier_init(&barrier, NULL, nr_of_threads);
	thread_input th_in[nr_of_threads];

	for (int i = 0; i < nr_of_threads; i++)
	{
		th_in[i].objects = objects;
		th_in[i].object_count = &object_count;
		th_in[i].sack_capacity = &sack_capacity;
		th_in[i].generations_count = &generations_count;
		th_in[i].start = i * (double)object_count / nr_of_threads;
		th_in[i].end = min((i + 1) * (double)object_count / nr_of_threads, object_count);
		th_in[i].index = i;
		th_in[i].nr_of_threads = &nr_of_threads;
		th_in[i].power_of_two = &power_of_two;
		th_in[i].next_generation = next_generation;
		th_in[i].current_generation = current_generation;
		th_in[i].copy_generation = generation_copy;
		th_in[i].barrier = &barrier;
	}

	for (int i = 0; i < nr_of_threads; i++)
	{
		pthread_create(&th[i], NULL, run_genetic_algorithm, &th_in[i]);
	}

	for (int i = 0; i < nr_of_threads; i++)
	{
		pthread_join(th[i], NULL);
	}

	free(objects);
	free(current_generation);
	free(next_generation);
	free(generation_copy);
	return 0;
}