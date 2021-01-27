
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

#define DHTPIN 2     // Digital pin connected to the DHT sensor 
// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS; /// 1000 ms

#define WIFI_SSID "a.Cucumber"
#define WIFI_PASSWORD "Cucumber123"

#define FIREBASE_HOST "dht11-4b097-default-rtdb.firebaseio.com"

/** The database secret is obsoleted, please use other authentication methods,
   see examples in the Authentications folder.
*/
#define FIREBASE_AUTH "NYqaVUXCx35ARjuswlzhlX3Ybhsr5nNaNltCEH7w"

//Define FirebaseESP8266 data object
FirebaseData fbdo1;
FirebaseData fbdo2;

unsigned long sendDataPrevMillis = 0;

String parentPath = "/Test/Stream";
String childPath[2] = {"/humidity", "/temperature"};
size_t childPathSize = 2;

float temperature = 0 ;
float humidity = 0 ;

void printResult(FirebaseData &data);// bu ne işe yarıyor

void streamCallback(MultiPathStreamData stream)
{
  Serial.println();
  Serial.println("Stream Data1 available...");

  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++) //// burayı iyice anla
  {
    if (stream.get(childPath[i]))
    {
      Serial.println("path: " + stream.dataPath + ", type: " + stream.type + ", value: " + stream.value);
    }
  }

  Serial.println();

}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
    Serial.println();
    Serial.println("Stream timeout, resume streaming...");
    Serial.println();
  }
}

void setup()
{

  Serial.begin(115200);

  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print TEMPERATURE sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print HUMIDITY sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set the size of WiFi rx/tx buffers in the case where we want to work with large data.
  fbdo1.setBSSLBufferSize(1024, 1024);

  //Set the size of HTTP response buffers in the case where we want to work with large data.
  fbdo1.setResponseSize(1024);


  //Set the size of WiFi rx/tx buffers in the case where we want to work with large data.
  fbdo2.setBSSLBufferSize(1024, 1024);

  //Set the size of HTTP response buffers in the case where we want to work with large data.
  fbdo2.setResponseSize(1024);

  if (!Firebase.beginMultiPathStream(fbdo1, parentPath, childPath, childPathSize))
  {
    Serial.println("------------------------------------");
    Serial.println("Can't begin stream connection...");
    Serial.println("REASON: " + fbdo1.errorReason());
    Serial.println("------------------------------------");
    Serial.println();
  }

  Firebase.setMultiPathStreamCallback(fbdo1, streamCallback, streamTimeoutCallback);
}

void loop()
{
  //delay(delayMS);// wait for a measurement
  if (millis() - sendDataPrevMillis > delayMS)
  {
    sendDataPrevMillis = millis();

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      temperature = event.temperature;
      Serial.println(F("°C"));
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    }
    else {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      humidity = event.relative_humidity;
      Serial.println(F("%"));
    }

    //This will trig the another stream event.
    FirebaseJson json;
    json.set("humidity", humidity);
    json.set("temperature", temperature);
    if (Firebase.setJSON(fbdo2, parentPath, json))
    {
      Serial.println("PASSED");
      Serial.println();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo2.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
    /*
        //This will trig the another stream event.
        Serial.println("------------------------------------");
        Serial.println("Set int...");

        if (Firebase.setInt(fbdo2, parentPath + "/humidity2/", event.relative_humidity))
        {
          Serial.println("PASSED");
          Serial.println();
        }
        else
        {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo2.errorReason());
          Serial.println("------------------------------------");
          Serial.println();
        }
        //This will trig the another stream event.

        Serial.println("------------------------------------");
        Serial.println("Set int...");

        if (Firebase.setInt(fbdo2, parentPath + "/temperature2/", event.temperature))
        {
          Serial.println("PASSED");
          Serial.println();
        }
        else
        {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo2.errorReason());
          Serial.println("------------------------------------");
          Serial.println();
        }*/
  }
}
