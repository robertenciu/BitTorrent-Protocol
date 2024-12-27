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
#define ACK 1212
#define DOWNLOAD 1
#define UPLOAD 2

// Client
char filesWanted[MAX_FILES][MAX_FILENAME];
int numberOfFilesWanted = 0;

void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;

    map<string, vector<int>>  swarms;
    MPI_Send(&numberOfFilesWanted, 1, MPI_INT, TRACKER_RANK, DOWNLOAD, MPI_COMM_WORLD);
    for (size_t i = 0; i < numberOfFilesWanted; ++i) {
        MPI_Send(&filesWanted[i], MAX_FILENAME, MPI_CHAR, TRACKER_RANK, DOWNLOAD, MPI_COMM_WORLD);
        int numberOfSeeds;
        MPI_Recv(&numberOfSeeds, 1, MPI_INT, TRACKER_RANK, DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (std::size_t i = 0; i < numberOfSeeds; i++) {
            int seeder;
            MPI_Recv(&seeder, 1, MPI_INT, TRACKER_RANK, DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            swarms[filesWanted[i]].push_back(seeder);
        }
        int nrOfHashes = 0;
        MPI_Recv(&nrOfHashes, 1, MPI_INT, TRACKER_RANK, DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < nrOfHashes; ++i) {
            char segment[HASH_SIZE];
            MPI_Recv(&segment, HASH_SIZE, MPI_CHAR, TRACKER_RANK, DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            cout << segment << endl;
        }
    }

    return NULL;
}

void *upload_thread_func(void *arg)
{
    int rank = *(int*) arg;


    return NULL;
}

void tracker(int numtasks, int rank) {
    map<string,  vector<string>> hashes;
    map<string, vector<int>>  swarms;

    for (size_t peer = 1; peer < numtasks; ++peer) {
        int nr = 0;
        MPI_Recv(&nr, 1, MPI_INT, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (size_t i = 0; i < nr; i++) {
            char fileName[MAX_FILENAME];
            MPI_Recv(&fileName, MAX_FILENAME, MPI_CHAR, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            swarms[fileName].push_back(peer);

            int nOfSegments;
            MPI_Recv(&nOfSegments, 1, MPI_INT, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (size_t i = 0; i < nOfSegments; ++i) {
                char segment[HASH_SIZE];
                MPI_Recv(&segment, HASH_SIZE, MPI_CHAR, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                hashes[fileName].push_back(segment);
            }

        }
    }

    for (size_t peer = 1; peer < numtasks; ++peer) {
        const int ack = ACK;
        MPI_Send(&ack, 1, MPI_INT, peer, 0, MPI_COMM_WORLD);
    }
    
    for (size_t peer = 1; peer < numtasks; ++peer) {
        MPI_Recv(&numberOfFilesWanted, 1, MPI_INT, peer, DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (size_t i = 0; i < numberOfFilesWanted; i++) {
            char fileWanted[MAX_FILENAME];
            MPI_Recv(&fileWanted, MAX_FILENAME, MPI_CHAR, peer, DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            int numberOfSeeders = swarms[fileWanted].size();
            MPI_Send(&numberOfSeeders, 1, MPI_INT, peer, DOWNLOAD, MPI_COMM_WORLD);
            vector<int> seeds = swarms[fileWanted];
            for (int seed : seeds) {
                MPI_Send(&seed, 1, MPI_INT, peer, DOWNLOAD, MPI_COMM_WORLD);
            }
            swarms[fileWanted].push_back(peer);

            int nrOfHases = hashes[fileWanted].size();
            MPI_Send(&nrOfHases, 1, MPI_INT, peer, DOWNLOAD, MPI_COMM_WORLD);
            for (string hash : hashes[fileWanted]) {
                char segment[HASH_SIZE] = {0};
                strncpy(segment, hash.c_str(), HASH_SIZE - 1);
                MPI_Send(&segment, HASH_SIZE, MPI_CHAR, peer, DOWNLOAD, MPI_COMM_WORLD);
            }

        }
    }
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    // Reading local files
    string inputFileName = "test1/in" + to_string(rank) + ".txt";
    ifstream inputFile(inputFileName);

    map<string, vector<string>> filesOwned;
    if (inputFile.is_open()) {
        int numberOfFiles;
        inputFile >> numberOfFiles;
        
        MPI_Send(&numberOfFiles, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

        // Files owned
        for (size_t i = 0; i < numberOfFiles; ++i) {
            char fileName[MAX_FILENAME];
            inputFile >> fileName;
            MPI_Send(&fileName, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);

            int nOfSegments;
            inputFile >> nOfSegments;
            MPI_Send(&nOfSegments, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

            for (size_t i = 0; i < nOfSegments; ++i) {
                char segment[HASH_SIZE];
                inputFile >> segment;
                filesOwned[fileName].push_back(segment);
                MPI_Send(&segment, HASH_SIZE, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
            }
        }

        // Files wanted
        inputFile >> numberOfFilesWanted;
        for (size_t i = 0; i < numberOfFilesWanted; ++i) {
            inputFile >> filesWanted[i];
        }
    }
    
    // Acknowledge for comunication
    int ackReceived = 0;
    MPI_Recv(&ackReceived, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (ackReceived != ACK) {
        printf("Eroare la primirea ack\n");
        exit(-1);
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
