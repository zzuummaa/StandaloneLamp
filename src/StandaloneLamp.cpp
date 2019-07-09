/*
  FSWebServer - Example WebServer with SPIFFS backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `\ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done

  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include "sta_settings.h"
#include <ArduinoJson.h>

FS *filesystem = &SPIFFS;
//FS* filesystem = &LittleFS;

#define DBG_OUTPUT_PORT Serial

const char *ssid = STASSID;
const char *password = STAPSK;
const char *host = "esp8266fs";

ESP8266WebServer server(80);

//format bytes
String formatBytes(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + "B";
    } else if (bytes < (1024 * 1024)) {
        return String(bytes / 1024.0) + "KB";
    } else if (bytes < (1024 * 1024 * 1024)) {
        return String(bytes / 1024.0 / 1024.0) + "MB";
    } else {
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
    }
}

String getContentType(String filename) {
    if (server.hasArg("download")) {
        return "application/octet-stream";
    } else if (filename.endsWith(".htm")) {
        return "text/html";
    } else if (filename.endsWith(".html")) {
        return "text/html";
    } else if (filename.endsWith(".ssi")) {
        return "text/html";
    } else if (filename.endsWith(".css")) {
        return "text/css";
    } else if (filename.endsWith(".js")) {
        return "application/javascript";
    } else if (filename.endsWith(".png")) {
        return "image/png";
    } else if (filename.endsWith(".gif")) {
        return "image/gif";
    } else if (filename.endsWith(".jpg")) {
        return "image/jpeg";
    } else if (filename.endsWith(".ico")) {
        return "image/x-icon";
    } else if (filename.endsWith(".xml")) {
        return "text/xml";
    } else if (filename.endsWith(".pdf")) {
        return "application/x-pdf";
    } else if (filename.endsWith(".zip")) {
        return "application/x-zip";
    } else if (filename.endsWith(".gz")) {
        return "application/x-gzip";
    }
    return "text/plain";
}

bool handleFileRead(String path) {
    DBG_OUTPUT_PORT.println("handleFileRead: " + path);
    if (path.endsWith("/")) {
        path += "index.html";
    }
    String contentType;
    if (path.lastIndexOf(".") == -1) {
        contentType = ".html";
        path += contentType;
    } else {
        contentType = getContentType(path);
    }
    String pathWithGz = path + ".gz";
    if (filesystem->exists(pathWithGz) || filesystem->exists(path)) {
        if (filesystem->exists(pathWithGz)) {
            path += ".gz";
        }
        File file = filesystem->open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void setup(void) {
    DBG_OUTPUT_PORT.begin(115200);
    DBG_OUTPUT_PORT.print("\n");
    DBG_OUTPUT_PORT.setDebugOutput(true);
    filesystem->begin();
    {
        Dir dir = filesystem->openDir("/");
        while (dir.next()) {
            String fileName = dir.fileName();
            size_t fileSize = dir.fileSize();
            DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
        }
        DBG_OUTPUT_PORT.printf("\n");
    }


    //WIFI INIT
    DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
    if (String(WiFi.SSID()) != String(ssid)) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DBG_OUTPUT_PORT.print(".");
    }

    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("Connected! IP address: ");
    DBG_OUTPUT_PORT.println(WiFi.localIP());

    if (MDNS.begin(host)) {
        DBG_OUTPUT_PORT.print("Open http://");
        DBG_OUTPUT_PORT.print(host);
        DBG_OUTPUT_PORT.println(".local/ to see the file browser");
    } else {
        DBG_OUTPUT_PORT.println("MDNS start faild");
    }
    

    //SERVER INIT
    server.on("/api/status", []() {
        server.send(200, "application/json", "");
    });

    server.on("/api/wifi", HTTP_GET, []() {
        server.send(200, "application/json", "{\"SSID\":\"" + WiFi.SSID() + "\",\"PSK\":\"" + WiFi.psk() + "\"}");
    });

    server.on("/api/wifi", HTTP_POST, []() {
        const String& body = server.arg("plain");
        if (body.length() > 512) {
            server.send(400, "application/json", "{\"error\":\"request body length > 512 bytes\"}");
            return;
        }

        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            DBG_OUTPUT_PORT.print(F("deserializeJson() failed: "));
            DBG_OUTPUT_PORT.println(error.c_str());
            server.send(400, "application/json", "{\"error\":\"invalid JSON format\"}");
            return;
        }

        const char *ssid = doc["SSID"];
        const char *password = doc["PSK"];

        DBG_OUTPUT_PORT.print("Parsed SSID=\"");
        DBG_OUTPUT_PORT.print(ssid);
        DBG_OUTPUT_PORT.print("\", PSK=\"");
        DBG_OUTPUT_PORT.print(password);
        DBG_OUTPUT_PORT.println('\"');

        WiFi.begin(ssid, password);
        server.send(200, "application/json", "");
    });

    //called when the url is not defined here
    //use it to load content from SPIFFS
    server.onNotFound([]() {
        if (!handleFileRead(server.uri())) {
            server.send(404, "text/plain", "FileNotFound");
        }
    });

    server.begin();
    DBG_OUTPUT_PORT.println("HTTP server started");

}

void loop(void) {
    server.handleClient();
    MDNS.update();
}
