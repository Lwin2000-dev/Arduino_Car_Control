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
