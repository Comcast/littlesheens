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

#include "machines.h"

char* readFile(const char *filename) {
  /* fprintf(stderr, "reading '%s'\n", filename); */
  
  char * buffer = 0;
  long length;
  FILE * f = fopen(filename, "rb");
  
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer) {
      long int n = fread(buffer, 1, length, f);
      if (n != length) {
	fprintf(stderr, "warning: readFile %s read %ld != %ld", filename, n, length);
      }
    }
    fclose(f);
  } else {
    fprintf(stderr, "couldn't read '%s'\n", filename);
    exit(1);
  }

  buffer[length-1] = 0;

  /* fprintf(stderr, "read %ld bytes from '%s'\n", length, filename); */

  return buffer;
}

/* specProvider just tries to read a spec (in JSON!) from the file
   system.  
   
   This function should be much smarter.  If the given mode is
   RESOLVE_IF_CHANGED and if the function knows that the requested
   spec hasn't changed, then the function should return NULL (per the
   API docs).

   See Makefile for an example of converting YAML to JSON. 

   This demo spec provider appends ".js" to the name.
*/
char * specProvider(void *this, const char *specname, const char *cached) {
  printf("main spec provider specname: %s\n", specname);

  char file_name[4096];
  snprintf(file_name, sizeof(file_name), "specs/%s.js", specname);

  return readFile(file_name);
  /* ToDo: Consider 'cached'? */
  
  /* ToDo: Free ... */
}

int evalFiles(int argc, char **argv) {
  int i;
  int some_rc = MACH_OKAY;
  for (i = 1; i < argc; i++) {
    char * src = readFile(argv[i]) ;
    size_t dst_limit = 16*1024;
    char * dst = (char*) malloc(dst_limit);
    int rc = mach_eval(src, dst, dst_limit);
    if (rc != MACH_OKAY) {
      printf("warning: mach_eval rc %d\n", rc);
      some_rc = rc;
    } else {
      printf("%s\n", dst);
    }
    free(src);
    free(dst);
  }
  return some_rc;
}
