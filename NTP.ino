int * timeNTP() {

  Serial.println("Iniciando sincronização automática do relógio!");
  String formattedDate;
  String dayStamp;
  String timeStamp;

  if (verifWifi()) {
    timeClient.update();
    int hour = timeClient.getHours();
    int minute = timeClient.getMinutes();
    int second = timeClient.getSeconds();
    
    Serial.println(timeClient.getFormattedDate());
    formattedDate = timeClient.getFormattedDate();
    int splitT = formattedDate.indexOf("T");
    String day = formattedDate.substring(splitT-2, splitT);
    String month = formattedDate.substring(splitT-5, splitT-3);
    String year = formattedDate.substring(0, splitT-6);

    static int r[6];
    r[0] = year.toInt();
    r[1] = month.toInt();
    r[2] = day.toInt();
    r[3] = int(hour);
    r[4] = int(minute);
    r[5] = int(second);

    return r;
  }
}
