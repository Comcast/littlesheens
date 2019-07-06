var Cfg = {
    MaxSteps: 100
};

var Stats = {
    GetSpec: 0,
    ParseSpec: 0,
    GetLib: 0,
    Process: 0,
    CrewProcess: 0,
    CrewUpdate: 0,
};

var DefaultSpecCacheLimit = 128;
var DefaultLibCacheLimit = 128;

var SpecCache = cache(DefaultSpecCacheLimit);
var LibCache = cache(DefaultLibCacheLimit);

function cache(defaultLimit) {
    var enabled = false;
    var entries = {};
    var limit = defaultLimit;
    var size = 0;
    var hits = 0, misses = 0;
    var makeRoom = function(n) {
	var keys = Object.keys(entries);
	var evict = (size + n) - limit;
	for (var i = 0; 0 < evict && i < keys.length; i++) {
	    delete entries[keys[i]];
	    size--;
	    evict--;
	}
    };
    return {
	enable: function() {
	    enabled = true;
	},
	disable: function() {
	    enabled = false;
	},
	clear: function() {
	    entries = {};
	    size = 0;
	    hits = 0;
	    misses = 0;
	},
	setLimit: function(n) {
	    limit = n;
	    makeRoom(0);
	},
	add: function(k,v) {
	    if (!enabled) {
		return;
	    }
	    if (limit <= 0) {
		return;
	    }
	    var existing = entries[k];
	    var haveKey = false;
	    if (existing) {
		haveKey = true;
	    }

	    if (!haveKey) {
		makeRoom(1);
	    }
	    if (!haveKey) {
		size++;
	    }
	    entries[k] = v;
	},
	get: function(k) {
	    if (!enabled) {
		return null;
	    }
	    var v = entries[k];
	    if (v) {
		hits++;
	    } else {
		misses++;
	    }
	    return v;
	},
	summary: function() {
	    return {
		size: size,
		numberOfEntries: Object.keys(entries).length,
		limit: limit,
		hits: hits,
		enabled: enabled,
		misses: misses
	    };
	}
    };
}

function getMergedSpec(filename) {
	var orig = GetSpec(filename);
	
	if (!orig.uses) {
		return orig;
	}
	
	for (var i = 0; i < orig.uses.length; i++) {
		var parent = getMergedSpec(orig.uses[i]);
		merge(parent, orig);
	}
	
	return orig;
}

function sanitize(o) {
	if (o) {
		return JSON.stringify(JSON.parse(o));
	}
	
	return "null";
}

function merge(src, dst) {
	// merge params
	if (!dst.paramspecs) {
		dst.paramspecs = src.paramspecs;	
	} else {
		for (var key in src.paramspecs) { 
			if (!dst.paramspecs[key]) {
				dst.paramspecs[key] = src.paramspecs[key];
			}
		}	
	}
	
	// merge nodes
	if (!dst.nodes) {
		dst.nodes = src.nodes;	
	} else {
		for (var key in src.nodes) {
			var node = dst.nodes[key];
			if (node) {
				var srcNode = src.nodes[key];
				
				// merge action
				if (!node.action) {
					node.action = srcNode.action;
				}
				
				// merge branching
				if (!node.branching) {
					node.branching = srcNode.branching;
				} else {
					for (var i=0; i<srcNode.branching.branches.length; i++) {
						var srcBranch = srcNode.branching.branches[i];
						var srcPattern = sanitize(srcBranch.pattern);
						var found = false;
						for (var j=0; j<node.branching.branches.length; j++) {
							if (srcPattern === sanitize(node.branching.branches[j].pattern)) {
								found = true;
								break;
							}
						}
						if (!found) {
							node.branching.branches.push(srcBranch);
						}
					}
				}
			} else {
				dst.nodes[key] = src.nodes[key];	
			}
		}	
	}
}

function GetSpec(filename) {
    Stats.GetSpec++;
    
    // print("GetSpec " + filename + " (cache size " + SpecCacheLimit + ")");
    
    var cached = SpecCache.get(filename);

    var cachedString = "";
    if (cached) {
	// print("GetSpec " + filename + " in cache");
	cachedString = cached.string;
    }
    var js = provider(filename, cachedString);
    // js can be null, the same as the given cachedString, or a new
    // string.
    
    if (!js) {
	if (cached) {
	    return cached.spec;
	}
	var err = {filename: filename, error: "not provided"};
	throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }

    if (js == cachedString) {
	return cached.spec;
    }
    
    var spec = JSON.parse(js);
    Stats.ParseSpec++;
    Object.seal(spec);
    SpecCache.add(filename, {
	    spec: spec,
	    string: js
    });

    return spec;
}

