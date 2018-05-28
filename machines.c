#include<stdio.h>
#include<duk_print_alert.h>

#include "duktape.h"
#include "machines.h"
#include "machines_js.h"

/* ctx is a global, shared duktape heap.  Sorry.  This variable is
   initialized by mach_open(). */
duk_context *ctx;

void mach_dump_stack(FILE *out, char *tag) {
  duk_push_context_dump(ctx);
  fprintf(out, "stack_dump %s\n%s\n", tag, duk_safe_to_string(ctx, -1));
  duk_pop(ctx);
}


/* _provider is a global, shared C function that's invoked by
   providerer().  _provider is set by mach_set_spec_provider(). */
provider _provider;

/* _ctx is yet another global, shared C thing that currently is passed
   as the first argument to _provider.  Despite its name, _ctx has
   absolutely nothing to do with ctx.  Sorry.  _ctx is set by
   mach_set_ctx(). */
void * _ctx;

void mach_set_spec_provider(void * ctx, provider f) {
  _ctx = ctx;
  _provider = f;
}

void mach_set_ctx(void * ctx) {
  _ctx = ctx;
}

/* providerer is a bridge function that is exposed in the ECMAScript
   environment as the value of 'provider'.  When ECMAScript code calls
   'provider', the C function that's stored at _provider is
   invoked. */
static duk_ret_t providerer(duk_context *ctx) {
  const char *s = duk_to_string(ctx, 0);
  /* printf("bridge provider %s\n", s); */
  const char *result = _provider(_ctx, s, MACH_RESOLVE);
  duk_push_string(ctx, result);
  return 1;
}

char * strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *acc = (char*) malloc(n);
  if (acc == (char*) 0) {
    return (char*) 0;
  }
  return (char*) memcpy(acc, s, n);
}

int copystr(char *dst, int limit, char *src) {
  if (src == NULL) {
    *dst = '\0';
    return MACH_OKAY;
  }
  if (limit <= strlen(src)) {
    return MACH_TOO_BIG;
  } else {
    strncpy(dst, src, limit);
    return MACH_OKAY;
  }
}

/* sandbox is a minimal EMCAscript evaluation sandbox in an empty
   environment.  The given source code better return a string.  Needs
   a little work ... 

   Like various explicit/known limits and ... 

   Also see https://github.com/svaarala/duktape/blob/master/doc/sandboxing.rst
*/

static duk_ret_t sandbox(duk_context *ctx) {
  const char *src = duk_to_string(ctx, 0);

  duk_context *box = duk_create_heap_default();
  duk_push_string(box, src);

  duk_ret_t rc = duk_peval(box);
  /* If we ran into an error, it's on the stack. */
  const char *result = duk_safe_to_string(box, -1);
  result = strdup(result);
  if (rc != DUK_EXEC_SUCCESS) {
    fprintf(stderr, "warning: sandbox returned non-zero rc=%d result=%s code:\n%s\n", rc, result, src);
  }

  duk_destroy_heap(box);
  duk_push_string(ctx, result);
  free((char*) result); /* result was interned! */

  return 1; /* If non-zero, caller will see 'undefined'. */
}


/* API: mach_open, which is an exposed library function, creates the
   duktape heap, sets the binding for 'router' function, and maybe
   does some other initialization. */
int mach_open() {
  ctx = duk_create_heap_default();

  duk_print_alert_init(ctx, 0);

  duk_push_c_function(ctx, providerer, 1);
  duk_put_global_string(ctx, "provider");

  duk_push_c_function(ctx, sandbox, 1);
  duk_put_global_string(ctx, "sandbox");

  if (1) {
    size_t dst_limit = 16*1024;
    char * dst = (char*) malloc(dst_limit);
    char * src = mach_machines_js();
    int rc = mach_eval(src, dst, dst_limit);
    free(dst);
    
    return rc;
  }

  return MACH_OKAY;
}

/* API: mach_close, which is an exposed library function, releases the
   ECMAScript heap. */
void mach_close() {
  duk_destroy_heap(ctx);
  ctx = NULL;
}

/* API: mach_eval, which is an exposed library function, evaluates the
   given string as ECMAScript in the global duktape heap. */
int mach_eval(char *src, JSON dst, int limit) {
  int rc = duk_peval_string(ctx, src);
  if (rc != 0) {
    return rc;
  }
  rc = copystr(dst, limit, (char*) duk_get_string(ctx, -1));
  duk_pop(ctx);
  return rc;
}

/* API: mach_process, which is an exposed library function, calls the
   ECMAScript function bound to Process.  Returns NULL. */
int mach_process(JSON state, JSON message, JSON dst, int limit) {
  JSON result;
  duk_get_global_string(ctx, "Process");
  duk_push_string(ctx, state);
  duk_push_string(ctx, message);
  if (duk_pcall(ctx, 2) == DUK_EXEC_SUCCESS) {
    result = (JSON) duk_get_string(ctx, -1);
    /* printf("mach_process result: %s\n", result); */
  } else {
    result = (JSON) duk_safe_to_string(ctx, -1);
    printf("mach_process error: %s\n", result);
  }
  int rc = copystr(dst, limit, result);
  duk_pop(ctx);
  return rc;
}

/* API: mach_match, which is an exposed library utility function, calls the
   ECMAScript function bound to Match.  Returns NULL. */
int mach_match(JSON pattern, JSON message, JSON bindings, JSON dst, int limit) {
  JSON result;
  duk_get_global_string(ctx, "Match");
  duk_push_object(ctx); // Another "ctx" ...
  duk_push_string(ctx, pattern);
  duk_push_string(ctx, message);
  duk_push_string(ctx, bindings);
  if (duk_pcall(ctx, 4) == DUK_EXEC_SUCCESS) {
    result = (JSON) duk_get_string(ctx, -1);
    /* printf("mach_match result: %s\n", result); */
  } else {
    result = (JSON) duk_safe_to_string(ctx, -1);
    printf("mach_match error: %s\n", result);
  }
  int rc = copystr(dst, limit, result);
  duk_pop(ctx);
  return rc;
}
