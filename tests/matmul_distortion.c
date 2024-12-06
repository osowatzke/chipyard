#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include<stdint.h>

#define N 128

// Function to perform matrix-vector multiplication
void matrix_vector_multiply(float A[N][N], float v[N], float ref[N]) {
    for (int i = 0; i < N; i++) {
        ref[i] = 0;
        for (int j = 0; j < N; j++) {
            ref[i] += A[i][j] * v[j];
        }
    }
}

// Function to calculate normalized distortion
void calculate_normalized_distortion(float ref[N], float meas[N], uint64_t *mean_distortion, uint64_t *max_distortion) {
    float numerator = 0.0;
    float denominator = 0.0;
    float max_dist = 0.0;

    for (int i = 0; i < N; i++) {
        float dist = pow(fabs(ref[i] - meas[i]), 2);  // (ref[i] - meas[i])^2
        numerator += dist;
        denominator += pow(fabs(ref[i]), 2);         // (ref[i])^2
        if (dist > max_dist) {
            max_dist = dist;
        }
    }

    *mean_distortion = numerator / denominator;
    *max_distortion = max_dist;
}

int main() {
    float A[N][N], v[N], ref[N], meas[N];
    uint64_t mean_distortion, max_distortion;

    // Initialize matrix A and vector v
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (float)(rand() % 100);
        }
        v[i] = (float)(rand() % 100);
        meas[i] = (float)(rand() % 100);  // Mock hardware result
    }

    // Perform matrix-vector multiplication
    matrix_vector_multiply(A, v, ref);

    // Compute normalized distortion
    calculate_normalized_distortion(ref, meas, &mean_distortion, &max_distortion);

    // Output results
    printf("Matrix-Vector Multiplication completed.\n");
    printf("Mean Normalized Distortion: %lu\n", mean_distortion);
    printf("Max Distortion: %lu\n", max_distortion);

    return 0;
}
