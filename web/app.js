

var use_write_without_response = true; // false for reliable write.

var make = "/usr/bin/make";
var srcdir = "src/";

var express = require("express")
    , app = express()
    , http = require('http')
    , server = http.createServer(app)
    , io = require('socket.io').listen(server)
   // , hid = require("HID")
    , sys = require('sys')
    , spawn = require('child_process').spawn
    , crypto = require('crypto')
    , fs = require('fs')
    , noble = require('noble')
   // , midi = require('midi')
    ;

io.configure('development', function () {
    io.set('transports', ['websocket', 'xhr-polling']);
    //io.disable('log');
});

io.set('log level', 2);

server.listen(8080);

var child;



app.get('/', function (req, res) {
  res.sendfile(__dirname + '/public/index.html');
});
app.get('/render', function(req, res) {

});

app.use(express.static('public'));


//var serviceUUID = "20cc01230b57436509f331c9227d4c4b";
//var characteristicUUID = serviceUUID.replace("0123", "0124");
//noble.on('discover', function(peripheral){
//    peripheral.connect(function(error) {
//        peripheral.on('servicesDiscover', function(services) {
//            var service = services[0];
//            service.on('characteristicsDiscover', function(characteristics) {
//                var characteristic = characteristics[0];
//                characteristic.on('write', function() {
//                    console.log("written.");
//                });
//                characteristic.write(new Buffer("Hello.", "utf8"));
//            });
//            service.discoverCharacteristics([characteristicUUID]);
//        });
//        peripheral.discoverServices([serviceUUID]);
//    });
//});
//noble.startScanning([serviceUUID], false);

var ble_presentation_formats =
{   0x00: ["rfu", 0x00, "Reserved For Future Use."],
    0x01: ["boolean", 0x01, "Boolean."],
    0x02: ["2bit", 0x02, "Unsigned 2-bit integer."],
    0x03: ["nibble", 0x03, "Unsigned 4-bit integer."],
    0x04: ["uint8", 0x04, "Unsigned 8-bit integer."],
    0x05: ["uint12", 0x05, "Unsigned 12-bit integer."],
    0x06: ["uint16", 0x06, "Unsigned 16-bit integer."],
    0x07: ["uint24", 0x07, "Unsigned 24-bit integer."],
    0x08: ["uint32", 0x08, "Unsigned 32-bit integer."],
    0x09: ["uint48", 0x09, "Unsigned 48-bit integer."],
    0x0A: ["uint64", 0x0A, "Unsigned 64-bit integer."],
    0x0B: ["uint128", 0x0B, "Unsigned 128-bit integer."],
    0x0C: ["sint8", 0x0C, "Signed 2-bit integer."],
    0x0D: ["sint12", 0x0D, "Signed 12-bit integer."],
    0x0E: ["sint16", 0x0E, "Signed 16-bit integer."],
    0x0F: ["sint24", 0x0F, "Signed 24-bit integer."],
    0x10: ["sint32", 0x10, "Signed 32-bit integer."],
    0x11: ["sint48", 0x11, "Signed 48-bit integer."],
    0x12: ["sint64", 0x12, "Signed 64-bit integer."],
    0x13: ["sint128", 0x13, "Signed 128-bit integer."],
    0x14: ["float32", 0x14, "IEEE-754 32-bit floating point."],
    0x15: ["float64", 0x15, "IEEE-754 64-bit floating point."],
    0x16: ["sfloat", 0x16, "IEEE-11073 16-bit SFLOAT."],
    0x17: ["float", 0x17, "IEEE-11073 32-bit FLOAT."],
    0x18: ["duint16", 0x18, "IEEE-20601 format."],
    0x19: ["utf8s", 0x19, "UTF-8 string."],
    0x1A: ["utf16s", 0x1A, "UTF-16 string."],
    0x1B: ["struct", 0x1B, "Opaque Structure."]
};

