#include <iostream>
#include<stdio.h>
#include<string.h>
#include<assert.h>

#include "DurableQueue-ciri.h"

#define NUM_THREADS 2

void api_begin() {};
void api_end() {};

void run_op(char **op, int tid, DurableQueue<int>* q, FILE *output_file) {
  int value;
  switch (op[0][0]) {
    case 'E':
      value = atol(op[1]);

      api_begin();
      q->enq(value);
      api_end();

      fprintf(output_file, "\n");
      break;
    case 'D':
      api_begin();
      value = q->deq(tid);
      api_end();

      fprintf(output_file, "%d\n", value);
      break;
  }
  fflush(output_file);
}

void read_op_and_run(char *op_file_path,
                     char *output_file_path,
                     DurableQueue<int>* q) {
  FILE *output_file = fopen(output_file_path, "w");
  FILE *op_file = fopen(op_file_path, "r");
  char line[256];
  while (fgets(line, sizeof line, op_file) != NULL) {
    char *op[2];
    char *p = strtok(line, ";");
    int i = 0;
    while (p != NULL && i < 2) {
      op[i++] = p;
      p = strtok (NULL, ";");
    }

    run_op(op, 0, q, output_file);
  }
  fclose(op_file);
  fclose(output_file);
}

struct thread_args {
  char **op;
  int tid;
  DurableQueue<int>* q;
  FILE *output_file;
};

void *run_op_in_parallel(void* ptr) {
  struct thread_args *args = (struct thread_args *) ptr;
  run_op(args->op, args->tid, args->q, args->output_file);
  return 0;
}

void run_ops_in_parallel(char* ops[][2], DurableQueue<int>* q, FILE *output_file) {

  pthread_t threads[NUM_THREADS];
  struct thread_args args[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].op = (char**) ops[i];
    args[i].tid = 0;
    args[i].q = q;
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
                     DurableQueue<int>* q) {
  FILE *output_file = fopen(output_file_path, "w");
  FILE *op_file = fopen(op_file_path, "r");
  char line[256];
  int count = 0;

  char* parallel_op[NUM_THREADS][2];
  for (int i = 0; i < NUM_THREADS; i++) {
    for (int j = 0; j < 2; j++) {
      parallel_op[i][j] = (char*) malloc(256 * sizeof(char));
    }
  }

  while (fgets(line, sizeof line, op_file) != NULL) {
    char *op[2];
    char *p = strtok(line, ";");
    int i = 0;
    while (p != NULL && i < 2) {
      op[i++] = p;
      p = strtok (NULL, ";");
    }
    for (int j = 0; j < 2; j++) {
      strcpy(parallel_op[count][j], op[j]);
    }
    count++;
  }
  assert(count == NUM_THREADS);

  run_ops_in_parallel(parallel_op, q, output_file);

  for (int i = 0; i < NUM_THREADS; i++) {
    for (int j = 0; j < 2; j++) {
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

  DurableQueue<int>* q = init_queue(pmem_path, pmem_size_in_mib, layout_name);

  if (mt) {
    read_op_and_run_mt(op_file_path, output_file_path, q);
  } else {
    read_op_and_run(op_file_path, output_file_path, q);
  }

  return 0;
}
