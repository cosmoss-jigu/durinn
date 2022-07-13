//===- Tracing.cpp - Implementation of dynamic slicing tracing runtime ----===//
//
//                     Giri: Dynamic Slicing in LLVM
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the run-time functions for tracing program execution.
// It is specifically designed for tracing events needed for performing dynamic
// slicing.
//
//===----------------------------------------------------------------------===//

#include "LayoutUtil.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <libpmemobj.h>

#ifdef DEBUG_GIRI_RUNTIME
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) do {} while (false)
#endif

#define ERROR(...) fprintf(stderr, __VA_ARGS__)

//===----------------------------------------------------------------------===//
//                           Forward declearation
//===----------------------------------------------------------------------===//
extern "C" void recordInit(const char *name);
extern "C" void locking(const char *inst_name);
extern "C" void unlocking(const char *inst_name);
extern "C" void recordLock(unsigned id, unsigned char *p);
extern "C" void recordLockBegin(unsigned id, unsigned char *p);
extern "C" void recordLockEnd(unsigned id, unsigned char *p);
extern "C" void recordUnlock(unsigned id, unsigned char *p);
extern "C" void recordUnlockBegin(unsigned id, unsigned char *p);
extern "C" void recordUnlockEnd(unsigned id, unsigned char *p);
extern "C" void recordAPIBegin(unsigned id);
extern "C" void recordAPIEnd(unsigned id);
extern "C" void recordBr(unsigned id);
extern "C" void recordLoad(unsigned id, unsigned char *p, uintptr_t);
extern "C" void recordStrLoad(unsigned id, char *p);
extern "C" void recordStore(unsigned id, unsigned char *p, uintptr_t length);
extern "C" void recordAtomicStore(unsigned id, unsigned char *p, uintptr_t length);
extern "C" void recordCAS(unsigned id, unsigned char *p, uint64_t v, uintptr_t);
extern "C" void recordStrStore(unsigned id, char *p);
extern "C" void recordStrcatStore(unsigned id, char *p, char *s);
extern "C" void recordFlush(unsigned id, unsigned char *p);
extern "C" void recordFlushWrapper(unsigned id, unsigned char *p, uintptr_t);
extern "C" void recordFence(unsigned id);
extern "C" void recordAlloc(unsigned id, unsigned char *p, uintptr_t);
extern "C" void recordTxAlloc(unsigned id, PMEMoid p, uintptr_t);
extern "C" void recordMmap(unsigned id, unsigned char *p, uintptr_t);

//===----------------------------------------------------------------------===//
//                       Basic Block and Function Stack
//===----------------------------------------------------------------------===//

/// the mutex of modifying the EntryCache
static pthread_mutex_t EntryCacheMutex;

char* no_alloc_br = getenv("NO_ALLOC_BR");
char* replay_mode = getenv("REPLAY_MOD");
std::ofstream record;

// PM start address
uintptr_t pm_addr_start = strtol(std::getenv("PMEM_MMAP_HINT"), NULL, 16);
uintptr_t pm_addr_start_dup = 0;
// PM end address
uintptr_t pm_addr_end = 0;
uintptr_t pm_addr_end_dup = 0;

//===----------------------------------------------------------------------===//
//                       Record and Helper Functions
//===----------------------------------------------------------------------===//

static bool recording = false;

void enableRecording(void) {
  DEBUG("[GIRI] Enable Recording now!\n");
  recording = true;
}

void disableRecording(void) {
  DEBUG("[GIRI] Disable Recording now!\n");
  recording = false;
}

/// helper function which is registered at atexit()
static void finish() {
  // Disable recording in case that other codes run after main
  disableRecording();

  DEBUG("[GIRI] Writing cache data to trace file and closing.\n");

  record.close();

  // destroy the mutexes
  pthread_mutex_destroy(&EntryCacheMutex);
}

