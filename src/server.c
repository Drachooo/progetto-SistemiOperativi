#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include "../include/shared.h"
#include "../include/sha256_utils.h"

// Funzione eseguita dal Thread lavoratore
void *worker_thread(void *arg) {
    
    //Recupero la richiesta e libero memoria allocata dal main
    request_t *req = (request_t *)arg;
    request_t local_req = *req; 
    free(req); 

    //Calcola l'hash (operazione lenta)
    uint8_t hash[SHA256_DIGEST_LENGTH];

    // Se il file non esiste, digest_file ritorna -1.
    if (digest_file(local_req.file_path, hash) != 0) {
        memset(hash, 0, SHA256_DIGEST_LENGTH);
    }

    char client_fifo_path[256];
    sprintf(client_fifo_path, "/tmp/client_%d_fifo", local_req.client_pid);

    int client_fd = open(client_fifo_path, O_WRONLY);
    if (client_fd != -1) {
        write(client_fd, hash, SHA256_DIGEST_LENGTH);
        close(client_fd);
    } else {
        perror("Impossibile aprire FIFO client");
    }

    pthread_exit(NULL);
}

//ascolto continuo per il client
int main() {
    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST) {
        perror("Errore creazione Server FIFO");
        exit(1);
    }

    printf("Server avviato. In attesa su %s...\n", SERVER_FIFO);

    //FIFO aperta in lettura
    int server_fd = open(SERVER_FIFO, O_RDONLY);
    if (server_fd == -1) {
        perror("Errore apertura Server FIFO");
        exit(1);
    }
    
    //pipe aperta in scrittra cosi da avere file descriptor aperto
    int dummy_fd = open(SERVER_FIFO, O_WRONLY);

    while (1) {
        request_t *req = malloc(sizeof(request_t));
        
        // Legge una richiesta dalla FIFO
        ssize_t bytes_read = read(server_fd, req, sizeof(request_t));
        
        if (bytes_read == sizeof(request_t)) {
            printf("[SERVER] Ricevuta richiesta da PID %d per file: %s\n", 
                   req->client_pid, req->file_path);

            //Creo un thread dedicato per la richiesta
            pthread_t tid;
            if (pthread_create(&tid, NULL, worker_thread, (void *)req) != 0) {
                perror("Errore creazione thread");
                free(req);
            } else {
                pthread_detach(tid);
            }
        } else {
            free(req);
        }
    }

    close(server_fd);
    close(dummy_fd);
    unlink(SERVER_FIFO);
    return 0;
}