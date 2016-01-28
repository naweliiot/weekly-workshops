//Library for the temp-humidity sensor
#include "Adafruit_DHT/Adafruit_DHT.h"

//Library to handle JSONs
#include "SparkJson/SparkJson.h"

//Library for the MQTT protocol
#include "MQTT/MQTT.h"

//Where's the sensor connected?
#define DHTPIN 2

//What type of temp/hum sensor are we using?
#define DHTTYPE DHT22   // DHT 22 (AM2302)

//Credentials from the developer dashboard
#define DEVICE_ID "your_device_id" 
#define MQTT_USER "your_mqtt_user" 
#define MQTT_PASSWORD "your_mqtt_password"
#define MQTT_CLIENTID "your_client_id"
#define MQTT_TOPIC "your_mqtt_topic"
#define MQTT_SERVER "mqtt.relayr.io"

//Some definitions, including the publishing period
const int led = D7;
int ledState = LOW;
unsigned long lastPublishTime = 0;
unsigned long lastBlinkTime = 0;
int publishingPeriod = 400;     //It's longer since the temperature sensor is very slow!

//The values of humidity and temperature will be stored here
float h;
float t;

//And this is for the temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);

//Initializing some stuff...
void setup()
{
  
    RGB.control(true);
    Serial.begin(9600);
    Serial.println("Hello there, I'm your Photon!");
    //Setup our LED pin
    pinMode(led, OUTPUT);
    //200ms is the minimum publishing period
    publishingPeriod = publishingPeriod > 200 ? publishingPeriod : 200;
    //This initializes the sensor
    dht.begin();
    mqtt_connect();
}


//This is the callback function, necessary for the MQTT communication
void callback(char* topic, byte* payload, unsigned int length);
//Create our instance of MQTT object
MQTT client(MQTT_SERVER, 1883, callback);
//Implement our callback method that's called on receiving data from a subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  //Store the received payload and convert it to string
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  //Print the topic and the received payload
  Serial.println("topic: " + String(topic));
  Serial.println("payload: " + String(p));
  //Call our method to parse and use the received payload
  handlePayload(p);
}

void handlePayload(char* payload) {
  StaticJsonBuffer<200> jsonBuffer;
  //Convert payload to json
  JsonObject& json = jsonBuffer.parseObject(payload);
  if (!json.success()) {
    Serial.println("json parsing failed");
    return;
  }
  //Get the value of the key "command", aka. listen to incoming commands
  const char* command = json["command"];
  Serial.println("parsed command: " + String(command));
  
  //We can send commands to change the color of the RGB
  if (String(command).equals("color"))
  {
    const char* color = json["value"];
    Serial.println("parsed color: " + String(color));
    String s(color);
    if (s.equals("red")){
      RGB.color(255, 0, 0);
    }
    else if (s.equals("blue"))
      RGB.color(0, 0, 255);
    else if (s.equals("green"))
      RGB.color(0, 255, 0);
  }
}

//This function establishes the connection with the MQTT server
void mqtt_connect() {
  Serial.println("Connecting to MQTT server...");
  if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("Connection successful! Subscribing to topic...");
    //This one subscribes to the topic "cmd", so we can listen to commands
    client.subscribe("/v1/"DEVICE_ID"/cmd");
  }
  else {
    Serial.println("Connection failed! Check your credentials or WiFi network");
  }
}

//This is for the LED to blink
void blink(int interval) {
  if (millis() - lastBlinkTime > interval) {
    //Save the last time you blinked the LED
    lastBlinkTime = millis();
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
    //Set the LED with the ledState of the variable:
    digitalWrite(led, ledState);
  }
}

//This is the main loop, it's repeated until the end of time :)
void loop()
{
  
  //If we're connected, we can send data...
  if (client.isConnected()) {

    client.loop();
    //Publish within the defined publishing period
        if (millis() - lastPublishTime > publishingPeriod) {
            
            lastPublishTime = millis();
            
            //Finally publishing...
            publish();
            
        }
            
        //Blink LED  
        blink(publishingPeriod / 2);
    }
    else {
    //If connection is lost, then reconnect
        Serial.println("Retrying...");
        mqtt_connect();
    }
        
    // Wait a few seconds between measurements
  delay(2000);
        
    // Reading temperature or humidity takes about 250 ms!
    // Sensor readings may also be up to 2s 'old' (it's a very slow sensor!)
  h = dht.getHumidity();
    // Read temperature as Celsius
  t = dht.getTempCelcius();
    // Read temperature as Farenheit
  float f = dht.getTempFarenheit();
        
    // Check if any reads failed and exit early (to try again)
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
        
    // Compute heat index
    // Must send in temp in Fahrenheit!
  float hi = dht.getHeatIndex();
  float dp = dht.getDewPoint();
  float k = dht.getTempKelvin();
        
}

//Publish function; here we add what we want to send to the relayr Cloud
void publish() {
  //Create our JsonArray
  StaticJsonBuffer<300> pubJsonBuffer;
  JsonArray& root = pubJsonBuffer.createArray();

//-------------------------------------------------  
//   //First object: Humidity sensor
//   JsonObject& leaf1 = root.createNestedObject();
//   //This is how we name what we are sending
//   leaf1["meaning"] = "humidity (%)";
//   //This contains the readings of the sensor
//   leaf1["value"] = h;
//-------------------------------------------------
  
//-------------------------------------------------  
  //Second object: Temperature sensor
  JsonObject& leaf2 = root.createNestedObject();
  //This is how we name what we are sending
  leaf2["meaning"] = "temperature (C)";
  //This contains the readings of the sensor
  leaf2["value"] = t;
//-------------------------------------------------

  char message_buff[128];
  root.printTo(message_buff, sizeof(message_buff));
  client.publish("/v1/"DEVICE_ID"/data", message_buff);
  Serial.println("Publishing " + String(message_buff));
}
