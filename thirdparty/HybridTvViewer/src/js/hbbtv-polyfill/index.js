import { keyEventInit } from "./keyevent-init.js";
import { hbbtvFn } from "./hbbtv.js";
import { VideoHandler } from "./hbb-video-handler.js";

var _DEBUG_ = false;
// var _DEBUG_ = true;

function init() {
    _DEBUG_ && console.log("hbbtv-polyfill: load");
    // global helper namespace to simplify testing
    window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {
    };
    window.HBBTV_POLYFILL_NS = {
        ...window.HBBTV_POLYFILL_NS, ...{
            keysetSetValueCount: 0,
            streamEventListeners: [],
        }
    };
    window.HBBTV_POLYFILL_NS.currentChannel = window.HBBTV_POLYFILL_NS.currentChannel || {
        'TYPE_TV': 12,
        'channelType': 12,
        'sid': 1,
        'onid': 1,
        'tsid': 1,
        'name': 'test',
        'ccid': 'ccid:dvbt.0',
        'dsd': ''
    };

    keyEventInit();
    hbbtvFn();

    new VideoHandler().initialize();

    // convenience method: Javascript to VDR wrapper method, one-way
    window.signalCef = function(command) {
        window.cefQuery({
            request: command,
            persistent: false,
            onSuccess: function(response) {},
            onFailure: function(error_code, error_message) {}
        });
    };

    window.signalVdr = function(command) {
        signalCef('VDR:' + command);
    };

    _DEBUG_ && console.log("hbbtv-polyfill: loaded");
}

if (!document.body) {
    _DEBUG_ && console.log("hbbtv-polyfill: add as event listener, DOMContentLoaded");

    document.addEventListener("DOMContentLoaded", init);
} else {
    _DEBUG_ && console.log("hbbtv-polyfill: call init");
    init();
}
