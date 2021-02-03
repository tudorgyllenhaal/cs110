/**
 * File: trace.cc
 * ----------------
 * Presents the implementation of the trace program, which traces the execution of another
 * program and prints out information about ever single system call it makes.  For each system call,
 * trace prints:
 *
 *    + the name of the system call,
 *    + the values of all of its arguments, and
 *    + the system calls return value
 */


#include<iostream>
#include<string>

#include<unistd.h>
#include<signal.h> //raise

#include<sys/ptrace.h>
#include<sys/types.h>
#include<sys/wait.h>



#include "string-utils.h" 
using namespace std;
#define STA_EXIT -2
#define STA_ABNORMAL -1
#define STA_IGNORE 0
#define STA_SYSCALL 1
#define STA_STOPPED 2
int IsSystemCallWrapper(int status){
	static const int mask=0x80;
	if(WIFEXITED(status)){
		cout<<"[DEBUG] The Child Process is Exited wtih Exied Code "<<WEXITSTATUS(status)<<endl;
		cout<<"[DEBUG] The Tracing Program will End"<<endl;
		return STA_EXIT;
	}
	if(WIFSIGNALED(status)){
		cout<<"[ERROR] This Chils Process is Terminated by Signal "<<WTERMSIG(status)<<endl;
		cout<<"[ERROR] This Tracing Program will End"<<endl;
		return STA_ABNORMAL;
	}
	if(WIFSTOPPED(status)){
		cout<<"[DEBUG] Status is "<<status<<" And Signal is "<<WSTOPSIG(status)<<endl;
		if(WSTOPSIG(status)==mask+SIGTRAP){
			cout<<"[DEBUG] SYSCALL!!!"<<endl;
			return STA_SYSCALL;
		}
		cout<<"[DEBUG] Child Process is Stopped by "<<WSTOPSIG(status)<<endl;
                return STA_STOPPED;
		
	}
	if(WIFCONTINUED(status)){
		cout<<"[DEBUG] Child Process is Continued"<<endl;
		return STA_IGNORE;
	}
	return STA_IGNORE;
}

void InspectArguements(pid_t pid,bool simplemode){
	cout<<"Do something to Arguements"<<endl;
	ptrace(PTRACE_SYSCALL,pid,0,0);
	return;
}
void InspectReturnValue(pid_t pid,bool simplemode){
	cout<<"Do something to Return Values"<<endl;
	ptrace(PTRACE_SYSCALL,pid,0,0);
	return;
}

int main(int argc, char *argv[]){
	bool simpleMode=true;
	char ** argvChild;
	if(argc<2){
		cout<<"Usage : ./trace [--simple|--full] <traced program>"<<endl;
		return  0;
	}
	string arg1(argv[1]);
	if(StringUtils::StartsWith(arg1,"--")){

		if(arg1.find_first_of("simple")==2){
			simpleMode=true;
			cout<<"Full simple"<<endl;
		}else if(arg1.find_first_of("full")==2){
			simpleMode=false;
			cout<<"Full full"<<endl;
		}
		argvChild=argv+2;

	}else if(StringUtils::StartsWith(arg1,"-")){
		if(arg1.find_first_of("s")==1){
			simpleMode=true;
			cout<<"Simple simple"<<endl;
		}else if(arg1.find_first_of("f")==1){
			simpleMode=false;
			cout<<"Simple full"<<endl;
		}
		argvChild=argv+2;
	}else{
		argvChild=argv+1;
		cout<<"No option"<<endl;

	}
	pid_t pid=fork();
	if(pid==0){
                // Child Process
		cout<<"Child Process is Started!"<<endl;
                ptrace(PTRACE_TRACEME);
		raise(SIGTRAP);  // Stop itself, wait to parent to give signal
		execvp(argvChild[0],argvChild);
                cout<<"[ERROR] Fail to execute this program "<<*argvChild[0]<<endl;
                return -1;
	}
	cout<<"[DEBUG] Child Process is "<<pid<<endl;
        // Parent Process
	int status;
	int result=0;
        // long addr;
        //int data;

	result=waitpid(pid,&status,0);
	if(result<0){
		cout<<"[Error] Something wrong occured!"<<endl;
		return -1;
	}
        if(WIFSIGNALED(status)){
                cout<<"Child Process is Terminated by a Signal by "<<WTERMSIG(status)<<endl;
        }
	if(WIFSTOPPED(status)){
		cout<<"Child Process is Stopped by "<<WSTOPSIG(status)<<endl;
	}
        ptrace(PTRACE_SETOPTIONS,pid,0,PTRACE_O_TRACESYSGOOD);
        ptrace(PTRACE_SYSCALL,pid,0,0);
        while(true){
                // first stop, inspect arguements
		cout<<"[DEBUG] Waiting for system call"<<endl;
		while(true){
			result=waitpid(pid,&status,0);
			if(result<0){
				cout<<"[ERROR] Something wrong occured!"<<endl;
				cout<<"[ERROR] Tracing is terminated!"<<endl;
				return -1;
			}
			result=IsSystemCallWrapper(status);
			if(result==STA_EXIT){
				return 0;
			}else if(result==STA_ABNORMAL){
				return -1;
			}else if(result==STA_STOPPED){
				return -1;
			}else if(result==STA_SYSCALL){
				InspectArguements(pid,simpleMode);
				break;
			}
		}
		cout<<"[DEBUG] Waiting for system call to return!"<<endl;
		//cout<<"[DEBUS] Is it continue? "<<WIFCONTINUED(status)<<endl;
		while(true){
			result=waitpid(pid,&status,0);
			if(result<0){
				cout<<"[ERROR] Something goes wrong!"<<endl;
				cout<<"[ERROR] Tracing is stopped!"<<endl;
				return -1;
			}
			result=IsSystemCallWrapper(status);
                        if(result==STA_EXIT){
                                return 0;
                        }else if(result==STA_ABNORMAL){
                                return -1;
                        }else if(result==STA_STOPPED){
                                return -1;
                        }else if(result==STA_SYSCALL){
                                InspectReturnValue(pid,simpleMode);
                                break;  
                        }
		}
	}
	
	return -1;
}
