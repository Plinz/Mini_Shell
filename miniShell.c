#define _POSIX_SOURCE
#define _DEFAULT_SOURCE
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
int currentIndex = 0;
int nbJobs = 0;

void display_prompt() {
	char * u = getenv("USER");
	char * p = malloc(100);
	p = getcwd(p,100);
	char h[50];
	int i = gethostname(h,50);
	i++;
	printf("%s@%s %s $ ",u,h,p);
	fflush(stdout);
	free(p);
}

void handler_ctrlC(int sig){
	printf("Control-C reçu ! \n");
}

void handler_ctrlZ(int sig){
	printf("Control-Z reçu ! \n");
}

void handler_child(int sig){
	int pid, i;
	while ((pid = waitpid(-1, NULL, WNOHANG|WUNTRACED)) > 0){
		if (kill(pid,0) == -1){
			for (i=0; i<currentIndex && jobs[i]!=pid ; i++);
			if (i != currentIndex){		
				jobs[i] = -1;
				currentIndex = ((--nbJobs) == 0 ? 0 : currentIndex);
				printf("\n[%d]+ Fini 		pid=%d\n", i+1, pid);
				display_prompt();
			}
		} else {
			printf("Arrêt du process detecté mais pas encore géré");
		}
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
			if (l->bg)                                      setpgid(pid, pid);
			else{						signal(SIGINT,  SIG_DFL);
									signal(SIGTSTP, SIG_DFL);}	
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
		jobs[currentIndex] = pid;
		printf("[%d] %d\n",++currentIndex, pid);
	} else {
		waitpid(pid,NULL,0);
	}
}

int extra_cmd(char** word){
	int ret = 0;
	int num, i;
	if (strcmp(word[0],"jobs") == 0){
		for (i=0; i<currentIndex; i++)
			if (jobs[i] != -1)
				printf("[%d]+ En cours d'exécution pid=%d\n", i+1, jobs[i]); 
		ret = 1;	
	} else if (strcmp(word[0],"fg") == 0){

		if (word[1] != NULL){
			if ((num = atoi(word[1])) > 0 && num <= currentIndex && jobs[num-1] != -1){
				waitpid(jobs[num-1], NULL, 0);
			} else
				printf("shell: fg: %s : Tâche inexistante\n", word[1]);
		} else {
			for (i=currentIndex-1; i>=0 && jobs[i]==-1; i--);
			if (i >= 0) 	
				waitpid(jobs[i], NULL, 0);
			else 		
				printf("Aucun jobs en cours d'exécution\n");
		}
		ret = 1;		
	} else if (strcmp(word[0],"bg") == 0){
		if ((num = atoi(word[1])) > 0)
			kill(num, SIGCONT);
		ret = 1;
	} else if (strcmp(word[0],"stop") == 0){
		if ((num = atoi(word[1])) > 0)
			kill(num, SIGSTOP);
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
		
		display_prompt();
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
		else if (l->seq[0]==0)
			printf("\n");
	}
}


