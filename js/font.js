var cssId = 'tfont';
if (!document.getElementById(cssId)) {
  var head  = document.getElementsByTagName('head')[0];
  var link  = document.createElement('link');
  link.id   = cssId;
  link.rel  = 'stylesheet';
  link.type = 'text/css';
  link.href = 'client://css/TiresiasPCfont.css';
  link.media = 'all';
  head.appendChild(link);
}

if (document && document.body) {
  if (window == window.top) {
    document.body.style["font-family"] = "Tiresias";
  }
} else {
  document.addEventListener("DOMContentLoaded", () => {
    document.body.style["font-family"] = "Tiresias";
  });
}