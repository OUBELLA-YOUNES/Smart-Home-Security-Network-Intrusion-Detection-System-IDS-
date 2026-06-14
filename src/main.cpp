// ...existing code...

std::string encryptData(const std::string& data) {
    const struct uECC_Curve_t * curve = uECC_secp256r1();
    uint8_t privateKey[32];
    uint8_t publicKey[64];
    uint8_t sharedSecret[32];

    // Generate a new private key
    if (!uECC_make_key(publicKey, privateKey, curve)) {
        Serial.println("❌ Failed to generate random private key");
        return "";
    }

    // Generate the shared secret
    if (!uECC_shared_secret(publicKey, privateKey, sharedSecret, curve)) {
        Serial.println("❌ Failed to generate shared secret");
        return "";
    }

    // Encrypt the data using the shared secret
    std::vector<uint8_t> encryptedData(data.begin(), data.end());
    for (size_t i = 0; i < encryptedData.size(); ++i) {
        encryptedData[i] ^= sharedSecret[i % 32];
    }

    return std::string(encryptedData.begin(), encryptedData.end());
}

void setup() {
    Serial.begin(115200);

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

    doorServo.attach(21);
    doorServo.write(0);

    pinMode(greenLedPin, OUTPUT);
    pinMode(redLedPin, OUTPUT);

    // Initialize LEDC channels properly
    ledcSetup(0, 5000, 8); // Channel 0, 5kHz, 8-bit resolution
    ledcAttachPin(greenLedPin, 0);
    ledcSetup(1, 5000, 8); // Channel 1, 5kHz, 8-bit resolution
    ledcAttachPin(redLedPin, 1);

    client.setServer(mqttServer, mqttPort);
    while (!client.connected()) {
        if (client.connect("esp32-client", mqttUser, mqttPassword)) {
            Serial.println("Connected to MQTT broker");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying...");
            delay(5000);
        }
    }
}

void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (!isnan(temperature) && !isnan(humidity)) {
        // Publish temperature directly
        Serial.println("Publishing temperature: " + String(temperature));
        client.publish(temperatureTopic, String(temperature).c_str());

        // Publish humidity directly
        Serial.println("Publishing humidity: " + String(humidity));
        client.publish(humidityTopic, String(humidity).c_str());

        // Encrypt and send to InfluxDB
        std::string temperatureData = std::to_string(temperature);
        std::string encryptedData = encryptData(temperatureData);
        if (encryptedData.empty()) {
            Serial.println("❌ Encryption failed, skipping InfluxDB write");
        } else {
            Serial.println("Encrypted Data: " + String(encryptedData.c_str()));
            InfluxDBClient influxClient(influxdb_url, influxdb_org, influxdb_bucket, influxdb_token);
            Point point("temperature");
            point.addField("value", temperature); // Store raw numerical value
            point.addField("encrypted_data", encryptedData.c_str()); // Store encrypted data
            if (!influxClient.writePoint(point)) {
                Serial.print("InfluxDB write failed: ");
                Serial.println(influxClient.getLastErrorMessage());
            } else {
                Serial.println("Data written to InfluxDB");
            }
        }
    }

    for (int i = 0; i < getArrayLengthMotionDetected; i++) {
        int distance = ultrasonic[i].read();
        motionDetected[i] = (distance < 10);
        if (motionDetected[i]) {
            Serial.println("Motion detected at sensor " + String(i + 1));
            client.publish(motionTopic, ("Motion detected at sensor " + String(i + 1)).c_str());
        }
    }

    // Check for door lock/unlock commands
    if (client.connected()) {
        client.loop();
    }

    delay(1000);
}

// ...existing code...
