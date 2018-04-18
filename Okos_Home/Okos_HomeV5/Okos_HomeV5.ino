#include <PubSubClient.h>
#include "WiFiEsp.h"
#include <SoftwareSerial.h>
#include <SimpleDHT.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
 
SoftwareSerial espSerial(10, 11); // RX, TX
long int baudRate = 9600;
/**************************WiFi/Server Details*********************************/ 
char ssid[] = "Murray_Secure";
char pass[] = "kkUPWE3RPFrA";
int status = WL_IDLE_STATUS; 
char server[] = "94.174.54.253";
char topicA[] = "livingroom/okoshome/temp";
char topicB[] = "livingroom/okoshome/rh";
char topicC[] = "livingroom/okoshome/dust";
char clientId[] = "testclient101";
const char* user = "earl";
const char* userpass = "";
 
WiFiEspClient wifi;
PubSubClient mqttClient(wifi);

 /***************************DHT 11 SENSOR*************************************/
int DHTPin = 3; // temperature + humidity sensor DHT11 - Arduino D3Pin
SimpleDHT11 dht11;
int dustPin = 6; // dust sensor - Arduino A6pin
int ledPin = 2; //dust sensor led - Arduino D2pin    

/***************************SHARP DUST SENSOR*********************************/
float voltsMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

/***************************OLED DISPLAY*************************************/
#define I2C_ADDRESS 0x3C
#define RST_PIN -1
SSD1306AsciiAvrI2c oled;

/***************************SETUP FUNCTION**************************************/
void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);//Setup DHT11 Measure Pin

  /*************************SETUP SSD1306 OLED DISPLAY*************************/
  #if RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  oled.setFont(Adafruit5x7);

  /**********************SETUP WiFi/MQTT Connection****************************/
  set_esp8266_baud_rate(baudRate);
  espSerial.begin(baudRate);
  espSerial.print("AT+UART_CUR=");
  espSerial.print(baudRate);
  espSerial.print(",8,1,0,0\r\n");
  WiFi.init(&espSerial);
  
  while ( status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
  }
  mqttClient.setServer(server, 1883);
}
 
void loop() {
  /**************************Calculate Dust Density********************************/
  digitalWrite(ledPin,LOW); // power on the LED
  delayMicroseconds(280);
  voltsMeasured = analogRead(dustPin); // read the dust value
  delayMicroseconds(40);
  digitalWrite(ledPin,HIGH); // turn the LED off
  delayMicroseconds(9680);
  calcVoltage = voltsMeasured * (5.0 / 1024.0);
  dustDensity = 0.17 * calcVoltage - 0.1;

  /**************************Calculate Temp/Humidity*******************************/
  byte temp = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;

  char result[9];
  char full[100];
  String prefix = "TEMP:";
  
  if((err = dht11.read(DHTPin, &temp, &humidity, NULL)) != SimpleDHTErrSuccess)
  {
    mqttClient.publish(topicA, "temp failed"); 
    mqttClient.publish(topicB, "rh failed");
  }
  else
  {
    prefix.toCharArray(full, 100);
    dtostrf(temp, 5, 0, result);
    strcat(full, result);
    mqttClient.publish(topicA, full);
    prefix = "RH:";
    prefix.toCharArray(full, 100);
    dtostrf(humidity, 5, 0, result);
    strcat(full, result);
    mqttClient.publish(topicA, full);
  }

  /*************************Publish to MQTT Broker*********************************/
  prefix = "DUST:";
  prefix.toCharArray(full, 100);
  dtostrf(dustDensity, 8, 2, result);
  strcat(full, result);
  mqttClient.publish(topicA, full);
   
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  /**************************Display on OLED**************************************/
  oled.clear();
  oled.set1X();
  oled.print("Temp : ");
  oled.print(temp);
  oled.print(" C\nHumidity : ");
  oled.print(humidity);
  oled.print(" %\nDust : ");
  oled.print(dustDensity);
  oled.print(" mg/m3\nCO : ");

  //delay 3 seconds before next loop
  delay(3000);
}
 
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(clientId, user, userpass)) {
      Serial.println("connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
 
void set_esp8266_baud_rate(long int baud_rate){
  long int baud_rate_array[] = {1200,2400,4800,9600,19200,38400,57600,74880,115200,230400};
  int i, j, pause=10;
  String response;
 
  Serial.println("Setting ESP8266 baud rate...");
  for (j=0; j<5; j++){
    for (int i=0; i<10; i++){
      espSerial.begin(baud_rate_array[i]);
      espSerial.print("AT\r\n");
      delay(pause);
      if (espSerial.available()) {
        response=espSerial.readString();
        response.trim();
        if (response.endsWith("OK")) {
          espSerial.print("AT+UART_CUR=");
          espSerial.print(baud_rate);
          espSerial.println(",8,1,0,0");
          delay(pause);
          if (espSerial.available()) {
            response=espSerial.readString();
          }
          espSerial.begin(baudRate);
          delay(pause);
          espSerial.println("AT");
          delay(pause);
          if (espSerial.available()) {
            response=espSerial.readString();
            response.trim();
            if (response.endsWith("OK"))
              break;
            else {
              Serial.println("Trying again...");
            }
          }
          else {
            Serial.println("Trying again...");
          }
        }
      }
    }
    if (response.endsWith("OK"))
      break;
  }
  espSerial.begin(baudRate);
  delay(pause);
  espSerial.println("AT");
  delay(pause);
  if (espSerial.available()) {
    response=espSerial.readString();
    response.trim();
    if (response.endsWith("OK")) {
      Serial.print("Baud rate is now ");
      Serial.println(baudRate);
    }
    else {
      Serial.println("Sorry - could not set baud rate");
    }
  }
}
