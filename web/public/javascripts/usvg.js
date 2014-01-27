
$(document).ready(function() {

    var debug = 0;

    var template = Handlebars.templates.bus_screen_wide_176_simple;
    var height = 176; // TODO read this from display.
    var width = 264;

    // configurable /////////////////////////////
    var forecastKey = "54f9a39dd33ac6c6c7196056d41c5685";
    var trimetAppId = "79A8324E3881B1758002B93DB";
    var stopId = 971; //9301;//948; //
    var routes = [44];

    var n_buses = 2;
    var y_text = 78;
    var y_icon = 52;
    var y_increment = 32;
    var bus_freq_msecs = 1000 * 60; // once a minute
    var out_the_door_time_msecs = 5 * 60 * 1000; // 5 mins
    var forecast_req_msecs = 1000 * 60 * 30; // twice an hour
    var time_freq_msecs = 1000 * 20;

    var run_length_encode = true; // use http://en.wikipedia.org/wiki/Elias_gamma_coding ?

    var info_chars = {"writes":{}, "outOfOrder":{}, "drawTime": {}};

    var max_len = 20;

    var len = 0;

    var writing = false;
    var queuedWrite = false;
    var listeners = {}; // randomString -> tuple of [object, name, function]

    var selected_peripheral;
    var selected_service;
    var selected_characteristic;
    var selected_format;
    var peripheral_handler;

    var latitude,longitude;

    // find stops:
    // http://developer.trimet.org/ws/V1/stops?ll=45.470709,-122.710&feet=1000&appId=79A8324E3881B1758002B93DB&json=true&showRoutes=true

    // the data that gets sent to the display.
    var data = {

    };

    // socket used to talk to the server.
    var socket = io.connect();

    function disconnected() {
        socket.removeListener("peripheral_" + selected_peripheral.uuid, peripheral_handler);
        selected_peripheral = selected_service = selected_characteristic = peripheral_handler = null;
        $("#status").text("(not connected)");
        writing = false;
        forEach(listeners, function(x, key) {
            x[0].removeListener(x[1], x[2]);
        });
        listeners = {};

        $("#rssi_container").text("").hide(200);

        for(var key in info_chars) {
            info_chars[key] = {};
            $("#" + key + "_container").hide(200);
            $("#" + key).text("");
        }

    }

    $("#status").text("(waiting for connection)");

    function testWrite() {
        if (selected_characteristic) {
            var data = len + " hello ";
            while(data.length < len) {
                data += "x";
            }
            len++;
            socket.emit("write", {
                peripheral: selected_peripheral.uuid,
                service: selected_characteristic.service,
                characteristic: selected_characteristic.uuid,
                data: data});
            console.log(data + " written");
        }
    }

    function render() {

        data.current_time = new Date().format("h:MM");
        data.current_date = new Date().format("ddd dd mmm");
        data.current_day_of_week = new Date().format("ddd");
        data.current_month = new Date().format("mmm");
        data.current_day = new Date().format("dd");



        var svgText = template(data);

        var svgel = $("#svg");
        svgel.html(svgText);

        var img =  $("#img");
        img.attr("height", height);
        img.attr("width",width);

        var canvas = $("#canvas")[0];

        var ctx = canvas.getContext("2d");
        ctx.imageSmoothingEnabled = false;
        ctx.webkitImageSmoothingEnabled = false;
        ctx.mozImageSmoothingEnabled = false;
        canvas.height = height;
        canvas.width = width;

        var ctx = canvas.getContext("2d");
        ctx.fillStyle = "#fff";

        ctx.fillRect(0,0,width,height);
        ctx.fillStyle = "#000";
        ctx.drawSvg(svgText, 0, 0, width, height);

        function doWrite() {

            //ctx.drawImage(img[0], 0, 0, width, height);


            var imgd = ctx.getImageData(0,0,width,height).data;

            var x = 0,
                y = 0,
                h = height,
                w = width;

            var flipped = false;
            if (h>w) {
                var tmp = w;
                w = h;
                h = tmp;
                flipped = true;
            }

            var header_len = 6;
            var messages = [];
            var msg_count = 0;  // modulo 256

            var totalSize = 0;
            var backups = 0;

            var debugstr = "";
            var countCounts = {};  // hash from run length -> number of occurrences of that run length

            while(x < w && y < h) {  // this loop happens for every message sent.

                var current_run_color = null;

                var xx = x;
                var yy = y;

                // how many bytes do we have to store image data in a message (excluding header bytes)?
                var image_len = Math.min(w * h - yy*w + x - header_len, max_len - header_len);

                var buffer = new ArrayBuffer(image_len + header_len);
                var dview = new DataView(buffer);

                var i = 0;
                var count = 0;

                // Each message begins with a 6 byte header:

                // 1 byte command
                dview.setInt8(i, (run_length_encode? 'P' :'p').charCodeAt(0)); // command: pixels (or run length encoded Pixels).
                i += 1;

                // 1 byte message counter (for detecting out of order delivery).
                // this is helpful in the client so it can order messages and it makes the
                // uint16's below have proper alignment.
                dview.setUint8(i, msg_count++);
                msg_count %= 256;
                i += 1;

                // 2 byte x coordinate at which this message's image data begins.
                // By sending coordinates, if messages are lost or received out of order we have fidelity.
                dview.setInt16(i, x, true);
                i+=2;

                // 2 byte y coordinate.
                dview.setInt16(i, y, true);
                i+=2;

                // end header.

                var b = 0;
                var mask = 0;
                var run_start_x = x;
                var run_start_y = y;
                var backup = false;

                // The rest of the message is image data pixels:
                while(i < max_len && x < w && y < h) {

                    // Set bit to the b/w color of pixel (x,y) set in svg image. Look at all three image color values and alpha.
                    var index = (4 * (flipped ? (y + (w-x) * h) : (x + y * w)));
                    var bit = 0;
                    for(var j = 0; j < 3; ++j) {
                        if (imgd[index+j] < 150) bit = 1;
                    }

                    if (run_length_encode) {

                        // Run length encoded image data consists of a single bit indicating whether the first pixel is
                        // black or white.  This is followed by as many Elias-encoded run counts as will fit into the message.

                        function write_count(zeroes, count) {
                            if (!(count in countCounts)) countCounts[count] = 0;
                            countCounts[count]++;

                            var cnt = zeroes;
                            while(cnt > 1) { // 10001 -> 0000
                                mask++; // leave a zero in this spot.
                                if (mask == 8) {
                                    dview.setUint8(i++, b);
                                    b = 0;
                                    mask = 0;
                                }
                                cnt--;
                            }
                            cnt = zeroes;
                            while (cnt > 0) {  // output high (guaranteed to be 1) bit first.
                                // 10001 -> 10001 : final: 000010001
                                var digit = count & (1 << (--cnt));
                                b |= (digit ? 1 : 0) << mask;
                                dview.setUint8(i, b);
                                mask++;
                                if (mask == 8) {
                                    if (i == max_len) {
                                        throw new Error("out of space.");
                                    }
                                    dview.setUint8(i++, b);
                                    b = 0;
                                    mask = 0;
                                }
                            }
                        }


                        if (current_run_color == null) {
                            current_run_color = bit;
                            b |= current_run_color;
                            mask++;
                            count++;
                        } else if (bit == current_run_color
                            && !(x == w-1 && y == h-1)) { // force us to output if we're the last pixel.

                            count++;
                        } else {
                            if (count <= 0) throw new Error("Can't have 0 count.");

                            var zeroes;

                            // count leading zeroes
                            for(zeroes = 31; zeroes >= 0; --zeroes) {
                                if (count & (1 << zeroes)) {
                                    break;
                                }
                            }
                            zeroes++;   // ...00000000000010001 -> 5

                            var len = zeroes + (zeroes-1);

                            // do we need more bits than we have left in this message?
                            if (((max_len - i -1) * 8 + (8 - mask)) < len) {
                                // hm what to do here?
                                // well, we could try to hold the bits over to the next message,
                                // but it's easier to simply back up to the start of this run
                                // and begin the next message there.  we'll do the actual logic of this
                                // below instead of advancing x.

                                backup = true;
                                backups++;
                                count = 0;

                            } else {
                                write_count(zeroes, count);

                                if (debug >=2) {
                                    if (debugstr.length != 0) debugstr += " ";
                                    debugstr += (current_run_color ? "b" : "w" ) + count;

                                }

                                run_start_x = x; // keep track of where the current run started, in case we have to back up.
                                run_start_y = y;

                                current_run_color = bit;
                                count = 1;

                            }


                        }
                    } else {
                        // this is the non-RLE path.  simpler, eh?

                        b |= bit << mask;
                        mask++;
                        if (mask == 8) {
                            dview.setUint8(i, b);
                            b = 0;
                            mask = 0;
                            i++;
                        }
                    }

                   // if (i != max_len) {
                        if (!backup) {
                            if (i!= max_len ) { //&& !(i == max_len-1 && mask == 7)
                                x++;
                            } else {
                                i++;
                            }
                            if (x == w) {
                                x = 0;
                                y++;
                            }
                        } else {

                            x = run_start_x;
                            y = run_start_y;
                            break;
                        }
                    //}

                }
                if (mask != 0) i++; // good to the last byte!  wasted about a day on this line.

                totalSize += i;
                var message = base64ArrayBuffer(buffer);

                if (debug >= 2) {
                    console.log('\t\t"' + message + '", // ' + xx + ', '+ yy + ": " + debugstr + " (" + (x + y * width) + ")" + (backup ? " backup" : ""));
                    debugstr = "";
                }

                messages.push(message);



                function printImage() {  // for testing.
                    var str = "";
                    var xx = 0,
                        yy = 0;
                    for (var ii = header_len; ii < max_len && yy < rows; ++ii) {
                        b = dview.getUint8(ii);
                        for(var j = 0; j < 8; ++j) {
                            str += (b & (1 << j)) ? "1" : "0";
                            xx++;
                            if (xx == w) {
                                console.log(str);
                                str = "";
                                xx = 0;
                                yy++;
                            }
                        }
                    }
                }
            }

            console.log("Encoded " + w + "x" + h + " image (" + (w*h/8) + " bytes raw) as " + messages.length + " messages totaling "
                + totalSize + " bytes (" + Math.round(totalSize/(w*h/8) * 100) + "% compression). " + backups + " backups.")

            if (selected_characteristic && ("format" in selected_characteristic) && (messages.length > 0) && !writing ) {

                // this is the logic that writes the command to update the screen after all the pixel messages
                // have been received.  This prrrobably belongs on the server somehow.  well, that doesn't really distinguish it...

                console.log('writing');
                writing = true;

                var dest = 'written_' + selected_peripheral.uuid;
                var writeCallbackId = randomString(10);
                var writeCallback = function(data){
                    delete listeners[writeCallbackId];

                    writeCallbackId = randomString(10);
                    var b = new ArrayBuffer(1);
                    var dv = new DataView(b);
                    dv.setInt8(0, 'd'.charCodeAt(0)); // command: display
                    writeCallback = function(data) {
                        console.log("Written.");
                        writing = false;

                        // read some statistics back from the device.  this is all just for debugging.

                        forEach(info_chars, function(characteristic) {
                            if (!characteristic.uuid) return;
                            socket.emit("read", {
                                "peripheral": characteristic.peripheral,
                                "service": characteristic.service,
                                "characteristic": characteristic.uuid
                            });
                        });

                        // update our signal strength reading.
                        socket.emit("update_rssi", {
                            peripheral: selected_peripheral.uuid
                        });

                        if (queuedWrite) {
                            queuedWrite = false;
                            render();
                        }

                    };
                    listeners[writeCallbackId] = [socket, dest, writeCallback];
                    socket.once(dest, writeCallback);

                    socket.emit("write", {
                        peripheral: selected_peripheral.uuid,
                        service: selected_characteristic.service,
                        characteristic: selected_characteristic.uuid,
                        data: base64ArrayBuffer(b)});

                };

                socket.once(dest, writeCallback);
                listeners[writeCallbackId] = [socket, dest, writeCallback];

                socket.emit("write", {
                    peripheral: selected_peripheral.uuid,
                    service: selected_characteristic.service,
                    characteristic: selected_characteristic.uuid,
                    data: messages});
                console.log("written:  x=" + xx + ", y="  + yy + ", ");

            } else {
                if (!selected_characteristic) {
                    console.log("not writing because no characteristic is selected.");
                } else if (!("format" in selected_characteristic)){
                    console.log("not writing because we don't have a format for " + selected_characteristic.uuid + " yet.");
                } else if ( messages.length == 0) {
                    console.log("not writing because there are no messages.");
                } else if (writing) {
                    console.log("not writing because we're already writing.");
                }
                queuedWrite = true;
            }
        }

        var dataurl = "data:image/svg+xml," + encodeURIComponent(svgText);

        if (img.attr("src") != dataurl) {
            img.on("load", doWrite);
            img.attr("src", dataurl);
        } else {
            doWrite();
        }

    }

    window.render = render;
    window.testWrite = testWrite;


    socket.on("peripheral_connect", function(data){
        if (selected_peripheral) {
            disconnected();
        }
        selected_peripheral = data;
        $('#status').text("Connected to " + data.name);
        writing = false;

        socket.emit("connect_peripheral", {
            peripheral: selected_peripheral.uuid
        });

        function on_peripheral(data) {
            if ('rssi' in data) {
                $("#rssi_container").show(200);
                $("#rssi").text(data.rssi);
            }

            forEach(data.services, function(svc) {


                forEach(svc.characteristics, function(characteristic) {
                    if (characteristic.name == 'bitmap') {
                        selected_service = svc;
                        if (!selected_characteristic) {
                            selected_characteristic = characteristic;
                            render();
                        } else {
                            for(var key in characteristic) {
                                var value = characteristic[key];
                                selected_characteristic[key] = value;

                                if (key == 'format' && selected_format != value[0]) {
                                    selected_format = value[0];
                                    render();
                                }
                            }

                        }

                    } else if (characteristic.name in info_chars) {
                        var hash = info_chars[characteristic.name];
                        var hadValue = false;
                        for(var key in characteristic) {
                            var value = characteristic[key];

                            hash[key] = value;
                            if (key == "value") {
                                hadValue = true;
                            }
                        }
                        if (hadValue) {
                            var value = hash["value"];
                            if ("format" in characteristic) {
                                if (characteristic.format[0].indexOf("float") == 0) {
                                    value = value.toPrecision(3);
                                }
                            }
                            $("#" + characteristic.name + "_container").show(200);
                            $("#" + characteristic.name ).text(value);
                        }

                    }
                })
            })
        }

        on_peripheral(data);

        socket.on("peripheral_" + selected_peripheral.uuid, on_peripheral);

        peripheral_handler = on_peripheral;
    });


    socket.on("peripheral_disconnect", disconnected);

    function fetchBus() {
        var busReq = new XMLHttpRequest();

        busReq.onload = function() {
            var busData = JSON.parse(this.responseText);

            data.buses = {
                arrival: []
            };

            var loc = busData.resultSet.location[0];

            $("#stop").show(200).text(routes.join(", ") + " " + loc.dir + " at " + loc.desc);


            var i = 0;
            busData.resultSet.arrival.forEach(function(arrival) {
                if (routes.indexOf(arrival.route) == -1) return;
                if (i > n_buses) return;
                var time = new Date(arrival.estimated ? arrival.estimated : arrival.scheduled);
                var timediff = time - new Date();
                var soon = timediff < 0 || timediff < out_the_door_time_msecs;
                var bold = soon ? "b" : "";
                if (!arrival.estimated) bold += " i";  // show estimated times in italic.

                data.buses.arrival.push({
                    time : time.format("h:MM"),
                    index: i,
                    y_text: y_text + i * y_increment,
                    y_icon: y_icon + i * y_increment,
                    icon : soon ? "out_the_door" : "bus",
                    bold : bold
                });
                i++;
            });

            render();
        }

        busReq.open("get", "http://developer.trimet.org/ws/V1/arrivals?locIDs=" + stopId + "&json=true&appID="+trimetAppId);
        busReq.send();
    }

    function fetchForecast() {
        if (!(latitude&&longitude)) return;
        $.ajax({
            url: "https://api.forecast.io/forecast/" + forecastKey + "/" + latitude + "," + longitude + "?callback=?",
            dataType: "json"
        }).done(function(weatherData) {

            var today = weatherData.daily.data[0];
            data.weather = {
                icon: today.icon,
                current_temp: Math.round(weatherData.currently.temperature),
                high_temp: Math.round(today.temperatureMax),
                percip_prob: today.precipProbability && today.icon == "rain" ? Math.round(today.precipProbability * 100) + "%" : ""
            };

            render();

        });
    }

    setInterval(render, time_freq_msecs);
    setInterval(fetchBus, bus_freq_msecs);
    setInterval(fetchForecast, forecast_req_msecs);

    fetchBus();

    navigator.geolocation.getCurrentPosition(function(position) {

        latitude = position.coords.latitude;
        longitude = position.coords.longitude;
        fetchForecast();

    });


});


