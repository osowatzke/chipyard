#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Matrix size (NxN)
#define N 64

// Function to read RISC-V cycles
static inline uint64_t read_cycles() {
    uint64_t cycles;
    asm volatile ("rdcycle %0" : "=r" (cycles));
    return cycles;
}

// Function to multiply matrix A and vector v, storing the result in vector b
void matrix_vector_multiply(volatile float A[N][N], volatile float v[N], volatile float b[N]) {
    for (int i = 0; i < N; i++) {
        b[i] = 0;
        for (int j = 0; j < N; j++) {
            b[i] += A[i][j] * v[j];
        }
    }
}

// Function to save the vector `ref` to a file
void save_to_file(const char *filename, volatile float ref[N]) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    fwrite((const void *)ref, sizeof(float), N, file);
    fclose(file);
}

int main() {
    // Declare and allocate memory for matrix A, vector v, and result vector ref as volatile
    volatile float A[N][N];
    volatile float v[N];
    volatile float ref[N];

    // Initialize matrix A and vector v with random values
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (float)(rand() % 100);
        }
        v[i] = (float)(rand() % 100);
    }

    // Start timing the matrix-vector multiplication
    uint64_t start_cycles = read_cycles();
    matrix_vector_multiply(A, v, ref);
    uint64_t end_cycles = read_cycles();

    // Save the result (ref) to a file
    save_to_file("ref.bin", ref);

    // Output the number of cycles taken
    printf("Matrix-Vector Multiplication completed.\n");
    printf("Cycles taken: %lu\n", end_cycles - start_cycles);

    // Optionally, print the resulting vector ref
    printf("Result vector (ref):\n");
    for (int i = 0; i < N; i++) {
        printf("%f ", ref[i]);
    }
    printf("\n");

    return 0;
}
