void transmitirDados(int PM10, int PM25, int PM100, float Temperatura, int Umidade, float Pressao, float bat) {
  
  #define SECRET_CH_ID 1485261
  #define SECRET_WRITE_APIKEY "VFDZMXDTBQWVSWPT"
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
