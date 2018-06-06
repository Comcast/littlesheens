/* Little Sheens C API */

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

/* provider is the signature for a function that can resolve the given
   SpecName to Spec.  The first argument will be _ctx.  The second
   argument is a string that represents the JSON representation of the
   cached spec if any.  If that cached representation is the current
   representation, then the provide can just return NULL. */
typedef char * (*mach_provider)(void*, const char *, const char *);

/* A generic mode type. */
typedef unsigned int mach_mode;

/* mach_make_ctx creates a new Little Sheens context (but does not use
   it).  Call mach_set_ctx() to use the ctx you create.  For example:

     mach_set_ctx(mach_make_ctx());
     mach_open();
     ...
     mach_close();
     free(mach_get_ctx());

   */
void *mach_make_ctx() ;

/* mach_set_ctx sets the given context as the (global) active
   context. */
void mach_set_ctx(void *c) ;

/* mach_get_ctx just returns the (global) active context. */  
void *mach_get_ctx() ;

/* mach_open creates and initializes the runtime.  (Currently there is
   only one global runtime.) */
int mach_open();

/* mach_close frees the runtime. */
void mach_close();

/* mach_process takes a machine State and a Message and returns the
   new state (if any) and any emitted messages and other data that
   indicates what happened.
   
   Also see mach_crew_process. */
int mach_process(JSON state, JSON message, JSON dst, int limit);

/* MACH_FREE_FOR_PROVIDER is a mode for a spec provider that indicates
 the the caller of the provider should free the spec (JSON) that the
 provider returns.  Useful in mach_set_spec_provider().*/
#define MACH_FREE_FOR_PROVIDER (1UL<<2UL)

/* mach_set_spec_provider registers the given function so that
   mach_process can resolve the SpecName to a Spec.  Available modes:
   MACH_FREE_FOR_PROVIDER, which will cause the string returned by the
   provider to be freed.  Use 0 if you don't want that. */
void mach_set_spec_provider(void * ctx, mach_provider f, mach_mode m);

/* mach_eval is a utilty function that executes the given ECMAScript
   source and writes the result, which better be a string, to dst.
   Returns MACH_OKAY on success.
*/
int mach_eval(char* src, char* dst, int limit);

/* mach_match is a utility that does pattern matching.  Returns
   bindings rendered as JSON to dst.  Returns MACH_OKAY on success.
*/
int mach_match(JSON pattern, JSON message, JSON bindings, JSON dst,
int limit);

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

/* mach_do_emitted iterates over emitted messages.  Not useful with
   closures? */
int mach_do_emitted(JSON steppeds, int (*f)(JSON)) ;

/* mach_set_spec_cache sets the spec cache entries limit. */
int mach_set_spec_cache_limit(int limit) ;

/* mach_clear_spec_cache clears the spec cache. */
int mach_clear_spec_cache() ;

/* mach_enable_spec_cache enables (1) or disables (0) the spec cache. */
int mach_enable_spec_cache(int enable) ;

/* A utility for seeing the current Duktape stack. */
void mach_dump_stack(FILE *out, char *tag);

#ifdef __cplusplus
}
#endif // __cplusplus
