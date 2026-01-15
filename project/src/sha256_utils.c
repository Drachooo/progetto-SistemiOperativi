#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include "sha256_utils.h" 

// Hashing file
int digest_file(const char *filename, uint8_t *hash) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buffer[32]; 
    
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        printf("File %s does not exist\n", filename);
        return -1;
    }

    ssize_t bR;
    do {
        // read the file in chunks of 32 
        bR = read(file, buffer, 32);
        if (bR > 0) {
            SHA256_Update(&ctx, (uint8_t *)buffer, bR);
        } else if (bR < 0) {
            printf("Read failed\n");
            close(file);
            return -1;             
        }
    } while (bR > 0);

    SHA256_Final(hash, &ctx);
    close(file);
    return 0; //Caso di successo
}

//Hashing stringa
void hash_to_string(const uint8_t *hash, char *output_buffer) {
    for(int i = 0; i < 32; i++)
        sprintf(output_buffer + (i * 2), "%02x", hash[i]);
}