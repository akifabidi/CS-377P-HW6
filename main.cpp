
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <utility>
#include <cstring>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits>
#include <pthread.h>
#include <math.h>

using namespace std;
#define MAX_THREADS 4
struct graphDetails threadArg[MAX_THREADS];
pthread_t handles[MAX_THREADS];
pthread_mutex_t graph_lock;

struct graphDetails {
    int myId;
    vector<int> rows;
    vector<pair<int, int>> edgeDetails;
    int numNodes;
    int sourceNode;
};

void relaxEdge(int currentNode, int destNode, int edgeSize, vector<int> nextIteration, vector<int> lastIteration) {
    //cout << "before" << currentNode << "  " << destNode << "  " << edgeSize << "  " << nextIteration[destNode] << "  "<< lastIteration[currentNode] << endl;
    if(lastIteration[currentNode] + edgeSize < nextIteration[destNode]) {    
        nextIteration[destNode] = lastIteration[currentNode] + edgeSize;
    }
    //cout << "after" << currentNode << "  " << destNode << "  " << edgeSize << "  " << nextIteration[destNode] << "  "<< lastIteration[currentNode] << endl;
}

int sequentialBf(vector<int> rows, vector<pair<int, int>> edgeDetails, int numNodes, int sourceNode) { 
    vector<int> lastIteration(numNodes);
    vector<int> nextIteration(numNodes);

    // we set lastIteration[sourceNode] = 0;
    for(int index = 1; index < lastIteration.size(); index++) {
        if(index != sourceNode) {
            lastIteration[index] = std::numeric_limits<int>::max();
            nextIteration[index] = std::numeric_limits<int>::max();
        } else {
            lastIteration[index] = 0;
            nextIteration[index] = 0;
        }
    }

    bool done = false;
    while(!done) {
        int firstCol = 1;
        int lastCol;
        for(int index = 1; index < rows.size(); index++) {
            lastCol = rows[index];
            for(firstCol; firstCol <= lastCol; firstCol++) {
                if (lastIteration[index] != std::numeric_limits<int>::max() && lastIteration[index] + edgeDetails[firstCol].second < nextIteration[edgeDetails[firstCol].first]) {    
                    nextIteration[ edgeDetails[firstCol].first] = lastIteration[index] + edgeDetails[firstCol].second;
                }
            }

        }

        done = true;
        for(int index = 1; index < lastIteration.size(); index++) {
            if(lastIteration[index] != nextIteration[index]){
                done = false;
                break;
            }
        }
        lastIteration = nextIteration;
        for(int index = 1; index < lastIteration.size(); index++) {
         cout << lastIteration[index] << endl;
        }
    }

}


// 2.1: course grain locking
void* graphMutex(void* input) {
    // TODO: Need to access graph details through thread args
    // Currently working through a struct, see http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html

    // We define variables locally from the input, for sake of simplicity
    int myId = (int) (((struct graphDetails*)input)->myId);;
    vector<int> rows = (vector<int>) (((struct graphDetails *)input)->rows);
    vector<pair<int, int>> edgeDetails = (vector<pair<int, int>>) (((struct graphDetails *)input)->edgeDetails);;
    int numNodes = (int) (((struct graphDetails *)input)->numNodes);
    int sourceNode = (int) (((struct graphDetails *)input)->sourceNode);
    

    
    vector<int> lastIteration(numNodes);
    vector<int> nextIteration(numNodes);

    // we set lastIteration[sourceNode] = 0;
    for(int index = 1; index < lastIteration.size(); index++) {
        if(index != sourceNode) {
            lastIteration[index] = std::numeric_limits<int>::max();
            nextIteration[index] = std::numeric_limits<int>::max();
        } else {
            lastIteration[index] = 0;
            nextIteration[index] = 0;
        }
    }

    bool done = false;
    while(!done) {
        int firstCol = myId;
        int lastCol;
        // THREAD DIVIDED BY NODES
        for(int index = 1; index < rows.size(); index+= MAX_THREADS) {
            lastCol = rows[index];

            for(firstCol; firstCol <= lastCol; firstCol++) {
                pthread_mutex_lock(&graph_lock);
                if (lastIteration[index] != std::numeric_limits<int>::max() && lastIteration[index] + edgeDetails[firstCol].second < nextIteration[edgeDetails[firstCol].first]) {    
                    nextIteration[ edgeDetails[firstCol].first] = lastIteration[index] + edgeDetails[firstCol].second;
                }
                pthread_mutex_unlock(&graph_lock);
            }

        }

        done = true;
        for(int index = 1; index < lastIteration.size(); index++) {
            if(lastIteration[index] != nextIteration[index]){
                done = false;
                break;
            }
        }
        lastIteration = nextIteration;
        for(int index = 1; index < lastIteration.size(); index++) {
         cout << lastIteration[index] << endl;
        }
    }
}

void* nodeMutex() { }

void* testSequentialBf() {
    vector<int> rows;
    vector<pair<int, int>> edgeDetails;
    rows.push_back(0);
    edgeDetails.push_back(make_pair(0, 0));
    edgeDetails.push_back(make_pair(2, -1));
    edgeDetails.push_back(make_pair(3, 4));
    rows.push_back(2);
    edgeDetails.push_back(make_pair(3, 3));
    edgeDetails.push_back(make_pair(4, 2));
    edgeDetails.push_back(make_pair(5, 2));
    rows.push_back(5);
    rows.push_back(5);
    edgeDetails.push_back(make_pair(3, 5));
    edgeDetails.push_back(make_pair(2, 1));
    rows.push_back(7);
    edgeDetails.push_back(make_pair(4, -3));
    rows.push_back(8);
    
    sequentialBf(rows, edgeDetails, 6, 1);
}


int main(int argc, char *argv[]) {
    // LOCK HANDELING
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_mutex_init(&graph_lock, NULL);

    // BEGIN RUN TIME
    uint64_t execTime; /*time in nanoseconds*/
    struct timespec tick, tock;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tick);

    // DEFINE GRAPH
    // TODO: need to translate from DIMAC file
    struct graphDetails *masterGraph = (struct graphDetails *) malloc(sizeof(struct graphDetails));
    // FILL IN WITH DETAILS
    // graph -> rows = ;
    // graph -> edgeDetails = ;
    // graph -> numNodes;
    // graph -> sourceNode;

    //----------------------------------------------

    // Part 2.1: Mutex on the graph
    for(int i = 0; i < MAX_THREADS; i++) {
        // TODO: I think we need to copy the graphDetails graph struct for every thread - becuase
        // we need to have a unique myId for every thread. Because we pass the address of the struct
        //we can't have one object. This is my attempt at it below
        threadArg[i] = *masterGraph;
        threadArg[i].myId = i;
        pthread_create(&handles[i], &attr, graphMutex, &threadArg[i]);
    }




    //----------------------------------------------
    // END RUN TIME
    clock_gettime(CLOCK_MONOTONIC_RAW, &tock);
    execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;
    printf("\n ----PART 4---- \n elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int)execTime);


    pthread_exit(NULL);
    return 0;
    
}