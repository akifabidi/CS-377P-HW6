
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
#include <tuple>
#include <atomic>

using namespace std;
#define MAX_THREADS 16

struct ThreadArgsInfo {
    int myId;
    struct GraphDetails *masterGraph;
};

struct ThreadArgsInfo threadArg[MAX_THREADS];
pthread_t handles[MAX_THREADS];
bool isDone = false;
int numIter = 0;

// PART 2
pthread_barrier_t varBarrier;

// part 2.1
pthread_mutex_t graph_lock;

// part 2.2
pthread_mutex_t* node_locks;
std::vector<pthread_attr_t *> attrs;

// part 2.3 
std::vector<pthread_spinlock_t *> node_spinlocks;

// part 2.4
// true if node is free, false if it is being operated on
bool booleanVal = true;
std::atomic<bool> masterBool;


struct GraphDetails {
    vector<int> labels;
    vector<int> rows;
    vector<pair<int, int>> edgeDetails;
    int numNodes;
    int sourceNode;
    vector<int> currIteration;
    vector<int> lastIteration;
    vector<int> nextIteration;
};



// main graph - details are saved here so that all threads access same struct in the heap
struct GraphDetails *mainGraph = (struct GraphDetails*) malloc(sizeof(struct GraphDetails));



void relaxEdge(int currentNode, int destNode, int edgeSize, vector<int> nextIteration, vector<int> lastIteration) {
    //cout << "before" << currentNode << "  " << destNode << "  " << edgeSize << "  " << nextIteration[destNode] << "  "<< lastIteration[currentNode] << endl;
    if(lastIteration[currentNode] + edgeSize < nextIteration[destNode]) {    
        nextIteration[destNode] = lastIteration[currentNode] + edgeSize;
    }
    //cout << "after" << currentNode << "  " << destNode << "  " << edgeSize << "  " << nextIteration[destNode] << "  "<< lastIteration[currentNode] << endl;
}

vector<int> sequentialBf(vector<int> labels, vector<int> rows, vector<pair<int, int>> edgeDetails, int numNodes, vector<int> lastIteration) { 

    bool notDone = true;
    int iteration = 0;
    while(notDone) {
        iteration++;
        notDone = false;
        int lastCol;
        for(int index = 1; index < rows.size(); index++) {
            if(index == rows.size() - 1) {
                lastCol = edgeDetails.size();
            } else {
                lastCol = rows[index + 1];
            }
            for(int firstCol = rows[index]; firstCol < lastCol; firstCol++) {
                if (lastIteration[labels[index]]!= std::numeric_limits<int>::max() && lastIteration[labels[index]] + edgeDetails[firstCol].second < lastIteration[edgeDetails[firstCol].first]) {   
                    lastIteration[ edgeDetails[firstCol].first] = lastIteration[labels[index]] + edgeDetails[firstCol].second;
                    notDone = true;
                }
            }

        }
        if(!notDone) {
            break;
        }
        //lastIteration = nextIteration;
    }
    cout << '\n' << iteration << endl;
    return lastIteration;//nextIteration;  
}

/*
for Source 8
1 - 3
2 - 1
3 - 2
4 - 2
5 - 1
6 - 2
 7 -inf 
8 - 0 
.... rest inf
*/


