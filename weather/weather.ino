#include <TridentTD_LineNotify.h>
#include <Wire.h>

#include <HTTPClient.h>
#include <Arduino_JSON.h>

#define LM73_ADDR 0x4D

String jsonBuffer;
String temp_sensor_string;
String temp_forecast_string;
float temp_sensor;
float temp_forecast;

String city = "Bangkok";
String countryCode = "TH";

void initWifi(){
  WiFi.begin(ssid, PASSWORD);

  Serial.println("WiFi connecting to");

  while (WiFi.status() != WL_CONNECTED) {
  Serial.print(".");
  delay(400);
  }

  Serial.println("WiFi connected");

}

void initLine(){
  LINE.setToken(LINE_TOKEN);
  Serial.println(LINE.getVersion());
}

void send_line(String message){
  LINE.notify(message);
}

float readTemperature(){
  Wire1.beginTransmission(LM73_ADDR);
  Wire1.write(0x00);
  Wire1.endTransmission();

  uint8_t count = Wire1.requestFrom(LM73_ADDR, 2);
  float temp = 0.0;
  if (count == 2) {
    byte buff[2];
    buff[0] = Wire1.read();
    buff[1] = Wire1.read();
    temp += (int)(buff[0]<<1);
    if (buff[1]&0b10000000) temp += 1.0;
    if (buff[1]&0b01000000) temp += 0.5;
    if (buff[1]&0b00100000) temp += 0.25;
    if (buff[0]&0b10000000) temp *= -1.0;
  }

  return temp;
}

float forecast(){
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
        
  jsonBuffer = httpGETRequest(serverPath.c_str());
  JSONVar myObject = JSON.parse(jsonBuffer);

  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(myObject) == "undefined"){
    Serial.println("Parsing input failed!");
    return 0.0;
  }

  String temp_real = JSON.stringify(  myObject["main"]["temp"]  );

  return(temp_real.toFloat() - 273.15 );
}

String httpGETRequest(const char* serverName){
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void setup(){

  Serial.begin(9600);
  initWifi();
  initLine();
  Wire1.begin(4, 5);

}

void loop(){
  temp_forecast = forecast();
  temp_sensor = readTemperature();

  temp_sensor_string = "Over Temperature: "+String(temp_sensor);
  temp_forecast_string = "Temperature From OpenWeather: "+String(temp_forecast);

  if(temp_sensor >= 32.00){
    send_line(temp_sensor_string+"\n\n"+temp_forecast_string);
    Serial.println(temp_sensor_string+"\n\n"+temp_forecast_string);
  }

  delay(5000);
}
