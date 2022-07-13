#include<assert.h>

#include "clht.h"

int main(int argc, char *argv[]) {
  assert(argc == 3);

  char *pmem_path = argv[1];
  char *layout_name = argv[2];

  clht_t *hashtable = init_clht(pmem_path, 0, layout_name);
  clht_print(hashtable->ht);
  printf("table size = %d\n", clht_size(hashtable->ht));
  return 0;
}
