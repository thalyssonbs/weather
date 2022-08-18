/*
 * The dataHora() function get the real time data from the RTC module and return the data separately.
 */

int * dataHora() {

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
