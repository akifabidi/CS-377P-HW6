
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
#include <math.h>


using namespace std;



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
void* graphMutex(vector<int> rows, vector<pair<int, int>> edgeDetails, int numNodes, int sourceNode) {
    pthread_mutex_t graph_lock;

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


int main(int argc, char *argv[]) {
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