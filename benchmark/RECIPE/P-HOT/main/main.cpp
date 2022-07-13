#include<stdio.h>
#include<string.h>
#include<assert.h>

#include <hot/rowex/HOTRowex.hpp>
#include <idx/contenthelpers/IdentityKeyExtractor.hpp>
#include <idx/contenthelpers/OptionalValue.hpp>

#define NUM_THREADS 2

void api_begin() {};
void api_end() {};

typedef struct IntKeyVal {
    uint64_t key;
    uintptr_t value;
} IntKeyVal;

template<typename ValueType = IntKeyVal *>
class IntKeyExtractor {
    public:
    typedef uint64_t KeyType;

    inline KeyType operator()(ValueType const &value) const {
        return value->key;
    }
};

using Tree = hot::rowex::HOTRowex<IntKeyVal*, IntKeyExtractor>;

void run_op(char **op, Tree* tree, FILE *output_file) {
  uint64_t key;
  uint64_t value;
  IntKeyVal *key_val;
  idx::contenthelpers::OptionalValue<IntKeyVal*> result;
  bool ret;
  switch (op[0][0]) {
    case 'i':
      key = atol(op[1]);
      value = atol(op[2]);
      nvm_aligned_alloc((void **)&key_val, 64, sizeof(IntKeyVal));
      key_val->key = key;
      key_val->value = value;
	    hot::commons::clflush((char *)key_val, sizeof(IntKeyVal));

      api_begin();
      ret = tree->insert(key_val);
      api_end();

      fprintf(output_file, "%d\n", ret);

      break;
    case 'u':
      key = atol(op[1]);
      value = atol(op[2]);
      nvm_aligned_alloc((void **)&key_val, 64, sizeof(IntKeyVal));
      key_val->key = key;
      key_val->value = value;
	    hot::commons::clflush((char *)key_val, sizeof(IntKeyVal));

      api_begin();
      result = tree->upsert(key_val);
      api_end();

      if (result.mIsValid) {
        //fprintf(output_file, "%d %lu\n", result.mIsValid, result.mValue->value);
        fprintf(output_file, "%lu\n", result.mValue->value);
      } else {
        fprintf(output_file, "(null)\n");
      }

      break;
    case 'g':
      key = atol(op[1]);

      api_begin();
      result = tree->lookup(key);
      api_end();

      if (result.mIsValid) {
        //fprintf(output_file, "%d %lu\n", result.mIsValid, result.mValue->value);
        fprintf(output_file, "%lu\n", result.mValue->value);
      } else {
        //fprintf(output_file, "%d\n", result.mIsValid);
        fprintf(output_file, "(null)\n");
      }

      break;
    case 'r':
      key = atol(op[1]);
      size_t range_num = atol(op[2]);

      api_begin();
      result = tree->scan(key, range_num);
      api_end();

      if (result.mIsValid) {
        //fprintf(output_file, "%d %lu\n", result.mIsValid, result.mValue->value);
        fprintf(output_file, "%lu\n", result.mValue->value);
      } else {
        //fprintf(output_file, "%d\n", result.mIsValid);
        fprintf(output_file, "(null)\n");
      }

      break;
  }
  fflush(output_file);
}

void read_op_and_run(char *op_file_path,
                     char *output_file_path,
                     Tree* tree) {
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

    run_op(op, tree, output_file);
  }
  fclose(op_file);
  fclose(output_file);
}

struct thread_args {
  char **op;
  Tree* tree;
  FILE *output_file;
};

void *run_op_in_parallel(void* ptr) {
  struct thread_args *args = (struct thread_args *) ptr;
  run_op(args->op, args->tree, args->output_file);
  return 0;
}

void run_ops_in_parallel(char* ops[][3], Tree *tree, FILE *output_file) {

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
                     Tree* tree) {
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

Tree *init_P_HOT(char *path, size_t size, char *layout_name) {
  int is_created;
  root_obj *root_obj = init_nvmm_pool(path, size, layout_name, &is_created);
  if (is_created == 0) {
    printf("Reading from an existing p-hot.\n");
    Tree *tree = (Tree *)root_obj->p_hot_ptr;
    tree->recovery();
    return tree;
  }
  Tree *tree = new hot::rowex::HOTRowex<IntKeyVal *, IntKeyExtractor>;

  root_obj->p_hot_ptr= (void*)tree;
  hot::commons::clflush(reinterpret_cast <char *> (root_obj), sizeof(root_obj));
  return tree;
}

int main(int argc, char *argv[]) {
  assert(argc== 7);

  char *pmem_path = argv[1];
  size_t pmem_size_in_mib = atoi(argv[2]);
  char *layout_name = argv[3];

  char *op_file_path = argv[4];
  char *output_file_path = argv[5];

  int mt = atoi(argv[6]);

  Tree* tree = init_P_HOT(pmem_path, pmem_size_in_mib, layout_name);

  if (mt) {
    read_op_and_run_mt(op_file_path, output_file_path, tree);
  } else {
    read_op_and_run(op_file_path, output_file_path, tree);
  }

  return 0;
}
