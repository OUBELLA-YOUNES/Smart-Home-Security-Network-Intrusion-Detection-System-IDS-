#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Ultrasonic.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include "Arduino.h"
#include "driver/ledc.h"
#include <InfluxDbClient.h>
#include <uECC.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// Function declarations
void generateECCKeys();
void publishPublicKey();
std::string encryptData(const std::string& data);
int rng_function(uint8_t *dest, unsigned size);

const char *ssid = "Wokwi-GUEST";
const char *password = "";
const char *mqttServer = "192.168.43.51";  // Replace with your Mosquitto server IP
const int mqttPort = 1884;
const char *mqttUser = "mqttUser";
const char *mqttPassword = "1234";
const char *temperatureTopic = "homeSecurity/temperature";
const char *humidityTopic = "homeSecurity/humidity";
const char *motionTopic = "homeSecurity/motion";
const char *doorLockTopic = "homeSecurity/doorLock";
const char *alertTopic = "homeSecurity/alert";
const char *publishKeyTopic = "homeSecurity/publishkey"; // Topic to publish public key

const char* influxdb_url = "http://192.168.43.51:8086";
const char* influxdb_org = "8ac2bfc01f6cadac";
const char* influxdb_bucket = "home-security";
const char* influxdb_token = "O349YO5i15XfRoT_1u0fXneATdwfYP_7s8ZJIiXb7GhlZ9eOutAeBjc7bY-k02vnZ58a2d07yf8p8smxhey3zA==";

DHT dht(15, DHT22);
Ultrasonic ultrasonic[] = {
    Ultrasonic(13, 35),
    Ultrasonic(19, 12),
    Ultrasonic(32, 14),
    Ultrasonic(33, 4)};
Servo doorServo;

WiFiClient espClient;
PubSubClient client(espClient);

// Declare persistent ECC key pair
uint8_t privateKey[32];
uint8_t publicKey[64];
uint8_t sharedSecret[32];

void setup() {
    Serial.begin(115200);

    // Print available memory
    Serial.print("Available heap before ECC key generation: ");
    Serial.println(ESP.getFreeHeap());

    // Register the custom RNG function
    uECC_set_rng(&rng_function);

    // Generate ECC key pair
    generateECCKeys();

    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Failed to connect to WiFi");
    }

    // Connect to MQTT
    client.setServer(mqttServer, mqttPort);
    while (!client.connected()) {
        if (client.connect("esp32-client", mqttUser, mqttPassword)) {
            Serial.println("Connected to MQTT broker");

            // Publish public key to the publishkey topic
            publishPublicKey();
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying...");
            delay(5000);
        }
    }

    doorServo.attach(21);
    doorServo.write(0);

    pinMode(25, OUTPUT);
    pinMode(26, OUTPUT);
}

void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (!isnan(temperature) && !isnan(humidity)) {
        // Encrypt and publish temperature and humidity
        std::string temperatureData = std::to_string(temperature);
        std::string encryptedTemperature = encryptData(temperatureData);

        std::string humidityData = std::to_string(humidity);
        std::string encryptedHumidity = encryptData(humidityData);

        Serial.println(("Encrypted Temperature: " + encryptedTemperature).c_str());
        Serial.println(("Encrypted Humidity: " + encryptedHumidity).c_str());

        // Publish encrypted temperature and humidity to MQTT
        client.publish(temperatureTopic, encryptedTemperature.c_str());
        client.publish(humidityTopic, encryptedHumidity.c_str());

        // Write data to InfluxDB
        InfluxDBClient influxClient(influxdb_url, influxdb_org, influxdb_bucket, influxdb_token);
        Point point("temperature");
        point.addField("value", temperature); // Store raw numerical value
        point.addField("encrypted_data", encryptedTemperature.c_str()); // Store encrypted data
        if (!influxClient.writePoint(point)) {
            Serial.print("InfluxDB write failed: ");
            Serial.println(influxClient.getLastErrorMessage());
        } else {
            Serial.println("Data written to InfluxDB");
        }
    }

    // Handle motion detection
    for (int i = 0; i < 4; i++) {
        int distance = ultrasonic[i].read();
        if (distance < 10) {
            String motionMsg = "Motion detected at sensor " + String(i + 1);
            client.publish(motionTopic, motionMsg.c_str());
        }
    }

    client.loop();
    delay(1000);
}

// Function to generate ECC key pair and shared secret
void generateECCKeys() {
    const uECC_Curve_t *curve = uECC_secp256r1();
    
    if (uECC_make_key(publicKey, privateKey, curve)) {
        Serial.println("✅ ECC Key Pair Generated!");
    } else {
        Serial.println("❌ Failed to generate ECC keys");
        Serial.print("Heap memory available: ");
        Serial.println(ESP.getFreeHeap()); // Debug memory issue
        return;
    }

    if (uECC_shared_secret(publicKey, privateKey, sharedSecret, curve)) {
        Serial.println("✅ Shared Secret Generated!");

        // Print the shared secret in hexadecimal format
        Serial.print("Shared Secret (Hex): ");
        for (int i = 0; i < 32; i++) {
            Serial.printf("%02x", sharedSecret[i]);
        }
        Serial.println();
    } else {
        Serial.println("❌ Failed to generate shared secret");
        return;
    }
}

// Function to encrypt data with the shared secret using XOR operation
std::string encryptData(const std::string& data) {
    std::vector<uint8_t> encryptedData(data.begin(), data.end());
    for (size_t i = 0; i < encryptedData.size(); ++i) {
        encryptedData[i] ^= sharedSecret[i % 32];
    }

    // Convert encrypted data to hex string
    std::stringstream ss;
    for (size_t i = 0; i < encryptedData.size(); ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)encryptedData[i];
    }
    return ss.str();
}

// Function to publish the ECC public key to the MQTT broker
void publishPublicKey() {
    std::stringstream ss;
    for (int i = 0; i < 64; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)publicKey[i];
    }
    std::string publicKeyHex = ss.str();
    client.publish(publishKeyTopic, publicKeyHex.c_str());
    Serial.println(("Public Key Published: " + publicKeyHex).c_str());
}

// RNG function using ESP32 hardware RNG
int rng_function(uint8_t *dest, unsigned size) {
    while (size) {
        *dest = (uint8_t)esp_random(); // Use ESP32 hardware RNG
        dest++;
        size--;
    }
    return 1; // Success
}
