const quirks = {
    "hbbtv-tatort.daserste.de": {
        start: [
            { body_background: "hidden" },
            { visible_class: { index: 0, class: "playerbar", value: "visible"} },
        ],
        stop: [
            { body_background: "visible" },
            { visible_class: { index: 0, class: "playerbar", value: "hidden"} }
        ]
    },

    "hbbtv.daserste.de": {
        start: [
            { body_background: "hidden" },
            { visible_class: { index: 0, parent: true, class: "playerbar", value: "visible" } },
            { visible_class: { index: 0, class: "playerbar", value: "visible" } }
        ],
        stop: [
            { body_background: "visible" },
            { visible_class: { index: 0, class: "playerbar", value: "hidden" }}
        ]
    },

    "tv.ardmediathek.de": {
        start: [
            { body_background: "hidden" },
            { visible_id: { name: [ "player" ], value: "visible" } }
        ],
        stop: [
            { body_background: "visible" },
            { visible_id: { name: [ "player" ], value: "hidden" } }
        ]
    },

    "itv.ard.de/replay": {
        start: [
            { backgroundColor: "transparent" }
        ],
        stop: [
            { backgroundColor: "black" }
        ]
    },

    "hbbtv.zdf.de": {
        start: [
            { body_background: "hidden" },
            { visible_id: { name: [ "player" ], value: "visible" }}
        ],
        stop: [
            { body_background: "visible" },
            { visible_id: { name: [ "player" ], value: "hidden" } }
        ],
    },

    "hbbtv-mediathek-ard-alpha.br.de": {
        start: [
            { body_background: "hidden" },
            { visible_class: { index: 0, class: "playerwin", value: "visible" } }
        ],
        stop: [
            { body_background: "visible" },
            { visible_class: { index: 0, class: "playerwin", value: "hidden" } }
        ],
    },

    "www.arte.tv": {
        start: [
            { body_background: "hidden" },
            { visible_id: { name: [ "videoOverlay" ], value: "visible" } }
        ],
        stop: [
            { body_background: "visible" },
            { visible_id: { name: [ "videoOverlay" ], value: "hidden" } }
        ]
    },

    "hbbtv.redbutton.de/extern/redorbit/hbbtv/apps/": {
        start: [
            { backgroundColor: "transparent" },
            { visible_id: { name: [ "screen" ], value: "hidden" } },
            { display_class: { index: 0, class: "slider", value: "none" } },
            { display_class: { index: 1, class: "slider", value: "none" } }
        ],
        stop: [
            { backgroundColor: "#2c2c2c" },
            { visible_id: { name: [ "screen" ], value: "visible" } },
            { display_class: { index: 0, class: "slider", value: "block" } },
            { display_class: { index: 1, class: "slider", value: "block" } }
        ]
    },

    "-digitaltext.rtl-hbbtv.de": {
        start: [
            { body_background: "hidden" },
        ],
        stop: [
            { body_background: "visible" },
        ]
    },

    "specials.rtl-hbbtv.de": {
        start: [
            { body_background: "hidden" }
        ],
        stop: [
            { body_background: "visible" }
        ]
    },

    "cdn.specials.digitaltext.rtl.de": {
        start: [
            { body_background: "hidden" }
        ],
        stop: [
            { body_background: "visible" }
        ]
    },

    "cdn.digitaltext.rtl.de": {
        start: [
            { body_background: "hidden" }
        ],
        stop: [
            { body_background: "visible" }
        ]
    },

    "bibeltv.c.nmdn.net": {
        start: [
            { visible_id: { name: [ "application" ], value: "hidden" } },
        ],
        stop: [
            { visible_id: { name: [ "application" ], value: "visible" } },
        ]
    }
}


function visible_class(config) {
    let cfg = config.visible_class;
    let index = cfg.index;
    let clazz = cfg.class;
    let value = cfg.value;
    let parent = (typeof cfg.parent !== 'undefined') ? cfg.parent : false;

    if (parent === true) {
        document.getElementsByClassName(clazz)[index].parentElement.style.visibility = value;
    } else {
        console.log("Search: " + clazz);
        if (typeof document.getElementsByClassName(clazz) !== 'undefined' && document.getElementsByClassName(clazz).length > 0) {
            document.getElementsByClassName(clazz)[index].style.visibility = value;
        }
    }
}

function visible_id(config) {
    let cfg = config.visible_id;
    let value = cfg.value;
    for (let i in cfg.name) {
        let el = document.getElementById(cfg.name[i]);
        if (typeof el !== 'undefined' && el != null) {
            el.style.visibility = value;
        }
    }
}

function backgroundColor(config) {
    let cfg = config.backgroundColor;
    document.body.style.backgroundColor = cfg;
}

function displayClass(config) {
    let cfg = config.display_class;
    let clazz = cfg.class;
    let index = cfg.index;
    let value = cfg.value;

    let el = document.getElementsByClassName(clazz)[index];
    if (el) {
        el.style.display = value;
    }
}

function body_background(config) {
    let cfg = config.body_background;
    document.body.style.visibility = cfg;
}

function activate_quirks(isStart) {
    let s, q, i;
    for (s in quirks) {
        if (document.location.href.search(s) > 0) {
            let config = isStart ? quirks[s].start : quirks[s].stop;
            if (typeof config !== 'undefined') {
                for (q in config) {
                    if (config[q].hasOwnProperty('body_background')) {
                        body_background(config[q]);
                    } else if (config[q].hasOwnProperty('visible_class')) {
                        visible_class(config[q]);
                    } else if (config[q].hasOwnProperty('visible_id')) {
                        visible_id(config[q]);
                    } else if (config[q].hasOwnProperty('backgroundColor')) {
                        backgroundColor(config[q]);
                    } else if (config[q].hasOwnProperty('display_class')) {
                        displayClass(config[q]);
                    }
                }
            }
        }
    }
}

window.process_video_quirk = function(dimension, target) {
    let player = document.getElementById('hbbtv-polyfill-video-player');
    let trans = player.src === 'client://movie/transparent-full.webm';

    let zindex = target.style.zIndex;

    // check wether activation or deactivation is needed
    if ((dimension.width === 1280 && dimension.height === 720) && !trans) {
        // fullscreen
        setTimeout(function () {
            window._HBBTV_DEBUG_ && console.log("====> START_VIDEO_QUIRK (in activate)");
            activate_quirks(true);
        }, 10);
    } else {
        // windowed video
        setTimeout(function() {
            window._HBBTV_DEBUG_ && console.log("====> STOP_VIDEO_QUIRK (in deactivate)");
            activate_quirks(false);
        }, 10);
    }
}

window.start_video_quirk = function() {
    setTimeout(function() {
        window._HBBTV_DEBUG_ && console.log("====> START_VIDEO_QUIRK (in activate)");
        activate_quirks(true);
    }, 10);
};

window.stop_video_quirk = function() {
    setTimeout(function() {
        window._HBBTV_DEBUG_ && console.log("====> STOP_VIDEO_QUIRK (in deactivate)");
        activate_quirks(false);
    }, 10);
};