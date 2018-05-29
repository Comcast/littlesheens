var Times = function() {
    var totals = {}
    var clocks = {};
    var enabled = false;

    return {
	enable: function() {
	    enabled = true;
	},
	disable: function() {
	    enabled = false;
	},
	tick: function(what) {
	    if (!enabled) return;
	    clocks[what] = new Date().getTime();
	},
	tock: function(what) {
	    if (!enabled) return;
	    var elapsed = new Date().getTime() - clocks[what];
	    var entry = totals[what];
	    if (!entry) {
		entry = {ms: 0, n: 0};
		totals[what] = entry;
	    }
	    entry.ms += elapsed;
	    entry.n++;
	},
	summary: function() {
	    return totals;
	},
	reset: function() {
	    totals = {};
	},
    };
}();

