#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>

#include <Hash.h>
#include <WebSocketsServer.h>
#include "FSWebServer.h"

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// WiFi Access Point
const char *ssid = "HP OfficeJet Direct";
const char *pass = "project2b";

// Web Sockets
WebSocketsServer socket = WebSocketsServer(81);

// LED Matrix Declaration
#define MATRIX_PIN  0
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(4, 8, MATRIX_PIN,
	NEO_MATRIX_TOP		+ NEO_MATRIX_RIGHT +
	NEO_MATRIX_COLUMNS	+ NEO_MATRIX_PROGRESSIVE,
	NEO_GRB		    	+ NEO_KHZ800);


// Handle Web Socket Events
void socketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
	IPAddress ip = socket.remoteIP(num);

	switch (type) {
		case WStype_DISCONNECTED:
			Serial.printf("WS: (%d.%d.%d.%d) closed.\n", ip[0], ip[1], ip[2], ip[3]);
			break;

		case WStype_CONNECTED:
			Serial.printf("WS: (%d.%d.%d.%d) opened.\n", ip[0], ip[1], ip[2], ip[3]);
			break;

		case WStype_TEXT:
			Serial.printf("WS: (%d.%d.%d.%d) '%s'.\n", ip[0], ip[1], ip[2], ip[3], payload);
			break;

		case WStype_BIN:
			Serial.printf("WS: (%d.%d.%d.%d) binary.\n", ip[0], ip[1], ip[2], ip[3]);
			break;
	}
}

void setup() {
    delay(1000);
    
    // Serial term
    Serial.begin(115200);
    Serial.printf("\nWiFi SSID: %s\n", ssid);
    Serial.printf("WiFi PASS: %s\n", pass);
    Serial.printf("Connecting");

    // Setup access point
    uint16_t timeout = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.printf(".");
    }
    Serial.printf(" Done.\n");
    Serial.printf("WiFi IP: ");
    Serial.println(WiFi.localIP());

    // Setup OTA updates
    ArduinoOTA.onStart([](){
    	matrix.fillScreen(matrix.Color(0, 0, 0));
    	matrix.drawRect(0, 0, 4, 8, matrix.Color(127, 127, 127));
    	matrix.show();
    });
    ArduinoOTA.onEnd([](){ 
    	matrix.drawRect(1, 1, 2, 6, matrix.Color(0, 255, 0));
    	matrix.show();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){
    	matrix.drawFastHLine(1, (progress / (total / 5)) + 1, 2, matrix.Color(0, 0, 255));
    	matrix.show();
    });
    ArduinoOTA.onError([](ota_error_t error){
	Serial.printf("Error[%u]");
    });
    ArduinoOTA.begin();

    // Setup Web Sockets
    socket.begin();
    socket.onEvent(socketEvent);

	// Setup mDNS
    MDNS.begin("ledmatrix");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);

    // Setup Web Server
    httpBegin();

    // Setup LED Matrix
    matrix.begin();
    matrix.setBrightness(40);
    matrix.setTextColor(matrix.Color(0, 255, 255));
}

int x = 3;
void loop() {
    ArduinoOTA.handle();
    httpHandle();
    socket.loop();

    matrix.fillScreen(0);
    matrix.drawChar(0, 0, 'C', matrix.Color(255, 255, 255), matrix.Color(0, 0, 0), 8);
    matrix.show();
    delay(100);
}

