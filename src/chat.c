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
#include <poll.h>

// on définit les variables et fonctions necessaires :
  
char user [30] ; // pour stocker le nom de l'utilisateur 
char receiver [30] ; // pour stocker le nom de l'interlocuteur
bool is_bot = false ; // pour le mode [--bot]
bool is_manuel = false ; // pour le mode [--manuel]

char send_path[71]; // pour le chemin du pipe "/tmp/user-receiver.chat"
char receive_path[71]; // pour le chemin du pipe "/tmp/receiver-user.chat"

int char_shared_memory_segment_status ; // smss : shared memory segment status
int int_counter_smss;

int fd_writer; //file discriptor pour le processus d'origine, il est globale parce qu'on veut l'acceder depuis la fonction
// end_send_process
int fd_reader;

char (*messages)[100]; // les pointeurs vers les segements de memeoire
int * message_counter = 0;

bool is_writer_opened = false; // pour verifier si les pipes sont ouverts 
bool is_reader_opened = false;

int char_array_len(char char_array[]); // compte la longueur d'une chaine de charachteres
bool char_array_identical(char arr1[], char arr2[]); // compare 2 chaines de charachteres
void check_pseudo(char pseudo[]); // verifie les pseudos 
void check_parameters(char param[]); // verifie les modes 
void init_check_prog_param(int argc, char* argv[]); // verifie les parametres et initie les variables : user, receiver, is_bot, is_manuel.
void init_paths(); // initie les chemins vers les pipes 
int generate_random_num(int min, int max); // genere un nombre aleatoir
void end_send_process(int ss_status, int ss_status2); // termine le processus pere 
void sig_send_process_handler (int sig); // s'occupe des signials du processus pere
void sig_receive_process_handler (int sig); // s'occupe des signials du processus fils 
void print_messages_from_sm(char (*msgs)[100], int* msg_counter); // imprime les message de sm : shared memory
void send_message(int fd, char msg[100]);


