#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <chrono>

#define TEST_ARRAY_LENGTH 500000000
#define TEST_TARGET_VALUE TEST_ARRAY_LENGTH - 1

#define NOTHING_FOUND_INDEX -1


class Logger {

    public:
        Logger() {
        }

       void info(const char* format, ...) {
            va_list argptr;
            va_start(argptr, format);
            vprintf(format, argptr);
            va_end(argptr);

            printf("\n");
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
            if (this->numberList != NULL) {
                delete this->numberList;
            }
        }

        static WorkData* loadFullData() {
            WorkData* workData = new WorkData(TEST_TARGET_VALUE, TEST_ARRAY_LENGTH);
            for (int i = 0; i < workData->listSize; i++) {
                workData->numberList[i] = i;
            }
            return workData;
        }

        int getSearchTarget() { return this->searchTarget; }
        int getListSize() { return this->listSize; }
        int* getNumberList() { return this->numberList; }

    private:
        int searchTarget;
        int listSize;
        int* numberList;
};


class Process {

    protected:
        Logger* logger;

    public:
        virtual void run() = 0;

        Process(Logger* logger) {
            this->logger = logger;
        }

};

class MasterProcess : public Process {

    public:
        MasterProcess(Logger* logger) : Process(logger) {
        }

        void run() {
            this->logger->info("Processo único iniciado.");

            WorkData* workData = WorkData::loadFullData();

            this->logger->info("Buscando => valor: %d, totItems: %d",
                    workData->getSearchTarget(), workData->getListSize());

            std::chrono::steady_clock::time_point startTime = this->now();

            int foundIndex = this->findTarget(workData);

            std::chrono::steady_clock::time_point searchTime = this->now();

            int searchTimeMillis = this->elapsedMillis(startTime, searchTime);
            this->logger->info("[[ Resultado da busca ]] => valorBuscado: %d, indiceResultado: %d, busca: %dms",
                    workData->getSearchTarget(),
                    foundIndex,
                    searchTimeMillis);

            delete workData;
        }

    private:
        std::chrono::steady_clock::time_point now() {
            return std::chrono::steady_clock::now();
        }

        int elapsedMillis(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
        }

        int findTarget(WorkData* workData) {
            int searchTarget = workData->getSearchTarget();
            int dataLength = workData->getListSize();
            int* data = workData->getNumberList();

            for (int index = 0; index < dataLength; index++) {
                if (data[index] == searchTarget) {
                    return index;
                }
            }
            return NOTHING_FOUND_INDEX;
        }

};


/**
 * main!
 */
int main(int argc, char** argv) {

    Logger* logger = new Logger();

    Process* process = new MasterProcess(logger);
    process->run();
    delete process;

    logger->info("Processo concluído");

    delete logger;
}
