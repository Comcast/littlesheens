#include<stdio.h>

#include "duktape.h"
#include "machines.h"

extern duk_context *ctx;

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
  if (duk_pcall(ctx, nargs) == DUK_EXEC_SUCCESS) {
    result = duk_get_string(ctx, -1);
    /* printf("result %s\n", result); */
  } else {
    result = duk_safe_to_string(ctx, -1);
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
  duk_pop(ctx);
  return rc;
}

int mach_set_machine(JSON crew, S id, S specRef, JSON bindings, S node, JSON dst, size_t limit) {
  duk_get_global_string(ctx, "SetMachine");
  duk_push_string(ctx, crew);
  duk_push_string(ctx, id);
  duk_push_string(ctx, specRef);
  duk_push_string(ctx, bindings);
  duk_push_string(ctx, node);
  return getResult(5, dst, limit);
}

int mach_rem_machine(JSON crew, S id, JSON dst, size_t limit) {
  duk_get_global_string(ctx, "RemMachine");
  duk_push_string(ctx, crew);
  duk_push_string(ctx, id);
  return getResult(2, dst, limit);
}

int mach_crew_process(JSON crew, JSON message, JSON dst, size_t limit) {
  duk_get_global_string(ctx, "CrewProcess");
  duk_push_string(ctx, crew);
  duk_push_string(ctx, message);
  return getResult(2, dst, limit);
}

int mach_get_emitted(JSON steppeds, JSON dsts[], int most, size_t limit) {
  /* ToDo: Stop ignoring 'most'. */
  duk_get_global_string(ctx, "GetEmitted");
  duk_push_string(ctx, steppeds);
  
  int rc = MACH_OKAY;
  
  if (duk_pcall(ctx, 1) == DUK_EXEC_SUCCESS) {
     duk_size_t i, n;
     n = duk_get_length(ctx, -1);
     for (i = 0; i < n; i++) {
       if (duk_get_prop_index(ctx, -1, i)) {
	 const char * result = duk_safe_to_string(ctx, -1);
	 duk_pop(ctx);
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
    const char *result = duk_safe_to_string(ctx, -1);
    printf("error: %s\n", result);
    return MACH_SAD;
  }

  duk_pop(ctx);

  return rc;
}

int mach_crew_update(JSON crew, JSON stepped, JSON dst, size_t limit) {
  duk_get_global_string(ctx, "CrewUpdate");
  duk_push_string(ctx, crew);
  duk_push_string(ctx, stepped);
  return getResult(2, dst, limit);
}
