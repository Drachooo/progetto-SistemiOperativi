#ifndef SHA256_UTILS_H
#define SHA256_UTILS_H

#include <stdint.h>

// Calcola l'hash SHA256 di un file.
// Ritorna 0 se successo, -1 se errore (es. file non trovato).
int digest_file(const char *filename, uint8_t *hash);

// Converte l'hash binario in stringa esadecimale (per la stampa)
void hash_to_string(const uint8_t *hash, char *output_buffer);

#endif