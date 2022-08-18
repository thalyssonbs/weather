bool verifWifi() {

  #ifndef STASSID
  #define STASSID "Thalysson"
  #define STAPSK  "hjpj64y8"
  #endif
  const char* ssid     = STASSID;
  const char* password = STAPSK;
  unsigned long tConnect;
  
  WiFi.mode(WIFI_STA);

  // Register multi WiFi networks
  // wifiMulti.addAP("Thalysson", "hjpj64y8");
  // wifiMulti.addAP("Unir", "r1c1unir");
  // wifiMulti.addAP("ThalyssoniPhone", "18027100");
  // wifiMulti.addAP("Lucas", "12345678");
  // More is possible

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
