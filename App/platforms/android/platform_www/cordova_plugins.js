cordova.define('cordova/plugin_list', function(require, exports, module) {
module.exports = [
    {
        "file": "plugins/org.apache.cordova.device/www/device.js",
        "id": "org.apache.cordova.device.device",
        "pluginId": "org.apache.cordova.device",
        "clobbers": [
            "device"
        ]
    },
    {
        "file": "plugins/org.apache.cordova.vibration/www/vibration.js",
        "id": "org.apache.cordova.vibration.notification",
        "pluginId": "org.apache.cordova.vibration",
        "merges": [
            "navigator.notification",
            "navigator"
        ]
    },
    {
        "file": "plugins/org.chromium.sockets.tcp/sockets.tcp.js",
        "id": "org.chromium.sockets.tcp.sockets.tcp",
        "pluginId": "org.chromium.sockets.tcp",
        "clobbers": [
            "chrome.sockets.tcp"
        ]
    },
    {
        "file": "plugins/org.chromium.common/events.js",
        "id": "org.chromium.common.events",
        "pluginId": "org.chromium.common",
        "clobbers": [
            "chrome.Event"
        ]
    },
    {
        "file": "plugins/org.chromium.common/errors.js",
        "id": "org.chromium.common.errors",
        "pluginId": "org.chromium.common"
    },
    {
        "file": "plugins/org.chromium.common/stubs.js",
        "id": "org.chromium.common.stubs",
        "pluginId": "org.chromium.common"
    },
    {
        "file": "plugins/org.chromium.common/helpers.js",
        "id": "org.chromium.common.helpers",
        "pluginId": "org.chromium.common"
    },
    {
        "file": "plugins/cordova.custom.plugins.exitapp/www/ExitApp.js",
        "id": "cordova.custom.plugins.exitapp.exitApp",
        "merges": [
            "navigator.app"
        ]
    }
];
module.exports.metadata = 
// TOP OF METADATA
{
    "org.apache.cordova.device": "0.3.0",
    "org.apache.cordova.console": "0.2.13",
    "org.apache.cordova.vibration": "0.3.13",
    "org.chromium.sockets.tcp": "1.3.2",
    "org.chromium.common": "1.0.6",
    "cordova.custom.plugins.exitapp": "1.0.0"
};
// BOTTOM OF METADATA
});