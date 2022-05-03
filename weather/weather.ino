// Sensor Temperature
#include <Wire.h>

// HTTP
#include <HTTPClient.h>

// Linenotify and 
#include <TridentTD_LineNotify.h>

// Tool for read JSON
#include <Arduino_JSON.h>

// LED
//#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// keep it a secret token.
#include "API_Token.h"
#include "Wifi_info.h"

// ThingSpeak.h
#include "ThingSpeak.h"

#define LM73_ADDR 0x4D
#define SEND_DELAY 15000

Adafruit_8x16minimatrix matrix;

String jsonBuffer;
String temp_sensor_string;
String temp_forecast_string;
String Mode = "LM73";
float temp;
float temp_sensor;
float temp_forecast;

WiFiClient client;
HTTPClient http;

String city = "Surat Thani";
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
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey; // link that take to JSON data
  //Serial.print(serverPath);
  jsonBuffer = httpGETRequest(serverPath.c_str());
  JSONVar myObject = JSON.parse(jsonBuffer);
  
  if (JSON.typeof(myObject) == "undefined"){
    Serial.println("Parsing input failed!");
    return 0.0;
  }

  String temp_real = JSON.stringify(  myObject["main"]["temp"]  );

  return(temp_real.toFloat() - 273.15 );
}

void thingspeak(float value1, float value2){
  ThingSpeak.setField(1, value1);
  ThingSpeak.setField(2, value2);

  int tempo = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if(tempo == 200){
      //Serial.println("Temperature Sensor: " + tempVal);
      //Serial.println("LDR Sensor: " + ldrVal);
      Serial.println("....Channel update successful.");
    }
  else{
      Serial.println("Problem updating channel. HTTP error code " + String(tempo));
  }
}

String httpGETRequest(const char* serverName){

  // Your Domain name with URL path or IP address with path
  // initiate Server and Client
  http.begin(client, serverName);
  
  // Start Connection And Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
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
  ThingSpeak.begin(client);
  Wire1.begin(4, 5);
  matrix.begin(0x70);

  matrix.setRotation(1);
  matrix.setTextSize(1);
  matrix.setTextColor(LED_ON);
  matrix.setTextWrap(false);
  attachInterrupt(16, ChangeMode, RISING);
  attachInterrupt(14, ChangeMode, RISING);
}

void loop(){
  temp_forecast = forecast();
  temp_sensor = readTemperature();
  
  temp_sensor_string = "Over Temperature: "+String(temp_sensor)+"C*";
  temp_forecast_string = "Temperature From OpenWeather: "+String(temp_forecast)+"C*";
  
  if(temp_sensor >= 32.00){
    send_line(temp_sensor_string+"\n\n"+temp_forecast_string);
    //Serial.println(temp_sensor_string+"\n\n"+temp_forecast_string);

  }

  void ChangeMode(){
    if(Mode == "LM73"){
      Mode = "openWeather";
    }else{
      Mode = "LM73";
    }
  }

  Serial.println(temp_sensor_string+"\n\n"+temp_forecast_string);

  //delay(SEND_DELAY);

  thingspeak(temp_forecast, temp_sensor);
  
  for(int8_t y = 0; y < 8; y++){
    if(Mode == "LM73"){
      temp = readTemperature();
      Serial.println("Now, Temperature is "+String(temp_sensor)+"C*");
    }else{
      temp = temp_forecast;
    }
    for (int8_t x = 3; x >= -16; x--) {
      matrix.clear();
      matrix.setCursor(x, 0);
      matrix.print(temp);
      matrix.writeDisplay();
      delay(100);
    }

  }
}
