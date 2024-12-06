#include "mmio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <riscv-pk/encoding.h>

#define DATA_ADDR 0x4000
#define DATA_COLS 0x4008
#define DATA_ROWS 0x400C
#define FILT_ADDR 0x4010
#define FILT_COLS 0x4018
#define FILT_ROWS 0x401C
#define DEST_ADDR 0x4020
#define START     0x4028
#define BUSY      0x402C

uint64_t run_hw_accelerator(float* data, int dataCols, int dataRows,
                            float* filt, int filtCols, int filtRows, float* result) {

    // Write configuration registers
    reg_write64(DATA_ADDR, (uint64_t) data);
    reg_write32(DATA_COLS, (uint32_t) dataCols);
    reg_write32(DATA_ROWS, (uint32_t) dataRows);
    reg_write64(FILT_ADDR, (uint64_t) filt);
    reg_write32(FILT_COLS, (uint32_t) filtCols);
    reg_write32(FILT_ROWS, (uint32_t) filtRows);
    reg_write64(DEST_ADDR, (uint64_t) result);

    // Read Start Time
    uint64_t start_time = rdcycle();

    // Start Hardware Accelerator
    reg_write32(START, 1);

    // Wait for hardware accelerator to finish
    while (reg_read32(BUSY));

    // Read Stop Time
    uint64_t end_time  = rdcycle();

    // Compute Execution Time
    uint64_t exec_time = end_time - start_time;

    // Display Execution Time
    printf("HW Accelerator Execution Time = %ld Cycles\n", exec_time);

    return exec_time;
}

uint64_t rocket_2Dconvo(float* data, int dataCols, int dataRows,
                        float* filt, int filtCols, int filtRows, float* result) {

    // Compute output dimensions
    int resultCols = dataCols - filtCols + 1;
    int resultRows = dataRows - filtRows + 1;

    // Compute size of matrices
    int dataSize   = dataCols   * dataRows;
    int filtSize   = filtCols   * filtRows;
    int resultSize = resultCols * resultRows;

    // Create cacheable matrices
    float* dataCached   = malloc(dataSize   * sizeof(float));
    float* filtCached   = malloc(filtSize   * sizeof(float));
    float* resultCached = malloc(resultSize * sizeof(float));

    // Read Start Time
    uint64_t startTime = rdcycle();

    // Transfer data into cacheable matrices
    memcpy(dataCached, data, dataSize * sizeof(float));
    memcpy(filtCached, filt, filtSize * sizeof(float));

    // Perform 2-D convolution
    // Output over valid region only to match HW
    for (int i = 0; i < resultRows; i++) {
        for (int j = 0; j < resultCols; j++) {
            float sum = 0.0f;
            for (int ki = 0; ki < filtRows ; ki++) {
                for (int kj = 0; kj < filtCols; kj++) {
                    int ni = i + ki;
                    int nj = j + kj;
                    sum += dataCached[ni * dataCols + nj] * 
                           filtCached[ki * filtCols + kj];
                    // printf("sum = %d\n", (int) sum);
                }
            }
            resultCached[i * resultCols + j] = sum;
        }
    }

    // Transfer data back to main memory
    memcpy(result, resultCached, resultSize * sizeof(float));

    // Read End Time
    uint64_t endTime = rdcycle();

    // Compute Execution Time
    uint64_t execTime = endTime - startTime;

    // Display Execution Time
    printf("Rocketchip Execution Time = %ld Cycles\n", execTime);

    free(dataCached);
    free(filtCached);
    free(resultCached);

    return execTime;
}

void print_distortion(float x)
{
    if (isinf(x)) {
      if (x > 0) {
         printf("Inf dB\n");
      } else {
         printf("-Inf dB\n");
      }
    } else {
        int xScaled = (int) round(x * 100);
        printf("%d.%02d dB\n", xScaled / 100, abs(xScaled % 100));
    }
}  

void distortion_metrics(float* meas, float* ref, int size, float* meanDist, float* maxDist)
{
    // Initialize max and mean distortion
    *meanDist = 0.0f;
    *maxDist  = 0.0f;

    for (int i = 0; i < size; ++i)
    {
        // Compute distortion of element
        float err = ref[i] - meas[i];
        float dist = (err * err) / (ref[i] * ref[i]);

        // Save max distortion
        if (dist > *maxDist) {
            *maxDist = dist;
        }

        // Get mean distortion
        *meanDist += dist;
    }

    // Scale mean distortion
    *meanDist = *meanDist/size;

    // Convert distortion to dB units
    *meanDist = (float) 10*log10((double) *meanDist);
    *maxDist  = (float) 10*log10((double) *maxDist);

    // Print Distortion
    printf("Mean Distortion = ");
    print_distortion(*meanDist);

    printf("Max Distortion = ");
    print_distortion(*maxDist);
}                   

int main(void)
{
    // Specify input dimensions
    int dataRows = 32;
    int dataCols = 32;
    int filtRows = 1;
    int filtCols = 32;
  
    // Compute output dimensions
    int resultCols = dataCols - filtCols + 1;
    int resultRows = dataRows - filtRows + 1;
  
    // Compute size of matrices
    int dataSize   = dataCols   * dataRows;
    int filtSize   = filtCols   * filtRows;
    int resultSize = resultCols * resultRows;
    
    // Allocate arrays in main memory for data and filter
    volatile float* data = malloc(dataSize * sizeof(float));
    volatile float* filt = malloc(filtSize * sizeof(float));
  
    // Allocate output arrays in main memory
    volatile float* result    = malloc(resultSize * sizeof(float));
    volatile float* resultRef = malloc(resultSize * sizeof(float));
  
    // populate input matrices
    for (int i = 0; i < dataSize; ++i){
        data[i] = ((float) rand()) * ((float) pow(2.0, -32.0)); // (float) (i + 1); // ((float) rand()) * ((float) pow(2.0, -32.0));
    }
  
    for (int i = 0; i < filtSize; ++i){
        filt[i] = ((float) rand()) * ((float) pow(2.0, -32.0)); //(float) (i + 1); // ((float) rand()) * ((float) pow(2.0, -32.0));
    }  
  
    // Run HW Accelerator
    run_hw_accelerator(data, dataCols, dataRows, filt, filtCols, filtRows, result);
  
    // Run Rocket Chip
    rocket_2Dconvo(data, dataCols, dataRows, filt, filtCols, filtRows, resultRef);

    // Compute distortion metrics
    float meanDist, maxDist;
    distortion_metrics(result, resultRef, resultSize, &meanDist, &maxDist);

    // Determine if test passes
    int passFail = 0;
    if ((maxDist > -60.0f) || (meanDist > -80.0f)) {
      passFail = 1;
    }
  
    free(data);
    free(filt);
    free(result);
    free(resultRef);
  
    return passFail;
}