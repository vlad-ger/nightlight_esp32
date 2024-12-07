#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <StreamString.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define LED_PIN     5
#define LED_COUNT  12
#define BRIGHTNESS 20 // (max = 255)
#define FADE  1
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB);

const char* ssid = "FRITZ!Box 6660 Cable TB";
const char* password = "30818744673088044484";
IPAddress local_IP(192, 168, 72, 200);
IPAddress gateway(192, 168, 72, 1);
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(80);

int fade = 500;
bool stopFlag = false;
bool running = false;

void setup() {
  Serial.begin(115200);  
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  if (!WiFi.config(local_IP, gateway, subnet)) {
  Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", handleRoot);
  server.on("/colors", handleSetColor);
  server.on("/fade", handleSetFade);
  server.on("/brightness", handleSetBrightness);
  server.on("/rainbow", rainbowFade);
  server.on("/stop", handleStop);
  server.begin();
  Serial.println("HTTP server started");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void handleRoot() {
  StreamString temp;
  temp.reserve(5000);
  temp.printf("<!DOCTYPE html>\
<html>\
<head>\
    <meta charset='UTF-8'>\
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\
    <title>Nachtlicht</title>\
    <style>\
        .color-box {\
            width: 90px;\
            height: 90px;\
            display: inline-block;\
            margin: 10px;\
            cursor: pointer;\
            padding: 10px;\
        }\
        .selected {\
            border: 10px solid white;\
            padding: 0px;\
        }\
    </style>\
</head>\
<body style='background-color: black;'>\
    <style>\
        #brightness-slider {\
            width: 45%%;\
            height: 100px;\
            margin-inline: 2%%;\
        }\
        #fade-slider {\
            width: 45%%;\
            height: 100px;\
            margin-inline: 2%%;\
        }\
    </style>\
    <div id='sliders'></div>\
    <div id='color-chart'></div>\
    <script>\
        function generateRGBColors() {\
            const colors = [];\
            for (let r = 0; r <= 255; r += 51) {\
                for (let g = 0; g <= 255; g += 51) {\
                    for (let b = 0; b <= 255; b += 51) {\
                        colors.push(`rgb(${r},${g},${b})`);\
                    }\
                }\
            }\
            return colors;\
        }\
        const colors = generateRGBColors();\
        const brightnessSlider = document.createElement('input');\
        brightnessSlider.type = 'range';\
        brightnessSlider.min = '0';\
        brightnessSlider.max = '200';\
        brightnessSlider.value = '60';\
        brightnessSlider.id = 'brightness-slider';\
        document.getElementById('sliders').appendChild(brightnessSlider);\
        let brightnessTimeout;\
        brightnessSlider.addEventListener('change', () => {\
            clearTimeout(brightnessTimeout);\
            const brightness = brightnessSlider.value;\
            brightnessTimeout = setTimeout(() => {\
                fetch('http://192.168.72.200/brightness', {\
                    method: 'POST',\
                    headers: {\
                        'Content-Type': 'application/json'\
                    },\
                    body: JSON.stringify({ brightness: brightness})\
                })\
                .then(response => response.json())\
                .then(data => console.log('Success:', data))\
                .catch((error) => console.error('Error:', error));\
            }, 300);\
        });\
        const fadeSlider = document.createElement('input');\
        fadeSlider.type = 'range';\
        fadeSlider.min = '25';\
        fadeSlider.max = '500';\
        fadeSlider.value = '500';\
        fadeSlider.id = 'fade-slider';\
        document.getElementById('sliders').appendChild(fadeSlider);\
        let fadeTimeout;\
        fadeSlider.addEventListener('change', () => {\
            clearTimeout(fadeTimeout);\
            const fade = fadeSlider.value;\
            fadeTimeout = setTimeout(() => {\
                fetch('http://192.168.72.200/fade', {\
                    method: 'POST',\
                    headers: {\
                        'Content-Type': 'application/json'\
                    },\
                    body: JSON.stringify({ fade: fade })\
                })\
                .then(response => response.json())\
                .then(data => console.log('Success:', data))\
                .catch((error) => console.error('Error:', error));\
            }, 300);\
        });\
        const rainbowButton = document.createElement('button');\
        rainbowButton.style.display = 'block';\
        rainbowButton.style.margin = '20px auto';\
        rainbowButton.style.padding = '20px 100px';\
        rainbowButton.style.background = 'linear-gradient(to right, red, orange, yellow, green, blue, indigo, violet)';\
        rainbowButton.style.border = 'none';\
        rainbowButton.style.borderRadius = '5px';\
        document.body.insertBefore(rainbowButton, document.getElementById('color-chart'));\
        let rainbowTimeout;\
        rainbowButton.addEventListener('click', () => {\
            clearTimeout(rainbowTimeout);\
            rainbowTimeout = setTimeout(() => {\
                fetch('http://192.168.72.200/stop', {\
                    method: 'POST'\
                })\
                .then(response => response.json())\
                .then(data => console.log('Success:', data))\
                .catch((error) => console.error('Error:', error));\
                fetch('http://192.168.72.200/rainbow', {\
                    method: 'POST',\
                    headers: {\
                        'Content-Type': 'application/json'\
                    },\
                    body: JSON.stringify({ mode: 'rainbow' })\
                })\
                .then(response => response.json())\
                .then(data => console.log('Success:', data))\
                .catch((error) => console.error('Error:', error));\
            }, 300);\
        });\
        const colorChart = document.getElementById('color-chart');\
        colors.forEach(color => {\
            const colorBox = document.createElement('div');\
            colorBox.className = 'color-box';\
            colorBox.style.backgroundColor = color;\
            let colorTimeout;\
            colorBox.addEventListener('click', () => {\
                if (document.querySelectorAll('.color-box.selected').length < 5 || colorBox.classList.contains('selected')) {\
                    colorBox.classList.toggle('selected');\
                    const selectedColors = Array.from(document.querySelectorAll('.color-box.selected')).map(box => box.style.backgroundColor);\
                    clearTimeout(colorTimeout);\
                    colorTimeout = setTimeout(() => {\
                        fetch('http://192.168.72.200/stop', {\
                            method: 'POST'\
                        })\
                        .then(response => response.json())\
                        .then(data => console.log('Success:', data))\
                        .catch((error) => console.error('Error:', error));\
                        fetch('http://192.168.72.200/colors', {\
                            method: 'POST',\
                            headers: {\
                                'Content-Type': 'application/json'\
                            },\
                            body: JSON.stringify({ colors: selectedColors})\
                        })\
                        .then(response => response.json())\
                        .then(data => console.log('Success:', data))\
                        .catch((error) => console.error('Error:', error));\
                    }, 300);\
                }\
            });\
            colorChart.appendChild(colorBox);\
        });\
    </script>\
</body>\
</html>");
  server.send(200, "text/html", temp.c_str());
}

void rainbowFade() {
  if (server.hasArg("plain")) {
    server.send(200, "text/plain", "Rainbow fade started");
    uint16_t i, j;
    while (true) {
      running = true;
      for (j = 0; j < 256; j++) {
        for (i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, Wheel((i + j) & 255));
          strip.show();
          if (stopFlag) {
            stopFlag = false;
            running = false;
            return;
          }
          delay(fade);
          server.handleClient();
          yield();
        }
      }
    }
  } else {
    server.send(400, "text/plain", "Invalid arguments");
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void handleSetColor() {
  if (server.hasArg("plain")) {
    String colors = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, colors);
    JsonArray colorArray = doc["colors"];
    while (true) {
      running = true;
      for (int i = 0; i < colorArray.size(); i++) {
        String colorStr = colorArray[i];
        int r, g, b;
        sscanf(colorStr.c_str(), "rgb(%d,%d,%d)", &r, &g, &b);
        for (int j = 0; j < strip.numPixels(); j++) {
          strip.setPixelColor(j, strip.Color(r, g, b));
          strip.show();
          if (stopFlag) {
            stopFlag = false;
            running = false;
            return;
          }
          delay(fade);
          server.handleClient();
          yield();
        }
      }
    }
  } else {
    server.send(400, "text/plain", "Invalid arguments");
  }
}

void handleSetFade() {
  if (server.hasArg("plain")) {
    String fadeData = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, fadeData);
    fade = doc["fade"];
    server.send(200, "text/plain", "Fade set");
  } else {
    server.send(400, "text/plain", "Invalid arguments");
  }
}

void handleSetBrightness() {
  if (server.hasArg("plain")) {
    String brightnessData = server.arg("plain");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, brightnessData);
    int brightness = doc["brightness"];
    strip.setBrightness(brightness);
    strip.show();
    server.send(200, "text/plain", "Brightness set");
  } else {
    server.send(400, "text/plain", "Invalid arguments");
  }
}

void handleStop() {
  if (running) {
    stopFlag = true;
    server.send(200, "text/plain", "Stopped");
  }
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
}