var ble_units =  // from http://developer.bluetooth.org/gatt/units/Pages/default.aspx
{   0x2700: { "uuid": 0x2700, "unit": "", "description": "unitless", abbreviation: ""},
    0x2701: { "uuid": 0x2701, "unit": "metre", "description": "length", abbreviation: "m"},
    0x2702: { "uuid": 0x2702, "unit": "kilogram", "description": "mass", abbreviation: "kg"},
    0x2703: { "uuid": 0x2703, "unit": "second", "description": "time", abbreviation: "s"},
    0x2704: { "uuid": 0x2704, "unit": "ampere", "description": "electric current", abbreviation: "A"},
    0x2705: { "uuid": 0x2705, "unit": "kelvin", "description": "thermodynamic temperature", abbreviation: "K"},
    0x2706: { "uuid": 0x2706, "unit": "mole", "description": "amount of substance", abbreviation: "m"},
    0x2707: { "uuid": 0x2707, "unit": "candela", "description": "luminous intensity", abbreviation: ""},
    0x2710: { "uuid": 0x2710, "unit": "square metres", "description": "area", abbreviation: ""},
    0x2711: { "uuid": 0x2711, "unit": "cubic metres", "description": "volume", abbreviation: ""},
    0x2712: { "uuid": 0x2712, "unit": "metres per second", "description": "velocity", abbreviation: "m/s"},
    0x2713: { "uuid": 0x2713, "unit": "metres per second squared", "description": "acceleration", abbreviation: "m/s^2"},
    0x2714: { "uuid": 0x2714, "unit": "reciprocal metre", "description": "wavenumber", abbreviation: "m^-1"},
    0x2715: { "uuid": 0x2715, "unit": "kilogram per cubic metre", "description": "density", abbreviation: "k/m^3"},
    0x2716: { "uuid": 0x2716, "unit": "kilogram per square metre", "description": "surface density", abbreviation: "k/m^2"},
    0x2717: { "uuid": 0x2717, "unit": "cubic metre per kilogram", "description": "specific volume", abbreviation: ""},
    0x2718: { "uuid": 0x2718, "unit": "ampere per square metre", "description": "current density", abbreviation: ""},
    0x2719: { "uuid": 0x2719, "unit": "ampere per metre", "description": "magnetic field strength", abbreviation: ""},
    0x271A: { "uuid": 0x271A, "unit": "mole per cubic metre", "description": "amount concentration", abbreviation: ""},
    0x271B: { "uuid": 0x271B, "unit": "kilogram per cubic metre", "description": "mass concentration", abbreviation: ""},
    0x271C: { "uuid": 0x271C, "unit": "candela per square metre", "description": "luminance", abbreviation: ""},
    0x271D: { "uuid": 0x271D, "unit": "", "description": "refractive index", abbreviation: ""},
    0x271E: { "uuid": 0x271E, "unit": "", "description": "relative permeability", abbreviation: ""},
    0x2720: { "uuid": 0x2720, "unit": "radian", "description": "plane angle", abbreviation: "rad"},
    0x2721: { "uuid": 0x2721, "unit": "steradian", "description": "solid angle", abbreviation: "sr"},
    0x2722: { "uuid": 0x2722, "unit": "hertz", "description": "frequency", abbreviation: "Hz"},
    0x2723: { "uuid": 0x2723, "unit": "newton", "description": "force", abbreviation: "N"},
    0x2724: { "uuid": 0x2724, "unit": "pascal", "description": "pressure", abbreviation: "Pa"},
    0x2725: { "uuid": 0x2725, "unit": "joule", "description": "energy", abbreviation: "J"},
    0x2726: { "uuid": 0x2726, "unit": "watt", "description": "power", abbreviation: "W"},
    0x2727: { "uuid": 0x2727, "unit": "coulomb", "description": "electric charge", abbreviation: "C"},
    0x2728: { "uuid": 0x2728, "unit": "volt", "description": "electric potential difference", abbreviation: "V"},
    0x2729: { "uuid": 0x2729, "unit": "farad", "description": "capacitance", abbreviation: "F"},
    0x272A: { "uuid": 0x272A, "unit": "ohm", "description": "electric resistance", abbreviation: "\u2126"},
    0x272B: { "uuid": 0x272B, "unit": "siemens", "description": "electric conductance", abbreviation: ""},
    0x272C: { "uuid": 0x272C, "unit": "weber", "description": "magnetic flex", abbreviation: ""},
    0x272D: { "uuid": 0x272D, "unit": "tesla", "description": "magnetic flex density", abbreviation: "T"},
    0x272E: { "uuid": 0x272E, "unit": "henry", "description": "inductance", abbreviation: "H"},
    0x272F: { "uuid": 0x272F, "unit": "degree Celsius", "description": "Celsius temperature", abbreviation: "°C"},
    0x2730: { "uuid": 0x2730, "unit": "lumen", "description": "luminous flux", abbreviation: "lm"},
    0x2731: { "uuid": 0x2731, "unit": "lux", "description": "illuminance", abbreviation: "lx"},
    0x2732: { "uuid": 0x2732, "unit": "becquerel", "description": "activity referred to a radionuclide", abbreviation: "Bq"},
    0x2733: { "uuid": 0x2733, "unit": "gray", "description": "absorbed dose", abbreviation: "gy"},
    0x2734: { "uuid": 0x2734, "unit": "sievert", "description": "dose equivalent", abbreviation: "Sv"},
    0x2735: { "uuid": 0x2735, "unit": "katal", "description": "catalytic activity", abbreviation: ""},
    0x2740: { "uuid": 0x2740, "unit": "pascal second", "description": "dynamic viscosity", abbreviation: ""},
    0x2741: { "uuid": 0x2741, "unit": "newton metre", "description": "moment of force", abbreviation: ""},
    0x2742: { "uuid": 0x2742, "unit": "newton per metre", "description": "surface tension", abbreviation: ""},
    0x2743: { "uuid": 0x2743, "unit": "radian per second", "description": "angular velocity", abbreviation: ""},
    0x2744: { "uuid": 0x2744, "unit": "radian per second squared", "description": "angular acceleration", abbreviation: ""},
    0x2745: { "uuid": 0x2745, "unit": "watt per square metre", "description": "heat flux density", abbreviation: ""},
    0x2746: { "uuid": 0x2746, "unit": "joule per kelvin", "description": "heat capacity", abbreviation: ""},
    0x2747: { "uuid": 0x2747, "unit": "joule per kilogram kelvin", "description": "specific heat capacity", abbreviation: ""},
    0x2748: { "uuid": 0x2748, "unit": "joule per kilogram", "description": "specific energy", abbreviation: ""},
    0x2749: { "uuid": 0x2749, "unit": "watt per metre kelvin", "description": "thermal conductivity", abbreviation: ""},
    0x274A: { "uuid": 0x274A, "unit": "joule per cubic metre", "description": "energy density", abbreviation: ""},
    0x274B: { "uuid": 0x274B, "unit": "volt per metre", "description": "electric field strength", abbreviation: ""},
    0x274C: { "uuid": 0x274C, "unit": "coulomb per cubic metre", "description": "electric charge density", abbreviation: ""},
    0x274D: { "uuid": 0x274D, "unit": "coulomb per square metre", "description": "surface charge density", abbreviation: ""},
    0x274E: { "uuid": 0x274E, "unit": "coulomb per square metre", "description": "electric flux density", abbreviation: ""},
    0x274F: { "uuid": 0x274F, "unit": "farad per metre", "description": "permittivity", abbreviation: ""},
    0x2750: { "uuid": 0x2750, "unit": "henry per metre", "description": "permeability", abbreviation: ""},
    0x2751: { "uuid": 0x2751, "unit": "joule per mole", "description": "molar energy", abbreviation: ""},
    0x2752: { "uuid": 0x2752, "unit": "joule per mole kelvin", "description": "molar entropy", abbreviation: ""},
    0x2753: { "uuid": 0x2753, "unit": "coulomb per kilogram", "description": "exposure", abbreviation: ""},
    0x2754: { "uuid": 0x2754, "unit": "gray per second", "description": "absorbed dose rate", abbreviation: ""},
    0x2755: { "uuid": 0x2755, "unit": "watt per steradian", "description": "radiant intensity", abbreviation: ""},
    0x2756: { "uuid": 0x2756, "unit": "watt per square meter steradian", "description": "radiance", abbreviation: ""},
    0x2757: { "uuid": 0x2757, "unit": "katal per cubic metre", "description": "catalytic activity concentration", abbreviation: ""},
    0x2760: { "uuid": 0x2760, "unit": "minute", "description": "time", abbreviation: "m"},
    0x2761: { "uuid": 0x2761, "unit": "hour", "description": "time", abbreviation: "h"},
    0x2762: { "uuid": 0x2762, "unit": "day", "description": "time", abbreviation: "d"},
    0x2763: { "uuid": 0x2763, "unit": "degree", "description": "plane angle", abbreviation: "°"},
    0x2764: { "uuid": 0x2764, "unit": "minute", "description": "plane angle", abbreviation: "\""},
    0x2765: { "uuid": 0x2765, "unit": "second", "description": "plane angle", abbreviation: "'"},
    0x2766: { "uuid": 0x2766, "unit": "hectare", "description": "area", abbreviation: "ha"},
    0x2767: { "uuid": 0x2767, "unit": "litre", "description": "volume", abbreviation: "L"},
    0x2768: { "uuid": 0x2768, "unit": "tonne", "description": "mass", abbreviation: ""},
    0x2780: { "uuid": 0x2780, "unit": "bar", "description": "pressure", abbreviation: ""},
    0x2781: { "uuid": 0x2781, "unit": "millimetre of mercury", "description": "pressure", abbreviation: "mmHg"},
    0x2782: { "uuid": 0x2782, "unit": "ångström", "description": "length", abbreviation: "Å"},
    0x2783: { "uuid": 0x2783, "unit": "nautical mile", "description": "length", abbreviation: "nm"},
    0x2784: { "uuid": 0x2784, "unit": "barn", "description": "area", abbreviation: ""},
    0x2785: { "uuid": 0x2785, "unit": "knot", "description": "velocity", abbreviation: "kn"},
    0x2786: { "uuid": 0x2786, "unit": "neper", "description": "logarithmic radio quantity", abbreviation: "Np"},
    0x2787: { "uuid": 0x2787, "unit": "bel", "description": "logarithmic radio quantity", abbreviation: "dB"},
    0x27A0: { "uuid": 0x27A0, "unit": "yard", "description": "length", abbreviation: "yd"},
    0x27A1: { "uuid": 0x27A1, "unit": "parsec", "description": "length", abbreviation: ""},
    0x27A2: { "uuid": 0x27A2, "unit": "inch", "description": "length", abbreviation: "\""},
    0x27A3: { "uuid": 0x27A3, "unit": "foot", "description": "length", abbreviation: "f"},
    0x27A4: { "uuid": 0x27A4, "unit": "mile", "description": "length", abbreviation: "mi"},
    0x27A5: { "uuid": 0x27A5, "unit": "pound-force per square inch", "description": "pressure", abbreviation: "psi"},
    0x27A6: { "uuid": 0x27A6, "unit": "kilometre per hour", "description": "velocity", abbreviation: "kph"},
    0x27A7: { "uuid": 0x27A7, "unit": "mile per hour", "description": "velocity", abbreviation: "mph"},
    0x27A8: { "uuid": 0x27A8, "unit": "revolution per minute", "description": "angular velocity", abbreviation: "rpm"},
    0x27A9: { "uuid": 0x27A9, "unit": "gram calorie", "description": "energy", abbreviation: ""},
    0x27AA: { "uuid": 0x27AA, "unit": "kilogram calorie", "description": "energy", abbreviation: ""},
    0x27AB: { "uuid": 0x27AB, "unit": "kilowatt hour", "description": "energy", abbreviation: "kWh"},
    0x27AC: { "uuid": 0x27AC, "unit": "degree Fahrenheit", "description": "thermodynamic temperature", abbreviation: "°F"},
    0x27AD: { "uuid": 0x27AD, "unit": "", "description": "percentage", abbreviation: "%"},
    0x27AE: { "uuid": 0x27AE, "unit": "", "description": "per mille", abbreviation: ""},
    0x27AF: { "uuid": 0x27AF, "unit": "beats per minute", "description": "period", abbreviation: "bpm"},
    0x27B0: { "uuid": 0x27B0, "unit": "ampere hours", "description": "electric charge", abbreviation: "Ah"},
    0x27B1: { "uuid": 0x27B1, "unit": "milligram per decilitre", "description": "mass density", abbreviation: ""},
    0x27B2: { "uuid": 0x27B2, "unit": "millimole per litre", "description": "mass density", abbreviation: ""},
    0x27B3: { "uuid": 0x27B3, "unit": "year", "description": "time", abbreviation: "y"},
    0x27B4: { "uuid": 0x27B4, "unit": "month", "description": "time", abbreviation: "mo"}
};

