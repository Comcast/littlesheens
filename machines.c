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
#include <duk_print_alert.h>
#include <asm/errno.h>
#include <errno.h>

#include "duktape.h"
#include "machines.h"
#include "machines_js.h"

typedef struct {
  duk_context *dctx;
  mach_provider provider;
  mach_mode provider_mode;
  void * provider_ctx;
} Ctx;

/* ctx is a global, shared context object. */
static Ctx *ctx = NULL;

void *mach_make_ctx() {
  void* ret = malloc(sizeof(Ctx));

  memset(ret, 0, sizeof(Ctx));

  return ret;
}

void mach_set_ctx(void *c) {
  ctx = c;
}

void *mach_get_ctx() {
  return ctx;
}

void mach_dump_stack(FILE *out, char *tag) {
  duk_push_context_dump(ctx->dctx);
  fprintf(out, "stack_dump %s\n%s\n", tag, duk_safe_to_string(ctx->dctx, -1));
  duk_pop(ctx->dctx);
}


void mach_set_spec_provider(void * pctx, mach_provider f, mach_mode m) {
  ctx->provider_ctx = pctx;
  ctx->provider = f;
  ctx->provider_mode = m;
}

/* providerer is a bridge function that is exposed in the ECMAScript
   environment as the value of 'provider'.  When ECMAScript code calls
   'provider', the C function that's stored at _provider is
   invoked. */
static duk_ret_t providerer(duk_context *dctx) {
  const char *name = duk_to_string(dctx, 0);
  const char *cached = duk_to_string(dctx, 1);
  /* printf("bridge provider %s\n", s); */
  const char *result = ctx->provider(ctx->provider_ctx, name, cached);
  duk_push_string(dctx, result);

  if (result != NULL && ctx->provider_mode & MACH_FREE_FOR_PROVIDER) {
    free((char*)result);
  }
  
  return 1;
}

char * strdup(const char *s) {
  size_t n;
  char *acc;

  if (s == NULL) return NULL;

  n = strlen(s) + 1;
  acc = (char*) malloc(n);
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
  static const size_t dst_limit = 16*1024;
  char *dst, *src;
  int rc;

  if (ctx == NULL) {
    return MACH_SAD;
  }

  src = mach_machines_js();
  if (src == NULL) {
    return MACH_SAD;
  }

  dst = malloc(dst_limit);
  if (dst == NULL) {
    errno = ENOMEM;
    return MACH_SAD;
  }

  if (ctx->dctx) {
    mach_close();
  }

  ctx->dctx = duk_create_heap_default();

  duk_print_alert_init(ctx->dctx, 0);

  duk_push_c_function(ctx->dctx, providerer, 2);
  duk_put_global_string(ctx->dctx, "provider");

  duk_push_c_function(ctx->dctx, sandbox, 1);
  duk_put_global_string(ctx->dctx, "sandbox");

  rc = mach_eval(src, dst, (int) dst_limit);
  free(dst);

  if (rc != MACH_OKAY) {
    mach_close();
  }

  return rc;
}

/* API: mach_close, which is an exposed library function, releases the
   ECMAScript heap. */
void mach_close() {
  if (ctx && ctx->dctx) {
    duk_destroy_heap(ctx->dctx);
    ctx->dctx = NULL;
  }
}

/* API: mach_eval, which is an exposed library function, evaluates the
   given string as ECMAScript in the global duktape heap. */
int mach_eval(char *src, JSON dst, int limit) {
  int rc = duk_peval_string(ctx->dctx, src);
  if (rc != 0) {
    const char *err = duk_safe_to_string(ctx->dctx, -1);
    fprintf(stderr, "mach_eval error %s\n", err);
    return MACH_SAD;
  }
  rc = copystr(dst, limit, (char*) duk_get_string(ctx->dctx, -1));
  duk_pop(ctx->dctx);
  return MACH_OKAY;
}

/* Utility function: Like printf except calls eval and does not return
   a result. */
