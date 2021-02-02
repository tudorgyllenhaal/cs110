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
#include<signal.h>
#include<sys/types.h>
#include<sys/ptrace.h>
#include<sys/wait.h>



#include "string-utils.h" 
using namespace std;

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

		ptrace(PT_TRACE_ME,0,0,0);
		raise(SIGTRAP);
		cout<<"Another program is executed!"<<endl;
		execvp(argvChild[0],argvChild);
	}

	int status;
	int result=0;
	result=waitpid(pid,&status,0);
	if(result<0){
		printf("[Error] Something wrong occured!\n");
		return -1;
	}
	if(WIFSTOPPED(status)){
		cout<<"I feel it is stoped"<<endl;
	}

	cout<<"It is stopped"<<endl;

	
	return 0;

}

