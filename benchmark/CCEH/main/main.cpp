#include <iostream>
#include<stdio.h>
#include<string.h>
#include<assert.h>

#include "src/CCEH.h"

#define VALUE_LEN 8

#define CACHE_LINE_SIZE 64

#define NUM_THREADS 2

void api_begin() {};
void api_end() {};

inline void mfence()
{
  asm volatile("mfence":::"memory");
}

inline void clflush(char *data, int len)
{
  volatile char *ptr = (char *)((unsigned long)data &~(CACHE_LINE_SIZE-1));
  for(; ptr<data+len; ptr+=CACHE_LINE_SIZE){
    asm volatile("clflush %0" : "+m" (*(volatile char *)ptr));
  }
  mfence();
}

void run_op(char **op, CCEH *cceh, FILE *output_file) {
  size_t key;
  char* value;
  bool ret;
  switch (op[0][0]) {
    case 'i':
      key = atol(op[1]);
      value = (char*) nvm_alloc(VALUE_LEN);
      memcpy(value, op[2], strlen(op[2])+1);
      clflush(value, VALUE_LEN);

      api_begin();
      cceh->Insert(key, value);
      api_end();

      fprintf(output_file, "\n");
      break;
    case 'o':
      key = atol(op[1]);
      value = (char*) nvm_alloc(VALUE_LEN);
      memcpy(value, op[2], strlen(op[2])+1);
      clflush(value, VALUE_LEN);

      api_begin();
      ret = cceh->InsertOnly(key, value);
      api_end();

      fprintf(output_file, "%d\n", ret);
      break;
    case 'd':
      // place holder for validation
      // because CCEH doesn't support deletion
      fprintf(output_file, "\n");
      break;
    case 'g':
      key = atol(op[1]);

      api_begin();
      const char* rt_get = cceh->Get(key);
      api_end();

      fprintf(output_file, "%s\n", rt_get);
      break;
  }
  fflush(output_file);
}

void read_op_and_run(char *op_file_path,
                     char *output_file_path,
                     CCEH *cceh) {
  FILE *output_file = fopen(output_file_path, "w");
  FILE *op_file = fopen(op_file_path, "r");
  char line[256];
  while (fgets(line, sizeof line, op_file) != NULL) {
    char *op[3];
    char *p = strtok(line, ";");
    int i = 0;
    while (p != NULL && i < 3) {
      op[i++] = p;
      p = strtok (NULL, ";");
    }

    run_op(op, cceh, output_file);
  }
  fclose(op_file);
  fclose(output_file);
}

struct thread_args {
  char **op;
  CCEH* cceh;
  FILE *output_file;
};

void *run_op_in_parallel(void* ptr) {
  struct thread_args *args = (struct thread_args *) ptr;
  run_op(args->op, args->cceh, args->output_file);
  return 0;
}

void run_ops_in_parallel(char* ops[][3], CCEH *cceh, FILE *output_file) {

  pthread_t threads[NUM_THREADS];
  struct thread_args args[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].op = (char**) ops[i];
    args[i].cceh = cceh;
    args[i].output_file = output_file;
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, &run_op_in_parallel, (void *) &args[i]);
  }
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
}

void read_op_and_run_mt(char *op_file_path,
                     char *output_file_path,
                     CCEH *cceh) {
  FILE *output_file = fopen(output_file_path, "w");
  FILE *op_file = fopen(op_file_path, "r");
  char line[256];
  int count = 0;

  char* parallel_op[NUM_THREADS][3];
  for (int i = 0; i < NUM_THREADS; i++) {
    for (int j = 0; j < 3; j++) {
      parallel_op[i][j] = (char*) malloc(256 * sizeof(char));
    }
  }

  while (fgets(line, sizeof line, op_file) != NULL) {
    char *op[3];
    char *p = strtok(line, ";");
    int i = 0;
    while (p != NULL && i < 3) {
      op[i++] = p;
      p = strtok (NULL, ";");
    }
    for (int j = 0; j < 3; j++) {
      strcpy(parallel_op[count][j], op[j]);
    }
    count++;
  }
  assert(count == NUM_THREADS);

  run_ops_in_parallel(parallel_op, cceh, output_file);

  for (int i = 0; i < NUM_THREADS; i++) {
    for (int j = 0; j < 3; j++) {
      free(parallel_op[i][j]);
    }
  }

  fclose(op_file);
  fclose(output_file);
}

int main(int argc, char *argv[]) {
  assert(argc== 7);

  char *pmem_path = argv[1];
  size_t pmem_size_in_mib = atoi(argv[2]);
  char *layout_name = argv[3];

  char *op_file_path = argv[4];
  char *output_file_path = argv[5];

  int mt = atoi(argv[6]);

  CCEH *cceh = init_CCEH(pmem_path, pmem_size_in_mib, layout_name);

  if (mt) {
    read_op_and_run_mt(op_file_path, output_file_path, cceh);
  } else {
    read_op_and_run(op_file_path, output_file_path, cceh);
  }

  return 0;
}
