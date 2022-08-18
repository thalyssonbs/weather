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