// 2.1: course-grain locking
void* graphMutex(void* input) {
    // TODO: Need to access graph details through thread args
    // Currently working through a struct, see http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html

    // We define variables locally from the input, for sake of simplicity
    int myId = (int) (((struct ThreadArgsInfo*)input)->myId);
    vector<int> labels = (vector<int>) (((struct ThreadArgsInfo *)input)->masterGraph->labels); 
    vector<int> rows = (vector<int>) (((struct ThreadArgsInfo *)input)->masterGraph->rows);
    vector<pair<int, int>> *edgeDetails = (vector<pair<int, int>> *) &(((struct ThreadArgsInfo *)input)->masterGraph->edgeDetails);
    int numNodes = (int) (((struct ThreadArgsInfo *)input)->masterGraph->numNodes);
    int sourceNode = (int) (((struct ThreadArgsInfo *)input)->masterGraph->sourceNode);
    vector<int> *currIteration = (vector<int> *) &(((struct ThreadArgsInfo *)input)->masterGraph->currIteration);

    while(!isDone) {
        if(myId == 0) {
            numIter++;
        }
        pthread_barrier_wait (&varBarrier);
        isDone = true;
        int firstCol = myId;
        int lastCol;
        // THREAD DIVIDED BY NODES
        for(int index = myId ; index < rows.size(); index+=MAX_THREADS) {
            if(index == rows.size() - 1) {
                lastCol = edgeDetails->size();
            } else {
                lastCol = rows[index + 1];
            }
            for(int firstCol = rows[index]; firstCol < lastCol; firstCol++) {
                pthread_mutex_lock(&graph_lock);
                if ((*currIteration)[labels[index]]!= std::numeric_limits<int>::max() && (*currIteration)[labels[index]] + (*edgeDetails)[firstCol].second < (*currIteration)[(*edgeDetails)[firstCol].first]) {   
                    (*currIteration)[ (*edgeDetails)[firstCol].first] = (*currIteration)[labels[index]] + (*edgeDetails)[firstCol].second;
                    isDone = false;
                }
                pthread_mutex_unlock(&graph_lock);
            }
        }
        // wait for all the threads to complete so everything in nextIteration is updated
        pthread_barrier_wait (&varBarrier);
    }
}


// 2.2: fine-grain locking
void* nodeMutex(void* input) {
    // TODO: Need to access graph details through thread args
    // Currently working through a struct, see http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html

    // We define variables locally from the input, for sake of simplicity
    int myId = (int) (((struct ThreadArgsInfo*)input)->myId);
    vector<int> labels = (vector<int>) (((struct ThreadArgsInfo *)input)->masterGraph->labels); 
    vector<int> rows = (vector<int>) (((struct ThreadArgsInfo *)input)->masterGraph->rows);
    vector<pair<int, int>> *edgeDetails = (vector<pair<int, int>> *) &(((struct ThreadArgsInfo *)input)->masterGraph->edgeDetails);
    int numNodes = (int) (((struct ThreadArgsInfo *)input)->masterGraph->numNodes);
    int sourceNode = (int) (((struct ThreadArgsInfo *)input)->masterGraph->sourceNode);
    vector<int> *currIteration = (vector<int> *) &(((struct ThreadArgsInfo *)input)->masterGraph->currIteration);

    while(!isDone) {
        if(myId == 0) {
            numIter++;
        }
        pthread_barrier_wait (&varBarrier);
        isDone = true;
        int firstCol = myId;
        int lastCol;
        // THREAD DIVIDED BY NODES
        for(int index = myId ; index < rows.size(); index+=MAX_THREADS) {
            if(index == rows.size() - 1) {
                lastCol = edgeDetails->size();
            } else {
                lastCol = rows[index + 1];
            }
            for(int firstCol = rows[index]; firstCol < lastCol; firstCol++) {
                pthread_mutex_lock(&node_locks[(*edgeDetails)[firstCol].first]);
                if ((*currIteration)[labels[index]]!= std::numeric_limits<int>::max() && (*currIteration)[labels[index]] + (*edgeDetails)[firstCol].second < (*currIteration)[(*edgeDetails)[firstCol].first]) {   
                    (*currIteration)[ (*edgeDetails)[firstCol].first] = (*currIteration)[labels[index]] + (*edgeDetails)[firstCol].second;
                    isDone = false;
                }
                pthread_mutex_unlock(&(node_locks[(*edgeDetails)[firstCol].first]));
            }
        }
        pthread_barrier_wait (&varBarrier);
    }
}

