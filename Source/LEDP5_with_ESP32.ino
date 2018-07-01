#include "EEPROM.h"
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Adafruit_GFX.h"
#include "ESP32RGBmatrixPanel.h"
#include <HTTPClient.h>

#include <WiFi.h>

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>



String serverCMI = "192.168.1.56";

WebServer server(80);

/*Khai bao Wifi*/
const char* ssid = "CMA";
const char* pass = "cma2018LHP515";



//G1  R1 |
//GND B1 |
//G2  R2 |
//GND B2 |
//B   A  |
//D   C  |
//LAT CLK|
//GND OE |

//Default connection
//uint8 OE = 23;
//uint8 CLK = 22;
//uint8 LAT = 03;
//uint8 CH_A = 21;
//uint8 CH_B = 19;
//uint8 CH_C = 18;
//uint8 CH_D = 5;
//uint8 R1 = 17;
//uint8 G1 = 16;
//uint8 BL1 = 4;
//uint8 R2 = 0;
//uint8 G2 = 2;
//uint8 BL2 = 15;

ESP32RGBmatrixPanel matrix(23, 22, 03, 17, 16, 04, 13, 02, 15, 21, 19, 18, 5); //Flexible connection

struct dataName {
  String maSoThe;
  String kg;
  bool redText;
  dataName() {};
  dataName(String ms, String kl, bool is_red) : maSoThe(ms), kg(kl), redText(is_red) {};
};

dataName listName[4] = {dataName("012", "4.23", false), dataName("124", "4.11", false), dataName("192", "4.03", false), dataName("023", "3.83", true)};

/* create a hardware timer */
hw_timer_t* displayUpdateTimer = NULL;
void IRAM_ATTR onDisplayUpdate() {
  matrix.update();
}

void handleRoot() {
  server.send(200, "text/plain", "hello from esp32! host=" +  serverCMI);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void printName(dataName data, int pos) {
  if (data.redText)
    matrix.setTextColor(matrix.AdafruitColor(255, 0, 0));
  else matrix.setTextColor(matrix.AdafruitColor(0, 255, 0));

  matrix.setCursor(2, pos * 8);
  matrix.print(data.maSoThe);
  matrix.print(": ");
  matrix.print(data.kg);
  vTaskDelay(10);
}


void splash_screen(int c) {
  Serial.println("Slash");
  matrix.black();
  matrix.setTextColor(matrix.AdafruitColor(0, 255, 0));
  matrix.setCursor(20, 2);
  matrix.print("WIFI");
  matrix.setCursor(2, 14);
  matrix.print("CONNECTING");
  matrix.setCursor(1, 20);
  char s[20];
  for (int i = 0; i < c; i++) {
    s[i] = '.';
  }
  s[c] = 0;
  matrix.print(s);
}

volatile bool is_update = false;
volatile bool is_wifi_connecting = false;
volatile int count_c = 0;

//runs faster then default void loop(). why? runs on other core?
void loop2_task(void *pvParameter)
{
  while (true) {
    if (is_wifi_connecting) {

      splash_screen(count_c);
      vTaskDelay(500);
      count_c += 1;
      if (count_c > 8) count_c = 1;
    } else {
      vTaskDelay(1000);
      if (is_update) {
        is_update = false;

      }
    }
  }
}

long prev_t;
long prev_timeStamp;

void show_ip() {
  matrix.black();
  matrix.setTextColor(matrix.AdafruitColor(255, 0, 0));

  matrix.setCursor(1, 1);
  matrix.print(WiFi.localIP());

  vTaskDelay(3000);

}

void update_eeprom() {
  EEPROM.writeByte(0,0xED);
  EEPROM.writeByte(1,serverCMI.length());
  for (int i=0; i<serverCMI.length(); i++) {
    EEPROM.writeChar(2+i,serverCMI[i]);
  }
  EEPROM.writeByte(2+serverCMI.length(),0);
  EEPROM.commit();
  
}


String read_from_eeprom() {
  int l = EEPROM.readByte(1);
  char s[17];
  if (l>16) return "";
  
  for (int i=0; i<l; i++) {
    s[i] = EEPROM.readChar(2+i);
  }
  s[l] = 0;
  return s;
}

void setup() {
  Serial.begin(115200);

  if (!EEPROM.begin(255)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (EEPROM.readByte(0) == 0xED) {
    serverCMI = read_from_eeprom();
  } else {
    
    update_eeprom();
  }
  

  //matrix.setBrightness(255);
  is_update = true;
  is_wifi_connecting = true;
  xTaskCreate(&loop2_task, "loop2_task", 2048, NULL, 5, NULL);
  /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  displayUpdateTimer = timerBegin(0, 120, true);

  /* Attach onTimer function to our timer */
  timerAttachInterrupt(displayUpdateTimer, &onDisplayUpdate, true);
  timerAlarmWrite(displayUpdateTimer, 2, true);
  timerAlarmEnable(displayUpdateTimer);


  prev_timeStamp = -1;
  /*Ket noi WiFi*/
  //matrix.black();
  //splash_screen();
  delay(5000);
  WiFiManager wifiManager;
  wifiManager.autoConnect("Bang LED 01");
  is_wifi_connecting = false;

  show_ip();

  Serial.println("OK");
  prev_t = millis();

  server.on("/", handleRoot);

  server.on("/set_host", []() {
    if (server.argName(0) == "ip") {
      serverCMI = server.arg(0);
      update_eeprom();
    }
    server.send(200, "text/plain", "update server ip = " + serverCMI);
  });

  server.onNotFound(handleNotFound);

  server.begin();

}


void loop() {
  if ((WiFi.status() == WL_CONNECTED) && (millis() - prev_t > 1000)) {
    HTTPClient http;
    http.begin("http://" + serverCMI + "/get_last_info.php");
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      //Serial.println(httpCode);
      Serial.println(payload);
      //Serial.println(payload);
      StaticJsonBuffer<1000> JSONBuffer;
      JsonObject& parsed = JSONBuffer.parseObject(payload);

      //const char* MST = parsed["MST"][0];
      //const char* KL  = parsed["KL"][0];
      //const bool isRed = parsed["isRed"][0];
      const char* ts = parsed["ts"];
      //Serial.println(ts);
      long timeStamp = atol(ts);
      Serial.println(timeStamp);
      if (prev_timeStamp < timeStamp) {

        prev_timeStamp = timeStamp;
        listName[0] = dataName(parsed["MST"][0], parsed["KL"][0], parsed["isRed"][0]);
        listName[1] = dataName(parsed["MST"][1], parsed["KL"][1], parsed["isRed"][1]);
        listName[2] = dataName(parsed["MST"][2], parsed["KL"][2], parsed["isRed"][2]);
        listName[3] = dataName(parsed["MST"][3], parsed["KL"][3], parsed["isRed"][3]);

        matrix.black();

        for (int i = 0; i < 4; i++) {
          printName(listName[i], i);
        }

      }
    }
    else Serial.println("Error on HTTP request");
    http.end();
    is_update = true;
    prev_t = millis();

  }
  server.handleClient();
}
