#include <WiFi.h>
#include <WebSocketsClient.h>  // include before MQTTPubSubClient.h
#include <MQTTPubSubClient.h>
#include <base64.h>
#include <ArduinoJson.h>
#include <esp_camera.h>


const char* ssid = "Test";
const char* pass = "w4asz57e";
uint8_t* tempImage;
int tempLen = 0;
int loop1 = 0;

StaticJsonBuffer<20000> JSONbuffer;
JsonObject& JSONencoder = JSONbuffer.createObject();


WebSocketsClient client;
// MQTTPubSubClient mqtt;
MQTTPubSub::PubSubClient<21000> mqtt;

void init_wifi(){
    WiFi.begin(ssid, pass);

    Serial.print("connecting to wifi...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(" connected!");
}

void init_camera(){
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = 5;
    config.pin_d1       = 18;
    config.pin_d2       = 19;
    config.pin_d3       = 21;
    config.pin_d4       = 36;
    config.pin_d5       = 39;
    config.pin_d6       = 34;
    config.pin_d7       = 35;
    config.pin_xclk     = 0;
    config.pin_pclk     = 22;
    config.pin_vsync    = 25;
    config.pin_href     = 23;
    config.pin_sscb_sda = 26;
    config.pin_sscb_scl = 27;
    config.pin_pwdn     = 32;
    config.pin_reset    = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = FRAMESIZE_QVGA; // QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;           
    config.fb_count     = 1;

    // If a camera with PSRAM
    if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
    }
    else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
    }
}

bool capture_image(){
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
    return false;
    }
    tempImage = fb->buf;
    tempLen = fb->len;
    Serial.println("CLIC");
    esp_camera_fb_return(fb);
    return true;
}


void publish_mqtt(String message){
    // publish message
    Serial.println("Start to publish: ");
    mqtt.publish("ADVTOPIC", message);
}

void send_picture(){
   Serial.println("Start to Send...");
    if(capture_image()){
        String encodedImage = base64::encode(tempImage, tempLen);
        JSONencoder["data"] = encodedImage.c_str();
        char JSONmessageBuffer[15000];
        JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        // String image = String(JSONmessageBuffer);
        publish_mqtt(String(JSONmessageBuffer));
        return;
    }
}

// void callback(const String& topic, const String& payload, unsigned int length){
//     Serial.print("Message arrived in topic: ");
//     Serial.println(topic);
 
//     Serial.print("Message:");
//     for (int i = 0; i < length; i++) {
//         Serial.print((char)payload[i]);
//     }
 
//     Serial.println();
//     Serial.println("-----------------------");
// }


void setup() {
    Serial.begin(115200);
    init_camera();
    init_wifi();

    Serial.println("connecting to host...");
    client.beginSSL("192.168.178.32", 443, "/ws/mqtt", NULL, "mqtt");
//    client.beginSSL("10.181.80.11", 443, "/ws/mqtt", NULL, "mqtt");
//    client.beginSSL("noderedlnxse20220516.o1jkfiv0l98.eu-de.codeengine.appdomain.cloud", 443, "/ws/mqtt", NULL, "mqtt");
    // client.beginSSL("test.mosquitto.org", 8081, "/", "", "mqtt");  // "mqtt" is required
    client.setReconnectInterval(2000);

    // initialize mqtt client


    mqtt.begin(client);

    Serial.print("connecting to mqtt broker...");
    while (!mqtt.connect("3753249", "03753249", "3753249")) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(" connected!");

    // subscribe callback which is called when every packet has come
    mqtt.subscribe([](const String& topic, const String& payload, const size_t size) {
        Serial.println("mqtt received: " + topic + " - " + payload);
    });

    // subscribe topic and callback which is called when /hello has come
    mqtt.subscribe("COMMAND", [](const String& payload, const size_t size) {
        Serial.print("COMMAND");
        Serial.println(payload);
    });
    
    mqtt.subscribe("ADVTOPIC", [](const String& payload, const size_t size) {
        Serial.print("ADVTOPIC");
        Serial.println(payload);
    });
}

void loop() {
    if(!mqtt.update()){
        Serial.println("MQTT Not Connected!!!");
        delay(1000);
    }
    if(WiFi.status() != WL_CONNECTED) {
        Serial.print("Wifi Not Connected");
        delay(1000);
    }
    if(loop1 == 0){
        loop1 ++;
        JSONencoder["id"] = "03753249";
        JSONencoder["data"] = "AdvTopic Xiaoyang Chen/03753249 Start Sending";
        char TempBuffer[80];
        JSONencoder["data"].printTo(TempBuffer, (JSONencoder["data"].measureLength()+1));
        String message = "{\"id\": \"03753249\", \"data\": " + String(TempBuffer) + "}";
        mqtt.publish("ADVTOPIC", message);
        // mqtt.publish("ADVTOPIC", JSONmessageBuffer);
    }else if (loop1 == 11){
        static uint32_t prev_ms = millis();
        if (millis() > prev_ms + 3000) {
            JSONencoder["data"] = "AdvTopic Xiaoyang Chen/03753249 End of Sending";
            char JSONmessageBuffer[80];
            JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
            mqtt.publish("ADVTOPIC", JSONmessageBuffer);
            mqtt.disconnect();
        }
    }
    else if((loop1 > 0) && (loop1 < 11)){
        static uint32_t prev_ms = millis();
        if (millis() > prev_ms + 3000) {
            loop1 ++;
            prev_ms = millis();
            send_picture();
            Serial.println(loop1);
        }
    }
}
