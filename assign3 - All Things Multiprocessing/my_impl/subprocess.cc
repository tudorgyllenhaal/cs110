/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */

#include "subprocess.h"
using namespace std;
/*
int Pipe(int pipefd[2]) {
	int p = pipe(pipefd);
	if(p == -1) {
		if(errno == EFAULT) throw("pipefd is not valid");
		if(errno == EMFILE) throw("Too many file descriptors are in use by the process");
		if(errno == ENFILE) throw("The system limit on the total number of open files has been reached");
		throw("unknown error");
	}
	return p;
}

int Dup2(int oldfd, int newfd) {
	int d = dup2(oldfd, newfd);
	if(d == -1) {
		if(errno == EBADF) throw("oldfd isn't an open fd, or newfd is out of the range for fd");
		if(errno == EBUSY) throw("race condition occured");
		if(errno == EINTR) throw("The dup2() call was interrupted by a siganl");
		if(errno == EMFILE) throw("Too many file descriptors are in use by the process");
		throw("unknown error");
	}
	return d;
}

int Close(int fd) {
	int c = close(fd);
	if(c == -1) {
		if(errno == EBADF) throw("fd isn't an valid open fd");
		if(errno == EINTR) throw("The close() call was interrupted by a signal");
		if(errno == EIO) throw("An I/O error occured");
		throw("unknown error");
	}
	return c;
}

void Execvp(const char* file, char* const argv[]) {
	int	e = execvp(file, argv);
	if(e == -1) {
		throw("execvp error");
	}
}
*/
subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException) {
	static const int NOT_UNUSED=-1;  //const used to signify the file descriptor isn't used
	int inputFds[2]={NOT_UNUSED,NOT_UNUSED};
	int outputFds[2]={NOT_UNUSED,NOT_UNUSED};
	subprocess_t subpr;
	int result;
	if(supplyChildInput){ // Input Pipe Setup
		result=pipe(inputFds);
		if(result<0){
			throw SubprocessException("Fail to Open Input Pipe\n");
		}
		subpr.supplyfd=inputFds[1];
	}else{
		subpr.supplyfd=kNotInUse;
	}
	
	if(ingestChildOutput){
		result=pipe(outputFds);
		if(result<0){
			throw SubprocessException("Fail to Open Ouput Pipe\n");
		}
		subpr.ingestfd=outputFds[0];
	}else{
		subpr.ingestfd=kNotInUse;
	}

	pid_t pid=fork();
	if(pid==0){
		// child process
		if(supplyChildInput){
			close(inputFds[1]); // close write of input pipe
			result=dup2(inputFds[0],STDIN_FILENO);
			if(result<0){
				throw SubprocessException("Fail to DUP2\n");
			}
			close(inputFds[0]);
		}
		if(ingestChildOutput){
			close(outputFds[0]); // close read of output pipe
			result=dup2(outputFds[1],STDOUT_FILENO);
			if(result<0){
				throw SubprocessException("Fail to DUP2\n");
			}
			close(outputFds[1]);
		}
		execvp(argv[0],argv);
		throw SubprocessException("Fail to open program");

	}
	// parent process
	subpr.pid=pid;
    if(supplyChildInput){
        close(inputFds[0]);
    }
    if(ingestChildOutput){
        close(outputFds[1]);
    }
	// close input pipes
    /*
    
	for(int i=0;i<2;i++){
		if(inputFds[i]!=NOT_UNUSED){
			close(inputFds[i]);
		}
	}
	// close output pipes
	for(int i=0;i<2;i++){
		if(outputFds[i]!=NOT_UNUSED){
			close(outputFds[i]);
		}
	}
    */
	return subpr;
}
