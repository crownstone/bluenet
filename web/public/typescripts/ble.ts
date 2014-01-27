
/// <reference path="vendor/jquery.d.ts"/>
/// <reference path="vendor/d3types.ts" />
/// <reference path="vendor/socket.io.d.ts" />



function merge(json:Object, obj:Object) {
    for(var key in json) {
        var val = json[key]
        if (typeof val != 'object') {
            obj[key = val];
        }
    }
}

export class PeripheralRegistry {

    peripheralsByUid:{ [uuid: string]: Peripheral; } = {};
    public peripherals:Peripheral[] = [];
    socket:Socket;

    constructor() {
        var self = this;

        $(document).ready(function() {

            self.socket = io.connect('http://localhost');

            self.socket.on("peripheral_connect", function(data) {
                self.onMessage(data);
            });

        });
    }

    public onMessage(obj) {

        var peripheral = this.peripheralsByUid[obj.uuid];
        if (!peripheral) {
            peripheral = this.peripheralsByUid[obj.uuid] = new Peripheral(obj.uuid);
            this.peripherals.push(peripheral);
            this.peripherals.sort(function(a,b) {
                return a.compareTo(b);
            })
        }
        peripheral.onMessage(obj);
    }

    public disconnect(peripheral_uuid:string) {

    }

}


export class Peripheral {

    constructor(uuid:string) {
        this.uuid = uuid;
    }


    public uuid:string;
    public name:string;
    public discovered:Date;
    servicesByUid:{ [uuid: string]: Service; } = {};
    public services:Service[] = [];

    public onMessage(obj) {

        if (obj.uuid == this.uuid) {
            merge(obj, this);

            for(var uuid in obj.services) {
                var serviceData = obj.services[uuid];
                var service = this.servicesByUid[uuid];
                if (service == null) {
                    service = this.servicesByUid[serviceData.uuid] = new Service(uuid);
                    this.services.push(service);
                    this.services.sort(function(a,b) {
                        return a.compareTo(b);
                    })
                }
                service.onMessage(serviceData);
            }
        }

    }

    public compareTo(rhs:Peripheral) {
        return (this.name && rhs.name) ? this.name.localeCompare(rhs.name) : this.discovered.getTime() - rhs.discovered.getTime();
    }

}

export class Service {
    public uuid:string;
    public name:string;

    characteristicsByUid:{ [uuid: string]: Characteristic; } = {};
    public characteristics:Characteristic[] = [];

    constructor(uuid:string) {
        this.uuid = uuid;
    }

    public onMessage(obj) {
        if (obj.uuid == this.uuid) {
            merge(obj, this);

            for(var uuid in obj.characteristics) {
                var charData = obj.characteristics[uuid];
                var characteristic = this.characteristics[uuid];
                if (characteristic == null) {
                    characteristic = this.characteristicsByUid[uuid] = new Characteristic(uuid);
                    this.characteristics.push(characteristic);
                    this.characteristics.sort(function(a, b) {
                        return a.compareTo(b);
                    });
                }
                characteristic.onMessage(charData);
                console.log("read");
            }
        }
    }

    public compareTo(rhs:Service) {
        return (this.name && rhs.name) ? this.name.localeCompare(rhs.name) : this.uuid.localeCompare(rhs.uuid);
    }

}

export class Characteristic {

    uuid:string;
    name:string;

    constructor(uuid:string) {
        this.uuid = uuid;
    }

    public onMessage(obj:Object) {
        for(var key in obj) {
            this[key] = obj[key];
        }


    }

    public compareTo(rhs:Characteristic) {
        return (this.name && rhs.name) ? this.name.localeCompare(rhs.name) : this.uuid.localeCompare(rhs.uuid);
    }

}

export class TreeController {


}




window['ble'] = new PeripheralRegistry();

