#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <chrono>

#define RANK_MESTRE 0

#define SINGLE_ELEMENT_LENGTH 1
#define NOTHING_FOUND_INDEX -1

#define TAG_LENGTH 1
#define TAG_DATA 2
#define TAG_TARGET 3
#define TAG_SLAVE_WORK_COMPLETE 4
#define TAG_STOP_SEARCH 5

#define SLAVE_SEARCH_BATCH_SIZE 1000

#define TEST_ARRAY_LENGTH 500000000
#define TEST_TARGET_VALUE TEST_ARRAY_LENGTH - 1

// O processo de liberação de memória ajuda com grandes payloads mas
// exige tempo considerável de processamento que não vale a pena com payload razoável
#define EARLY_MEMORY_FREE 0


/**
 * Dados referentes ao ambiente de execução.
 */
class MpiData {

    private:
        int worldSize;
        int processRank;
        char* processorName;

    public:
        MpiData(int worldSize, int processRank, char* processorName) {
            this->worldSize = worldSize;
            this->processRank = processRank;
            this->processorName = processorName;
        }

        ~MpiData() {
            if (this->processorName != NULL) {
                delete this->processorName;
            }
        }

        bool isMasterRank() {
            return this->processRank == RANK_MESTRE;
        }

        static MpiData* load() {
            int worldSize;
            MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

            int processRank;
            MPI_Comm_rank(MPI_COMM_WORLD, &processRank);

            int nameLength;
            char* processorName = new char[MPI_MAX_PROCESSOR_NAME];
            MPI_Get_processor_name(processorName, &nameLength);

            return new MpiData(worldSize, processRank, processorName);
        }

        int getWorldSize() { return this->worldSize; }
        int getSlaveWorldSize() { return this->worldSize - 1; }
        int getProcessRank() { return this->processRank; }
        char* getProcessorName() { return this->processorName; }
};


class Logger {

    public:
        Logger(MpiData* mpiData) {
            this->mpiData = mpiData;
        }

       void info(const char* format, ...) {
           printf("[%s] [rank: %d/%d] ",
                   this->mpiData->getProcessorName(),
                   this->mpiData->getProcessRank(),
                   this->mpiData->getWorldSize());

            va_list argptr;
            va_start(argptr, format);
            vprintf(format, argptr);
            va_end(argptr);

            printf("\n");
       }

    private:
       MpiData* mpiData;
};


class MpiHelper {

