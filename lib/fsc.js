//
// fsc flow runtime lib
//

var FSC_MAX_HOPS = 1000;
var FSC_MAX_MSG_SIZE = 250000;
//var FSC_MAX_MSG_SIZE = 5000;
var FSC_MAX_STATE_SIZE = 300000;
//var FSC_MAX_STATE_SIZE = 5000;
var DEFAULT_APP_ID = "xh";

// inject component results into props

function fsc_get_app_id() {
  var app_id = DEFAULT_APP_ID;
  if (_.props.app) {
    app_id = _.props.app;
  }
  return app_id;
}

function fsc_get_eel_endpoint() {
  return "{{EELURL}}";
}

function fsc_get_sheens_topic() {
  return "{{SHEENSTOPIC}}";
}

function fsc_get_basher_topic(runtimeEnvironment) {
  if (!runtimeEnvironment) {
    runtimeEnvironment = "default";
  }
  var basherTopic = "{{BASHERTOPIC}}." + fsc_get_app_id() + "." + runtimeEnvironment;
  return basherTopic;
}

function fsc_get_debug_topic() {
  return "{{DEBUGTOPIC}}";
}

// generic helper functions

function fsc_clone_object(obj) {
  if (!obj) {
    return obj;
  }
  if (typeof o === 'object') {
    var clone = {};
    for (var key in obj) {
      if (obj.hasOwnProperty(key)) {
        clone[key] = obj[key];
      }
    }
    return clone;
  }
  return obj;
}

function fsc_merge_into(source, target, key) {
  if (!source || !target || !key) {
    return;
  }
  if (typeof key != 'string') {
    return;
  }
  if (typeof source != 'object') {
    return;
  }
  if (typeof target != 'object') {
    return;
  }
  if (Array.isArray(source[key]) && Array.isArray(target[key])) {
    var na = [];
    for (var i=0; i<target[key].length; i++) {
      na.push(target[key][i]);
    }
    for (var i=0; i<source[key].length; i++) {
      na.push(source[key][i]);
    }
    target[key] = na;
  } else {
    target[key] = source[key];
  }
}

function fsc_get_object_size(obj) {
  if (!obj) {
    return 0;
  }
  var len = JSON.stringify(obj).length;
  return len;
}

function fsc_get_fact_key(key, shared) {
  if (shared) {
    return key;
  } else {
    return _.bindings.flow_id + "-" + key;
  }
} 

function fsc_inject_result(fsc_ctx, key, value) {
  // component instance level results
  if (!fsc_ctx.props[_.bindings.component_label]) {
    fsc_ctx.props[_.bindings.component_label] = {};
  }
  fsc_ctx.props[_.bindings.component_label][key] = value;
}

function fsc_get_flow_session_id(msg) {
  if (msg && msg.flow_session_id) {
    return msg.flow_session_id;
  } else {
    return "";
  }
}

// get fsc context from matching pattern

function fsc_get_context(msg) {
  if (!msg && _.props.message) {
    msg = _.props.message.payload;
  }
  var fsc_ctx = {};
  // from incoming flow message
  if (msg) {
    if (msg.payload) {
      fsc_ctx.payload = msg.payload;
    }
    if (msg.props) {
      fsc_ctx.props = msg.props;
    }
    if (msg.message_id) {
      fsc_ctx.message_id = msg.message_id;
    }
    if (msg.path_id) {
      fsc_ctx.path_id = msg.path_id;
    }
    if (msg.component_in_label) {
      fsc_ctx.component_in_label = msg.component_in_label;
    }
  }
  // from current component
  fsc_ctx.flow_type = _.bindings.flow_type;
  fsc_ctx.flow_id = _.bindings.flow_id;
  if (_.bindings.flow_merged) {
    fsc_ctx.flow_merged = _.bindings.flow_merged;
  } else {
    fsc_ctx.flow_merged = false;
  }
  fsc_ctx.component_type = _.bindings.component_type;
  fsc_ctx.component_label = _.bindings.component_label;
  fsc_ctx.subflow_component_id = _.bindings.subflow_component_id;
  fsc_ctx.subflow_type_id = _.bindings.subflow_type_id;
  // from sheen internals
  fsc_ctx.component_id = _.props.sheenId;
  fsc_ctx.component_name = _.props.spec;
  if (_.props.message) {
    fsc_ctx.flow_hop_counter = _.props.message.payload.flow_hop_counter;
  }
  fsc_ctx.location = _.props.location;
  fsc_ctx.trace_id = _.props.traceId;
  fsc_ctx.span_id = _.props.spanId;
  fsc_ctx.flow_session_id = fsc_get_flow_session_id(msg);
  // user params including flow overrides
  fsc_ctx.user_params = fsc_get_all_user_params(fsc_ctx);
  return fsc_ctx;
}

