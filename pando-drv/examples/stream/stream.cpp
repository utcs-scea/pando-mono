/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.  */
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <unistd.h>
#include <DrvAPI.hpp>

using namespace DrvAPI;

/* Drv stuff - CHANGEME */
DrvAPIAddress DRAM_START = 0x40000000;
uint32_t NUM_OF_FG_THREADS_PER_TILE = 16;

/* Usual Stream defs */
#ifndef STREAM_ARRAY_SIZE
#define STREAM_ARRAY_SIZE 1024
#endif

#ifndef NTIMES
#define NTIMES  1
#endif

#ifndef STREAM_TYPE
#define STREAM_TYPE double
#endif

extern void drv_STREAM_Copy();
extern void drv_STREAM_Scale(STREAM_TYPE scalar);
extern void drv_STREAM_Add();
extern void drv_STREAM_Triad(STREAM_TYPE scalar);

inline uint64_t
index_a (uint64_t j) {
  uint32_t coreid = DrvAPIThread::current()->coreId();
  return ((DRAM_START) +
          (NUM_OF_FG_THREADS_PER_TILE * coreid * STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE)) +
          (j * sizeof(STREAM_TYPE)));
}
inline uint64_t
index_b (uint64_t j) {
  uint32_t coreid = DrvAPIThread::current()->coreId();
  return ((DRAM_START + STREAM_ARRAY_SIZE*sizeof(STREAM_TYPE)) +
          (NUM_OF_FG_THREADS_PER_TILE * coreid * STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE)) +
          (j * sizeof(STREAM_TYPE)));
}
inline uint64_t
index_c (uint64_t j) {
  uint32_t coreid = DrvAPIThread::current()->coreId();
  return ((DRAM_START + 2*STREAM_ARRAY_SIZE*sizeof(STREAM_TYPE)) +
          (NUM_OF_FG_THREADS_PER_TILE * coreid * STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE)) +
          (j * sizeof(STREAM_TYPE)));
}

void
StreamInfo() {
  int elemsize;
  printf("\n");
  printf("STREAM version: SST Drv Custom \n");
  elemsize = sizeof(STREAM_TYPE);
  printf("sizeof(a[i])=%d bytes\n", elemsize);
  printf("Memory per array = %.1f MiB (= %.1f GiB).\n",
    elemsize * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024.0),
    elemsize * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024.0/1024.0));
  printf("Total memory per PH-TILE = %.1f MiB (= %.1f GiB).\n",
    (3.0 * elemsize * NUM_OF_FG_THREADS_PER_TILE) * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024.),
    (3.0 * elemsize * NUM_OF_FG_THREADS_PER_TILE) * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024./1024.));
}

/*-------------------------- MAIN ------------------------*/
int
StreamMain(int argc, char* argv[]) {
  int           n;
  ssize_t       j;
  STREAM_TYPE   scalar;
  scalar = 5.0;
  DRAM_START = DrvAPI::DrvAPIVAddress::MyL2Base().encode();

  if (DrvAPIThread::current()->coreId() == 0 &&
      DrvAPIThread::current()->threadId() == 0) {
    StreamInfo();
  }

  if (DrvAPIThread::current()->coreId() == 0 &&
      DrvAPIThread::current()->threadId() == 0) {
    printf("DrvAPI::write() to arrays\n");
    printf("Initializing a[], b[], and c[] ...might take some time...\n");
  }

  for (j=0; j<STREAM_ARRAY_SIZE; j++) {
    uint64_t tid = DrvAPIThread::current()->threadId();
    DrvAPI::write(index_a(j), 1.0); //a[j] = 1.0;
    DrvAPI::write(index_b(j), 2.0); //b[j] = 2.0;
    DrvAPI::write(index_c(j), 0.0); //c[j] = 0.0;
  }

  if (DrvAPIThread::current()->coreId() == 0 &&
    DrvAPIThread::current()->threadId() == 0) {
    printf("<===== MAIN LOOP ======> \n");
  }
  /*  --- MAIN LOOP --- */
  for (n=0; n<NTIMES; n++) {
    //drv_STREAM_Copy();
    //drv_STREAM_Scale(scalar);
    //drv_STREAM_Add();
    drv_STREAM_Triad(scalar);
  }
  if (DrvAPIThread::current()->coreId() == 0 &&
      DrvAPIThread::current()->threadId() == 0) {
    printf("Done Drving!\n");
  }
  return 0;
}
/*------------------------ MAIN END-----------------------*/


/*-------------------- STREAM ROUTINES -------------------*/
void drv_STREAM_Copy()
{
  ssize_t j;
  uint64_t tid = DrvAPIThread::current()->threadId();
  if (DrvAPIThread::current()->coreId() == 0 &&
      DrvAPIThread::current()->threadId() == 0) {
    printf("drv_STREAM_Copy() ...\n");
  }
  for (j=0; j<STREAM_ARRAY_SIZE; j++) {
    double val = DrvAPI::read<double>(index_a(j));
    DrvAPI::write(index_c(j), val); //c[j] = a[j];
  }
}

void drv_STREAM_Scale(STREAM_TYPE scalar)
{
  ssize_t j;
  uint64_t tid = DrvAPIThread::current()->threadId();
  if (DrvAPIThread::current()->coreId() == 0 &&
      DrvAPIThread::current()->threadId() == 0) {
    printf("drv_STREAM_Scale() ...\n");
  }
  for (j=0; j<STREAM_ARRAY_SIZE; j++) {
      double val = DrvAPI::read<double>(index_c(j));
      val *= scalar;
      DrvAPI::write(index_b(j), val); //b[j] = scalar*c[j];
  }
}

void drv_STREAM_Add()
{
  ssize_t j;
  uint64_t tid = DrvAPIThread::current()->threadId();
  if (DrvAPIThread::current()->coreId() == 0 &&
      DrvAPIThread::current()->threadId() == 0) {
    printf("drv_STREAM_Add() ...\n");
  }
  for (j=0; j<STREAM_ARRAY_SIZE; j++) {
      double val1, val2, val3;
      val1 = DrvAPI::read<double>(index_a(j));
      val2 = DrvAPI::read<double>(index_b(j));
      val3 = val1+ val2;
      DrvAPI::write(index_c(j), val3); //c[j] = a[j]+b[j];
  }
}

void drv_STREAM_Triad(STREAM_TYPE scalar)
{
  ssize_t j;
  uint64_t tid = DrvAPIThread::current()->threadId();
  if (DrvAPIThread::current()->coreId() == 0 &&
      DrvAPIThread::current()->threadId() == 0) {
    printf("drv_STREAM_Triad() ...\n");
  }
  for (j=0; j<STREAM_ARRAY_SIZE; j++) {
    double val1, val2, val3;
    val1 = DrvAPI::read<double>(index_b(j));
    val2 = DrvAPI::read<double>(index_c(j));
    val3 = val1+ scalar*val2;
    DrvAPI::write(index_a(j), val3); //a[j] = b[j]+scalar*c[j];
  }
}
/*------------------ STREAM ROUTINES END -----------------*/

declare_drv_api_main(StreamMain);
