#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "machines.h"
#include "util.h"

int main(int argc, char **argv) {

  mach_set_ctx(mach_make_ctx());

  int rc = mach_open();
  if (rc != MACH_OKAY) {
    printf("mach_open error %d\n", rc);
    exit(rc);
  }

  mach_set_spec_provider(NULL, specProvider, MACH_FREE_FOR_PROVIDER);
  rc = mach_set_spec_cache_limit(32);
  if (rc != MACH_OKAY) {
    printf("warning: failed to set spec cache size\n");
  }

  rc = evalFiles(argc, argv);
  if (MACH_OKAY != rc) {
    exit(rc);
  }

  mach_close();
  
  free(mach_get_ctx());
}
