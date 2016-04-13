#include "FSWebServer.h"

ESP8266WebServer http(80);
File fsUploadFile;

void httpHandle() {
	http.handleClient();
}

void httpBegin() {

	// Begin SPIFFS
	SPIFFS.begin();

	// List Directory
	http.on("/list", HTTP_GET, handleFileList);

  	// Create File
	http.on("/edit", HTTP_PUT, handleFileCreate);

  	// Delete File
	http.on("/edit", HTTP_DELETE, handleFileDelete);

  	// First callback is called after the request has ended with all parsed arguments
  	// Second callback handles file uploads at that location
	http.on("/edit", HTTP_POST, [](){
		Serial.printf("HTTP: POST %s\n", http.uri().c_str());

		http.send(200, "text/plain", ""); 
	}, handleFileUpload);

	// File Editor
	http.on("/edit", HTTP_GET, [](){
		Serial.printf("HTTP: GET %s\n", http.uri().c_str());

		if(!handleFileRead("/edit.htm")) http.send(404, "text/plain", "FILE NOT FOUND");
	});

  	// Called when the url is not defined here
  	// Use it to load content from SPIFFS
	http.onNotFound([](){
		Serial.printf("HTTP: GET %s\n", http.uri().c_str());

		if(!handleFileRead(http.uri()))
			http.send(404, "text/plain", "FILE NOT FOUND");
	});

	http.begin();
}

String formatBytes(size_t bytes) {
	if (bytes < 1024){
		return String(bytes)+"B";
	} else if(bytes < (1024 * 1024)){
		return String(bytes/1024.0)+"KB";
	} else if(bytes < (1024 * 1024 * 1024)){
		return String(bytes/1024.0/1024.0)+"MB";
	} else {
		return String(bytes/1024.0/1024.0/1024.0)+"GB";
	}
}

String getContentType(String filename) {
	if(http.hasArg("download")) return "application/octet-stream";
	else if(filename.endsWith(".htm")) return "text/html";
	else if(filename.endsWith(".html")) return "text/html";
	else if(filename.endsWith(".css")) return "text/css";
	else if(filename.endsWith(".js")) return "application/javascript";
	else if(filename.endsWith(".png")) return "image/png";
	else if(filename.endsWith(".gif")) return "image/gif";
	else if(filename.endsWith(".jpg")) return "image/jpeg";
	else if(filename.endsWith(".ico")) return "image/x-icon";
	else if(filename.endsWith(".xml")) return "text/xml";
	else if(filename.endsWith(".pdf")) return "application/x-pdf";
	else if(filename.endsWith(".zip")) return "application/x-zip";
	else if(filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

bool handleFileRead(String path) {
	if (path.endsWith("/")) path += "index.htm";

	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";

	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {

		if(SPIFFS.exists(pathWithGz))
			path += ".gz";
		File file = SPIFFS.open(path, "r");
		size_t sent = http.streamFile(file, contentType);
		file.close();
		return true;

	}

	return false;
}

void handleFileUpload() {
	if (http.uri() != "/edit") return;
	HTTPUpload& _upload = http.upload();

	if (_upload.status == UPLOAD_FILE_START) {

		String filename = _upload.filename;
		if (!filename.startsWith("/")) filename = "/" + filename;
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();

	} else if (_upload.status == UPLOAD_FILE_WRITE) {

		if (fsUploadFile)
			fsUploadFile.write(_upload.buf, _upload.currentSize);

	} else if (_upload.status == UPLOAD_FILE_END) {
		
		if (fsUploadFile)
			fsUploadFile.close();

	}
}

void handleFileDelete() {
	if (http.args() == 0)
		return http.send(500, "text/plain", "BAD ARGS");

	String path = http.arg(0);

	if (path == "/")
		return http.send(500, "text/plain", "BAD PATH");

	if (!SPIFFS.exists(path))
		return http.send(404, "text/plain", "FILE NOT FOUND");

	SPIFFS.remove(path);
	http.send(200, "text/plain", "");
	path = String();
}

void handleFileCreate() {
	if(http.args() == 0)
		return http.send(500, "text/plain", "BAD ARGS");

	String path = http.arg(0);

	if(path == "/")
		return http.send(500, "text/plain", "BAD PATH");

	if(SPIFFS.exists(path))
		return http.send(500, "text/plain", "FILE EXISTS");

	File file = SPIFFS.open(path, "w");
	if(file)
		file.close();
	else
		return http.send(500, "text/plain", "CREATE FAILED");

	http.send(200, "text/plain", "");
	path = String();
}

void handleFileList() {
	if(!http.hasArg("dir")) return http.send(500, "text/plain", "BAD ARGS");

	String path = http.arg("dir");
	Dir dir = SPIFFS.openDir(path);
	path = String();

	String output = "[";
	while(dir.next()){
		File entry = dir.openFile("r");
		if (output != "[") output += ',';
		bool isDir = false;
		output += "{\"type\":\"";
		output += (isDir)?"dir":"file";
		output += "\",\"name\":\"";
		output += String(entry.name()).substring(1);
		output += "\"}";
		entry.close();
	}

	output += "]";
	http.send(200, "text/json", output);
}