function fsc_evaluate_string(fsc_ctx, message) {
  var fm = fsc_ctx;
  var props = fsc_ctx.props;
  var payload = fsc_ctx.payload;
  var error = {};
  if (_.props.message.payload.error) {
    error = _.props.message.payload.error;
  }
  while (message.indexOf("{{") >= 0 && message.indexOf("}}") >= 0) {
    var si = message.indexOf("{{");
    var ei = message.indexOf("}}");
    var keyword = message.substring(si+2,ei);
    var retainType = false;
    if (message.length-keyword.length == 4) {
      retainType = true;
    }
    var value = eval(keyword);
    if (retainType) {
      return value;
    } else {
      message = message.substring(0,si) + value + message.substring(ei+2);
    }
  }
  return message;
}

function fsc_evaluate_object_keys(fsc_ctx, obj, cnt) {
  if (!cnt) {
    cnt = 0;
  }
  var fm = fsc_ctx;
  var props = fsc_ctx.props;
  var payload = fsc_ctx.payload;
  var error = {};
  if (_.props.message.payload.error) {
    error = _.props.message.payload.error;
  }
  //fsc_log({"cnt": cnt, "payload": payload, "props": props, "obj": obj});
  for (var key in obj) {
    if (obj.hasOwnProperty(key)) {
      if (typeof obj[key] === "object") {
        fsc_evaluate_object_keys(fsc_ctx, obj[key], cnt+1);
      }
      var evalkey = fsc_evaluate_string(fsc_ctx, key);
      if (typeof evalkey === "string" && evalkey != key) {
        obj[evalkey] = obj[key];
        delete obj[key];
      }
    }
  }
}

function fsc_evaluate_object(fsc_ctx, obj, cnt) {
  if (!cnt) {
    cnt = 0;
  }
  var fm = fsc_ctx;
  var props = fsc_ctx.props;
  var payload = fsc_ctx.payload;
  var error = {};
  if (_.props.message.payload.error) {
    error = _.props.message.payload.error;
  }
  if (typeof obj === "object") {
    for (var key in obj) {
      if (obj.hasOwnProperty(key)) {
        obj[key] = fsc_evaluate_object(fsc_ctx, obj[key], cnt+1);
      }
    } 
  } else if (Array.isArray(obj)) {
    for (var i=0; i<obj.length; i++) {
      obj[i] = fsc_evaluate_object(fsc_ctx, obj[i], cnt+1)
    }
  } else if (typeof obj === "string") {
    obj = fsc_evaluate_string(fsc_ctx, obj);
  }  
 //fsc_log({"cnt": cnt, "payload": payload, "props": props, "obj": obj});
 return obj;
}

function fsc_match_pattern(fsc_ctx, obj, pattern) {
  for (var key in pattern) {
    if (pattern.hasOwnProperty(key)) {
      if (typeof pattern[key] === "object") {
        if (!(typeof obj[key] === "object")) {
          return false;
        }
        if (!fsc_match_pattern(fsc_ctx, obj[key], pattern[key])) {
          return false;
        }
      } else if (Array.isArray(pattern[key])) {
        if (!Array.isArray(obj[key])) {
          return false;
        }
        if (pattern[key].length > obj[key].length) {
          return false;
        }
        for (var i=0; i<pattern[key].length; i++) {
          if (!fsc_match_pattern(fsc_ctx, obj[key][i], pattern[key][i])) {
            return false;
          }
        }
      } else {
        if (pattern[key] != obj[key]) {
          return false;
        }
      }
    }
  }
  return true;
}

// get user param from merged sheen

