/*
 * The gravarSD() function save the last data in a file on SD card.
 * The gravarLog() function save the log string in a separate file on SD card.
 */

void gravarSD(String dados) {
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
