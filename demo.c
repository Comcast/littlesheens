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

void checkrc(int rc) {
  if (rc != MACH_OKAY) {
    printf("non-zero rc %d\n", rc);
    exit(rc);
  }
}

void rcprintf(int rc, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  checkrc(rc);
}

int printer(JSON js) {
  printf("emitting %s\n", js);
  return 0;
}

JSON getMessage(int n, int dst_limit) {
    char * msg = (char *) malloc(dst_limit);
    
    //char * tmp = "{\"double\": %d}";
    char * tmp = "{\"message\":{\"op\":\"process\",\"status\":200,\"payload\":{\"double\": %d}}}";
    
    snprintf(msg, dst_limit, tmp, n);
    return msg;
}

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

  rc = mach_enable_spec_cache(1);
  if (rc != MACH_OKAY) {
    printf("mach_enable_spec_cache error %d\n", rc);
    exit(rc);
  }

  mach_set_lib_provider(NULL, libProvider, MACH_FREE_FOR_PROVIDER);
  rc = mach_set_lib_cache_limit(32);
  if (rc != MACH_OKAY) {
    printf("warning: failed to set lib cache size\n");
  }

  rc = mach_enable_lib_cache(1);
  if (rc != MACH_OKAY) {
    printf("mach_enable_lib_cache error %d\n", rc);
    exit(rc);
  }

  rc = evalFiles(argc, argv);
  if (MACH_OKAY != rc) {
    exit(rc);
  }
  
  /* Most of functions in the Machines library provide output by
     writing JSON to a char*.  We allocate that destination here. */
  size_t dst_limit = 128*1024;
  char *dst = (char*) malloc(dst_limit);
  
  /* A quick utility test. The API function mach_eval is only a
     utility function to execute arbitrary Javascript.  You probably
     won't use this function in production code. 
     
     Note that we return a JSON representation of what we
     compute. */
  rc = mach_eval("JSON.stringify(1+2)", dst, dst_limit);
  rcprintf(rc, "eval %s\n", dst);
  
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
  rcprintf(rc, "process %s\n", dst);
  
  /* The library function mach_match is another utility function
     that probably would not be used in production.  This function
     invokes pattern matching.  The first argument is a JSON
     representation of a pattern, and the second argument is a JSON
     representation of a message.  The result is JSON representing
     zero or more bindings. */
  
  rc = mach_match("{\"wants\":\"?wants\"}", "{\"wants\":\"tacos\"}", "{}", dst, dst_limit);
  rcprintf(rc, "match %s\n", dst);
  
  /* Make a crew object.  A crew is a set of machines. */
  char * crew = (char*) malloc(dst_limit);
  rc = mach_make_crew("simpsons", crew, dst_limit);
  if (rc == MACH_OKAY) {
    printf("crew %s\n", crew);
  } else {
    rcprintf(rc, "make_crew\n");
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
  
  char * steppeds = (char*) malloc(dst_limit);

  rc = mach_set_machine(crew, "hal", "double", "{\"count\":0}", "start", steppeds, dst_limit);
  if (rc == MACH_OKAY) {
    printf("added %s\n", steppeds);
  } else {
    rcprintf(rc, "set_machine\n");
  }

  rc = mach_do_emitted(steppeds, printer);
  printf("do emitted: %d\n", rc);

  rc = mach_crew_update(crew, steppeds, dst, dst_limit);
  if (rc == MACH_OKAY) {
    printf("updated %s\n", dst);
  } else {
    printf("rc %d\n", rc);
  }
  strcpy(crew, dst);
  
  /* Remove that machine. */
  rc = mach_rem_machine(crew, "hal", steppeds, dst_limit);
  if (rc == MACH_OKAY) {
    printf("removed %s\n", steppeds);
  } else {
    rcprintf(rc, "rem_machine\n");
  }

  rc = mach_crew_update(crew, steppeds, dst, dst_limit);
  if (rc == MACH_OKAY) {
    printf("updated %s\n", dst);
  } else {
    printf("rc %d\n", rc);
  }
  strcpy(crew, dst);
  
  /* Add another machine to the crew. */
  rc = mach_set_machine(crew, "doubler", "double", "{\"count\":0}", "start", steppeds, dst_limit);
  if (rc == MACH_OKAY) {
    printf("added %s\n", steppeds);
  } else {
    printf("rc %d\n", rc);
  }

  rc = mach_crew_update(crew, steppeds, dst, dst_limit);
  if (rc == MACH_OKAY) {
    printf("updated %s\n", dst);
  } else {
    printf("rc %d\n", rc);
  }
  strcpy(crew, dst);

  JSON msg = getMessage(10, dst_limit);
  
    /* Process a message. */
  rc = mach_crew_process(crew, msg, steppeds, dst_limit);
  if (rc == MACH_OKAY) {
    printf("processed %s\n", steppeds);
  } else {
    printf("rc %d\n", rc);
  }

  free(msg);
  
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
    
    for (i = 0; i < iterations; i++) {
      JSON msg = getMessage(100*i, dst_limit);
      rc = mach_crew_process(crew, msg, steppeds, dst_limit);
      if (rc == MACH_OKAY) {
	printf("%d processed %s\n", i, steppeds);
      } else {
	printf("%d processed error rc %d\n", i, rc);
      }
      free(msg);
      
      /* Show the messages we generated. */
      if ((rc = mach_do_emitted(steppeds, printer)) == MACH_OKAY) {
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
    rcprintf(rc, "mach_eval\n");
  }

  free(dst);
  
  free(crew);

  mach_close();
  
  free(mach_get_ctx());
}