int evalf(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t buf_limit = 16*1024;
  char * buf = (char*) malloc(buf_limit);

  int wrote = vsnprintf(buf, buf_limit, fmt, args);
  if (buf_limit <= wrote) {
    free(buf);
    return MACH_TOO_BIG;
  }
  int rc = duk_peval_string(ctx->dctx, buf);
  va_end(args);
  free(buf);
  
  if (rc != 0) {
    const char *err = duk_safe_to_string(ctx->dctx, -1);
    fprintf(stderr, "evalf error %s\n", err);
    rc = MACH_SAD;
  } else {
    rc = MACH_OKAY;
  }
  duk_pop(ctx->dctx);
  
  return rc;
}

/* API: mach_process, which is an exposed library function, calls the
   ECMAScript function bound to Process.  Returns NULL. */
int mach_process(JSON state, JSON message, JSON dst, int limit) {
  JSON result;
  duk_get_global_string(ctx->dctx, "Process");
  duk_push_string(ctx->dctx, state);
  duk_push_string(ctx->dctx, message);
  if (duk_pcall(ctx->dctx, 2) == DUK_EXEC_SUCCESS) {
    result = (JSON) duk_get_string(ctx->dctx, -1);
    /* printf("mach_process result: %s\n", result); */
  } else {
    result = (JSON) duk_safe_to_string(ctx->dctx, -1);
    fprintf(stderr, "mach_process error: %s\n", result);
  }
  int rc = copystr(dst, limit, result);
  duk_pop(ctx->dctx);
  return rc;
}

/* API: mach_match, which is an exposed library utility function, calls the
   ECMAScript function bound to Match.  Returns NULL. */
int mach_match(JSON pattern, JSON message, JSON bindings, JSON dst, int limit) {
  JSON result;
  duk_get_global_string(ctx->dctx, "Match");
  duk_push_object(ctx->dctx); // Another "ctx" ...
  duk_push_string(ctx->dctx, pattern);
  duk_push_string(ctx->dctx, message);
  duk_push_string(ctx->dctx, bindings);
  if (duk_pcall(ctx->dctx, 4) == DUK_EXEC_SUCCESS) {
    result = (JSON) duk_get_string(ctx->dctx, -1);
    /* printf("mach_match result: %s\n", result); */
  } else {
    result = (JSON) duk_safe_to_string(ctx->dctx, -1);
    fprintf(stderr, "mach_match error: %s\n", result);
  }
  int rc = copystr(dst, limit, result);
  duk_pop(ctx->dctx);
  return rc;
}

/* API: mach_set_spec_cache_limit sets the spec cache limit.  This
   function does NOT enable the cache if it is not already enabled.

   The default limit is 'DefaultSpecCacheLimit' in 'driver.js'. */
int mach_set_spec_cache_limit(int limit) {
  return evalf("SpecCache.setLimit(%d)", limit);
}

int mach_enable_spec_cache(int enable) {
  if (enable) {
    return evalf("SpecCache.enable()");
  } else {
    return evalf("SpecCache.disable()");
  }
}

/* API: mach_clear_spec_cache empties the cache (and resets cache
   statistics). */
int mach_clear_spec_cache(int enable) {
  return evalf("SpecCache.clear()");
}

int mach_make_crew(S id, JSON dst, size_t limit) {
  /* We'll just sprintf the answer (for now). */
  int n = snprintf(dst, limit, "{\"id\":\"%s\",\"machines\":{}}", id);
  if (limit < n) {
    return MACH_TOO_BIG;
  }
  return MACH_OKAY;
}

int getResult(int nargs, JSON dst, size_t limit) {
  const char *result;
  if (duk_pcall(ctx->dctx, nargs) == DUK_EXEC_SUCCESS) {
    result = duk_get_string(ctx->dctx, -1);
    /* printf("result %s\n", result); */
  } else {
    result = duk_safe_to_string(ctx->dctx, -1);
    printf("getResult serror %s\n", result);
  }
  if (result == NULL) {
    result = "";
  }
  int n = strlen(result);
  int rc = MACH_OKAY;
  if (limit < n) {
    rc = MACH_TOO_BIG;
  } else {
    strncpy(dst, result, limit);
  }
  duk_pop(ctx->dctx);
  return rc;
}