function fsc_get_user_param_merged(fsc_ctx, key) {
  // alternative would be to have guard move all user params to top level
  var component_label = _.bindings["component_label"];
  if (!_.bindings[component_label]) {
    return "";
  }
  if (!_.bindings[component_label][key]) {
    return _.bindings[component_label][key];
  }
  var fm = fsc_ctx;
  var props = fsc_ctx.props;
  var payload = fsc_ctx.payload;
  var error = {};
  if (_.props.message && _.props.message.payload.error) {
    error = _.props.message.payload.error;
  }
  if (typeof _.bindings[component_label][key] === 'string') {
    if (_.bindings[component_label][key].indexOf("{{") >= 0 && _.bindings[component_label][key].indexOf("}}") >= 0) {
      try {
        var val = fsc_evaluate_string(fsc_ctx, _.bindings[component_label][key]);
        // clone to avoid circular references
        return JSON.parse(JSON.stringify(val));
      } catch (e) {
        fsc_log({"fsc_get_user_param_error" : e.message, "val" : _.bindings[component_label][key]});
      }
    }
  } else if (typeof _.bindings[component_label][key] === 'object' || Array.isArray(_.bindings[component_label][key])) {
    try {
      var val = JSON.parse(JSON.stringify(_.bindings[component_label][key]));
      fsc_evaluate_object(fsc_ctx, val, 0);
      fsc_evaluate_object_keys(fsc_ctx, val, 0);
      return JSON.parse(JSON.stringify(val));
    } catch (e) {
      fsc_log({"fsc_get_user_param_error" : e.message, "val" : _.bindings[component_label][key]});
    }
  }
  return _.bindings[key];
}

// get user param from component sheen

function fsc_get_user_param_simple(fsc_ctx, key) {
  if (!_.bindings[key]) {
    return _.bindings[key];
  }
  var fm = fsc_ctx;
  var props = fsc_ctx.props;
  var payload = fsc_ctx.payload;
  var error = {};
  if (_.props.message && _.props.message.payload.error) {
    error = _.props.message.payload.error;
  }
  if (typeof _.bindings[key] === 'string') {
    if (_.bindings[key].indexOf("{{") >= 0 && _.bindings[key].indexOf("}}") >= 0) {
      try {
        var val = fsc_evaluate_string(fsc_ctx, _.bindings[key]);
        // clone to avoid circular references
        return JSON.parse(JSON.stringify(val));
      } catch (e) {
        fsc_log({"fsc_get_user_param_error" : e.message, "val" : _.bindings[key]});
      }
    }
  } else if (typeof _.bindings[key] === 'object' || Array.isArray(_.bindings[key])) {
    try {
      var val = JSON.parse(JSON.stringify(_.bindings[key]));
      fsc_evaluate_object(fsc_ctx, val, 0);
      fsc_evaluate_object_keys(fsc_ctx, val, 0);
      return JSON.parse(JSON.stringify(val));
    } catch (e) {
      fsc_log({"fsc_get_user_param_error" : e.message, "val" : _.bindings[key]});
    }
  }
  return _.bindings[key];
}


function fsc_get_user_param(fsc_ctx, key) {
  if (fsc_ctx.flow_merged) {
    return fsc_get_user_param_merged(fsc_ctx, key);
  } else {
    return fsc_get_user_param_simple(fsc_ctx, key);
  }
}

// get all user params

function fsc_get_all_user_params_merged(fsc_ctx) {
  var component_label = _.bindings["component_label"];
  var user_param_map = {};
  if (_.bindings[component_label] && _.bindings[component_label].user_params) {
    for (var i = 0; i < _.bindings[component_label].user_params.length; i++) {
      user_param_map[_.bindings[component_label].user_params[i]] = fsc_get_user_param(fsc_ctx, _.bindings[component_label].user_params[i]);
    }
  }
  return user_param_map;
}

function fsc_get_all_user_params_simple(fsc_ctx) {
  var user_param_map = {};
  if (_.bindings.user_params) {
    for (var i = 0; i < _.bindings.user_params.length; i++) {
      user_param_map[_.bindings.user_params[i]] = fsc_get_user_param(fsc_ctx, _.bindings.user_params[i]);
    }
  }
  return user_param_map;
}

