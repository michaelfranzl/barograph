// we put everything into one variable
var baro = {
  fn: {},
  data: {
    altitude: 90.5, // set the altitude of your location here
    charts: {}
  },
}

baro.fn.processTemperature = function(str) {
  var t = parseFloat(str);
  $("#temperature").html(t.toFixed(1) + " °C");
}

baro.fn.processPressure = function(str) {
  var p = parseFloat(str);
  var sealevel = p / Math.pow(1-(baro.data.altitude / 44330.0), 5.255);
  
  $("#pressure").html(sealevel.toFixed(3) + " mbar");
  
  return;
  // the following draws a simple point-based time chart. Disabled for now because it needs more work to be useful.

  var obj = baro.data.charts["time"];

  var p = parseFloat(str);
  if (p < obj.y_min) obj.y_min = p;
  if (p > obj.y_max) obj.y_max = p;
  
  obj.y_min -= 0.02;
  obj.y_max += 0.02;
  
  var x = obj.curr_x;
  
  if (x == 0) {
    baro.fn.clearCanvas("time");
  }
  
  var y = Math.round(100 * (p - obj.y_min) / (obj.y_max - obj.y_min));
  baro.fn.drawPixel("time", x, y, 0, 0, 0, 255);
  
  x += 1;
  baro.fn.updateCanvas("time");
  
  for (i = 0; i < obj.height; i++) {
    baro.fn.drawPixel("time", obj.curr_x + 2, i, 200, 200, 200, 255);
    baro.fn.drawPixel("time", obj.curr_x + 1, i, 255, 255, 255, 255);
  }

  if (x > obj.width) {
    x = 0;
  }
  obj.curr_x = x;
}


// converts a value from 0..1 into spectrum RGB values
// Modified code from: https://www.particleincell.com/2014/colormap
baro.fn.scalarToRGB = function(val) {
  var r, g, b;
  
  var a=(val)/0.14;
  var X=Math.floor(a);
  var Y=Math.floor(255*(a-X));
  switch(X) {
    case 0: r=0;     g=0;         b=Y;break;
    case 1: r=0;     g=Y;         b=255;break;
    case 2: r=0;     g=255;       b=255-Y;break;
    case 3: r=Y;     g=255-Y;     b=0;break;
    case 4: r=255;   g=Y;         b=0;break;
    case 5: r=255;   g=255;       b=Y;break;
    case 6: r=255;   g=255;       b=255;break;
    default: r=255;  g=255;       b=255; break;
  }
  return [r, g, b]
}


baro.fn.processFFT = function(csv) {
  var arr = csv.split(";");
  arr.splice(-1); // remove last empty element
  
  var fftsize = arr.length;
  
  var obj = baro.data.charts["freq"];
  obj.curr_x += 1;

  for (i = 1; i < arr.length / 2; i++) {
    // get real and imaginary part from DFT result.
    var real = parseFloat(arr[i]);
    var imag = parseFloat(arr[fftsize/2 + i]);
    
    // caculate magnitude from real and image
    var mag = Math.sqrt(real * real + imag * imag);
    
    // Noise in atmospheric pressure seems to be brown(ian) noise with logarithmic
    // amplitude distribution. That is difficult to plot, so we make it approx. linear.
    // First, take logarithm, this will make it almost linear.
    var maglog = Math.log10(1 + mag);
    // Second, result of logarithm can be made even smoother by fitting a trend line and dividing by it. The following formula was trial-and-error in OpenOffice Calc until the spectrum at "silence" was uniformly blue.
    var magcorr = maglog / (3.5 + 30 * Math.pow(i, -1)); // is now in range 0 to 1
   
    // print info about DFT bins
    //$("#log").append("bin " + i + ": mag=" + mag.toFixed(2) + " log=" + maglog.toFixed(2) + " magcorr=" + magcorr.toFixed(2) + "</br>");
    
    var rgb = baro.fn.scalarToRGB(magcorr);
    baro.fn.drawPixel("freq", obj.curr_x, i, rgb[0], rgb[1], rgb[2], 255);
    
    if (obj.curr_x > 1000) obj.curr_x = 0;
  }
  
  for (i = 0; i < obj.height; i++) {
    //baro.fn.drawPixel("freq", obj.curr_x + 2, i, 200, 200, 200, 255);
    baro.fn.drawPixel("freq", obj.curr_x + 1, i, 255, 255, 255, 255);
  }
  
  baro.fn.updateCanvas("freq");
}

baro.fn.drawPixel = function(id, x, y, r, g, b, a) {
  var obj = baro.data.charts[id];
  var index = (x + y * obj.width) * 4;
  
  obj.dat.data[index + 0] = r;
  obj.dat.data[index + 1] = g;
  obj.dat.data[index + 2] = b;
  obj.dat.data[index + 3] = a;
}

baro.fn.updateCanvas = function(id) {
  baro.data.charts[id].ctx.putImageData(baro.data.charts[id].dat, 0, 0);
}

baro.fn.clearCanvas = function(id) {
  baro.data.charts[id].ctx.clearRect(0, 0, baro.data.charts[id].width, baro.data.charts[id].height);
  baro.data.charts[id].dat = baro.data.charts[id].ctx.createImageData(baro.data.charts[id].width, baro.data.charts[id].height);
}

baro.fn.setFftSize = function(size) {
  // 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 and 16384
  baro.data.ws.send("fftsize:" + size)
};

baro.fn.setupCanvas = function(id) {
  var c = document.getElementById(id);
  var w = c.width;
  var h = c.height;
  var ctx = c.getContext("2d");
  var dat = ctx.getImageData(0, 0, w, h);
  
  var obj = {
    canvas: c,
    width: w,
    height: h,
    ctx: ctx,
    dat: dat,
    curr_x: 0,
    y_min: 999999,
    y_max: -999999,
  };
  baro.data.charts[id] = obj;
}


$(function() {
  baro.fn.setupCanvas("freq");
  baro.fn.setupCanvas("time");
  baro.fn.setupCanvas("legend");
  
  // draw a spectrum legend for the magnitude
  // TODO: add labels
  var obj = baro.data.charts["legend"];
  for (y=0; y<obj.height; y++) {
    for (x=0; x<obj.width; x++) {
      var rgb = baro.fn.scalarToRGB(y/obj.height);
      baro.fn.drawPixel("legend", x, y, rgb[0], rgb[1], rgb[2], 255);
      
    }
  }
  baro.fn.updateCanvas("legend");
    

  // connect to the Qt C++ backend
  baro.data.ws = new WebSocket("ws://192.168.24.252:7600");
  baro.data.ws.onmessage = function(e) {
    var parts = e.data.split(":");
    if (parts[0] == "fft") {
      baro.fn.processFFT(parts[1]);
      
    } else if (parts[0] == "p") {
      baro.fn.processPressure(parts[1]);
      
    } else if (parts[0] == "t") {
      baro.fn.processTemperature(parts[1]);
      
    }
    //console.log(e);
  }
  
  // -----------------------
  
  
  
});