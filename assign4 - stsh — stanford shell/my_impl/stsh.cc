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

static void handle_fg(const pipeline& p) {
	cout<<"[DEBUG] Get into fg handler"<<endl;
	int jobNum=atoi(p.commands[0].tokens[0]);
	if(jobList.containsJob(jobNum)){
		pid_t gpid=jobList.getJob(jobNum).getGroupID();
		int result=kill(-gpid,SIGCONT);
		if(result<0){
			cout<<"[DEBUG] Failt to Send SIGCONT"<<endl;
		}
		// set it to foreground
		jobList.getJob(jobNum).setState(kForeground);
	}else{
		throw STSHException("No Such Job Number");
	}
	sigset_t myset;

        sigfillset(&myset);
        sigdelset(&myset,SIGCHLD);
        sigdelset(&myset,SIGINT);
        sigdelset(&myset,SIGTSTP);
        while(true){
		cout<<"[DEBUG] Handle Foreground Job"<<endl;
                sigsuspend(&myset);
                if(!jobList.hasForegroundJob()){
                        break;
                }
        }
        cout<<"[DEBUG] No Foreground Jobs"<<endl;
}

static void handle_bg(const pipeline& p) {
	cout<<"[DEBUG] Get into bg handler"<<endl;
	int jobNum=atoi(p.commands[0].tokens[0]);
        if(jobList.containsJob(jobNum)){
                pid_t gpid=jobList.getJob(jobNum).getGroupID();
                int result=kill(-gpid,SIGCONT);
                if(result<0){
                        cout<<"[DEBUG] Failt to Send SIGCONT"<<endl;
                }
                // set it to foreground
         	jobList.getJob(jobNum).setState(kBackground);
        }else{
                throw STSHException("No Such Job Number");
	}
}

static void handle_slay(const pipeline& p) {
	cout<<"[DEBUG] Get into slay handler"<<endl;
	int procNum=atoi(p.commands[0].tokens[0]);
	if(jobList.containsProcess(procNum)){
		int result=kill(procNum,SIGINT);
		if(result<0){
			cout<<"[DEBUG] Fail to Send SIGINT"<<endl;
		}
	}else{
		cout<<"[DEBUG] No Such Process Number"<<endl;
	}
}

static void handle_halt(const pipeline& p) {
	cout<<"[DEBUG] Get into halt handler"<<endl;
        int procNum=atoi(p.commands[0].tokens[0]);
        if(jobList.containsProcess(procNum)){
                int result=kill(procNum,SIGTSTP);
                if(result<0){
                        cout<<"[DEBUG] Fail to Send SIGTSTP"<<endl;
                }
        }else{
                cout<<"[DEBUG] No Such Process Number"<<endl;
        }
}

