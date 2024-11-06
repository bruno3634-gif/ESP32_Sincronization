#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SD.h>
#include <FS.h>

#define CS_PIN 5
#define MOSI_PIN 23
#define MISO_PIN 19
#define SCK_PIN 18

const uint16_t OLED_Color_Black = 0x0000;
const uint16_t OLED_Color_Blue = 0x001F;
const uint16_t OLED_Color_Yellow = 0xFFE0;

uint16_t OLED_Text_Color = OLED_Color_Yellow;
uint16_t OLED_Backround_Color = OLED_Color_Black;

const uint8_t OLED_pin_scl_sck = 14;
const uint8_t OLED_pin_sda_mosi = 13;
const uint8_t OLED_pin_cs_ss = 15;
const uint8_t OLED_pin_res_rst = 4;
const uint8_t OLED_pin_dc_rs = 16;

Adafruit_SSD1331 display = Adafruit_SSD1331(OLED_pin_cs_ss, OLED_pin_dc_rs, OLED_pin_sda_mosi, OLED_pin_scl_sck, OLED_pin_res_rst);

unsigned long update_display = 0;
unsigned long send_time = 0;
unsigned long last_sent = 0;

bool recording = false;
String success;
uint8_t BROADCST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

esp_now_peer_info peer_info[3];

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    unsigned long response_time = millis() - last_sent;
    success = (status == ESP_NOW_SEND_SUCCESS) ? "Delivery Success" : "Delivery Fail";

    // Log response time and MAC address to SD card if recording is active
    if (recording) {
        File file = SD.open("/teste_delays.csv", FILE_APPEND);
        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }
        file.print("Response Time: ");
        file.print(response_time);
        file.print(" ms, MAC: ");
        for (int i = 0; i < 6; i++) {
            file.print(mac_addr[i], HEX);
            if (i < 5) file.print(":");
        }
        file.print(", Status: ");
        file.println(success);
        file.close();
    }

    Serial.print("Last Packet Send Status: ");
    Serial.println(success);
}

void start_recordings() {
    recording = !recording;
}

void setup() {
    update_display = millis();
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    display.begin();
    display.setFont();
    display.fillScreen(OLED_Backround_Color);
    display.setTextColor(OLED_Text_Color);
    display.setTextSize(1);

    if (esp_now_init()) {
        Serial.println("Failed to initialize ESP-NOW");
        return;
    } else {
        Serial.println("ESP-NOW Initialized");
    }

    esp_now_register_send_cb(OnDataSent);

    memcpy(peer_info[0].peer_addr, BROADCST_ADDRESS, 6);
    peer_info[0].channel = 0;
    peer_info[0].encrypt = false;

    if (esp_now_add_peer(&peer_info[0]) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
    Serial.println("Setup complete");

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    if (!SD.begin(CS_PIN)) {
        Serial.println("Card Mount Failed");
        return;
    } else {
        Serial.println("Card Mount Success");
        File file = SD.open("/teste_delays.csv", FILE_WRITE);
    
        if (file) {
            file.println("Response Time (ms);MAC Address;Status");
            file.close();
        }
    }

    pinMode(36, INPUT_PULLUP);
    attachInterrupt(36, start_recordings, FALLING);
}

void loop() {
    if (millis() - send_time >= 5000 && recording) {  // Broadcast every 5 seconds
        int dummy_data = 16;  // Placeholder data to send
        if (esp_now_send(BROADCST_ADDRESS, (uint8_t *)&dummy_data, sizeof(dummy_data)) == ESP_NOW_SEND_SUCCESS) {
            last_sent = millis();
        } else {
            Serial.println("Failed to send");
        }
        send_time = millis();
    }

    if (millis() >= update_display + 1000) {
        display.fillScreen(OLED_Backround_Color);
        display.setCursor(0, 0);

        display.setTextColor(OLED_Color_Blue);
        display.print("Recording: ");
        display.setTextColor(recording ? OLED_Color_Yellow : OLED_Color_Black);
        display.println(recording ? "Yes" : "No");

        update_display = millis();
    }
}