function base64ArrayBuffer(arrayBuffer) {
    var base64    = ''
    var encodings = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

    var bytes         = new Uint8Array(arrayBuffer)
    var byteLength    = bytes.byteLength
    var byteRemainder = byteLength % 3
    var mainLength    = byteLength - byteRemainder

    var a, b, c, d
    var chunk

    // Main loop deals with bytes in chunks of 3
    for (var i = 0; i < mainLength; i = i + 3) {
        // Combine the three bytes into a single integer
        chunk = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2]

        // Use bitmasks to extract 6-bit segments from the triplet
        a = (chunk & 16515072) >> 18 // 16515072 = (2^6 - 1) << 18
        b = (chunk & 258048)   >> 12 // 258048   = (2^6 - 1) << 12
        c = (chunk & 4032)     >>  6 // 4032     = (2^6 - 1) << 6
        d = chunk & 63               // 63       = 2^6 - 1

        // Convert the raw binary segments to the appropriate ASCII encoding
        base64 += encodings[a] + encodings[b] + encodings[c] + encodings[d]
    }

    // Deal with the remaining bytes and padding
    if (byteRemainder == 1) {
        chunk = bytes[mainLength]

        a = (chunk & 252) >> 2 // 252 = (2^6 - 1) << 2

        // Set the 4 least significant bits to zero
        b = (chunk & 3)   << 4 // 3   = 2^2 - 1

        base64 += encodings[a] + encodings[b] + '=='
    } else if (byteRemainder == 2) {
        chunk = (bytes[mainLength] << 8) | bytes[mainLength + 1]

        a = (chunk & 64512) >> 10 // 64512 = (2^6 - 1) << 10
        b = (chunk & 1008)  >>  4 // 1008  = (2^6 - 1) << 4

        // Set the 2 least significant bits to zero
        c = (chunk & 15)    <<  2 // 15    = 2^4 - 1

        base64 += encodings[a] + encodings[b] + encodings[c] + '='
    }

    return base64
}

function forEach(object, block, context) {
    for (var key in object) {
        if ((!this.prototype) || typeof this.prototype[key] == "undefined") {
            block.call(context, object[key], key, object);
        }
    }
}

function randomString(n) {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for( var i=0; i < n; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}
