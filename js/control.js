var webSocketCameraUrl = "ws://" + window.location.hostname + "/Camera";
var webSocketCarInputUrl = "ws://" + window.location.hostname + "/CarInput";
var websocketCamera;
var websocketCarInput;

function initCameraWebSocket() {
  websocketCamera = new WebSocket(webSocketCameraUrl);
  websocketCamera.binaryType = "blob";
  websocketCamera.onopen = function (event) {};
  websocketCamera.onclose = function (event) {
    setTimeout(initCameraWebSocket, 2000);
  };
  websocketCamera.onmessage = function (event) {
    var imageId = document.getElementById("cameraImage");
    imageId.src = URL.createObjectURL(event.data);
  };
}

function initCarInputWebSocket() {
  websocketCarInput = new WebSocket(webSocketCarInputUrl);
  websocketCarInput.onopen = function (event) {
    sendButtonInput("Speed", document.getElementById("speed").value);
    sendButtonInput("Light", document.getElementById("light").value);
    sendButtonInput("Pan", document.getElementById("horizontal").value);
    sendButtonInput("Tilt", document.getElementById("vertical").value);
  };
  websocketCarInput.onclose = function (event) {
    setTimeout(initCarInputWebSocket, 2000);
  };
  websocketCarInput.onmessage = function (event) {};
}

function initWebSocket() {
  initCameraWebSocket();
  initCarInputWebSocket();
}

function sendButtonInput(key, value) {
  var data = key + "," + value;
  websocketCarInput.send(data);
}

window.onload = initWebSocket;
// document
//   .getElementById("mainTable")
//   .addEventListener("touchend", function (event) {
//     event.preventDefault();
//   });
