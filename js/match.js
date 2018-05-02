// A Javascript implementation of Rules Core (and Sheens) pattern matching
//
// Status: Frequently compiles.

// function(CTX,P,M,BS), where CTX is an unused context, P is a
// pattern, M is a message, and BS are input bindings.
//
// Returns null or a set of sets of bindings.
var match = function() {

    var isVar = function(s) {
	return typeof s == 'string' && s.charAt(0) == '?';
    };

    var isOptVar = function(s) {
	return typeof s == 'string' && s.substring(0,2) == '??';
    };

    var copyMap = function(m) {
	var acc = {};
	for (p in m) {
	    acc[p] = m[p];
	}
	return acc;
    };

    var copyArray = function(xs) {
	return xs.slice();
    }

    var isAnonymous = function(s) {
	return s === "?";
    }

    var extend = function(bs, b, v) {
	var acc = copyMap(bs);
	if (isAnonymous(b)) {
	    return acc;
	}
	acc[b] = v;
	return acc;
    };

    var match;

    var matchWithBindings = function(ctx, bss, v, mv) {
	var acc = [];
	for (var i = 0; i < bss.length; i++) {
	    var bs = bss[i];
	    acc = acc.concat(match(ctx, v, mv, bs))
	}
	return acc;
    };

    var dbg = function(tag, x) {
	// print(tag + " " + JSON.stringify(x));
    };

    var arraycatMatch = function(ctx, bss, p, m, varCount) {
	dbg("arraycatMatch", {p: p, m: m, bss: bss});

	if (varCount === undefined) {
	    varCount = 0;
	}

	// The pattern should be an array.
	if (!Array.isArray(p)) {
	    throw "internal error: pattern " + JSON.stringify(p) + " isn't an array";
	}

	if (!Array.isArray(m)) {
	    return [];
	}

	if (p.length == 0) {
	    // An empty pattern array matches any array.
	    return bss;
	}

	// Recursive implementation
	var y = p[0];
	if (isVar(y)) {
	    if (0 < varCount) {
		throw "can't have more than one variable in array";
	    }
	    varCount++;
	}
	dbg("y", y);
	var acc = []; // Accumulate sets of output bindings.
	for (var i = 0; i < bss.length; i++) {
	    var bs = bss[i];
	    // How many ways can we match y? An array is a *set*, not
	    // a list.
	    var some = false;
	    for (var j = 0; j < m.length; j++) {
		var x = m[j];
		var bss_ = match(ctx, y, x, bs);
		dbg("match", {x:x,y:y,bs:bs,bss:bss_});
		// Filter bindings based on remaining pattern and the
		// message with the current element removed.  This
		// approach is probably not optimal, but it's easier
		// to understand.
		var p_ = p.slice(i+1);
		var m_ = copyArray(m); m_.splice(j,1);
		bss_ = arraycatMatch(ctx, bss_, p_, m_, varCount);
		for (var k = 0; k < bss_.length; k++) {
		    acc.push(bss_[k]);
		    some = true;
		}
	    }
	    if (!some && isOptVar(y)) {
		dbg("opt", {y:y,bs:bs});
		acc.push(bs);
	    }
	}

	dbg("->", {acc: acc, bss: bss, p: p, m: m});
	return acc;
    };

    var mapcatMatch = function(ctx, bss, p, m) {
	for (k in p) {
	    var v = p[k];
	    var mv = m[k];
	    if (mv === undefined) {
		if (!isOptVar(v)) {
		    return [];
		}
	    }
	    var acc = matchWithBindings(ctx, bss, v, mv)
	    if (acc.length == 0) {
		return [];
	    }
	    bss = acc;
	}
	return bss;
    };

    match = function(ctx,p,m,bs) {
	if (!bs) {
	    bs = [];
	}
	if (isVar(p)) {
	    var binding = bs[p];
	    if (binding) {
		return match(ctx, binding, m, bs);
	    } else {
		return [extend(bs, p, m)];
	    }
	} else {
	    switch (typeof p) {
	    case 'object':
		if (Array.isArray(p)) {
		    if (Array.isArray(m)) {
			return arraycatMatch(ctx, [bs], p, m);
		    } else {
			return [];
		    }
		}
		switch (typeof m) {
		case 'object':
		    if (null === p) {
			return [];
		    }
		    if (p.length == 0) {
			return [bs];
		    }
		    return mapcatMatch(ctx, [bs], p, m);
		default:
		    return [];
		}
	    default:
		if (p == m) {
		    return [bs];
		}
		return [];
	    }
	}
    };

    return match;
}();

