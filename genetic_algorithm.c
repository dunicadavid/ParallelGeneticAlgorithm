#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "genetic_algorithm.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *nr_of_threads, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 3)
	{
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL)
	{
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2)
	{
		fclose(fp);
		return 0;
	}

	if (*object_count % 10)
	{
		fclose(fp);
		return 0;
	}

	*nr_of_threads = (int)strtol(argv[3], NULL, 10);

	sack_object *tmp_objects = (sack_object *)calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i)
	{
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2)
		{
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int)strtol(argv[2], NULL, 10);

	if (*generations_count == 0)
	{
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i)
	{
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i)
	{
		for (int j = 0; j < generation[i].chromosome_length; ++j)
		{
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int start, int end, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = start; i < end; ++i)
	{
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j)
		{
			if (generation[i].chromosomes[j])
			{
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *)a;
	individual *second = (individual *)b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0)
	{
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i)
		{
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0)
		{
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0)
	{
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step)
		{
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
	else
	{
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step)
		{
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step)
	{
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i)
	{
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

//void run_genetic_algorithm(const sack_object *objects, int object_count, int generations_count, int sack_capacity)
void *run_genetic_algorithm(void *arg)
{

	thread_input argu = *(thread_input *)arg;
	int count, cursor;
	individual *tmp = NULL;

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = argu.start; i < argu.end; ++i)
	{
		argu.current_generation[i].fitness = 0;
		argu.current_generation[i].chromosomes = (int *)calloc(*(argu.object_count), sizeof(int));
		argu.current_generation[i].chromosomes[i] = 1;
		argu.current_generation[i].index = i;
		argu.current_generation[i].chromosome_length = *(argu.object_count);

		argu.next_generation[i].fitness = 0;
		argu.next_generation[i].chromosomes = (int *)calloc(*(argu.object_count), sizeof(int));
		argu.next_generation[i].index = i;
		argu.next_generation[i].chromosome_length = *(argu.object_count);
	}

	// iterate for each generation
	for (int k = 0; k < *(argu.generations_count); ++k)
	{

		cursor = 0;

		// compute fitness and sort by it
		compute_fitness_function(argu.objects, argu.current_generation, argu.start, argu.end, *(argu.sack_capacity));
		pthread_barrier_wait(argu.barrier);

		// merge-sort
		argu.start = argu.index * (double)*(argu.power_of_two) / *(argu.nr_of_threads);
		argu.end = min((argu.index + 1) * (double)*(argu.power_of_two) / *(argu.nr_of_threads), *(argu.power_of_two));

		for (int i = 1; i < *(argu.power_of_two); i *= 2)
		{
			argu.start = (argu.start / (2 * i)) * (2 * i);
			argu.end = min(*(argu.power_of_two), (argu.end / (2 * i)) * (2 * i));
			for (int j = argu.start; j < argu.end; j = j + 2 * i)
			{
				merge_sort(argu.current_generation, j, j + i, j + 2 * i, argu.copy_generation);
			}

			pthread_barrier_wait(argu.barrier);
			tmp = argu.current_generation;
			argu.current_generation = argu.copy_generation;
			argu.copy_generation = tmp;
			pthread_barrier_wait(argu.barrier);
		}

		// keep first 30% children (elite children selection)
		count = *(argu.object_count) * 3 / 10;
		argu.start = argu.index * (double)count / *(argu.nr_of_threads);
		argu.end = min((argu.index + 1) * (double)count / *(argu.nr_of_threads), count);

		for (int i = argu.start; i < argu.end; ++i)
		{
			copy_individual(argu.current_generation + i, argu.next_generation + i);
		}
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = *(argu.object_count) * 2 / 10;
		argu.start = argu.index * (double)count / *(argu.nr_of_threads);
		argu.end = min((argu.index + 1) * (double)count / *(argu.nr_of_threads), count);

		for (int i = argu.start; i < argu.end; ++i)
		{
			copy_individual(argu.current_generation + i, argu.next_generation + cursor + i);
			mutate_bit_string_1(argu.next_generation + cursor + i, k);
		}
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = *(argu.object_count) * 2 / 10;

		for (int i = argu.start; i < argu.end; ++i)
		{
			copy_individual(argu.current_generation + i + count, argu.next_generation + cursor + i);
			mutate_bit_string_2(argu.next_generation + cursor + i, k);
		}
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = *(argu.object_count) * 3 / 10;

		if (count % 2 == 1)
		{
			copy_individual(argu.current_generation + *(argu.object_count) - 1, argu.next_generation + cursor + count - 1);
			count--;
		}

		argu.start = argu.index * (double)count / *(argu.nr_of_threads);
		argu.start += argu.start % 2;
		argu.end = min((argu.index + 1) * (double)count / *(argu.nr_of_threads), count);

		for (int i = argu.start; i < argu.end; i += 2)
		{
			crossover(argu.current_generation + i, argu.next_generation + cursor + i, k);
		}
		pthread_barrier_wait(argu.barrier);

		// switch to new generation
		tmp = argu.current_generation;
		argu.current_generation = argu.next_generation;
		argu.next_generation = tmp;

		argu.start = argu.index * (double)*(argu.object_count) / *(argu.nr_of_threads);
		argu.end = min((argu.index + 1) * (double)*(argu.object_count) / *(argu.nr_of_threads), *(argu.object_count));

		for (int i = argu.start; i < argu.end; ++i)
		{
			argu.current_generation[i].index = i;
		}

		if (k % 5 == 0 && argu.index == 0)
		{
			print_best_fitness(argu.current_generation);
		}
	}

	compute_fitness_function(argu.objects, argu.current_generation, argu.start, argu.end, *(argu.sack_capacity));
	pthread_barrier_wait(argu.barrier);

	// merge-sort
	argu.start = argu.index * (double)*(argu.power_of_two) / *(argu.nr_of_threads);
	argu.end = min((argu.index + 1) * (double)*(argu.power_of_two) / *(argu.nr_of_threads), *(argu.power_of_two));

	for (int i = 1; i < *(argu.power_of_two); i *= 2)
	{
		argu.start = (argu.start / (2 * i)) * (2 * i);
		argu.end = min(*(argu.power_of_two), (argu.end / (2 * i)) * (2 * i));
		for (int j = argu.start; j < argu.end; j = j + 2 * i)
		{
			merge_sort(argu.current_generation, j, j + i, j + 2 * i, argu.copy_generation);
		}

		pthread_barrier_wait(argu.barrier);
		tmp = argu.current_generation;
		argu.current_generation = argu.copy_generation;
		argu.copy_generation = tmp;
		pthread_barrier_wait(argu.barrier);
	}

	if (argu.index == 0)
		print_best_fitness(argu.current_generation);

	pthread_exit(NULL);
}

void merge_sort(individual *generation, int start, int mid, int end, individual *copy_generation)
{
	int iA = start;
	int iB = mid;
	int i;
	for (i = start; i < end; i++)
	{
		if (end == iB || (iA < mid && generation[iA].fitness >= generation[iB].fitness))
		{
			copy_generation[i] = generation[iA];
			iA++;
		}
		else
		{
			copy_generation[i] = generation[iB];
			iB++;
		}
	}
}