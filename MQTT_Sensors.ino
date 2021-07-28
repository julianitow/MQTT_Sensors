#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h> 
#include <PubSubClient.h>

#define SSID "ESGI"
#define PASSWORD "Reseau-GES"

#define MQTT_SERVER "group1.local"
#define MQTT_USER "esgi"
#define MQTT_PASSWORD "esgi"

#define TOPIC_STATUS "esgi/group1/status"
#define TEMP_TOPIC "esgi/group1/temp"
#define RELAY_TOPIC "esgi/group1/relay"
#define SET_RELAY_TOPIC "esgi/group1/setRelay"
#define RGB_TOPIC "esgi/group1/rgb"
#define SET_RGB_TOPIC "esgi/group1/setRGB"
#define TURN_RBG_ONOFF "esgi/group1/setOn"
#define SHOCK_TOPIC "esgi/group1/shock"

#define DEBUG true

int led = 2 ;// Déclaration de la broche de sortie LED
int shockPin = 4; // Déclaration de la broche d'entrée du capteur
int redpin = 13; //select the pin for the red LED
int bluepin = 12; // select the pin for the  blue LED
int greenpin = 14;// select the pin for the green LED
int relayPin = 16;
int tempPin = 5;
int valShock; // Variable temporaire
int valRGB; //variable RGB
long lastMsg = 0;
char message_buff[100];

// Configuration des libraries
OneWire oneWire(tempPin);          
DallasTemperature sensors(&oneWire);   

//MQTT needs
WiFiClient espClient;
PubSubClient client(espClient);
 
void setup ()
{
  setupPins();
  sensors.begin(); //init temp sensor
  Serial.begin(9600);
  setupWifi();
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
}

void setupPins(){
  pinMode(redpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(led, OUTPUT) ; // Initialisation de la broche de sortie
  pinMode(shockPin, INPUT); // Initialisation de la broche du capteur
  pinMode(tempPin, INPUT);
  digitalWrite(shockPin, HIGH); // Activation de la résistance de Pull-up interne
  pinMode (relayPin, OUTPUT);
}

void setupWifi(){
  Serial.println("WIFI Connection:");
  WiFi.begin(SSID, PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting...");
  }
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("OK, subscribing to topics...");
      client.subscribe(SET_RELAY_TOPIC);
      client.subscribe(SET_RGB_TOPIC);
      client.subscribe(TURN_RBG_ONOFF);
      publishRelay();
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println("Retrying in 5 seconds");
      delay(5000);
    }
  }
}

int publishTemp(long temp){
  return client.publish(TEMP_TOPIC, String(temp).c_str(), true);
}

int publishRelay(){
  if(DEBUG) {
    Serial.println(String(digitalRead(relayPin)).c_str());
  }
  return client.publish(RELAY_TOPIC, String(digitalRead(relayPin)).c_str(), true);
}

void shockLoop(){
  valShock = digitalRead(shockPin);
  if(valShock == HIGH){
    digitalWrite(led, HIGH);
    delay(2000);
  } else {
    digitalWrite(led, LOW);
  }
}

void RGBLoop(){
  //RGB LED
  for(valRGB = 255; valRGB > 0; valRGB--)
  {
    analogWrite(redpin, valRGB);  //set PWM value for red
    analogWrite(bluepin, 255 - valRGB); //set PWM value for blue
    analogWrite(greenpin, 128 - valRGB); //set PWM value for green
    //Serial.println(valRGB); //print current value 
    delay(1); 
  }
  for(valRGB = 0; valRGB < 255; valRGB++)
  {
    analogWrite(redpin, valRGB);
    analogWrite(bluepin, 255 - valRGB);
    analogWrite(greenpin, 128 - valRGB);
    //Serial.println(valRGB);
    delay(1); 
  }
}

void setRGB(String RGB[]){
  analogWrite(redpin, RGB[0].toInt());
  analogWrite(greenpin, RGB[1].toInt());
  analogWrite(bluepin, RGB[2].toInt());
}

void turnOnOffRGB(int state){
  if(state > 0){
    state = HIGH;
  } else {
    state = LOW;
  }
  digitalWrite(redpin, state);
  digitalWrite(greenpin, state);
  digitalWrite(bluepin, state);
}

long getTemp(){
  sensors.requestTemperatures();
  long temp = sensors.getTempCByIndex(0);
  if(DEBUG){
    Serial.print("Temperature: ");
    Serial.println(temp);
  }
  return temp;
}

void relayLoop(){
  if(digitalRead(relayPin) == HIGH) {
    digitalWrite (relayPin, LOW);
  } else {
    digitalWrite (relayPin, HIGH);
  }
  if(DEBUG){
    Serial.print("Relay state:");
    Serial.println(digitalRead(relayPin));
  }
   // "NO" est passant;
  delay (1000);
}

void setRelay(int state){
  Serial.print("State");
  Serial.println(state);
  if(state == 1){
    digitalWrite (relayPin, HIGH);
  } else {
    digitalWrite (relayPin, LOW);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  String topicString = String(topic);
  if(DEBUG){
    Serial.println("Message recu =>  topic: " + String(topic));
    Serial.println(" | longueur: " + String(length,DEC));
  }
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++){
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String msgString = String(message_buff);
  if(topicString == SET_RELAY_TOPIC){
    setRelay(msgString.toInt());
  } else if (topicString == RELAY_TOPIC){
    publishRelay();
  } else if (topicString == SET_RGB_TOPIC){;
    String RGB[3];
    char* splited = strtok(message_buff, ",");
    RGB[0] = splited;
    int i = 1;
    while(splited != NULL){
      splited = strtok(NULL, ",");
      RGB[i] = splited;
      if(i < 3){
        i++;
      }
    }
    setRGB(RGB);
  } else if (topicString == TURN_RBG_ONOFF){
    turnOnOffRGB(msgString.toInt());
  }
  if(DEBUG){
    Serial.println("Payload: " + msgString);
  }
}
  
void loop ()
{ 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //Shock sensor
  // shockLoop();
  
  //RGB
  // RGBLoop();

  //TEMP SENSOR
  long temp = getTemp();
  //Automatisation relay si température inférieure à 20 on allume le relay pour par exemple un chauffage
  if(temp < 20) {
    setRelay(1);
  } else {
    setRelay(0);
  }
  publishRelay();
  if(DEBUG && publishTemp(temp) > 0){
    Serial.println();
    Serial.print("Published: ");
    Serial.println(String(temp).c_str()); 
  }
  delay(1000);    // 5s de pause avant la mesure suivante
  //RELAY
  // relayLoop();
}
