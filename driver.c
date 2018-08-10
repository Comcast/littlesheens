/* Copyright 2018 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