function GetLib(filename) {
    Stats.GetLib++;
    
    // print("GetLib " + filename + " (cache size " + SpecCacheLimit + ")");
    
    var cached = LibCache.get(filename);

    var cachedString = "";
    if (cached) {
		// print("GetLib " + filename + " in cache");
		cachedString = cached;
    }
    var lib = lib_provider(filename, cachedString);
    // js can be null, the same as the given cachedString, or a new
    // string.
    
    if (!lib) {
		if (cached) {
		    return cached;
		}
		var err = {filename: filename, error: "not provided"};
		throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }

    if (lib == cachedString) {
		return cached;
    }
    
    LibCache.add(filename, lib);

    return lib;
}

function Process(state_js, message_js) {
    Stats.Process++;
    try {
	var state = JSON.parse(state_js);

	// Don't do anything to the name of the spec that will be
	// handed to the spec provider.  Let the spec provider do
	// what's required.
	// var specFilename = state.spec;
	
	var spec = getMergedSpec(state.spec);
	delete state.spec;
	var message = JSON.parse(message_js);
	
	// TODO how to determine mid?
	var mid = "none";
	var props = getStepProps(mid, spec, message.tx);
	
	var stepped = walk(Cfg, spec, state, message, props);
	
	return JSON.stringify(stepped);
    } catch (err) {
	print("driver Process error", err, JSON.stringify(err));
	throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }
}

function Match(_, pattern_js, message_js, bindings_js) {
    try {
	if (bindings_js.length == 0) {
	    bindings_js = "{}";
	}
	var bss = match(null, JSON.parse(pattern_js), JSON.parse(message_js), JSON.parse(bindings_js));
	return JSON.stringify(bss);
    } catch (err) {
	print("driver Match error", err, JSON.stringify(err));
	throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }
}

function SetMachine(crew_js, id, specRef, bindings_js, nodeName) {
    try {
	
	var crew = JSON.parse(crew_js);
	var bs = JSON.parse(bindings_js);
	var machines = crew.machines;
	if (!machines) {
	    machines = {};
	    crew.machines = machines;
	}
	machines[id] = {
	    spec: specRef,
	    node: nodeName,
	    bs: bs
	};

//	return JSON.stringify(crew);
	var message = {
		to: {
			receiptHandle: id,
		},
		message:{
		},
	};
	var out = process(crew, message);
	var stepped = out[id];
	stepped.spec = specRef;
	
	return JSON.stringify(out);
	
    } catch (err) {
	print("driver SetMachine error", err, JSON.stringify(err));
	throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }
}

function RemMachine(crew_js, id) {
    try {
	
	var crew = JSON.parse(crew_js);
	var machines = crew.machines;
	if (machines) {
	    var machine = machines[id];
		if (machine) {
			var spec = getMergedSpec(machine.spec);
			var node = spec.nodes["end"];
			var out;
			if (node) {
				var state = {
					node: "end",
					bs: {},
				};
				var props = getStepProps(id, spec, null);
				out = step(Cfg, spec, state, null, props);
			}
			
			// update delete flag for machine to be deleted
			if (!out) {
				out = {};
			}
			var stepped = out[id];
			if (!stepped) {
				stepped = {};
				out[id] = stepped;
			}
			stepped.delete = true;	
			
			return JSON.stringify(out);
		}
	}
	return "{}";

    } catch (err) {
	print("driver RemMachine error", err, JSON.stringify(err));
	throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }
}

function getStepProps(sheenId, spec, tx) {
	//TODO: review location, app
	var props = {
		location: "local",
		app: "",
		sheenId: sheenId,
		traceId: "",
		spanId: "",
		spec: spec.name,
	};
	
	//TODO: populate tx if not in msg
	if (tx) {
		if (tx.traceId) {
			props.traceId = tx.traceId;
		}
		if (tx.spanId) {
			props.spanId = tx.spanId;
		}
	}
	
	return props;
}

