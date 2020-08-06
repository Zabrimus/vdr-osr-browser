/*
  TODO
     Tagesschau
     SAT1
     Pro7
     Servus TV: gurad prevented to enter Mediathek ???

     Home&Garden TV 81
     HS24 Extra     253
     1-2-3          254
     HSE            256
     HSE            257
     Juwelo         258
     Sonnenklar     263 (Video liegt über dem Menu)
     MTV            265
     Bibel TV       366
     n-tv           383
     nick           385
     Sport1         398 (Videos nicht aufrufen!)
     Comedy Central 405
 */

const quirks = {
    "hbbtv-tatort.daserste.de": {
        start: {
            body_background: "hidden",
            visible_class: {
                index: 0,
                class: "playerbar",
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_class: {
                index: 0,
                class: "playerbar",
                value: "hidden"
            }
        }
    },

    "tv.ardmediathek.de": {
        start: {
            body_background: "hidden",
            visible_id: {
                name: [
                    "player"
                ],
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_id: {
                name: [
                    "player"
                ],
                value: "hidden"
            }
        },

    },

    "itv.ard.de/replay": {
        start: {
            backgroundColor: "transparent",
        },
        stop: {
            backgroundColor: "black",
        }
    },

    "hbbtv.zdf.de": {
        start: {
            body_background: "hidden",
            visible_id: {
                name: [
                    "player"
                ],
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_id: {
                name: [
                    "player"
                ],
                value: "hidden"
            }
        },
    },

    "hbbtv-mediathek-ard-alpha.br.de": {
        start: {
            body_background: "hidden",
            visible_class: {
                index: 0,
                class: "playerwin",
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_class: {
                index: 0,
                class: "playerwin",
                value: "hidden"
            }
        },
    },

    "www.arte.tv": {
        start: {
            body_background: "hidden",
            visible_id: {
                name: [
                    "videoOverlay"
                ],
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_id: {
                name: [
                    "videoOverlay"
                ],
                value: "hidden"
            }
        }
    },

    "hbbtv.redbutton.de/extern/redorbit/hbbtv/apps/": {
        start: {
            backgroundColor: "transparent",
            visible_id: {
                name: [
                    "screen"
                ],
                value: "hidden"
            },
            display_class: {
                index: 0,
                class: "slider",
                value: "none"
            }
        },

        stop: {
            backgroundColor: "#2c2c2c",
            visible_id: {
                name: [
                    "screen"
                ],
                value: "visible"
            },
            display_class: {
                index: 0,
                class: "slider",
                value: "block"
            }
        }
    },

    "-digitaltext.rtl-hbbtv.de": {
        start: {
            body_background: "hidden",
            visible_id: {
                name: [
                ],
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_id: {
                name: [
                ],
                value: "hidden"
            }
        }
    },

    "cdn.digitaltext.rtl.de": {
        start: {
            body_background: "hidden",
            visible_id: {
                name: [
                ],
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_id: {
                name: [
                ],
                value: "hidden"
            }
        }
    },

    "specials.rtl-hbbtv.de": {
        start: {
            body_background: "hidden",
            visible_id: {
                name: [
                ],
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_id: {
                name: [
                ],
                value: "hidden"
            }
        }
    },

    "cdn.specials.digitaltext.rtl.de": {
        start: {
            body_background: "hidden",
            visible_id: {
                name: [
                ],
                value: "visible"
            }
        },
        stop: {
            body_background: "visible",
            visible_id: {
                name: [
                ],
                value: "hidden"
            }
        }
    }

}

// "http://hbbtv.rtl2.de/cta/?cc=de"

function visible_class(cfg) {
    let index = cfg.index;
    let clazz = cfg.class;
    let value = cfg.value;
    document.getElementsByClassName(clazz)[index].style.visibility = value;
}

function visible_id(cfg) {
    let value = cfg.value;
    for (let i in cfg.name) {
        let el = document.getElementById(cfg.name[i]);
        if (typeof el !== 'undefined' && el != null) {
            el.style.visibility = value;
        }
    }
}

function backgroundColor(cfg) {
    document.body.style.backgroundColor = cfg;
}

function displayClass(cfg) {
    let clazz = cfg.class;
    let index = cfg.index;
    let value = cfg.value;

    let el = document.getElementsByClassName(clazz)[index];
    if (el) {
        el.style.display = value;
    }
}

function body_background(cfg) {
    document.body.style.visibility = cfg;
}

function activate_quirks(isStart) {
    let s, q, i;
    for (s in quirks) {
        if (document.location.href.search(s) > 0) {
            if (isStart) {
                let start = quirks[s].start;

                if (typeof start !== 'undefined') {
                    for (q in start) {
                        switch (q) {
                            case 'body_background':
                                body_background(start[q]);
                                break;

                            case 'visible_class':
                                visible_class(start[q]);
                                break;

                            case "visible_id":
                                visible_id(start[q]);
                                break;

                            case "backgroundColor":
                                backgroundColor(start[q]);
                                break;

                            case "display_class":
                                displayClass(start[q]);
                                break;

                            default:
                                break;
                        }
                    }
                }
            } else {
                let stop = quirks[s].stop;

                if (typeof stop !== 'undefined') {
                    for (q in stop) {
                        switch (q) {
                            case 'body_background':
                                body_background(stop[q]);
                                break;

                            case 'visible_class':
                                visible_class(stop[q]);
                                break;

                            case "visible_id":
                                visible_id(stop[q]);
                                break;

                            case "backgroundColor":
                                backgroundColor(stop[q].backgroundColor);
                                break;

                            case "display_class":
                                displayClass(stop[q]);
                                break;

                            default:
                                break;
                        }
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