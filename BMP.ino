double * bmpDados() {
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
