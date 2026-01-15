#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "shared.h"
#include "sha256_utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <percorso_file> oppure %s -q (per interrogare la cache)\n", argv[0], argv[0]);
        exit(1);
    }

    request_t req;
    req.client_pid = getpid();

    //Gestione flag 
    if (strcmp(argv[1], "-q") == 0 || strcmp(argv[1], "--query") == 0) {
        req.type = REQ_QUERY;
        strcpy(req.file_path, "");
        printf("[CLIENT] Invio richiesta interrogazione cache...\n");
    } else {
        req.type = REQ_CALC;
        strncpy(req.file_path, argv[1], MAX_PATH - 1);
        req.file_path[MAX_PATH - 1] = '\0';
    }

    // Creazione FIFO privata
    char client_fifo_path[256];
    sprintf(client_fifo_path, "/tmp/client_%d_fifo", req.client_pid);

    if (mkfifo(client_fifo_path, 0666) == -1 && errno != EEXIST) {
        perror("Errore creazione FIFO client");
        exit(1);
    }

    //Invio richiesta
    int server_fd = open(SERVER_FIFO, O_WRONLY);
    if (server_fd == -1) {
        perror("Errore apertura server FIFO");
        unlink(client_fifo_path);
        exit(1);
    }
    write(server_fd, &req, sizeof(request_t));
    close(server_fd);

    //Acquisizione Risposta
    int client_fd = open(client_fifo_path, O_RDONLY);
    if (client_fd == -1) {
        perror("Errore apertura FIFO client lettura");
        unlink(client_fifo_path);
        exit(1);
    }

    if (req.type == REQ_CALC) {
        //caso base: vengono letti 32 byte di hash
        uint8_t hash[SHA256_DIGEST_LENGTH];
        if (read(client_fd, hash, SHA256_DIGEST_LENGTH) > 0) {
            char hash_string[65];
            hash_to_string(hash, hash_string);
            printf("%s\n", hash_string);
        }
    } else {
        //caso coda: lista 
        char buffer[4096]; //Buffer per testo
        ssize_t n;
        printf("--- CONTENUTO CACHE SERVER ---\n");
        //Rimane in lettura finché il server non chiude la connessione (EOF)
        while ((n = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
        }
        printf("------------------------------\n");
    }

    close(client_fd);
    unlink(client_fifo_path);
    return 0;
}