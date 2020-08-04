import {PC_KEYCODES} from "../shared/pc-keycodes.js";


export const keyEventInit = function () {
    console.log("hbbtv-polyfill: keyEventInit");

    window.KeyEvent = window.KeyEvent || {}; // defining default global KeyEvent as defined in CEA-HTML 2014 specs
    window.KeyEvent.VK_LEFT = (typeof window.KeyEvent.VK_LEFT !== 'undefined' ? window.KeyEvent.VK_LEFT : 0x25);
    window.KeyEvent.VK_UP = (typeof window.KeyEvent.VK_UP !== 'undefined' ? window.KeyEvent.VK_UP : 0x26);
    window.KeyEvent.VK_RIGHT = (typeof window.KeyEvent.VK_RIGHT !== 'undefined' ? window.KeyEvent.VK_RIGHT : 0x27);
    window.KeyEvent.VK_DOWN = (typeof window.KeyEvent.VK_DOWN !== 'undefined' ? window.KeyEvent.VK_DOWN : 0x28);
    window.KeyEvent.VK_ENTER = (typeof window.KeyEvent.VK_ENTER !== 'undefined' ? window.KeyEvent.VK_ENTER : 13);
    window.KeyEvent.VK_BACK = (typeof window.KeyEvent.VK_BACK !== 'undefined' ? window.KeyEvent.VK_BACK : 461);

    window.KeyEvent.VK_RED = (typeof window.KeyEvent.VK_RED !== 'undefined' ? window.KeyEvent.VK_RED : 403);
    window.KeyEvent.VK_GREEN = (typeof window.KeyEvent.VK_GREEN !== 'undefined' ? window.KeyEvent.VK_GREEN : 404);
    window.KeyEvent.VK_YELLOW = (typeof window.KeyEvent.VK_YELLOW !== 'undefined' ? window.KeyEvent.VK_YELLOW : 405);
    window.KeyEvent.VK_BLUE = (typeof window.KeyEvent.VK_BLUE !== 'undefined' ? window.KeyEvent.VK_BLUE : 406);

    window.KeyEvent.VK_PLAY = (typeof window.KeyEvent.VK_PLAY !== 'undefined' ? window.KeyEvent.VK_PLAY : 0x50);
    window.KeyEvent.VK_PAUSE = (typeof window.KeyEvent.VK_PAUSE !== 'undefined' ? window.KeyEvent.VK_PAUSE : 0x51);
    window.KeyEvent.VK_STOP = (typeof window.KeyEvent.VK_STOP !== 'undefined' ? window.KeyEvent.VK_STOP : 0x53);

    window.KeyEvent.VK_FAST_FWD = (typeof window.KeyEvent.VK_FAST_FWD !== 'undefined' ? window.KeyEvent.VK_FAST_FWD : 0x46);
    window.KeyEvent.VK_REWIND = (typeof window.KeyEvent.VK_REWIND !== 'undefined' ? window.KeyEvent.VK_REWIND : 0x52);

    window.KeyEvent.VK_PAGE_UP = (typeof window.KeyEvent.VK_PAGE_UP !== 'undefined' ? window.KeyEvent.VK_PAGE_UP : 0x21);
    window.KeyEvent.VK_PAGE_DOWN = (typeof window.KeyEvent.VK_PAGE_DOWN !== 'undefined' ? window.KeyEvent.VK_PAGE_DOWN : 0x22);

    window.KeyEvent.VK_0 = (typeof window.KeyEvent.VK_0 !== 'undefined' ? window.KeyEvent.VK_0 : 0x30);
    window.KeyEvent.VK_1 = (typeof window.KeyEvent.VK_1 !== 'undefined' ? window.KeyEvent.VK_1 : 0x31);
    window.KeyEvent.VK_2 = (typeof window.KeyEvent.VK_2 !== 'undefined' ? window.KeyEvent.VK_2 : 0x32);
    window.KeyEvent.VK_3 = (typeof window.KeyEvent.VK_3 !== 'undefined' ? window.KeyEvent.VK_3 : 0x33);
    window.KeyEvent.VK_4 = (typeof window.KeyEvent.VK_4 !== 'undefined' ? window.KeyEvent.VK_4 : 0x34);
    window.KeyEvent.VK_5 = (typeof window.KeyEvent.VK_5 !== 'undefined' ? window.KeyEvent.VK_5 : 0x35);
    window.KeyEvent.VK_6 = (typeof window.KeyEvent.VK_6 !== 'undefined' ? window.KeyEvent.VK_6 : 0x36);
    window.KeyEvent.VK_7 = (typeof window.KeyEvent.VK_7 !== 'undefined' ? window.KeyEvent.VK_7 : 0x37);
    window.KeyEvent.VK_8 = (typeof window.KeyEvent.VK_8 !== 'undefined' ? window.KeyEvent.VK_8 : 0x38);
    window.KeyEvent.VK_9 = (typeof window.KeyEvent.VK_9 !== 'undefined' ? window.KeyEvent.VK_9 : 0x39);

    // patches for some CE-HTML apps ...

    window.VK_LEFT = (typeof window.VK_LEFT !== 'undefined' ? window.VK_LEFT : 0x25);
    window.VK_UP = (typeof window.VK_UP !== 'undefined' ? window.VK_UP : 0x26);
    window.VK_RIGHT = (typeof window.VK_RIGHT !== 'undefined' ? window.VK_RIGHT : 0x27);
    window.VK_DOWN = (typeof window.VK_DOWN !== 'undefined' ? window.VK_DOWN : 0x28);
    window.VK_ENTER = (typeof window.VK_ENTER !== 'undefined' ? window.VK_ENTER : 13);
    window.VK_BACK = (typeof window.VK_BACK !== 'undefined' ? window.VK_BACK : 461);

    window.VK_RED = (typeof window.VK_RED !== 'undefined' ? window.VK_RED : 403);
    window.VK_GREEN = (typeof window.VK_GREEN !== 'undefined' ? window.VK_GREEN : 404);
    window.VK_YELLOW = (typeof window.VK_YELLOW !== 'undefined' ? window.VK_YELLOW : 405);
    window.VK_BLUE = (typeof window.VK_BLUE !== 'undefined' ? window.VK_BLUE : 406);

    window.VK_PLAY = (typeof window.VK_PLAY !== 'undefined' ? window.VK_PLAY : 0x50);
    window.VK_PAUSE = (typeof window.VK_PAUSE !== 'undefined' ? window.VK_PAUSE : 0x51);
    window.VK_STOP = (typeof window.VK_STOP !== 'undefined' ? window.VK_STOP : 0x53);

    window.VK_FAST_FWD = (typeof window.VK_FAST_FWD !== 'undefined' ? window.VK_FAST_FWD : 0x46);
    window.VK_REWIND = (typeof window.VK_REWIND !== 'undefined' ? window.VK_REWIND : 0x52);

    window.VK_PAGE_UP = (typeof window.VK_PAGE_UP !== 'undefined' ? window.VK_PAGE_UP : 0x21);
    window.VK_PAGE_DOWN = (typeof window.VK_PAGE_DOWN !== 'undefined' ? window.VK_PAGE_DOWN : 0x22);

    window.VK_0 = (typeof window.VK_0 !== 'undefined' ? window.VK_0 : 0x30);
    window.VK_1 = (typeof window.VK_1 !== 'undefined' ? window.VK_1 : 0x31);
    window.VK_2 = (typeof window.VK_2 !== 'undefined' ? window.VK_2 : 0x32);
    window.VK_3 = (typeof window.VK_3 !== 'undefined' ? window.VK_3 : 0x33);
    window.VK_4 = (typeof window.VK_4 !== 'undefined' ? window.VK_4 : 0x34);
    window.VK_5 = (typeof window.VK_5 !== 'undefined' ? window.VK_5 : 0x35);
    window.VK_6 = (typeof window.VK_6 !== 'undefined' ? window.VK_6 : 0x36);
    window.VK_7 = (typeof window.VK_7 !== 'undefined' ? window.VK_7 : 0x37);
    window.VK_8 = (typeof window.VK_8 !== 'undefined' ? window.VK_8 : 0x38);
    window.VK_9 = (typeof window.VK_9 !== 'undefined' ? window.VK_9 : 0x39);



    window.addEventListener(
        "keydown",
        (evt) => {
            console.log("hbbtv-polyfill: browser keydown " + evt.keyCode, "internal", evt.detail && evt.detail.hbbInternal === true);
            if (evt.detail && evt.detail.hbbInternal === true) {
                return;
            }

            evt.preventDefault();
            evt.stopPropagation();
            var keyCode = evt.which || evt.keyCode;
            if (keyCode === 82) {
                // 'r' key on PC
                doKeyPress("VK_RED");
            } else if (keyCode === 71) {
                // 'g' key on PC
                doKeyPress("VK_GREEN");
            } else if (keyCode === 89) {
                // 'y' key on PC
                doKeyPress("VK_YELLOW");
            } else if (keyCode === 66) {
                // 'b' key on PC
                doKeyPress("VK_BLUE");
            } else if (keyCode === 37) {
                doKeyPress("VK_LEFT");
            } else if (keyCode === 38) {
                doKeyPress("VK_UP");
            } else if (keyCode === 39) {
                doKeyPress("VK_RIGHT");
            } else if (keyCode === 40) {
                // DOWN on PC
                doKeyPress("VK_DOWN");
            } else if (keyCode === 13) {
                // ENTER on PC
                doKeyPress("VK_ENTER");
            } else if (keyCode === 8) {
                // Backspace on PC
                doKeyPress("VK_BACK");
            } else if (keyCode === 96) {
                doKeyPress("VK_0");
            } else if (keyCode === 97) {
                doKeyPress("VK_1");
            } else if (keyCode === 98) {
                doKeyPress("VK_2");
            } else if (keyCode === 99) {
                doKeyPress("VK_3");
            } else if (keyCode === 100) {
                doKeyPress("VK_4");
            } else if (keyCode === 101) {
                doKeyPress("VK_5");
            } else if (keyCode === 102) {
                doKeyPress("VK_6");
            } else if (keyCode === 103) {
                doKeyPress("VK_7");
            } else if (keyCode === 104) {
                doKeyPress("VK_8");
            } else if (keyCode === 105) {
                doKeyPress("VK_9");
            } else if (keyCode === 116) {
                // F5 in browser
                window.location.reload();
            } else if (keyCode === PC_KEYCODES.k) {
                doKeyPress("VK_PAUSE");
            } else if (keyCode === PC_KEYCODES.j) {
                doKeyPress("VK_PLAY");
            } else if (keyCode === PC_KEYCODES.h) {
                doKeyPress("VK_REWIND");
            } else if (keyCode === PC_KEYCODES.l) {
                doKeyPress("VK_STOP");
            } else if (keyCode === PC_KEYCODES.oe) {
                doKeyPress("VK_FAST_FWD");
            }
        },
        true
    );

    function doKeyPress(keyCode) {
        // document.activeElement.dispatchEvent(new Event('focus'));

        // create and dispatch the event (keydown)
        var keyDownEvent = new KeyboardEvent("keydown", {
            bubbles: true,
            cancelable: true,
        });

        delete keyDownEvent.keyCode;
        Object.defineProperty(keyDownEvent, "keyCode", { "value": window[keyCode] });
        Object.defineProperty(keyDownEvent, "detail", { "value": { hbbInternal: true } });

        document.dispatchEvent(keyDownEvent);

        // create and dispatch the event (keypress)
        var keypressEvent = new KeyboardEvent("keypress", {
            bubbles: true,
            cancelable: true,
        });
        delete keypressEvent.keyCode;
        Object.defineProperty(keypressEvent, "keyCode", { "value": window[keyCode] });
        Object.defineProperty(keypressEvent, "detail", { "value": { hbbInternal: true } });

        document.dispatchEvent(keypressEvent);

        // create and dispatch the event (keyup)
        var keyUpEvent = new KeyboardEvent("keyup", {
          bubbles: true,
          cancelable: true,
        });

        delete keyUpEvent.keyCode;
        Object.defineProperty(keyUpEvent, "keyCode", { "value": window[keyCode] });
        Object.defineProperty(keyUpEvent, "detail", { "value": { hbbInternal: true } });

        document.dispatchEvent(keyUpEvent);

        /*
        $(document).ready(function() {
          setTimeout(function() { document.activeElement.focus() }, 20);
        });
        */

        /*
        setTimeout(function() { document.activeElement.focus() }, 20);
        */

        /*
        setTimeout(function() {
            let el = document.activeElement;
            if (typeof el !== 'undefined') {
              el.blur();
              el.focus();
            }
        }, 3000);
        */
    };

    window.cefKeyPress = function(keyCode) {
       // special handling if possible (use a whitelist, because not all sites works really good with this hack)
       // whitelist
       // http://www.tagesschau.de/itv/hbbtv/hbbtv.cehtml

       var href = document.location.href;
       var next;

       if (href.search("www.tagesschau.de") !== -1) {
         if (keyCode === 'VK_LEFT' || keyCode === 'VK_UP') {
           next = findNextFocusableElement(true, document.activeElement);
         } else if (keyCode === 'VK_RIGHT' || keyCode === 'VK_DOWN') {
           next = findNextFocusableElement(false, document.activeElement);
         }
       }

       if (typeof next !== 'undefined') {
          // TODO: Something is still wrong here. Focus, Click and what else?
          next.focus();
          next.click();
       } else {
         doKeyPress(keyCode);
       }
    };

};
