# Little Gears

Little gears will wrap on top of [little sheens](https://github.com/Comcast/littlesheens/) and provide C apis to interact with Zilker for enhanced functionalities below.

## Multi-tenant with app header

### TODO

Create machines by tenant.
Process messages by tenant.

## Libraries (javascript) with common interface to be used in specs

Gears uses the following libraries:
- util.js
- fsc.js
- moment.js
- moment-timezone.js
- moment-timezone-with-data.js
- zipcodes.js
- tz.js
- suncalc.js

They can be found [here](https://github.comcast.com/Gears/app-data/tree/dev/lib).  Most of them are LittleSheens compatible except `util.js`.  `util.js` sends messages with functions below.  They should be replaced with local implementation.

- func raptor()        // for devices query/control
- func elasticron()    // for crontab like scheduling
- func nae()           // for notifications (email, sms, push, x1)
- func timer()         // for internal timers
- func out()           // for internal messages b/w sheens

Added `mach_set_lib_provider` to load bundled javascript libraries from local resources.

## Common sheen engine implementation

### Propagate common step properties

* location             (account identifier)
* app                  (application, use `xh` for digital home)
* sheenId
* traceId
* spanId
* spec                 (spec name)
* status               (http like status code for response message)
* message              (input message)
* reply                (handle for replying to message sender in `out` function above)

Updated `CrewProcess (driver.js)` to construct step properties and propagate step properties.

### Implement start / end nodes

`Start` node will be executed when sheen is deployed and `End` node will be executed when sheen is deleted.

Updated `SetMachine (driver.js)` and `RemMachine (driver.js)` to inspect specs and do `CrewProcess (driver.js)` accordingly.

### Implement internal message routing

little sheen needs to route messages by sheen id without broadcasting. Gears use [out](https://github.comcast.com/Gears/app-data/blob/dev/lib/util.js#L150-L167) function in `util.js` to deliver messages.

Updated `CrewProcess (driver.js)` to recursively run emitted internal messages. 

### Implement requires

Updated `step (step.js)` to use `lib_provider` to concat libraries specified in `requires`.

### Implement spec inheritance

Gears implement the following logic to achieve spec inheritance to share common logic:

Updated `GetSpec (driver.js)` to imeplement spec inheritance.

```
name: trigger
uses:
- base
doc: |-
  Trigger component.
paramspecs:
  out:
    doc: out channel description
    primitiveType: out
    required: true
parsepatterns: true
patternsyntax: json
nodes:    
  forward:
    action:
      interpreter: goja
      source:
        requires:
        - util.js
        - fsc.js
        code: |-
          // trigger component has to initiate flow message
          ...
          // cleanup
          ...
          return _.bindings;
    branching:
      branches:
      - target: listen
```

- uses: spec is merged with other specs in the uses field according to their order.  Missing fields are added to the spec while existing fields are merged
- paramspecs: Missing params are added while existing params remain unchanged
- nodes: Missing nodes are added.  Existing nodes’ branches are merged.

![](ComponentHierarchy.png)

### Implement response model

Updated the payload in `CrewProcess (driver.js)` to the following if op is `response`.

```
{
  "_type_":  "response",
  "status":  "<status>",
  "payload": "<payload>"
}
```

## Mechanism to transform inbound and outbound messages with common schema

Need to define common schema to be shared b/w cloud and local. Current message format used by Gears is [here](https://github.comcast.com/Gears/home/blob/master/docs/messages/README.md#message).

#### TODO

Update `CrewProcess (driver.js)` to transform inbound/outbound messages according to common schema agreed.

## Mechanism to persist states

#### TODO

Caller will get updated crew from `mach_crew_update` to persist state changes in either flash or local persistence.

## C apis

[machines.h](https://github.com/Comcast/littlesheens/blob/dev-mlu/machines.h)

A set of new or updated apis as followings:

```
/* mach_set_machine adds or updates a machine in the given Crew.
   Writes a map from machine ids to steppeds as JSON to dst. */
int mach_set_machine(JSON crew, S id, S specRef, JSON bindings, S node, JSON dst, size_t limit);

/* mach_rem_machine removes the given machine from the given Crew.
   Writes a map from machine ids to steppeds as JSON to dst. */
int mach_rem_machine(JSON crew, S id, JSON dst, size_t limit) ;

/* mach_set_lib_provider registers the given function so that
   mach_process can resolve the LibName to a Lib.  Available modes:
   MACH_FREE_FOR_PROVIDER, which will cause the string returned by the
   provider to be freed.  Use 0 if you don't want that. */
void mach_set_lib_provider(void * ctx, mach_provider f, mach_mode m);

/* mach_set_lib_cache sets the lib cache entries limit. */
int mach_set_lib_cache_limit(int limit) ;

/* mach_clear_lib_cache clears the lib cache. */
int mach_clear_lib_cache() ;

/* mach_enable_lib_cache enables (1) or disables (0) the lib cache. */
int mach_enable_lib_cache(int enable) ;
```