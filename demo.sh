#!/bin/bash

set -e

make specs/double.js specs/turnstile.js sheensio

cat<<EOF > crew.json
{"id":"simpsons",
 "machines":{
   "doubler":{"spec":"specs/double.js","node":"listen","bs":{"count":0}},
   "turnstile":{"spec":"specs/turnstile.js","node":"locked","bs":{}}}}
EOF

cat<<EOF | ./sheensio
{"double":1}
{"double":10}
{"double":100}
{"input":"push"}
{"input":"coin"}
{"input":"coin"}
{"input":"push"}
EOF

echo "done"
