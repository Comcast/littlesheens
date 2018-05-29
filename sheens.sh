#!/bin/bash

(for I in `seq 1000`; do echo "{\"double\":$I}"; echo '{"input":"push"}'; echo '{"input":"coin"}'; done) > sheens.input

cat<<EOF > crew.js
{"id":"simpsons",
 "machines":{
   "doubler":{"spec":"double","node":"listen","bs":{"count":0}},
   "turnstile":{"spec":"turnstile","node":"locked","bs":{}}}}
EOF

make sheens && \
    time (cat sheens.input | \
		 ./sheens -c -p | \
		 tee sheens.output | \
		 wc -l) && \
    tail -5 sheens.output
