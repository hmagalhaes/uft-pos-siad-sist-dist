#include <mpi.h>
#include <stdio.h>
#define RANK_MESTRE 0
#define TAG_OPERACOES 50

int main(int argc, char** argv) {

	// Initialize the MPI environment. The two arguments to MPI Init are not
	// currently used by MPI implementations, but are there in case future
	// implementations might need the arguments.
	MPI_Init(NULL, NULL);

	// Get the number of processes
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Get the rank of the process
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	// Get the name of the processor
	int name_len;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Get_processor_name(processor_name, &name_len);


	int numbers[2];
	int response[2];
	if(world_rank == RANK_MESTRE)
	{
		// Define valores a serem enviados
		numbers[0] = 10;
		numbers[1] = 20;

		// Envia os valores para cada um dos escravos
		for(int i=1; i<world_size; i++)
		{
    		MPI_Send(&numbers[0], 2, MPI_INT, i, TAG_OPERACOES, MPI_COMM_WORLD);
		}
		printf("Processo mestre executando em %s.\n", processor_name);

		// Recebe o resultado processado por cada um dos escravos
		for(int i=1; i<world_size; i++)
		{
    		MPI_Recv(&response[0], 2, MPI_INT, i, TAG_OPERACOES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    		printf("Resultado de %d: %d.\n", response[0], response[1]);
		}
	}
	else
	{
		MPI_Recv(&numbers[0], 2, MPI_INT, RANK_MESTRE, TAG_OPERACOES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Processo escravo %d executando em %s recebeu %d e %d.\n", world_rank, processor_name, numbers[0], numbers[1]);

		// Escravo 1 calcula soma dos valores recebidos
		if(world_rank == 1)
		{
			response[0] = 1;
			response[1] = numbers[0] + numbers[1];
		}

		// Escravo 2 calcula subtracao dos valores recebidos
		else if(world_rank == 2)
		{
			response[0] = 2;
			response[1] = numbers[0] - numbers[1];
		}

		// Escravo 1 calcula produto dos valores recebidos
		else if(world_rank == 3)
		{
			response[0] = 3;
			response[1] = numbers[0] * numbers[1];
		}

		// Escravo envia resposta para o mestre
		MPI_Send(&response[0], 2, MPI_INT, RANK_MESTRE, TAG_OPERACOES, MPI_COMM_WORLD);
	}

	// Finalize the MPI environment. No more MPI calls can be made after this
	MPI_Finalize();
}
