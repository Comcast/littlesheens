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

// sandboxedAction wishes to be a function that can evaluate
// ECMAScript source in a fresh, pristine, sandboxed environment.
//
// Returns {bs: BS, emitted: MESSAGES}.
function sandboxedAction(ctx, bs, src, props) {
    // This function calls a (presumably primitive) 'sandbox' function
    // to do the actual work.  That function is probably in
    // 'machines.c'.

    // ToDo: Different env for guards: no emitting.
    Times.tick("sandbox");

    if (!bs) {
	bs = {};
    }

    var bs_js = JSON.stringify(bs);
    var props_js = JSON.stringify(props);
    
    // NOTE: use `print.apply(null, arguments);` to dynamically pass input arguments
    var code = "\n" +
	"var emitting = [];\n" + 
	"var env = {\n" + 
	"  bindings: " + bs_js + ",\n" +  // Maybe JSON.parse.
	"  props: " + props_js + ",\n" +  // Maybe JSON.parse.
	"  log: function(x) { x['log.level']='info';print(JSON.stringify(x)); },\n" + 
	"  metric: function(x) { x['log.level']='metric';print(JSON.stringify(x)); },\n" + 
	"  gensym: function () { var ret='';var chs='abcdefghijklmnopqrstuvwxyz0123456789';var len=chs.length;for(var i=0;i<32;i++) {ret+=chs.charAt(Math.floor(Math.random()*len));}return ret; },\n" + 
	"  out: function(x) { emitting.push(x); }\n" + 
        "}\n" + 
	"\n" + 
	"var bs = (function(_) {\n" + src + "\n})(env);\n";

    // The following conditional checks for 'safeEval', might have
    // been defined by https://www.npmjs.com/package/safe-eval.  That
    // 'safeEval' wants an expression, while the Duktape-based sandbox
    // just takes a block.
    if (typeof safeEval === 'undefined') { // Just for ../nodemodify.sh
	code += "JSON.stringify({bs: bs, emitted: emitting});\n";
    } else {
	code = "function() {\n" + code + "\n" +
	    "return JSON.stringify({bs: bs, emitted: emitting});\n" +
	    "}();\n";
    }

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
        Times.tock("sandbox");
    }
}

