#ifndef SHARED_H
#define SHARED_H

#include <sys/types.h>

//FIFO pubblica e percorso file
#define SERVER_FIFO "/tmp/sha256_server_fifo"
#define MAX_PATH 256

#define SHA256_DIGEST_LENGTH 32

// Struttura del messaggio di richiesta (Client -> Server)
typedef struct {
    pid_t client_pid;          //PID del client
    char file_path[MAX_PATH];  //file del hash
} request_t;

#endif