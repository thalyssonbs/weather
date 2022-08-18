double battery() {
  double sensorValue = analogRead(A0);
  double voltage = sensorValue * 0.0167;
  Serial.println(sensorValue);
  Serial.println(voltage);
  return voltage;

}