/// Signal handler to write only tracing data to file
static void cleanup_only_tracing(int signum) {
  ERROR("[GIRI] Abnormal termination, signal number %d\n", signum);
  exit(signum);
}

void recordInit(const char *name) {
  // Open the file for recording the trace if it hasn't been opened already.
  // Truncate it in case this dynamic trace is shorter than the last one
  // stored in the file.
  record.open(name);

  pthread_mutex_init(&EntryCacheMutex, NULL);

  atexit(finish);

  // Register the signal handlers for flushing of diagnosis tracing data to file
  signal(SIGINT, cleanup_only_tracing);
  signal(SIGQUIT, cleanup_only_tracing);
  signal(SIGSEGV, cleanup_only_tracing);
  signal(SIGABRT, cleanup_only_tracing);
  signal(SIGTERM, cleanup_only_tracing);
  signal(SIGKILL, cleanup_only_tracing);
  signal(SIGILL, cleanup_only_tracing);
  signal(SIGFPE, cleanup_only_tracing);

  enableRecording();
}

void locking(const char *inst_name) {
  if (!recording) {
    return;
  }

  // pthread_mutex_lock(&EntryCacheMutex);
  DEBUG("[GIRI] Lock for instruction: %s\n", inst_name);
}

/// \brief Unlock the entry cache mutex.
void unlocking(const char *inst_name) {
  if (!recording) {
    return;
  }

  DEBUG("[GIRI] Release the lock for instruction: %s\n", inst_name);
  // pthread_mutex_unlock(&EntryCacheMutex);
}

void recordLock(unsigned id, unsigned char *p) {
  if (!recording) {
    return;
  }
  if (replay_mode) {
    return;
  }
  pthread_t tid = pthread_self();

  record << "Lock," << std::dec << tid << ","
                    << std::hex << (uintptr_t) p << ","
                    << std::dec<< id << std::endl;
}

void recordLockBegin(unsigned id, unsigned char *p) {
  if (!recording) {
    return;
  }
  if (replay_mode) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "LockBegin," << std::dec << tid << ","
                         << std::hex << (uintptr_t)p << ","
                         << "NA" << std::endl;
}

void recordLockEnd(unsigned id, unsigned char *p) {
  if (!recording) {
    return;
  }
  if (replay_mode) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "LockEnd," << std::dec << tid << ","
                       << std::hex << (uintptr_t)p << ","
                       << "NA" << std::endl;
}

void recordUnlock(unsigned id, unsigned char *p) {
  if (!recording) {
    return;
  }
  if (replay_mode) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "UnLock," << std::dec << tid << ","
                      << std::hex << (uintptr_t)p << ","
                      << std::dec << id << std::endl;
}

void recordUnlockBegin(unsigned id, unsigned char *p) {
  if (!recording) {
    return;
  }
  if (replay_mode) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "UnLockBegin," << std::dec << tid << ","
                           << std::hex << (uintptr_t)p << ","
                           << "NA" << std::endl;
}

void recordUnlockEnd(unsigned id, unsigned char *p) {
  if (!recording) {
    return;
  }
  if (replay_mode) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "UnLockEnd," << std::dec << tid << ","
                         << std::hex << (uintptr_t)p << ","
                         << "NA" << std::endl;
}

void recordAPIBegin(unsigned id) {
  if (!recording) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "APIBegin," << std::dec << tid << ",NA" << std::endl;
}

void recordAPIEnd(unsigned id) {
  if (!recording) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "APIEnd," << std::dec << tid << ",NA" << std::endl;
}

bool is_pm_addr(uintptr_t addr) {
  if(addr >= pm_addr_start && addr < pm_addr_end) {
    return true;
  }

  if(addr >= pm_addr_start_dup && addr < pm_addr_end_dup) {
    return true;
  }
  return false;
}

uintptr_t get_pm_addr(uintptr_t p) {
  if(p >= pm_addr_start_dup && p < pm_addr_end_dup) {
    return p - pm_addr_start_dup + pm_addr_start;
  }
  return p;
}

