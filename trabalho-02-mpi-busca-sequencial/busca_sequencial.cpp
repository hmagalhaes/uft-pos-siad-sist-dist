#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define RANK_MESTRE 0
#define TAG_OPERACOES 50

#define SLAVE_SEARCH_BATCH_SIZE 100

#define TAG_LENGTH 1
#define TAG_DATA 2
#define TAG_TARGET 3
#define TAG_SLAVE_WORK_COMPLETE 4

#define TEST_ARRAY_LENGTH 5000
#define TEST_TARGET_VALUE 4000


/**
 * Dados referentes ao ambiente de execução.
 */
class MpiData {

    private:
        int worldSize;
        int processRank;
        char* processorName;

    public:
        MpiData() {
            this->processorName = new char[MPI_MAX_PROCESSOR_NAME];
        }

        ~MpiData() {
            delete this->processorName;
        }

        int getWorldSize() {
            return this->worldSize;
        }

        int getSlaveWorldSize() {
            return this->worldSize - 1;
        }

        int getProcessRank() {
            return this->processRank;
        }

        char* getProcessorName() {
            return this->processorName;
        }

        bool isMasterRank() {
            return this->processRank == RANK_MESTRE;
        }

        static MpiData* load() {
            int nameLength;

            MpiData* data = new MpiData();
            MPI_Comm_size(MPI_COMM_WORLD, &data->worldSize);
            MPI_Comm_rank(MPI_COMM_WORLD, &data->processRank);
            MPI_Get_processor_name(data->processorName, &nameLength);
            return data;
        }

};


/**
 * Dados de trabalho, incluindo vetor numérico e tamanho do vetor.
 */
class WorkData {

    private:
        int* numberList;
        int listSize;
        int searchTarget;

    public:
        WorkData(int listSize, int searchTarget) {
            this->listSize = listSize;
            this->searchTarget = searchTarget;
            this->numberList = new int[listSize];
        }

        ~WorkData() {
            delete this->numberList;
        }

        static WorkData* load() {
            WorkData* workData = new WorkData(TEST_ARRAY_LENGTH, TEST_TARGET_VALUE);
            for (int i = 0; i < workData->listSize; i++) {
                workData->numberList[i] = i;
            }
            return workData;
        }

        int* getNumberList() {
            return this->numberList;
        }

        int getListSize() {
            return this->listSize;
        }

        int getSearchTarget() {
          return this->searchTarget;
        }

};


// class Logger {
//
//     private:
//         MpiData* mpiData;
//
//     public:
//         void print(char* msg) {
//             printf("[%s] [rank: %d/%d] %s",
//                     this->mpiData->getProcessorName(),
//                     this->mpiData->getProcessRank(),
//                     this->mpiData->getWorldSize(),
//                     msg);
//         }
//
// }

class MpiHelper {

    public:
        static int receiveInt(int tag) {
            int value;
            MPI_Recv(&value, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE);
            return value;
        }

        static int* receiveIntList(int tag, int listLength) {
            int* list = new int[listLength];
            MPI_Recv(list, listLength, MPI_INT, MPI_ANY_SOURCE, tag,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            return list;
        }

        static void sendIntAsync(int tag, int targetRank, int* dataPointer, int dataLength) {
            MPI_Request r;
            MPI_Isend(dataPointer, dataLength, MPI_INT, targetRank, tag,
                    MPI_COMM_WORLD, &r);
        }

        static void sendInt(int tag, int targetRank, int* dataPointer, int dataLength) {
            MPI_Send(dataPointer, dataLength, MPI_INT, targetRank, tag, MPI_COMM_WORLD);
        }

};


class Process {

    protected:
        MpiData* mpiData;

    public:
        virtual void run() = 0;

        Process(MpiData* mpiData) {
            this->mpiData = mpiData;
        }

};

class MasterProcess : public Process {

    private:
        int calculateLastPageSize(WorkData* workData) {
            int defaultPageSize = this->calculateDefaultPageSize(workData);

            int coveredItems = defaultPageSize * this->mpiData->getSlaveWorldSize();
            if (coveredItems == workData->getListSize()) {
                return defaultPageSize;
            }

            if (coveredItems < workData->getListSize()) {
                int missingItems = workData->getListSize() - coveredItems;
                return defaultPageSize + missingItems;
            }

            int surplus = coveredItems - workData->getListSize();
            return defaultPageSize - surplus;
            // TODO isso não deve ocorrer e deveria lançar erro
        }

