#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>

#include <Hash.h>
#include <WebSocketsServer.h>
#include "FSWebServer.h"

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// WiFi Access Point
const char *ssid = "LED Matrix";
const char *pass = "";

// Web Sockets
WebSocketsServer socket = WebSocketsServer(81);

// LED Matrix Declaration
#define MATRIX_WIDTH	4	
#define MATRIX_HEIGHT	8
#define MATRIX_PIN  	0
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_PIN,
	NEO_MATRIX_TOP		+ NEO_MATRIX_RIGHT +
	NEO_MATRIX_COLUMNS	+ NEO_MATRIX_PROGRESSIVE,
	NEO_GRB		    	+ NEO_KHZ800);

// Process command
void processCommand(char * payload) {
	int x, y, r, g, b;

	// Get command
	char * cmd = strtok(payload, " ");

	// PIXEL
	if (strcmp(cmd, "PIXEL") == 0) {
		x = atoi(strtok(NULL, " "));
		y = atoi(strtok(NULL, " "));
		r = atoi(strtok(NULL, " "));
		g = atoi(strtok(NULL, " "));
		b = atoi(strtok(NULL, " "));

		// Validate position
		if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT) {
			Serial.printf("CMD: (PIXEL) Invalid Position\n");
			return;
		}

		// Validate color
		if (r > 255 || g > 255 || b > 255) {
			Serial.printf("CMD: (PIXEL) Invalid Color\n");
			return;
		}

		// Execute
		matrix.drawPixel(x, y, matrix.Color(r, g, b));
		return;
	}

	// FILL
	if (strcmp(cmd, "FILL") == 0) {
		r = atoi(strtok(NULL, " "));
		g = atoi(strtok(NULL, " "));
		b = atoi(strtok(NULL, " "));

		// Validate color
		if (r > 255 || g > 255 || b > 255) {
			Serial.printf("CMD: (PIXEL) Invalid Color\n");
			return;
		}

		// Execute
		matrix.fillScreen(matrix.Color(r, g, b));
		return;
	}

	// SHOW
	if (strcmp(cmd, "SHOW") == 0) {
		// Execute
		matrix.show();
		return;
	}

	// CLEAR
	if (strcmp(cmd, "CLEAR") == 0) {
		// Execute
		matrix.fillScreen(matrix.Color(0, 0, 0));
		matrix.show();
		return;
	}

	// BRIGHT
	if (strcmp(cmd, "BRIGHT") == 0) {
		b = atoi(strtok(NULL, " "));

		// Validate value
		if (b > 255) {
			Serial.printf("CMD: (BRIGHT) Out Of Range\n");
			return;
		}

		// Execute
		matrix.setBrightness(b);
		return;
	}

	// NOT IMPLEMENTED
	Serial.printf("CMD: (%s) Not Implemented\n", cmd);
}

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
			
			processCommand((char *)payload);

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
    //Serial.printf("Access Point Started.");

    // Setup access point
    uint16_t timeout = millis();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);

    /*WiFi.begin(ssid, pass);
    while (millis() - timeout < 5000 && WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.printf(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
	    Serial.printf(" Done.\n");
	    Serial.printf("WiFi IP: ");
	    Serial.println(WiFi.localIP());
	} else {
		Serial.printf(" Unavailable.\n");
	}*/

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
    matrix.setBrightness(100);
    matrix.fillScreen(matrix.Color(0, 0, 0));
    matrix.show();
}

int x = 3;
void loop() {
    ArduinoOTA.handle();
    httpHandle();
    socket.loop();

    //delay(100);
}