void recordBr(unsigned id) {
  if (no_alloc_br) {
    return;
  }

  if (!recording) {
    return;
  }

  if (replay_mode) {
    return;
  }
  pthread_t tid = pthread_self();
  record << "Br," << std::dec << tid << ","
                  << std::dec << id << std::endl;
}

/// Record that a load has been executed.
void recordLoad(unsigned id, unsigned char *p, uintptr_t length) {
  if (!recording) {
    return;
  }

  if (replay_mode) {
    return;
  }

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  // Some memcpy may use 0 length
  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Load," << std::dec << tid << ","
                    << std::hex << addr << ","
                    << std::dec << length << ","
                    << std::dec << id << std::endl;
}

/// Record that a string has been read.
void recordStrLoad(unsigned id, char *p) {
  if (!recording) {
    return;
  }

  if (replay_mode) {
    return;
  }

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  // First determine the length of the string.  Add one byte to include the
  // string terminator character.
  uintptr_t length = strlen(p) + 1;

  // Some memcpy may use 0 length
  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Load," << std::dec << tid << ","
                    << std::hex << addr << ","
                    << std::dec << length << ","
                    << std::dec << id << std::endl;
}

/// Record that a store has occurred.
/// \param id     - The ID assigned to the store instruction in the LLVM IR.
/// \param p      - The starting address of the store.
/// \param length - The length, in bytes, of the stored data.
void recordStore(unsigned id, unsigned char *p, uintptr_t length) {
  if (!recording) {
    return;
  }

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  // Some memcpy may use 0 length
  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Store," << std::dec << tid << ","
                     << std::hex << addr << ","
                     << std::dec << length << ",";

  for(uintptr_t i = 0; i < length - 1; ++i) {
    record << std::hex << std::setfill('0') << std::setw(2)
               << (int)p[i] << " ";
  }
  record << std::hex << std::setfill('0') << std::setw(2)
             << (int)p[length-1];

  record << "," << std::dec << id << ",0" << std::endl;
}

void recordAtomicStore(unsigned id, unsigned char *p, uintptr_t length) {
  if (!recording) {
    return;
  }

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  // Some memcpy may use 0 length
  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Store," << std::dec << tid << ","
                     << std::hex << addr << ","
                     << std::dec << length << ",";

  for(uintptr_t i = 0; i < length - 1; ++i) {
    record << std::hex << std::setfill('0') << std::setw(2)
               << (int)p[i] << " ";
  }
  record << std::hex << std::setfill('0') << std::setw(2)
             << (int)p[length-1];

  record << "," << std::dec << id << ",1" << std::endl;
}

void recordCAS(unsigned id, unsigned char *p, uint64_t v, uintptr_t length) {
  recordLoad(id, p, length);

  if (memcmp(p, &v, length) == 0) {
    recordAtomicStore(id, p, length);
  }
}

/// Record that a string has been written.
/// \param id - The ID of the instruction that wrote to the string.
/// \param p  - A pointer to the string.
void recordStrStore(unsigned id, char *p) {
  if (!recording) {
    return;
  }

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  // First determine the length of the string.  Add one byte to include the
  // string terminator character.
  uintptr_t length = strlen(p) + 1;

  // Some memcpy may use 0 length
  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Store," << std::dec << tid << ","
                     << std::hex << addr << ","
                     << std::dec << length << ",";

  for(uintptr_t i = 0; i < length - 1; ++i) {
    record << std::hex << std::setfill('0') << std::setw(2)
               << (int)p[i] << " ";
  }
  record << std::hex << std::setfill('0') << std::setw(2)
             << (int)p[length-1];

  record << "," << std::dec <<  id << ",0" << std::endl;
}

