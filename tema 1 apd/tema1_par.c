#include <stdlib.h>
#include "genetic_algorithm.h"
#include <stdio.h>
#include <pthread.h>
typedef struct param
{
	int id;
	sack_object *objects;
	int object_count;
	int generations_count;
	int sack_capacity;
	pthread_barrier_t *barrier;
	pthread_mutex_t *mutex;
	int P;
	individual *current_generation;
	individual *next_generation;
	individual *tmp;
	int *cur_count_table;
	int *next_count_table;
} params;

int min(int a, int b)
{
	return a < b ? a : b;
}

void start_end_calc(int *start, int *end, int id, int P, int N) // n = size
{
	*start = id * N / P;
	*end = min((id + 1) * N / P, N);
}

void *f(void *arg)
{
	params par = *(params *)arg;
	int id = par.id;
	sack_object *objects = par.objects;
	int object_count = par.object_count;
	int generations_count = par.generations_count;
	int sack_capacity = par.sack_capacity;
	pthread_barrier_t *barrier = par.barrier;
	pthread_mutex_t *mutex = par.mutex;
	int P = par.P;
	individual *current_generation = par.current_generation;
	individual *next_generation = par.next_generation;
	individual *tmp = par.tmp;
	int *cur_count_table = par.cur_count_table;
	int *next_count_table = par.next_count_table;
	int count, cursor, start, end, prc20, prc30;
	int p0, p1, p2, p3, p4;

	start_end_calc(&start, &end, id, P, object_count);
	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = start; i < end; ++i)
	{
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int *)calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;
		current_generation[i].count_table = cur_count_table;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int *)calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
		next_generation[i].count_table = cur_count_table;
	}
	pthread_barrier_wait(barrier);
	// iterate for each generation
	for (int k = 0; k < generations_count; ++k)
	{

		for (int i = start; i < end; ++i)
		{
			current_generation[i].count_table[i] = 0;
		}

		cursor = 0;

		// compute fitness and sort by it
		compute_fitness_function(objects, current_generation, object_count, sack_capacity, start, end);
		pthread_barrier_wait(barrier);

		for (int i = start; i < end; ++i)
		{
			for (int j = 0; j < current_generation[i].chromosome_length; ++j)
			{
				current_generation[i].count_table[i] += current_generation[i].chromosomes[j];
			}
		}

		pthread_barrier_wait(barrier);

		if (id == 0)
		{
			qsort(current_generation, object_count, sizeof(individual), cmpfunc);
			count = object_count * 3 / 10;
			for (int i = 0; i < count; ++i)
			{
				copy_individual(current_generation + i, next_generation + i);
			}
			cursor = count;
			count = object_count * 2 / 10;
			for (int i = 0; i < count; ++i)
			{
				copy_individual(current_generation + i, next_generation + cursor + i);
				mutate_bit_string_1(next_generation + cursor + i, k);
			}
			cursor += count;
			count = object_count * 2 / 10;
			for (int i = 0; i < count; ++i)
			{
				copy_individual(current_generation + i + count, next_generation + cursor + i);
				mutate_bit_string_2(next_generation + cursor + i, k);
			}
			cursor += count;
			count = object_count * 3 / 10;
			if (count % 2 == 1)
			{
				copy_individual(current_generation + object_count - 1, next_generation + cursor + count - 1);
				count--;
			}
			for (int i = 0; i < count; i += 2)
			{
				crossover(current_generation + i, next_generation + cursor + i, k);
			}
			tmp = current_generation;
			current_generation = next_generation;
			next_generation = tmp;

			for (int i = 0; i < object_count; ++i)
			{
				current_generation[i].index = i;
			}

			if (k % 5 == 0)
			{
				print_best_fitness(current_generation);
			}
		}
		pthread_barrier_wait(barrier);
	}

	pthread_barrier_wait(barrier);
	compute_fitness_function(objects, current_generation, object_count, sack_capacity, start, end);
	pthread_barrier_wait(barrier);

	if (id == 0)
	{
		qsort(current_generation, object_count, sizeof(individual), cmpfunc);
		print_best_fitness(current_generation);

		// free resources for old generation
		free_generation(current_generation);
		free_generation(next_generation);

		// free resources
		free(current_generation);
		free(next_generation);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	// array with all the objects that can be placed in the sack
	int P;
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, argc, argv, &P))
	{
		return 0;
	}

	individual *current_generation = (individual *)calloc(object_count, sizeof(individual));
	individual *next_generation = (individual *)calloc(object_count, sizeof(individual));
	individual *tmp = NULL;

	int r;
	void *status;
	params *arguments = (params *)calloc(P, sizeof(params));
	pthread_t *threads = (pthread_t *)calloc(P, sizeof(pthread_t));
	int *cur_count_table = (int *)calloc(object_count, sizeof(int));
	int *next_count_table = (int *)calloc(object_count, sizeof(int));
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, P);
	for (int i = 0; i < P; i++)
	{
		arguments[i].id = i;
		arguments[i].objects = objects;
		arguments[i].generations_count = generations_count;
		arguments[i].object_count = object_count;
		arguments[i].sack_capacity = sack_capacity;
		arguments[i].mutex = &mutex;
		arguments[i].barrier = &barrier;
		arguments[i].P = P;
		arguments[i].current_generation = current_generation;
		arguments[i].next_generation = next_generation;
		arguments[i].tmp = tmp;
		arguments[i].cur_count_table = cur_count_table;
		arguments[i].next_count_table = next_count_table;
		r = pthread_create(&threads[i], NULL, f, &arguments[i]);

		if (r)
		{
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

	for (int i = 0; i < P; i++)
	{
		r = pthread_join(threads[i], &status);

		if (r)
		{
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}
	pthread_barrier_destroy(&barrier);
	free(objects);

	return 0;
}
