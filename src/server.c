#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include "shared.h"
#include "sha256_utils.h"

#define NUM_THREADS 6

typedef struct pid_node {
    pid_t pid;
    struct pid_node *next;
} pid_node_t;

typedef enum { STATUS_IN_PROGRESS, STATUS_COMPLETED } cache_status_t;

typedef struct cache_entry {
    char file_path[MAX_PATH];
    uint8_t hash[SHA256_DIGEST_LENGTH];
    cache_status_t status;
    pid_node_t *waiting_pids;
    struct cache_entry *next;
} cache_entry_t;

typedef struct task {
    char file_path[MAX_PATH];
    off_t file_size;
    struct task *next;
} task_t;

cache_entry_t *cache_head = NULL;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

task_t *task_queue_head = NULL;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

//FUNZIONI DI SUPPORTO

void add_waiting_pid(cache_entry_t *entry, pid_t pid) {
    pid_node_t *new_node = malloc(sizeof(pid_node_t));
    new_node->pid = pid;
    new_node->next = entry->waiting_pids;
    entry->waiting_pids = new_node;
}

void reply_to_all_waiting(cache_entry_t *entry) {
    pid_node_t *current = entry->waiting_pids;
    while (current != NULL) {
        char client_fifo_path[256];
        sprintf(client_fifo_path, "/tmp/client_%d_fifo", current->pid);
        
        int client_fd = open(client_fifo_path, O_WRONLY);
        if (client_fd != -1) {
            write(client_fd, entry->hash, SHA256_DIGEST_LENGTH);
            close(client_fd);
            printf("[CACHE] Risposto a PID %d (Cache Hit/Coalescing)\n", current->pid);
        }
        
        pid_node_t *temp = current;
        current = current->next;
        free(temp);
    }
    entry->waiting_pids = NULL;
}

void enqueue_task_sorted(const char *path, off_t size) {
    task_t *new_task = malloc(sizeof(task_t));
    strncpy(new_task->file_path, path, MAX_PATH);
    new_task->file_size = size;
    new_task->next = NULL;

    pthread_mutex_lock(&queue_mutex);

    if (task_queue_head == NULL || task_queue_head->file_size > size) {
        new_task->next = task_queue_head;
        task_queue_head = new_task;
    } else {
        task_t *current = task_queue_head;
        while (current->next != NULL && current->next->file_size <= size) {
            current = current->next;
        }
        new_task->next = current->next;
        current->next = new_task;
    }
    
    printf("[SCHEDULER] Accodato file %s (Size: %ld bytes)\n", path, size);
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

//Funzione per la QUERY
void handle_cache_query(pid_t client_pid) {
    char client_fifo_path[256];
    sprintf(client_fifo_path, "/tmp/client_%d_fifo", client_pid);
    
    int client_fd = open(client_fifo_path, O_WRONLY);
    if (client_fd == -1) {
        perror("Errore apertura FIFO client per query");
        return;
    }

    pthread_mutex_lock(&cache_mutex);
    
    cache_entry_t *curr = cache_head;
    if (curr == NULL) {
        write(client_fd, "La cache e' vuota.\n", 19);
    } else {
        char line_buffer[512];
        char hex_hash[65];
        while (curr != NULL) {
            if (curr->status == STATUS_COMPLETED) {
                hash_to_string(curr->hash, hex_hash);
                sprintf(line_buffer, "FILE: %s | HASH: %s\n", curr->file_path, hex_hash);
            } else {
                sprintf(line_buffer, "FILE: %s | STATUS: IN_PROGRESS\n", curr->file_path);
            }
            write(client_fd, line_buffer, strlen(line_buffer));
            curr = curr->next;
        }
    }
    
    pthread_mutex_unlock(&cache_mutex);
    close(client_fd);
    printf("[MAIN] Gestita richiesta query cache per PID %d\n", client_pid);
}

//WORKER THREAD 
void *worker_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        while (task_queue_head == NULL) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        task_t *task = task_queue_head;
        task_queue_head = task->next;
        pthread_mutex_unlock(&queue_mutex);

        printf("[WORKER] Elaborazione file: %s\n", task->file_path);
        
        uint8_t temp_hash[SHA256_DIGEST_LENGTH];
        int res = digest_file(task->file_path, temp_hash);

        pthread_mutex_lock(&cache_mutex);
        cache_entry_t *curr = cache_head;
        while (curr != NULL) {
            if (strcmp(curr->file_path, task->file_path) == 0) {
                if (res == 0) {
                    memcpy(curr->hash, temp_hash, SHA256_DIGEST_LENGTH);
                    curr->status = STATUS_COMPLETED;
                } else {
                    memset(curr->hash, 0, SHA256_DIGEST_LENGTH);
                    curr->status = STATUS_COMPLETED; 
                }
                reply_to_all_waiting(curr);
                break;
            }
            curr = curr->next;
        }
        pthread_mutex_unlock(&cache_mutex);
        free(task);
    }
    return NULL;
}

//MAIN
int main() {
    pthread_t threads[NUM_THREADS];
    for(int i=0; i<NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }
    printf("[SERVER] Thread Pool avviato con %d workers.\n", NUM_THREADS);

    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST) {
        perror("Errore mkfifo");
        exit(1);
    }

    int server_fd = open(SERVER_FIFO, O_RDONLY);
    int dummy_fd = open(SERVER_FIFO, O_WRONLY);

    while (1) {
        request_t req;
        ssize_t n = read(server_fd, &req, sizeof(request_t));
        if (n != sizeof(request_t)) continue;

        //GESTIONE TIPO DI RICHIESTA
        if (req.type == REQ_QUERY) {
            handle_cache_query(req.client_pid);
            continue; // Passa alla prossima richiesta
        }

        //Qua è REQ_CALC ovvero Calcolo normale
        printf("[MAIN] Richiesta calcolo: %s (PID %d)\n", req.file_path, req.client_pid);

        pthread_mutex_lock(&cache_mutex);
        cache_entry_t *curr = cache_head;
        cache_entry_t *found = NULL;
        while (curr != NULL) {
            if (strcmp(curr->file_path, req.file_path) == 0) {
                found = curr;
                break;
            }
            curr = curr->next;
        }

        if (found) {
            if (found->status == STATUS_COMPLETED) {
                printf("[CACHE] HIT! Risposta immediata.\n");
                add_waiting_pid(found, req.client_pid);
                reply_to_all_waiting(found);
            } else {
                printf("[CACHE] COALESCING! File già in lavorazione.\n");
                add_waiting_pid(found, req.client_pid);
            }
        } else {
            cache_entry_t *new_entry = malloc(sizeof(cache_entry_t));
            strncpy(new_entry->file_path, req.file_path, MAX_PATH);
            new_entry->status = STATUS_IN_PROGRESS;
            new_entry->waiting_pids = NULL;
            add_waiting_pid(new_entry, req.client_pid);
            
            new_entry->next = cache_head;
            cache_head = new_entry;

            struct stat st;
            off_t fsize = 0;
            if (stat(req.file_path, &st) == 0) fsize = st.st_size;

            enqueue_task_sorted(req.file_path, fsize);
        }
        pthread_mutex_unlock(&cache_mutex);
    }

    close(server_fd);
    close(dummy_fd);
    unlink(SERVER_FIFO);
    return 0;
}