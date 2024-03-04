#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>
#include <ESP32Servo.h>
#include <FS.h>

#define PAN_PIN 14
#define TILT_PIN 15

Servo panServo;
Servo tiltServo;

struct MOTOR_PINS
{
  int pinEn;
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS> motorPins =
    {
        {2, 12, 13}, // RIGHT_MOTOR Pins (EnA, IN1, IN2)
        {2, 1, 3},   // LEFT_MOTOR  Pins (EnB, IN3, IN4)
};
#define LIGHT_PIN 4

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int PWMSpeedChannel = 2;
const int PWMLightChannel = 3;

// Camera related constants
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

const char *ssid = "MyWiFiCar";
const char *password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;


// Include the HTML content from the file
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(

<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Controller Ui</title>
    <style>
      body {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
        height: 100vh;
        overflow-x: hidden;
      }
      .bg-slate-50 {
        background: #f8fafc;
      }
      .bg-slate-100 {
        background-color: #f1f5f9;
      }
      .border-slate-800 {
        border-color: #1e293b !important;
      }
      .container {
        display: flex;
        justify-content: center;
        align-items: center;
      }
      .camera-container {
        padding: 5px;
        width: 100%;
        text-align: center;
      }
      .full-screen {
        margin: 20px;
        width: 100% !important;
        height: 93vh !important;
      }
      .camera-view {
        width: 80%;
        /* max-width: 50%; */
        height: 350px;
        border: 1.5px solid #6b7280;
        border-radius: 10px;
        position: relative;
        /* overflow: hidden; */
      }
      .bg-camera-image {
        position: absolute;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        border-radius: 10px;
        object-fit: cover;
      }
      .camera-button {
        position: absolute;
        bottom: 0px;
        right: -70px;
        background-color: #1d4ed8;
        color: white;
        width: 50px;
        height: auto;
        padding: 5px 10px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        transition: 0.3s;
      }
      .full-screen .camera-button {
        bottom: 50px;
        right: 70px;
      }
      .camera-button:hover {
        background-color: #1e40af;
      }
      .camera-button-icon {
        width: 30px;
        height: 30px;
      }
      .scope-image {
        position: absolute;
        top: 7.5%;
        left: 36.5%;
      }
      .controller-container {
        display: flex;
        justify-content: center;
        align-items: center;
        margin-top: 20px;
      }
      .controller-content {
        width: 75%;
        height: auto;
        background-color: white;
        padding: 20px;
        border-radius: 10px;
        border: 1px solid #6b7280;
        display: flex;
        gap: 10px;
        justify-content: space-between;
        margin-bottom: 50px;
      }
      .controller-circle {
        width: 40%;
        height: auto;
        display: flex;
        justify-content: center;
        align-items: center;
        /* background-color: red; */
      }
      .controller-range {
        width: 60%;
        border-left: 1px solid #6b7280;
        padding-left: 10px;
        /* background-color: blue; */
      }

      @media only screen and (max-width: 600px) {
        body {
          width: 100%;
          background-color: #1f2937 !important;
          overflow: scroll;
        }
        .camera-container {
          display: block;
          width: auto;
        }
        .camera-view {
          width: 100%;
        }
        .full-screen {
          margin: 0px 0px 35px 0px !important;
        }
        .camera-button {
          right: 10px;
          bottom: 10px;
        }
        .full-screen .camera-button {
          bottom: 20px;
          right: 20px;
        }
        .scope-image {
          top: 10px;
          left: 100px;
        }
        .controller-container {
          width: 100%;
          display: block;
        }
        .controller-content {
          width: auto;
          flex-direction: column;
          padding: 10px 0;
        }
        .controller-circle {
          width: 100%;
        }
        .controller-range {
          width: auto;
          padding: 10px;
        }
      }

      .vertical-line,
      .horizontal-line {
        position: absolute;
        background-color: #6b7280;
      }

      .vertical-line {
        width: 2px;
      }

      .vertical-line-top {
        height: calc(50% - 30px - 20%);
        top: 20%;
        left: 50%;
      }

      .vertical-line-bottom {
        height: calc(50% - 30px - 20%);
        /* 50% - 25px - 20% */
        bottom: 20%;
        left: 50%;
      }

      .horizontal-line {
        height: 2px;
      }
      .horizontal-line-left {
        width: calc(50% - 30px - 30%);
        top: 50%;
        left: 30%;
      }
      .horizontal-line-right {
        right: 0;
        width: calc(50% - 30px - 30%);
        top: 50%;
        right: 30%;
      }
      .hole {
        position: absolute;
        width: 50px;
        height: 50px;
        border: 1px solid #e11d48;
        /* #be123c */
        border-radius: 50%;
        background-color: transparent;
        top: calc(50% - 25px);
        left: calc(50% - 25px);
      }
      .z-50 {
        z-index: 50;
      }
      .indicator-name-container {
        margin-bottom: 10px;
        display: flex;
      }
      .indicator-group {
        display: flex;
        gap: 2px;
        margin-right: 20px;
      }
      .indicator-circle {
        width: 15px;
        height: 15px;
        border-radius: 50%;
      }
      .indicator-group:first-child .indicator-circle {
        background-color: #4f46e5;
      }
      .indicator-group:nth-child(2) .indicator-circle {
        background-color: #059669;
      }
      .indicator-group:nth-child(3) .indicator-circle {
        background-color: #ca8a04;
      }
      .indicator-group:last-child .indicator-circle {
        background-color: #e11d48;
      }

/* ========= Remote Css Start */
/* Main Button Class */

.button {
  display: block;
  width: 46%;
  height: 46%;
  margin: 2%;
  position: relative;
  float: left;
  cursor: pointer;
  box-shadow: 1px 0px 3px 1px rgba(0, 0, 0, 0.4), inset 0 0 0 1px #000;
}
.button::after {
  content: "";
  display: block;
  width: 50%;
  height: 50%;
  background: #191919;
  position: absolute;
  border-radius: inherit;
}

.icon {
  cursor: pointer;
  width: 50px;
  height: 50px;
}

.icon-color {
  fill: #4f46e5;
}

#center,
#top,
#bottom,
#left,
#right {
  position: absolute;
  transform: translateY(-50%);
  transform: translateX(-50%);
  transform: rotate(-45deg);
  color: #999;
  z-index: 99999;
  font-size: 2.5rem;
}

.center-button {
  display: block;
  height: 38%;
  width: 38%;
  position: absolute;
  top: 31%;
  left: 31%;
  background: #212121;
  border-radius: 50%;
  box-shadow: 1px 0 4px rgba(0, 0, 0, 0.8);
}

#center {
  top: 27%;
  left: 40%;
  display: flex;
  flex-direction: column;
  text-align: center;
}
#center .mileage-label {
  font-size: 20px;
  color: #14b8a6;
}
#center .mileage {
  border-radius: 5px;
  padding: 5px;
  font-size: 20px;
  text-align: center;
  background-color: #2a272a;
}

