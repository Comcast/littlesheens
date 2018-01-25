/* Little process to read messages from stdin and write things to
   stdout.  Expects a crew at 'crew.js'. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "machines.h"

char* readFile(const char *filename) {
  fprintf(stderr, "readFile '%s'\n", filename);
  
  char * buffer = 0;
  long length;
  FILE * f = fopen(filename, "rb");
  
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  } else {
    fprintf(stderr, "couldn't read '%s'\n", filename);
    exit(1);
  }

  buffer[length-1] = 0;

  return buffer;
}

char * specProvider(void *this, const char *specname, int mode) {
  char filename[4096];
  snprintf(filename, sizeof(filename), "specs/%s", specname);
  return readFile(filename);
  /* ToDo: Free ... */
}

int main(int argc, char **argv) {

  int rc = mach_open();
  if (rc != MACH_OKAY) {
    printf("mach_open error %d\n", rc);
    exit(rc);
  }

  mach_set_spec_provider(NULL, specProvider);

  char *crew  = readFile("crew.js");

  {
    int line_limit = 16*1024;
    char *line = malloc(line_limit);
    size_t dst_limit = 16*1024;
    char * steppeds = (char*) malloc(dst_limit);
    char * dst = (char*) malloc(dst_limit);

    char * emitted[16];
    int i;
    for (i = 0; i < 16; i++) {
      emitted[i] = (char*) malloc(dst_limit);
    }


    while (1) {
      char *s = fgets(line, line_limit, stdin);
      if (s == NULL) {
	break;
      }
      printf("in\t%s", line); /* Already has newline. */

      rc = mach_crew_process(crew, line, steppeds, dst_limit);
      if (rc == MACH_OKAY) {
	printf("steps\t%s\n", steppeds);
      } else {
	printf("mach_crew_process error %d\n", rc);
	exit(rc);
      }
      
      rc = mach_get_emitted(steppeds, emitted, 16, dst_limit);
      if (rc == MACH_OKAY) {
	for (i = 0; i < 16; i++) {
	  JSON msg = emitted[i];
	  if (msg[0]) {
	    printf("out\t%s\n", emitted[i]);
	  }
	}
      } else {
	printf("emitted error %d\n", rc);
	exit(rc);
      }

      rc = mach_crew_update(crew, steppeds, dst, dst_limit);
      if (rc == MACH_OKAY) {
	printf("updated\t%s\n", dst);
      } else {
	  printf("update error %d\n", rc);
	  exit(rc);
      }
      strcpy(crew, dst);
    }

    free(line);
    free(steppeds);
    free(dst);
    for (i = 0; i < 16; i++) {
      free(emitted[i]);
    }
    free(crew);
  }


  mach_close();
}