int main(int argc, char* argv[]) {
	// on check les parametres et on initie les variables 
	init_check_prog_param(argc, argv);
	init_paths();
	
	// on cree le repertoire "tmp" pour mettre les pipes dedans.
	errno = 0 ;
	int mkdir_feedback = mkdir("tmp", 0777);
	if ( mkdir_feedback == -1 && errno != EEXIST) {
    fprintf(stderr, "Directory creation failed");
	}
	
	// on cree un segement de donnees communnes
	// la cle doit etre generer de facon aleatoir sinon tous les programmes chat auront le meme espace commun
	srand(time(0));
	int key = generate_random_num(1, 1000); 
	int key2 = generate_random_num(1, 1000); 
	size_t commun_space_size = 41 * 100; // 41 est le nombre de messages et 100 est la taille d'un message
	// si on a 41 message ca veut dire qu'il y a plus de 4096 octects a afficher 
	
	char_shared_memory_segment_status = shmget(key, commun_space_size, IPC_CREAT|IPC_EXCL|0600);
	int_counter_smss = shmget(key2, sizeof(int), IPC_CREAT|IPC_EXCL|0600);
	// error handling et verifier que les cles ne coincident pas 
	// pour le char array
	if (char_shared_memory_segment_status == -1){
		if (errno != EEXIST){
			fprintf(stderr, "Failed to create commun memory"); exit(1);}
		else{// si on a par hasard la meme cle on va s'assurer qu'on a une differente cle !
			int new_key = key;
			while(new_key == key){
				new_key = generate_random_num(1, 1000);
				char_shared_memory_segment_status = shmget(new_key, commun_space_size, IPC_CREAT|IPC_EXCL|0600);
			}
		}}
	// pour le int message_counter
	if (int_counter_smss == -1){
		if (errno != EEXIST){
			fprintf(stderr, "Failed to create commun memory"); exit(1);}
		else{// si on a par hasard la meme cle on va s'assurer qu'on a une differente cle !
			int new_key = key2;
			while(new_key == key2){
				new_key = generate_random_num(1, 1000);
				int_counter_smss = shmget(key2, sizeof(int), IPC_CREAT|IPC_EXCL|0600);
			}
		}}
	// on attache 
	messages = shmat(char_shared_memory_segment_status, NULL, 0);
	message_counter = shmat(int_counter_smss, NULL, 0);
	if (messages == (void *) -1) {fprintf(stderr, "Failed to attach the shared memory segment"); exit(1);}
	if (message_counter == (void *) -1) {fprintf(stderr, "Failed to attach the shared memory segment"); exit(1);}
	
	
	// on va fork 
	pid_t p_id = fork();
	if (p_id < 0){
		fprintf(stderr, "Le fork n'a pas réussi !");
		exit(1);
	}else if(p_id > 0){
		// on cree le pipe 
		mkfifo(send_path, 0666);
		int message_size = 100;
		char message[message_size];
		
		// on gere les signals 
		signal(SIGTERM, sig_send_process_handler);
		signal(SIGPIPE, sig_send_process_handler);
		signal(SIGINT, sig_send_process_handler);
		signal(SIGHUP, sig_send_process_handler);
		
		// on ouvre le pipe pour ecrire 
			fd_writer = open(send_path, O_WRONLY);
			if (fd_writer == -1) {
			fprintf(stderr, "Failed to open the pipe");
			close(fd_writer);
			exit(1);
			}
		is_writer_opened = true;
		
		// on va verifier si le stdin contient des infos avant d'essayer de les lire, on utilise poll.h
		struct pollfd fds;

		// Configuration pour surveiller l'entrée standard
		fds.fd = STDIN_FILENO;
		fds.events = POLLIN; // On est interesse par les entrees
		
		while(true){
			// on veut verifier si le fils a termine, si oui ca veut dire que la connexion est coupee donc on termine aussi
			int child_exit_status;
            pid_t child_result = waitpid(p_id, &child_exit_status, WNOHANG); // ca ne bloque pas 

            if (child_result == p_id) {
                break;
				
            } else if (child_result != 0){
                // An error occurred
                fprintf(stderr, "waitpid failed");
                break;
            }
			
        int ret = poll(&fds, 1, 100); // timeout 100ms
		
		// si le poll echue
        if (ret == -1) {
			// on ignore les SIGINT
			if (errno != EINTR){
				fprintf(stderr, "poll failed");
				break;
			}else{continue;} 
        }
		
		// s'il n'y a rien a lire 
        if (ret == 0) {
            continue;
        }
		
		// si on a qqch a lire 
        if (fds.revents & POLLIN) {
			// on considere le cas ou l'entrée est plus grande que message_size
			while (fgets(message, message_size, stdin) != NULL) {
				// on fait la lecture
				send_message(fd_writer, message);

				// on regarde s'il y a encore des choses a lire 
				if (feof(stdin) || (char_array_len(message) < message_size - 1 || message[message_size - 2] == '\n')) {
					break;
				}
			}

			// Si jamais on EOF on termine le programme
			if (feof(stdin)) {
				break;
			} else if (ferror(stdin)) {
				fprintf(stderr, "stdin failed");
			}
		}
			
			
		}//end while
		close(fd_writer);
		
		// on suprime le pipe 
		unlink(send_path);
		// on suprime les segements de memeoire partage
		if (shmctl(char_shared_memory_segment_status, IPC_RMID, NULL) < 0) {
        fprintf(stderr, "shmctl failed");
        exit(1);
		}
		if (shmctl(int_counter_smss, IPC_RMID, NULL) < 0) {
        fprintf(stderr, "shmctl failed");
        exit(1);
		}
		
		
	}else if (p_id == 0){
		// on gere les signaux
		signal(SIGINT, sig_receive_process_handler);
		signal(SIGTERM, sig_receive_process_handler);
		signal(SIGHUP, sig_receive_process_handler); // on l'ignore psk le processus pere va se terminer, ce qui va causer la terminaison de 
		// celui-ci 
		
		int message_size = 100;
		size_t message_size_to_read = (size_t)message_size;
		char message[message_size];
		int parent_pid = getppid();
		
		// avant d'ouvrir les pipes, on verifie que le pipe existe
		while (parent_pid == getppid()){
			if (access(receive_path, F_OK) == 0) {
				break ;
			}
		}
		if (parent_pid != getppid()){
			exit(4);
		}
		
		// on ouvre le pipe pour lire
			fd_reader = open(receive_path, O_RDONLY);
			if (fd_reader == -1) {
			fprintf(stderr, "Failed to open the pipe");
			close(fd_reader);
			exit(0);
			}
			
		is_reader_opened = true;
		
		// si le pere se termine le fils va se terminer aussi
		while(parent_pid == getppid()){
			
			if (access(receive_path, F_OK) != 0) {
				// on va verifier si le pipe est toujours la 
				close(fd_reader);
				exit(0);
			}
			
			
			if ((read(fd_reader, message, message_size_to_read))== -1){
				fprintf(stderr, "Failed to read message");
			}
			// on regarde si le pere est toujours la 
			if (parent_pid == getppid()){
				if (!is_manuel){
				if (is_bot){// on gere le mode --bot
					printf("[%s] %s \n", receiver, message);
				}else{
					printf("[\x1B[4m%s\x1B[0m] %s \n", receiver, message);
				}
				}else{
					// on ecrit dans la memeoire partage
					printf("\a");
					fflush(stdout);
					(*message_counter) ++ ;
					strcpy(messages[(*message_counter) - 1], message);
				}
			}
			// si on depasse les 4096 octects dans le segement de memoire partage
			if ((*message_counter) >= 41){
				print_messages_from_sm(messages, message_counter);
			}
			fflush(stdout);
		}
		close(fd_reader);
		
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
				printf("The pseudo shouldn't include /, -, [, or ]\n");
				exit(3);
			}
			
		}
		
	}else {
		printf("The pseudo is over 30 charachters\n");
		exit(2);
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
	Cette fonction verifie que les modes sont bien ecrits et ensuite met a jour les variables globales bool 
	sinon on termine le programme avec une erreur
	*/
	if (char_array_identical(param, "--bot")){
		is_bot = true ;
	}else if (char_array_identical(param, "--manuel")){
		is_manuel = true;
	}else {
		printf("Wrong parameters ! Use [--bot] or/and [--manuel]\n");
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
		if (!char_array_identical(argv[1], "--bot") && !char_array_identical(argv[1], "--manuel") && 
		!char_array_identical(argv[2], "--bot") && !char_array_identical(argv[2], "--manuel")){
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
			fprintf(stderr, "chat pseudo_utilisateur pseudo_destinataire [--bot] [--manuel]");
			exit(1);
		}
		
	}else{
		// une erreur 1
		fprintf(stderr,"chat pseudo_utilisateur pseudo_destinataire [--bot] [--manuel]");
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
	/*
	Cette fonction retourne une valeur aleatoire entre min et max 
	*/
	return rand() % (max - min + 1) + min;
}

void end_send_process(int ss_status, int ss_status2){
	/*
	Cette fonction finit le processus pere ("writer")
	*/
	close(fd_writer);
	unlink(send_path);
	if (shmctl(ss_status, IPC_RMID, NULL) < 0) {
        fprintf(stderr, "shmctl failed");
        exit(1);
	}
	if (shmctl(ss_status2, IPC_RMID, NULL) < 0) {
        fprintf(stderr, "shmctl failed");
        exit(1);
	}
	exit(0);
}
void sig_send_process_handler (int sig){
	/*
	Cette fonction gere les signials SIGINT, SIGHUP, SIGPIPE, SIGTERM pour le processus pere ("writer")
	*/
	if (sig == SIGPIPE){
		end_send_process(char_shared_memory_segment_status, int_counter_smss);
	}else if (sig == SIGTERM){
		end_send_process(char_shared_memory_segment_status, int_counter_smss);
	}
	if (sig == SIGINT){
		if (is_writer_opened){
			if (is_manuel){
				// afficher les messages dans le segement 
				
				// on veut enlever le ^C qui apparait apres avoir appuie control c 
				
				printf("\033[1D");   // Déplacer le curseur d'une position vers la gauche
				printf("\033[1D");   // Déplacer le curseur d'une position vers la gauche
				
				print_messages_from_sm(messages, message_counter);
			}
		}else{
			unlink(send_path);
			if (shmctl(char_shared_memory_segment_status, IPC_RMID, NULL) < 0) {
				fprintf(stderr, "shmctl failed");
				exit(1);
			}
			if (shmctl(int_counter_smss, IPC_RMID, NULL) < 0) {
				fprintf(stderr, "shmctl failed");
				exit(1);
			}
			exit(4);
		}
	}
	if (sig == SIGHUP){
		end_send_process(char_shared_memory_segment_status, int_counter_smss);
	}
}

void sig_receive_process_handler (int sig){
	/*
	Cette fonction gere le signial SIGINT pour le processus fils ("reader")
	*/
	if (sig == SIGINT){
		if (!is_reader_opened){
			exit(0);
		}
	}
	if (sig == SIGTERM){
		close(fd_reader);
		exit(0);
	}
	if (sig == SIGHUP){
		close(fd_reader);
		exit(0);
	}
	
}

void print_messages_from_sm(char (*msgs)[100], int* msg_counter){
	/*
	Cette fonction va imprimer les messages dans le segment de memoire partage, elle prend des pointeurs vers 
	les segments de memeoire partage, le premier pointe vers une matrice de char de longueur de 100 charachteres et le deuxieme 
	vers un entier qui contient le nombre de messages en attente. 
	*/
	for (int i = 0; i < (*msg_counter); i++) {
		if (is_bot){// on gere le mode --bot
			printf("[%s] %s \n", receiver, msgs[i]);
		}else{
			printf("[\x1B[4m%s\x1B[0m] %s \n", receiver, msgs[i]);
		}
    }
	(*msg_counter) = 0;
}

void send_message(int fd, char message[100]){
	/*
	Cette fonction gere l'envoie de message a travers le tuyau
	*/
			// le fgets ajoute \n ; si la ligne ne fait pas completement la taille permis au message;
			// a la fin de la ligne et on doit l'enlever
			int message_len = char_array_len(message);
			size_t message_size_to_write = (size_t)(message_len + 1);
			if (message_len > 0 && message[message_len - 1] == '\n'){
				message[message_len - 1] = '\0';
			}
			
			// ceci est une cle qui peut etre recu par chat-bot lorsque la commande "au revoir" est envoye
			// nous avons fait ce traitement particulier apres avoir observer un comportement non attendue lors que 
			// nous fermons chat depuis chat-bot avec kill -SIGTERM "$chat_pid". Le programme se fermait mais le chat de 
			// l'utilisateur ne detectait pas la deconnexion et donc apres de longue heure de debuggage, nous contournos le 
			// problem en utilisant une cle qui vaut en decimal : 26112024
			// la probabilite que l'utilisateur l'ecrive au hazard est tres petite donc ca ne risque pas de creer des 
			// comportements inattendus.
			if (char_array_identical(message, "1100011100111000000011000")){
				
				// on termine le processus
				close(fd_writer);
				// on suprime le pipe 
				unlink(send_path);
				// on suprime les segements de memeoire partage
				if (shmctl(char_shared_memory_segment_status, IPC_RMID, NULL) < 0) {
				fprintf(stderr, "shmctl failed");
				exit(1);
				}
				if (shmctl(int_counter_smss, IPC_RMID, NULL) < 0) {
				fprintf(stderr, "shmctl failed");
				exit(1);
				}
				exit(1);
			}
			
			if (!is_bot){
			printf("[\x1B[4m%s\x1B[0m] %s \n",user, message);
			}
			
			if ((write(fd, message, message_size_to_write)) == -1){
				fprintf(stderr, "Failed to send message");
			}
			
			//si on a ecrit alors il faut afficher les messages dans le segement de memoire partage, si le mode manuel est active
			if (is_manuel){
				print_messages_from_sm(messages, message_counter);
			}
			
			fflush(stdout);
}
