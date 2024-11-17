#include <stdio.h>
#include <errno.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>

char user [30] ;
char receiver [30] ;
bool is_bot = false ;
bool is_manuel = false ;

char send_path[71];
char receive_path[71];

int char_array_len(char char_array[]);
bool char_array_identical(char arr1[], char arr2[]);
void check_pseudo(char pseudo[]);
void check_parameters(char param[]);
void init_check_prog_param(int argc, char* argv[]);
void init_paths();
int generate_random_num(int min, int max);
void end_send_process(int sig);
void sig_send_process_handler (int sig);


int main(int argc, char* argv[]) {
	
	init_check_prog_param(argc, argv);
	init_paths();
	
	errno = 0 ;
	int mkdir_feedback = mkdir("tmp", 0777);
	if ( mkdir_feedback == -1 && errno != EEXIST) {
    perror("Directory creation failed");
	}
	// on cree un segement de donnees communnes
	// la cle doit etre generer de facon aleatoir sinon tout les programmes chat auront le meme espace commun
	srand(time(0));
	int shared_space_status, key = generate_random_num(1, 1000); 
	size_t commun_space_size = 41 * 100; // 41 est le nombre de messages et 100 est la taille d'un message
	// si on a 41 message ca veut dire qu'il y a plus de 4096 octects a afficher 
	char (*messages)[100]; 
	shared_space_status = shmget(key, commun_space_size, IPC_CREAT|IPC_EXCL|0600);
	if (shared_space_status == -1){
		if (errno != EEXIST){
			perror("Failed to create commun memory"); exit(1);}
		else{// si on a par hasard la meme cle on va s'assurer qu'on a une differente cle !
			int new_key = key;
			while(new_key == key){
				new_key = generate_random_num(1, 1000);
			}
		}}
	messages = shmat(shared_space_status, NULL, 0);
	if (messages == (void *) -1) {perror("Failed to attach the shared memory segment"); exit(1);}
	
	
	// on va fork 
	pid_t p_id = fork();
	if (p_id < 0){
		perror("Le fork n'a pas réussi !");
	}else if(p_id > 0){
		mkfifo(send_path, 0666);
		int message_size = 100;
		char message[message_size];

		signal(SIGTERM, end_send_process);
		signal(SIGPIPE, sig_send_process_handler);
		
		while(!char_array_identical(message, "exit")){
			
			if ((fgets(message, message_size, stdin))== NULL){
				perror("stdin Failed");
			} // il nous faut une fonction trim
			// le fgets ajoute \n ; si la ligne ne fait pas completement la taille permis au message 
			//;a la fin de la ligne et on doit l'enlever
			int message_len = char_array_len(message);
			size_t message_size_to_write = (size_t)(message_len + 1);
			if (message_len > 0 && message[message_len - 1] == '\n'){
				message[message_len - 1] = '\0';
			}
			
			printf("\033[F");  // Déplacer le curseur d'une ligne vers le haut
			printf("\033[K");  // Effacer la ligne du curseur à la fin
			
			printf("[%s] : %s \n",user, message);
			
			int fd_writer = open(send_path, O_WRONLY);
			if (fd_writer == -1) {
			perror("Failed to open the pipe");
			close(fd_writer);
			exit(1);
			}
			
			
			if ((write(fd_writer, message, message_size_to_write)) == -1){
				perror("Failed to send message");
			}
			close(fd_writer);
			
			
		}
		// on suprime le pipe 
		unlink(send_path);
		// on suprime le segement de memeoire partage
		if (shmctl(shared_space_status, IPC_RMID, NULL) < 0) {
        perror("shmctl failed");
        exit(1);
		}
		
		
	}else if (p_id == 0){
		int message_size = 100;
		size_t message_size_to_read = (size_t)message_size;
		char message[message_size];
		int parent_pid = getppid();
		
		printf("Connecting ...\n");
		while (true){
			if (access(receive_path, F_OK) == 0) {
				break ;
			}
		}
		printf("Connected ! \n");
		while(parent_pid == getppid()){
			
			int fd_reader = open(receive_path, O_RDONLY);
			if (fd_reader == -1) {
			// cette erreur n'arrive que s'on a perdu la connextion avec l'autre programme chat
			//perror("Failed to open the pipe");
			close(fd_reader);
			// on va tuer le processus pere parce qu'il n'y a plus de connexion, 
			printf("Disconected ! \n");
			kill(getppid(), SIGTERM);
			// on suprime le segement de memeoire partage
			if (shmctl(shared_space_status, IPC_RMID, NULL) < 0) {
				perror("shmctl failed");
				exit(1);
			}
			exit(0);
			}
			
			if ((read(fd_reader, message, message_size_to_read))== -1){
				perror("Failed to read message");
			}
			if (parent_pid == getppid()){printf("[%s] : %s \n", receiver, message);}
			
			close(fd_reader);
			
		}
		
		
	}
	
	
	
   return 0;
}

