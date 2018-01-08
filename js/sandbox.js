// sandboxedAction wishes to be a function that can evaluate
// ECMAScript source in a fresh, pristine, sandboxed environment.
//
// Returns {bs: BS, emitted: MESSAGES}.
function sandboxedAction(ctx, bs, src) {
    // ToDo: Different env for guards: no emitting.
    
    // ToDo: try/catch sandbox protection.

    // ToDo: A real sandbox! Should only have access to a new,
    // standard ES5 (?) environment and whatever additional stuff
    // specifically provided.  A sandbox environment should be
    // discarded after use. Most likely will need a new primitive
    // implemented in C.

    if (!bs) {
	bs = {};
    }

    var bs_js = JSON.stringify(bs);
    
    var code = "\n" +
	"var emitting = [];\n" + 
	"var env = {\n" + 
	"  bindings: " + bs_js + ",\n" +  // Maybe JSON.parse.
	"  out: function(x) { emitting.push(x); }\n" + 
        "}\n" + 
	"\n" + 
	"var bs = (function(_) {\n" + src + "\n})(env);\n" +
	"JSON.stringify({bs: bs, emitted: emitting});\n" + 
	"\n";

    try {
	var result_js = sandbox(code);
	return JSON.parse(result_js);
    } catch (e) {
	print("walk error", e);
	// Make a binding for the error so that branches could deal
	// with the error.
	//
	// ToDo: Do not overwrite?
	//
	// ToDo: Implement the spec switch that enabled
	// branching-based action error-handling.
	bs.error = e;
	return {bs: bs, error: e};
    }
}

