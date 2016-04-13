#ifndef FSWEBSERVER_H
#define FSWEBSERVER_H

#include <FS.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

String formatBytes(size_t bytes);
String getContentType(String filename);

bool handleFileRead(String path);

void handleFileUpload();
void handleFileDelete();
void handleFileCreate();
void handleFileList();

void httpBegin();
void httpHandle();

#endif // FSWEBSERVER_H