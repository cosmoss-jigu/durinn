#include<stdio.h>
#include<string.h>
#include<assert.h>

#include "Tree.h"

#define BUFF_LEN 2048

#define NUM_THREADS 2

using namespace ART_ROWEX;

void api_begin() {};
void api_end() {};

void run_op(char **op, Tree* tree, ThreadInfo t, FILE *output_file) {
  uint64_t key_num;
  uint64_t key_num_min;
  uint64_t key_num_max;
  Key* key;
  Key* key_min;
  Key* key_max;
  uint64_t value_num;
  uint64_t* value;
  int ret;
  switch (op[0][0]) {
    case 'i':
      key_num = atol(op[1]);
      value_num = atol(op[2]);

      api_begin();
   	  key = key->make_leaf(key_num, sizeof(uint64_t), value_num);
	    tree->insert(key, t);
      api_end();

      fprintf(output_file, "\n");

      break;
    case 'd':
      key_num = atol(op[1]);

   	  key = key->make_leaf(key_num, sizeof(uint64_t), 0);
      api_begin();
	    tree->remove(key, t);
      api_end();

      fprintf(output_file, "\n");
      break;
    case 'g':
      key_num = atol(op[1]);

   	  key = key->make_leaf(key_num, sizeof(uint64_t), 0);
      api_begin();
	    value = (uint64_t*) tree->lookup(key, t);
      if (value != NULL) {
        value_num = *value;
      }
      api_end();

      if (value != NULL) {
        fprintf(output_file, "%lu\n", value_num);
      } else {
        fprintf(output_file, "(null)\n");
      }
      break;
    case 'r':
      key_num_min = atol(op[1]);
      key_num_max = atol(op[2]);

      Key *results[BUFF_LEN];
      for (int i = 0; i < BUFF_LEN; i++) {
        results[i] = NULL;
      }
      Key *continueKey = NULL;
      size_t resultsFound = 0;
      size_t resultsSize = BUFF_LEN;

   	  key_min = key->make_leaf(key_num_min, sizeof(uint64_t), 0);
   	  key_max = key->make_leaf(key_num_max, sizeof(uint64_t), 0);
      api_begin();
	    tree->lookupRange(key_min, key_max, continueKey, results, resultsSize, resultsFound, t);
      api_end();

      int i = 0;
      for (i = 0; i < BUFF_LEN; i++) {
        if (results[i] == NULL) {
          break;
        }
        fprintf(output_file, "%lu ", results[i]->value);
      }
      if (i == 0) {
        fprintf(output_file, "(null)\n");
      } else {
        fprintf(output_file, "\n");
      }

      break;
  }
  fflush(output_file);
}

void read_op_and_run(char *op_file_path,
                     char *output_file_path,
                     Tree* tree,
                     ThreadInfo t) {
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

    run_op(op, tree, t, output_file);
  }
  fclose(op_file);
  fclose(output_file);
}

struct thread_args {
  char **op;
  Tree* tree;
  ThreadInfo *t;
  FILE *output_file;
};

void *run_op_in_parallel(void* ptr) {
  struct thread_args *args = (struct thread_args *) ptr;
  run_op(args->op, args->tree, *args->t, args->output_file);
  return 0;
}

void run_ops_in_parallel(char* ops[][3], Tree *tree, ThreadInfo t, FILE *output_file) {

  pthread_t threads[NUM_THREADS];
  struct thread_args args[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].op = (char**) ops[i];
    args[i].tree = tree;
    args[i].t = &t;
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
                     Tree* tree,
                     ThreadInfo t) {
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

  run_ops_in_parallel(parallel_op, tree, t, output_file);

  for (int i = 0; i < NUM_THREADS; i++) {
    for (int j = 0; j < 3; j++) {
      free(parallel_op[i][j]);
    }
  }

  fclose(op_file);
  fclose(output_file);
}

void loadKey(TID tid, Key &key) {
    return ;
}

int main(int argc, char *argv[]) {
  assert(argc== 7);

  char *pmem_path = argv[1];
  size_t pmem_size_in_mib = atoi(argv[2]);
  char *layout_name = argv[3];

  char *op_file_path = argv[4];
  char *output_file_path = argv[5];

  int mt = atoi(argv[6]);

  Tree *tree = init_P_ART(pmem_path,
                          pmem_size_in_mib,
                          layout_name);
  Tree tt(loadKey); // Dummy tree to get the threadinfo.. Need to talk with Xinwei
  ThreadInfo t = tt.getThreadInfo();

  if (mt) {
    read_op_and_run_mt(op_file_path, output_file_path, tree, t);
  } else {
    read_op_and_run(op_file_path, output_file_path, tree, t);
  }

  exit(0);
  return 0;
}