function processOne(crew, msg) {
	// for backward compatibility
	var message;
	if (msg.message) {
		message = msg;
	} else {
		message = {
			message: {
				op: "process",
				status: 0,
				payload: msg,
			}	
		};
	}		
	
	var targets;
	if (message.to && message.to.receiptHandle) {
		targets = message.to.receiptHandle;
	}
	
	if (targets) {
	    // Routing to specific machine(s).
	    if (typeof targets == 'string') {
			targets = [targets];
	    }
	    print("driver CrewProcess routing", JSON.stringify(targets));
	} else {
	    // The entire crew will see this message.
	    targets = [];
	    for (var mid in crew.machines) {
			targets.push(mid);
	    }
	}
	
	var op = message.message;
	if ("response" == op.op) {
		op = {
			op: op.op,
			status: op.status,
			payload: {
				_type_:  op.op,
				status:  op.status,
				payload: op.payload,
		    },
		};
	}
	
	//print("op: "+JSON.stringify(op))

	var steppeds = {};
	for (var i = 0; i < targets.length; i++) {
	    var mid = targets[i];
	    var machine = crew.machines[mid];
	    if (machine) {
			var spec = getMergedSpec(machine.spec);
			
			var state = {
			    node: machine.node,
			    bs: machine.bs
			};
			
			var props = getStepProps(mid, spec, message.tx);
			props.status = op.status;
			props.message = op;
			// TODO: review reply function
			props.reply = null;
			
			steppeds[mid] = walk(Cfg, spec, state, op.payload, props);
	    } // Otherwise just move on.
	}
	
	return steppeds;
}

function CrewProcess(crew_js, message_js) {
    Stats.CrewProcess++;

    try {
		var crew = JSON.parse(crew_js);
		var message = JSON.parse(message_js);
	
		// Optionally direct the message to a single machine as
		// specified in the message's (optional) "to" property.  For
		// example, if the message has the form {"to":"m42",...}, then
		// that message will be sent to machine m42 only.  Generalize
		// to accept an array: "to":["m42","m43"].
	
		var out = process(crew, message);
		
		return JSON.stringify(out);
		return "{}";
    } catch (err) {
		print("driver CrewProcess error", err, JSON.stringify(err));
		throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }
}

function process(crew, message) {
	var out = {};
	var messages = [message];
	while (messages.length > 0) {
		var tmp = [];
		for (var i=0; i<messages.length; i++) {
			var steppeds = processOne(crew, messages[i]);
			
			// process stepped
			for (var sheenId in steppeds) {
				var stepped = steppeds[sheenId];
			
				// add to output
				// `to` for states updating
				// `emitted` for outbound messages
				// `consumed` to indicate with message has been consumed by the sheen
				var orig = out[sheenId];
				if (!orig) {
					orig = {
						to: stepped.to,
						emitted: [],
						consumed: stepped.consumed,
					};
					out[sheenId] = orig;
				} else if (stepped.consumed) {
					orig.to = stepped.to;
					orig.consumed = stepped.consumed;
				}
				
				var emitted = stepped.emitted;
				for (var j=0; j<emitted.length; j++) {
					var msg = emitted[j];
					
					// outbound message if receiptHandle (sheen id) is not defined
					if (!msg.message || !msg.to || !msg.to.receiptHandle) {
						orig.emitted.push(msg);
					}
					
					// a typed route is handled outside of crew
					if (!msg.message || !msg.to || !msg.to.type) {
						tmp.push(msg);
					}
				}
			}
		}
		messages = tmp;
	}
	
	return out;
}

function CrewUpdate(crew_js, steppeds_js) {
    Stats.CrewUpdate++;
    try {
	
	var crew = JSON.parse(crew_js);
	var steppeds = JSON.parse(steppeds_js);
	for (var mid in steppeds) {
		var stepped = steppeds[mid];
		if (stepped.delete) {
			delete(crew.machines[mid]);
		} else if(stepped.spec) {
			crew.machines[mid] = {
				spec: stepped.spec,
				node: stepped.to.node,
				bs: stepped.to.bs,
			}
		} else {
			var sheen = crew.machines[mid];
			// sheen may not exist for delete action
			if (sheen) {
			    crew.machines[mid].node = stepped.to.node;
			    crew.machines[mid].bs = stepped.to.bs;
			}
		}
	}
	
	return JSON.stringify(crew);
    } catch (err) {
	print("driver CrewUpdate error", err, JSON.stringify(err));
	throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }
}

function GetEmitted(steppeds_js) {
    try {
	
	var steppeds = JSON.parse(steppeds_js);
	var emitted = [];
	for (var mid in steppeds) {
	    var stepped = steppeds[mid];
	    var msgs = stepped.emitted;
	    for (var i = 0; i < msgs.length; i++) {
		emitted.push(JSON.stringify(msgs[i]));
	    }
	}
	
	return emitted;
    } catch (err) {
	print("driver GetEmitted error", err, JSON.stringify(err));
	throw JSON.stringify({err: err, errstr: JSON.stringify(err)});
    }
}

// sandbox('JSON.stringify({"bs":{"x":1+2},"emitted":["test"]})');
