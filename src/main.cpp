/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-video-streaming-web-server-camera-home-assistant/
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems
#include "esp_http_server.h"
#include "FS.h"     // SD Card ESP32
#include "SD_MMC.h" // SD Card ESP32
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h>
#include <EEPROM.h>            // read and write from flash memory


struct tm timeinfo;

typedef struct StatusDuet
{
    int layer;
    String status;
    String folder;
} StatusDuet;
 
StatusDuet statusDuet;

//TOTO externalize that on config.txt
const char* ntpServer = "0.br.pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

//Replace with your network credentials
String ssid = "Corona Virus";
String password = "orlando21";
String printer = "192.168.5.31";
unsigned long updateInterval = 1000;
// define the number of bytes you want to access
#define EEPROM_SIZE 3

#define PART_BOUNDARY "123456789000000000000987654321"

// This project was tested with the AI Thinker Model, M5STACK PSRAM Model and M5STACK WITHOUT PSRAM
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#else
#error "Camera model not selected"
#endif

//static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
//static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
//static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

unsigned long miliseconds = 0;


//httpd_handle_t stream_httpd = NULL;
void startCameraServer();


/*

static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
  {
    return res;
  }

  while (true)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->width > 400)
      {
        if (fb->format != PIXFORMAT_JPEG)
        {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted)
          {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        }
        else
        {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK)
    {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    }
    else if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK)
    {
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  //Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(stream_httpd, &index_uri);
  }
}
*/

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void configure()
{
  Serial.println("configure.");

  //Serial.println("Starting SD Card");
  if (!SD_MMC.begin())
  {
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD Card attached");
    return;
  }

  File file = SD_MMC.open("/config.txt");

  if (!file)
  {
    Serial.println("Opening file to write failed");

    String dados = "ssid=" + String(ssid) + "\n" + 
                   "password=" + password + "\n" + 
                   "printer=" + printer + "\n" + 
                   "updateInterval=" + updateInterval;

    file = SD_MMC.open("/config.txt", FILE_WRITE);

    if (!file)
    {
      Serial.println("Failed to open file for writing");
      return;
    }

    if (file.print(dados))
    {
      Serial.println("File written");
    }
    else
    {
      Serial.println("Write failed");
    }
    file.close();
    return;
  }
  else
  {
    Serial.println("Arquivo encontrado lendo os dados ");

    while (file.available())
    {
      String ssidString = file.readStringUntil('\n');
      ssid = getValue(ssidString, '=', 1);
      String passwordString = file.readStringUntil('\n');
      password = getValue(passwordString, '=', 1);
      printer = getValue(file.readStringUntil('\n'), '=', 1);
      String updateIntervalStr = getValue(file.readStringUntil('\n'), '=', 1);
      updateInterval  = strtol( updateIntervalStr.c_str(), NULL, 0 );

      Serial.println("ssidString:" + ssid);
      Serial.println("passwordString:" + password);
      Serial.println("printer:" + printer);
      Serial.println("updateInterval:" + updateInterval);
    }
  }

  // Wi-Fi connection
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  Serial.setDebugOutput(false);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  configure();
  // Start streaming web server
  startCameraServer();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}


DynamicJsonDocument getHttp(String url){

  HTTPClient http;

  // Send request
  http.useHTTP10(true);
  http.begin(url);
  http.setTimeout(2000);
  Serial.println("url: " + url);
  
  DynamicJsonDocument doc(2048);

  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    deserializeJson(doc, http.getStream());
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Disconnect
  http.end();
  return doc;
}

String getDate(){
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }

  char now[120];
  strftime(now, sizeof(now), "%Y-%m-%dT%H-%M", &timeinfo);
  return now;
}

//Create a dir in SD card
void createDir(const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(SD_MMC.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void takePicture(){
// initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  int pictureNumber = EEPROM.read(0) + 1;

  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Path where new picture will be saved in SD Card
  String path = statusDuet.folder +  "/picture" + String(pictureNumber) +".jpg";

  fs::FS &fs = SD_MMC; 
  Serial.printf("Picture file name: %s\n", path.c_str());
  
  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  } 
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb); 
}

void getHttpStatus(){
  ////rr_status?type=3
  ///rr_model?key=job

  String url = "http://" + printer ;
  DynamicJsonDocument doc(2048);

  url +=  "/rr_status?type=3";
  doc = getHttp(url);

  String state = doc["status"];
  Serial.println("retorno:"+ state);
  
  int currentLayer = doc["currentLayer"];
  Serial.println("retorno1:"+ currentLayer);

  // Verify if printer change status from idle to printing
  if(statusDuet.status == "I"   &&  state == "P"){
      String dateTime = getDate();
      Serial.println("create folder at:"  + dateTime);  
      statusDuet.folder = '/' + dateTime;
      createDir(statusDuet.folder.c_str());
      EEPROM.write(0, 0);
      EEPROM.commit();
  }else  if(statusDuet.status == "P"   &&  state == "I"){
    takePicture();
  }

  //TODO create a constant
  String validos = "HPIE";
  if (validos.indexOf(state) >= 0 ){
    Serial.println("state changed.");
    statusDuet.status = state;
  }

  if(state == "P"){
    if (currentLayer > statusDuet.layer){
      takePicture();
    }
  }
  statusDuet.layer = currentLayer;
}

void readDuet()
{
  Serial.println("executa duet");
  getHttpStatus();
  
}

void loop()
{
  delay(1);

  unsigned long currentMillis = millis();
  if (currentMillis - miliseconds >= updateInterval)
  {
    // save the last time you blinked the LED
    miliseconds = currentMillis;
    readDuet();
  }
}