        int calculateDefaultPageSize(WorkData* workData) {
            return workData->getListSize() / this->mpiData->getSlaveWorldSize();
        }

        void sendData(int targetRank, int pageOffset, int pageSize, WorkData* workData) {
            int* pageOffsetPointer = &workData->getNumberList()[pageOffset];

            printf("Enviando alvo da busca para escravo => rank: %d, alvo: %d\n",
                    targetRank, workData->getSearchTarget());
            MpiHelper::sendInt(TAG_TARGET, targetRank, &workData->getSearchTarget(), 1);

            printf("Enviando tamanho dos dados para escravo => rank: %d, dataLength: %d\n",
                    targetRank, pageSize);
            MpiHelper::sendIntAsync(TAG_LENGTH, targetRank, &pageSize, 1);

            printf("Enviando dados para escravo => rank: %d, listStartPointer: %d\n",
                    targetRank, pageOffsetPointer);
            MpiHelper::sendIntAsync(TAG_DATA, targetRank, pageOffsetPointer, pageSize);
        }

        int calculatePageOffset(int rank, int pageSize) {
            return (rank - 1) * pageSize;
        }

    public:
        MasterProcess(MpiData* mpiData) : Process(mpiData) {
        }

        void run() {
            printf("[%s] [%d]/[%d] Processo mestre iniciado.\n",
                    this->mpiData->getProcessorName(),
                    this->mpiData->getProcessRank(),
                    this->mpiData->getWorldSize());

            WorkData* workData = WorkData::load();

            int pageSize = this->calculateDefaultPageSize(workData);
            printf("[%s] [rank: %d/%d] Enviando dados para escravos => totalRegistros: %d, valorBuscado: %d, tamanhoBatch: %d.\n",
                    this->mpiData->getProcessorName(),
                    this->mpiData->getProcessRank(),
                    this->mpiData->getWorldSize(),
                    workData->getListSize(),
                    workData->getSearchTarget(),
                    pageSize);

            // Envia mensagem para todos escravos exceto o último
            {
                int slaveCount = this->mpiData->getSlaveWorldSize();
                for (int processRank = 1; processRank < slaveCount; processRank++) {

                    int pageOffset = this->calculatePageOffset(processRank, pageSize);
                    this->sendData(processRank, pageOffset, pageSize, workData);
                }
            }

            // O último escravo recebe dados adicionais que sobrarem da divisão
            // no cálculo dos lotes
            {
                int lastPageSize = this->calculateLastPageSize(workData);
                int processRank = this->mpiData->getSlaveWorldSize();
                int pageOffset = this->calculatePageOffset(processRank, pageSize);

                this->sendData(processRank, pageOffset, lastPageSize, workData);
            }

            delete workData;

            printf("[%s] [rank: %d/%d] Aguardando resultado da busca.\n",
                    this->mpiData->getProcessorName(),
                    this->mpiData->getProcessRank(),
                    this->mpiData->getWorldSize());

    		for(int i=1; i<mpiData->getWorldSize(); i++) {
                int* response = MpiHelper::receiveIntList(TAG_SLAVE_WORK_COMPLETE, 3);
                int processRank = response[0];
                int partialIndex = response[1];
                int found = response[2];

                int pageOffset = this->calculatePageOffset(processRank, pageSize);
                int realIndex = pageOffset + partialIndex;

                printf("[%s] [rank: %d/%d] Resultado recebido => slaveRank: %d, partialIndex: %d, realIndex: %d, found: %d\n",
                        this->mpiData->getProcessorName(),
                        this->mpiData->getProcessRank(),
                        this->mpiData->getWorldSize(),
                        processRank,  // rank
                        partialIndex,  // index
                        realIndex,
                        found  // resultado
                        );


        		MPI_Recv(&response[0], 2, MPI_INT, i, TAG_OPERACOES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("[%s] [%d]/[%d] Resultado de %d: %d.\n", mpiData->getProcessorName()
                        mpiData->getProcessRank(), mpiData->getWorldSize(), response[0], response[1]);
    		}
        }

};


class SlaveProcess : public Process {

