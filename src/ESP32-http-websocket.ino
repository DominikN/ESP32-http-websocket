#include <Husarnet.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ArduinoJson.h>

#include <SPI.h>
#include <TFT_eSPI.h>


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

// you can provide credentials to multiple WiFi networks
WiFiMulti wifiMulti;

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

StaticJsonDocument<200> jsonDocTx;
StaticJsonDocument<100> jsonDocRx;

extern const char index_html_start[] asm("_binary_src_index_html_start");
const String html = String((const char *)index_html_start);

bool wsconnected = false;

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    wsconnected = true;
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    // client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    wsconnected = false;
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(),
                  *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_PONG) {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len,
                  (len) ? (char*)data : "");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      // the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(),
                    client->id(), (info->opcode == WS_TEXT) ? "text" : "binary",
                    info->len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char)data[i];
        }

        deserializeJson(jsonDocRx, msg);

        uint8_t ledState = jsonDocRx["led"];

        Serial.printf("LED state = %d\r\n", ledState);
        if (ledState == 1) {
          digitalWrite(LED_PIN, HIGH);
        }
        if (ledState == 0) {
          digitalWrite(LED_PIN, LOW);
        }
        jsonDocRx.clear();
      }
      Serial.printf("%s\n", msg.c_str());
    }
  }
}

void taskWifi(void* parameter);
void taskStatus(void* parameter);

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  xTaskCreate(taskWifi,   /* Task function. */
              "taskWifi", /* String with name of task. */
              20000,      /* Stack size in bytes. */
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

  /* Start web server and web socket server */

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", html);
  });
  server.onNotFound(notFound);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();

  while (1) {
    while (WiFi.status() == WL_CONNECTED) {
      ws.cleanupClients();
      delay(100);
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

      ws.textAll(output);
    }
    delay(100);
  }
}

void loop() { delay(5000); }
