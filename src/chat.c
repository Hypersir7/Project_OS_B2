#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_PSEUDO_LEN 30
#define PIPE_NAME_SIZE 50
#define MAX_MESSAGE_SIZE 1024

int verify_length(const char* pseudonyme) {
    int pseudo_length = strlen(pseudonyme);
    if (pseudo_length > MAX_PSEUDO_LEN){
        return 0; // Pseudonyme trop long --> return 0 
    }else{
        return pseudo_length; // Longueur valide 
    } 
}

bool verify_chars(const char* pseudonyme) {
    bool forbidden_char_found = false;
    int idx = 0;
    while (pseudonyme[idx] != '\0') {
        if (pseudonyme[idx] == '/' || pseudonyme[idx] == '[' || pseudonyme[idx] == ']' ||
            pseudonyme[idx] == '-' || pseudonyme[idx] == '.') {
            forbidden_char_found = true;
            break;
        }
        idx++;
    }
    return !forbidden_char_found; // Si un char interdit est trouve, retourne false
}

int is_valide_pseudonyme(const char* pseudonyme) {
    int pseudo_length = verify_length(pseudonyme);
    if (pseudo_length == 0) { // si pseudo_length == 0 donc on est ici voir ligne 20-21
        fprintf(stderr, "[ERROR] : Le pseudonyme trop long, il faut un maximum de %d caractères!\n", MAX_PSEUDO_LEN);
        return 2; // char Trop long > 0
    }
    if (!verify_chars(pseudonyme)) {
        fprintf(stderr, "[ERROR] : Le pseudonyme contient des caractères invalides.\n");
        fprintf(stderr, "|-------  Caractères interdits : / [ ] - .\n");
        return 3; // Caracteres interdits
    }
    return 1; // Pseudonyme valide
}

/*
La fonction handle_chat_session() n'est pas vraiment complete!
    il manque la logique pour:
        - le mode --bot
        - le mode --manual
        - la gestion des signaux
*/

void handle_chat_session(const char *pseudo_sender, const char *pseudo_receiver, int is_bot_on ,int is_manual_on ){

    char message[MAX_MESSAGE_SIZE];
    char sender_pipe[PIPE_NAME_SIZE], receiver_pipe[PIPE_NAME_SIZE];
    int sender_pipe_fd, receiver_pipe_fd;
    snprintf(sender_pipe, PIPE_NAME_SIZE, "/tmp/%s-%s.chat", pseudo_sender, pseudo_receiver);
    snprintf(receiver_pipe, PIPE_NAME_SIZE, "/tmp/%s-%s.chat", pseudo_receiver, pseudo_sender);

    // creation des fifos --> named pipes avec une communication bidirectionelle!
    int sender_fifo_status_code = mkfifo(sender_pipe, 0666); // permission reade/write
    int receiver_fifo_status_code = mkfifo(receiver_pipe, 0666); // permission reade/write

    if(sender_fifo_status_code == -1){
        if(errno == EEXIST){
            printf("[INFO] : le pipe d'envoi %s existe déja dans le file System !.\n\n", sender_pipe);  
        }else{
            perror("[ERROR] : Impossible de créer le pipe d'envoi\n");
            exit(1);
        }
        
    }
     if(receiver_fifo_status_code == -1){
        if(errno == EEXIST){
            printf("[INFO] : le pipe de réception %s existe déja dans le file System!.\n\n", receiver_pipe);  
        }else{
            perror("[ERROR] : Impossible de créer le pipe de réception\n");
            exit(1); 
        }
    }
    

    pid_t pid = fork();
    if(pid < 0){
        fprintf(stderr, "[ERROR] : Le fork a echoué.\n");
        exit(1);
    }else if(pid == 0){ // processus enfant va lire les message reçus
        receiver_pipe_fd = open(receiver_pipe,O_RDONLY);
        if(receiver_pipe_fd == -1){ // si renvoie -1 --> erreur
            perror("[ERROR] : Impossible d'ouvrir le pipe de réception pour la lecture les messages \n");
            exit(1);
        }
        while(1){
            ssize_t bytes_read = read(receiver_pipe_fd, message, MAX_MESSAGE_SIZE);
            // verification si il y a  des erreurs lors de la leture!
            if(bytes_read == -1){
                perror("[ERROR] : Erreur de lecture sur le pipe de réception");
                close(receiver_pipe_fd);
                exit(1);
            }
            if(bytes_read <= 0){
                break;
            }
            if(!is_bot_on && !is_manual_on){ // par defaut on souligne et [colorie en rouge!]
                printf("[\x1B[31m\x1B[4m%s\x1B[0m] %s\n", pseudo_receiver, message);
            }
        }
        close(receiver_pipe_fd); // on est sorti de la boucle
        exit(0); // exit reussie!

    }else{// processus parent va lire les message envoyés
        sender_pipe_fd = open(sender_pipe, O_WRONLY);
        if(sender_pipe_fd == -1){
            perror("[ERROR] : Impossible d'ouvrir le pipe d'envoi pour l'écriture des messages \n.");
            exit(1);
        }
        while(1){
            ssize_t bytes_read = read(STDIN_FILENO, message, MAX_MESSAGE_SIZE);
            if(bytes_read == -1){
                perror("[ERROR] : Erreur de lecture sur le pipe de réception");
                close(sender_pipe_fd);
                exit(1);
            }
            if(bytes_read <= 0){
                break;
            }
            message[bytes_read] = '\0';
            write(sender_pipe_fd, message, bytes_read);
        }
        close(sender_pipe_fd);
        wait(NULL); // attendre la fin de l'execution du processus fils!

        unlink(sender_pipe);
        unlink(receiver_pipe);
    }
}


/*
    UTILISATION du fichier chat.c
    1) compiler: gcc -Wall -Wextra chat.c -o chat
    2) ./chat <pseudo1> <pseudo2> dans une instance du terminal
    3) ./chat <pseudo2> <pseudo1> dans une instance dupliquee du terminal 
    4) exemple: ./chat Alex Wale
    5) communication
    
    // testez et si y a des problems signalez le sur GitHub ou Discord
    // il manque l'ortographe, les accents dans quelques phrases, etc.. 
    // j'utlise le calvier anglais donc je mets pas les accents mtn
    // je vais tout bien redocumenter apres...
*/

int main(int argc, char *argv[]){
    if(argc < 3 || argc > 4){
        fprintf(stderr, "Mode d'usage: ./chat <pseudo_sender> <pseudo_receiver> [--bot] [--manuel]\n");
        return 1;
    }
    const char *pseudo_sender = argv[1]; // le argv[0] est le nom du programm meme --> ./chat
    const char *pseudo_receiver = argv[2];
    int is_bot_on = 0, is_manual_on = 0;

    if (is_valide_pseudonyme(pseudo_sender) != 1 || is_valide_pseudonyme(pseudo_receiver) != 1) {
        fprintf(stderr, "[SUCESS] : Les pseudos sont bien invalides;)\n");
        return 2;
    }
    for(int i =0; i < 3 ; i ++){
        if(strcmp(argv[i], "--bot") == 0){
            is_bot_on = 1;
        }else if(strcmp(argv[i], "--manual") == 0){
            is_manual_on = 1;
        }
        /*
        ici il manque ainsi de faire le traitement pour l'option '--bot' et '--manual'
        */
    }

    handle_chat_session(pseudo_sender, pseudo_receiver, is_bot_on, is_manual_on);

    return 0;
}

