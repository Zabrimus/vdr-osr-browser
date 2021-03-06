# HybridTV Dev Environment
Original repository can be found at: https://github.com/smartclip/HybridTvViewer 

This repository contains only files which are needed for my purposes.

## HbbTV polyfill and Chrome extension
Polyfill to add basic HbbTV functionality to a browser. Uses HTML5 video elements for 
media playback and dispatches media events onto OIPF objects. Available as Chrome extension but also made for functional testing in a browser without extension support (eg. Headless Chrome).
## Motivation
There are quite a few limitations and pitfalls for testing HbbTV-based applications. 
The ultimate goal of this project is to provide a "perfect" environment to run HbbTV apps in a browser as they would on a real TV set.
 
## Getting Started
Build the package. 
```bash
npm i
npm run build
```
The resulting "build" folder is the full package and can be installed as an extension 
in Chrome through "developer mode".
The extension injects the polyfill whenever a valid HbbTV header has been detected.
"hbbtv_polyfill.js" is the standalone polyfill. Injected into a page, it emulates basic 
HbbTV behaviour.
##  Work done (changes to fork)
Headless Chrome does not support extensions, so we uncoupled the polyfill from the rest of the code. To have a clean test environment, we also built the polyfill to be as discreet as possible. We overhauled the forked code and also fixed some issues regarding async script loading and video functionality on the way.
* Added webpack build for polyfill and extension
* Fixed race conditions
* Fixed video handling
* Listen and react to OIPF <object> data and type changes
* Added dash.js
* Removed UI
## Credits
Project has been forked from Karl Rousseau's HybridTVViewer (https://github.com/karl-rousseau/HybridTvViewer)
Project has been forked from Smartclips HybridTVViewer (https://github.com/smartclip/HybridTvViewer)
Background videos are kindly provided by Blender Foundation (https://www.blender.org/about/projects/)
Thank you very much!