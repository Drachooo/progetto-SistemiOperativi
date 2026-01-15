#ifndef SHARED_H
#define SHARED_H

#include <sys/types.h>

//FIFO pubblica e percorso file
#define SERVER_FIFO "/tmp/sha256_server_fifo"
#define MAX_PATH 256
#define SHA256_DIGEST_LENGTH 32

//Tipo di richiesta possibile
#define REQ_CALC  0  //Calcola Hash
#define REQ_QUERY 1  //contenuto della Cache 

// Struttura del messaggio di richiesta (Client -> Server)
typedef struct {
    pid_t client_pid;          //PID del client
    char file_path[MAX_PATH];  //file del hash
    int type;                  //TIpo di comando
} request_t;

#endif