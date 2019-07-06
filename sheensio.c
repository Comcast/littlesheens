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

/* Little process to read messages from stdin and write things to
   stdout.  Expects a crew at 'crew.json'.  See 'demo.sh' for
   an example. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h> 

#include "machines.h"
#include "util.h"

int logging = 0;

void lgf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (logging) {
    vfprintf(stderr, fmt, args);
  }

  va_end(args);
}

void eval(char * src) {
  size_t dst_limit = 16*1024;
  char * dst = (char*) malloc(dst_limit);
  int rc = mach_eval(src, dst, dst_limit);
  if (rc != MACH_OKAY) {
    printf("warning: %s rc %d\n", src, rc);
  } else {
    printf("%s\n", dst);
  }
  free(dst);
}

int main(int argc, char **argv) {

  int useSpecCache = 0;
  int profiling = 0;
  int stats = 0;
  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];
    if (strcmp(arg, "-d") == 0) {
      logging = 1;
    } else if (strcmp(arg, "-c") == 0) {
      useSpecCache = 1;
    } else if (strcmp(arg, "-p") == 0) {
      profiling = 1;
    } else if (strcmp(arg, "-s") == 0) {
      stats = 1;
    }
  }

  mach_set_ctx(mach_make_ctx());

  int rc = mach_open();
  if (rc != MACH_OKAY) {
    printf("mach_open error %d\n", rc);
    exit(rc);
  }

  if (profiling) {
    eval("Times.enable(); true;");
  }

  mach_set_spec_provider(NULL, specProvider, MACH_FREE_FOR_PROVIDER);
  if (useSpecCache) {
    rc = mach_enable_spec_cache(1);
    if (rc != MACH_OKAY) {
      printf("mach_enable_spec_cache error %d\n", rc);
      exit(rc);
    }
    rc = mach_set_spec_cache_limit(64);
    if (rc != MACH_OKAY) {
      printf("mach_set_spec_cache_limit error %d\n", rc);
      exit(rc);
    }
  }

  mach_set_lib_provider(NULL, libProvider, MACH_FREE_FOR_PROVIDER);
  if (useSpecCache) {
    rc = mach_enable_lib_cache(1);
    if (rc != MACH_OKAY) {
      printf("mach_enable_lib_cache error %d\n", rc);
      exit(rc);
    }
    rc = mach_set_lib_cache_limit(64);
    if (rc != MACH_OKAY) {
      printf("mach_set_lib_cache_limit error %d\n", rc);
      exit(rc);
    }
  }

  char *src  = readFile("crew.json");

  {
    size_t line_limit = 16*1024;
    size_t max_emitted = 128;
    char *line = malloc(line_limit);
    size_t dst_limit = 16*1024;
    char * steppeds = (char*) malloc(dst_limit);
    char * dst = (char*) malloc(dst_limit);
    
    // allocate bigger buffer for crew so it can be updated later
    char * crew = (char*) malloc(dst_limit);
    strcpy(crew, src);
    free(src);

    char * emitted[max_emitted];
    int i;
    for (i = 0; i < max_emitted; i++) {
      emitted[i] = (char*) malloc(dst_limit);
    }

    int count=0;

    while (1) {
      clock_t cp0 = clock(); 
      char *s = fgets(line, (int) line_limit, stdin);
      if (s == NULL || 0 == strcmp(s, "\n")) {
	break;
      }
      lgf("in\t%s", line); /* Already has newline. */

      clock_t cp1 = clock();
      printf("got: %f\n",((double)cp1-cp0)/CLOCKS_PER_SEC); 
    
      rc = mach_crew_process(crew, line, steppeds, dst_limit);
      clock_t cp2 = clock(); 
      printf("processed: %f\n",((double)cp2-cp1)/CLOCKS_PER_SEC); 
    
      if (rc == MACH_OKAY) {
	lgf("steps\t%s\n", steppeds);
      } else {
	printf("mach_crew_process error %d\n", rc);
	exit(rc);
      }
      
      rc = mach_get_emitted(steppeds, emitted, max_emitted, dst_limit);
      if (rc == MACH_OKAY) {
	for (i = 0; i < max_emitted; i++) {
	  JSON msg = emitted[i];
	  if (msg[0]) {
	    printf("out\t%s\n", emitted[i]);
	  }
	}
      } else {
	printf("emitted error %d\n", rc);
	exit(rc);
      }
    
      clock_t cp3 = clock(); 
      printf("emitted: %f\n",((double)cp3-cp2)/CLOCKS_PER_SEC);

      rc = mach_crew_update(crew, steppeds, dst, dst_limit);
      if (rc == MACH_OKAY) {
	lgf("updated\t%s\n", dst);
      } else {
	  printf("update error %d\n", rc);
	  exit(rc);
      }
      strcpy(crew, dst);
    
      clock_t cp4 = clock(); 
      printf("updated: %f\n",((double)cp4-cp3)/CLOCKS_PER_SEC);
    
      count++;
      printf("count: %d, %f\n", count, ((double)cp4-cp0)/CLOCKS_PER_SEC);
    }

    free(line);
    free(steppeds);
    free(dst);
    for (i = 0; i < max_emitted; i++) {
      free(emitted[i]);
    }
    free(crew);
  }

  if (stats) {
    eval("'SpecCache: ' + JSON.stringify(SpecCache.summary())");
    eval("'Stats:     ' + JSON.stringify(Stats)");
    eval("'Times:     ' + JSON.stringify(Times.summary())");
  }

  mach_close();

  free(mach_get_ctx());
}
