

$(document).ready(function() {

    function disconnected() {
        selected_peripheral = null;
        $("#code").attr('disabled', true).val("");
        $("#write").attr('disabled', true);
        $("#disconnect").attr('disabled', true);

        $('.peripheral').text("(Not connected.)");
        $('.peripheral_id').text("");
        $("#characteristic").text("");
        $("#characteristic_id").text("");
    }

    disconnected();

    function forEach(object, block, context) {
        for (var key in object) {
            if ((!this.prototype) || typeof this.prototype[key] == "undefined") {
                block.call(context, object[key], key, object);
            }
        }
    };

    var selected_peripheral = null;
    var selected_service = null;
    var selected_characteristic = null;

    var socket = io.connect();
    socket.on('output', function (data) {
        if (data.error) {
            $('#error')
                .text(data.error)
                .show(50)
            ;
            $(".output-header").addClass("failed");
        }
        if (data.status) {
            if (data.status != 0) {
                $(".output-header").addClass("failed");
            } else {
                $(".output-header").removeClass("failed");
                $(".output-header").addClass("succeeded");
            }
        }
        var txt = "";
        if (data.stderr) txt += data.stderr.replace(/\r/g, "<br>");
        if (data.stdout) txt += data.stdout.replace(/\r/g, "<br>");
        $('#output').html( $('#output').html() + txt + "<br>");
        $("#output").animate({ scrollTop: $("#output")[0].scrollHeight}, 100);
    });

    socket.on("rendered", function(data) {
        if (data.img) $("#rendered").attr("src", data.img);
    });

    socket.on("peripheral_connect", function(data){
        if (selected_peripheral) {
            disconnected();
        }
        selected_peripheral = data;
        $('.peripheral').text("Connected to " + data.name);
        $('.peripheral_id').text("(" + data.uuid + ")");
        $("#code").val("");

        socket.emit("connect_peripheral", {
            peripheral: selected_peripheral.uuid
        });

        function on_peripheral(data) {
            forEach(data.services, function(svc) {
                forEach(svc.characteristics, function(characteristic) {
                    if (characteristic.name == 'text') {
                        selected_service = svc;
                        if (!selected_characteristic) {
                            selected_characteristic = characteristic;
                            socket.emit("read", {
                                peripheral: selected_peripheral.uuid,
                                service: selected_service.uuid,
                                characteristic: characteristic.uuid
                            })
                        } else {
                            for(var key in characteristic) {
                                selected_characteristic[key] = characteristic[key];
                                if (key == "value") {
                                    $("#code").val(characteristic[key])
                                }
                            }
                        }
                        $("#characteristic").text(characteristic.name + ":");
                        $("#characteristic_id").text("(" + characteristic.uuid + ")");
                        $("#code").attr('disabled', false).focus();
                        $("#write").attr('disabled', false);
                        $("#disconnect").attr('disabled', false);
                    }
                })
            })
        }

        on_peripheral(data);

        socket.on("peripheral_" + selected_peripheral.uuid, on_peripheral)
    });


    socket.on("peripheral_disconnect", disconnected);
    socket.on("disconnect", disconnected);

    $("#code").keyup(function(evt) {
        if (evt.keyCode == 13) {
            evt.preventDefault();
            write();
        }
    });

    $('input,select').keypress(function(event) { return event.keyCode != 13; });

    function clearOutput() {
        $('#error')
            .text('')
            .hide(50);
        $('#output').text("");
        $(".output-header").removeClass("failed");
    }

    function write() {
        var text = $("#code").val();
        console.log(text);
        if (selected_peripheral) {
            socket.emit("write", {
                peripheral: selected_peripheral.uuid,
                service: selected_characteristic.service,
                characteristic: selected_characteristic.uuid,
                data: text});
        }
    }

    $("#make").click(function() {
        clearOutput();
        socket.emit("make");
    });
    $("#run").click(function() {
        clearOutput();
        socket.emit("render", {data: $("#code").val()});
    });

    $("#write").click(write);
    $("#disconnect").click(function() {
        if (selected_peripheral) {
            socket.emit("peripheral_disconnect", {peripheral: selected_peripheral.uuid});
        }
    })

});


