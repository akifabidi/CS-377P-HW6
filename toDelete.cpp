main() {

	lastIteration and nextIteration initialization
	done = false
	while(!done) {
		createThreads loop
		Threads joinn	
		check if lastIteration and nextIteration are same
		if not same  done = false else true
		
	}
}

threadMethod(edgeDetails, nodes, labels, lastIteration, nextIteration) {

	for(i = threadID; i < nodes.size(); i += NumThreads){
	     lock/mutex lock();
	     check if currNode + weight < desNode dist is less and update
	     unlock
	}
}


or try 
threadMethod() {
	while(!done)
	for(i = threadID; i < nodes.size(); i += NumThreads){
	     lock/mutex lock();
	     check if currNode + weight < desNode dist is less and update
	     unlock
	}

	barrier wait() // wait for all the threads to complete so everything in nextIteration is updated
	check if lastIteration and nextIteration are same, if not, done = false else true
	}
}
// in this case, main will not have to continously create new threads and check if the same
// idk which version they want