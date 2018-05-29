// Step attempts to advance to the next state.
//
// Returns {to: STATE, consumed: BOOL, emitted: MESSAGES}.
//
// STATE will be null when there was no transition.  'Consumed'
// reports whether the message was consumed during the transition.
// MESSAGES are the zero or more messages emitted by the action.
function step(ctx,spec,state,message) {
    var current = state.bs;
    var bs = current;
    var emitted = [];

    // The following strings are interpretered as aliases for our only
    // actual interpreter, which is probably close to Ecmascript 5.1.
    // "goja" is in this list for backwards compatability due to
    // vestiges of github.com/Comcast/sheens history.
    var interpreterAliases = ["ecmascript", "ecmascript-5.1", "goja"];

    var node = spec.nodes[state.node];
    if (!node) {
	throw {error: "node not found", node: state.node};
    }

    var action = node.action;
    if (action) {
	if (interpreterAliases.indexOf(action.interpreter) < 0) {
	    throw {error: "bad interpreter", interpreter: action.interpreter};
	}
	var evaled = sandboxedAction(ctx, bs, action.source);
	bs = evaled.bs;
	emitted = evaled.emitted;
    }

    var branching = node.branching;
    if (!branching || !branching.branches) {
	return null;
    }
    var against = state.bs;
    var consuming = false;
    if (branching.type == "message") {
	if (!message) {
	    return null;
	}
	consuming = true;
	against = message;
    }
    var branches = branching.branches;
    for (var i = 0; i < branches.length; i++) {
	var branch = branches[i];
	var pattern = branch.pattern;
	if (pattern) {
	    if (spec.parsepatterns) {
		pattern = JSON.parse(pattern);
	    }
	    var bss = match(ctx, pattern, against, state.bs);
	    if (!bss || bss.length == 0) {
		continue;
	    }
	    if (1 < bss.length) {
		throw {error: "too many sets of bindings", bss: bss};
	    }
	    bs = bss[0];
	}
	if (branch.guard) {
	    if (interpreterAliases.indexOf(branch.guard.interpreter) < 0) {
		throw {error: "bad guard interpreter", interpreter: branch.guard.interpreter};
	    }
	    var evaled = sandboxedAction(ctx, bs, branch.guard.source);
	    if (!evaled.bs) {
		continue;
	    }
	    bs = evaled.bs;
	    // Check that we didn't emit any messages ...
	}
	return {to: {node: branch.target, bs: bs}, consumed: consuming, emitted: emitted};
    }

    return null;
}

// walk advances from the given state as far as possible.
//
// Returns {to: STATE, consumed: BOOL, emitted: MESSAGES}.
//
// For an description of the returned value, see doc for 'step'.
function walk(ctx,spec,state,message) {
    // ToDo: Get limit from ctx.

    if (!state) {
	state = {node: "start", bs: {}};
    }

    var emitted = [];
    var consumed = false;

    stepped = {to: state, consumed: false};

    for (var i = 0; i < 100; i++) {
	// print("stepping from ", i, JSON.stringify(stepped.to), JSON.stringify(message));
	var maybe = step(ctx, spec, stepped.to, message);
	// print("         to   ", i, JSON.stringify(maybe));

	if (!maybe) {
	    // We went nowhere.  Stop.
	    break;
	}

	if (maybe.consumed) {
	    // We consumed the pending message; don't use it again.
	    message = null;
	    consumed = true;
	}

	stepped = maybe;

	if (stepped && 0 < stepped.emitted.length) {
	    // Accumulated emitted messages.
	    emitted = emitted.concat(stepped.emitted);
	}
    }

    stepped.emitted = emitted;
    stepped.consumed = consumed;

    return stepped;
}

