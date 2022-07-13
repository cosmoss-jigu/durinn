#include<stdio.h>
#include<string.h>
#include<assert.h>

#include "btree.h"

#define VALUE_LEN 8

#define BUFF_LEN 2048

#define NUM_THREADS 2

void api_begin() {};
void api_end() {};

void run_op(char **op, btree *bt, FILE *output_file) {
  int64_t key;
  int64_t key_min, key_max;
  char* value;
  switch (op[0][0]) {
    case 'i':
      key = atol(op[1]);
      value = (char*) nvm_alloc(VALUE_LEN);
      memcpy(value, op[2], strlen(op[2])+1);
      clflush(value, VALUE_LEN);

      api_begin();
      bt->btree_insert(key, value);
      api_end();

      fprintf(output_file, "\n");

      break;
    case 'd':
      key = atol(op[1]);

      api_begin();
      bt->btree_delete(key);
      api_end();

      fprintf(output_file, "\n");
      break;
    case 'g':
      key = atol(op[1]);

      api_begin();
      value = bt->btree_search(key);
      api_end();

      fprintf(output_file, "%s\n", value);
      break;
    case 'r':
      key_min = atol(op[1]);
      key_max = atol(op[2]);

      unsigned long buff[BUFF_LEN];
      for (int i = 0; i < BUFF_LEN; i++) {
        buff[i] = 0;
      }

      api_begin();
      bt->btree_search_range(key_min, key_max, buff);
      api_end();

      for (int i = 0; i < BUFF_LEN; i++) {
        if (buff[i] == 0) {
          break;
        }
        fprintf(output_file, "%s ", ((char*)buff[i]));
      }
      fprintf(output_file, "\n");
      break;
  }
  fflush(output_file);
}

void read_op_and_run(char *op_file_path,
                     char *output_file_path,
                     btree *bt) {
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

    run_op(op, bt, output_file);
  }
  fclose(op_file);
  fclose(output_file);
}

struct thread_args {
  char **op;
  btree* btree;
  FILE *output_file;
};

void *run_op_in_parallel(void* ptr) {
  struct thread_args *args = (struct thread_args *) ptr;
  run_op(args->op, args->btree, args->output_file);
  return 0;
}

void run_ops_in_parallel(char* ops[][3], btree* btree, FILE *output_file) {

  pthread_t threads[NUM_THREADS];
  struct thread_args args[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].op = (char**) ops[i];
    args[i].btree = btree;
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
                        btree *bt) {
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

  run_ops_in_parallel(parallel_op, bt, output_file);

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

  btree *bt = init_FastFair(pmem_path, pmem_size_in_mib, layout_name);

  if (mt) {
    read_op_and_run_mt(op_file_path, output_file_path, bt);
  } else {
    read_op_and_run(op_file_path, output_file_path, bt);
  }

  return 0;
}
