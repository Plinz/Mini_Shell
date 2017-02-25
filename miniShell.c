#define _POSIX_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "readcmd.h"
#include "jobs.h"


struct jobs_etat jobs[100];
int currentIndex = 0;
int nbJobs = 0;
struct jobs_etat fg;

int getIndexJobByPid(int pid){
	int i;
	for(i = 0; i < currentIndex && jobs[i].pid != pid; i++);
	if (i == currentIndex)
		i = -1;
	return i;
}

void changeEtat(int pid, int new_etat){
	int i = getIndexJobByPid(pid);
	if(i == -1)
		printf("Le processus à changer d'état n'a pas été trouvé.\n");
	else
		jobs[i].etat = new_etat; 
}

struct jobs_etat createWithInfos(int pid, int etat, char * nom){
	struct jobs_etat retour;
	retour.pid = pid;
	retour.etat = etat;
	retour.nom = (char *)malloc(strlen(nom)+1);
	strcpy(retour.nom,nom);
	return retour;
}

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

void handler_ctrlc(int sig){
	printf("\n");
	if (fg.pid > 0)
		kill(fg.pid, sig);
	else
		display_prompt();
}

void handler_ctrlz(int sig){
	if (fg.pid > 0){
		jobs[currentIndex] = createWithInfos(fg.pid,STOPPED, fg.nom);
		kill(jobs[currentIndex].pid, sig);
		currentIndex++;
		nbJobs++;
		printf("\n[%d]+  %s\t\t\t%s\n",currentIndex,etat_jobs[jobs[currentIndex-1].etat] ,jobs[currentIndex-1].nom);
		fg.pid = -1;
	} else {
		printf("\n");
		display_prompt();
	}
}

void handler_child(){
	int pid, i;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0){
		for (i=0; i<currentIndex && jobs[i].pid!=pid ; i++);
		if (i != currentIndex){		
			jobs[i].pid = -1;
			if (i == currentIndex - 1) currentIndex--;
			currentIndex = ((--nbJobs) == 0 ? 0 : currentIndex);
			printf("\n[%d]+  Fini\t\t\t%s\n", i+1, jobs[i].nom);
			display_prompt();
		}
	}		
}

void redirection(char* in_out, int pip){
	int fd = open(in_out, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	dup2(fd, pip);
	close(fd);
}

int getIndexFromCmd(char ** word){
	int num;
	if (word[1][0] == '%' && (num = atoi(&word[1][1])) > 0 && --num < currentIndex && jobs[num].pid != -1);
	else if (word[1][0] != '%' && (num = getIndexJobByPid(atoi(word[1]))) != -1);
	else num = -1;
	return num;
}

void bgAndStopCore(char ** word, char* cmd, char * string, int oldEtat, int newEtat, int sig){
	int num;	
	if (word[1] != NULL){
		if ((num = getIndexFromCmd(word)) != -1){
			if (jobs[num].etat == oldEtat){
				kill(jobs[num].pid, sig);
				changeEtat(jobs[num].pid,newEtat);
				printf("\n[%d]+ %s &\n", num+1, jobs[num].nom);
			} else
				printf("shell: %s: la tâche %d est déjà %s\n", cmd, num+1, string);
		} else
			printf("shell: %s: %s : tâche inexistante\n", cmd, word[1]);
	} else {
		for (num=currentIndex-1; num>=0 && jobs[num].pid==-1 && jobs[num].etat != oldEtat; num--);
		if (num >= 0) {	
			kill(jobs[num].pid, sig);
			changeEtat(jobs[num].pid,newEtat);
			printf("\n[%d]+ %s &\n", num+1, jobs[num].nom);
		} else {
			for (num=currentIndex-1; num>=0 && jobs[num].pid==-1; num--);
			if (num >= 0)
				printf("shell: %s: la tâche %d est déjà %s\n", cmd, num, string);
			else
				printf("shell: %s: courant : tâche inexistante\n", cmd);
		}
	}

}

void fgProcess(int num){
	fg = createWithInfos(jobs[num].pid,RUNNING, jobs[num].nom);
	jobs[num].pid = -1;
	if (num == currentIndex -1) currentIndex--;
	currentIndex = ((--nbJobs) == 0 ? 0 : currentIndex);
	kill(fg.pid, SIGCONT);
	printf("%s\n",jobs[num].nom);
	waitpid(fg.pid, NULL, WUNTRACED);
}

void fgCore(char ** word){
	int num;
	if (word[1] != NULL){
		if ((num = getIndexFromCmd(word)) != -1)
			fgProcess(num);
		else
			printf("shell: fg: %s : tâche inexistante\n", word[1]);
	} else {
		for (num=currentIndex-1; num>=0 && jobs[num].pid==-1; num--);
		if (num >= 0)
			fgProcess(num);
		else 		
			printf("bash: fg: courant : tâche inexistante\n");
	}
}

int extra_cmd(char** word){
	int ret = 0;
	int i;
	if (strcmp(word[0],"jobs") == 0){
		for (i=0; i<currentIndex; i++)
			if (jobs[i].pid != -1)
				printf("[%d]   %s\t\t\t%s\n", i+1,etat_jobs[jobs[i].etat], jobs[i].nom); 
		ret = 1;	
	} else if (strcmp(word[0],"fg") == 0){
		fgCore(word);
		ret = 1;		
	} else if (strcmp(word[0],"bg") == 0){
		bgAndStopCore(word, "bg", "en arrière plan", STOPPED, RUNNING, SIGCONT);
		ret = 1;
	} else if (strcmp(word[0],"stop") == 0){
		bgAndStopCore(word, "stop", "arrêtée", RUNNING, STOPPED, SIGSTOP);
		ret = 1;	
	}
	return ret;
}

void run_cmd(struct cmdline *l){
	int pid = 0;
	int index = 0;
	int pipefd[2];
	int fd_in = 0;
	int status;
	fg = createWithInfos(-1,STOPPED, "");
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
			if (execvp(l->seq[index][0],l->seq[index]) == -1){
				printf("%s: command not found\n",l->seq[index][0]);
				exit(0);
			}
		} else {
			close(pipefd[1]);
			fd_in = pipefd[0];
			index++;
		}
	}
	if(l->bg){
		jobs[currentIndex++] = createWithInfos(pid,RUNNING,l->seq[0][0]);
		nbJobs++;
		printf("[%d] %d\n",currentIndex,pid);
	} else {
		fg = createWithInfos(pid,RUNNING, l->seq[0][0]);;
		waitpid(pid,&status,WUNTRACED);
	}
}

int main()
{
	int i;
	signal(SIGINT, handler_ctrlc);
	signal(SIGTSTP, handler_ctrlz);
	signal(SIGCHLD, handler_child);

	while (1) {
		struct cmdline *l;
		
		display_prompt();
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			for (i=0; i<currentIndex; i++)
				if (jobs[i].pid != -1)
					kill(jobs[i].pid, SIGINT);
			printf("\n");			
			exit(0);
		}
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}
		if(l->seq[0]!=0 && !extra_cmd(l->seq[0]))
			run_cmd(l);
	}
}