// 2.3: spinlocking
void* nodeSpinLock(void* input) {
    // TODO: Need to access graph details through thread args
    // Currently working through a struct, see http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html

    // We define variables locally from the input, for sake of simplicity
    int myId = (int) (((struct ThreadArgsInfo*)input)->myId);
    vector<int> rows = (vector<int>) (((struct ThreadArgsInfo *)input)->masterGraph->rows);
    vector<pair<int, int>> *edgeDetails = (vector<pair<int, int>> *) &(((struct ThreadArgsInfo *)input)->masterGraph->edgeDetails);
    int numNodes = (int) (((struct ThreadArgsInfo *)input)->masterGraph->numNodes);
    int sourceNode = (int) (((struct ThreadArgsInfo *)input)->masterGraph->sourceNode);
    
 
    
    vector<int> *lastIteration = (vector<int> *) &(((struct ThreadArgsInfo *)input)->masterGraph->lastIteration);
    vector<int> *nextIteration = (vector<int> *) &(((struct ThreadArgsInfo *)input)->masterGraph->nextIteration);

    // we set lastIteration[sourceNode] = 0;
    for(int index = 1; index < lastIteration->size(); index++) {
        if(index != sourceNode) {
            (*lastIteration)[index] = std::numeric_limits<int>::max();
            (*nextIteration)[index] = std::numeric_limits<int>::max();
        } else {
            (*lastIteration)[index] = 0;
            (*nextIteration)[index] = 0;
        }
    }

    bool done = false;
    while(!done) {
        int firstCol = myId;
        int lastCol;
        // THREAD DIVIDED BY NODES
        for(int index = myId; index < rows.size(); index+= MAX_THREADS) {
            pthread_spin_lock(node_spinlocks[index]);
            lastCol = rows[index];

            for(firstCol; firstCol <= lastCol; firstCol++) {
                if ((*lastIteration)[index] != std::numeric_limits<int>::max() && (*lastIteration)[index] + edgeDetails->at(firstCol).second < (*nextIteration)[(*edgeDetails)[firstCol].first]) {    
                    (*nextIteration)[ (*edgeDetails)[firstCol].first] = (*lastIteration)[index] + (*edgeDetails)[firstCol].second;
                }
            }
            pthread_spin_unlock(node_spinlocks[index]);
        }

        pthread_barrier_wait (&varBarrier);
        done = true;
        for(int index = 1; index < lastIteration->size(); index++) {
            if(lastIteration[index] != nextIteration[index]){
                done = false;
                break;
            }
        }
        lastIteration = nextIteration;
        for(int index = 1; index < lastIteration->size(); index++) {
         cout << (*lastIteration)[index] << endl;
        }
    }

}

// 2.4: compare swap
void* nodeCompareSwap(void* input) {

    // We define variables locally from the input, for sake of simplicity
    int myId = (int) (((struct ThreadArgsInfo*)input)->myId);
    vector<int> rows = (vector<int>) (((struct ThreadArgsInfo *)input)->masterGraph->rows);
    vector<pair<int, int>> *edgeDetails = (vector<pair<int, int>> *) &(((struct ThreadArgsInfo *)input)->masterGraph->edgeDetails);
    int numNodes = (int) (((struct ThreadArgsInfo *)input)->masterGraph->numNodes);
    int sourceNode = (int) (((struct ThreadArgsInfo *)input)->masterGraph->sourceNode);
    

    
    vector<int> *lastIteration = (vector<int> *) &(((struct ThreadArgsInfo *)input)->masterGraph->lastIteration);
    vector<int> *nextIteration = (vector<int> *) &(((struct ThreadArgsInfo *)input)->masterGraph->nextIteration);

    // we set lastIteration[sourceNode] = 0;
    for(int index = 1; index < lastIteration->size(); index++) {
        if(index != sourceNode) {
            (*lastIteration)[index] = std::numeric_limits<int>::max();
            (*nextIteration)[index] = std::numeric_limits<int>::max();
        } else {
            (*lastIteration)[index] = 0;
            (*nextIteration)[index] = 0;
        }
    }

    bool done = false;
    while(!done) {
        int firstCol = myId;
        int lastCol;
        // THREAD DIVIDED BY NODES
        for(int index = myId; index < rows.size(); index+= MAX_THREADS) {
            lastCol = rows[index];
            bool old = masterBool;
            bool curr = true ;
            for(firstCol; firstCol <= lastCol; firstCol++) {
                do {
                    if ((*lastIteration)[index] != std::numeric_limits<int>::max() && (*lastIteration)[index] + edgeDetails->at(firstCol).second < (*nextIteration)[edgeDetails->at(firstCol).first]) {    
                        (*nextIteration)[ edgeDetails->at(firstCol).first] = (*lastIteration)[index] + edgeDetails->at(firstCol).second;
                    }
                } while (!masterBool.compare_exchange_weak(old, true));

            }

        }

        // wait for all the threads to complete so everything in nextIteration is updated
        pthread_barrier_wait (&varBarrier);
        
        done = true;
        for(int index = 1; index < lastIteration->size(); index++) {
            if(lastIteration[index] != nextIteration[index]){
                done = false;
                break;
            }
        }
        lastIteration = nextIteration;
        for(int index = 1; index < lastIteration->size(); index++) {
         cout << (*lastIteration)[index] << endl;
        }
    }
}

