
const int STOPPED = 0;

const int RUNNING = 1;

const char * etat_jobs[] = { "Stopped", "Running" };

struct jobs_etat {
	int pid;	//PID du job 
	int etat;	//0 pour stopped et 1 pour running. Pas d'autres valeurs autorisées !
	//char etat_activite;	//A implémenter .
	char * nom;	//Nom de la commande
};
