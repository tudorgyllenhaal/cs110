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

static STSHJobList joblist; // the one piece of global data we need so signal handlers can access it

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
/**
 * Function: installSignalHandlers
 * -------------------------------
 * Installs user-defined signals handlers for four signals
 * (once you've implemented signal handlers for SIGCHLD, 
 * SIGINT, and SIGTSTP, you'll add more installSignalHandler calls) and 
 * ignores two others.
 */
static void installSignalHandlers() {
	
}

static void showPipeline(const pipeline& p) {
	
}

/**
 * Function: createJob
 * -------------------
 * Creates a new job on behalf of the provided pipeline.
 */
static void createJob(const pipeline& p) {
	// showPipeline(p)
	
	char *argv[]={"sleep","5",NULL};
	cout<<"milestone 2"<<endl;
	pid_t pid=fork();

	if(pid==0){
		cout<<"[DEBUG] To Execuate "<<argv[0]<<endl;
		execvp(argv[0],argv);
		// fail to execuate the command
		cout<<"[ERROR] Fail to Execute "<<argv[0]<<endl;
		cout<<"[ERROR] Failed Because of "<<errno<<endl;
		//return -1;
	}
	int status;
	int result;
	while(true){
		result=waitpid(pid,&status,0);
		if(result<0){
			cout<<"[ERROR] Errors Occured Whild Waiting for Child Process"<<endl;
			//return -1;
			return;
		}
		if(WIFEXITED(status)){
			cout<<"[DEBUG] Child Process Exited"<<endl;
			return;
			//return 0;
		}
		if(WIFSIGNALED(status)){
			cout<<"[DEBUG] Child Process is Terminated by Signal "<<WTERMSIG(status)<<endl;
			return;
			//return -1;
		}
		if(WIFSTOPPED(status)){
			cout<<"[DEBUG] Child Process is Stopped by Signal "<<WSTOPSIG(status)<<endl;
			//return -1;
		}
	}
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
