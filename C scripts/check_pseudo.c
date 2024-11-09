<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_PSEUDONYME_LEN 30

int verify_length(const char* pseudonyme) {
    int pseudo_length = strlen(pseudonyme);

    if (pseudo_length > MAX_PSEUDONYME_LEN) {
        return 0; // erreur si trop long > 30 bytes
    } else {
        return pseudo_length; // renvoie la longueur valide < 30 bytes
    }
}

bool verify_chars(const char* pseudonyme) {
    /*
    Fonction verifiant si le pseudonyme contient des caracters interdits!
    Caracters interdits : /, -, [, ], .
    */
    bool forbidden_char_found = false;
    int idx = 0;

    while (pseudonyme[idx] != '\0') {
        if (pseudonyme[idx] == '/' || pseudonyme[idx] == '[' || pseudonyme[idx] == ']' ||
            pseudonyme[idx] == '-' || pseudonyme[idx] == '.') {
            forbidden_char_found = true;
            break; // des qu'on a trouver un char invalide sortir et renvoyer le res!
        }
        idx++;
    }

    return !forbidden_char_found; // si un caracter interdit est trouve, return false
}

int is_valide_pseudonyme(const char* pseudonyme) {
    int pseudo_length = verify_length(pseudonyme);
    if (pseudo_length == 0) { // pseudo > 30 bytes
        fprintf(stderr, "[ERROR] : Pseudonyme trop long, il faut un maximum de %d caracters!\n", MAX_PSEUDONYME_LEN);
        return 2; // renvoie 2 si trop long selon l'enoncer
    }
    if (!verify_chars(pseudonyme)) { // verifie s'il ya des chars invalides
        fprintf(stderr, "[ERROR] : Pseudonyme contient des caracters invalides\n");
        fprintf(stderr, "|-------  Caracteres interdits : / [ - . ]\n");
        return 3; // renvoie 3 si un char interdits est trouver!
    }
    return 1; // pseudonyme valide // on a passe toutes verifications
}


int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr , "[ERROR] : usage: %s <pseudo1> <pseudo2> \n", argv[0]);
        // exemple : main alex leith
        return 1;
    }

    for(int i = 1; i < 3 ; i++){
        const char* pseudo = argv[i];
        int status_code = is_valide_pseudonyme(pseudo);
        if(status_code == 1){
            printf("[SUCCESS] : Le pseudonyme %s est valide!\n", pseudo);
        }else if(status_code == 2){
            printf("[ERROR] : Le pseudonyme %s est trop long (> 30 bytes )!\n", pseudo);
        }else if(status_code == 3){
            printf("[ERROR] : Le pseudonyme %s contient des caracters interdits!\n", pseudo);
        }
    }

    return 0;
}
=======
>>>>>>> 4dbbb8a0c0abb42675841a04b9861a44e40396d8

