/* jshint quotmark: false, unused: vars, browser: true */
/* global console, $, ble, _, refreshButton, deviceList, previewColor, red, green, blue, disconnectButton, connectionScreen, colorScreen, rgbText, messageDiv, powerSwitch, brightness */
'use strict';

var pixel = {
    service: 'ccc0',
    color: 'ccc1',
    brightness: 'ccc2',
    switch: 'ccc3'
};

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
        deviceList.ontouchstart = app.connect; // assume not scrolling
        refreshButton.ontouchstart = app.scan;
        disconnectButton.ontouchstart = app.disconnect;

        // throttle color changes
        var throttledOnColorChange = _.throttle(app.onColorChange, 200);
        $('.color-range').on('change', throttledOnColorChange);

        // throttle brightness changes
        var throttledOnBrightnessChange = _.throttle(app.onBrightnessChange, 200);
        $('#brightness').on('change', throttledOnBrightnessChange);

        powerSwitch.onchange = app.onSwitchChange;

        app.scan();
    },
    scan: function(e) {
        deviceList.innerHTML = ""; // clear the list
        app.setStatus("Scanning for Bluetooth Devices...");

        ble.startScan([pixel.service],
            app.onDeviceDiscovered,
            function() { app.setStatus("Listing Bluetooth Devices Failed"); }
        );

        // stop scan after 5 seconds
        setTimeout(ble.stopScan, 5000, app.onScanComplete);

    },
    connect: function (e) {
        app.setStatus("Connecting...");
        var deviceId = e.target.dataset.deviceId;
        console.log("Requesting connection to " + deviceId);
        ble.connect(deviceId, app.onConnect, app.onDisconnect);
    },
    disconnect: function(event) {
        if (event) {
            event.preventDefault();
        }

        app.setStatus("Disconnecting...");
        ble.disconnect(app.connectedPeripheral.id, app.onDisconnect);
    },
    onConnect: function(peripheral) {
        app.connectedPeripheral = peripheral;
        connectionScreen.hidden = true;
        colorScreen.hidden = false;
        app.setStatus("Connected.");
        app.syncUI();
    },
    onDisconnect: function() {
        connectionScreen.hidden = false;
        colorScreen.hidden = true;
        app.setStatus("Disconnected.");
    },
    syncUI: function() {
        var id = app.connectedPeripheral.id;
        ble.read(id, pixel.service, pixel.color, function(buffer) {
            var data = new Uint8Array(buffer);
            red.value = data[0];
            green.value = data[1];
            blue.value = data[2];
            app.updatePreview();
        });
        ble.read(id, pixel.service, pixel.brightness, function(buffer) {
            var data = new Uint8Array(buffer);
            brightness.value = data[0];
        });
        ble.read(id, pixel.service, pixel.switch, function(buffer) {
          var data = new Uint8Array(buffer);
          powerSwitch.checked = data[0] !== 0;
        });

// notification
ble.startNotification(id, pixel.service, pixel.brightness, function(buffer) {
    var data = new Uint8Array(buffer);
    brightness.value = data[0];
});
ble.startNotification(id, pixel.service, pixel.switch, function(buffer) {
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
        rgbText.innerText = c;
        previewColor.style.backgroundColor = "rgb(" + c + ")";
    },
    getColor: function () {
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
        ble.write(app.connectedPeripheral.id, pixel.service, pixel.color, value.buffer,
            function() {
                app.setStatus("Set color to " + app.getColor());
            },
            function(error) {
                app.setStatus("Error setting characteristic " + error);
            }
        );
    },
    onBrightnessChange: function(evt) {
      // this is the slider changing
      var value = new Uint8Array(1);
      value[0] = brightness.value;
      ble.write(app.connectedPeripheral.id, pixel.service, pixel.brightness, value.buffer,
          function() {
              app.setStatus("Set brightness to " +  brightness.value);
          },
          function(error) {
              app.setStatus("Error setting characteristic " + error);
          }
      );
    },
    onSwitchChange: function(evt) {
      // this is the UI changing
      var value = new Uint8Array(1);
      if (powerSwitch.checked) {
        // turn on
        value[0] = 1;
      } else {
        // turn off
        value[0] = 0;
      }
      ble.write(app.connectedPeripheral.id, pixel.service, pixel.switch, value.buffer,
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
    }
};
