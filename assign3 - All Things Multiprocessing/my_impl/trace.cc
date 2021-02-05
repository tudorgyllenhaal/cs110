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
#include<map>
#include<cerrno>
#include<unordered_map>
#include<algorithm>

#include<unistd.h>
#include<signal.h> //raise
#include<string.h>

#include<sys/ptrace.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/reg.h>


#include "string-utils.h" 
#include "trace-system-calls.h"
#include "trace-error-constants.h"
using namespace std;
#define STA_EXIT -2
#define STA_ABNORMAL -1
#define STA_IGNORE 0
#define STA_SYSCALL 1
#define STA_STOPPED 2

typedef map<int,string> sys_num_table;
typedef map<string,systemCallSignature> sys_sig_table;
typedef map<int,string> error_table;

int HandleStopped(pid_t);
int TraceSYSCALL(pid_t,bool,const sys_num_table&,const sys_sig_table&,const error_table&);
int SYSCALLEnter(pid_t,bool,const sys_num_table&,const sys_sig_table&);
int SYSCALLLeave(pid_t,int,const sys_num_table&,const sys_sig_table&,const error_table&,bool,bool);
string DtoH(unsigned long);

string DtoH(unsigned long number){
	static const char table[]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	string result="";
	unsigned int reminder=number;
	while(reminder!=0){
		result=result+table[reminder%16];
		reminder/=16;
	}
	reverse(result.begin(),result.end());
	return result;
	
}

int ReadArguements(pid_t,systemCallSignature);
string ReadString(pid_t,long);

long ReadReg(pid_t,size_t);