// Part 3
void* edgeDistribution(void* input) {
    
    int myId = (int) (((struct ThreadArgsInfo*)input)->myId);
    vector<int> rows = (vector<int>) (((struct ThreadArgsInfo *)input)->masterGraph->rows);
    vector<pair<int, int>> *edgeDetails = (vector<pair<int, int>> *) &(((struct ThreadArgsInfo *)input)->masterGraph->edgeDetails);
    int numNodes = (int) (((struct ThreadArgsInfo *)input)->masterGraph->numNodes);
    int sourceNode = (int) (((struct ThreadArgsInfo *)input)->masterGraph->sourceNode);
    
    vector<int> lastIteration(numNodes);
    vector<int> nextIteration(numNodes);
    int startEdge = (edgeDetails->size() / MAX_THREADS) * myId;
    int endEdge = startEdge + (edgeDetails->size() / MAX_THREADS);
    int startIndex;
    for(int currIndex = myId; currIndex < rows.size(); currIndex++) {
        if(rows[currIndex] >= startEdge) {
                startIndex = currIndex;
        }
    }
    bool done = false;
    while(!done) {
        int firstCol = startEdge;
        int lastCol;
        // Find starting node edge 
        bool lastNode = false;
        for(int index = startIndex; index < rows.size(); index++) {
            if(rows[index] >= endEdge) {
                lastCol = std::min(rows[index], endEdge);
                lastNode = true;
            } else {
                lastCol = rows[index];
            }
            bool expected = false;
            for(firstCol; firstCol <= lastCol; firstCol++) {
                while (!masterBool.compare_exchange_weak(expected, true, std::memory_order_acq_rel, std::memory_order_relaxed)){
                    if (lastIteration[index] != std::numeric_limits<int>::max() && lastIteration[index] + edgeDetails->at(firstCol).second < nextIteration[edgeDetails->at(firstCol).first]) {    
                        nextIteration[ edgeDetails->at(firstCol).first] = lastIteration[index] + edgeDetails->at(firstCol).second;
                    }
                }

            }
            if(lastNode) {
                break;
            }
        }

        // wait for all the threads to complete so everything in nextIteration is updated
        pthread_barrier_wait (&varBarrier);
        
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
    
    //sequentialBf(rows, edgeDetails, 6, 1);
}

pair<vector<vector<int> >, int> dimacToCoo(string fileName) {
    // Read in DIMAC to COO in perperation to be converted into CSR
    // Construct a double array that takes every edge as 3 col 
    // vector. Add vector to double array   

    ifstream file;
    file.open(fileName, ios::in);


    vector<vector<int> > dummy;

	if (!file) {
		cout << "\n ERROR: File cannot be read";
	} else {
        // DO MAGIC
        string strLine; 

        getline(file, strLine);
     
        cout << "\n";
        stringstream ss(strLine);
        string word;
        ss >> word;
        ss >> word;
        ss >> word;

        
        int nodesTotal = stoi(word);
        ss >> word;

        int edges = stoi(word);

        // Testing p line
        printf("\n Nodes: %d", nodesTotal);
        printf("\n Edges: %d", edges);
        cout << endl;
        // set up COO vector
        vector<vector<int> > coo(edges, vector<int>(3, 0));

        int index = 0;

        // Populate COO vector with edges
        while (getline(file, strLine)) {
            // only looking at edges 
            stringstream ss(strLine);
            ss >> word;
            if (word == "a") {
                char* ptr;
                ss >> word;
                coo[index][0] = stoi(word);
                ss >> word;
                coo[index][1] = stoi(word);
                ss >> word;
                coo[index][2] = stoi(word);
                index++;
            }
        }

        std::sort(coo.begin(), coo.end());
        // for(int index = 0; index < coo.size(); index++) {
        //     cout << coo[index][0] << "  " << coo[index][1] << "  " << coo[index][2] << endl;
        // }
        return make_pair(coo, nodesTotal);
    }
    return make_pair(dummy, 0);
}


tuple<vector<int>, vector<int>, vector<pair<int, int> >, int> dimacToCsr(string fileName) {
    // get COO representation of DIMAC
    pair<vector<vector<int> >, int> values = dimacToCoo(fileName);
    vector<vector<int> > coo = values.first;
    // TODO: SORT????
    vector<int> labels; 
    labels.push_back(0);
    vector<int> rp;
    rp.push_back(0);
    vector<pair<int, int> > edgedetails; 
    edgedetails.push_back(make_pair(0,0));

    for(int i = 0; i < coo.size(); i++) {
        // we will add new labels to our labels vector
        // make this coo[i][2]?
        if(labels[labels.size() - 1] != coo[i][0]) {
            labels.push_back(coo[i][0]);
            rp.push_back(edgedetails.size());
            edgedetails.push_back(make_pair(coo[i][1], coo[i][2]));
        } else {
            if(edgedetails[edgedetails.size() - 1].first == coo[i][1]) {
                // takes the min value
                edgedetails[edgedetails.size() - 1].second = min(coo[i][2], edgedetails[edgedetails.size() - 1].second);
            } else {
                edgedetails.push_back(make_pair(coo[i][1], coo[i][2]));
            }
        }
    }    
    return make_tuple(labels, rp, edgedetails, values.second);

}

int main(int argc, char *argv[]) {
    // DIMACS TO CSR
    vector<int> labels;
    vector<int> rp;
    vector<pair<int, int> > edgedetails; 
    int numNodes;
    tie(labels, rp, edgedetails, numNodes) = dimacToCsr("rmat.dimacs");
    int sourceNode = 1;

    // part 1 code
    /*vector<int> currIteration(numNodes + 1);
    // we set lastIteration[sourceNode] = 0;
    for(int index = 1; index < currIteration.size(); index++) {
        if(index != sourceNode) {
            currIteration[index] = std::numeric_limits<int>::max();
        } else {
            currIteration[index] = 0;
        }
    }*/
    // BEGIN RUN TIME
    uint64_t execTime; // time in nanoseconds
    struct timespec tick, tock;
    /*clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
    // part 1 code
    vector<int> solution = sequentialBf(labels, rp, edgedetails, numNodes, currIteration);
    clock_gettime(CLOCK_MONOTONIC_RAW, &tock);
    execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;
    printf("\n ----PART 4---- \n elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int)execTime);
    std::ofstream outfile ("sspDetailsNY.dimacs");
    for (int i = 1; i < solution.size(); i++) {
        outfile << i << "  " << solution[i] << endl;
    }
    return 0;*/
    // DEFINE GRAPH
    // TODO: need to translate from DIMAC file
    GraphDetails *masterGraph = new GraphDetails();
    // FILL IN WITH DETAILS
    masterGraph -> labels = labels;
    masterGraph -> rows = rp;
    masterGraph -> edgeDetails = edgedetails;
    masterGraph -> numNodes = numNodes;
    masterGraph -> sourceNode = sourceNode;
    
    // Set up Iterations
    
    masterGraph -> currIteration.push_back(std::numeric_limits<int>::max());
    for(int index = 1; index <= numNodes; index++) {
        if(index != masterGraph->sourceNode) {
            masterGraph -> currIteration.push_back(std::numeric_limits<int>::max());
        } else {
            masterGraph -> currIteration.push_back(0);
        }
    }

    // Barrier
    pthread_barrier_init (&varBarrier,NULL,MAX_THREADS);
    //----------------------------------------------

    // Part 2.1: Mutex on the graph
        // LOCK HANDELING
    /*pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    pthread_mutex_init(&graph_lock, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
    for(int i = 0; i < MAX_THREADS; i++) {
        // TODO: I think we need to copy the GraphDetails graph struct for every thread - becuase
        // we need to have a unique myId for every thread. Because we pass the address of the struct
        //we can't have one object. This is my attempt at it below

        // make a soft copy of mastergraphs and chaing myId to divide up threads
        threadArg[i].masterGraph = masterGraph;
        threadArg[i].myId = i;
        pthread_create(&handles[i], &attr, graphMutex, &threadArg[i]);
    }

    for (int i=0; i< MAX_THREADS; i++) {
        pthread_join(handles[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &tock);
    execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;
    printf("\n ----PART 4---- \n elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int)execTime);
    std::ofstream outfile ("sspDetailsNY.dimacs");
    for (int i = 1; i < masterGraph->currIteration.size(); i++) {
        outfile << i << "  " << masterGraph->currIteration[i] << endl;
    }
    cout << "iterations" << numIter << endl;
    pthread_exit(NULL);
    return 0;
*/
    // Part 2.2: Mutex on the nodes
    // initialize locks

    // is rows and numNodes the same amount of node??? - prolly not cus rows is just outgoing right?
    //  i think we just need to have a mutex for every node because we cannot tell right now which nodes will be used or not


    pthread_attr_t attr;
    pthread_attr_init(&attr);
    //pthread_mutex_t* temp_node = new pthread_mutex_t[numNodes + 1];
    node_locks = new pthread_mutex_t[numNodes + 1];
    for (int i = 0; i < masterGraph ->numNodes + 1; i ++) {
        pthread_mutex_init(&node_locks[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
    for(int i = 0; i < MAX_THREADS; i++) {
        // make a soft copy of mastergraphs and chaing myId to divide up threads
        threadArg[i].masterGraph = masterGraph;
        threadArg[i].myId = i;
        pthread_create(&handles[i], &attr, nodeMutex, &threadArg[i]);
    }

    for (int i=0; i< MAX_THREADS; i++) {
        pthread_join(handles[i], NULL);
    }
        clock_gettime(CLOCK_MONOTONIC_RAW, &tock);
    execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;
    printf("\n ----PART 4---- \n elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int)execTime);
    std::ofstream outfile ("sspDetailsNY.dimacs");
    for (int i = 1; i < masterGraph->currIteration.size(); i++) {
        outfile << i << "  " << masterGraph->currIteration[i] << endl;
    }
    cout << "iterations" << numIter << endl;
    pthread_exit(NULL);
    return 0;

    // Part 2.3: Spinlock on nodes

    // create spinlocks. See https://man7.org/linux/man-pages/man3/pthread_spin_init.3.html
    // and https://man7.org/linux/man-pages/man3/pthread_spin_lock.3.html for documentation
    for (int i = 0; i < masterGraph ->numNodes; i ++) {
        pthread_spinlock_t node_spinlock;
        node_spinlocks.push_back(&node_spinlock);
        pthread_mutex_init(&node_locks[i], NULL);

        pthread_spin_init(node_spinlocks[i], PTHREAD_PROCESS_SHARED);
    }

    for(int i = 0; i < MAX_THREADS; i++) {
        // make a soft copy of mastergraphs and chaing myId to divide up threads
        threadArg[i].masterGraph = masterGraph;
        threadArg[i].myId = i;
        pthread_create(&handles[i], &attr, nodeMutex, &threadArg[i]);
    }




    // JOIN THREADS
    for (int i=0; i< MAX_THREADS; i++) {
        pthread_join(handles[i], NULL);
    }

    //---------------------------------------------

    // END RUN TIME
    clock_gettime(CLOCK_MONOTONIC_RAW, &tock);
    execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;
    printf("\n ----PART 4---- \n elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int)execTime);


    pthread_exit(NULL);
    return 0;
    
}
