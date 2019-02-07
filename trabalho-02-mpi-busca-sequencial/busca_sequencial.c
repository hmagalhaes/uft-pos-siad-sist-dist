#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define RANK_MESTRE 0
#define TAG_OPERACOES 50
#define ARRAY_LENGTH 50000;





int getWorldSize() {
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    return size;
}

int getProcessRank() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
}

char* getProcessorName() {
    int nameLength;
    char* name = malloc(MPI_MAX_PROCESSOR_NAME);
    MPI_Get_processor_name(name, &nameLength);
    return name;
}

// char[MPI_MAX_PROCESSOR_NAME] getProcessorName() {
//     return null;
// }


int main(int argc, char** argv) {

	// Initialize the MPI environment. The two arguments to MPI Init are not
	// currently used by MPI implementations, but are there in case future
	// implementations might need the arguments.
	MPI_Init(NULL, NULL);

	const int worldSize = getWorldSize();
	const int processRank = getProcessRank();
    char* processorName = getProcessorName();

	// Get the name of the processor
	// int name_len;
	// char processorName[MPI_MAX_PROCESSOR_NAME];
    // const char* processorName = malloc(MPI_MAX_PROCESSOR_NAME);
	// MPI_Get_processor_name(processorName, &name_len);
	// int[3] inputData = loadInput();

	if (processRank == RANK_MESTRE) {
        printf("[%s] [%d]/[%d] Processo mestre executando.\n", processorName,
                processRank, worldSize);

		// Define valores a serem enviados
        int numbers[2];
        int response[2];
		numbers[0] = 10;
		numbers[1] = 20;

		// Envia os valores para cada um dos escravos
		for(int i=1; i<worldSize; i++) {
    		MPI_Send(&numbers[0], 2, MPI_INT, i, TAG_OPERACOES, MPI_COMM_WORLD);
		}

        printf("[%s] [%d]/[%d] Comandos enviados para escravos.\n", processorName,
                processRank, worldSize);

		// Recebe o resultado processado por cada um dos escravos
		for(int i=1; i<worldSize; i++) {
    		MPI_Recv(&response[0], 2, MPI_INT, i, TAG_OPERACOES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("[%s] [%d]/[%d] Resultado de %d: %d.\n", processorName,
                    processRank, worldSize, response[0], response[1]);
		}
	} else {
        printf("[%s] [%d]/[%d] Processo escravo iniciado.\n",
                processorName, processRank, worldSize);

        int numbers[2];
        int response[2];
		MPI_Recv(&numbers[0], 2, MPI_INT, RANK_MESTRE, TAG_OPERACOES,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("[%s] [%d]/[%d] Dados recebidos => %d, %d.\n",
                processorName, processRank, worldSize, numbers[0], numbers[1]);

		if (processRank == 1) {
            // Escravo 1 calcula soma dos valores recebidos
			response[0] = 1;
			response[1] = numbers[0] + numbers[1];
		} else if(processRank == 2) {
            // Escravo 2 calcula subtracao dos valores recebidos
			response[0] = 2;
			response[1] = numbers[0] - numbers[1];
		} else if(processRank == 3) {
            // Escravo 1 calcula produto dos valores recebidos
			response[0] = 3;
			response[1] = numbers[0] * numbers[1];
		}

        printf("[%s] [%d]/[%d] Enviando resposta => %d, %d.\n",
                processorName, processRank, worldSize, response[0], response[1]);
		// Escravo envia resposta para o mestre
		MPI_Send(&response[0], 2, MPI_INT, RANK_MESTRE, TAG_OPERACOES, MPI_COMM_WORLD);
	}

    free(processorName);
	// Finalize the MPI environment. No more MPI calls can be made after this
	MPI_Finalize();

    return 0;
}
