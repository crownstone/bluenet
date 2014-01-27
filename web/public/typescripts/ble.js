define(["require", "exports"], function(require, exports) {
    function merge(json, obj) {
        for(var key in json) {
            var val = json[key];
            if(typeof val != 'object') {
                obj[key = val];
            }
        }
    }
    var PeripheralRegistry = (function () {
        function PeripheralRegistry() {
            this.peripheralsByUid = {
            };
            this.peripherals = [];
            var self = this;
            $(document).ready(function () {
                self.socket = io.connect('http://localhost');
                self.socket.on("peripheral_connect", function (data) {
                    self.onMessage(data);
                });
            });
        }
        PeripheralRegistry.prototype.onMessage = function (obj) {
            var peripheral = this.peripheralsByUid[obj.uuid];
            if(!peripheral) {
                peripheral = this.peripheralsByUid[obj.uuid] = new Peripheral(obj.uuid);
                this.peripherals.push(peripheral);
                this.peripherals.sort(function (a, b) {
                    return a.compareTo(b);
                });
            }
            peripheral.onMessage(obj);
        };
        PeripheralRegistry.prototype.disconnect = function (peripheral_uuid) {
        };
        return PeripheralRegistry;
    })();
    exports.PeripheralRegistry = PeripheralRegistry;    
    var Peripheral = (function () {
        function Peripheral(uuid) {
            this.servicesByUid = {
            };
            this.services = [];
            this.uuid = uuid;
        }
        Peripheral.prototype.onMessage = function (obj) {
            if(obj.uuid == this.uuid) {
                merge(obj, this);
                for(var uuid in obj.services) {
                    var serviceData = obj.services[uuid];
                    var service = this.servicesByUid[uuid];
                    if(service == null) {
                        service = this.servicesByUid[serviceData.uuid] = new Service(uuid);
                        this.services.push(service);
                        this.services.sort(function (a, b) {
                            return a.compareTo(b);
                        });
                    }
                    service.onMessage(serviceData);
                }
            }
        };
        Peripheral.prototype.compareTo = function (rhs) {
            return (this.name && rhs.name) ? this.name.localeCompare(rhs.name) : this.discovered.getTime() - rhs.discovered.getTime();
        };
        return Peripheral;
    })();
    exports.Peripheral = Peripheral;    
    var Service = (function () {
        function Service(uuid) {
            this.characteristicsByUid = {
            };
            this.characteristics = [];
            this.uuid = uuid;
        }
        Service.prototype.onMessage = function (obj) {
            if(obj.uuid == this.uuid) {
                merge(obj, this);
                for(var uuid in obj.characteristics) {
                    var charData = obj.characteristics[uuid];
                    var characteristic = this.characteristics[uuid];
                    if(characteristic == null) {
                        characteristic = this.characteristicsByUid[uuid] = new Characteristic(uuid);
                        this.characteristics.push(characteristic);
                        this.characteristics.sort(function (a, b) {
                            return a.compareTo(b);
                        });
                    }
                    characteristic.onMessage(charData);
                    console.log("read");
                }
            }
        };
        Service.prototype.compareTo = function (rhs) {
            return (this.name && rhs.name) ? this.name.localeCompare(rhs.name) : this.uuid.localeCompare(rhs.uuid);
        };
        return Service;
    })();
    exports.Service = Service;    
    var Characteristic = (function () {
        function Characteristic(uuid) {
            this.uuid = uuid;
        }
        Characteristic.prototype.onMessage = function (obj) {
            for(var key in obj) {
                this[key] = obj[key];
            }
        };
        Characteristic.prototype.compareTo = function (rhs) {
            return (this.name && rhs.name) ? this.name.localeCompare(rhs.name) : this.uuid.localeCompare(rhs.uuid);
        };
        return Characteristic;
    })();
    exports.Characteristic = Characteristic;    
    var TreeController = (function () {
        function TreeController() { }
        return TreeController;
    })();
    exports.TreeController = TreeController;    
    window['ble'] = new PeripheralRegistry();
})
//@ sourceMappingURL=ble.js.map
