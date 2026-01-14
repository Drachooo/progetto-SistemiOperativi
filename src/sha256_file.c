#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <openssl/sha.h>

void digest_file(const char *filename, uint8_t * hash) {

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buffer[32];

    int file = open(filename, O_RDONLY, 0);
    if (file == -1) {
        printf("File %s does not exist\n", filename);
        exit(1);
    }

    ssize_t bR;
    do {
        // read the file in chunks of 32 characters
        bR = read(file, buffer, 32);
        if (bR > 0) {
            SHA256_Update(&ctx,(uint8_t *)buffer,bR);
        } else if (bR < 0) {
            printf("Read failed\n");
            exit(1);
        }
    } while (bR > 0);

    SHA256_Final(hash, &ctx);

    close(file);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    uint8_t hash[32];
    digest_file(argv[1], hash);

    char char_hash[65];
    for(int i = 0; i < 32; i++)
        sprintf(char_hash + (i * 2), "%02x", hash[i]);

    printf("%s\n", char_hash);

    return 0;
}