    public:
        static int receiveInt(int tag) {
            int value;
            MPI_Recv(&value, SINGLE_ELEMENT_LENGTH, MPI_INT, MPI_ANY_SOURCE, tag,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            return value;
        }

        static int* receiveIntList(int tag, int listLength) {
            int* list = new int[listLength];
            MPI_Recv(list, listLength, MPI_INT, MPI_ANY_SOURCE, tag,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            return list;
        }

        static int receiveIntAsync(int tag) {
            int value;
            MPI_Request r;
            MPI_Irecv(&value, SINGLE_ELEMENT_LENGTH, MPI_INT, MPI_ANY_SOURCE, tag,
                    MPI_COMM_WORLD, &r);
            return value;
        }

        static void sendInt(int tag, int targetRank, int* dataPointer) {
            MpiHelper::sendIntList(tag, targetRank, dataPointer, SINGLE_ELEMENT_LENGTH);
        }

        static void sendIntList(int tag, int targetRank, int* dataPointer, int dataLength) {
            MPI_Send(dataPointer, dataLength, MPI_INT, targetRank, tag, MPI_COMM_WORLD);
        }

        static void sendIntAsync(int tag, int targetRank, int* dataPointer) {
            MpiHelper::sendIntListAsync(tag, targetRank, dataPointer, SINGLE_ELEMENT_LENGTH);
        }

        static void sendIntListAsync(int tag, int targetRank, int* dataPointer, int dataLength) {
            MPI_Request r;
            MPI_Isend(dataPointer, dataLength, MPI_INT, targetRank, tag, MPI_COMM_WORLD, &r);
        }
};


/**
 * Dados de trabalho, incluindo vetor numérico e tamanho do vetor.
 */
class WorkData {

    public:
        WorkData(int searchTarget, int listSize) {
            this->listSize = listSize;
            this->searchTarget = searchTarget;
            this->numberList = new int[listSize];
        }

        WorkData(int searchTarget, int listSize, int* numberList) {
            this->listSize = listSize;
            this->searchTarget = searchTarget;
            this->numberList = numberList;
        }

        ~WorkData() {
            this->freeData();
        }

        static WorkData* loadFullData() {
            WorkData* workData = new WorkData(TEST_TARGET_VALUE, TEST_ARRAY_LENGTH);
            for (int i = 0; i < workData->listSize; i++) {
                workData->numberList[i] = i;
            }
            return workData;
        }

        void freeData() {
            if (this->numberList != NULL) {
                delete this->numberList;
                this->numberList = NULL;
            }
        }

        void sendTo(int targetRank, int offset, int limit) {
            int* offsetPointer = &this->numberList[offset];

            MpiHelper::sendInt(TAG_TARGET, targetRank, &this->searchTarget);
            MpiHelper::sendInt(TAG_LENGTH, targetRank, &this->listSize);
            MpiHelper::sendIntList(TAG_DATA, targetRank, offsetPointer, limit);
        }

        static WorkData* receive() {
            int searchTarget = MpiHelper::receiveInt(TAG_TARGET);
            int listSize = MpiHelper::receiveInt(TAG_LENGTH);
            int* numberList = MpiHelper::receiveIntList(TAG_DATA, listSize);

            return new WorkData(searchTarget, listSize, numberList);
        }

        int getSearchTarget() { return this->searchTarget; }
        int getListSize() { return this->listSize; }
        int* getNumberList() { return this->numberList; }

    private:
        int searchTarget;
        int listSize;
        int* numberList;
};

class SearchCompleteResponse {

    private:
        int senderRank;
        int foundIndex;

    public:
        SearchCompleteResponse(int senderRank, int foundIndex) {
            this->senderRank = senderRank;
            this->foundIndex = foundIndex;
        }

        void sendTo(int targetRank) {
            int* data = new int[2];
            data[0] = this->senderRank;
            data[1] = this->foundIndex;

            MpiHelper::sendIntList(TAG_SLAVE_WORK_COMPLETE, targetRank, data, 2);

            delete data;
        }

        static SearchCompleteResponse* receive() {
            int* data = MpiHelper::receiveIntList(TAG_SLAVE_WORK_COMPLETE, 2);
            SearchCompleteResponse* msg = new SearchCompleteResponse(data[0], data[1]);
            delete data;
            return msg;
        }

        int getSenderRank() { return this->senderRank; }
        int getFoundIndex() { return this->foundIndex; }
};


class Process {

    protected:
        MpiData* mpiData;
        Logger* logger;

    public:
        virtual void run() = 0;

        Process(MpiData* mpiData, Logger* logger) {
            this->mpiData = mpiData;
            this->logger = logger;
        }

};

class MasterProcess : public Process {

    public:
        MasterProcess(MpiData* mpiData, Logger* logger)
                : Process(mpiData, logger) {
        }

        void run() {
            this->logger->info("Processo mestre iniciado");

            WorkData* workData = WorkData::loadFullData();

            std::chrono::steady_clock::time_point startTime = this->now();

            this->dispatchPayload(workData);

            if (EARLY_MEMORY_FREE != 0) {
                workData->freeData();
            }

            std::chrono::steady_clock::time_point dispatchTime = this->now();

            int foundIndex = this->receiveResponse(workData);

            std::chrono::steady_clock::time_point searchTime = this->now();

            this->sendStopMessage();

            std::chrono::steady_clock::time_point finalTime = this->now();

            int dispatchTimeMillis = this->elapsedMillis(startTime, dispatchTime);
            int searchTimeMillis = this->elapsedMillis(dispatchTime, searchTime);
            int stopTimeMillis = this->elapsedMillis(searchTime, finalTime);
            int totalTimeMillis = this->elapsedMillis(startTime, finalTime);
            this->logger->info("[[ Resultado da busca ]] => valorBuscado: %d, indiceResultado: %d, envioDados: %dms, busca: %dms, msgParada: %dms, total: %dms, memoriaLiberadaAntecipadamente: %d",
                    workData->getSearchTarget(),
                    foundIndex,
                    dispatchTimeMillis,
                    searchTimeMillis,
                    stopTimeMillis,
                    totalTimeMillis,
                    EARLY_MEMORY_FREE);

            delete workData;
        }

    private:
        std::chrono::steady_clock::time_point now() {
            return std::chrono::steady_clock::now();
        }

        int elapsedMillis(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
        }

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

        int calculatePageOffset(int rank, int pageSize) {
            return (rank - 1) * pageSize;
        }

        void dispatchPayload(WorkData* workData) {
            int pageSize = this->calculateDefaultPageSize(workData);
            this->logger->info("Enviando dados para escravos => totRegistros: %d, valorBuscado: %d, tamanhoBatch: %d.",
                    workData->getListSize(),
                    workData->getSearchTarget(),
                    pageSize);

            // Envia mensagem para todos escravos exceto o último
            {
                int slaveCount = this->mpiData->getSlaveWorldSize();
                for (int processRank = 1; processRank < slaveCount; processRank++) {

                    int pageOffset = this->calculatePageOffset(processRank, pageSize);
                    workData->sendTo(processRank, pageOffset, pageSize);
                }
            }

            // O último escravo recebe dados adicionais que sobrarem da divisão
            // no cálculo dos lotes
            {
                int lastPageSize = this->calculateLastPageSize(workData);
                int processRank = this->mpiData->getSlaveWorldSize();
                int pageOffset = this->calculatePageOffset(processRank, pageSize);

                workData->sendTo(processRank, pageOffset, lastPageSize);
            }
        }

        int receiveResponse(WorkData* workData) {
            this->logger->info("Aguardando resultado da busca");

            int workingSlaves = mpiData->getSlaveWorldSize();
            while (workingSlaves > 0) {
                SearchCompleteResponse* response = SearchCompleteResponse::receive();

                if (response->getFoundIndex() >= 0) {
                    int absoluteIndex = this->calculateAbsoluteIndex(workData,
                            response->getSenderRank(),
                            response->getFoundIndex());
                    this->logger->info("Escravo encerrado com sucesso! => slaveRank: %d, indiceEncontradoParcial: %d, indiceEncontradoAbsoluto: %d",
                            response->getSenderRank(),
                            response->getFoundIndex(),
                            absoluteIndex);
                    delete response;
                    return absoluteIndex;
                }

                this->logger->info("Escravo encerrado sem sucesso => slaveRank: %d", response->getSenderRank());
                workingSlaves--;
                delete response;
    		}

            return NOTHING_FOUND_INDEX;
        }

        int calculateAbsoluteIndex(WorkData* workData, int processRank, int relativeIndex) {
            int pageSize = this->calculateDefaultPageSize(workData);
            int pageOffset = this->calculatePageOffset(processRank, pageSize);
            return pageOffset + relativeIndex;
        }

        void sendStopMessage() {
            this->logger->info("Enviando mensagem de finalização para escravos.");

            int buff = 1;
            for (int rank = 0; rank < mpiData->getWorldSize(); rank++) {
                MpiHelper::sendInt(TAG_STOP_SEARCH, rank, &buff);
            }
        }

};



class SlaveProcess : public Process {

    public:
        SlaveProcess(MpiData* mpiData, Logger* logger)
                : Process(mpiData, logger) {
        }

        void run() {
            this->logger->info("Processo escravo iniciado");

            WorkData* workData = WorkData::receive();
            this->logger->info("Dados recebidos => searchTarget: %d, qtdItens: %d, primeiroItem: %d, ultimoItem: %d",
                    workData->getSearchTarget(),
                    workData->getListSize(),
                    workData->getNumberList()[0],
                    workData->getNumberList()[ workData->getListSize() - 1 ]);

            int foundIndex = this->findTarget(workData);
            this->sendCompleteMsg(foundIndex);

            delete workData;
        }

    private:
        int findTarget(WorkData* workData) {
            int searchTarget = workData->getSearchTarget();
            int dataLength = workData->getListSize();
            int* data = workData->getNumberList();

            int currentIndex = 0;
            while (true) {

                int lastIndex = this->calculateIterationLastIndex(currentIndex,
                        dataLength);
                for (int index = currentIndex; index < dataLength; index++) {
                    if (data[index] == searchTarget) {
                        return index;
                    }
                    currentIndex = index;
                }

                if (this->shouldStop(currentIndex, dataLength)) {
                    return NOTHING_FOUND_INDEX;
                }
            }
        }

        int calculateIterationLastIndex(int currentIndex, int dataLength) {
            int lastAvailable = dataLength - 1;
            int index = currentIndex + SLAVE_SEARCH_BATCH_SIZE;

            return index < lastAvailable ? index : lastAvailable;
        }

        bool shouldStop(int currentIndex, int dataLength) {
            if (currentIndex >= (dataLength - 1)) {
                return true;
            }
            return 0 != MpiHelper::receiveIntAsync(TAG_STOP_SEARCH);
        }

        void sendCompleteMsg(int foundIndex) {
            SearchCompleteResponse* msg = new SearchCompleteResponse(
                    this->mpiData->getProcessRank(), foundIndex);
            msg->sendTo(RANK_MESTRE);
            delete msg;
        }

};


Process* createProcess(MpiData* mpiData, Logger* logger) {
    if (mpiData->isMasterRank()) {
        return new MasterProcess(mpiData, logger);
    }
    return new SlaveProcess(mpiData, logger);
}


/**
 * main!
 */
int main(int argc, char** argv) {

	MPI_Init(NULL, NULL);

    MpiData* mpiData = MpiData::load();
    Logger* logger = new Logger(mpiData);

    Process* process = createProcess(mpiData, logger);
    process->run();
    delete process;

    logger->info("Processo concluído");

    delete logger;
    delete mpiData;

    MPI_Finalize();
}
