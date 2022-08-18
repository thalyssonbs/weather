double * dhtDados() {
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