function fsc_get_all_user_params(fsc_ctx) {
  if (fsc_ctx.flow_merged) {
    return fsc_get_all_user_params_merged(fsc_ctx);
  } else {
    return fsc_get_all_user_params_simple(fsc_ctx);
  }
}

// get debug payload consisting of user params and variable length list of key value pairs; usage: fsc_get_debug_payload(k1, v1, k2, v2,...)

function fsc_get_debug_payload(fsc_ctx) {
  var debug_payload = {};
  debug_payload["user_params"] = fsc_get_all_user_params(fsc_ctx);
  if (arguments.length>=3 && arguments.length%2==1) {
    for (var i = 0; i < Math.floor(arguments.length/2); i++) {
      debug_payload[arguments[2*i+1]] = arguments[2*i+2];
    }
  }
  return debug_payload;
}

// fsc uuid

function fsc_genid(length) {
  var uid = "";
  var alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  for (var i = 0; i < length; i++) {
    uid += alphabet.charAt(Math.floor(Math.random()*alphabet.length));
	}
  return uid;
}

// emit debug message

function fsc_emit_debug_event(message) {
  if (_.bindings.debug) {
    message.component_out_label = _.bindings["?component_out_label"];
    message.component_in_label = _.bindings["?component_in_label"];
    fsc_kafka(fsc_sanitize(message), fsc_get_debug_topic());
  }
}

// send basher event

function fsc_emit_basher_event(message, partition) {
  var topic = fsc_get_basher_topic(message.runtime_env);
  fsc_log({"info" : "basher", "topic" : topic, "partition" : partition});
  return fsc_kafka_with_reply(message, topic, partition);
}

// make basher message

function fsc_escape_source(source) {
  //source = source.replace("\n","\\n");
  return source;
}

function fsc_make_basher_msg (fsc_ctx, action, job_id) {
  var runtime_env = fsc_ctx.user_params.runtime_env;
  if (!runtime_env) {
    runtime_env = "default";
  }
  var msg = {
    "script_type" : fsc_ctx.user_params.script_type,
    "script_name" : fsc_evaluate_string(fsc_ctx, fsc_ctx.user_params.script_name),
    "credentials" : fsc_ctx.user_params.credentials,
    "script_source" : fsc_escape_source(fsc_ctx.user_params.script_source),
    "cmd_params" : fsc_ctx.user_params.cmd_params,
    "runtime_env" : runtime_env,
    "remote_host" : fsc_ctx.user_params.remote_host,
    "remote_user" : fsc_ctx.user_params.remote_user,
    "remote_port" : fsc_ctx.user_params.remote_port,
    "ssh_key" : fsc_ctx.user_params.ssh_key,
    "app_id" :  fsc_get_app_id(),
    "payload" : fsc_ctx.payload,
    "props" : fsc_ctx.props,
    "action" : action,
    "job_id" : job_id,
    "debug" : _.bindings.debug,
    "location" : fsc_ctx.location,
    "flow_id" : fsc_ctx.flow_id,
    "flow_name" : fsc_ctx.flow_type,
    "component_label" : fsc_ctx.component_label,
    "include_output" : fsc_ctx.user_params.include_output
  };
  return msg;
}

// send kafka message

function fsc_kafka_with_reply(message, topic, partition, location, appid) {
  var replyTo = {
        "type": "eel",
        "endpoint": fsc_get_eel_endpoint(),
        "app": _.props.app,
        "location": _.props.location,
        "receiptHandle": _.props.sheenId
      };
  var to = {type: "kafka", topic: topic};
  if (partition) {
    to.partition = partition;
  }
  if (location) {
    to.location = location;
  }
  if (appid) {
    to.app = appid;
  }
  var span_id = _send({"payload": message, "op": "process"}, to, replyTo);
  //var span_id = _send({"payload": JSON.parse(JSON.stringify(message))}, {type: "kafka", topic: topic}, replyTo);
  fsc_log({"info" : "emitting_reply_event", "topic" : topic, "partition" : partition, "message" : message, "span_id" : span_id, "endpoint" : fsc_get_eel_endpoint()});
  return span_id;
}

