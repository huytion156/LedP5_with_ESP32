#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Adafruit_GFX.h"
#include "ESP32RGBmatrixPanel.h"
#include <WiFi.h>
#include <HTTPClient.h>

/*Khai bao Wifi*/
const char* ssid = "CMA";
const char* pass = "cma2018LHP515";

/*So do chan cam LED P5*/
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

/*Danh sach 4 nhan vien tren LED*/
struct dataName {
  String maSoThe;
  String kg;
  bool redText; 
  dataName() {};
  dataName(String ms, String kl, bool is_red) : maSoThe(ms), kg(kl), redText(is_red) {};  
};
dataName listName[4];

/* create a hardware timer */
hw_timer_t* displayUpdateTimer = NULL;
void IRAM_ATTR onDisplayUpdate() {
  matrix.update();
}

void printName(dataName data, int pos) {
  matrix.setCursor(2, pos * 8);
  
  if (data.redText)
    matrix.setTextColor(matrix.AdafruitColor(255, 0, 0));
  else matrix.setTextColor(matrix.AdafruitColor(0, 255, 0));
  
  matrix.print(data.maSoThe);
  matrix.print(" : ");
  matrix.print(data.kg);
  vTaskDelay(10);
}

volatile bool is_update = false;

//runs faster then default void loop(). why? runs on other core?
void loop2_task(void *pvParameter)
{
  while (true) {
    vTaskDelay(1000);
    if (is_update) {
      is_update = false;
    }
  }
}

long prev_t;
void setup() {
  Serial.begin(115200);

  /*Ket noi WiFi*/
  delay(4000);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");
  
  matrix.setBrightness(1); //Dieu chinh do sang cua LED 
  is_update = true;

  xTaskCreate(&loop2_task, "loop2_task", 2048, NULL, 5, NULL);
  /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  displayUpdateTimer = timerBegin(0, 120, true);

  /* Attach onTimer function to our timer */
  timerAttachInterrupt(displayUpdateTimer, &onDisplayUpdate, true);
  timerAlarmWrite(displayUpdateTimer, 2, true);
  timerAlarmEnable(displayUpdateTimer);
  
  prev_t = millis();
  
}


void loop() {
  //Kiem tra ket noi internet 
  if ((WiFi.status() == WL_CONNECTED) && (millis() - prev_t > 1000)) {
    HTTPClient http;
    http.begin("http://192.168.1.55:3000/person"); //Server tra JSON 
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload= http.getString();
      //Serial.println(httpCode);
      //Serial.println(payload);

      /*Nhan chuoi JSON va Parse*/
      StaticJsonBuffer<500> JSONBuffer;
      JsonObject& parsed = JSONBuffer.parseObject(payload);

      const char* MST = parsed["MST"][0];
      const char* KL  = parsed["KL"][0];
      const bool isRed= parsed["isRed"][0];

      listName[0] = dataName(parsed["MST"][0], parsed["KL"][0], parsed["isRed"][0]);
      listName[1] = dataName(parsed["MST"][1], parsed["KL"][1], parsed["isRed"][1]);
      listName[2] = dataName(parsed["MST"][2], parsed["KL"][2], parsed["isRed"][2]);
      listName[3] = dataName(parsed["MST"][3], parsed["KL"][3], parsed["isRed"][3]);

      /*In danh sach ra bang LED*/
      for(int i = 0; i < 4; i++) printName(listName[i], i);      
    }
    else Serial.println("Error on HTTP request"); //Request that bai 
    http.end();
    is_update = true;
    prev_t = millis();
  }
}
