#include<stdio.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>

#include "pmdk.h"
#include "clht.h"
#include "ssmem.h"

#define BUFF_LEN 2048

#define NUM_THREADS 2

void api_begin() {};
void api_end() {};

void run_op(char **op, clht_t* hashtable, FILE *output_file) {
  uint64_t key_num;
  uint64_t value_num;
  int ret;
  int size;
  uintptr_t val;
  switch (op[0][0]) {
    case 'i':
      key_num = atol(op[1]);
      value_num = atol(op[2]);
      api_begin();
        ret = clht_put(hashtable, key_num, value_num);
      api_end();

      fprintf(output_file, "%d\n", ret);

      break;
    case 'd':
      key_num = atol(op[1]);
      api_begin();
   	  val = clht_remove(hashtable, key_num);
      api_end();

      fprintf(output_file, "%lu\n",val);
      break;
    case 'g':
      key_num = atol(op[1]);
      api_begin();
   	  val = clht_get(hashtable->ht,key_num);
      if (val != 0) {
        value_num = val;
      }
      api_end();

      if (val != 0) {
        fprintf(output_file, "%lu\n", value_num);
      } else {
        fprintf(output_file, "(null)\n");
      }
      break;
    case 'z':
      api_begin();
   	  size = clht_size(hashtable->ht);
      api_end();

      fprintf(output_file, "%d\n", size);
      break;
  }
  fflush(output_file);
}
struct thread_args {
  char **op;
  clht_t *ht;
  FILE *output_file;
};

void *run_op_in_parallel(void* ptr) {
  struct thread_args *args = (struct thread_args *) ptr;
  run_op(args->op, args->ht, args->output_file);
  return 0;
}

void run_ops_in_parallel(char* ops[][3], clht_t *hashtable, FILE *output_file) {

  pthread_t threads[NUM_THREADS];
  struct thread_args args[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].op = (char**) ops[i];
    args[i].ht = hashtable;
    args[i].output_file = output_file;
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, &run_op_in_parallel, (void *) &args[i]);
  }
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
}

void read_op_and_run(char *op_file_path,
                     char *output_file_path,
                     clht_t *hashtable) {
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

  run_ops_in_parallel(parallel_op, hashtable, output_file);

  for (int i = 0; i < NUM_THREADS; i++) {
    for (int j = 0; j < 3; j++) {
      free(parallel_op[i][j]);
    }
  }

  fclose(op_file);
  fclose(output_file);
}

int main(int argc, char *argv[]) {
  assert(argc== 6);

  char *pmem_path = argv[1];
  size_t pmem_size_in_mib = atoi(argv[2]);
  char *layout_name = argv[3];

  char *op_file_path = argv[4];
  char *output_file_path = argv[5];

  clht_t *hashtable = init_clht(pmem_path,
                          pmem_size_in_mib,
                          layout_name);

  read_op_and_run(op_file_path,
                  output_file_path,
                  hashtable);

  return 0;
}