function fsc_kafka(message, topic, partition, location, appid) {
  //var span_id = _send({"payload": JSON.parse(JSON.stringify(message))}, {type: "kafka", topic: topic});
  var to = {type: "kafka", topic: topic};
  if (partition) {
    to.partition = partition;
  }
  if (location) {
    to.location = location;
  }
  if (appid) {
    to.app = appid;
  }
  var span_id = _send({"payload": message, "op": "process"}, to);
  fsc_log({"info" : "emitting_event", "topic" : topic, "partition" : partition, "message" : message, "span_id" : span_id});
  return span_id;
}

// retain a flow message before doing something async

function fsc_get_context_key() {
  return _.props.spanId+"_"+_.props.sheenId;
}

function fsc_retain_message(key) {
  if (!key) {
    key = fsc_get_context_key();
  }
  if (!_.bindings.fsc_msgs) {
    _.bindings.fsc_msgs = {};
  }
  _.bindings.fsc_msgs[key] = JSON.parse(JSON.stringify(_.props.message.payload));
}

// restore a flow message after something async

function fsc_restore_message(key) {
  if (!key) {
    key = fsc_get_context_key();
  }
  if (!_.bindings.fsc_msgs || !_.bindings.fsc_msgs[key]) {
    return;
  }
  var msg = _.bindings.fsc_msgs[key];
  _.props.message.payload.flow_hop_counter = msg.flow_hop_counter;
  //_.props.message.payload = msg;
  return msg;
}

function fsc_remove_message(key) {
  if (!key) {
    key = fsc_get_context_key();
  }
  if (!_.bindings.fsc_msgs) {
    return;
  }
  delete _.bindings.fsc_msgs[key];
}

// send http request

function fsc_http(request) {
  http(request);
  /*var message =
    {
      replyTo: {
        receiptHandle: _.props.sheenId
      },
      message: request
    };
    out(message, "http");*/
}

// create elasticron timer

function fsc_create_elasticron_timer(payload, timer_id, schedule, timezone, zipcode, latitude, longitude, resolution) {
  elasticron("create", schedule, payload, timer_id, _.props.sheenId, timezone, zipcode, latitude, longitude, resolution);
}

// create elasticron timer

function fsc_delete_elasticron_timer(timer_id) {
  elasticron("delete", "", "", timer_id);
}

// create timer

function fsc_create_timer(payload, timer_id, timeout) {
  timer(timeout, payload, timer_id, _.props.sheenId);
  /*var message =
    {
      to: {
        receiptHandle: _.props.sheenId
      },
      message: {
        op: "process",
        payload: payload
      },
      tx: {
        traceId: _.props.traceId,
        spanId: _.props.spanId
      }
    };
    out({makeTimer: {id: timer_id, in: timeout, message: message}}, "timers");*/
}

// delete timer

function fsc_delete_timer(timer_id) {
  out({deleteTimer: timer_id}, "timers", _.props.sheenId);
}

// create new flow message

function fsc_make_flow_msg(fsc_ctx, debug, error) {
  var msg = {
    "debug" : debug,
    "timestamp" : Date.now(),
    "payload" : fsc_ctx.payload,
    "props" : fsc_ctx.props,
    "message_id" : fsc_ctx.message_id,
    "path_id" : fsc_ctx.path_id,
    "flow_type" : fsc_ctx.flow_type,
    "flow_id" : fsc_ctx.flow_id,
    "app_id" : fsc_get_app_id(),
    "component_type" : fsc_ctx.component_type,
    "component_label" : fsc_ctx.component_label,
    "component_id" : fsc_ctx.component_id,
    "flow_hop_counter" : fsc_ctx.flow_hop_counter,
    "location" : fsc_ctx.location,
    "trace_id" : fsc_ctx.trace_id,
    "span_id" : fsc_ctx.span_id,
    "flow_session_id" : fsc_ctx.flow_session_id,
    "subflow_component_id" : fsc_ctx.subflow_component_id,
    "subflow_type_id" : fsc_ctx.subflow_type_id
  };
  if (_.props.message.payload.error) {
    msg.error = _.props.message.payload.error;
  }
  // new error overwrites earlier error
  if (error) {
    msg.error = error;
  }
  return msg;
}

// compose message and send it to target sheens, also manage message counter

