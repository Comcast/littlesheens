// sandboxedAction wishes to be a function that can evaluate
// ECMAScript source in a fresh, pristine, sandboxed environment.
//
// Returns {bs: BS, emitted: MESSAGES}.
function sandboxedAction(ctx, bs, src) {
    // This function calls a (presumably primitive) 'sandbox' function
    // to do the actual work.  That function is probably in
    // 'machines.c'.

    // ToDo: Different env for guards: no emitting.
    Times.tick("action");

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
	"JSON.stringify({bs: bs, emitted: emitting});\n";

    try {
	var result_js = sandbox(code);
	try {
	    return JSON.parse(result_js);
	} catch (e) {
	    throw e + " on result parsing of '" + result_js + "'";
	}
    } catch (e) {
	print("walk action sandbox error", e);
	// Make a binding for the error so that branches could deal
	// with the error.
	//
	// ToDo: Do not overwrite?
	//
	// ToDo: Implement the spec switch that enabled
	// branching-based action error-handling.
	bs.error = e;
	return {bs: bs, error: e};
    } finally {
        Times.tock("action");
    }
}

