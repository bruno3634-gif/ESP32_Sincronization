#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>


#define CS_PIN 5
#define MOSI_PIN 23
#define MISO_PIN 19
#define SCK_PIN 18



unsigned long update_display = 0;

unsigned long time_log = 0;

int incomingsensor;
int incomingdistance;
float incomingbat;

String success;


esp_now_peer_info peer_info[3]; // estrutura para guardar dados dos outros ESP
int count = 0;






uint8_t BROADCST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_dados
{
    int distance;
    int sensor;
    float position;
} struct_message;

struct_dados incoming_readings;



void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    if (status == 0)
    {
        success = "Delivery Success :)";
    }
    else
    {
        success = "Delivery Fail :(";
    }
}
volatile boolean recording = false;

struct_message incomingReadings;


void start_recordings(){
    if(recording == false){
        SD.open("/data.csv", FILE_WRITE);
        SD.end();
        recording = true;
    }
    else{
        recording = false;
    }
    

    
}

int store_data(int distance, int sensor, float bat,int position){
    if(recording == false){
        return 0;
    }
    else{
        File file = SD.open("/data.csv", FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return -1;
    }
    else{
        unsigned long seconds = millis() / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;

        String time = String(hours) + ":" + String(minutes % 60) + ":" + String(seconds % 60);

        file.print(time);
        file.print(";");
        file.print(";");
        file.print(position);
        file.print(";");
        file.print(distance);
        file.print(";");
        file.print(sensor);
        file.print(";");
        file.print(bat);
        file.println();
        file.close();
        return 1;
    }
    }
}




void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    Serial.print("Bytes received: ");
    Serial.println(len);
    incomingsensor = incomingReadings.sensor;
    Serial.println(incomingsensor);
    incomingdistance = incomingReadings.distance;
    Serial.println(incomingdistance);
    incomingbat = incomingReadings.position;
    Serial.println(incomingbat);
    store_data(incomingdistance, incomingsensor, incomingbat, 1);
}





void setup()
{
    update_display = millis();
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);

    /* Tenta inicializar o espnow */
    if (esp_now_init())
    {
        Serial.println("Não foi possível iniciar");
        return;
    }
    else
    {
        Serial.println("Iniciado com sucesso");
    }

    /* Callback para enviar */
    esp_now_register_send_cb(OnDataSent);

    /* Callback para receber */
    esp_now_register_recv_cb(OnDataRecv);

    // Registrar peer
    memcpy(peer_info[0].peer_addr, BROADCST_ADDRESS, 6);
    peer_info[0].channel = 0;
    peer_info[0].encrypt = false;

    // Adiciona peer
    if (esp_now_add_peer(&peer_info[0]) != ESP_OK)
    {
        Serial.println("Falha ao adicionar peer");
        return;
    }
    Serial.println("Setup feito");


    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    if (!SD.begin(CS_PIN)) {
        Serial.println("Card Mount Failed");
        return;
    }
    else
    {
        Serial.println("Card Mount Success");
    }

    //configure button interrupt
    pinMode(4, INPUT_PULLUP);
    attachInterrupt(4, start_recordings, FALLING);
}

void loop(){

}


