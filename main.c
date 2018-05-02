#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "machines.h"

char* readFile(const char *filename) {
  fprintf(stderr, "reading '%s'\n", filename);
  
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

  fprintf(stderr, "read %ld bytes from '%s'\n", length, filename);

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
char * specProvider(void *this, const char *specname, int mode) {
  printf("main spec provider specname: %s\n", specname);

  char file_name[4096];
  snprintf(file_name, sizeof(file_name), "specs/%s.js", specname);

  return readFile(file_name);
  /* ToDo: Free ... */
}

int main(int argc, char **argv) {

  int rc = mach_open();
  if (rc != MACH_OKAY) {
    printf("mach_open error %d\n", rc);
    exit(rc);
  }

  mach_set_spec_provider(NULL, specProvider);

  int i;
  for (i = 1; i < argc; i++) {
    char * src = readFile(argv[i]) ;
    size_t dst_limit = 16*1024;
    char * dst = (char*) malloc(dst_limit);
    int rc = mach_eval(src, dst, dst_limit);
    if (rc != MACH_OKAY) {
      printf("warning: mach_eval rc %d\n", rc);
    } else {
      printf("%s\n", dst);
    }
    free(src);
    free(dst);
  }

  if (argc == 1) {
    printf("executing demo main.c code\n");

    /* Most of functions in the Machines library provide output by
       writing JSON to a char*.  We allocate that destination here. */
    size_t dst_limit = 128*1024;
    char *dst = (char*) malloc(dst_limit);

    /* Most of the Machines library functions return a int status
       code. MACH_OKAY means all appears well. */
    int rc;

    /* A quick utility test. The API function mach_eval is only a
       utility function to execute arbitrary Javascript.  You probably
       won't use this function in production code. 
       
       Note that we return a JSON representation of what we
       compute. */
    rc = mach_eval("JSON.stringify(1+2)", dst, dst_limit);
    printf("eval [%d] %s\n", rc, dst);

    /* The library function mach_process gives a message to a single
       machine.  See mach_crew_process for a function that gives a
       message to a set ("crew") of machines.

       The given spec should be resolved by the spec provider set via
       mach_set_spec_provider, and the result should be JSON that
       represents a Machines specification.

       The return value is a JSON string that represents any state
       changes and messages to be emitted. */
    rc = mach_process("{\"spec\":\"double\",\"bs\":{\"count\":0},\"node\":\"start\"}",
		      "{\"double\":10}", 
		      dst, 
		      dst_limit); 
    printf("process [%d] %s\n",rc, dst);

    /* The library function mach_match is another utility function
       that probably would not be used in production.  This function
       invokes pattern matching.  The first argument is a JSON
       representation of a pattern, and the second argument is a JSON
       representation of a message.  The result is JSON representing
       zero or more bindings. */

    rc = mach_match("{\"wants\":\"?wants\"}", "{\"wants\":\"tacos\"}", "{}", dst, dst_limit);
    printf("match [%d] %s\n", rc, dst);

    /* Make a crew object.  A crew is a set of machines. */
    char * crew = (char*) malloc(dst_limit);
    rc = mach_make_crew("simpsons", crew, dst_limit);
    if (rc == MACH_OKAY) {
      printf("crew %s\n", crew);
    } else {
      printf("rc %d\n", rc);
    }
      
    /* Add a machine to the crew.

       Args:
	 
       crew: JSON representing the target crew

       spec name: The name (just a string) for the machine's
       specification.  The spec provider should be able to resolve
       that name into a machine specification (in JSON).

       machine id: The id (just a string) for the new machine.

       initial machine bindings: In JSON.

       The result is JSON representing the updated crew.
    */

    rc = mach_set_machine(crew, "hal", "hal9000", "{}", "start", dst, dst_limit);
    if (rc == MACH_OKAY) {
      printf("added %s\n", dst);
    } else {
      printf("rc %d\n", rc);
    }
    /* Update our crew definition. */
    strcpy(crew, dst);

    /* Remove that machine. */
    rc = mach_rem_machine(crew, "hal", dst, dst_limit);
    if (rc == MACH_OKAY) {
      printf("removed %s\n", dst);
    } else {
      printf("rc %d\n", rc);
    }
    strcpy(crew, dst);

    /* Add another machine to the crew. */
    rc = mach_set_machine(crew, "doubler", "double", "{\"count\":0}", "start", dst, dst_limit);
    if (rc == MACH_OKAY) {
      printf("added %s\n", dst);
    } else {
      printf("rc %d\n", rc);
    }
    strcpy(crew, dst);

    /* Process a message. */
    char * steppeds = (char*) malloc(dst_limit);
    rc = mach_crew_process(crew, "{\"double\":10}", steppeds, dst_limit);
    if (rc == MACH_OKAY) {
      printf("processed %s\n", steppeds);
    } else {
      printf("rc %d\n", rc);
    }

    /* That stepped business includes state changes for machines
       that moved as well as any messages emitted by the machines.
       "Emitted" really means "generated".  At this point, no
       messages have actually gone anywhere.
	 
       So we'll first deal with any "emitted" messages.
    */
      
    /* Allocate some space to hold the messages we want to examine. */
    char * emitted[16];
    int i;
    for (i = 0; i < 16; i++) {
      emitted[i] = (char*) malloc(dst_limit);
    }
      
    /* Now parse the 'steppeds' data to extract the generated messages. */
    rc = mach_get_emitted(steppeds, emitted, 16, dst_limit);
    if (rc == MACH_OKAY) {
      for (i = 0; i < 16; i++) {
	JSON msg = emitted[i];
	if (msg[0]) {
	  /* Here's a message generated by a machine. We should
	     really do something with this message, but we
	     don't. */
	  printf("emitted %s\n", emitted[i]);
	}
      }
    } else {
      printf("emitted error rc %d\n", rc);
    }
      

    /* Update the crew state.
	 
       Some machines might have changed their states. If so, we'll
       update our crew to reflect these state changes.
	 
       At this point, we'd likely write out the state changes to
       someplace that's durable.
	 
	 
       Note that we could be a lot more clever.  For example, we
       could only write out net state changes.  (Also note that a
       net state change might not actually require a write if that
       last state change is identical to the state of the machine
       when message processing started.)
	 
    */
      
    rc = mach_crew_update(crew, steppeds, dst, dst_limit);
    if (rc == MACH_OKAY) {
      printf("updated %s\n", dst);
    } else {
      printf("rc %d\n", rc);
    }
    strcpy(crew, dst);
      
      
    {
      /* Let's run a bunch of messages through our sophisticated
	 'double' machine. */
	
      int iterations = 10;
      int i;

      char * msg = (char *) malloc(dst_limit);
	
      for (i = 0; i < iterations; i++) {
	snprintf(msg, dst_limit, "{\"double\": %d}", 100*i);
	rc = mach_crew_process(crew, msg, steppeds, dst_limit);
	if (rc == MACH_OKAY) {
	  printf("%d processed %s\n", i, steppeds);
	} else {
	  printf("%d processed error rc %d\n", i, rc);
	}

	/* Show the messages we generated. */
	if ((rc = mach_get_emitted(steppeds, emitted, 16, dst_limit)) == MACH_OKAY) {
	  int j;
	  for (j = 0; j < 16; j++) {
	    JSON msg = emitted[i];
	    if (msg[0]) {
	      printf("%d emitted %s\n", i, emitted[j]);
	    }
	  }
	} else {
	  printf("%d emitted error rc %d\n", i, rc);
	}
	  
	/* Update our crew state. */
	rc = mach_crew_update(crew, steppeds, dst, dst_limit);
	if (rc == MACH_OKAY) {
	  printf("%d updated %s\n", i, dst);
	} else {
	  printf("rc %d\n", rc);
	}
	strcpy(crew, dst);
      }
	
      free(msg);
    }

    for (i = 0; i < 16; i++) {
      free(emitted[i]);
    }

    free(steppeds);

    /* Let's peek into the crew to find the number of times that our
       'double' machine doubled something. */
    char * src = (char *) malloc(dst_limit);
    snprintf(src, dst_limit, "JSON.stringify((%s).machines.doubler.bs.count)", crew);
    rc = mach_eval(src, dst, dst_limit);
    free(src);
    if (rc == MACH_OKAY) {
      printf("processed %s\n", dst);
    } else {
      printf("warning: mach_eval rc %d\n", rc);
    }

    free(dst);

    free(crew);

  }


  /* Free our global runtime. */
  mach_close();
}
