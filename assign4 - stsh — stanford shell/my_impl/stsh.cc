/**
 * File: stsh.cc
 * -------------
 * Defines the entry point of the stsh executable.
 */

#include "stsh-parser/stsh-parse.h"
#include "stsh-parser/stsh-readline.h"
#include "stsh-parser/stsh-parse-exception.h"
#include "stsh-signal.h"
#include "stsh-job-list.h"
#include "stsh-job.h"
#include "stsh-process.h"
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>  // for fork
#include <signal.h>  // for kill
#include <sys/wait.h>
#include <assert.h>
#include <iomanip>

#include<string.h>
#include<errno.h>
using namespace std;

static STSHJobList jobList; // the one piece of global data we need so signal handlers can access it

static void handle_fg(const pipeline& pipeline) {

}

static void handle_bg(const pipeline& p) {
	
}

static void handle_slay(const pipeline& p) {
	
}

static void handle_halt(const pipeline& p) {

}

static void handle_cont(const pipeline& p) {
	
}
/**
 * Function: handleBuiltin
 * -----------------------
 * Examines the leading command of the provided pipeline to see if
 * it's a shell builtin, and if so, handles and executes it.  handleBuiltin
 * returns true if the command is a builtin, and false otherwise.
 */
static const string kSupportedBuiltins[] = {"quit", "exit", "fg", "bg", "slay", "halt", "cont", "jobs"};
static const size_t kNumSupportedBuiltins = sizeof(kSupportedBuiltins)/sizeof(kSupportedBuiltins[0]);
static bool handleBuiltin(const pipeline& pipeline) {
	return false;	
}

static void sigint_handler(int sig) {
	
}

static void sigtstp_handler(int sig) {

}
static void sigchld_handler(int sig){
	int status;
	while(true){
		pid_t pid=waitpid(-1,&status,WNOHANG);
		if(pid<0){
			//cout<<"[ERROR] Errors Occured While Waiting for Child Process";
			break;
		}else if(pid==0){
			break;
		}else{
			if(WIFSIGNALED(status)||WIFEXITED(status)){
				cout<<"[DEBUG] Child Process is Terminated (normally or abnormally)"<<endl;
				jobList.getJobWithProcess(pid).getProcess(pid).setState(kTerminated);
			}else if(WIFSTOPPED(status)){
				cout<<"[DEBUG] Child Process "<<pid<<" is stopped"<<endl;
			}
		}
	}
	cout<<jobList<<endl;
}
/**
 * Function: installSignalHandlers
 * -------------------------------
 * Installs user-defined signals handlers for four signals
 * (once you've implemented signal handlers for SIGCHLD, 
 * SIGINT, and SIGTSTP, you'll add more installSignalHandler calls) and 
 * ignores two others.
 */
static void installSignalHandlers() {
	signal(SIGCHLD,sigchld_handler);
	
}

static void showPipeline(const pipeline& p) {
	
}
void cmd2argv(command& c,char* argv[]){
	//argv=(char**)malloc((kMaxArguments+2)*sizeof(void *));
	for(unsigned int i=0;i<kMaxArguments+2;i++){
		if(i==0){
			argv[i]=c.command;
		}else{
			argv[i]=c.tokens[i-1];
			if((c.tokens[i-1])==NULL){
				break;
			}
		}
	}
}
/**
 * Function: createJob
 * -------------------
 * Creates a new job on behalf of the provided pipeline.
 */


static void createJob(const pipeline& p) {
	// showPipeline(p)
	
	string cmd="sleep 5";

	pipeline pip(cmd);
 
	char* argv[kMaxArguments+2]={NULL};
	cmd2argv(pip.commands[0],argv);

	cout<<"[DEBUG] To Execuate in Child "<<argv[0]<<" with "<<argv[1]<<endl;

	pid_t pid=fork();

	if(pid==0){
		cout<<"[DEBUG] To Execuate "<<argv[0]<<" with "<<argv[1]<<endl;
		execvp(argv[0],argv);
		// fail to execuate the command
		cout<<"[ERROR] Fail to Execute "<<argv[0]<<endl;
		cout<<"[ERROR] Failed Because of "<<strerror(errno)<<endl;
		//return -1;
	}
	int result;
	
	result=setpgid(pid,0);
	if(result<0){
		cout<<"[ERROR] Errors Occured While Setting Group ID for Child Process"<<endl;
		cout<<"[ERROR] Because of "<<strerror(errno)<<endl;
		return;
	}
	// create and set up process
	

	STSHProcess pro{pid,pip.commands[0]};
	
	// get a new job from the job list, and add process to this job
	

	jobList.addJob(kForeground).addProcess(pro);
	

	cout<<jobList<<endl;
	
	// block main process 
	sigset_t newset,oldset;
	
	sigfillset(&newset);
	sigdelset(&newset,SIGCHLD);
	//sigemptyset(&newset);
	//sigaddset(&newset,SIGCHLD);
	//sigprocmask(SIG_UNBLOCK,,&myset);
	//while(true){
	sigsuspend(&newset);
	//jobList.get	
}

/**
 * Function: main
 * --------------
 * Defines the entry point for a process running stsh.
 * The main function is little more than a read-eval-print
 * loop (i.e. a repl).  
 */
int main(int argc, char *argv[]) {
	pid_t stshpid = getpid();
	installSignalHandlers();
	rlinit(argc, argv);
	while (true) {
		string line;
		if (!readline(line)) break;
		//if (line.empty()) continue;
		try {
			pipeline p(line);
			bool builtin = handleBuiltin(p);
			if (!builtin) createJob(p);
		} catch (const STSHException& e) {
			cerr << e.what() << endl;
			if (getpid() != stshpid) exit(0); // if exception is thrown from child process, kill it
		}
	}

	return 0;
}
