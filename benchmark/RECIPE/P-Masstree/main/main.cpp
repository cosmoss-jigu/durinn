#include<stdio.h>
#include<string.h>
#include<assert.h>

#include "masstree.h"

#define VALUE_LEN 8

#define NUM_THREADS 2

void api_begin() {};
void api_end() {};

void run_op(char **op, masstree::masstree* tree, FILE *output_file) {
  int num_or_str;
  uint64_t key;
  char* key_str;
  int range_num;
  char* value;
  uint64_t value_str;
  uint64_t* tmp;
  int ret;
  switch (op[0][0]) {
    case 'i':
      num_or_str = atoi(op[3]);

      if (num_or_str == 0) {
        key = atol(op[1]);
        value = (char*) nvm_alloc(VALUE_LEN);
        memcpy(value, op[2], strlen(op[2])+1);
        masstree::clflush(value, VALUE_LEN, true);

        api_begin();
        tree->put(key, value);
        api_end();

        fprintf(output_file, "\n");
      } else {
        assert(num_or_str == 1);
        key_str = op[1];
        value_str = atol(op[2]);

        api_begin();
        tree->put(key_str, value_str);
        api_end();

        fprintf(output_file, "\n");
      }

      break;
    case 'd':
      num_or_str = atoi(op[2]);

      if (num_or_str == 0) {
        key = atol(op[1]);

        api_begin();
        tree->del(key);
        api_end();

        fprintf(output_file, "\n");
      } else {
        assert(num_or_str == 1);
        key_str = op[1];

        api_begin();
        tree->del(key_str);
        api_end();

        fprintf(output_file, "\n");
      }

      break;
    case 'g':
      num_or_str = atoi(op[2]);

      if (num_or_str == 0) {
        key = atol(op[1]);

        api_begin();
        value = (char*) tree->get(key);
        api_end();

        fprintf(output_file, "%s\n", value);
      } else {
        assert(num_or_str == 1);
        key_str = op[1];

        api_begin();
        tmp = (uint64_t*) tree->get(key_str);
        api_end();

        if (tmp == NULL) {
          fprintf(output_file, "%s\n", NULL);
        } else {
          fprintf(output_file, "%lu\n", *tmp);
        }
      }

      break;
    case 'r':
      num_or_str = atoi(op[3]);

      if (num_or_str == 0) {
        key = atol(op[1]);
        range_num = atoi(op[2]);
        uint64_t results[range_num];
        for (int i = 0; i < range_num; i++) {
          results[i] = 0;
        }

        api_begin();
        tree->scan(key, range_num, results);
        api_end();

        for (int i = 0; i < range_num; i++) {
          if (results[i] == 0 || atol((char*)results[i]) == 0) {
            break;
          }
          fprintf(output_file, "%s ", (char*)results[i]);
        }
        fprintf(output_file, "\n");
      } else {
        assert(num_or_str == 1);

        key_str = op[1];
        range_num = atoi(op[2]);

        masstree::leafvalue *results[range_num];
        for (int i = 0; i < range_num; i++) {
          results[i] = NULL;
        }

        api_begin();
        tree->scan(key_str, range_num, results);
        api_end();

        for (int i = 0; i < range_num; i++) {
          if (results[i] == NULL) {
            break;
          }
          fprintf(output_file, "%lu ", results[i]->value);
        }
        fprintf(output_file, "\n");
      }

      break;
  }
  fflush(output_file);
}

void read_op_and_run(char *op_file_path,
                     char *output_file_path,
                     masstree::masstree* tree) {
  FILE *output_file = fopen(output_file_path, "w");
  FILE *op_file = fopen(op_file_path, "r");
  char line[256];
  while (fgets(line, sizeof line, op_file) != NULL) {
    char *op[4];
    char *p = strtok(line, ";");
    int i = 0;
    while (p != NULL && i < 4) {
      op[i++] = p;
      p = strtok (NULL, ";");
    }

    run_op(op, tree, output_file);

  }
  fclose(op_file);
  fclose(output_file);
}

struct thread_args {
  char **op;
  masstree::masstree* tree;
  FILE *output_file;
};

void *run_op_in_parallel(void* ptr) {
  struct thread_args *args = (struct thread_args *) ptr;
  run_op(args->op, args->tree, args->output_file);
  return 0;
}

void run_ops_in_parallel(char* ops[][3], masstree::masstree* tree, FILE *output_file) {

  pthread_t threads[NUM_THREADS];
  struct thread_args args[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].op = (char**) ops[i];
    args[i].tree= tree;
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
                     masstree::masstree* tree) {
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

  run_ops_in_parallel(parallel_op, tree, output_file);

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

  masstree::masstree *tree = masstree::init_P_MASSTREE(pmem_path,
                                                       pmem_size_in_mib,
                                                       layout_name);

  if (mt) {
    read_op_and_run_mt(op_file_path, output_file_path, tree);
  } else {
    read_op_and_run(op_file_path, output_file_path, tree);
  }

  return 0;
}
