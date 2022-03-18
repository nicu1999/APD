#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void read_children(int* nrChildren, int**  children, char* filename){
    FILE *fp;
    int child;
    int nr;
    fp = fopen(filename, "r");
    fscanf(fp, "%d", &nr);
    *children = malloc(sizeof(int) * nr);

    for(int i = 0; i < nr; i++){
        fscanf(fp, "%d", &child);
        (*children)[i] = child;
    }

    *nrChildren = nr; 
}


void M(int from, int to){
    printf("M(%d,%d)\n", from, to);
}

void print_topology( 
    int rank,   
    int nrChildren0,
    int nrChildren1,
    int nrChildren2,
	int* children0,
    int* children1,
    int* children2 ){
    
    printf("%d -> ", rank);
    printf("0:");
    for(int i = 0; i < nrChildren0; i++){
        if(i != nrChildren0 - 1)
            printf("%d,", children0[i]);
        else
            printf("%d ", children0[i]);
    }
    printf("1:");
    for(int i = 0; i < nrChildren1; i++){
        if(i != nrChildren1 - 1)
            printf("%d,", children1[i]);
        else
            printf("%d ", children1[i]);
    }
    printf("2:");
    for(int i = 0; i < nrChildren2; i++){
        if(i != nrChildren2 - 1)
            printf("%d,", children2[i]);
        else
            printf("%d ", children2[i]);
    }
    printf("\n");

}

