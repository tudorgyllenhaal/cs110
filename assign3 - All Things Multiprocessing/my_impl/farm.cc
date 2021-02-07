#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>
#include<stdio.h>

#include<iostream>
#include<vector>
#include<string>

#include "subprocess.h"

using namespace std;

struct worker {
	worker() {}
	worker(char *argv[]) : sp(subprocess(argv, true, false)), available(false) {}
	subprocess_t sp;
	bool available;
};

static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN);
static vector<worker> workers(kNumCPUs);
static size_t numWorkersAvailable = 0;

static void markWorkersAsAvailable(int sig) {
	int status;
	int pid;
	while(true){
		pid=waitpid(-1,&status,WNOHANG);
		if(pid>0){
			for(auto it=workers.begin();it!=workers.end();it++){
				if((pid==it->sp.pid)&&(!it->available)){
					it->available=true;
					numWorkersAvailable++;
					break;
				}
			}
			
		}else{
			if(pid<0){
				cout<<"[ERROR] Error occured while waiting for child process"<<endl;
			}
			break;
		}
	}
}

static char *kWorkerArguments[] = {"./factor.py", "--self-halting", NULL};
static void spawnAllWorkers() {
	for(unsigned int i=0;i<kNumCPUs;i++){
		workers[i]=worker(kWorkerArguments);
	}
}
static size_t getAvailableWorker() {
	sigset myset,childset;
	while(numWorkersAvailable==0){
		sigfillset(&myset);
	}
	for(size_t i=0;i<kNumCPUs;i++){
		if(workers[i].available){
			return i;
		}
	}
	
}

static void broadcastNumbersToWorkers() {
	while(true){
		string line;
		getline(cin,line);
		if(cin.fail()) break;
		size_t endpos;
		stoll(line,&endpos);
		if(endpos!=line.size()) break;
		size_t index=getAvailableWorker();
		workers[index].available=false;
		numWorkersAvailable--;
		int  supply_fd=workers[index].sp.supplyfd;
		dprintf(supply_fd,"%s\n",line.c_str());
		kill(workers[index].sp.pid,SIGCONT);
	}
}

static void waitForAllWorkers() {
	
	for(auto it=workers.begin();it!=workers.end();it++){
		if(it->available){
			continue;
		}

	}
}

static void closeAllWorkers() {
	signal(SIGCHLD,SIG_DEL);
	for(int i=0;i<kNumCPUs;i++){
		close(workers[i].sp.supplyfd);
		kill(workers[i].sp.pid,SIGCONT);
	}
	for(unsigned int i=0;i<kNumCPUs;i++){
		int status;
		while(true){
			waitpid(workers[i].sp.pid,&status,0);
			if(WIFEXITED(status)){
				break;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	signal(SIGCHLD, markWorkersAsAvailable);
	spawnAllWorkers();
	broadcastNumbersToWorkers();
	waitForAllWorkers();
	closeAllWorkers();
	return 0;
}