    private:
        void findTarget(int dataLength, int* data, int searchTarget) {
            int currentIndex = 0;
            while (true) {

                int lastIndex = this->calculateIterationLastIndex(currentIndex, dataLength);
                for (int index = currentIndex; index < dataLength; index++) {
                    if (data[index] == searchTarget) {
                        this->publishTargetFound(index);
                        return;
                    }
                    currentIndex = index;
                }

                if (this->shouldStop()) {
                    return;
                }
            }
        }

        int calculateIterationLastIndex(int currentIndex, int dataLength) {
            int lastAvailable = dataLength - 1;
            int index = currentIndex + SLAVE_SEARCH_BATCH_SIZE;

            return index < lastAvailable ? index : lastAvailable;
        }

        bool shouldStop() {
            return 0 != MpiHelper::receiveIntAsync(TAG_STOP);
        }

        void publishTargetFound(int index) {
            int[2] data = { this->mpiData->getProcessRank(), index };
            MpiHelper::sendInt(TAG_SLAVE_WORK_COMPLETE, RANK_MESTRE, &data, 2);
        }

    public:
        SlaveProcess(MpiData* mpiData) : Process(mpiData) { }

        void run() {
            printf("[%s] [%d]/[%d] Processo escravo iniciado.\n",
                    this->mpiData->getProcessorName(),
                    this->mpiData->getProcessRank(),
                    this->mpiData->getWorldSize());

            int searchTarget = MpiHelper::receiveInt(TAG_TARGET);
            int dataLength = MpiHelper::receiveInt(TAG_LENGTH);
            int* data = MpiHelper::receiveIntList(TAG_DATA, dataLength);

            printf("[%s] [%d]/[%d] Dados recebidos => itens: %d, primeiroItem: %d, ultimoItem: %d\n",
                    this->mpiData->getProcessorName(),
                    this->mpiData->getProcessRank(),
                    this->mpiData->getWorldSize(),
                    dataLength,
                    data[0],
                    data[dataLength - 1]);

            this->findTarget(dataLength, data, searchTarget);

            delete data;
        }

};


Process* createProcess(MpiData* mpiData) {
    if (mpiData->isMasterRank()) {
        return new MasterProcess(mpiData);
    }
    return new SlaveProcess(mpiData);
}


/**
 * main!
 */
int main(int argc, char** argv) {

	// Initialize the MPI environment.
	MPI_Init(NULL, NULL);

    MpiData* mpiData = MpiData::load();

    Process* process = createProcess(mpiData);
    process->run();
    delete process;

    delete mpiData;

        // int numbers[2];
        // int response[2];
		// MPI_Recv(&numbers, 2, MPI_INT, RANK_MESTRE, TAG_OPERACOES,
        //     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // printf("[%s] [%d]/[%d] Dados recebidos => %d, %d.\n",
        //         mpiData->getProcessorName(), mpiData->getProcessRank(), mpiData->getWorldSize(), numbers[0], numbers[1]);
        //
		// if (mpiData->getProcessRank() == 1) {
        //     // Escravo 1 calcula soma dos valores recebidos
		// 	response[0] = 1;
		// 	response[1] = numbers[0] + numbers[1];
		// } else if(mpiData->getProcessRank() == 2) {
        //     // Escravo 2 calcula subtracao dos valores recebidos
		// 	response[0] = 2;
		// 	response[1] = numbers[0] - numbers[1];
		// } else if(mpiData->getProcessRank() == 3) {
        //     // Escravo 1 calcula produto dos valores recebidos
		// 	response[0] = 3;
		// 	response[1] = numbers[0] * numbers[1];
		// }
        //
        // printf("[%s] [%d]/[%d] Enviando resposta => %d, %d.\n",
        //         mpiData->getProcessorName(), mpiData->getProcessRank(), mpiData->getWorldSize(), response[0], response[1]);
		// // Escravo envia resposta para o mestre
		// MPI_Send(&response[0], 2, MPI_INT, RANK_MESTRE, TAG_OPERACOES, MPI_COMM_WORLD);


    // Finalize the MPI environment. No more MPI calls can be made after this
    MPI_Finalize();
}
