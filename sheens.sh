#!/bin/bash

set -e

(for I in `seq 1000`; do echo "{\"double\":$I}"; echo '{"input":"push"}'; echo '{"input":"coin"}'; done) > sheens.input

cat<<EOF > crew.js
{"id":"simpsons",
 "machines":{
   "doubler":{"spec":"double","node":"listen","bs":{"count":0}},
   "turnstile":{"spec":"turnstile","node":"locked","bs":{}}}}
EOF

echo "Processing $(cat sheens.input | wc -l) messages."
echo "Using profiling and spec cache."

make sheens && time (cat sheens.input | ./sheens -c -p > sheens.output)

echo "Emitted $(cat sheens.output | grep -e '^out' | wc -l) messages."

tail -3 sheens.output
