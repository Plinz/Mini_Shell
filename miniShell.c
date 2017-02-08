/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "readcmd.h"

int jobs[100];
int p = 0;

void handler_ctrlC(int sig){
	printf("Control-C reçu ! \n");
}

void handler_ctrlZ(int sig){
	printf("Control-Z reçu ! \n");
}

void handler_child(int sig){
	int pid;
	pid = waitpid(-1, NULL, WNOHANG|WUNTRACED);
	//printf("pid=%d et p=%d\n",pid, p);
	if (pid > 0){
		int i;
		for (i=0; i<p && jobs[i]!=pid ; i++);		
		jobs[i] = jobs[p-1];
		p--;
	}
	
}

void redirection(char* in_out, int pip){
	int fd = open(in_out, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	dup2(fd, pip);
	close(fd);
}

void run_cmd(struct cmdline *l){
	int pid = 0;
	int index = 0;
	int pipefd[2];
	int fd_in = 0;
	while(l->seq[index]!=0){
		pipe(pipefd);
		if((pid = fork()) == 0){
			if (index == 0 && l->in != NULL) 		redirection(l->in,0);
			else  						dup2(fd_in, 0);
			if (l->seq[index+1] == 0 && l->out != NULL)	redirection(l->out,1);
			else if (l->seq[index+1] != 0) 			dup2(pipefd[1], 1);
			dup2(fd_in, 0);
			close(pipefd[0]);
			if (execvp(l->seq[index][0],l->seq[index]) == -1)
				exit(0);
		} else {
			close(pipefd[1]);
			fd_in = pipefd[0];
			index++;
		}
	}
	if(l->bg){
		jobs[p++] = pid;
	} else {
		waitpid(pid,NULL,0);
	}
}

int extra_cmd(char** word){
	int ret = 0;
	int pid;
	if (strcmp(word[0],"jobs") == 0){
		for (int i=0; i<p; i++)
			printf("[%d]+ En cours d'exécution pid=%d\n", i+1, jobs[i]); 
		ret = 1;	
	} else if (strcmp(word[0],"fg") == 0){
		if ((pid = atoi(word[1])) > 0)
			waitpid(pid, NULL, 0);
		ret = 1;	
	} else if (strcmp(word[0],"bg") == 0){
		if ((pid = atoi(word[1])) > 0)
			kill(pid, SIGCONT);
		ret = 1;
	} else if (strcmp(word[0],"stop") == 0){
		if ((pid = atoi(word[1])) > 0)
			kill(pid, SIGSTOP);
		ret = 1;	
	}
	return ret;
}

int main()
{

	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGCHLD, handler_child);


	while (1) {
		struct cmdline *l;
		
		printf("\x1b[31mminiShell>\x1b[31m");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}
		/**
		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);

		Display each command of the pipe 
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			//printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
				//printf("%s ", cmd[j]);
			}
			printf("\n");
		}**/
		if(l->seq[0]!=0 && !extra_cmd(l->seq[0]))
			run_cmd(l);
	}
}


