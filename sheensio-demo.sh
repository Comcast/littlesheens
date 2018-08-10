#!/bin/bash

# Demo use of sheensio.

N=${1:-1000}

set -e

make sheensio

(for I in `seq $N`; do echo "{\"double\":$I}"; echo '{"input":"push"}'; echo '{"input":"coin"}'; done) > sheensio-demo.input

cat<<EOF > crew.json
{"id":"simpsons",
 "machines":{
   "doubler":{"spec":"double","node":"listen","bs":{"count":0}},
   "turnstile":{"spec":"turnstile","node":"locked","bs":{}}}}
EOF

echo "Processing $(cat sheensio-demo.input | wc -l) messages."
echo "Using profiling and spec cache."

time (cat sheensio-demo.input | ./sheensio -c -p > sheensio-demo.output)

echo "Emitted $(cat sheensio-demo.output | grep -e '^out' | wc -l) messages."

tail -3 sheensio-demo.output