/// Record that a string has been written on strcat.
/// \param id - The ID of the instruction that wrote to the string.
/// \param  p  - A pointer to the string.
void recordStrcatStore(unsigned id, char *p, char *s) {
  if (!recording) {
    return;
  }

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  // Determine where the new string will be added Don't. add one byte
  // to include the string terminator character, as write will start
  // from there. Then determine the length of the written string.
  char *start = p + strlen(p);
  uintptr_t length = strlen(s) + 1;

  // Some memcpy may use 0 length
  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Store," << std::dec << tid << ","
                     << std::hex << addr << ","
                     << std::dec << length << ",";

  for(uintptr_t i = 0; i < length - 1; ++i) {
    record << std::hex << std::setfill('0') << std::setw(2)
               << (int)p[i] << " ";
  }
  record << std::hex << std::setfill('0') << std::setw(2)
             << (int)p[length-1];

  record << "," << std::dec <<  id << ",0" << std::endl;
}

/// This function records a Cacheline Flush instruction.
/// \param id - The ID assigned to the corresponding instruction in the LLVM IR
/// \param ptr - The ptr value passed from the asm call.
void recordFlush(unsigned id, unsigned char *p) {
  if (!recording) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Flush," << std::dec << tid << ","
                     << std::hex << addr << ","
                     << std::dec << id << std::endl;
}

/// This function records a Cacheline Flush instruction.
/// \param id - The ID assigned to the corresponding instruction in the LLVM IR
/// \param ptr - The ptr value passed from the call.
//  \param length - the length for flushing
void recordFlushWrapper(unsigned id, unsigned char *ptr, uintptr_t length) {
  // Get the start address of the cached line of the ptr
  unsigned char* ptr_aligned = LayoutUtil::get_aligned_addr_64(ptr);
  for(; ptr_aligned < ptr + length; ptr_aligned += LayoutUtil::CACHELINE_SIZE){
    recordFlush(id, ptr_aligned);
  }
}

/// This function records a Memory Fence instruction.
/// \param id - The ID assigned to the corresponding instruction in the LLVM IR
void recordFence(unsigned id) {
  if (!recording) {
    return;
  }

  pthread_t tid = pthread_self();
  record << "Fence," << std::dec << tid << ","
                     << std::dec << id << std::endl;
}

void recordAlloc(unsigned id, unsigned char *oidp, uintptr_t length) {
  if (no_alloc_br) {
    return;
  }

  if (!recording) {
    return;
  }

  if (replay_mode) {
    return;
  }

  void * p = pmemobj_direct(*(PMEMoid*)oidp);

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Alloc," << std::dec << tid << ","
                    << std::hex << addr << ","
                    << std::dec << length << ","
                    << std::dec << id << std::endl;
}

void recordTxAlloc(unsigned id, PMEMoid oid, uintptr_t length) {
  if (no_alloc_br) {
    return;
  }

  if (!recording) {
    return;
  }

  if (replay_mode) {
    return;
  }

  void * p = pmemobj_direct(oid);

  // check if it is inside the PM range
  if (!is_pm_addr((uintptr_t) p)) {
    return;
  }

  if (length == 0) {
    return;
  }

  uintptr_t addr = get_pm_addr((uintptr_t) p);

  pthread_t tid = pthread_self();
  record << "Alloc," << std::dec << tid << ","
                    << std::hex << addr << ","
                    << std::dec << length << ","
                    << std::dec << id << std::endl;
}

void recordMmap(unsigned id, unsigned char *ptr, uint64_t length) {
  if (!recording) {
    return;
  }

  uintptr_t addr = (uintptr_t) ptr;

  // debug
  // std::cout << std::hex << (uintptr_t) ptr << "," << std::dec << length << std::endl;

  if (addr == 18446744073709551615) {
    return;
  }

  if (addr == pm_addr_start) {
    pm_addr_end = addr + length;
    return;
  }

  assert(pm_addr_start_dup == 0 || pm_addr_start_dup == addr);
  assert(pm_addr_end_dup == 0 || pm_addr_end_dup == addr + length);

  pm_addr_start_dup = addr;
  pm_addr_end_dup = addr + length;
}
