
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
#include <math>


using namespace std;



int sequentialBf(vector<int> rows, vector<pair<int>> edgeDetails, int numNodes) { 
    vector<int> lastIteration;
    vector<int> nextIteration;
    lastIteration.reserve(numNodes);
    firstIteration.reserve(numNodes)

    // we set lastIteration[sourceNode] = 0;
    for(int index = 1; index < lastIteration.size(); index++) {
        if(index != sourceNode) {
            lastIteration[index] = std::numeric_limits<int>::max();
        } else {
            lastIteration[index] = 0;
        }
    }
    int firstCol = 1;
    int lastCol;
    bool done = false;
    while(!done) {
        for(int index = 1; index < rows.size(); index++) {
            lastCol = rows[index];
            for(firstCol; firstCol <= lastCol; firstCol++) {
                relaxEdge(index, edgeDetails[firstCol][0], edgeDetails[firstCol][1]);
            }
        }
        done = true;
        for(int index = 1; index < lastIteration.size(); index++) {
            if(lastIteration[index] != nextIteration[index]){
                done = false;
                break;
            }
        }
    }
}

void relaxEdge(int currentNode, int destNode, int edgeSize, vector<int> nextIteration, vector<int> lastIteration) {
    if(lastIteration[currentNode] + edgeSize < nextIteration[destNode]) {
        nextIteration[destNode] = lastIteration[currentNode] + edgeSize;
    }
}
    
void* graphMutex() { }

void* nodeMutex() { }


int main(int argc, char *argv[]) {

}