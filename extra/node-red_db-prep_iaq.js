// from https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/node-red_db-prep_iaq.js
// payload is an array; [0] is a table of values, [1] is a table of tags.
// Per influxDB design, all values written are tagged with the tags given
// by [1], and therefore can be selected based on those tags.

var result =
{
    payload:
[{
        msgID: msg._msgid,
        counter: msg.counter,
        //time: new Date(msg.metadata.time).getTime(),
},
{
    devEUI: msg.hardware_serial,
    devID: msg.dev_id,
    displayName: msg.display_id,
    displayKey: msg.display_key,
    nodeType: msg.local.nodeType,
    platformType: msg.local.platformType,
    radioType: msg.local.radioType,
    applicationName: msg.local.applicationName,
}]
};

var t = result.payload[0];
var tags = result.payload[1];

// copy the fields we want as values to the database slot 0.
var value_keys = [
            "vBat", "boot", "t", "tDew", "p", "p0", "rh", "lux",
            "powerUsed", "powerRev", "powerUsedDeriv", "powerRevDeriv",
            "iaq", "r_gas", "log_r_gas", "iaqQuality"
            ];

// copy the fields we want as tags to the database slot 1
var tag_keys = [
    "iaqQuality"
    ];

for (var i in value_keys)
    {
    var key = value_keys[i];
    if (key in msg.payload)
        t[key] = msg.payload[key];
    }

for (var i in tag_keys)
    {
    var key = tag_keys[i];
    if (key in msg.payload)
        tags[key] = msg.payload[key];
    }

return result;
