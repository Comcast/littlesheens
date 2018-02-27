/* Sheen Core C API */

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* JSON is a string in JSON syntax. */
typedef char * JSON;

/* S is just a string (in contrast to JSON). */
typedef char * S;

/* Most of the following symbols have the prefix "mach_", which is
   short for "machines".  I thought long and hard about "meh_", which
   is phonetially short for "meh-sheens", but I obviously didn't
   follow through with that brilliant idea. */

/* mach_open creates and initializes the runtime.  (Currently there is
   only one global runtime.) */
int mach_open();

/* mach_close frees the runtime. */
void mach_close();

/* mach_process takes a machine State and a Message and returns the
   new state (if any) and any emitted messages.  (Structure currently
   subject to change ... )
   
   This function should probably evolve to return an int and write a
   given string.

   Also see mach_crew_process. */
int mach_process(JSON state, JSON message, JSON dst, int limit);

/* provider is the signature for a function that can resolve the given
   SpecName to Spec.  The first argument will be _ctx.  The second
   argument is a string that can be resolved to a spec.  The third
   argument is the resolution mode, which should be either RESOLVE or
   RESOLVE_IF_CHANGED.  The latter means that the function should
   return NULL if the Spec hasn't changed.  Otherwise the function
   should return the requested Spec. */
typedef char * (*provider)(void*, const char *, int);

#define MACH_RESOLVE 0
#define MACH_RESOLVE_IF_CHANGED

void mach_dump_stack(FILE *out, char *tag);

/* mach_set_spec_provider registers the given function so that
   mach_process can resolve the SpecName to a Spec. */
void mach_set_spec_provider(void * ctx, provider f);

/* mach_set_ctx does that. */
void mach_set_ctx(void * ctx);

/* mach_eval is a utilty function that executes the given ECMAScript
   source and returns the result, which better be a string.  Return
   the result rendered in JSON.
   
   This function should probably evolve to return an int and write a
   given string. */
int mach_eval(char* src, char* dst, int limit);

/* mach_match is a utility that does pattern matching.  Returns
   bindings rendered as JSON.
   
   This function should probably evolve to return an int and write a
   given string. */
int mach_match(JSON pattern, JSON message, JSON bindings, JSON dst, int limit);

/* Experimental objects API below */

/* MACH_OKAY is a return code reporting that all is well in the
   world. */
#define MACH_OKAY (0)

/* MACH_SAD is a return code reporting grave, unspecified sadness. */
#define MACH_SAD (1)

/* MACH_TOO_BIG is a return code reporting that the result, when
   serialized, was larger than the given limit. */
#define MACH_TOO_BIG (2)

/* mach_make_crew creates a new Crew with the given id.  The new Crew
   is serialized as JSON and written to dst. */
int mach_make_crew(S id, JSON dst, size_t limit);

/* mach_set_machine adds or updates a machine in the given Crew.
   Reserializes the Crew and writes it to dst. */
int mach_set_machine(JSON crew, S id, S specRef, JSON bindings, S node, JSON dst, size_t limit);

/* mach_rem_machine removes the given machine from the given Crew.
   Reserializes the Crew and writes it to dst. */
int mach_rem_machine(JSON crew, S id, JSON dst, size_t limit) ;

/* mach_crew_process gives the message to the Crew for processing.
   Writes a map from machine ids to steppeds as JSON to dst. */
int mach_crew_process(JSON crew, JSON message, JSON dst, size_t limit) ;

/* mach_crew_update updates the Crew to reflect the net state changes
   of the given steppeds (as written by mach_crew_process. */
int mach_crew_update(JSON crew, JSON steppeds, JSON dst, size_t limit) ;

/* mach_get_emitted just extracts emitted messages from the given
   steppeds map (as written by mach_crew_process). */
int mach_get_emitted(JSON steppeds, JSON dsts[], int most, size_t limit) ;

#ifdef __cplusplus
}
#endif // __cplusplus
