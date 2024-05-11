#include <DHT.h>
#include <MQ135.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#define DHTPIN 4         // Pin connected to DHT11 data pin
#define DHTTYPE DHT11    // DHT sensor type
#define MQ135_PIN 13     // Digital pin connected to MQ135 sensor
#define MQ2_PIN 2        // Pin connected to MQ-2 sensor

DHT dht(DHTPIN, DHTTYPE);
MQ135 gasSensor(MQ135_PIN);


//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

const float R_ZERO = 76.63; // Resistance at clean air

// Insert your network credentials
#define WIFI_SSID "realme GT NEO 3T"
#define WIFI_PASSWORD "12345687"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCm8WKE6E_SFuYB292W_dWmjZ4-3QBtGxU"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://aqi-2-fce5a-default-rtdb.asia-southeast1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;                     //since we are doing an anonymous sign in 


void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  FirebaseConfig config;
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
 /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}


void loop() {
float temperature = dht.readTemperature();
  float humidity=dht.readHumidity();
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "DHT_11/Temperature", temperature)){
      Serial.print("Temperature : ");
      Serial.println(temperature);
    }
    else {
      Serial.println("Failed To Read From The Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
   
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "DHT_11/Humidity", 0.01 + humidity)){
      Serial.print("Humidity : ");
      Serial.println(humidity);
    }
    else {
      Serial.println("Failed To Read From The Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  
  int mq135_value = digitalRead(MQ135_PIN);
  float mq135_sensorResistance = ((float)R_ZERO * (1023.0 - (float)mq135_value)) / ((float)mq135_value);
  float mq135_ppm = gasSensor.getPPM();

  int mq2_value = digitalRead(MQ2_PIN);
  float mq2_ppm = map(mq2_value, 0, 1023, 0, 500);

  if (Firebase.ready()) {
    if (Firebase.RTDB.setFloat(&fbdo, "MQ135/PPM", mq135_ppm)) {
      Serial.print("CO2 Concentration (ppm): ");
      Serial.println(mq135_ppm);
    } else {
      Serial.println("Failed to write CO2 concentration to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "MQ2/PPM", mq2_ppm)) {
      Serial.print("Methane or Butane (ppm): ");
      Serial.println(mq2_ppm);
    } else {
      Serial.println("Failed to write MQ-2 data to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }
  } else {
    Serial.println("Firebase not ready");
  }

  delay(3000); // Adjust delay as needed
}
}
