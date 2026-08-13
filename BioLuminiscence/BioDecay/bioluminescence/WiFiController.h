#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include <stdlib.h>
#include <string.h>
#include <WiFiS3.h>

#include "ProjectConfig.h"
#include "ModeController.h"

class WiFiController {
public:
  WiFiController() : _ready(false) {}

  bool begin() {
    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(WIFI_SSID);

    int status = WL_IDLE_STATUS;
    uint8_t attempts = 0;

    while (status != WL_CONNECTED && attempts < 20) {
      status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      attempts++;
      delay(1000);
      Serial.print('.');
    }
    Serial.println();

    _ready = (status == WL_CONNECTED);
    if (_ready) {
      _server.begin();
      Serial.print("WiFi connected. IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Open http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi connection failed. Serial mode switching remains available.");
    }

    return _ready;
  }

  void handle(ModeController& modeController, Adafruit_NeoPixel& strip) {
    if (!_ready) return;

    WiFiClient client = _server.available();
    if (!client) return;

    client.setTimeout(50);

    char requestLine[128];
    if (!readLine(client, requestLine, sizeof(requestLine))) {
      client.stop();
      return;
    }

    drainHeaders(client);

    uint8_t modeIndex = 0;
    if (extractModeIndex(requestLine, modeIndex)) {
      modeController.selectMode(modeIndex, strip);
    }

    sendPage(client, modeController);
    delay(1);
    client.stop();
  }

private:
  WiFiServer _server{WIFI_HTTP_PORT};
  bool _ready;

  bool readLine(WiFiClient& client, char* out, size_t outSize) {
    if (out == nullptr || outSize < 2) return false;
    size_t n = client.readBytesUntil('\n', out, outSize - 1);
    out[n] = '\0';
    if (n > 0 && out[n - 1] == '\r') {
      out[n - 1] = '\0';
    }
    return n > 0;
  }

  void drainHeaders(WiFiClient& client) {
    char line[128];
    while (readLine(client, line, sizeof(line))) {
      if (line[0] == '\0') break;
    }
  }

  bool extractModeIndex(const char* requestLine, uint8_t& outIndex) {
    const char* key = strstr(requestLine, "GET /mode?index=");
    if (key == nullptr) return false;

    key += strlen("GET /mode?index=");
    int parsed = atoi(key);
    if (parsed < 0 || parsed > 255) return false;

    outIndex = (uint8_t)parsed;
    return true;
  }

  void sendPage(WiFiClient& client, ModeController& modeController) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<title>BioLighting Modes</title>");
    client.println("<style>body{font-family:Arial,sans-serif;background:#07131a;color:#dff7ff;margin:0;padding:24px;}h1{font-size:28px;}p{color:#9fc2cf;}button{display:block;width:100%;max-width:320px;margin:12px 0;padding:16px;border:0;border-radius:12px;background:#0f7c8c;color:white;font-size:18px;}button.active{background:#33b36b;}a{text-decoration:none;}</style></head><body>");
    client.println("<h1>BioLighting Mode Control</h1>");
    client.print("<p>Current mode: ");
    client.print(modeController.modeName(modeController.activeIndex()));
    client.println("</p>");

    for (uint8_t i = 0; i < modeController.modeCount(); ++i) {
      client.print("<a href='/mode?index=");
      client.print(i);
      client.print("'><button");
      if (i == modeController.activeIndex()) {
        client.print(" class='active'");
      }
      client.print(">");
      client.print(i + 1);
      client.print(" - ");
      client.print(modeController.modeName(i));
      client.println("</button></a>");
    }

    client.println("</body></html>");
  }
};

#endif // WIFI_CONTROLLER_H
