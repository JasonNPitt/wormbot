//define rectangle object
var rect = {x:0,y:0,w:0,h:0};
var rects = [];
var numrects =0;


// get references to the canvas and context
var canvas = document.getElementById("canvas");
var ctx = canvas.getContext("2d");
var img = new Image();
img.onload = function() {
    ctx.drawImage(img, 0, 0);
};
img.src = 'http://127.0.0.1/robot_data/700/frame000100.png';

// style the context
ctx.strokeStyle = "red";
ctx.lineWidth = 3;

// calculate where the canvas is on the window
// (used to help calculate mouseX/mouseY)
var $canvas = $("#canvas");
var canvasOffset = $canvas.offset();
var offsetX = canvasOffset.left;
var offsetY = canvasOffset.top;
var scrollX = $canvas.scrollLeft();
var scrollY = $canvas.scrollTop();

// this flage is true when the user is dragging the mouse
var isDown = false;

// these vars will hold the starting mouse position
var startX;
var startY;


function handleMouseDown(e) {
    e.preventDefault();
    e.stopPropagation();
    scrollX = $canvas.scrollLeft();
		scrollY = $canvas.scrollTop();

    // save the starting x/y of the rectangle
    startX = parseInt(e.clientX - offsetX - scrollX);
    startY = parseInt(e.clientY - offsetY - scrollY);

    // set a flag indicating the drag has begun
    isDown = true;
    numrects++;
}

function addNewRect(){
	isDown = false;
  var newRect = {x:startX,y:startY,w:mouseX-startX,h:mouseY-startY};
  rects.push(newRect);
}

function handleMouseUp(e) {
    e.preventDefault();
    e.stopPropagation();

    // the drag is over, clear the dragging flag
    
    addNewRect();
}

function handleMouseOut(e) {
    e.preventDefault();
    e.stopPropagation();

    // the drag is over, clear the dragging flag
   addNewRect();
    
}

function handleMouseMove(e) {
    e.preventDefault();
    e.stopPropagation();

    // if we're not dragging, just return
    if (!isDown) {
        return;
    }
    scrollX = $canvas.scrollLeft();
		scrollY = $canvas.scrollTop();

    // get the current mouse position
    mouseX = parseInt(e.clientX - offsetX - scrollX);
    mouseY = parseInt(e.clientY - offsetY - scrollY);

    // Put your mousemove stuff here

    // clear the canvas
  // ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.drawImage(img,0,0);
  


    // calculate the rectangle width/height based
    // on starting vs current mouse position
    var width = mouseX - startX;
    var height = mouseY - startY;

    // draw a new rect from the start position 
    // to the current mouse position
    ctx.strokeRect(startX, startY, width, height);
    ctx.fillText(numrects,10,10);
    ctx.fillText($canvas.scrollLeft(),100,100);
    drawRects();
    
    

}

function drawRects(){
	for (i=0; i < rects.length; i++ ){
		ctx.strokeRect(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
    }
}

// listen for mouse events
$("#canvas").mousedown(function (e) {
    handleMouseDown(e);
});
$("#canvas").mousemove(function (e) {
    handleMouseMove(e);
});
$("#canvas").mouseup(function (e) {
    handleMouseUp(e);
});
$("#canvas").mouseout(function (e) {
    handleMouseOut(e);
});
