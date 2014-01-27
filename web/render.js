
var DOMParser = require('xmldom').DOMParser;
var canvg = require("canvg");
var Canvas = require('canvas');
var fs = require('fs');

process.argv.slice(2).forEach(function(arg){

    var svgText = fs.readFileSync(arg, 'utf8');
    var doc = new DOMParser().parseFromString(svgText, "text/xml");

    var w = doc.documentElement.getAttribute("width").replace(/^(\d+).*/, "\1");
    var h = doc.documentElement.getAttribute("width").replace(/^(\d+).*/, "\1");


    var canvas = new Canvas(w,h)
    , ctx = canvas.getContext('2d');

    canvg(canvas, svgText);

    var out = fs.createWriteStream(arg + ".png");

    var stream = canvas.pngStream();

    stream.on('data', function(chunk){
        out.write(chunk);
    });

    stream.on('end', function(){
        console.log('saved ' + arg + '.png');
    });


});


