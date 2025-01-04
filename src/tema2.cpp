#include <cstddef>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <pthread.h>
#include <stdlib.h>
#include <string>
#include <mpi.h>


using namespace std;

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100
#define ACK 1212
#define DOWNLOAD 1
#define UPLOAD 2
#define COUNT 3
#define CONNECT 4

// Client
char filesWanted[MAX_FILES][MAX_FILENAME];
int numberOfFilesWanted = 0;
map<string, vector<string>> filesOwned;
void displayMap(const map<string, vector<string>>& hashes) {
    for (const auto& [key, value] : hashes) {
        cout << "Key: " << key << "\nValues: ";
        for (const auto& v : value) {
            cout << v << " ";
        }
        cout << "\n";
    }
}
void *download_thread_func(void *arg)
{
    int rank = *(int*) arg;
    
    // Getting tracker informations
    map<string, vector<int>>  swarms;
    map<string,  vector<string>> hashes;
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
        
        // ofstream out("client" + to_string(rank) + "_" + filesWanted[i]);
        for (int j = 0; j < nrOfHashes; ++j) {
            char segment[HASH_SIZE];
            MPI_Recv(&segment, HASH_SIZE, MPI_CHAR, TRACKER_RANK, DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // out << segment << endl;
            hashes[filesWanted[i]].push_back(segment);
        }
    }

    // Connecting to seeds/peers
    for (size_t i = 0; i < numberOfFilesWanted; ++i) {
        string fileWanted = filesWanted[i];
        vector<int> seeds = swarms[fileWanted];
        vector<string> it  = hashes[fileWanted];
        
        for (int seed : seeds) {
            MPI_Send(&filesWanted[i], MAX_FILENAME, MPI_CHAR, seed, COUNT, MPI_COMM_WORLD);
            int ack = 0;
            MPI_Recv(&ack, 1, MPI_INT, seed, COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (ack == ACK) {
                for (string hash: it) {
                    int bestSeed = 0;
                    int minClients = 20;
                    char msg[HASH_SIZE];
                    strncpy(msg, hash.c_str(), HASH_SIZE);
                        
                    // Daca are fisierul verificam daca are segment-ul si cati sunt conectati
                    MPI_Send(&msg, HASH_SIZE, MPI_CHAR, seed, COUNT, MPI_COMM_WORLD);
                    int connected = 0;
                    MPI_Recv(&connected, 1, MPI_INT, seed, COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    if (connected >= 0 && connected < minClients) { // Are hash-ul
                        minClients = connected;
                        bestSeed = seed;
                    }
                }
                        // cout << connected << endl;
                    // }
                    // cout << rank << " " << bestSeed << endl;
                    // Proceeding download
                    // for (int seed : seeds) {
                    //     if (seed == bestSeed) {
                    //         char startDownload = 1;
                    //         MPI_Send(&startDownload, 1, MPI_BYTE, bestSeed, COUNT, MPI_COMM_WORLD);
                    //     } else {
                    //         char startDownload = 0;
                    //         MPI_Send(&startDownload, 1, MPI_BYTE, bestSeed, COUNT, MPI_COMM_WORLD);
                    //     }
                    // }
                    // PRIMIM hash-ul DE LA SEED-ul ales
                    // int ack = 0;
                    // MPI_Recv(&ack, 1, MPI_INT, bestSeed, COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    // ofstream out("client" + to_string(rank) + "_" + filesWanted[i]);
                    // if (ack != ACK) {
                    //     cout << "CEVA NU A MERS" << endl;
                    // }
                    // filesOwned[filesWanted[i]].push_back(hash);
                    // out << hash << endl;
                // }
        }
    }
    
    return NULL;
}

void *upload_thread_func(void *arg)
{
    int rank = *(int*) arg;

    int connected = 0;
    while (1) {
        MPI_Status status;
        char fileName[MAX_FILENAME];

        MPI_Recv(&fileName, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE, COUNT, MPI_COMM_WORLD, &status);
        int tag = status.MPI_TAG;
        int source = status.MPI_SOURCE;
        cout << fileName << " "<<  source << endl;
        if (!filesOwned[fileName].empty()) {
            int ack = ACK;
            MPI_Send(&ack, 1, MPI_INT, source, COUNT, MPI_COMM_WORLD);
        //     char msg[HASH_SIZE];
        //     // MPI_Recv(&msg, HASH_SIZE, MPI_CHAR, source, COUNT, MPI_COMM_WORLD, &status);
            
        //     // // Sending number of connected clients
        //     // MPI_Send(&connected, 1, MPI_INT, source, COUNT, MPI_COMM_WORLD);
        //     // // cout << "sursa:" << source << " rank:" << rank << endl;

        //     // char downloadingStatus = 0;
        //     // MPI_Recv(&downloadingStatus, 1, MPI_BYTE, source, COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //     // if (downloadingStatus == 1) { // Daca suntem seed-ul ales sa uploadam fisierul, il uploadam
        //     //     connected++;
        //     //     int ack = ACK;
        //     //     MPI_Send(&ack, 1, MPI_INT, source, COUNT, MPI_COMM_WORLD);
            // }
        } else {
            int ack = 0;
            MPI_Send(&ack, 1, MPI_INT, source, COUNT, MPI_COMM_WORLD);
        }
        
        // break;
    }

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
            bool alreadyGotten = false;
            if (!swarms[fileName].empty()) {
                alreadyGotten = true;
            }
            swarms[fileName].push_back(peer);

            int nOfSegments;
            MPI_Recv(&nOfSegments, 1, MPI_INT, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (size_t i = 0; i < nOfSegments; ++i) {
                char segment[HASH_SIZE];
                MPI_Recv(&segment, HASH_SIZE, MPI_CHAR, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (!alreadyGotten) {
                    hashes[fileName].push_back(segment);
                }
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
                strncpy(segment, hash.c_str(), HASH_SIZE);
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
