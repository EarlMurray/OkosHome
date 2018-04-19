#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const char* ssid = "";
const char* password = "kkUPWE3RPFrA";
const char* mqtt_server = "94.174.54.253";
char topic[] = "hall/pir";
const char* user = "l";
const char* userpass = "";
char subA[] = "livingroom/heating";
char subB[] = "livingroom/filter";
char subC[] = "livingroom/lights";
char subD[] = "hall/pir";
char subE[] = "proxMode";

const int MOTION_PIN = 2; //PIR SENSOR DATA PIN - (NodeMCU D4)
const int LED_PIN = 13; // LED pin - (NodeMCU D7)
const int HALL_LIGHT = 4;// HALL LIGHT pin (NodeMCU D2)
const int MAIN_LIGHT = 0; //LIVINGROOM LIGHT pin (NodeMCU D3)
const int HEATER = 5; //HEATER pin (NodeMCU D1)
const int FILTER = 10; //AIR PURIFIER  pin (NodeMCU SD3)

WiFiClient espClient;
PubSubClient client(espClient);

int timeout;//TIMEOUT COUNTER FOR HALL_LIGHT
bool isProx;//PROXIMITY MODE CHECKER FOR HALL_LIGHT

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

  /******************************** LISTEN FOR INCOMING TOPICS ******************************************/
  /********************************** THEN TOGGLE ON OR OFF *********************************************/
  if(strcmp(topic,"livingroom/heating")==0)//HEATER
  {  
    if((char)payload[0] == 'N')
    {
         if(digitalRead(HEATER) == LOW)//IF OFF THEN TURN ON
        {
          Serial.println("HEATING ON");
          digitalWrite(HEATER, HIGH);
        }
    }
    if((char)payload[0] == 'F')
    {
      if(digitalRead(HEATER) == HIGH)//IF ON THEN TURN OFF
      {
        Serial.println("HEATING OFF");
        digitalWrite(HEATER, LOW);
      }
    }
  }
    
  if(strcmp(topic,"livingroom/filter")==0)//AIR PURIFIER
  {
    if((char)payload[0] == 'N')
    {
      if(digitalRead(FILTER) == LOW)//IF OFF THEN TURN ON
      {
        Serial.println("AIR PURIFIER ON");
        digitalWrite(FILTER, HIGH);
      }
    }
    if((char)payload[0] == 'F')
    {
      if(digitalRead(FILTER) == HIGH)//IF ON THEN TURN OFF
      {
        Serial.println("AIR PURIFIER OFF");
        digitalWrite(FILTER, LOW);
      }
    }
    
  }
  if(strcmp(topic,"livingroom/lights")==0)//LIVINGROOM LIGHT
  { 
    if((char)payload[0] == 'N')
    {
      if(digitalRead(MAIN_LIGHT) == LOW)//IF OFF THEN TURN ON
      {
        Serial.println("LIVING ROOM LIGHT ON");
        digitalWrite(MAIN_LIGHT, HIGH);
      }
    }
    if((char)payload[0] == 'F')
    {
      if(digitalRead(MAIN_LIGHT) == HIGH)//IF ON THEN TURN OFF
      {
        digitalWrite(MAIN_LIGHT, LOW);
      }
    }   
  }
  if(strcmp(topic,"hall/pir")==0)//HALL LIGHT
  {
    if((char)payload[0] == 'N')
    {
      if(digitalRead(HALL_LIGHT) == LOW)//IF OFF THEN TURN ON
      {
        Serial.println("HALL LIGHT ON");
        digitalWrite(HALL_LIGHT, HIGH);
      }
    }
    if((char)payload[0] == 'F')
    {
      if((digitalRead(HALL_LIGHT) == HIGH) && (!isProx))//IF NOT IN PROXIMITY MODE AND IS ON THEN TURN OFF
      {
        Serial.println("HALL LIGHT OFF");
        digitalWrite(HALL_LIGHT, LOW);
      }
    } 
  }
  if(strcmp(topic, "proxMode") ==0)//SWITCH HALL LIGHT BETWEEN PROXIMITY MODE AND TIMED MODE
  {
    if((char)payload[0] == 'F')
    {
      isProx = false;
    }
    if((char)payload[0] == 'N')//TURN ON PROXIMITY MODE
    {
      isProx = true;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), user, userpass)) 
    {
      Serial.println("connected");

      //SUBSCRIBE TO TOPICS
      client.subscribe(subA);
      client.subscribe(subB);
      client.subscribe(subC);
      client.subscribe(subD);
      client.subscribe(subE);
     } 
     else 
     {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
     }
   }
}

void setup() 
{  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //SET PINS
  pinMode(MOTION_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MAIN_LIGHT, OUTPUT);
  pinMode(HALL_LIGHT, OUTPUT);
  pinMode(HEATER, OUTPUT);
  pinMode(FILTER, OUTPUT);

  //LIGHT CHECK -- TURN ON ALL LIGHTS FOR 1 SECOND, THEN TURN OFF
  digitalWrite(MAIN_LIGHT, HIGH);
  digitalWrite(HALL_LIGHT, HIGH);
  digitalWrite(HEATER, HIGH);
  digitalWrite(FILTER, HIGH);

  delay(1000);
  
  digitalWrite(MAIN_LIGHT, LOW);
  digitalWrite(HALL_LIGHT, LOW);
  digitalWrite(HEATER, LOW);
  digitalWrite(FILTER, LOW);

  //INITIALIZE VARIABLES
  timeout = 1;
  isProx = false;
}//end setup

void loop() 
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  /*************************PIR SENSOR********************************/
  int proximity = digitalRead(MOTION_PIN);
  if (proximity == HIGH) 
  {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Motion detected!");
    if(isProx)
    {
      client.publish(topic, "Motion detected");
    }
  } 
  else
  {
    digitalWrite(LED_PIN, LOW);
  }
  
  /*************************HALL LIGHT*********************************/
  if(isProx)//HALL LIGHT IS IN PROXIMITY MODE
  {
    if(timeout % 9 == 0)// HALL LIGHT HAS BEEN ON FOR 16 SECONDS
    {
      digitalWrite(HALL_LIGHT, LOW);
      Serial.println("HALL LIGHT OFF.");
      timeout++;
    }
    else if(digitalRead(HALL_LIGHT) == HIGH)// HALL LIGHT HAS BEEN ON FOR LESS THAN 16 SECONDS
    {
      timeout++;
    }
  }
  delay(2000);
}//end loop

