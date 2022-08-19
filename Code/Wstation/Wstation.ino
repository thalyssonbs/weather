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

bool verifWifi() {
  /*
    * The verifWifi() function checks the WiFi connection and tries to reconnect for 30 seconds.
  */
  
  const char* ssid     = STASSID;
  const char* password = STAPSK;
  unsigned long tConnect;
  
  WiFi.mode(WIFI_STA);

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println();
    Serial.print("Tentando se conectar à rede ");
    Serial.println(ssid);
    tConnect = millis();
    WiFi.begin(ssid, password);
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

int * dataHora() {
  /*
   * The dataHora() function get the real time data from the RTC module and return the data separately.
  */

  char daysOfTheWeek[7][12] = {"Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado"};
  DateTime now = rtc.now();
  int ano = (now.year());
  int mes = (now.month());
  int dia = (now.day());
  String diaSemana = (daysOfTheWeek[now.dayOfTheWeek()]);
  int hora = (now.hour());
  int minuto = (now.minute());
  int segundo = (now.second());
  double tempRTC = (rtc.getTemperature());

  static int r[8];
  r[0] = ano;
  r[1] = mes;
  r[2] = dia;
  r[4] = hora;
  r[5] = minuto;
  r[6] = segundo;
  r[7] = tempRTC;  

  Serial.print(dia);
  Serial.print('/');
  Serial.print(mes);
  Serial.print('/');
  Serial.print(ano);
  Serial.print(' ');
  Serial.print(diaSemana);
  Serial.print(' ');
  Serial.print(hora);
  Serial.print(':');
  Serial.print(minuto);
  Serial.print(':');
  Serial.print(segundo);
  Serial.println();

  return r;
}

void gravarSD(String dados) {
  /*
    * The gravarSD() function save the last data in a file on SD card.
    * The gravarLog() function save the log string in a separate file on SD card.
  */
  File dataFile = SD.open("dados.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dados);
    dataFile.close();
    logBook += ",SD=OK";
    Serial.println("Dados gravados no cartão Micro SD!");
    Serial.println(dados);
  }
  else {
    Serial.println("Erro ao abrir arquivo.");
    logBook += ",SD=ERRO";
  }
}

void gravarLog(String dados) {
  File dataFile = SD.open("Log.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dados);
    dataFile.close();
    Serial.println("Log registrado!");
    Serial.println(dados);
  }
  else {
    Serial.println("Erro ao abrir arquivo de log.");
  }
}

void transmitirDados(int PM10, int PM25, int PM100, float Temperatura, int Umidade, float Pressao, float bat) {
  /*
    * The transmitirDados() function send the data to ThingSpeak server.
  */
  unsigned long myChannelNumber = SECRET_CH_ID;
  const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

  ThingSpeak.setField(1, PM10);
  ThingSpeak.setField(2, PM25);
  ThingSpeak.setField(3, PM100);
  if (Temperatura != 0) {
    ThingSpeak.setField(4, Temperatura);
  }
  if (Umidade != 0){
    ThingSpeak.setField(5, Umidade);
  }
  if (Pressao != 0) {
    ThingSpeak.setField(6, Pressao);
  }
  ThingSpeak.setField(7, bat);
  
  
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Transmissão concluída.");
    logBook += ",POST=OK";
  }
  else{
    Serial.println("Erro na transmissão dos dados. Código de erro HTTP " + String(x));
    logBook += ",POST=ERRO";
  }
}

int * pmSensor() {
  /*
    * The pmSensor() function read and return the particulate matter values from the PMS7003 sensor.
  */

  PMS::DATA data;
  pms.passiveMode();
  Serial.println(F("Estabilizando sensor... Aguarde!"));
  pms.wakeUp();
  delay(60000);
  pms.requestRead();

  if (pms.readUntil(data))
  {
    int pm10 = data.PM_AE_UG_1_0;
    int pm25 = data.PM_AE_UG_2_5;
    int pm100 = data.PM_AE_UG_10_0;
    static int r[3];
    r[0] = pm10;
    r[1] = pm25;
    r[2] = pm100;
    pms.sleep();
    logBook += ",PMS=OK";
    return r;
  }
  else
  {
    Serial.println("Sem dados.");
    logBook += ",PMS=ERRO";
    pms.sleep();
  }
}

int * timeNTP() {
  /*
    * The timeNTP() function get the real time data from NTP server and return the formatted values separately.
  */

  Serial.println("Iniciando sincronização automática do relógio!");
  String formattedDate;
  String dayStamp;
  String timeStamp;
  
  if (verifWifi()) {
    timeClient.begin();
    timeClient.setTimeOffset(-14400);
    timeClient.update();
    int hour = timeClient.getHours();
    int minute = timeClient.getMinutes();
    int second = timeClient.getSeconds();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    int currentYear = ptm->tm_year+1900;

    static int r[6];
    r[0] = currentYear;
    r[1] = currentMonth;
    r[2] = monthDay;
    r[3] = int(hour);
    r[4] = int(minute);
    r[5] = int(second);

    return r;
  }
}

double * dhtDados() {
  /*
    * The dhtDados() function read and return humidity and temperature data from the DHT11 sensor.
    * Note that the reading process is repeating due an error in reply from DHT at first read.
    * Adjust needed after sensor replacement.
  */
  double umid;
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  double temp1 = event.temperature;
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    umid = NULL;
  }
  else {
    umid = event.relative_humidity;
  }
  delay(2000);

  /* Repetindo o código para evitar o erro de leitura */
  dht.temperature().getEvent(&event);
  temp1 = event.temperature;
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    umid = NULL;
    logBook += ",DHT=ERRO";
  }
  else {
    umid = event.relative_humidity;
    logBook += ",DHT=OK";
  }

  static double r[2];
  r[0] = temp1;
  r[1] = umid;
  return r;
}

double * bmpDados() {
  /*
    * The bmpDados() function get and return temperature and atmosphere pressure data from BMP280 sensor.
  */
  if (bmp.takeForcedMeasurement()) {
    double temperatura = bmp.readTemperature();
    double pressao = bmp.readPressure();
    double alt = bmp.readAltitude(1013.25);
    static double r[3];
    r[0] = temperatura;
    r[1] = pressao;
    r[2] = alt;
    logBook += ",BMP=OK";
    return r;
    }
    else {
    Serial.println("Falha no sensor BMP!");
    logBook += ",BMP=ERRO";
  }
}

double battery() {
  /*
    * The battery() function read the ADC input and calculate the battery voltage and return the value.
  */

  double sensorValue = analogRead(A0);
  double voltage = sensorValue * 0.0167;
  Serial.println(sensorValue);
  Serial.println(voltage);
  return voltage;

}
