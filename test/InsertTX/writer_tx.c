//
// Created by 王珅祎 on 2021/10/20.
//

#include <stdio.h>
#include <string.h>
#include <libpmemobj.h>

#include "layout.h"

int main (int argc, char *argv[]) {
  if (argc < 3) {
    printf("usage: %s [file-name] [input-string]\n", argv[0]);
    return 1;
  }

  PMEMobjpool *pop = pmemobj_create(argv[1], LAYOUT_NAME,PMEMOBJ_MIN_POOL, 0666);

  if (pop == NULL) {
    pop = pmemobj_open(argv[1], LAYOUT_NAME);
    if (pop == NULL) {
      perror("pmemobj cannot create or read");
      return 1;
    }
  }

  if (strlen(argv[2]) >= MAX_BUF_LEN){
    printf("input string exceed max buff line %u", MAX_BUF_LEN);
    return 1;
  }

  PMEMoid root = pmemobj_root(pop, sizeof(struct my_root));
  struct my_root *rootp = (struct my_root *)pmemobj_direct(root);

  char buf[MAX_BUF_LEN];
  strcpy(buf, argv[2]);

  rootp->len = strlen(buf);
 	TX_BEGIN(pop){
   pmemobj_persist(pop, &rootp->len, sizeof (rootp->len));
  	}TX_END
  
  pmemobj_memcpy_persist(pop, rootp->buf, buf, rootp->len);

  pmemobj_close(pop);

  return 0;
}