.remote-container {
  width: 300px;
  height: 300px;
  background: #191919;
  border-radius: 50%;
  padding: 20px;
  transform: rotate(45deg);
  box-shadow: inset -10px 0 12px -6px #111111, inset 12px 0 5px -6px #222222,
    inset 0 0 0 10px #222222, inset 2px 0 4px 10px rgba(0, 0, 0, 0.4),
    1px 0 4px rgba(0, 0, 0, 0.8);
  box-sizing: border-box;
  margin: 5px auto;
}

/* Top - Button */

.button.top {
  border-radius: 100% 0 0 0;
  background: -webkit-radial-gradient(
    bottom right,
    ellipse cover,
    #212121 35%,
    #292929 75%
  );
  transition: transform 0.3s ease;
}
.button.top::after {
  bottom: 0;
  right: 0;
  box-shadow: inset 2px 1px 2px 0 rgba(0, 0, 0, 0.4), 10px 10px 0 10px #191919;
  -webkit-transform: skew(-3deg, -3deg) scale(0.96);
  -moz-transform: skew(-3deg, -3deg) scale(0.96);
  transform: skew(-3deg, -3deg) scale(0.96);
}

#top {
  top: 7%;
  left: 22%;
}

.button.top:active {
  background: -webkit-radial-gradient(
    bottom right,
    ellipse cover,
    #4f46e5 35%,
    #292929 75%
  );
  background: radial-gradient(
    ellipse cover at bottom right,
    #4f46e5 35%,
    #292929 75%
  );
  transform: scale(0.95);
}

