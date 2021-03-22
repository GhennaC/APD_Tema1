/*
 * APD - Tema 1
 * Octombrie 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#define min(x, y) (((x) < (y)) ? (x) : (y))



char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;

// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

int P;
pthread_barrier_t barrier;
params par_j;
params par_m;
int width_j, height_j,width_m, height_m;
int **result_j,**result_m;

// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	P = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
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

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
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

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}

void  *thread_function(void *arg) {
//Julia
	
	int thread_id = *(int *)arg;
		
	int w, h, i;
	int width,height;

	width = width_j;
	height = height_j;

	int start,end;
	start = thread_id*(double)width/P;
	end = min((thread_id + 1)*(double)width/P,width);
	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			int step = 0;
			complex z = { .a = w * par_j.resolution + par_j.x_min,
							.b = h * par_j.resolution + par_j.y_min };

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par_j.iterations) {
				complex z_aux = { .a = z.a, .b = z.b };
				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par_j.c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par_j.c_julia.b;
				step++;
			}

			result_j[h][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier);
		// transforma rezultatul din coordonate matematice in coordonate ecran
	start = thread_id*(double)(height/2)/P;
	end = min((thread_id + 1)*(double)(height/2)/P,(height/2));
	for (i = start; i < end; i++) {
		int *aux = result_j[i];
		result_j[i] = result_j[height - i - 1];
		result_j[height - i - 1] = aux;
	}
	pthread_barrier_wait(&barrier);

	write_output_file(out_filename_julia, result_j, width_j, height_j);

	
//Mandelbot
	
	width = width_m;
	height = height_m;

	start = thread_id*(double)width/P;
	end = min((thread_id + 1)*(double)width/P,width);

	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			complex c = { .a = w * par_m.resolution + par_m.x_min,
							.b = h * par_m.resolution + par_m.y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par_m.iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;
				
				step++;
			}

			result_m[h][w] = step % 256;
		}
	}
	pthread_barrier_wait(&barrier);

		// transforma rezultatul din coordonate matematice in coordonate ecran
	start = thread_id*(double)(height/2)/P;
	end = min((thread_id + 1)*(double)(height/2)/P,(height/2));
	for (i = start; i < end; i++) {
		int *aux = result_m[i];
		result_m[i] = result_m[height - i - 1];
		result_m[height - i - 1] = aux;
	}
	pthread_barrier_wait(&barrier);

	write_output_file(out_filename_mandelbrot, result_m, width_m, height_m);

	pthread_exit(NULL);

	
}

int main(int argc, char *argv[])
{
	

	// se citesc argumentele programului
	get_args(argc, argv);
	int i;
	pthread_t tid[P];
	int thread_id[P];
	pthread_barrier_init(&barrier,NULL,P);
	// Julia:
	// - se citesc parametrii de intrare
	// - se aloca tabloul cu rezultatul
	// - se ruleaza algoritmul
	// - se scrie rezultatul in fisierul de iesire
	// - se elibereaza memoria alocata
	read_input_file(in_filename_julia, &par_j);
	width_j = (par_j.x_max - par_j.x_min) / par_j.resolution;
	height_j = (par_j.y_max - par_j.y_min) / par_j.resolution;
	result_j = allocate_memory(width_j, height_j);

	// Mandelbrot:
	// - se citesc par_mametrii de intrare
	// - se aloca tabloul cu rezultatul
	// - se ruleaza algoritmul
	// - se scrie rezultatul in fisierul de iesire
	// - se elibereaza memoria alocata
	read_input_file(in_filename_mandelbrot, &par_m);
	width_m = (par_m.x_max - par_m.x_min) / par_m.resolution;
	height_m = (par_m.y_max - par_m.y_min) / par_m.resolution;
	result_m = allocate_memory(width_m, height_m);
	
	for (i = 0; i < P; i++) {
		thread_id[i] = i;
		pthread_create(&tid[i], NULL, thread_function, &thread_id[i]);
	}

	for (i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}
	pthread_barrier_destroy(&barrier);

	free_memory(result_j, height_j);
	free_memory(result_m, height_m);

	pthread_exit(NULL);

	return 0;
}
