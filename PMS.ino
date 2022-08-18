/*
 * The pmSensor() function read and return the particulate matter values from the PMS7003 sensor.
 */

int * pmSensor() {

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