int char_array_len(char char_array[]){
	/*
	Cette fonction compte la longueur d'une chaine de caracteres 
	*/
	int i = 0;
	while (char_array[i] != '\0'){
		i++ ;
	}
	return i ;
}

void check_pseudo(char pseudo[]){
	/*
	Cette fonction verifie que le pseudo est <= 30 characters et qu'il est bien ecrit
	*/
	int pseudo_len = char_array_len(pseudo);
	if (pseudo_len <= 30){
		for (int i = 0; i < pseudo_len; i ++){
			if (pseudo[i] == '[' || pseudo[i] == ']' || pseudo[i] == '-' || pseudo[i] == '/'){
				errno = 3;
				perror("The pseudo shouldn't include /, -, [, or ]");
				exit(1);
			}
			
		}
		
	}else {
		errno = 2;
		perror("The pseudo is over 30 charachters");
		exit(1);
	}
	
}

bool char_array_identical(char arr1[], char arr2[]){
	/*
	Cette fonction compare si deux arraies sont identiques 
	*/
	int i = 0;
	while (true){
		if (arr1[i] != arr2[i]){
			return false ;
		}
		if (arr1[i] == '\0' && arr2[i] == '\0'){
			break ;
		}
		i++ ;
	}
	return true ;
}

void check_parameters(char param[]){
	/*
	Cette fonction verifie que les parametres sont bien ecrits et ensuite met a jour les variables globales bool 
	sinon on termine le programme avec une erreur
	*/
	if (char_array_identical(param, "[--bot]")){
		is_bot = true ;
	}else if (char_array_identical(param, "[--manuel]")){
		is_manuel = true;
	}else {
		errno = 1;
		perror("Wrong parameters ! Use [--bot] or/and [--manuel]");
		exit(1);
	}
	
}
void init_check_prog_param(int argc, char* argv[]){
	/*
	Cette fonction va verifier si les parametres du programme sont correctement ecrit et ensuite elle va 
	les initier les variables : user, receiver, is_bot, is_manuel avec ce qu'elle a verifie. 
	*/
	// on va gerer les erreurs due aux parametres 
	if (argc >= 3 && argc < 6){
		// on a le bon nombre de parametres
		if (!char_array_identical(argv[1], "[--bot]") && !char_array_identical(argv[1], "[--manuel]") && 
		!char_array_identical(argv[2], "[--bot]") && !char_array_identical(argv[2], "[--manuel]")){
			// on a regarde s'il manquait des pseudo 
			check_pseudo(argv[1]);
			check_pseudo(argv[2]);
			// apres avoir verifie que les pseudos sont la, bien ecrit et de la bonne taille, on initie user et receiver
			strcpy(user, argv[1]);
			strcpy(receiver, argv[2]);
			
			// mtn on verifie si les parametres [--bot] et [--manuel] sont la et bien ecrit 
			if (argc > 3){
				for (int i = 3; i < argc; i++){
					check_parameters(argv[i]);
				}
			}
				
		}else {
		// une erreur 1	
			errno = 1 ;
			perror("chat pseudo_utilisateur pseudo_destinataire [--bot] [--manuel]");
			exit(1);
		}
		
	}else{
		// une erreur 1
		errno = 1 ;
		perror("chat pseudo_utilisateur pseudo_destinataire [--bot] [--manuel]");
		exit(1);
	}
}

void init_paths(){
	/*
	Cette fonction initie les chemins d'acces vers les pipes, ce sont les chemins qu'on va utiliser 
	pour lire et ecrire dans pipes
	*/
	// on construit le chemin d'ecriture
	strcat(send_path, "/tmp/");
	strcat(send_path, user);
	strcat(send_path, "-");
	strcat(send_path, receiver);
	strcat(send_path, ".chat");
	
	// on construit le chemin de lecture
	strcat(receive_path, "/tmp/");
	strcat(receive_path, receiver);
	strcat(receive_path, "-");
	strcat(receive_path, user);
	strcat(receive_path, ".chat");
	
}

int generate_random_num(int min, int max){
	return rand() % (max - min + 1) + min;
}

void end_send_process(int sig){
	unlink(send_path);
	exit(0);
}
void sig_send_process_handler (int sig){
	if (sig == SIGPIPE){
		printf("Writing to nobody !");
	}
}
