#!/bin/bash

N=${1:-100}

time (yes '{"double":1}' | head -$N | ./sheens > sheens-perf.log 2>&1)

echo "logged  $(wc -l sheens-perf.log)"
echo "emitted $(grep '"doubled":' sheens-perf.log | wc -l)"



