/*
 * APD - Tema 1
 * Octombrie 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

pthread_barrier_t barrier;
char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;
int NUM_THREADS;
int **result;
// structura pentru un numar complex
typedef struct _complex
{
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params
{
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;
// definesc par ca global
params par;

// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 6)
	{
		printf("Numar insuficient de parametri:\n\t"
			   "./tema1 fisier_intrare_julia fisier_iesire_julia "
			   "fisier_intrare_mandelbrot fisier_iesire_mandelbrot N\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	NUM_THREADS = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params *par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL)
	{
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
		   &par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia)
	{
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL)
	{
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int *));
	if (result == NULL)
	{
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++)
	{
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL)
		{
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++)
	{
		free(result[i]);
	}
	free(result);
}

// ruleaza algoritmul Julia
void run_julia(params *par, int **result, int width, int height)
{
	int w, h, i;

	for (w = 0; w < width; w++)
	{
		for (h = 0; h < height; h++)
		{
			int step = 0;
			complex z = {.a = w * par->resolution + par->x_min,
						 .b = h * par->resolution + par->y_min};

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations)
			{
				complex z_aux = {.a = z.a, .b = z.b};

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}

	// transforma rezultatul din coordonate matematice in coordonate ecran
	for (i = 0; i < height / 2; i++)
	{
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}
}

// ruleaza algoritmul Mandelbrot
void run_mandelbrot(params *par, int **result, int width, int height)
{
	int w, h, i;

	for (w = 0; w < width; w++)
	{
		for (h = 0; h < height; h++)
		{
			complex c = {.a = w * par->resolution + par->x_min,	 // what tf is this???
						 .b = h * par->resolution + par->y_min}; // i get it now, literally every complex point within the stated rezolution
			complex z = {.a = 0, .b = 0};						 // with such good coding, how tf am i soposed to do NNN?
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations)
			{
				complex z_aux = {.a = z.a, .b = z.b};

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}

	// transforma rezultatul din coordonate matematice in coordonate ecran
	for (i = 0; i < height / 2; i++)
	{
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}
}

void *f(void *arg)
{
	int thread_id = *(int *)arg;
	int w, h, i, width, height, start, end, h_height;

	if (thread_id == 0)
	{
		read_input_file(in_filename_julia, &par);
		width = (par.x_max - par.x_min) / par.resolution;
		height = (par.y_max - par.y_min) / par.resolution;
		result = allocate_memory(width, height);
	}

	pthread_barrier_wait(&barrier);

	start = thread_id * (double)width / NUM_THREADS;
	end = (thread_id + 1) * (double)width / NUM_THREADS;
	if (end > width)
		end = width;

	for (w = start; w < end; w++)
	{
		for (h = 0; h < height; h++)
		{
			int step = 0;
			complex z = {.a = w * par.resolution + par.x_min,
						 .b = h * par.resolution + par.y_min};

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par.iterations)
			{
				complex z_aux = {.a = z.a, .b = z.b};

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par.c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par.c_julia.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier);

	// transforma rezultatul din coordonate matematice in coordonate ecran
	h_height = height / 2;
	start = thread_id * (double)h_height / NUM_THREADS;
	end = (thread_id + 1) * (double)h_height / NUM_THREADS;
	if (end > h_height)
		end = h_height;

	for (i = start; i < end; i++)
	{
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}

	if (thread_id == 0)
	{
		free_memory(result, height);
	}

	pthread_barrier_wait(&barrier);

	if (thread_id == 0)
	{
		read_input_file(in_filename_mandelbrot, &par);
		width = (par.x_max - par.x_min) / par.resolution;
		height = (par.y_max - par.y_min) / par.resolution;
		result = allocate_memory(width, height);
	}

	pthread_barrier_wait(&barrier);
	start = thread_id * (double)width / NUM_THREADS;
	end = (thread_id + 1) * (double)width / NUM_THREADS;
	if (end > width)
		end = width;

	for (w = start; w < end; w++)
	{
		for (h = 0; h < height; h++)
		{
			complex c = {.a = w * par.resolution + par.x_min,
						 .b = h * par.resolution + par.y_min};
			complex z = {.a = 0, .b = 0};
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par.iterations)
			{
				complex z_aux = {.a = z.a, .b = z.b};

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}
	pthread_barrier_wait(&barrier);

	// transforma rezultatul din coordonate matematice in coordonate ecran
	h_height = height / 2;
	start = thread_id * (double)h_height / NUM_THREADS;
	end = (thread_id + 1) * (double)h_height / NUM_THREADS;
	if (end > h_height)
		end = h_height;

	for (i = start; i < end; i++)
	{
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}

	if (thread_id == 0)
	{
		free_memory(result, height);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

	int i, r;
	void *status;
	// se citesc argumentele programului
	get_args(argc, argv);
	pthread_t threads[NUM_THREADS];
	int arguments[NUM_THREADS];

	pthread_barrier_init(&barrier, NULL, NUM_THREADS);
	for (i = 0; i < NUM_THREADS; i++)
	{
		arguments[i] = i;
		r = pthread_create(&threads[i], NULL, f, &arguments[i]);

		if (r)
		{
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

	for (i = 0; i < NUM_THREADS; i++)
	{
		r = pthread_join(threads[i], &status);

		if (r)
		{
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}
	pthread_barrier_destroy(&barrier);

	return 0;
}