var peripherals = {};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DISCOVER PERIPHERALS ////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


noble.on('discover', function(peripheral) {
	console.log("device " + peripheral.uuid + ": " + peripheral.advertisement.localName
        + " rssi=" + peripheral.rssi + "/" + peripheral.advertisement.txPowerLevel);
    peripheral.advertisement.serviceUuids.forEach(function(uuid) { console.log("\tadvertises service " + uuid); });

    noble.stopScanning();

    function log(message) {
        console.log(message);
        io.sockets.in("ble").emit("output", {stdout:message + "\n"});
    }
    function error(message) {
        console.error(message);
        io.sockets.in("ble").emit("log", {stderr:message + "\n"});
    }

    peripheral.connect(function(error) {

        var peripheralData = {
            uuid: peripheral.uuid,
            name: peripheral.advertisement.localName,
            rssi: peripheral.rssi,
            advertisement: peripheral.advertisement,
            discovered: new Date(),
            services: {}
        };

        var dest = "peripheral_" + peripheral.uuid;

        //peripheral.updateRssi();
        peripheral.on('rssiUpdate', function(error, rssi) {
            console.log("Peripheral " + peripheral.uuid + " rssi = " + peripheral.rssi + "/"+ peripheral.advertisement.txPowerLevel);
            peripheralData.rssi = peripheral.rssi;
            io.sockets.in(dest).emit(dest, peripheralData);
        });

        peripherals[peripheral.uuid] = {
            uuid: peripheral.uuid,
            peripheral: peripheral,
            data: peripheralData,
            services: {}
        };
        io.sockets.in("ble").emit("peripheral_connect", peripheralData);
        log("\t\tdiscovered at " + new Date())

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // DISCOVER SERVICES ///////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        peripheral.discoverServices();

        peripheral.on('servicesDiscover', function(services){
            console.log("Discovered services:");
            services.forEach(function(service){
                console.log("\tdiscovered service " + service.uuid);

                if (error) {
                    error(error);
                    return;
                }

                var serviceData = {
                    peripheral: peripheral.uuid,
                    uuid: service.uuid,
                    characteristics: {}
                };

                var per = peripherals[peripheral.uuid];
                per.services[service.uuid] = service;
                per.data.services[service.uuid] = serviceData;

                var d = {};
                d[service.uuid] = serviceData;


                io.sockets.in(dest).emit(dest, {uuid: peripheral.uuid, services: d});

                ////////////////////////////////////////////////////////////////////////////////////////////////////////
                // DISCOVER CHARACTERISTICS ////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////////////////////////////////////////////////////////////////////////////

                service.on('characteristicsDiscover', function(characteristics){
                    characteristics.forEach(function(characteristic) {
                        log("\t\thas characteristic " + characteristic);

                        var per = peripherals[peripheral.uuid];
                        var serv = per.services[service.uuid];
                        var chars = serv.characteristics;
                        chars[characteristic.uuid] = characteristic;

                        var charData = {
                            peripheral: peripheral.uuid,
                            service: service.uuid,
                            uuid: characteristic.uuid
                        };
                        per.data.services[service.uuid].characteristics[characteristic.uuid] = charData;

                        var p = {
                            peripheral: peripheral.uuid,
                            services:{}
                        };
                        var s = p.services[service.uuid] = {
                            service: service.uuid,
                            characteristics: {}
                        };
                        var c = s.characteristics[characteristic.uuid] = charData;

                        var dest = "peripheral_" + peripheral.uuid;
                        io.sockets.in(dest).emit(dest, p);

                        characteristic.on('descriptorsDiscover', function(descriptors) {

                            descriptors.forEach(function(descriptor) {

                                var dest = "peripheral_" + peripheral.uuid;

                                // look specifically for name or presentation format.
                                if (descriptor.uuid == '2901') {

                                    descriptor.readValue(function (err, data) {
                                        var val = data.toString('utf8');
                                        log("\t\t\tCharacteristic " + characteristic.uuid + " has name '" + val + "'");
                                        c.name = charData.name = characteristic.name = val;
                                        io.sockets.in(dest).emit(dest, p);
                                    });
                                } else if (descriptor.uuid == '2904') {
                                    descriptor.readValue(function (err, data) {
                                       /*
                                        typedef struct
                                        {
                                            uint8_t          format;      //Format of the value, see @ref BLE_GATT_CPF_FORMATS.
                                            int8_t           exponent;    // Exponent for integer data types.
                                            uint16_t         unit;        // UUID from Bluetooth Assigned Numbers.
                                            uint8_t          name_space;  // Namespace from Bluetooth Assigned Numbers, see @ref BLE_GATT_CPF_NAMESPACES.
                                            uint16_t         desc;        // Namespace description from Bluetooth Assigned Numbers, see @ref BLE_GATT_CPF_NAMESPACES.
                                        } ble_gatts_char_pf_t;
                                        */
                                        var format = data.readUInt8(0),
                                            exponent = data.readInt8(1),
                                            unit = data.readUInt16LE(2);

                                        if (format in ble_presentation_formats) {
                                            format = ble_presentation_formats[format];
                                        } else if (format == 0x9876) {
//                                            var output = new midi.output();
//                                            var port = output.openVirtualPort(peripheral.name);
//                                            characteristic.on("write", function(data) {
//                                                port.sendMessage(data);
//                                            })
                                        }
                                        if (unit in ble_units) {
                                            unit = ble_units[unit];
                                        }
                                        c.format = charData.format = characteristic.format = format;
                                        c.exponent = charData.exponent = characteristic.exponent = exponent;
                                        c.unit = charData.unit = characteristic.unit = unit;

                                        io.sockets.in(dest).emit(dest, p);
                                        log("\t\t\tCharacteristic " + characteristic.uuid + " has unit " + unit + ", format " + format[0] + ", exponent " + exponent );
                                    });
                                } else {
                                    // for some reason, attempts to retrieve some descriptor values, in particular
                                    // 2902 org.bluetooth.descriptor.gatt.client_characteristic_configuration, causes the nordic stack to hang.
                                    log("\t\t\tCharacteristic " + characteristic.uuid + " has descriptor " + descriptor);
                                }

                            });

                        });
                        characteristic.discoverDescriptors();

                    })

                });
                service.discoverCharacteristics();

            })


        });
    });

    peripheral.on("disconnect", function() {
        io.sockets.in("ble").emit("peripheral_disconnect", {peripheral: peripheral.uuid});
        log("device " + peripheral.uuid + " disconnected.");
        delete peripherals[peripheral.uuid];
        noble.startScanning();
    });
});
noble.startScanning();
console.log("Scanning for nrf...");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INCOMING SOCKET CONNECTIONS ////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

