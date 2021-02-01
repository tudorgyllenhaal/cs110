/**
 * File: pipeline.c
 * ----------------
 * Presents the implementation of the pipeline routine.
 */

#include "pipeline.h"
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>

void pipeline(char *argv1[], char *argv2[], pid_t pids[]) {
	//TODO
	int fds[2];
	int result=pipe(fds);
	if(result<0){
		printf("[Error] Fail to open pipe!\n");
		return;
	}

	
	pid_t pid1=fork();
	if(pid1==0){
		// child process 1
		
		close(fds[0]); // close read
		
		result=dup2(fds[1],STDOUT_FILENO);
		
		if(result<0){
			printf("[ERROR] An Error occured during dup2, the errno is %d\n",errno);
			return;
		}
		close(fds[1]);
		
		execvp(argv1[0],argv1);
		// execvp never return
		printf("[Error] Fail to execuate program %s!\n",argv1[0]);
		return;
	}
	pids[0]=pid1;
    
	pid_t pid2=fork();

	if(pid2==0){
		// child process 2
		
		close(fds[1]);
		result=dup2(fds[0],STDIN_FILENO);
		
		if(result<0){
			printf("[ERROR] An Error occured during dup2, the errno is %d\n",errno);
			return;
		}
		close(fds[0]);
		
		execvp(argv2[0],argv2);
		printf("[ERROR] Fail to execuate program %s!\n",argv2[0]);
		return;
	}
	pids[1]=pid2;
	close(fds[0]);
	close(fds[1]);
	
	//printf("[DEBUG] %d : %d\n",pids[0],pids[1]);
	//printf("[DEBUG] Program returned!\n");
	return;

}
