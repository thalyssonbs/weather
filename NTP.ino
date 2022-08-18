int * timeNTP() {

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
