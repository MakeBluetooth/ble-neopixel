// Bluetooth Low Energy NeoPixel Lamp (c) 2015 Don Coleman

// NeoPixel Service UUIDs
var NEOPIXEL_SERVICE = 'ccc0';
var COLOR = 'ccc1';
var BRIGHTNESS = 'ccc2';
var POWER_SWITCH = 'ccc3';

var app = {
    initialize: function() {
        this.bind();
    },
    bind: function() {
        document.addEventListener('deviceready', this.deviceready, false);
        colorScreen.hidden = true;
    },
    deviceready: function() {

        // wire buttons to functions
        deviceList.ontouchstart = app.connect;
        refreshButton.ontouchstart = app.scan;
        disconnectButton.ontouchstart = app.disconnect;

        red.onchange = app.onColorChange;
        green.onchange = app.onColorChange;
        blue.onchange = app.onColorChange;
        brightness.onchange = app.onBrightnessChange;

        powerSwitch.onchange = app.onSwitchChange;
        app.scan();
    },
    scan: function(e) {
        deviceList.innerHTML = ""; // clear the list
        app.setStatus("Scanning for Bluetooth Devices...");

        // iOS and Newer Android
        ble.startScan([NEOPIXEL_SERVICE],
            app.onDeviceDiscovered,
            function() { app.setStatus("Listing Bluetooth Devices Failed"); }
        );

        // stop scan after 5 seconds
        setTimeout(ble.stopScan, 5000, app.onScanComplete);

    },
    onDeviceDiscovered: function(device) {
        var listItem, rssi;

        console.log(JSON.stringify(device));
        listItem = document.createElement('li');
        listItem.dataset.deviceId = device.id;
        if (device.rssi) {
            rssi = "RSSI: " + device.rssi + "<br/>";
        } else {
            rssi = "";
        }
        listItem.innerHTML = device.name + "<br/>" + rssi + device.id;
        deviceList.appendChild(listItem);

        var deviceListLength = deviceList.getElementsByTagName('li').length;
        app.setStatus("Found " + deviceListLength + " device" + (deviceListLength === 1 ? "." : "s."));
    },
    onScanComplete: function() {
        var deviceListLength = deviceList.getElementsByTagName('li').length;
        if (deviceListLength === 0) {
            app.setStatus("No Bluetooth Peripherals Discovered.");
        }
    },
    connect: function (e) {
        app.setStatus("Connecting...");
        var deviceId = e.target.dataset.deviceId;
        console.log("Requesting connection to " + deviceId);
        ble.connect(deviceId, app.onConnect, app.onDisconnect);
    },
    disconnect: function(event) {
        app.setStatus("Disconnecting...");
        ble.disconnect(app.connectedPeripheral.id, app.onDisconnect);
    },
    onConnect: function(peripheral) {
        app.connectedPeripheral = peripheral;
        connectionScreen.hidden = true;
        colorScreen.hidden = false;
        app.setStatus("Connected.");
        app.syncUI();

        // notifications update the UI if the values change on the light
        ble.startNotification(peripheral.id, NEOPIXEL_SERVICE, BRIGHTNESS, function(buffer) {
            var data = new Uint8Array(buffer);
            brightness.value = data[0];
        });

        ble.startNotification(peripheral.id, NEOPIXEL_SERVICE, POWER_SWITCH, function(buffer) {
          var data = new Uint8Array(buffer);
          powerSwitch.checked = data[0] !== 0;
        });

    },
    onDisconnect: function() {
        connectionScreen.hidden = false;
        colorScreen.hidden = true;
        app.setStatus("Disconnected.");
    },
    syncUI: function() {
      // read values from light and update the phone UI
        var id = app.connectedPeripheral.id;
        ble.read(id, NEOPIXEL_SERVICE, COLOR, function(buffer) {
            var data = new Uint8Array(buffer);
            red.value = data[0];
            green.value = data[1];
            blue.value = data[2];
            app.updatePreview();
        });
        ble.read(id, NEOPIXEL_SERVICE, BRIGHTNESS, function(buffer) {
            var data = new Uint8Array(buffer);
            brightness.value = data[0];
        });
        ble.read(id, NEOPIXEL_SERVICE, POWER_SWITCH, function(buffer) {
          var data = new Uint8Array(buffer);
          powerSwitch.checked = data[0] !== 0;
        });
    },
    onColorChange: function (evt) {
        app.updatePreview();
        app.sendColorToArduino();
    },
    updatePreview: function() {
        var c = app.getColor();
        rgbText.innerText = "color rgb(" + c  + ")";
        previewColor.style.backgroundColor = "rgb(" + c + ")";
    },
    getColor: function () {
        // returns a string of red, green, blue values
        var color = [];
        color.push(red.value);
        color.push(green.value);
        color.push(blue.value);
        return color.join(',');
    },
    sendColorToArduino: function() {
        var value = new Uint8Array(3);
        value[0] = red.value;
        value[1] = green.value;
        value[2] = blue.value;
        ble.write(app.connectedPeripheral.id, NEOPIXEL_SERVICE, COLOR, value.buffer,
            function() {
                app.setStatus("Set color to " + app.getColor());
            },
            function(error) {
                app.setStatus("Error setting characteristic " + error);
            }
        );
    },
    onBrightnessChange: function(evt) {
      // user adjusted the brightness with the slider
      var value = new Uint8Array(1);
      value[0] = brightness.value;
      ble.write(app.connectedPeripheral.id, NEOPIXEL_SERVICE, BRIGHTNESS, value.buffer,
          function() {
              app.setStatus("Set brightness to " +  brightness.value);
          },
          function(error) {
              app.setStatus("Error setting characteristic " + error);
          }
      );
    },
    onSwitchChange: function(evt) {
      // the user toggled the power switch
      var value = new Uint8Array(1);
      if (powerSwitch.checked) {
        value[0] = 1; // turn on
      } else {
        value[0] = 0; // turn off
      }
      ble.write(app.connectedPeripheral.id, NEOPIXEL_SERVICE, POWER_SWITCH, value.buffer,
          function() {
              app.setStatus("Set switch to " +  value[0]);
          },
          function(error) {
              app.setStatus("Error setting characteristic " + error);
          }
      );
    },
    timeoutId: 0,
    setStatus: function(status) {
        if (app.timeoutId) {
            clearTimeout(app.timeoutId);
        }
        messageDiv.innerText = status;
        app.timeoutId = setTimeout(function() { messageDiv.innerText = ""; }, 4000);
    }
};

app.initialize();