/* Bottom - Button */

.button.bottom {
  border-radius: 0 0 100% 0;
  background: -webkit-radial-gradient(
    top left,
    ellipse cover,
    #212121 35%,
    #292929 75%
  );
  transition: transform 0.3s ease;
}

.button.bottom::after {
  top: 0;
  left: 0;
  box-shadow: inset -2px -3px 2px -2px rgba(0, 0, 0, 0.4),
    -10px -10px 0 10px #191919;
  -webkit-transform: skew(-3deg, -3deg) scale(0.96);
  -moz-transform: skew(-3deg, -3deg) scale(0.96);
  transform: skew(-3deg, -3deg) scale(0.96);
}

#bottom {
  top: 50%;
  left: 64%;
}

.button.bottom:active {
  background: -webkit-radial-gradient(
    top left,
    ellipse cover,
    #4f46e5 35%,
    #292929 75%
  );
  background: radial-gradient(
    ellipse cover at top left,
    #4f46e5 35%,
    #292929 75%
  );
  transform: scale(0.95);
}

/* Left-button */

.button.left {
  border-radius: 0 0 0 100%;
  background: -webkit-radial-gradient(
    top right,
    ellipse cover,
    #212121 35%,
    #292929 75%
  );
  transition: transform 0.3s ease;
}

.button.left::after {
  top: 0;
  right: 0;
  box-shadow: inset 2px -1px 2px 0 rgba(0, 0, 0, 0.4), 10px -10px 0 10px #191919;
  -webkit-transform: skew(3deg, 3deg) scale(0.96);
  -moz-transform: skew(3deg, 3deg) scale(0.96);
  transform: skew(3deg, 3deg) scale(0.96);
}

#left {
  top: 48%;
  left: 21%;
}

.button.left:active {
  background: -webkit-radial-gradient(
    top right,
    ellipse cover,
    #4f46e5 35%,
    #292929 75%
  );
  background: radial-gradient(
    ellipse cover at top right,
    #4f46e5 35%,
    #292929 75%
  );
  transform: scale(0.95);
}

/* Right-button */

.button.right {
  border-radius: 0 100% 0 0;
  background: -webkit-radial-gradient(
    bottom left,
    ellipse cover,
    #212121 35%,
    #292929 75%
  );
  transition: transform 0.3s ease;
}

.button.right::after {
  bottom: 0;
  left: 0;
  box-shadow: inset -2px 3px 2px -2px rgba(0, 0, 0, 0.4),
    -10px 10px 0 10px #191919;
  -webkit-transform: skew(3deg, 3deg) scale(0.96);
  -moz-transform: skew(3deg, 3deg) scale(0.96);
  transform: skew(3deg, 3deg) scale(0.96);
}

#right {
  top: 7%;
  left: 65%;
}

.button.right:active {
  background: -webkit-radial-gradient(
    bottom left,
    ellipse cover,
    #4f46e5 35%,
    #292929 75%
  );
  background: radial-gradient(
    ellipse cover at bottom left,
    #4f46e5 35%,
    #292929 75%
  );
  transform: scale(0.95);
}

  /* ========= Remote Css End  */
  /* ========= Slider Css Start */
      .slider-container {
  border: 1px solid #1e293b;
  border-radius: 5px;
  width: auto;
  height: auto;
  padding: 10px;
  background-color: white;
  margin-bottom: 10px;

  /* background: -webkit-radial-gradient(
    top left,
    ellipse cover,
    #212121 35%,
    #292929 75%
  ); */
  /* box-shadow: 0px 5px 20px rgb(117, 116, 116); */
}

