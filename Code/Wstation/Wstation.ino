#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <SD.h>
#include <SPI.h>
#include "PMS.h"
#include <SoftwareSerial.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Secrets.h"

int medidasHora = 6;    // Measures per hour

/* PMS */
SoftwareSerial mySerial(0, 2); // RX=D3, TX=D4
PMS pms(mySerial);

/* WiFi */
WiFiClient client;
const uint32_t connectTimeoutMs = 5000;

/* DHT11 */
#define DHTPIN 10
#define DHTTYPE    DHT11 
DHT_Unified dht(DHTPIN, DHTTYPE);

/* BMP280 */
Adafruit_BMP280 bmp;

/* Cliente NTP */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/* RTC */ 
RTC_DS3231 rtc;

/* Reboot counter */
#define RTC_MARKER 0x1234
unsigned int marker = 0;
unsigned int reboots = 0;

String logBook;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  delay(2000);
  Serial.println("");
  ThingSpeak.begin(client);
  dht.begin();
  rtc.begin();
  timeClient.begin();
  timeClient.setTimeOffset(-14400);
  dataHora();

  /* If the RTC was lost power sync real time from NTP again. */  
  if (rtc.lostPower()) {
    int* NT = timeNTP();
    rtc.adjust(DateTime(NT[0], NT[1], NT[2], NT[3], NT[4], NT[5]));
    Serial.println("Relógio sincronizado com o servidor NTP.");
  }  

  /* Inicia e configura o sensor BMP280 */
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  Wire.begin();
  SD.begin(15);

  /* Verifica e registra o número de reboots após energizado */
  ESP.rtcUserMemoryRead(0, &marker, sizeof(marker));
  if (marker != RTC_MARKER) {
    marker = RTC_MARKER;
    reboots = 0;
    ESP.rtcUserMemoryWrite(0, &marker,sizeof(marker));
  } else {
    ESP.rtcUserMemoryRead(sizeof(marker), &reboots, sizeof(reboots));
  }
  reboots++;
  ESP.rtcUserMemoryWrite(sizeof(marker), &reboots, sizeof(reboots));
  Serial.println("");
  Serial.printf("Boot número: %d\r\n", reboots);


  if (reboots > 1) { /* Only run this function if not is the first boot after power on. */
    logBook += ("BOOT=" + String(reboots));

    /* Coleta medidas do sensor PMS */
    int* p = pmSensor();
    int PM10 = p[0];
    int PM25 = p[1];
    int PM100 = p[2];

    /* Coleta medidas do sensor DHT */
    double* d = dhtDados();
    int Umidade = d[1];

    /* Coleta medidas do sensor BMP */
    double* b = bmpDados();
    float Temperatura = b[0];
    float Pressao = (b[1]/100);
    double bat = battery();
  
    /* Coleta e imprime data e hora */
    int* horaCerta = dataHora();

    /* Concatena e grava as medidas no cartão Micro SD */
    String writSD = (String(p[0]) + "," + 
                    String(p[1]) + "," + 
                    String(p[2]) + "," + 
                    String(d[0]) + "," + 
                    String(b[0]) + "," + 
                    String(horaCerta[7]) + "," + 
                    String(d[1]) + "," + 
                    String(b[1]) + "," +
                    String(horaCerta[2]) + "/" + 
                    String(horaCerta[1]) + "/" + 
                    String(horaCerta[0]) + "," + 
                    String(horaCerta[4]) + ":" + 
                    String(horaCerta[5]) + ":" + 
                    String(horaCerta[6]) + "," + 
                    String(bat));
    gravarSD(writSD);

    /* Conecta na rede WiFi e transmite os dados para a internet */
    if (verifWifi()) {
      logBook += ",WIFI=OK";
      transmitirDados(PM10, PM25, PM100, Temperatura, Umidade, Pressao, bat);
    }
    else {
      Serial.println("Sem conexão com a internet. Dados não transmitidos.");
      logBook += ",WIFI=ERRO,POST=ERRO";
    }

    /* Concatena e grava o Log no cartão Micro SD */
    logBook += "--" + 
              String(horaCerta[2]) + "/" + 
              String(horaCerta[1]) + "/" + 
              String(horaCerta[0]) + "-" + 
              String(horaCerta[4]) + ":" + 
              String(horaCerta[5]) + ":" + 
              String(horaCerta[6]);
    gravarLog(logBook);
  }

  /* Coleta a hora atual */
  int* horaCerta = dataHora();
  int minuto = horaCerta[5];
  int segundos = horaCerta[6];
  Serial.println("");
  delay(200);

  /* Calcula o tempo de suspensão até a próxima medida */
  float intervalo = 60 / medidasHora;
  float tempoSono = ((intervalo*60)-(60*minuto + segundos));
  while (tempoSono < 0) {
    tempoSono = tempoSono + (intervalo*60);
  }

  /* Suspende o módulo ESP */
  
  Serial.print("Entrando em suspensão por ");
  if (int(tempoSono/60) != 0) {
    int minSono = tempoSono/60;
    Serial.print(String(minSono) + " minutos e ");
  }
  int segSono = ((tempoSono/60)-(int(tempoSono/60)))*60;
  Serial.println(String(segSono) + " segundos...");
  pms.sleep();
  ESP.deepSleep((tempoSono-30)*1e6);
  
}

void loop() {}


/*
 * The verifWifi() function checks the WiFi connection and tries to reconnect for 30 seconds.
 */

bool verifWifi() {
  
  const char* ssid     = STASSID;
  const char* password = STAPSK;
  unsigned long tConnect;
  
  WiFi.mode(WIFI_STA);

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println();
    Serial.print("Tentando se conectar à rede ");
    Serial.println(ssid);
    tConnect = millis();
    //WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    //wifiMulti.run(connectTimeoutMs);
    while (WiFi.status() != WL_CONNECTED && millis()-tConnect < 30000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.print("Endereço IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Tempo decorrido: ");
      Serial.print(int((millis()-tConnect)/1000));
      Serial.println(" Segundos.");
      return true;
    }
    else {
      Serial.println("");
      Serial.print("Não foi possível se conectar à rede ");
      Serial.print(ssid);
      Serial.println(".");
      return false;
    }
  }
  else {
    Serial.println("WiFi conectado!");
    return true;
  }
}
