#include <emmintrin.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <libpmemobj.h>
#include <valgrind/pmemcheck.h>


#define LAYOUT_NAME "intro_1"
#define MAX_BUF_LEN 2

struct my_block {
  int flag;
  int data;
};

struct my_root {
  size_t pcounter;
  struct my_block array[MAX_BUF_LEN];
};
int
main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("usage: ./writer ${pmfile} \n");
    return 1;
  }

  PMEMobjpool *pop = pmemobj_create(argv[1], LAYOUT_NAME,
                                    PMEMOBJ_MIN_POOL, 0666);

  if (pop == NULL) {
    perror("pmemobj_create");
    return 1;
  }


  PMEMoid root = pmemobj_root(pop, sizeof(struct my_root));
  struct my_root *rootp = (struct my_root *)pmemobj_direct(root);

  rootp->pcounter = 0;
  pmemobj_persist(pop, &rootp->pcounter, sizeof(rootp->pcounter));

  for (size_t i=0; i<MAX_BUF_LEN; i++) {
    rootp->array[i].flag = -1;
    pmemobj_persist(pop, &rootp->array[i], sizeof(rootp->array[i]));
  }
TX_BEGIN(pop){
  for (size_t i=0; i<MAX_BUF_LEN; i++) {
    rootp->pcounter++;
    pmemobj_persist(pop, &rootp->pcounter, sizeof(rootp->pcounter));
    if (rand() % 2 == 0) {
      rootp->array[i].data = 66;
      rootp->array[i].flag = 0;
    } else {

            rootp->array[i].flag = 1;
    }
    pmemobj_persist(pop, &rootp->array[i], sizeof(rootp->array[i]));
  }
}TX_END


  return 0;
}