.slider {
  position: relative;
  /* display: flex; */
}
.top-container {
  margin-bottom: 10px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.slider_label {
  position: relative;
  display: block;
  font-size: 20px;
  /* color: #cbd5e1; */
  color: #4f46e5;
  text-shadow: 2px 2px 2px rgba(127, 126, 126, 0.42);
}
.range_value {
  font-size: 25px;
  color: #cbd5e1;
  font-weight: 400;
  background-color: #6b7280;
  padding: 1px 10px;
  border-radius: 5px;
  box-shadow: inset 2px 1px 2px 0 rgba(0, 0, 0, 0.4);
}
.range {
  width: 100%;
  height: 15px;
  -webkit-appearance: none;
  background-color: #334155;
  outline: none;
  border: 1px solid #6b7280;
  border-radius: 15px;
  overflow: hidden;
  box-shadow: inset 0 0 5px rgba(0, 0, 0, 1);
}
.range::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 15px;
  height: 15px;
  border-radius: 50%;
  background-color: #4f46e5;
  cursor: pointer;
  border: 2px solid #332c49;
  box-shadow: -407px 0 0 400px #4f46e5;
  position: relative;
}

.indicator {
  width: 100px;
  height: 20px;
  border-radius: 5px;
  border: 1px solid #e2e8f0;
  background-color: #475569;
  box-shadow: inset 3px 2px 3px 2px rgba(0, 0, 0, 0.4);
}

      /* ========= Slider Css End */
    </style>
  </head>
  <body class="bg-slate-50">
    <div class="camera-container container">
      <div class="camera-view" id="camera-view">
        <div class="vertical-line vertical-line-top z-50"></div>
        <div class="vertical-line vertical-line-bottom z-50"></div>
        <div class="horizontal-line horizontal-line-left z-50"></div>
        <div class="horizontal-line horizontal-line-right z-50"></div>
        <div class="hole z-50"></div>
        <img id="cameraImage" src="" alt="camera image">
        <button class="camera-button" id="camera-button">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            fill="none"
            viewBox="0 0 24 24"
            stroke-width="1.5"
            stroke="currentColor"
            class="camera-button-icon"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              d="M7.5 3.75H6A2.25 2.25 0 0 0 3.75 6v1.5M16.5 3.75H18A2.25 2.25 0 0 1 20.25 6v1.5m0 9V18A2.25 2.25 0 0 1 18 20.25h-1.5m-9 0H6A2.25 2.25 0 0 1 3.75 18v-1.5M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z"
            />
          </svg>
        </button>
      </div>
    </div>
    <div class="controller-container">
      <div class="controller-content">
        <div class="controller-circle">
          <div class="remote-container">
            <div
              class="button top"
              href="#"
              ontouchstart='sendButtonInput("MoveCar","1")'
              ontouchend='sendButtonInput("MoveCar","0")'
            ></div>
            <div
              class="button right"
              href="#"
              ontouchstart='sendButtonInput("MoveCar","4")'
              ontouchend='sendButtonInput("MoveCar","0")'
            ></div>
            <div
              class="button left"
              href="#"
              ontouchstart='sendButtonInput("MoveCar","3")'
              ontouchend='sendButtonInput("MoveCar","0")'
            ></div>
            <div
              class="button bottom"
              href="#"
              ontouchstart='sendButtonInput("MoveCar","2")'
              ontouchend='sendButtonInput("MoveCar","0")'
            ></div>
            <div class="center-button" href="#"></div>
            <p id="center">
              <span class="mileage-label"> MPH </span>
              <span class="mileage"> <span id="mileage">550</span> % </span>
            </p>
            <p id="top">
              <svg
                class="icon"
                viewBox="0 0 24 24"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  class="icon-color"
                  d="M17.7 17.29l-5-5c-.4-.4-1.03-.4-1.42 0l-5 5c-.4.39-.4 1.02 0 1.41 .39.39 1.02.39 1.41 0l5-5h-1.42l5 5c.39.39 1.02.39 1.41 0 .39-.4.39-1.03 0-1.42Zm0-7l-5-5c-.4-.4-1.03-.4-1.42 0l-5 5c-.4.39-.4 1.02 0 1.41 .39.39 1.02.39 1.41 0l5-5h-1.42l5 5c.39.39 1.02.39 1.41 0 .39-.4.39-1.03 0-1.42Z"
                />
              </svg>
            </p>
            <p id="left">
              <svg
                class="icon"
                viewBox="0 0 24 24"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  class="icon-color"
                  d="M18.7 16.29l-5-5v1.41l5-5c.39-.4.39-1.03 0-1.42 -.4-.4-1.03-.4-1.42 0l-5 5c-.4.39-.4 1.02 0 1.41l5 5c.39.39 1.02.39 1.41 0 .39-.4.39-1.03 0-1.42Zm-7 0l-5-5v1.41l5-5c.39-.4.39-1.03 0-1.42 -.4-.4-1.03-.4-1.42 0l-5 5c-.4.39-.4 1.02 0 1.41l5 5c.39.39 1.02.39 1.41 0 .39-.4.39-1.03 0-1.42Z"
                />
              </svg>
            </p>
            <p id="bottom">
              <svg
                class="icon"
                viewBox="0 0 24 24"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  class="icon-color"
                  d="M6.29 13.7l5 5c.39.39 1.02.39 1.41 0l5-5c.39-.4.39-1.03 0-1.42 -.4-.4-1.03-.4-1.42 0l-5 5h1.41l-5-5c-.4-.4-1.03-.4-1.42 0 -.4.39-.4 1.02 0 1.41Zm0-7l5 5c.39.39 1.02.39 1.41 0l5-5c.39-.4.39-1.03 0-1.42 -.4-.4-1.03-.4-1.42 0l-5 5h1.41l-5-5c-.4-.4-1.03-.4-1.42 0 -.4.39-.4 1.02 0 1.41Z"
                />
              </svg>
            </p>
            <p id="right">
              <svg
                class="icon"
                viewBox="0 0 24 24"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  class="icon-color"
                  d="M6.7 17.7l5-5c.39-.4.39-1.03 0-1.42l-5-5c-.4-.4-1.03-.4-1.42 0 -.4.39-.4 1.02 0 1.41l5 5v-1.42l-5 5c-.4.39-.4 1.02 0 1.41 .39.39 1.02.39 1.41 0Zm7 0l5-5c.39-.4.39-1.03 0-1.42l-5-5c-.4-.4-1.03-.4-1.42 0 -.4.39-.4 1.02 0 1.41l5 5v-1.42l-5 5c-.4.39-.4 1.02 0 1.41 .39.39 1.02.39 1.41 0Z"
                />
              </svg>
            </p>
          </div>
        </div>
        <div class="controller-range">
          <div class="indicator-name-container">
            <div class="indicator-group">
              <div class="indicator-circle"></div>
              <small>Low</small>
            </div>
            <div class="indicator-group">
              <div class="indicator-circle"></div>
              <small>Normal</small>
            </div>
            <div class="indicator-group">
              <div class="indicator-circle"></div>
              <small>High</small>
            </div>
            <div class="indicator-group">
              <div class="indicator-circle"></div>
              <small>Max</small>
            </div>
          </div>
          <div class="slider-container">
            <div class="slider">
              <div class="top-container">
                <span class="slider_label">
                  Speed:
                  <span id="range_value_speed" class="range_value">0</span>
                </span>
                <div class="indicator-container">
                  <div id="indicator_speed" class="indicator"></div>
                </div>
              </div>
              <input
                type="range"
                name=""
                id="speed"
                class="range"
                min="0"
                max="255"
                value="150"
                oninput='sendButtonInput("Speed",value)'
              />
            </div>
          </div>
          <div class="slider-container">
            <div class="slider">
              <div class="top-container">
                <span class="slider_label">
                  Light:
                  <span id="range_value_light" class="range_value">0</span>
                </span>
                <div class="indicator-container">
                  <div id="indicator_light" class="indicator"></div>
                </div>
              </div>
              <input
                type="range"
                name=""
                id="light"
                class="range"
                min="0"
                max="255"
                value="0"
                oninput='sendButtonInput("Light",value)'
              />
            </div>
          </div>
          <div class="slider-container">
            <div class="slider">
              <div class="top-container">
                <span class="slider_label">
                  Horizontal:
                  <span id="range_value_horizontal" class="range_value">0</span>
                </span>
                <div class="indicator-container">
                  <div id="indicator_horizontal" class="indicator"></div>
                </div>
              </div>
              <input
                type="range"
                name=""
                id="horizontal"
                class="range"
                min="0"
                max="180"
                value="90"
                oninput='sendButtonInput("Pan",value)'
              />
            </div>
          </div>
          <div class="slider-container">
            <div class="slider">
              <div class="top-container">
                <span class="slider_label">
                  Vertical:
                  <span id="range_value_vertical" class="range_value">0</span>
                </span>
                <div class="indicator-container">
                  <div id="indicator_vertical" class="indicator"></div>
                </div>
              </div>
              <input
                type="range"
                name=""
                id="vertical"
                class="range"
                min="0"
                max="180"
                value="90"
                oninput='sendButtonInput("Tilt",value)'
              />
            </div>
          </div>
        </div>
      </div>
    </div>
    <script>
    // App Init Start
