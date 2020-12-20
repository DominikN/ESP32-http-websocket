#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Husarnet.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <WiFiMulti.h>

/* =============== config section start =============== */
const int BUTTON_PIN = 35;
const int LED_PIN = 17;

// Husarnet credentials
const char* hostName =
    "esp32websocket";  // this will be the name of the 1st ESP32 device at
                       // https://app.husarnet.com

#if __has_include("credentials.h")
#include "credentials.h"
#else
// WiFi credentials
#define NUM_NETWORKS 2
// Add your networks credentials here
const char* ssidTab[NUM_NETWORKS] = {
    "wifi-ssid-one",
    "wifi-ssid-two",
};
const char* passwordTab[NUM_NETWORKS] = {
    "wifi-pass-one",
    "wifi-pass-two",
};

/* to get your join code go to https://app.husarnet.com
   -> select network
   -> click "Add element"
   -> select "join code" tab

   Keep it secret!
*/
const char* husarnetJoinCode = "xxxxxxxxxxxxxxxxxxxxxx";
const char* dashboardURL = "default";
#endif
/* =============== config section end =============== */

#define HTTP_PORT 8000
#define WEBSOCKET_PORT 8001

// you can provide credentials to multiple WiFi networks
WiFiMulti wifiMulti;

WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);

AsyncWebServer server(HTTP_PORT);

StaticJsonDocument<200> jsonDocTx;

StaticJsonDocument<100> jsonDocRx;

const String html =
#include "html.h"
    ;

bool wsconnected = false;

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload,
                      size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
      wsconnected = false;
      Serial.printf("[%u] Disconnected\r\n", num);
    } break;
    case WStype_CONNECTED: {
      wsconnected = true;
      Serial.printf("\r\n[%u] Connection from Husarnet \r\n", num);
    } break;

    case WStype_TEXT: {
      Serial.printf("[%u] Text:\r\n", num);
      for (int i = 0; i < length; i++) {
        Serial.printf("%c", (char)(*(payload + i)));
      }
      Serial.println();

      deserializeJson(jsonDocRx, payload);

      uint8_t ledState = jsonDocRx["led"];

      Serial.printf("LED state = %d\r\n", ledState);
      if (ledState == 1) {
        digitalWrite(LED_PIN, HIGH);
      }
      if (ledState == 0) {
        digitalWrite(LED_PIN, LOW);
      }
      jsonDocRx.clear();
    } break;

    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

void taskWifi(void* parameter);
void taskHTTP(void* parameter);
void taskWebSocket(void* parameter);
void taskStatus(void* parameter);

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  xTaskCreate(taskWifi,   /* Task function. */
              "taskWifi", /* String with name of task. */
              10000,      /* Stack size in bytes. */
              NULL,       /* Parameter passed as input of the task */
              1,          /* Priority of the task. */
              NULL);      /* Task handle. */

  xTaskCreate(taskStatus,   /* Task function. */
              "taskStatus", /* String with name of task. */
              10000,        /* Stack size in bytes. */
              NULL,         /* Parameter passed as input of the task */
              1,            /* Priority of the task. */
              NULL);        /* Task handle. */
}

void taskWifi(void* parameter) {
  uint8_t stat = WL_DISCONNECTED;

  /* Configure Wi-Fi */
  for (int i = 0; i < NUM_NETWORKS; i++) {
    wifiMulti.addAP(ssidTab[i], passwordTab[i]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssidTab[i],
                  passwordTab[i]);
  }

  while (stat != WL_CONNECTED) {
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
    delay(100);
  }

  Serial.printf("WiFi connected\r\n", (int)stat);
  Serial.printf("IP address: ");
  Serial.println(WiFi.localIP());

  /* Start Husarnet */
  Husarnet.selfHostedSetup(dashboardURL);
  Husarnet.join(husarnetJoinCode, hostName);
  Husarnet.start();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", html);
  });
  server.onNotFound(notFound);
  server.begin();

  while (1) {
    while (WiFi.status() == WL_CONNECTED) {
      webSocket.loop();
      delay(2);
    }

    Serial.printf("WiFi disconnected, reconnecting\r\n");
    delay(500);
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
  }
}

void taskStatus(void* parameter) {
  String output;
  int cnt = 0;
  uint8_t button_status = 0;
  while (1) {
    if (wsconnected == true) {
      if (digitalRead(BUTTON_PIN) == LOW) {
        button_status = 1;
      } else {
        button_status = 0;
      }
      output = "";

      jsonDocTx.clear();
      jsonDocTx["counter"] = cnt++;
      jsonDocTx["button"] = button_status;
      serializeJson(jsonDocTx, output);

      Serial.print(F("Sending: "));
      Serial.print(output);
      Serial.println();

      webSocket.sendTXT(0, output);
    }
    delay(100);
  }
}

void loop() { delay(5000); }
