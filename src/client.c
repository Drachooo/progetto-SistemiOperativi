#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/shared.h"
#include "../include/sha256_utils.h" // Per hash_to_string


//ascolta continuamente e 
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <percorso_file>\n", argv[0]);
        exit(1);
    }

    //Prepara la richiesta
    request_t req;
    req.client_pid = getpid();
    strncpy(req.file_path, argv[1], MAX_PATH - 1);
    req.file_path[MAX_PATH - 1] = '\0';

    //Crea la FIFO Privata per la risposta (/tmp/client_PID_fifo)
    char client_fifo_path[256];
    sprintf(client_fifo_path, "/tmp/client_%d_fifo", req.client_pid);

    if (mkfifo(client_fifo_path, 0666) == -1 && errno != EEXIST) {
        perror("Errore creazione FIFO client");
        exit(1);
    }

    // 3. Invia la richiesta al Server (FIFO Pubblica)
    int server_fd = open(SERVER_FIFO, O_WRONLY);
    if (server_fd == -1) {
        perror("Errore apertura server FIFO (Il server è avviato?)");
        unlink(client_fifo_path);
        exit(1);
    }
    
    write(server_fd, &req, sizeof(request_t));
    close(server_fd);

    // 4. Attende la risposta (apre la propria FIFO in lettura)
    // Il programma si blocca qui finché il thread del server non apre la FIFO in scrittura
    int client_fd = open(client_fifo_path, O_RDONLY);
    uint8_t hash[SHA256_DIGEST_LENGTH];
    
    if (read(client_fd, hash, SHA256_DIGEST_LENGTH) > 0) {
        char hash_string[65];
        hash_to_string(hash, hash_string);
        printf("%s\n", hash_string); // Stampa solo l'hash come richiesto
    } else {
        printf("Errore nella ricezione della risposta\n");
    }

    // 5. Pulizia
    close(client_fd);
    unlink(client_fifo_path); // Cancella la FIFO dal disco
    return 0;
}