static void handle_cont(const pipeline& p) {
	cout<<"[DEBUG] Get into cont handler"<<endl;
        int procNum=atoi(p.commands[0].tokens[0]);
        if(jobList.containsProcess(procNum)){
                int result=kill(procNum,SIGCONT);
                if(result<0){
                        cout<<"[DEBUG] Fail to Send SIGCONT"<<endl;
                }
        }else{
                cout<<"[DEBUG] No Such Process Number"<<endl;
        }
}
static void handle_jobs(const pipeline& p){
	cout<<jobList<<endl;
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
static bool handleBuiltin(const pipeline& p) {
	const char* command=p.commands[0].command;
	int matched=-1;
	for(size_t i=0;i<kNumSupportedBuiltins;i++){
		if(strcmp(command,kSupportedBuiltins[i].c_str())==0){
			matched=i;
			break;
		}
	}
	//cout<<"Matched Number is "<<matched<<endl;
	switch(matched){
		case 0:
			exit(0);
			break;
		case 1:
			exit(0);
			break;
		case 2:
			handle_fg(p);
			break;
		case 3:
			handle_bg(p);
			break;
		case 4:
			handle_slay(p);
			break;
		case 5:
			handle_halt(p);
			break;
		case 6:
			handle_cont(p);
			break;
		case 7:
			handle_jobs(p);
			break;
		default:
			return false;
	}
	return true;
}



static void sigint_handler(int sig) {
	if(jobList.hasForegroundJob()){
		pid_t gpid=jobList.getForegroundJob().getGroupID();
		int result=kill(-gpid,SIGINT);
		if(result<0){
			cout<<"[ERROR] Failt Sending Signal SIGINT to "<<-gpid<<endl;
		}
	}else{
		exit(0);
	}
	
}

static void sigtstp_handler(int sig) {
	if(jobList.hasForegroundJob()){
                pid_t gpid=jobList.getForegroundJob().getGroupID();
                int result=kill(-gpid,SIGTSTP);
                if(result<0){
                        cout<<"[ERROR] Failt Sending Signal SIGTSTP to "<<-gpid<<endl;
                }else{
			cout<<"[DEBUG] SIGTSTP is Send"<<endl;
		}
        }
}
static void sigchld_handler(int sig){
	int status;
	while(true){
		pid_t pid=waitpid(-1,&status,WNOHANG|WUNTRACED|WCONTINUED);
		if(pid<0){
			//cout<<"[ERROR] Errors Occured While Waiting for Child Process";
			break;
		}else if(pid==0){
			//cout<<"[DEBUG] Nothing to Return"<<endl;
			break;
		}else{
			if(WIFSIGNALED(status)){
				//cout<<"[DEBUG] Child Process is Terminated (normally or abnormally)"<<endl;
				jobList.getJobWithProcess(pid).getProcess(pid).setState(kTerminated);
				jobList.synchronize(jobList.getJobWithProcess(pid));
				//cout<<jobList<<endl;
				//cout<<"[DEBUG]"<<endl;
				cout<<"[LOG] Child Process "<<pid<<" is Terminated by "<<WTERMSIG(status)<<endl;
				cout<<jobList<<endl;
			}else if(WIFEXITED(status)){
				jobList.getJobWithProcess(pid).getProcess(pid).setState(kTerminated);
                                jobList.synchronize(jobList.getJobWithProcess(pid));
				if(WEXITSTATUS(status)<0){
					cout<<"[LOG] Child Process Exited Abornamlly with Exit Number "<<WEXITSTATUS(status)<<endl;
					cout<<jobList<<endl;
				}
			}else if(WIFSTOPPED(status)){
				//cout<<"[DEBUG] Child Process "<<pid<<" is stopped"<<endl;
				jobList.getJobWithProcess(pid).getProcess(pid).setState(kStopped);
                                jobList.synchronize(jobList.getJobWithProcess(pid));
				cout<<jobList<<endl;
			}else if(WIFCONTINUED(status)){
				//cout<<"[DEBUG] Chils process "<<pid<< "is continued"<<endl;
				jobList.getJobWithProcess(pid).getProcess(pid).setState(kRunning);
				jobList.synchronize(jobList.getJobWithProcess(pid));
				cout<<jobList<<endl;
			}else{
				//cout<<"[DEBUG] Child State Changed"<<endl;
			}
		}
	}
	//cout<<jobList<<endl;
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
	signal(SIGINT,sigint_handler);
	signal(SIGTSTP,sigtstp_handler);
	signal(SIGCHLD,sigchld_handler);
	
}

static void showPipeline(const pipeline& p) {
	
}
void cmd2argv(const command& c,char* argv[]){
	//argv=(char**)malloc((kMaxArguments+2)*sizeof(void *));
	for(unsigned int i=0;i<kMaxArguments+2;i++){
		if(i==0){
			argv[i]=const_cast<char*>(c.command);
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
int transfterTerminalControl(const char* argv,pid_t pgid){
	static const vector<string> approvedProg={"cat","more","emacs","vi"};
	for(auto it=approvedProg.begin();it!=approvedProg.end();it++){
		if(strcmp(argv,it->c_str())==0){
			int result=tcsetpgrp(getpid(),pgid);
			if(result>=0){
				return 1;
			}
			return -1;
		}
	}
	return -1;
}

static void createJob(const pipeline& pip) {
	char* argv[kMaxArguments+2]={NULL};
	int fds[2];
	int pipeIn;
	int pipeOut;
	pid_t gpid;
	for(unsigned int i=0;i<pip.commands.size();i++){

		// block the handling of child process until we are good with its data structure
		sigset_t newset,oldset; 
                sigemptyset(&newset);
                sigaddset(&newset,SIGCHLD);
                sigprocmask(SIG_BLOCK,&newset,&oldset);

		// prepare for pipeline 
		int result;
		//cout<<"[DEBUG] i is "<<i<<endl;
		if(i!=pip.commands.size()-1){
			result=pipe(fds);
			assert(result>=0);
			pipeOut=fds[1]; //used as standard Out for this comman
			
		}
		
		// perpare argv for new program
		cmd2argv(pip.commands[i],argv);

		// fork a new process
		pid_t pid=fork();

		while(pid==0){// failure in child process should result in a exit not a return 
			if(i!=pip.commands.size()-1){ // dup for standard output
				close(fds[0]); // this is for the next process, we have no use of it
				result=dup2(pipeOut,STDOUT_FILENO); // exchange for standard out
				assert(result>0);
			}else{ // check if there is a redirection
				if(!pip.output.empty()){
					int reFd=open(pip.output.c_str(),O_WRONLY|O_APPEND|O_CREAT,0644);
					if(reFd>=0){
						result=dup2(reFd,STDOUT_FILENO);
						assert(result>=0);
					}else{
						cerr<<"[ERROR] Fail to Open(Creat) File "<<pip.output<<" Because "<<strerror(errno)<<endl;
						exit(-1);
					}
				}
						
			}
			if(i!=0){
				result=dup2(pipeIn,STDIN_FILENO); // exchange for standard in
				
			}else{// check if there is a redirection
				if(!pip.input.empty()){
					int reFd=open(pip.input.c_str(),O_RDONLY);
					if(reFd>=0){
						result=dup2(reFd,STDIN_FILENO);
						assert(result>=0);
				
					}else{
						cerr<<"[ERROR] Open "<<pip.input<<" Failed Because "<<strerror(errno)<<endl;
						exit(-1);
						
					}
				}
			}
			execvp(argv[0],argv); // new program
			cerr<<"[ERROR] Failt to Execute Because of "<<strerror(errno)<<endl;
			exit(-1);
		}
		// parent process
		if(i!=0){
			close(pipeIn); //close the input end of pipeline
		}
		if(i!=pip.commands.size()-1){ 
			pipeIn=fds[0];       // prepare standard in for the next comand
			close(pipeOut);      // close standard out for this command		
		}
        	
		// set group id
		if(i==0){
			result=setpgid(pid,0);
			gpid=pid;
		}else{
			result=setpgid(pid,0);
		}
        	if(result<0){ // error process
			cout<<"[DEBUG] Try to set "<<pid<< " to "<<gpid<<endl; 
                	cout<<"[ERROR] Errors Occured While Setting Group ID for Child Process"<<endl;
                	cout<<"[ERROR] Because of "<<strerror(errno)<<endl;
			return;
                }

		result=transfterTerminalControl(argv[0],pid);
		if(result>0){
			cout<<"[DEBUG] Control of Terminal is Transfer to Child Process"<<endl;
		}
		// create process
		STSHProcess pro{pid,pip.commands[i]};
		if(!pip.background){
	                jobList.addJob(kForeground).addProcess(pro);
	                cout<<jobList<<endl;
	                
	                // block main process 
	                sigset_t myset;
			 sigfillset(&myset);
	                sigdelset(&myset,SIGCHLD);
	                sigdelset(&myset,SIGINT);
	                sigdelset(&myset,SIGTSTP);
	                while(true){
	                        sigsuspend(&myset);

	                        if(!jobList.hasForegroundJob()){
	                                break;
	                        }
	                }
	                sigprocmask(SIG_SETMASK,&oldset,0); //get back to origin signal set
	        }else{
	                jobList.addJob(kBackground).addProcess(pro);
	                cout<<jobList<<endl;
	                sigprocmask(SIG_SETMASK,&oldset,0);
	        }
		
        }


		
		
	
	/*
	cmd2argv(pip.commands[0],argv);
	//bool isInBackground=pip.background;

	//cout<<"[DEBUG] To Execuate in Child "<<argv[0]<<" with "<<argv[1]<<endl;

	pid_t pid=fork();

	if(pid==0){
		//cout<<"[DEBUG] To Execuate "<<argv[0]<<" with "<<argv[1]<<endl;
		execvp(argv[0],argv);
		// fail to execuate the command
		cout<<"[ERROR] Fail to Execute "<<argv[0]<<endl;
		cout<<"[ERROR] Failed Because of "<<strerror(errno)<<endl;
		//return -1;
	}
	int result;
	sigset_t newset,oldset; // block child sigchld until data is add into data structure
	sigemptyset(&newset);
	sigaddset(&newset,SIGCHLD);
	sigprocmask(SIG_BLOCK,&newset,&oldset);

	result=setpgid(pid,0);
	if(result<0){
		cout<<"[ERROR] Errors Occured While Setting Group ID for Child Process"<<endl;
		cout<<"[ERROR] Because of "<<strerror(errno)<<endl;
		return;
	}
	

	// create and set up process
	STSHProcess pro{pid,pip.commands[0]};
	
	// get a new job from the job list, and add process to this job
	
	if(!pip.background){
		jobList.addJob(kForeground).addProcess(pro);
		cout<<jobList<<endl;
		
       		// block main process 
        	sigset_t myset;

        	sigfillset(&myset);
        	sigdelset(&myset,SIGCHLD);
        	sigdelset(&myset,SIGINT);
        	sigdelset(&myset,SIGTSTP);
		while(true){
                	sigsuspend(&myset);
		
                	if(!jobList.hasForegroundJob()){
                        	break;
                	}
        	}
		sigprocmask(SIG_SETMASK,&oldset,0); //get back to origin signal set
        	//cout<<"[DEBUG] No Foreground Jobs"<<endl;
	}else{
		//cout<<"[DEBUG] In Background"<<endl;
		jobList.addJob(kBackground).addProcess(pro);
		cout<<jobList<<endl;
		sigprocmask(SIG_SETMASK,&oldset,0);
	}
	*/
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
		if (line.empty()) continue;
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
