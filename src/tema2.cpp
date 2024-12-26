#include <cstddef>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <mpi.h>
#include <pthread.h>
#include <stdlib.h>
#include <string>

using namespace std;

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;
    
    return NULL;
}

void *upload_thread_func(void *arg)
{
    int rank = *(int*) arg;

    return NULL;
}

void tracker(int numtasks, int rank) {

}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    // Reading local files
    string inputFile = "test1/in" + to_string(rank) + ".txt";
    ifstream input(inputFile);

    map<string, vector<string>> files;
    int numberOfFiles;
    if (input.is_open()) {
        input >> numberOfFiles;

        for (size_t i = 0; i < numberOfFiles; ++i) {
            string fileName;
            int nOfSegments;
            input >> fileName >> nOfSegments;

            for (size_t i = 0; i < nOfSegments; ++i) {
                string segment;
                input >> segment;
                files[fileName].push_back(segment);
            }
        }
    }
    
    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &rank);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
