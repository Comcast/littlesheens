#!/bin/bash

set -e

make specs sheensio

cat<<EOF > crew.json
{
  "id": "littlesheen",
  "machines": {
    "862f2c70.26d6": {
      "spec": "door",
      "node": "listen",
      "bs": {
        "component_id": "862f2c70.26d6",
        "component_label": "Door",
        "component_type": "door",
        "debug": false,
        "door_ids": [],
        "door_state": [],
        "err": {
          "component_in_labels": [],
          "path_id": "",
          "target_mids": []
        },
        "flow_id": "Flow1",
        "flow_merged": false,
        "flow_type": "Flow1",
        "funcional_type": "action",
        "in": "p_door-862f2c70.26d6_out_5958b31835ac4952",
        "log": false,
        "out": {
          "component_in_labels": [
            "in"
          ],
          "path_id": "p_door-862f2c70.26d6_out_5958b31835ac4952",
          "target_mids": [
            "b0eb124.3158ff"
          ]
        },
        "subflow_component_id": "",
        "subflow_type_id": "",
        "user_params": [
          "door_ids",
          "door_state"
        ]
      }
    },
    "b0eb124.3158ff": {
      "spec": "light",
      "node": "listen",
      "bs": {
        "component_id": "b0eb124.3158ff",
        "component_label": "Light",
        "component_type": "light",
        "debug": false,
        "err": {
          "component_in_labels": [],
          "path_id": "",
          "target_mids": []
        },
        "flow_id": "Flow1",
        "flow_merged": false,
        "flow_type": "Flow1",
        "funcional_type": "action",
        "log": false,
        "out": {
          "component_in_labels": [],
          "path_id": "",
          "target_mids": []
        },
        "subflow_component_id": "",
        "subflow_type_id": "",
        "user_params": [
          "retries",
          "delay",
          "deadline",
          "light_ids",
          "light_state",
          "dim_level"
        ],
        "deadline": "60s",
        "delay": "3s",
        "light_ids": [],
        "light_state": "on",
        "retries": 0,
        "timers": {}
      }
    },
    "s_731281ab9abd41e2": {
      "spec": "flow_meta_data",
      "node": "start",
      "bs": {
        "component_id": "s_731281ab9abd41e2",
        "component_label": "FlowMetaData",
        "component_type": "flow_meta_data",
        "debug": false,
        "flow_id": "Flow1",
        "flow_type": "Flow1",
        "funcional_type": "metadata",
        "log": false,
        "user_params": [],
        "flow": "eJzMV0lv6zYQ/isPc1aDxIlTP116KXrq7R16CAyCksYSAYpkudgOAv/3got2pXlO3nYzh9TMN98sX/ICFZpSM2WZFJADZMBlSdOJM2s5mgZRQAaVLOMLQVuEHP7i8nTnP6AF8nT+5A0HLk/EPqvxo2A7ojZ9nGBh1ehNKVslBQprIH96AdRaav9zny1R9mAqVBpLarGC/EC5wQzwTFvFcQK2klKPsP4Zj5bWPgA0skXYZxDg7B43h035++3N5rF6hAzOkN/vbjN4hnzzcOthcqm74IWrIbfaoSeu7iG8SsEk4chsBHfzTFve2cgqkOTPalbX6OE3rKq8gxTUuIIMQSAYwrkndnHjXfZGZkiy9y6pUuH6Cc5NyPdAHbeeKups09HAhHKWBG5TvaSzE5M3QOZLCsNtKYXA0tc0FlxR20QwinhKfpsQQKSzZPt5uyvu73b3W1o+fN5uYBoK8hRohAhyYCLUWteYGChusbjbPNzc3213hwNcBkxKaptyiD5GBkU1bdGijnBHnUVYZSCDI+UOIReO875axmom6qc9ZKDxX8e0b9TYMH0PHylnFQlfm+7zvonjsWM+IjPEGdQkAOq8tY5bpjgSgzyxSijnJzmaDMeIxbONUf0hYIwRHCOLKWupUkzU/nDJphkbSy1+s5yfQKqwZEouDVa+Sa4gIGX3vRkIPcEso5wUTFRM+OXxArPpmk/tcD1bPsNFoi3tqMlS6bsrrsGB+XD285SPB2filonJTKYBaJO3y9pGWttbTFw5keNF6Mcxf+9or6fiAS3SmY+0T++q9XfJYGipUNcx9R1Zs9u+FJcMNF1ch48hzMH4ubdcxq34T8PK5pN/8se4K6PCHFmJpJUV8s5cSmFpab+gMKFh1vrWHOv4o7Et7+a3V1TQVFkZdvGHlZWzurEjaf07nVe1dValIK7bTlwfv4u4Bnwr6rqAkjzSsDl+XXVN7f8Ohf2IygUWf67M/aAtn81yflvorpI531nSL6jXc4+Pfnb2FWsJxyPyV3IXri3CX6FD7in+uwoej7+ysC8WxlLZu923kPZuSU522g9R72Fwg5txT6dG+1+5vg7SxxR3RWT7HkyVfCubpQ4PHmDKRtTlNQ9zYQ5vvkqZw8svJ2bL5qt1eT9ISyIBBS34m63dHeJ/g4SVXQjHSNSwqW0G5fIfAAAA//8BAAD//4RpWYo=",
        "flow_nr": "eJyskD9PwzAQR7/Lbz5VadqG6mZGRrYoihzsgCU3tvwHKFW/O2pc00KZEJvz5NzzvfYALcHYNvVYP91Vi7qRDQhx7xQY0loPwgcYIExid4L3GdoUXYoBXBHewattRdiD63VFeNNeBXDbYqjUsKzXi9Vysx1HrtBR23UEJ7zYqah8AB9mT69lAKPtQPk7RBFVJkeCnkabn2HEoEw+Sh3EYJQEj8IERdChT0H5/mv8abpOrr8Yok+ni4WdLfP/swY8JWPm/crRib2xQj7mKDjSudr35S7ZjH5+iTfdHs70R7hNCddch/s9k971Rr2W7WfNdbYMSjc7/Ue3i7JcTa6/Mt/QvwTtPgEAAP//AQAA//9qXM/H",
        "functional_type": "metadata"
      }
    }
  }
}
EOF

rm -f nohup.out
rm sheensio.fifo
mkfifo sheensio.fifo
nohup ./sheensio <sheensio.fifo &

#for (( c=0; c<=10000; c++ ))
for (( ; ; ))
do 

cat<<EOF >sheensio.fifo
{"deviceId":"ic_111.0","id":"175632.1519236108","meta":{"friendlyName":"Door 1","id":"ic_111.0","iot:capabilities":["contactSensor"]},"metadata":{"deviceType":"zoneDoor","deviceid":"ic_111.0","eventType":"","group":"xh","instanceName":"Door 1","manufacturer":"xh","name":"xh","siteid":"175632","timestamp":1519251394062},"properties":{"contactSensor.attachedTo":"door","contactSensor.state":"open","state":"true"},"siteid":"175632","ts":1519251394062}
EOF

done

echo >sheensio.fifo

echo "done"