function fsc_fanout(fsc_ctx, component_out_label, outchan, debug, error) {
  if (!outchan) {
    return;
  }
  // do not add debug info to flow message unless debug is turned on
  if (!_.bindings.debug) {
    debug = {};
  }
  if (!_.props.message.payload.flow_hop_counter) {
    _.props.message.payload.flow_hop_counter = 0;
    fsc_ctx.flow_hop_counter = 0;
  }
  fsc_ctx.flow_hop_counter = fsc_ctx.flow_hop_counter + 1;
  if (fsc_get_object_size(fsc_ctx.props) > FSC_MAX_MSG_SIZE) {
    // drop message if it is too fat
    //fsc_ctx.props = {};
    fsc_log({"info":"dropping_message_too_large_props"});
    fsc_handle_exception (fsc_ctx, "dropping message because props object exceeds size limit of " + FSC_MAX_MSG_SIZE + " bytes", "flows", {}, debug);
  } else if (fsc_get_object_size(fsc_ctx.payload) > FSC_MAX_MSG_SIZE) {
    // drop message if it is too fat
    //fsc_ctx.payload = {};
    fsc_log({"info":"dropping_message_too_large_payload"});
    fsc_handle_exception (fsc_ctx, "dropping message because payload object exceeds size limit of " + FSC_MAX_MSG_SIZE + " bytes", "flows", {}, debug);
  } else if (_.props.message.payload.flow_hop_counter > FSC_MAX_HOPS) {
    // drop message if it has been traversing the flow for more than MAX hops
    fsc_log({"info":"dropping_message_too_many_hops"});
    fsc_handle_exception (fsc_ctx, "dropping message after " + FSC_MAX_HOPS + " hops", "flows", {}, debug);
  } else {
    var msg = fsc_make_flow_msg(fsc_ctx, debug, error);
    for (var i = 0; i < outchan.target_mids.length; i++) {
      // compose flow message
      msg.component_out_label = component_out_label;
      msg.component_in_label = outchan.component_in_labels[i];
      msg.path_id = outchan.path_id;
      // if this is a merged sheen send it to urself
      if (fsc_ctx.flow_merged) {
        out(msg, _.props.sheenId);
      } else {
        out(msg, outchan.target_mids[i]);
      }
    }
  }
}

// handle generic sheen error state

function fsc_handle_error (fsc_ctx) {
  if (!fsc_ctx) {
    fsc_ctx = fsc_get_context();
  }
  var debug_payload = fsc_get_debug_payload(fsc_ctx);
  fsc_handle_exception(fsc_ctx, _.bindings.error, "sheen_error", { "action_error" : _.bindings.actionError, "last_node" : _.bindings.lastNode }, debug_payload);
  // delete all state information in bindings but retain initial bindings
  fsc_delete_bindings();
  fsc_delete_error_bindings();
}

function fsc_sanitize(o) {
  if(!o) {
    return o;
  }
  var str = JSON.stringify(o);
  try {
    return JSON.parse(str);
  } catch(e) {
    fsc_log({"sanitize_error": str});
    return str;
  }
}

// handle java script exception

function fsc_handle_exception (fsc_ctx, message, type, details, debug) {
  fsc_metric(fsc_ctx, "FlowTriggerError", message)
  if (!_.props.message.payload.flow_hop_counter) {
    _.props.message.payload.flow_hop_counter = 0;
    fsc_ctx.flow_hop_counter = 0;
  }
  fsc_ctx.flow_hop_counter = fsc_ctx.flow_hop_counter + 1;
  if (_.props.message.payload.flow_hop_counter < FSC_MAX_HOPS) {
    var error = {
      "message" : message,
      "type" : type,
      "details" : details
    };
    var dpl;
    if (debug) {
      dpl = debug;
    } else {
      dpl = fsc_get_debug_payload(fsc_ctx);
    }
    fsc_log({"error": error});
    if (_.bindings.err) {
      var outchan = _.bindings.err;
      fsc_ctx.props = fsc_sanitize(fsc_ctx.props);
      var msg = fsc_make_flow_msg(fsc_ctx, dpl, error);
      for (var i = 0; i < outchan.target_mids.length; i++) {
        // compose flow message
        msg.component_out_label = "err";
        msg.component_in_label = outchan.component_in_labels[i];
        out(msg, outchan.target_mids[i]);
      }
    }
  }
}