void send_vector(int from, int to, int size, int *vector){
    MPI_Send(&size, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
    M(from, to);
    MPI_Send(vector, size, MPI_INT, to, 0, MPI_COMM_WORLD);
    M(from, to);
}

void recv_vector(int from, int* size, int** vect, MPI_Status* status){
    MPI_Recv(size, 1, MPI_INT, from, 0, MPI_COMM_WORLD, status);
    *vect = malloc(sizeof(int) * (*size));
    MPI_Recv(*vect, *size, MPI_INT, from, 0, MPI_COMM_WORLD, status);
}

void recv_vector_processed(int from, int* size, int* vect, MPI_Status* status){
    MPI_Recv(size, 1, MPI_INT, from, 0, MPI_COMM_WORLD, status);
    MPI_Recv(vect, *size, MPI_INT, from, 0, MPI_COMM_WORLD, status);
}


void print_vector(int *v, int n){
    printf("Rezultat: ");
    for(int i = 0; i < n; i++){
        if(i != n - 1)
            printf("%d ", v[i]);
        else
            printf("%d\n", v[i]);
    }
}

void process_vector(int from, int nrChildren, int* children, int vec_chunck, int* vec, MPI_Status* status, int n){
    int np;
    int las_chunck = n - (nrChildren - 1) * vec_chunck;
    for(int i = 0; i < nrChildren; i++){
        if(i != nrChildren - 1)
            send_vector(from, children[i], vec_chunck, (vec +  vec_chunck * i ) );
        else
            send_vector(from, children[i], las_chunck, (vec +  vec_chunck * i ) );

    }
    for(int i = 0; i < nrChildren; i++){
        if (i != nrChildren - 1)
            recv_vector_processed(children[i], &np, vec +  vec_chunck * i , status);
        else
            recv_vector_processed(children[i], &np, vec +  vec_chunck * i , status);
    }
}

int main(int argc, char *argv[])
{
	int rank, nProcesses, num_procs, leader, rankP, n, vec_chunck, np;
    int nrChildren0;
    int nrChildren1;
    int nrChildren2;
	int* children0;
    int* children1;
    int* children2;
    int* vec;

	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

    if(rank < 3){
        if(rank == 0){
            read_children(&nrChildren0, &children0, "cluster0.txt");
            
            send_vector(0, 1, nrChildren0, children0);
            recv_vector(2, &nrChildren2, &children2, &status);
            send_vector(0, 2, nrChildren0, children0);
            recv_vector(1, &nrChildren1, &children1, &status);

            print_topology(0, nrChildren0, nrChildren1, nrChildren2, children0, children1, children2);

            for(int i = 0; i < nrChildren0; i++){
                MPI_Send(&rank, 1, MPI_INT, children0[i], 0, MPI_COMM_WORLD);
                M(0, children0[i]);
                send_vector(0, children0[i], nrChildren0, children0);
                send_vector(0, children0[i], nrChildren1, children1);
                send_vector(0, children0[i], nrChildren2, children2);
            }

            n = atoi(argv[1]);

            vec = malloc(sizeof(int) * n);
            for(int i = 0; i < n; i++){
               vec[i] = i; 
            }
            vec_chunck = n / (nrChildren0 + nrChildren1 + nrChildren2);
            send_vector(0, 1, vec_chunck * nrChildren1, (vec +  vec_chunck * nrChildren0 ) );
            send_vector(0, 2, vec_chunck * nrChildren2, (vec +  vec_chunck * (nrChildren0 + nrChildren1) ) );
    
            process_vector(0, nrChildren0, children0, vec_chunck, vec, &status, n);
            
            recv_vector_processed(1, &np, vec +  vec_chunck * nrChildren0, &status);
            recv_vector_processed(2, &np, vec +  vec_chunck * (nrChildren0 + nrChildren1), &status);
            
            sleep(1);
            print_vector(vec, n);
            
        }

        if(rank == 1){
            read_children(&nrChildren1, &children1, "cluster1.txt");
            recv_vector(0, &nrChildren0, &children0, &status);
            send_vector(1, 2, nrChildren1, children1);
            recv_vector(2, &nrChildren2, &children2, &status);
            send_vector(1, 0, nrChildren1, children1);
            print_topology(1, nrChildren0, nrChildren1, nrChildren2, children0, children1, children2);

            for(int i = 0; i < nrChildren1; i++){
                MPI_Send(&rank, 1, MPI_INT, children1[i], 0, MPI_COMM_WORLD);
                M(1, children1[i]);
                send_vector(1, children1[i], nrChildren0, children0);
                send_vector(1, children1[i], nrChildren1, children1);
                send_vector(1, children1[i], nrChildren2, children2);
            }

            recv_vector(0, &n, &vec, &status);
            vec_chunck = n / nrChildren1;

            for(int i = 0; i < nrChildren1; i++){
                send_vector(1, children1[i], vec_chunck, (vec +  vec_chunck * i ) );
            }
            for(int i = 0; i < nrChildren1; i++){
                recv_vector_processed(children1[i], &np, vec +  vec_chunck * i , &status);
            }    
                    
            send_vector(1, 0, n, vec);

        }

        if(rank == 2){
            read_children(&nrChildren2, &children2, "cluster2.txt");
            recv_vector(1, &nrChildren1, &children1, &status);
            send_vector(2, 0, nrChildren2, children2);
            recv_vector(0, &nrChildren0, &children0, &status); 
            send_vector(2, 1, nrChildren2, children2);

            print_topology(2, nrChildren0, nrChildren1, nrChildren2, children0, children1, children2);

            for(int i = 0; i < nrChildren2; i++){
                MPI_Send(&rank, 1, MPI_INT, children2[i], 0, MPI_COMM_WORLD);
                M(2, children2[i]);

                send_vector(2, children2[i], nrChildren0, children0);
                send_vector(2, children2[i], nrChildren1, children1);
                send_vector(2, children2[i], nrChildren2, children2);
            }

            recv_vector(0, &n, &vec, &status);
            vec_chunck = n /  nrChildren2;
            for(int i = 0; i < nrChildren2; i++){
                send_vector(2, children2[i], vec_chunck, (vec +  vec_chunck * i ) );
            }
            for(int i = 0; i < nrChildren2; i++){
                recv_vector_processed(children2[i], &np, vec +  vec_chunck * i , &status);
            }

            send_vector(2, 0, n, vec); 
        }
    } else{ //toate celelate noduri

        MPI_Recv(&rankP, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        recv_vector(rankP, &nrChildren0, &children0, &status); //primeste topologia
        recv_vector(rankP, &nrChildren1, &children1, &status);
        recv_vector(rankP, &nrChildren2, &children2, &status);
        print_topology(rank, nrChildren0, nrChildren1, nrChildren2, children0, children1, children2);

        recv_vector(rankP, &n, &vec, &status); //vectorul de prelucrat

        for(int i = 0; i < n; i++){
            vec[i] = 2 * vec[i]; 
        }

        send_vector(rank, rankP, n, vec); // trimite inapoi la parinte
    }

	MPI_Finalize();
	return 0;
}