int mach_set_machine(JSON crew, S id, S specRef, JSON bindings, S node, JSON dst, size_t limit) {
  duk_get_global_string(ctx->dctx, "SetMachine");
  duk_push_string(ctx->dctx, crew);
  duk_push_string(ctx->dctx, id);
  duk_push_string(ctx->dctx, specRef);
  duk_push_string(ctx->dctx, bindings);
  duk_push_string(ctx->dctx, node);
  return getResult(5, dst, limit);
}

int mach_rem_machine(JSON crew, S id, JSON dst, size_t limit) {
  duk_get_global_string(ctx->dctx, "RemMachine");
  duk_push_string(ctx->dctx, crew);
  duk_push_string(ctx->dctx, id);
  return getResult(2, dst, limit);
}

int mach_crew_process(JSON crew, JSON message, JSON dst, size_t limit) {
  duk_get_global_string(ctx->dctx, "CrewProcess");
  duk_push_string(ctx->dctx, crew);
  duk_push_string(ctx->dctx, message);
  return getResult(2, dst, limit);
}

int mach_get_emitted(JSON steppeds, JSON dsts[], int most, size_t limit) {
  /* ToDo: Stop ignoring 'most'. */
  duk_get_global_string(ctx->dctx, "GetEmitted");
  duk_push_string(ctx->dctx, steppeds);
  
  int rc = MACH_OKAY;
  
  if (duk_pcall(ctx->dctx, 1) == DUK_EXEC_SUCCESS) {
     duk_size_t i, n;
     n = duk_get_length(ctx->dctx, -1);
     for (i = 0; i < n; i++) {
       if (duk_get_prop_index(ctx->dctx, -1, i)) {
	 const char * result = duk_safe_to_string(ctx->dctx, -1);
	 duk_pop(ctx->dctx);
	 int n = strlen(result);
	 if (limit < n) {
	   rc = MACH_TOO_BIG;
	   break;
	 }
	 strncpy(dsts[i], result, limit);
       } else {
	 printf("error: no item at %d\n", (int) i);
       }
     }
     for (; i < most; i++) {
       memset(dsts[i], 0, limit);
     }
  } else {
    const char *result = duk_safe_to_string(ctx->dctx, -1);
    printf("error: %s\n", result);
    return MACH_SAD;
  }

  duk_pop(ctx->dctx);

  return rc;
}

int mach_do_emitted(JSON steppeds, int (*f)(JSON)) {
  /* ToDo: Stop ignoring 'most'. */
  duk_get_global_string(ctx->dctx, "GetEmitted");
  duk_push_string(ctx->dctx, steppeds);
  
  int rc = MACH_OKAY;
  
  if (duk_pcall(ctx->dctx, 1) == DUK_EXEC_SUCCESS) {
     duk_size_t i, n;
     n = duk_get_length(ctx->dctx, -1);
     for (i = 0; i < n; i++) {
       if (duk_get_prop_index(ctx->dctx, -1, i)) {
	 const char * result = duk_safe_to_string(ctx->dctx, -1);
	 duk_pop(ctx->dctx);
	 int ret = f((char *) result);
	 if (ret) {
	   break;
	 }
       } else {
	 printf("error: no item at %d\n", (int) i);
       }
     }
  } else {
    const char *result = duk_safe_to_string(ctx->dctx, -1);
    printf("error: %s\n", result);
    return MACH_SAD;
  }

  duk_pop(ctx->dctx);

  return rc;
}

int mach_crew_update(JSON crew, JSON stepped, JSON dst, size_t limit) {
  duk_get_global_string(ctx->dctx, "CrewUpdate");
  duk_push_string(ctx->dctx, crew);
  duk_push_string(ctx->dctx, stepped);
  return getResult(2, dst, limit);
}