const speedSlider = document.getElementById("speed");
const lightSlider = document.getElementById("light");
const horizontalSlider = document.getElementById("horizontal");
const verticalSlider = document.getElementById("vertical");
const cameraView = document.getElementById("camera-view");
const mileageValue = document.getElementById("mileage");
const fullScreen = document.getElementById("camera-button");

fullScreen.addEventListener("click", () => {
  cameraView.classList.toggle("full-screen");
});

const speedMaxLimit = speedSlider.attributes.max.value;
const lightMaxLimit = lightSlider.attributes.max.value;
const horizontalMaxLimit = horizontalSlider.attributes.max.value;
const verticalMaxLimit = verticalSlider.attributes.max.value;

showRangerValue(speedSlider.value, "speed");
showRangerValue(lightSlider.value, "light");
showRangerValue(horizontalSlider.value, "horizontal");
showRangerValue(verticalSlider.value, "vertical");

mileageValue.innerHTML = speedSlider.attributes.value.value;
speedSlider.addEventListener("click", function () {
  showRangerValue(this.value, "speed");
  mileageValue.innerHTML = this.value;
});
speedSlider.addEventListener("mousemove", function () {
  showRangerValue(this.value, "speed");
  mileageValue.innerHTML = this.value;
});

lightSlider.addEventListener("click", function () {
  showRangerValue(this.value, "light");
});
lightSlider.addEventListener("mousemove", function () {
  showRangerValue(this.value, "light");
});

