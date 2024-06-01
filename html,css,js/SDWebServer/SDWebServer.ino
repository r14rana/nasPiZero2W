#include <FS.h> 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // display display width, in pixels
#define SCREEN_HEIGHT 64 // display display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define display_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, display_RESET);

//JioFi3_43B3C3
//nk77k5jyrr

//76697555
//TP-Link_F6EE

//Redmi Note 7 Pro
//123456789

const char* ssid = "Redmi Note 7 Pro";
const char* password = "123456789";
const char* host = "esp8266sd";

ESP8266WebServer server(80);

static bool hasSD = false;
File uploadFile;


void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

bool loadFromSdCard(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";

  File dataFile = SD.open(path.c_str());
  if(dataFile.isDirectory()){
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile)
    return false;

  if (server.hasArg("download")) dataType = "application/octet-stream";

  
const int MAX_RETRIES = 2;
int retries = 0;
size_t dataSent = 0;

while (retries < MAX_RETRIES) {
    dataSent = server.streamFile(dataFile, dataType);
    if (dataSent == dataFile.size()) {
        Serial.println("Data sent successfully.");
        break;
    } else {
        Serial.println("Sent less data than expected, retrying...");
        retries++;
    }
}

if (retries == MAX_RETRIES) {
    Serial.println("Failed to send the complete file after multiple attempts.");
}

  
  /*if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.println("Sent less data than expected!");
  }*/



  dataFile.close();
  return true;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    if(SD.exists((char *)upload.filename.c_str())) SD.remove((char *)upload.filename.c_str());
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    Serial.print("Upload: START, filename: "); Serial.println(upload.filename);
    // Display and log the file upload start information
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Upload: START");
    display.setCursor(0, 10);
    display.print("Filename: ");
    display.print(upload.filename);
    display.display();

  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    Serial.print("Upload: WRITE, Bytes: "); Serial.println(upload.currentSize);
    // Display and log the file upload write information
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Upload: WRITE");
    display.setCursor(0, 10);
    display.print("Bytes: ");
    display.print(upload.currentSize);
    display.display();
  } else if(upload.status == UPLOAD_FILE_END){
    if(uploadFile) uploadFile.close();
    Serial.print("Upload: END, Size: "); Serial.println(upload.totalSize);
    // Display and log the file upload end information
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Upload: END");
    display.setCursor(0, 10);
    display.print("Total Size: ");
    display.print(upload.totalSize/1024);
    display.print("KB");
    display.display();
    oled_dis();
  }
}

void deleteRecursive(String path){
  File file = SD.open((char *)path.c_str());
  if(!file.isDirectory()){
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while(true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" +entry.name();
    if(entry.isDirectory()){
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}

void handleDelete(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void handleCreate(){
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if(path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if(path.indexOf('.') > 0){
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if(file){
      file.write((const char *)0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void printDirectory() {
  if(!server.hasArg("dir")) return returnFail("BAD ARGS");
  String path = server.arg("dir");
  if(path != "/" && !SD.exists((char *)path.c_str())) return returnFail("BAD PATH");
  File dir = SD.open((char *)path.c_str());
  path = String();
  if(!dir.isDirectory()){
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();

  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
    break;

    String output;
    if (cnt > 0)
      output = ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    server.sendContent(output);
    entry.close();
 }
 server.sendContent("]");
 dir.close();
}

void handleNotFound(){
  //Serial.println(hasSD);
  if(hasSD && loadFromSdCard(server.uri())) return;
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.print(message);
}
void oled_dis(){
  delay(3000);
  display.clearDisplay();
  display.println("Connecting to... ");
  display.println(ssid);
  
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display(); 
}
void setup(void){
  Serial.begin(115200);

// initialize the display object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
    }

    // Clear the buffer.
  display.clearDisplay();

  // Display Text
  //display.setTextSize(1);
  display.setTextColor(WHITE);
  //display.setCursor(0,28);
  //delay(2000);
  //display.clearDisplay();


  Serial.setDebugOutput(true);
  Serial.print("\n");
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  display.println("Connecting to... ");
  display.println(ssid);
  
  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  if(i == 21){
    Serial.print("Could not connect to");
    Serial.println(ssid);
    while(1) delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();

/*  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println(".local");
  }*/


  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on("/edit", HTTP_POST, [](){ returnOK(); }, handleFileUpload);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  if (SD.begin(SS)){
     Serial.println("SD Card initialized.");
     hasSD = true;
  }
}

void loop(void){
  server.handleClient();
}
