[
    {
        "id": "10bd8fcb.8dee5",
        "type": "subflow",
        "name": "Add dewpoints",
        "info": "This flow converts RH and Temperature to dewpoint, all in degrees C.\n\nIt handles both the normal Catena RH/Temp pair and the optional soil RH/Temp pair,.\n\nIt simply modifies the message and passes it on.",
        "in": [
            {
                "x": 45,
                "y": 341,
                "wires": [
                    {
                        "id": "4994c93e.3b253"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 365,
                "y": 341,
                "wires": [
                    {
                        "id": "4994c93e.3b253",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "4994c93e.3b253",
        "type": "function",
        "z": "10bd8fcb.8dee5",
        "name": "Add dewpoints",
        "func": "function dewpoint(t, rh)\n    {\n    var c1 = 243.04;\n    var c2 = 17.625;\n    var h = rh / 100;\n    if (h <= 0.01)\n        h = 0.01;\n    else if (h > 1.0)\n        h = 1.0;\n\n    var lnh = Math.log(h);\n    var tpc1 = t + c1;\n    var txc2 = t * c2;\n    var txc2_tpc1 = txc2 / tpc1;\n    \n    var tdew = c1 * (lnh + txc2_tpc1) / (c2 - lnh - txc2_tpc1);\n    return tdew;    \n    }\n\nif (\"rh\" in msg.payload && \"t\" in msg.payload)\n    {\n    msg.payload.tDew = dewpoint(msg.payload.t, msg.payload.rh);\n    }\n\nif (\"rhSoil\" in msg.payload && \"tSoil\" in msg.payload)\n    {\n    msg.payload.tDewSoil = dewpoint(msg.payload.tSoil, msg.payload.rhSoil);\n    }\n\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 205,
        "y": 341,
        "wires": [
            []
        ]
    },
    {
        "id": "6ce1c33e.75b30c",
        "type": "subflow",
        "name": "Set Node Mapping",
        "info": "Input is a message\nOutput is same message, with display_key set to \"{app_id}.{dev_id}\" and display_name set to a friendly name from the built-in map.",
        "in": [
            {
                "x": 108,
                "y": 350,
                "wires": [
                    {
                        "id": "938164f8.2db698"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 515,
                "y": 346,
                "wires": [
                    {
                        "id": "938164f8.2db698",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "938164f8.2db698",
        "type": "function",
        "z": "6ce1c33e.75b30c",
        "name": "Map devID to friendy name",
        "func": "// set up the table of names\nvar nodeMap = global.get(\"nodeMap\");\nif (nodeMap === undefined)\n    {\n    var myApp = \"hualian-garden-v2\";\n    var myNodes = [\n        [ myApp + \".device-02\", \"Mushrooom 1\"],\n        [ myApp + \".device-04\", \"Gateway room/kitchen\" ],\n        [ myApp + \".device-05\", \"Upper seaweed tank\" ],\n        [ myApp + \".device-06\", \"Greenhouse 3\" ],\n        [ myApp + \".device-07\", \"Greenhouse 2\" ],\n        [ myApp + \".device-09\", \"Greenhouse 1\" ],\n        [ myApp + \".device-10\", \"Office 1\" ], // 95\n        [ myApp + \".device-11\", \"Field office 1\" ], // 96\n        [ myApp + \".device-12\", \"Big seaweed tank\" ], // 97\n        [ myApp + \".device-13\", \"Mushroom 2\" ], // 98\n        [ myApp + \".device-14\", \"Office 2\" ], // 99\n        [ myApp + \".device-15\", \"Field office 2\" ], // a0\n        ];\n    // populate the map if needed.\n    nodeMap = {};\n    for (var i in myNodes)\n    {\n        var pair = myNodes[i];\n        var key = pair[0];\n        if (pair[1] !== \"\")\n            nodeMap[key] = pair[1];\n    }\n    //node.warn(nodeMap);\n    global.set(\"nodeMap\", nodeMap);\n    }\n\n// use app_id.dev_id to form a key\n// and put into the message\nvar sKey = msg.app_id + \".\" + msg.dev_id;\n\nmsg.display_key = sKey;\n\n// translate the key if needed.\nif (sKey in nodeMap)\n    {\n    msg.display_id = nodeMap[sKey];\n    }\nelse\n    {\n    msg.display_id = sKey;\n    }\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 316,
        "y": 349,
        "wires": [
            []
        ]
    },
    {
        "id": "7a446f37.b8f37",
        "type": "subflow",
        "name": "Catena 4450 message 0x15 decode",
        "info": "",
        "in": [
            {
                "x": 84,
                "y": 62,
                "wires": [
                    {
                        "id": "f8714c8d.77ebd"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 471,
                "y": 170,
                "wires": [
                    {
                        "id": "132e152.12ff06b",
                        "port": 0
                    }
                ]
            },
            {
                "x": 433,
                "y": 423,
                "wires": [
                    {
                        "id": "811068cb.7c55b",
                        "port": 0
                    }
                ]
            },
            {
                "x": 990,
                "y": 426,
                "wires": [
                    {
                        "id": "438effb5.23366",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "811068cb.7c55b",
        "type": "function",
        "z": "7a446f37.b8f37",
        "name": "Prepare for DataBase",
        "func": "var result =\n{\n    payload:\n[{\n        msgID: msg._msgid,\n        counter: msg.counter,\n        //time: new Date(msg.metadata.time).getTime(),\n},\n{\n    devEUI: msg.hardware_serial,\n    devID: msg.dev_id,\n    displayName: msg.display_id,\n    displayKey: msg.display_key,\n    nodeType: msg.local.nodeType,\n    platformType: msg.local.platformType,\n    radioType: msg.local.radioType,\n    applicationName: msg.local.applicationName,\n}]\n};\n\nvar t = result.payload[0];\n\n// copy the fields we want to the database slot 0.\nvar nodes = [ \"vBat\", \"vBus\", \"boot\", \"t\", \"tDew\", \"p\", \"p0\", \"rh\", \"lux\", \n              \"tWater\", \"tSoil\", \"rhSoil\", \"tDewSoil\" ];\n\nfor (var i in nodes)\n    {\n    var key = nodes[i];\n    if (key in msg.payload)\n        t[key] = msg.payload[key];\n    }\n    \nreturn result;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 265,
        "y": 423,
        "wires": [
            []
        ]
    },
    {
        "id": "132e152.12ff06b",
        "type": "function",
        "z": "7a446f37.b8f37",
        "name": "Decode data (15)",
        "func": "var bytes;\nif (\"payload_raw\" in msg)\n    {\n    // the console already decoded this\n    bytes = msg.payload_raw;  // pick up data for convenience\n    // msg.payload_fields still has the decoded data\n    }\nelse\n    {\n    // no console debug\n    bytes = msg.payload;  // pick up data for conveneince\n    }\n\nvar decoded = { /* pm: 0 */ };\n\nif (bytes[0] != 0x15)\n    {\n    // not one of ours\n    node.error(\"not ours! \" + bytes[0].toString());\n    return;\n    }\n    \nvar i = 1;\n// fetch the bitmap.\nvar flags = bytes[i++];\n\nif (flags & 0x1) {\n  // set vRaw to a uint16, and increment pointer\n  var vRaw = (bytes[i] << 8) + bytes[i + 1];\n  i += 2;\n  // interpret uint16 as an int16 instead.\n  if (vRaw & 0x8000)\n\t  vRaw += -0x10000;\n  // scale and save in decoded.\n  decoded.vBat = vRaw / 4096.0;\n}\n\nif (flags & 0x2) {\n  var vRawBus = (bytes[i] << 8) + bytes[i + 1];\n  i += 2;\n  if (vRawBus & 0x8000)\n\t  vRawBus += -0x10000;\n  decoded.vBus = vRawBus / 4096.0;\n}\n\nif (flags & 0x4) {\n  var iBoot = bytes[i];\n  i += 1;\n  decoded.boot = iBoot;\n}\n\nif (flags & 0x8) {\n  // we have temp, pressure, RH\n  var tRaw = (bytes[i] << 8) + bytes[i + 1];\n  if (tRaw & 0x8000)\n\t  tRaw = -0x10000 + tRaw;\n  i += 2;\n  var pRaw = (bytes[i] << 8) + bytes[i + 1];\n  i += 2;\n  var hRaw = bytes[i++];\n\n  decoded.t = tRaw / 256;\n  decoded.p = pRaw * 4 / 100.0;\n  decoded.rh = hRaw / 256 * 100;\n}\n\nif (flags & 0x10) {\n  // we have lux\n  var luxRaw = (bytes[i] << 8) + bytes[i + 1];\n  i += 2;\n  decoded.lux = luxRaw;\n}\n\nif (flags & 0x20) {\n  var tempRaw = (bytes[i] << 8) + bytes[i + 1];\n  // onewire temperature\n  i += 2;\n  decoded.tWater = tempRaw / 256;\n}\n\nif (flags & 0x40) {\n  // temperature followed by RH\n  var tempRaw = (bytes[i] << 8) + bytes[i + 1];\n  i += 2;\n  var tempRH = bytes[i];\n  i += 1;\n  decoded.tSoil = tempRaw / 256;\n  decoded.rhSoil = tempRH / 256 * 100;\n}\n\nmsg.payload = decoded;\nmsg.local = \n    {\n    nodeType: \"Catena 4450-M102\",\n    platformType: \"Feather M0 LoRa\",\n    radioType: \"RF95\",\n    applicationName: \"Soil/Water monitoring\"\n    };\n\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 269.5,
        "y": 170,
        "wires": [
            [
                "f041f60f.82b4c8"
            ]
        ]
    },
    {
        "id": "1ec72b66.a9289d",
        "type": "function",
        "z": "7a446f37.b8f37",
        "name": "Add normalized pressure",
        "func": "//\n// calculate sealevel pressure given altitude of sensor and station\n// pressure.\n//\nvar h = 305; // meters\nif (\"p\" in msg.payload)\n    {\n    var p = msg.payload.p;\n    var L = -0.0065;\n    var Tb = 288.15;\n    var ep = -5.2561;\n    \n    var p0 = p * Math.pow(1 + (L * h)/Tb, ep);\n    msg.payload.p0 = p0;\n    }\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 273,
        "y": 294,
        "wires": [
            [
                "811068cb.7c55b",
                "438effb5.23366"
            ]
        ]
    },
    {
        "id": "438effb5.23366",
        "type": "function",
        "z": "7a446f37.b8f37",
        "name": "Prep for RF store",
        "func": "var result = \n{\n    payload:\n[{\n    frequency: msg.metadata.frequency,\n    channel: msg.metadata.gateways[0].channel,\n    datarate: msg.metadata.data_rate,\n    codingrate: msg.metadata.coding_rate,\n    rssi: msg.metadata.gateways[0].rssi,\n    snr: msg.metadata.gateways[0].snr,\n    msgID: msg._msgid,\n    counter: msg.counter,\n},\n{\n    devEUI: msg.hardware_serial,\n    devID: msg.dev_id,\n    displayName: msg.display_id,\n    displayKey: msg.display_key,\n    gatewayEUI: msg.metadata.gateways[0].gtw_id,\n    nodeType: msg.local.nodeType,\n    platformType: msg.local.platformType,\n    radioType: msg.local.radioType,\n    applicationName: msg.local.applicationName,\n    // we make these tags so we can plot rssi by \n    // channel, etc.\n    frequency: msg.metadata.frequency,\n    channel: msg.metadata.gateways[0].channel,\n    datarate: msg.metadata.data_rate,\n    codingrate: msg.metadata.coding_rate,\n}]\n};\nreturn result;",
        "outputs": 1,
        "noerr": 0,
        "x": 847.3888888888889,
        "y": 425.2222222222222,
        "wires": [
            []
        ]
    },
    {
        "id": "f8714c8d.77ebd",
        "type": "subflow:6ce1c33e.75b30c",
        "z": "7a446f37.b8f37",
        "name": "",
        "x": 270,
        "y": 62,
        "wires": [
            [
                "132e152.12ff06b"
            ]
        ]
    },
    {
        "id": "f041f60f.82b4c8",
        "type": "subflow:10bd8fcb.8dee5",
        "z": "7a446f37.b8f37",
        "x": 266,
        "y": 231,
        "wires": [
            [
                "1ec72b66.a9289d"
            ]
        ]
    },
    {
        "id": "10bd8fcb.8dee5",
        "type": "subflow",
        "name": "Add dewpoints",
        "info": "This flow converts RH and Temperature to dewpoint, all in degrees C.\n\nIt handles both the normal Catena RH/Temp pair and the optional soil RH/Temp pair,.\n\nIt simply modifies the message and passes it on.",
        "in": [
            {
                "x": 45,
                "y": 341,
                "wires": [
                    {
                        "id": "4994c93e.3b253"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 365,
                "y": 341,
                "wires": [
                    {
                        "id": "4994c93e.3b253",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "4994c93e.3b253",
        "type": "function",
        "z": "10bd8fcb.8dee5",
        "name": "Add dewpoints",
        "func": "function dewpoint(t, rh)\n    {\n    var c1 = 243.04;\n    var c2 = 17.625;\n    var h = rh / 100;\n    if (h <= 0.01)\n        h = 0.01;\n    else if (h > 1.0)\n        h = 1.0;\n\n    var lnh = Math.log(h);\n    var tpc1 = t + c1;\n    var txc2 = t * c2;\n    var txc2_tpc1 = txc2 / tpc1;\n    \n    var tdew = c1 * (lnh + txc2_tpc1) / (c2 - lnh - txc2_tpc1);\n    return tdew;    \n    }\n\nif (\"rh\" in msg.payload && \"t\" in msg.payload)\n    {\n    msg.payload.tDew = dewpoint(msg.payload.t, msg.payload.rh);\n    }\n\nif (\"rhSoil\" in msg.payload && \"tSoil\" in msg.payload)\n    {\n    msg.payload.tDewSoil = dewpoint(msg.payload.tSoil, msg.payload.rhSoil);\n    }\n\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 205,
        "y": 341,
        "wires": [
            []
        ]
    },
    {
        "id": "6ce1c33e.75b30c",
        "type": "subflow",
        "name": "Set Node Mapping",
        "info": "Input is a message\nOutput is same message, with display_key set to \"{app_id}.{dev_id}\" and display_name set to a friendly name from the built-in map.",
        "in": [
            {
                "x": 108,
                "y": 350,
                "wires": [
                    {
                        "id": "938164f8.2db698"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 515,
                "y": 346,
                "wires": [
                    {
                        "id": "938164f8.2db698",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "938164f8.2db698",
        "type": "function",
        "z": "6ce1c33e.75b30c",
        "name": "Map devID to friendy name",
        "func": "// set up the table of names\nvar nodeMap = global.get(\"nodeMap\");\nif (nodeMap === undefined)\n    {\n    var myApp = \"hualian-garden-v2\";\n    var myNodes = [\n        [ myApp + \".device-02\", \"Mushrooom 1\"],\n        [ myApp + \".device-04\", \"Gateway room/kitchen\" ],\n        [ myApp + \".device-05\", \"Upper seaweed tank\" ],\n        [ myApp + \".device-06\", \"Greenhouse 3\" ],\n        [ myApp + \".device-07\", \"Greenhouse 2\" ],\n        [ myApp + \".device-09\", \"Greenhouse 1\" ],\n        [ myApp + \".device-10\", \"Office 1\" ], // 95\n        [ myApp + \".device-11\", \"Field office 1\" ], // 96\n        [ myApp + \".device-12\", \"Big seaweed tank\" ], // 97\n        [ myApp + \".device-13\", \"Mushroom 2\" ], // 98\n        [ myApp + \".device-14\", \"Office 2\" ], // 99\n        [ myApp + \".device-15\", \"Field office 2\" ], // a0\n        ];\n    // populate the map if needed.\n    nodeMap = {};\n    for (var i in myNodes)\n    {\n        var pair = myNodes[i];\n        var key = pair[0];\n        if (pair[1] !== \"\")\n            nodeMap[key] = pair[1];\n    }\n    //node.warn(nodeMap);\n    global.set(\"nodeMap\", nodeMap);\n    }\n\n// use app_id.dev_id to form a key\n// and put into the message\nvar sKey = msg.app_id + \".\" + msg.dev_id;\n\nmsg.display_key = sKey;\n\n// translate the key if needed.\nif (sKey in nodeMap)\n    {\n    msg.display_id = nodeMap[sKey];\n    }\nelse\n    {\n    msg.display_id = sKey;\n    }\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 316,
        "y": 349,
        "wires": [
            []
        ]
    },
    {
        "id": "e2b00748.8e28f",
        "type": "subflow",
        "name": "Catena 4450 message 0x14 decode",
        "info": "This subflow decodes messages of type 0x14 and prepares them for storage into the database. The input should be connected to a TTN application.\n\nThe upper output is normally connected to a debug node, or left open.\n\nThe middle output carries the data that is to be sent to the database.\n\nTo lower output carries RF data that is to be sent to one or more RF databases.\n\nDatabases are usually InfluxDB databases.",
        "in": [
            {
                "x": 39.77777099609375,
                "y": 197.88888549804688,
                "wires": [
                    {
                        "id": "37a0656e.cc0bb2"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 371,
                "y": 277,
                "wires": [
                    {
                        "id": "6a8d820c.250c6c",
                        "port": 0
                    }
                ]
            },
            {
                "x": 376.5,
                "y": 530,
                "wires": [
                    {
                        "id": "d50d1f86.db306",
                        "port": 0
                    }
                ]
            },
            {
                "x": 948.8888888888889,
                "y": 532.2222222222222,
                "wires": [
                    {
                        "id": "7194c17a.a0f0b",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "d50d1f86.db306",
        "type": "function",
        "z": "e2b00748.8e28f",
        "name": "Prepare for DataBase",
        "func": "var result =\n{\n    payload:\n[{\n        msgID: msg._msgid,\n        counter: msg.counter,\n        //time: new Date(msg.metadata.time).getTime(),\n},\n{\n    devEUI: msg.hardware_serial,\n    devID: msg.dev_id,\n    displayName: msg.display_id,\n    displayKey: msg.display_key,\n    nodeType: msg.local.nodeType,\n    platformType: msg.local.platformType,\n    radioType: msg.local.radioType,\n    applicationName: msg.local.applicationName,\n}]\n};\n\nvar t = result.payload[0];\n\n// copy the fields we want to the database slot 0.\nvar nodes = [ \"vBat\", \"boot\", \"t\", \"tDew\", \"p\", \"p0\", \"rh\", \"lux\", \n              \"powerUsed\", \"powerRev\", \"powerUsedDeriv\", \"powerRevDeriv\" ];\n\nfor (var i in nodes)\n    {\n    var key = nodes[i];\n    if (key in msg.payload)\n        t[key] = msg.payload[key];\n    }\n    \nreturn result;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 196.5,
        "y": 530,
        "wires": [
            []
        ]
    },
    {
        "id": "6a8d820c.250c6c",
        "type": "function",
        "z": "e2b00748.8e28f",
        "name": "Decode data (14)",
        "func": "var b;\nif (\"payload_raw\" in msg)\n    {\n    // the console already decoded this\n    b = msg.payload_raw;  // pick up data for convenience\n    // msg.payload_fields still has the decoded data\n    }\nelse\n    {\n    // no console debug\n    b = msg.payload;  // pick up data for conveneince\n    }\n\nvar result = { /* pm: 0 */ };\n\nif (b[0] != 0x14)\n    {\n    // not one of ours\n    node.error(\"not ours! \" + b[0].toString());\n    return;\n    }\n    \n// node.error(\"decode\");\nvar i = 1;\nvar flags = b[i++];\n\nif (flags & 0x1)\n    {\n    var vRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    if (vRaw & 0x8000)\n        vRaw += -0x10000;\n    result.vBat = vRaw / 4096.0;\n    }\n\nif (flags & 0x2)\n    {\n    var vRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    if (vRaw & 0x8000)\n        vRaw += -0x10000;\n    result.vBus = vRaw / 4096.0;\n    }\n\nif (flags & 0x4)\n    {\n    var iBoot = b[i];\n    i += 1;\n    result.boot = iBoot;\n    }\n\nif (flags & 0x8)\n    {\n    // we have temp, pressure, RH\n    var tRaw = (b[i] << 8) + b[i+1];\n    if (tRaw & 0x8000)\n        tRaw = -0x10000 + tRaw;\n    i += 2;\n    var pRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    var hRaw = b[i++];\n    \n    result.t = tRaw / 256;\n    result.p = pRaw * 4 / 100.0;\n    result.rh = hRaw / 256 * 100;\n    }\n\nif (flags & 0x10)\n    {\n    // we have lux\n    var luxRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    result.lux = luxRaw;\n    }\n\n\nif (flags & 0x20)   // watthour\n    {\n    var powerIn = (b[i] << 8) + b[i+1];\n    i += 2;\n    var powerOut = (b[i] << 8) + b[i+1];\n    i += 2;\n    result.powerUsed = powerIn;\n    result.powerRev = powerOut;\n    }\n    \nif (flags & 0x40)  // normalize floating pulses per hour\n    {\n    var floatIn = (b[i] << 8) + b[i+1];\n    i += 2;\n    var floatOut = (b[i] << 8) + b[i+1];\n    i += 2;\n    \n    var exp1 = floatIn >> 12;\n    var exp2 = floatOut >> 12;\n    var mant1 = (floatIn & 0xFFF) / 4096.0;\n    var mant2 = (floatOut & 0xFFF) / 4096.0;\n    var powerPerHourIn = mant1 * Math.pow(2, exp1 - 15) * 60 * 60 * 4;\n    var powerPerHourOut = mant2 * Math.pow(2, exp2 - 15) * 60 * 60 * 4;\n    result.powerUsedDeriv = powerPerHourIn;\n    result.powerRevDeriv = powerPerHourOut;\n    }\n\nmsg.payload = result;\nmsg.local = \n    {\n    nodeType: \"Catena 4450-M101\",\n    platformType: \"Feather M0 LoRa\",\n    radioType: \"RF95\",\n    applicationName: \"AC Power Monitoring\"\n    };\n\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 201,
        "y": 277,
        "wires": [
            [
                "8aec5378.d1b038"
            ]
        ]
    },
    {
        "id": "d93144c3.29256",
        "type": "function",
        "z": "e2b00748.8e28f",
        "name": "Add normalized pressure",
        "func": "//\n// calculate sealevel pressure given altitude of sensor and station\n// pressure.\n//\nvar h = 305; // meters\nif (\"p\" in msg.payload)\n    {\n    var p = msg.payload.p;\n    var L = -0.0065;\n    var Tb = 288.15;\n    var ep = -5.2561;\n    \n    var p0 = p * Math.pow(1 + (L * h)/Tb, ep);\n    msg.payload.p0 = p0;\n    }\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 204.5,
        "y": 401,
        "wires": [
            [
                "d50d1f86.db306",
                "7194c17a.a0f0b"
            ]
        ]
    },
    {
        "id": "7194c17a.a0f0b",
        "type": "function",
        "z": "e2b00748.8e28f",
        "name": "Prep for RF store",
        "func": "var result = \n{\n    payload:\n[{\n    frequency: msg.metadata.frequency,\n    channel: msg.metadata.gateways[0].channel,\n    datarate: msg.metadata.data_rate,\n    codingrate: msg.metadata.coding_rate,\n    rssi: msg.metadata.gateways[0].rssi,\n    snr: msg.metadata.gateways[0].snr,\n    msgID: msg._msgid,\n    counter: msg.counter,\n},\n{\n    devEUI: msg.hardware_serial,\n    devID: msg.dev_id,\n    displayName: msg.display_id,\n    displayKey: msg.display_key,\n    gatewayEUI: msg.metadata.gateways[0].gtw_id,\n    nodeType: msg.local.nodeType,\n    platformType: msg.local.platformType,\n    radioType: msg.local.radioType,\n    applicationName: msg.local.applicationName,\n    // we make these tags so we can plot rssi by \n    // channel, etc.\n    frequency: msg.metadata.frequency,\n    channel: msg.metadata.gateways[0].channel,\n    datarate: msg.metadata.data_rate,\n    codingrate: msg.metadata.coding_rate,\n}]\n};\nreturn result;",
        "outputs": 1,
        "noerr": 0,
        "x": 778.8888888888889,
        "y": 532.2222222222222,
        "wires": [
            []
        ]
    },
    {
        "id": "37a0656e.cc0bb2",
        "type": "subflow:6ce1c33e.75b30c",
        "z": "e2b00748.8e28f",
        "x": 189,
        "y": 198,
        "wires": [
            [
                "6a8d820c.250c6c"
            ]
        ]
    },
    {
        "id": "8aec5378.d1b038",
        "type": "subflow:10bd8fcb.8dee5",
        "z": "e2b00748.8e28f",
        "x": 205,
        "y": 341,
        "wires": [
            [
                "d93144c3.29256"
            ]
        ]
    },
    {
        "id": "6ce1c33e.75b30c",
        "type": "subflow",
        "name": "Set Node Mapping",
        "info": "Input is a message\nOutput is same message, with display_key set to \"{app_id}.{dev_id}\" and display_name set to a friendly name from the built-in map.",
        "in": [
            {
                "x": 108,
                "y": 350,
                "wires": [
                    {
                        "id": "938164f8.2db698"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 515,
                "y": 346,
                "wires": [
                    {
                        "id": "938164f8.2db698",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "938164f8.2db698",
        "type": "function",
        "z": "6ce1c33e.75b30c",
        "name": "Map devID to friendy name",
        "func": "// set up the table of names\nvar nodeMap = global.get(\"nodeMap\");\nif (nodeMap === undefined)\n    {\n    var myApp = \"hualian-garden-v2\";\n    var myNodes = [\n        [ myApp + \".device-02\", \"Mushrooom 1\"],\n        [ myApp + \".device-04\", \"Gateway room/kitchen\" ],\n        [ myApp + \".device-05\", \"Upper seaweed tank\" ],\n        [ myApp + \".device-06\", \"Greenhouse 3\" ],\n        [ myApp + \".device-07\", \"Greenhouse 2\" ],\n        [ myApp + \".device-09\", \"Greenhouse 1\" ],\n        [ myApp + \".device-10\", \"Office 1\" ], // 95\n        [ myApp + \".device-11\", \"Field office 1\" ], // 96\n        [ myApp + \".device-12\", \"Big seaweed tank\" ], // 97\n        [ myApp + \".device-13\", \"Mushroom 2\" ], // 98\n        [ myApp + \".device-14\", \"Office 2\" ], // 99\n        [ myApp + \".device-15\", \"Field office 2\" ], // a0\n        ];\n    // populate the map if needed.\n    nodeMap = {};\n    for (var i in myNodes)\n    {\n        var pair = myNodes[i];\n        var key = pair[0];\n        if (pair[1] !== \"\")\n            nodeMap[key] = pair[1];\n    }\n    //node.warn(nodeMap);\n    global.set(\"nodeMap\", nodeMap);\n    }\n\n// use app_id.dev_id to form a key\n// and put into the message\nvar sKey = msg.app_id + \".\" + msg.dev_id;\n\nmsg.display_key = sKey;\n\n// translate the key if needed.\nif (sKey in nodeMap)\n    {\n    msg.display_id = nodeMap[sKey];\n    }\nelse\n    {\n    msg.display_id = sKey;\n    }\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 316,
        "y": 349,
        "wires": [
            []
        ]
    },
    {
        "id": "10bd8fcb.8dee5",
        "type": "subflow",
        "name": "Add dewpoints",
        "info": "This flow converts RH and Temperature to dewpoint, all in degrees C.\n\nIt handles both the normal Catena RH/Temp pair and the optional soil RH/Temp pair,.\n\nIt simply modifies the message and passes it on.",
        "in": [
            {
                "x": 45,
                "y": 341,
                "wires": [
                    {
                        "id": "4994c93e.3b253"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 365,
                "y": 341,
                "wires": [
                    {
                        "id": "4994c93e.3b253",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "4994c93e.3b253",
        "type": "function",
        "z": "10bd8fcb.8dee5",
        "name": "Add dewpoints",
        "func": "function dewpoint(t, rh)\n    {\n    var c1 = 243.04;\n    var c2 = 17.625;\n    var h = rh / 100;\n    if (h <= 0.01)\n        h = 0.01;\n    else if (h > 1.0)\n        h = 1.0;\n\n    var lnh = Math.log(h);\n    var tpc1 = t + c1;\n    var txc2 = t * c2;\n    var txc2_tpc1 = txc2 / tpc1;\n    \n    var tdew = c1 * (lnh + txc2_tpc1) / (c2 - lnh - txc2_tpc1);\n    return tdew;    \n    }\n\nif (\"rh\" in msg.payload && \"t\" in msg.payload)\n    {\n    msg.payload.tDew = dewpoint(msg.payload.t, msg.payload.rh);\n    }\n\nif (\"rhSoil\" in msg.payload && \"tSoil\" in msg.payload)\n    {\n    msg.payload.tDewSoil = dewpoint(msg.payload.tSoil, msg.payload.rhSoil);\n    }\n\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 205,
        "y": 341,
        "wires": [
            []
        ]
    },
    {
        "id": "9d152480.645c5",
        "type": "subflow",
        "name": "Catena 4410 message 0x11 decode",
        "info": "Decode messages from Catena 4410, producing debug, RF database and Sensor database outputs.\n\nDoes nothing if the message is not type 0x11.",
        "in": [
            {
                "x": 211,
                "y": 101,
                "wires": [
                    {
                        "id": "cc8fbd40.4ddfd8"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 561,
                "y": 251,
                "wires": [
                    {
                        "id": "dd3326.6eba54d8",
                        "port": 0
                    }
                ]
            },
            {
                "x": 666,
                "y": 328.5,
                "wires": [
                    {
                        "id": "f54056fe.f70e7",
                        "port": 0
                    }
                ]
            },
            {
                "x": 693,
                "y": 418,
                "wires": [
                    {
                        "id": "ed52dcff.8a3e68",
                        "port": 0
                    }
                ]
            }
        ]
    },
    {
        "id": "dc08617f.60acd8",
        "type": "function",
        "z": "9d152480.645c5",
        "name": "Decode Hualian",
        "func": "var i = 0;\nvar b= msg.payload_raw;\nvar result = { /* p: 0, t: 0, rh: 0,*/ };\n\nif (b[i++] != 0x11)\n    // not one of ours\n    return;\n\nvar flags = b[i++];\n\nif (flags & 0x1)\n    {\n    var vRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    if (vRaw & 0x8000)\n        vRaw += -0x10000;\n    result.vBat = vRaw / 4096.0;\n    }\nif (flags & 0x2)\n    {\n    var vRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    if (vRaw & 0x8000)\n        vRaw += -0x10000;\n    result.vBus = vRaw / 4096.0;\n    }\nif (flags & 0x4)\n    {\n    // we have temp, pressure, RH\n    var tRaw = (b[i] << 8) + b[i+1];\n    if (tRaw & 0x8000)\n        tRaw = -0x10000 + tRaw;\n    i += 2;\n    var pRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    var hRaw = b[i++];\n    \n    result.t = tRaw / 256;\n    result.p = pRaw * 4 / 100.0;\n    result.rh = hRaw / 256 * 100;\n    }\nif (flags & 0x8)\n    {\n    // we have lux\n    var luxRaw = (b[i] << 8) + b[i+1];\n    i += 2;\n    result.lux = luxRaw;\n    }\nelse\n    {\n    // things plot better if we have zeros;\n    // in this application, all devices\n    // should have lux sensors.\n    result.lux = 0;\n    }\nif (flags & 0x10)   // water\n    {\n    // we have temp\n    var tRaw = (b[i] << 8) + b[i+1];\n    if (tRaw & 0x8000)\n        tRaw = -0x10000 + tRaw;\n    i += 2;\n    result.tWater = tRaw / 256.0;\n    }\nif (flags & 0x20)   // soil sensor\n    {\n    // we have temp, RH\n    var tRaw = (b[i] << 8) + b[i+1];\n    if (tRaw & 0x8000)\n        tRaw = -0x10000 + tRaw;\n    i += 2;\n    var hRaw = b[i++];\n    \n    result.tSoil = tRaw / 256;\n    result.rhSoil = hRaw / 256 * 100;\n    }\n\nmsg.payload = result;\nmsg.local = \n    {\n    nodeType: \"Catena 4410\",\n    platformType: \"Feather M0 LoRa\",\n    radioType: \"RF95\",\n    applicationName: \"Hualian garden\"\n    };\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 375,
        "y": 167,
        "wires": [
            [
                "96885b63.858b6"
            ]
        ]
    },
    {
        "id": "f54056fe.f70e7",
        "type": "function",
        "z": "9d152480.645c5",
        "name": "Hualian prep for sensor db",
        "func": "var result = \n{\n    payload:\n[{\n    msgID: msg._msgid,\n    counter: msg.counter,\n},\n{\n    devEUI: msg.hardware_serial,\n    devID: msg.dev_id,\n    displayName: msg.display_id,\n    displayKey: msg.display_key,\n    nodeType: msg.local.nodeType,\n    platformType: msg.local.platformType,\n    radioType: msg.local.radioType,\n    applicationName: msg.local.applicationName,\n}]\n};\n\n// shorthand for \"result.payload[0]\"\nvar t = result.payload[0];\n\n// copy the fields we want to the database slot 0.\nvar nodes = [ \"vBat\", \"vBus\", \"boot\", \"t\", \"tDew\", \"p\", \"p0\", \"rh\", \"lux\", \n              \"tWater\", \"tSoil\", \"rhSoil\", \"tDewSoil\" ];\n\nfor (var i in nodes)\n    {\n    var key = nodes[i];\n    if (key in msg.payload)\n        t[key] = msg.payload[key];\n    }\n\nreturn result;",
        "outputs": 1,
        "noerr": 0,
        "x": 482,
        "y": 327,
        "wires": [
            []
        ]
    },
    {
        "id": "dd3326.6eba54d8",
        "type": "function",
        "z": "9d152480.645c5",
        "name": "Add sealevel pressure",
        "func": "//\n// calculate sealevel pressure given altitude of sensor and station\n// pressure.\n//\nvar h = 65; // meters for Hualian garden\nif (\"p\" in msg.payload)\n    {\n    var p = msg.payload.p;\n    var L = -0.0065;\n    var Tb = 288.15;\n    var ep = -5.2561;\n    \n    var p0 = p * Math.pow(1 + (L * h)/Tb, ep);\n    msg.payload.p0 = p0;\n    }\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "x": 381,
        "y": 251,
        "wires": [
            [
                "f54056fe.f70e7",
                "ed52dcff.8a3e68"
            ]
        ]
    },
    {
        "id": "ed52dcff.8a3e68",
        "type": "function",
        "z": "9d152480.645c5",
        "name": "Prepare for RF database",
        "func": "var result = \n{\n    payload:\n[{\n    frequency: msg.metadata.frequency,\n    channel: msg.metadata.gateways[0].channel,\n    datarate: msg.metadata.data_rate,\n    codingrate: msg.metadata.coding_rate,\n    rssi: msg.metadata.gateways[0].rssi,\n    snr: msg.metadata.gateways[0].snr,\n    msgID: msg._msgid,\n    counter: msg.counter,\n},\n{\n    devEUI: msg.hardware_serial,\n    devID: msg.dev_id,\n    displayName: msg.display_id,\n    displayKey: msg.display_key,\n    gatewayEUI: msg.metadata.gateways[0].gtw_id,\n    nodeType: msg.local.nodeType,\n    platformType: msg.local.platformType,\n    radioType: msg.local.radioType,\n    applicationName: msg.local.applicationName,\n    // we make these tags so we can plot rssi by \n    // channel, etc.\n    frequency: msg.metadata.frequency,\n    channel: msg.metadata.gateways[0].channel,\n    datarate: msg.metadata.data_rate,\n    codingrate: msg.metadata.coding_rate,\n}]\n};\nreturn result;",
        "outputs": 1,
        "noerr": 0,
        "x": 495,
        "y": 418.5,
        "wires": [
            []
        ]
    },
    {
        "id": "96885b63.858b6",
        "type": "subflow:10bd8fcb.8dee5",
        "z": "9d152480.645c5",
        "x": 369,
        "y": 209,
        "wires": [
            [
                "dd3326.6eba54d8"
            ]
        ]
    },
    {
        "id": "cc8fbd40.4ddfd8",
        "type": "subflow:6ce1c33e.75b30c",
        "z": "9d152480.645c5",
        "x": 364,
        "y": 102,
        "wires": [
            [
                "dc08617f.60acd8"
            ]
        ]
    },
    {
        "id": "c2f526b5.5a7fc8",
        "type": "subflow",
        "name": "Catena message decoder (all forms)",
        "info": "",
        "in": [
            {
                "x": 49,
                "y": 49,
                "wires": [
                    {
                        "id": "bf176e04.7332e"
                    }
                ]
            }
        ],
        "out": [
            {
                "x": 636,
                "y": 227,
                "wires": [
                    {
                        "id": "339e30f6.dd2e18",
                        "port": 0
                    },
                    {
                        "id": "76f0d83d.3ca",
                        "port": 0
                    },
                    {
                        "id": "ceefe8a2.e3dc1",
                        "port": 0
                    }
                ]
            },
            {
                "x": 638,
                "y": 300,
                "wires": [
                    {
                        "id": "339e30f6.dd2e18",
                        "port": 1
                    },
                    {
                        "id": "76f0d83d.3ca",
                        "port": 1
                    },
                    {
                        "id": "ceefe8a2.e3dc1",
                        "port": 1
                    }
                ]
            },
            {
                "x": 644,
                "y": 386,
                "wires": [
                    {
                        "id": "339e30f6.dd2e18",
                        "port": 2
                    },
                    {
                        "id": "76f0d83d.3ca",
                        "port": 2
                    },
                    {
                        "id": "ceefe8a2.e3dc1",
                        "port": 2
                    }
                ]
            }
        ]
    },
    {
        "id": "339e30f6.dd2e18",
        "type": "subflow:9d152480.645c5",
        "z": "c2f526b5.5a7fc8",
        "name": "",
        "x": 254,
        "y": 234,
        "wires": [
            [],
            [],
            []
        ]
    },
    {
        "id": "76f0d83d.3ca",
        "type": "subflow:e2b00748.8e28f",
        "z": "c2f526b5.5a7fc8",
        "name": "",
        "x": 254,
        "y": 304,
        "wires": [
            [],
            [],
            []
        ]
    },
    {
        "id": "ceefe8a2.e3dc1",
        "type": "subflow:7a446f37.b8f37",
        "z": "c2f526b5.5a7fc8",
        "x": 258,
        "y": 392,
        "wires": [
            [],
            [],
            []
        ]
    },
    {
        "id": "bf176e04.7332e",
        "type": "switch",
        "z": "c2f526b5.5a7fc8",
        "name": "Switch on message code",
        "property": "payload_raw[0]",
        "propertyType": "msg",
        "rules": [
            {
                "t": "eq",
                "v": "0x11",
                "vt": "str"
            },
            {
                "t": "eq",
                "v": "0x14",
                "vt": "str"
            },
            {
                "t": "eq",
                "v": "0x15",
                "vt": "str"
            }
        ],
        "checkall": "true",
        "outputs": 3,
        "x": 206,
        "y": 50,
        "wires": [
            [
                "339e30f6.dd2e18"
            ],
            [
                "76f0d83d.3ca"
            ],
            [
                "ceefe8a2.e3dc1"
            ]
        ]
    },
    {
        "id": "940089fa.bc57b8",
        "type": "tab",
        "label": "Hualian Garden"
    },
    {
        "id": "f63585fa.75d71",
        "type": "ttn message",
        "z": "940089fa.bc57b8",
        "name": "Hualian Garden Sensors",
        "app": "bf2c1db6.7da4e",
        "dev_id": "",
        "field": "",
        "x": 123,
        "y": 27,
        "wires": [
            [
                "b3bff91.9f81e88"
            ]
        ]
    },
    {
        "id": "c52fd2cb.986fd",
        "type": "debug",
        "z": "940089fa.bc57b8",
        "name": "Message after processing",
        "active": true,
        "console": "false",
        "complete": "true",
        "x": 651,
        "y": 198,
        "wires": []
    },
    {
        "id": "c1cb9651.5d0a5",
        "type": "debug",
        "z": "940089fa.bc57b8",
        "name": "Garden data",
        "active": true,
        "console": "false",
        "complete": "payload",
        "x": 651,
        "y": 260,
        "wires": []
    },
    {
        "id": "778f8d5f.474e0c",
        "type": "debug",
        "z": "940089fa.bc57b8",
        "name": "RF Data",
        "active": true,
        "console": "false",
        "complete": "payload",
        "x": 661,
        "y": 373,
        "wires": []
    },
    {
        "id": "b5a63cdb.2a591",
        "type": "influxdb out",
        "z": "940089fa.bc57b8",
        "influxdb": "ab5ef4db.7fd44",
        "name": "Garden data",
        "measurement": "hualian-garden",
        "x": 665,
        "y": 304,
        "wires": []
    },
    {
        "id": "60299781.19078",
        "type": "influxdb out",
        "z": "940089fa.bc57b8",
        "influxdb": "ab5ef4db.7fd44",
        "name": "RF data",
        "measurement": "RF-data",
        "x": 664,
        "y": 421,
        "wires": []
    },
    {
        "id": "b3bff91.9f81e88",
        "type": "subflow:c2f526b5.5a7fc8",
        "z": "940089fa.bc57b8",
        "x": 282,
        "y": 276.5,
        "wires": [
            [
                "c52fd2cb.986fd"
            ],
            [
                "c1cb9651.5d0a5",
                "b5a63cdb.2a591"
            ],
            [
                "778f8d5f.474e0c",
                "60299781.19078"
            ]
        ]
    },
    {
        "id": "bf2c1db6.7da4e",
        "type": "ttn app",
        "z": "",
        "appId": "hualian-garden-v2",
        "region": "asia-se",
        "accessKey": "ttn-account-v2.vqZX9mQiRTACz3j2XWRNBXaNtyzU77v75znsni_cEr4"
    },
    {
        "id": "ab5ef4db.7fd44",
        "type": "influxdb",
        "z": "",
        "hostname": "influxdb",
        "port": "8086",
        "protocol": "http",
        "database": "garden",
        "name": "Garden data",
        "usetls": false,
        "tls": ""
    }
]