horizontalSlider.addEventListener("click", function () {
  showRangerValue(this.value, "horizontal");
});
horizontalSlider.addEventListener("mousemove", function () {
  showRangerValue(this.value, "horizontal");
});

verticalSlider.addEventListener("click", function () {
  showRangerValue(this.value, "vertical");
});
verticalSlider.addEventListener("mousemove", function () {
  showRangerValue(this.value, "vertical");
});

function showRangerValue(value, name) {
  const indicator = document.getElementById("indicator_" + name);
  const showValue = document.getElementById("range_value_" + name);
  showValue.innerHTML = value;
  let limit;
  if (name === "speed") {
    limit = speedMaxLimit;
  } else if (name === "light") {
    limit = lightMaxLimit;
  } else if (name === "horizontal") {
    limit = horizontalMaxLimit;
  } else if (name === "vertical") {
    limit = verticalMaxLimit;
  } else {
    limit = 0;
  }
  // const indicatorColor = getIndicatorColor(value);
  const indicatorColor = getIndicatorColorByLimit(value, limit);
  indicator.style.backgroundColor = indicatorColor;
}

function getIndicatorColor(value) {
  if (value >= 1 && value < 40) {
    return "#4f46e5";
  } else if (value >= 40 && value < 60) {
    return "#059669";
  } else if (value >= 60 && value < 80) {
    return "#ca8a04";
  } else if (value > 80) {
    return "#e11d48";
  } else {
    return "#475569";
  }
}
function getIndicatorColorByLimit(value, limit) {
  if (value >= 1 && value < limit * 0.4) {
    return "#4f46e5";
  } else if (value >= limit * 0.4 && value < limit * 0.6) {
    return "#059669";
  } else if (value >= limit * 0.6 && value < limit * 0.8) {
    return "#ca8a04";
  } else if (value > limit * 0.8) {
    return "#e11d48";
  } else {
    return "#475569";
  }
}

    // App Init End
    // Controller Api Start
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
    // Controller Api End
    </script>
  </body>
</html>

)HTMLHOMEPAGE";

void rotateMotor(int motorNumber, int motorDirection)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  }
  else
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
}

