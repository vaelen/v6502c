/**
 * bin2woz - Convert binary file to Wozmon format
 *
 * Usage: bin2woz <start_address> <filename>
 *
 * Outputs Wozmon-compatible hex dump to stdout.
 * Start address should be in hexadecimal (e.g., D000 or 0xD000).
 * 
 * Copyright 2025 Andrew C. Young
 * LICENSE: MIT
 */

#include <stdio.h>
#include <stdlib.h>

#define BYTES_PER_LINE 8

int main(int argc, char **argv) {
    FILE *f;
    unsigned int addr;
    int c, col;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <start_address> <filename>\n", argv[0]);
        fprintf(stderr, "  start_address: hex address (e.g., D000 or 0xD000)\n");
        return 1;
    }

    if (sscanf(argv[1], "%x", &addr) != 1) {
        fprintf(stderr, "Error: Invalid start address '%s'\n", argv[1]);
        return 1;
    }

    f = fopen(argv[2], "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", argv[2]);
        return 1;
    }

    col = 0;
    while ((c = fgetc(f)) != EOF) {
        if (col == 0) {
            printf("%04X:", addr);
        }
        printf(" %02X", c);
        col++;
        addr++;
        if (col >= BYTES_PER_LINE) {
            printf("\n");
            col = 0;
        }
    }

    if (col > 0) {
        printf("\n");
    }

    fclose(f);
    return 0;
}
