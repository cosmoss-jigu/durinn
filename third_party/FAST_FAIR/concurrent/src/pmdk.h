#ifndef _PMDK_H
#define _PMDK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <libpmemobj.h>

// 1 MiB
#define MIB_SIZE ((size_t)(1024 * 1024))

typedef struct level_hashing_root_obj
{
  void *fnf_ptr;
} root_obj;

root_obj *init_nvmm_pool(char *path,
                         size_t size,
                         char *layout_name,
                         int *is_created);
int destroy_nvmm_pool();
void *nvm_alloc(size_t size);
int nvm_aligned_alloc(void **res, size_t align, size_t len);
void nvm_free(void *addr);

#endif /* pmdk.h*/