io.sockets.on('connection', function (socket) {
    function log(message) {
        console.log(message);
        socket.emit("output", {stdout:message + "\n"});
    }
    function error(message) {
        console.error(message);
        socket.emit("log", {stderr:message + "\n"});
    }

    function handleOutput(child) {
        child.stdout.setEncoding("utf8");
        child.stdout.on('data', function(data) {
            socket.emit('output', {pid:child.pid, stdout: data});
        });
        child.stderr.setEncoding("utf8");
        child.stderr.on('data', function(data) {
            socket.emit('output', {pid:child.pid, stderr: data});
        });
        child.on('error', function(err) {
            socket.emit('output', {pid: child.pid, status:"failed", error:err});
            console.error(err);
        });
    }

    function get_characteristic(data) {
        var peripheral = peripherals[data.peripheral];
        if (!peripheral) {
            log("No such peripheral " + data.peripheral);
            return null;
        }
        var service = peripheral.services[data.service];
        if (!service) {
            log("No such service " + data.service + " on peripheral " + data.peripheral);
            return null;
        }
        var characteristic = service.characteristics[data.characteristic];
        if (!characteristic) {
            log("No such characteristic " + data.characteristic + " for service " + data.service + " on peripheral " + data.peripheral);
            return null;
        }
        return {peripheral: peripheral, service: service, characteristic: characteristic};
    }

    socket.join("ble");

    socket.on("connect_peripheral", function(data) {
        socket.join("peripheral_" + data.peripheral);
        console.log("Socket " + socket.id + " joined " + "peripheral_" + data.peripheral);
        var peripheral = peripherals[data.peripheral];
        if (!peripheral) {
            console.log("Unknown peripheral " + data.peripheral)
        } else {
            socket.emit("peripheral_" + data.peripheral, peripheral.data);
        }

    });

    socket.on("peripheral_disconnect", function(data) {
        socket.leave("peripheral_" + data.peripheral);
        var peripheral = peripherals[data.peripheral];
        if (!peripheral) return;
        peripheral.peripheral.disconnect();
        peripheral.peripheral.once("disconnect", function() {
            log(data.peripheral + " disconnected");
        })
    });

    function doRead(c) {
        if (!c) return;
        var characteristic = c.characteristic,
            service = c.service,
            peripheral = c.peripheral;

        characteristic.once('read', function(data) {
            var charData = peripheral.data.services[service.uuid].characteristics[characteristic.uuid];

            var format = characteristic.format[0];

            var len = data.length;
            var val = null;

            if (format == 'utf8s') {
                val = data.toString('utf8');
            } else if (format == 'struct') {
                val = data.toString('base64');
            } else if (format == 'uint32') {
                val = data.readUInt32LE(0);
            } else if (format == "sint32") {
                val = data.readInt32LE(0);
            } else if (format == "float32") {
                val = data.readFloatLE(0);
            } else if (format == "float64") {
                val = data.readDoubleLE(0);
            } else {
                console.log("Unknown format " + format + " for characteristic " + characteristic.uuid);
                return;
            }

             // todo: more datatypes

            charData.value = val;

            var p = {
                peripheral: peripheral.uuid,
                services:{}
            };
            var s = p.services[service.uuid] = {
                service: service.uuid,
                characteristics: {}
            };
            var c = s.characteristics[characteristic.uuid] = charData;
            socket.emit('peripheral_' + peripheral.uuid, p);
            log("Peripheral " + peripheral.uuid + " read characteristic " + characteristic.uuid + " value: '" + val + "', length: " + len);
        });
        characteristic.read();
    }

    socket.on("read", function(data) {
        var c = get_characteristic(data);
        doRead(c);
    });

    socket.on("update_rssi", function(data) {
        var peripheral = peripherals[data.peripheral];
        if (!peripheral) {
            log("No such peripheral " + data.peripheral);
            return;
        }
        noble.updateRssi(peripheral.uuid);
    });

    function doWrite(characteristic, format, data, withoutResponse, callback) {
        var d;
        var size = 0;

        if (format == 'struct') {  // this lame.  need some better way.
            d = new Buffer(data, 'base64');
        } else if (format == 'utf8s') {
            d = new Buffer(data, "utf8")
        } else {
            console.log("Unknown format " + format + " for characteristic " + characteristic.uuid);
            return ;
        }
        size = d.length;

        if (callback) {
            characteristic.once('write', callback);
        }

        characteristic.write(d, withoutResponse);
        return size;
    }

    socket.on("write", function(data) {

        var c = get_characteristic(data);
        if (!c) return;
        //console.log("Writing to ", data);
        var characteristic = c.characteristic,
            service = c.service,
            peripheral = c.peripheral;

        var format;

        if ("format" in data) {
            format = data['format'];
        } else {
            if (!("format" in characteristic)) {
                console.log("No format for characteristic " + characteristic.uuid);
                return ;
            } else {
                format = characteristic.format[0];
            }
        }

        if (Array.isArray(data.data)) {
            var i = 0;

            var conn_interval = 12;
            var outstanding_writes = 6;
            var totalSize = 0;
            var start = new Date().getTime();

            function onDone() {

                var end = new Date().getTime();
                var dur = (end - start) / 1000.;
                log("Characteristic " + characteristic.uuid + ": " + data.data.length
                    + " elements totalling " + totalSize + " bytes written " + (use_write_without_response? "without" : "with") + " response in " + dur + " secs (" + parseFloat(Math.round(totalSize/dur * 100) / 100).toFixed(2) + " bytes/sec).");
                socket.emit("written_" + peripheral.uuid, {
                    characteristic: characteristic.uuid,
                    service: service.uuid,
                    peripheral: peripheral.uuid
                });

            }

            var callback = function() {

                function doIndvWrite(d, i) {
                    return doWrite(characteristic, format, d, use_write_without_response, function() {
                        //log("Characteristic " + characteristic.uuid + ": '" + d + "' written.");
                        if (!use_write_without_response) {
                            if (i == data.data.length-1) onDone();
                            if (i % outstanding_writes == 0) callback();
                        }
                    });
                }

                for (var j = 0; j < outstanding_writes && i < data.data.length; ++j, i++) {
                    var d = data.data[i];
                    totalSize += doIndvWrite(d, i);

                }


                if (use_write_without_response) {
                    if (i == data.data.length) {
                        onDone();
                    } else {
                        setTimeout(callback, conn_interval);
                    }
                }

            };
            callback();
        } else {
            var callback = function() {
                //log("Characteristic " + characteristic.uuid + ": '" + data.data + "' written.");
                socket.emit("written_" + peripheral.uuid, {
                    characteristic: characteristic.uuid,
                    service: service.uuid,
                    peripheral: peripheral.uuid
                })
            };

            doWrite(characteristic, format, data.data, false, callback);
        }


    });

    socket.on("make", function() {
        child = spawn("/usr/bin/make", [], {
            cwd: srcdir
        });
        child.on('exit', function (code) {
            console.log("made (" + child.pid+ "): " + code);
            socket.emit('made', {pid: child.pid, status:code });
        });
        handleOutput(child);
        console.log("making (" + child.pid + ")...");
    });

    socket.on("render", function(data) {
        var name = crypto.randomBytes(20, function(ex, buf) {
            var token = buf.toString('hex');

            var imgfile = "/tmp/" + token + ".png";

            child = spawn(__dirname + "/" + srcdir + "render", [imgfile], {
                cwd: srcdir
            });
            child.on('exit', function (code) {
                console.log("rendered (" + child.pid+ "): " + code);

                if (code != 0 || ! fs.existsSync(imgfile)) {
                    socket.emit('rendered', {pid: child.pid, status:code});
                } else {
                    fs.readFile(imgfile, function (err, data) {
                        if (err) {
                            socket.emit('rendered', {pid:child.pid, error: err});
                        } else {
                            var img = "data:image/png;base64," + data.toString('base64');
                            socket.emit('rendered', {pid:child.pid, img: img});
                        }
                    });
                }
            });
            handleOutput(child);
            child.stdin.setEncoding("utf8");
            child.stdin.end(data.data);

        });

    });


    // send the current values of all peripherals:

    for(var uuid in peripherals) {
        var peripheral = peripherals[uuid];
        socket.emit("peripheral_connect", peripheral.data);
    }


});