// check if incoming message is a flow messahe

function fsc_is_flow_msg (msg) {
  if (!msg.flows_id) {
    _fsc.log({"flow_msg_error" : "missing flow id"});
    return false;
  }
  if (!msg.flow_type) {
    _fsc.log({"flow_msg_error" : "missing flow type"});
    return false;
  }
  if (!msg.component_id) {
    _fsc.log({"flow_msg_error" : "missing component id"});
    return false;
  }
  if (!msg.component_type) {
    _fsc.log({"flow_msg_error" : "missing component type"});
    return false;
  }
  if (!msg.message_id) {
    _fsc.log({"flow_msg_error" : "missing message id"});
    return false;
  }
  if (!msg.path_id) {
    _fsc.log({"flow_msg_error" : "missing path id"});
    return false;
  }
  if (!msg.payload) {
    _fsc.log({"flow_msg_error" : "missing payload"});
    return false;
  }
  if (!msg.props) {
    _fsc.log({"flow_msg_error" : "missing props"});
    return false;
  }
  return true;
}

function fsc_metric (fsc_ctx, statKey, message) {
  if (fsc_ctx.location && fsc_ctx.flow_id && fsc_ctx.component_type) {
	var m = {
		"stat.key": statKey, 
		location: fsc_ctx.location, 
		flowId: fsc_ctx.flow_id, 
		componentType: fsc_ctx.component_type,
	};
    if (message) {
    	m.message = message;
      _.metric(m);
    } else {
      _.metric(m);
    }
  }
}

// emit JSON log line

function fsc_log (data) {
  if (_.bindings.log || _.bindings.debug) {
    // inject flow meta data
    data["log"] = "DEBUG";
    if (_.bindings.component_type) {
      data["action"] = _.bindings.component_type;
    }
    if (_.bindings.flow_type) {
      data["flow_type"] = _.bindings.flow_type;
    }
    if (_.bindings.flow_type) {
      data["flow_id"] = _.bindings.flow_type;
    }
    if (_.bindings.component_type) {
      data["component_type"] = _.bindings.component_type;
    }
    if (_.bindings.component_id) {
      data["component_id"] = _.bindings.component_id;
    }
    if (_.bindings.component_label) {
      data["component_label"] = _.bindings.component_label;
    }
    if (_.props.location) {
      data["location"] = _.props.location;
    }
    if (_.props.sheenId) {
      data["sheen_id"] = _.props.sheenId;
    }
    if (_.props.traceId) {
      data["trace_id"] = _.props.traceId;
    }
    if (_.props.spanId) {
      data["span_id"] = _.props.spanId;
    }
    if (_.props.message && _.props.message.payload && _.props.message.payload.flow_hop_counter) {
      data["hop_counter"] = _.props.message.payload.flow_hop_counter;
    }
    if (_.bindings.subflow_component_id) {
      data["subflow_component_id"] = _.bindings.subflow_component_id;
    }
    if (_.bindings.subflow_type_id) {
      data["subflow_type_id"] = _.bindings.subflow_type_id;
    }
    _.log(data);
  }
}

// change debug setting

function fsc_debug (on) {
  _.bindings.debug = on;
}

// change log setting

function fsc_set_log (on) {
  _.bindings.log = on;
}

// delete flow meta data bindings

function fsc_delete_bindings () {
  delete _.bindings["?message_id"];
  delete _.bindings["?flow_type"];
  delete _.bindings["?flow_id"];
  delete _.bindings["?component_type"];
  delete _.bindings["?component_id"];
  delete _.bindings["?component_label"];
  delete _.bindings["?component_in_label"];
  delete _.bindings["?component_out_label"];
  delete _.bindings["?path_id"];
  delete _.bindings["?payload"];
  delete _.bindings["?props"];
}

function fsc_delete_all_bindings () {
  for (var key in _.bindings) {
    if (key.slice(0, 1) === "?") {
      delete _.bindings[key];
    }
  }
}

// delete sheen error bindings

function fsc_delete_error_bindings () {
  delete _.bindings.actionError;
  delete _.bindings.error;
  delete _.bindings.lastBindings;
  delete _.bindings.lastNode;
}