void moveCar(int inputValue)
{
  Serial.printf("Got value as %d\n", inputValue);
  switch (inputValue)
  {

  case UP:
    rotateMotor(RIGHT_MOTOR, FORWARD);
    rotateMotor(LEFT_MOTOR, FORWARD);
    break;

  case DOWN:
    rotateMotor(RIGHT_MOTOR, BACKWARD);
    rotateMotor(LEFT_MOTOR, BACKWARD);
    break;

  case LEFT:
    rotateMotor(RIGHT_MOTOR, FORWARD);
    rotateMotor(LEFT_MOTOR, BACKWARD);
    break;

  case RIGHT:
    rotateMotor(RIGHT_MOTOR, BACKWARD);
    rotateMotor(LEFT_MOTOR, FORWARD);
    break;

  case STOP:
    rotateMotor(RIGHT_MOTOR, STOP);
    rotateMotor(LEFT_MOTOR, STOP);
    break;

  default:
    rotateMotor(RIGHT_MOTOR, STOP);
    rotateMotor(LEFT_MOTOR, STOP);
    break;
  }
}

void handleRoot(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server,
                              AsyncWebSocketClient *client,
                              AwsEventType type,
                              void *arg,
                              uint8_t *data,
                              size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    moveCar(0);
    ledcWrite(PWMLightChannel, 0);
    panServo.write(90);
    tiltServo.write(90);
    break;
  case WS_EVT_DATA:
    AwsFrameInfo *info;
    info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
      std::string myData = "";
      myData.assign((char *)data, len);
      std::istringstream ss(myData);
      std::string key, value;
      std::getline(ss, key, ',');
      std::getline(ss, value, ',');
      Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
      int valueInt = atoi(value.c_str());
      if (key == "MoveCar")
      {
        moveCar(valueInt);
      }
      else if (key == "Speed")
      {
        ledcWrite(PWMSpeedChannel, valueInt);
      }
      else if (key == "Light")
      {
        ledcWrite(PWMLightChannel, valueInt);
      }
      else if (key == "Pan")
      {
        panServo.write(valueInt);
      }
      else if (key == "Tilt")
      {
        tiltServo.write(valueInt);
      }
    }
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  default:
    break;
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server,
                            AsyncWebSocketClient *client,
                            AwsEventType type,
                            void *arg,
                            uint8_t *data,
                            size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    cameraClientId = client->id();
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    cameraClientId = 0;
    break;
  case WS_EVT_DATA:
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  default:
    break;
  }
}

void setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_4;
  config.ledc_timer = LEDC_TIMER_2;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  if (psramFound())
  {
    heap_caps_malloc_extmem_enable(20000);
    Serial.printf("PSRAM initialized. malloc to take memory from psram above this size");
  }
}

void sendCameraPicture()
{
  if (cameraClientId == 0)
  {
    return;
  }
  unsigned long startTime1 = millis();
  // capture a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Frame buffer could not be acquired");
    return;
  }

  unsigned long startTime2 = millis();
  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);

  // Wait for message to be delivered
  while (true)
  {
    AsyncWebSocketClient *clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull()))
    {
      break;
    }
    delay(1);
  }

  unsigned long startTime3 = millis();
  Serial.printf("Time taken Total: %d|%d|%d\n", startTime3 - startTime1, startTime2 - startTime1, startTime3 - startTime2);
}

void setUpPinModes()
{
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

  // Set up PWM
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);
  ledcSetup(PWMLightChannel, PWMFreq, PWMResolution);

  for (int i = 0; i < motorPins.size(); i++)
  {
    pinMode(motorPins[i].pinEn, OUTPUT);
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
    /* Attach the PWM Channel to the motor enb Pin */
    ledcAttachPin(motorPins[i].pinEn, PWMSpeedChannel);
  }
  moveCar(STOP);

  pinMode(LIGHT_PIN, OUTPUT);
  ledcAttachPin(LIGHT_PIN, PWMLightChannel);
}

void setup(void){
  setUpPinModes();
  // Serial.begin(115200);

  // Define new ip address
  IPAddress ip(192, 168, 1, 100);
  // Config new ip address
  WiFi.softAPConfig(ip);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started");

  setupCamera();
}

void loop()
{
  wsCamera.cleanupClients();
  wsCarInput.cleanupClients();
  sendCameraPicture();
  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
}
