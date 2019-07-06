// parse_duration parse str per golang duration syntax and return duration in milliseconds
function parse_duration(s) {
	if (!s) {
		return 0;
	}
	
	if (s == "0") {
		return 0;
	}
	
	if (s == "") {
		return 0;// errors.New("time: invalid duration " + orig)
	}
	
	var d = 0;
	while (s != "") {
		// The next character must be [0-9.]
		if (!('0' <= s[0] && s[0] <= '9')) {
			return 0;// errors.New("time: invalid duration " + orig)
		}
		
		// Consume [0-9]*
		var v = 0, i = 0;
		for (; i < s.length; i++) {
			var c = s[i];
			if (c < '0' || c > '9') {
				break;
			}
			if (v > (1<<63-1)/10) {
				// overflow
				return 0;// "", errLeadingInt
			}
			v = v*10 + c - '0';
			if (v < 0) {
				// overflow
				return 0;// "", errLeadingInt
			}
		}
		s = s.substring(i);
		
		// Consume unit.
		for (i = 0; i < s.length; i++) {
			var c = s[i];
			if ('0' <= c && c <= '9') {
				break;
			}
		}
		if (i == 0) {
			return 0;// errors.New("time: missing unit in duration " + orig)
		}
		
		var u = s.substring(0, i);
		s = s.substring(i);
		
		var unit = 1;
		if (u == "h") {
			unit = 60 * 60 * 1000;
		} else if (u == "m") {
			unit = 60 * 1000;
		} else if (u == "s") {
			unit = 1000;
		} else if (u != "ms") {
			return 0;// errors.New("time: unit not supported")	
		}
		
		if (v > (1<<63-1)/unit) {
			// overflow
			return 0;// errors.New("time: invalid duration " + orig)
		}
		
		d += v*unit;
		if (d < 0) {
			// overflow
			return 0;// errors.New("time: invalid duration " + orig)
		}
	}
	
	return d;
}

function timer(schedule, message, id, target) {
  if (!id) {
    id = _.props.sheenId;
  }

  if (!target) {
    target = _.props.sheenId;
  }
    
  var msg = {
    "makeTimer": {
      "id": id,
      "in": schedule,
      "message": {
        "to": {
          "receiptHandle": target
        },
		"message": {
          "op": "process",
          "payload": message
		},
        "tx": {
          "traceId": _.props.traceId,
          "spanId": _.props.spanId
        }
      }
    }
  };

  var step;
  return out(msg, "timers", _.props.sheenId, step, true);
}

function http(request) {
  var step;
  return out(request, "http", _.props.sheenId, step, true);
}

// elasticron sends a request to proper elasticron topic to post a message back later
//
// schedule: schedule for the job
//
// message: message to be posted back to sheens when the job fires
//
// This function does no error-checking.
function elasticron(op, schedule, message, id, target, timezone, zipcode, latitude, longitude, resolution) {
  var to = {
    "type": "kafka",
    "topic": "{{CRONTOPIC}}",
    "app" : _.props.app,
    "location" : _.props.location
  };
  if (!id) {
    id = _.props.sheenId;
  }
  var msg = {};
  if ("delete" == op) {
      msg = {
        "op": "delete",
        "payload": {
          "id": id    
        }  
      };
  } else {
      if (!target) {
        target = _.props.sheenId;
      }      
      msg = {
        "op": "update",
        "payload": {
          "id": id,
          "job": {
            "schedule": schedule,
            "url": "{{EELURL}}",
            "method": "POST",
            "header": {
              "Application-Id": _.props.app
            },
            "body": JSON.stringify({
              "to": {
                "app": _.props.app,
                "location": _.props.location,
                "receiptHandle": target
              },
              "message": {
                "op": "process",
                "payload": message
              }
            }),
            "tag": _.props.location
          }
        }
      };
      if (timezone) {
        msg.payload.job.timezone = timezone;
      }
      if (zipcode) {
        msg.payload.job.location = { "zipcode" : zipcode };
      }
      if (latitude && longitude) {
        msg.payload.job.location = { "latitude" : latitude, "longitude" : longitude };
      }
      if (resolution) {
        msg.payload.job.resolution = resolution;
      }
  } 
  return _send(msg, to, replyTo(), 5000, true);
}

// nae sends a request to proper nae topic
//
// code: code to be sent
//
// This function does no error-checking.
function nae(code) {
  var to = {
    "app": _.props.app,
    "type": "kafka",
    "topic":"{{NAETOPIC}}"
  };
	
  var msg = {
	  "op": "notify",
	  "payload": code
  };
  
  return _send(msg, to, replyTo(), 5000, true);
}

// raptor sends a request to proper raptor topic
//
// op: operation type.  valid value is 'fact' or 'action'
//
// code: code to be sent
//
// This function does no error-checking.
function raptor(op, code) {
  var to = {
    "type": "kafka",
    "topic": "{{RAPTORTOPIC}}"
  };	

  var msg = {
	  "op": op,
	  "payload": code
  };
  
  return _send(msg, to, replyTo(), 15000, true);
}

function replyTo() {
  return {
    "type": "eel",
    "endpoint": "{{EELURL}}", 
    "app": _.props.app,
    "location": _.props.location,
    "receiptHandle": _.props.sheenId
  };
}

// out sends a message to the current crew or to the specified sheenId
//
// message: message to be sent
//
// sheenId: optional sheenId to send the message to
//
// step: optional step to the reply message 
//
// This function does no error-checking.
function out(message, target, replyTo, step, sameSpan) {
    var msg = {
        "op": "process",
        "payload": message
    };
    
    var toObj, replyToObj, ttl;

    if (target) {
      toObj = {"receiptHandle": target};
    } else {
      toObj = {};	
    }

    if (replyTo) {
      if (step) {
        replyToObj = {"receiptHandle": {"sheenId": replyTo, "step": step}};
      } else {
        replyToObj = {"receiptHandle": replyTo};
      }
    }
    
    return _send(msg, toObj, replyToObj, ttl, sameSpan);
}

// _send sends a message per "to" field
//
// message: message to be sent
//
// to: where to send the message.  It supports the follow fields:
//   topic: kafka topic
//   receiptHandle: sheenId
//
// replyTo: whether message consumer should send a reply
//
// This function does no error-checking.
function _send(message, to, replyTo, ttl, sameSpan) {
  var spanId = _.props.spanId;
  if (!sameSpan || "" == spanId) {
    spanId = _.gensym();
  }

  var msg = {
    "message": message,
    "tx": {
      "traceId": _.props.traceId,
      "spanId": spanId
    }
  };

  if (to) {
    msg.to = to;
  }
	
  if (replyTo) {
    msg.replyTo = replyTo;
  }
  
  if (ttl) {
    msg.expires = Date.now() + ttl;  
  }

//  _.log(JSON.stringify(msg));

  _.out(msg);
  
  return spanId;
}

function double(x) {
	return x*2;
}
