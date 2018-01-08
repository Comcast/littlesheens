// A Javascript (!) program that demonstrates a little use of the
// Process() API, which the the core machine API.
//
// See 'main.c' for real C examples that work with a collection of
// machines (called a "crew").

var initialState = {
    bs: {count: 0},
    node: "start",
    spec: "double"
};

var messages = [{double:1}, {double:10}, {double:100}];

var state = initialState;

for (var i = 0; i < messages.length; i++) {
    var message = messages[i];

    var state_js = JSON.stringify(state);
    var message_js = JSON.stringify(message);
    print("Process", i, state_js, message_js);

    // This Process() call is the same as its C API equivalent.
    var result_js = Process(state_js, message_js);
    print("Process returned", result_js);
    var result = JSON.parse(result_js);

    // Let's just use that state.
    // (We might want to write it out first.)
    state.bs = result.to.bs;
    state.node = result.to.node;
    // state.spec hasn't changed.
    print("state", i, JSON.stringify(state));

    // We should probably now Process each "emitted" message
    // (recursively with a limit).
}