int IsSystemCallWrapper(int status){
	static const int mask=0x80;
	if(WIFEXITED(status)){
		cout<<"[LOG] The Child Process is Exited wtih Exied Code "<<WEXITSTATUS(status)<<endl;
		cout<<"[LOG] The Tracing Program will End"<<endl;
		return STA_EXIT;
	}
	if(WIFSIGNALED(status)){
		cout<<"[ERROR] This Chils Process is Terminated by Signal "<<WTERMSIG(status)<<endl;
		cout<<"[ERROR] This Tracing Program will End"<<endl;
		return STA_ABNORMAL;
	}
	if(WIFSTOPPED(status)){
		//cout<<"[DEBUG] Status is "<<status<<" And Signal is "<<WSTOPSIG(status)<<endl;
		if(WSTOPSIG(status)==(mask|SIGTRAP)){
			//cout<<"[DEBUG] SYSCALL!!!"<<endl;
			return STA_SYSCALL;
		}
		//cout<<"[DEBUG] Child Process is Stopped by "<<WSTOPSIG(status)<<endl;
                return STA_STOPPED;
		
	}
	if(WIFCONTINUED(status)){
		//cout<<"[DEBUG] Child Process is Continued"<<endl;
		return STA_IGNORE;
	}
	return STA_IGNORE;
}
int TraceSYSCALL(pid_t pid,bool simplemode,const sys_num_table& snt,const sys_sig_table& sst,
		const error_table& et){
	int status;
	int result;
	int syscallNum;
	bool outputControl=true; //if we can not recognize the syscall, ignore its return value
	//cout<<"Now I ma traceing this program"<<endl;
	while(true){
		result=waitpid(pid,&status,0);
		if(result<0){
			cout<<"[ERROR} Error occured while waiting for "<<pid<<endl;
			cout<<"[ERROR] Tracing is stopping"<<endl;
			return -1;
		}
		result=IsSystemCallWrapper(status);
		if(result==STA_EXIT){
			return 1;
		}else if(result==STA_ABNORMAL){
			return -1;
		}else if(result==STA_SYSCALL){
			break;
		}else if(result==STA_STOPPED){
			HandleStopped(pid);
		}
	}
	syscallNum=SYSCALLEnter(pid,simplemode,snt,sst);
	if(syscallNum<0){
		outputControl=false;
	}
	while(true){
		result=waitpid(pid,&status,0);
		if(result<0){
			cout<<"[ERROR] Error occured while waiting for "<<pid<<endl;
			cout<<"[ERROR] Tracing is stopping"<<endl;
			return -1;
		}
		result=IsSystemCallWrapper(status);
		if(result==STA_EXIT){
			return 1;
		}else if(result==STA_ABNORMAL){
			return -1;
		}else if(result==STA_SYSCALL){
			break;
		}else if(result==STA_STOPPED){
			HandleStopped(pid);
		}
	}
	
	SYSCALLLeave(pid,syscallNum,snt,sst,et,simplemode,outputControl);
	return 0;
		
}
int SYSCALLEnter(pid_t pid,bool simplemode,const sys_num_table& snt,const sys_sig_table& sst){
	static const string simpleTemplate="syscall(";
	int syscallNum=ReadReg(pid,ORIG_RAX);
	if(simplemode){
		cout<<simpleTemplate<<syscallNum<<") ";
	}else{
		//cout<<"FULL Process"<<endl;
		if(snt.find(syscallNum)==snt.cend()){
			cout<<"[LOG] SYSCALL ("<<syscallNum<<") not Found"<<endl;
			ptrace(PTRACE_SYSCALL,pid,0,0);
			return -1;
		}
		string syscallName=snt.at(syscallNum);
		if(sst.find(syscallName)==sst.cend()){
			cout<<"[LOG] SYSCALL Signature ("<<syscallName<<"("<<syscallNum<<"))"<<" is not Found"<<endl;
			ptrace(PTRACE_SYSCALL,pid,0,0);
			return -1;
		}
		systemCallSignature sign=sst.at(syscallName);
		cout<<syscallName;
		ReadArguements(pid,sign);

	}
	ptrace(PTRACE_SYSCALL,pid,0,0);
	return syscallNum;
}
int SYSCALLLeave(pid_t pid,int syscallnum,const sys_num_table& snt,
		const sys_sig_table& sst,const error_table& et,bool simplemode,bool outputcontrol){
	static const string simpleTemplate=" = ";
	static const unordered_map<string,string> returnException={{"brk","true"},{"sbrk","true"},{"mmap","true"}}; //system call that don't return interger

	// extract information about this system call
	//string syscallName=snt.at(syscallnum);
	// extract return value of system call
	int returnValue=ReadReg(pid,RAX);
	if(outputcontrol){// cannot recoganize this syscall ignore it
		if(simplemode){
			cout<<simpleTemplate<<returnValue<<endl;
		}else{
			string syscallName=snt.at(syscallnum);
			//cout<<syscallName<<endl;
			if(returnException.find(syscallName)!=returnException.cend()){// return pointer
				cout<<simpleTemplate<<" ";
				cout<<"0x"<<DtoH((unsigned int)returnValue)<<endl;
			}else{
				cout<<simpleTemplate<<returnValue<<" ";
				if(returnValue<0){ //print error information
					cout<<et.at(abs(returnValue));
					cout<<" ("<<strerror(abs(returnValue))<<")"<<endl;			
				}else{
					cout<<endl;
				}
			}
		}
	}
	ptrace(PTRACE_SYSCALL,pid,0,0);
	return 0;
}
int ReadArguements(pid_t pid,systemCallSignature sign){
	const static size_t offset[]={RDI,RSI,RDX,R10,R8,R9};
	int numArguements=sign.size();
	long content;
	//cout<<"[DEBUG] There are "<<numArguements;
	cout<<"(";
	for(auto i=0;i<numArguements;i++){
		scParamType argType=sign[i];
		content=ReadReg(pid,offset[i]);
		if(argType==SYSCALL_INTEGER){
			//cout<<"[DEBUG] Integer";
			cout<<content;
		}else if(argType==SYSCALL_STRING){
			//cout<<"[DEBUG] String";
			cout<<"\""<<ReadString(pid,content)<<"\"";
		}else if(argType==SYSCALL_POINTER){
			//cout<<"[DEBUG] Poniter";
			cout<<"0x"<<DtoH((unsigned long)content);
		}else{
			cout<<"[Unknown]";
		}
		if(i!=numArguements-1){
			cout<<",";
		}
	}
	cout<<")";
	return 0;
}
long ReadReg(pid_t pid,size_t reg){
	return ptrace(PTRACE_PEEKUSER,pid,reg*sizeof(long));
}
string ReadString(pid_t pid,long startaddr){
	long addr=startaddr;
	long content;
	string strResult="";
	char temp;
	bool conFlag=true;
	while(conFlag){
		content=ptrace(PTRACE_PEEKDATA,pid,addr,0);
		for(unsigned int i=0;i<(sizeof(long)/sizeof(char));i++){
			temp=(char)(content>>(8*i));
			if(temp=='\0'){
				conFlag=conFlag&&false;
				break;
			}else{
				strResult+=temp;
			}
		}
		addr+=(sizeof(long)/sizeof(char));
	}
	return strResult;
}
/*
void InspectArguements(pid_t pid,bool simplemode){
	//cout<<"Do something to Arguements"<<endl;

	long sysNum=ReadReg(pid,ORIG_RAX);
        cout<<"Content of System Number is "<<sysNum<<endl;
	ptrace(PTRACE_SETOPTIONS,pid,0,PTRACE_O_TRACESYSGOOD);
	ptrace(PTRACE_SYSCALL,pid,0,0);
	return;
}
void InspectReturnValue(pid_t pid,bool simplemode){
	//cout<<"Do something to Return Values"<<endl;
	long result=ReadReg(pid,RAX);
	cout<<"Result of System Call is "<<result<<endl;
	ptrace(PTRACE_SETOPTIONS,pid,0,PTRACE_O_TRACESYSGOOD);
	ptrace(PTRACE_SYSCALL,pid,0,0);
	return;
}
*/
int HandleStopped(pid_t pid){
	ptrace(PTRACE_SYSCALL,pid,0,0);
	return 0;
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

	map<int,string> syscallNumbers;
        map<string,int> syscallNames;
        map<string,systemCallSignature> syscallSignatures;
	error_table errorMessage;
	// extract syscall data
	if(!simpleMode){
		cout<<"I am running"<<endl;
		compileSystemCallData(syscallNumbers,syscallNames,syscallSignatures,false);
		compileSystemCallErrorStrings(errorMessage);
	}
	pid_t pid=fork();
	if(pid==0){
                // Child Process
		cout<<"[LOG] Child Process is Started!"<<endl;
                ptrace(PTRACE_TRACEME);
		raise(SIGTRAP);  // Stop itself, wait to parent to give signal
		execvp(argvChild[0],argvChild);
                cout<<"[ERROR] Fail to execute this program "<<argvChild[0]<<endl;
                return -1;
	}
	cout<<"[LOG] Child Process is "<<pid<<endl;
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
                cout<<"[ERROR] Child Process is Terminated by a Signal by "<<WTERMSIG(status)<<endl;
        }
	if(WIFSTOPPED(status)){
		cout<<"[LOG] Child Process is Stopped by "<<WSTOPSIG(status)<<endl;
	}
        ptrace(PTRACE_SETOPTIONS,pid,0,PTRACE_O_TRACESYSGOOD);
        ptrace(PTRACE_SYSCALL,pid,0,0);
        while(true){
		result=TraceSYSCALL(pid,simpleMode,syscallNumbers,syscallSignatures,errorMessage);
                if(result<0){
			cout<<"[LOG] Program is Terminated!"<<endl;
			return 1;
		}else if(result>0){
			cout<<"[LOG] Progam Exits"<<endl;
			return -1;
		}
			
	}
}
