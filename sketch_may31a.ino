// librarii necesare
#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include "time.h"
  #include <ESP_Google_Sheet_Client.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
#endif
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GS_SDHelper.h>

// Data wire conectat la GPIO 4 
#define ONE_WIRE_BUS 4

// Google Project ID
#define PROJECT_ID "iot-datalog-425022"

// email service account
#define CLIENT_EMAIL "iot-datalog@iot-datalog-425022.iam.gserviceaccount.com"

// cheie privata service account
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\[REDACTED]\n-----END PRIVATE KEY-----\n";

// ID spreadsheet
const char spreadsheetId[] = "[REDACTED]]";

// instanta onewire
OneWire oneWire(ONE_WIRE_BUS);

// onewire -> senzor dallas
DallasTemperature sensors(&oneWire);

// valori temp
String temperatureC = "";
String temperatureF = "";
String temperatureK = "";

// variabile timer
unsigned long lastTime = 0;  
unsigned long timerDelay = 10000; // delay citire senzor temp

// functie token callback
void tokenStatusCallback(TokenInfo info);

// apelare timp epoch la server NTP
const char* ntpServer = "pool.ntp.org";

// variabila pt salvare timp epoch
unsigned long epochTime; 

// functie care preia timpul epoch
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Incercarea de a prelua timpul a esuat");
    return(0);
  }
  time(&now);
  return now;
}


// date logare retea
const char* ssid = "-----"; // username wifi
const char* password = "---------"; // parola wifi

// AsyncWebServer pe port 80
AsyncWebServer server(80);

// CELSIUS
String readDSTemperatureC() {

  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  if(tempC == -127.00) {
    Serial.println("Eroare citire senzor temperatura Celsius");
    return "--";
  } else {
    Serial.print("temp celsius: ");
    Serial.println(tempC); 
  }
  return String(tempC);
}

// FAHRENHEIT
String readDSTemperatureF() {

  sensors.requestTemperatures(); 
  float tempF = sensors.getTempFByIndex(0);

  if(int(tempF) == -196){
    Serial.println("Eroare citire senzor temperatura Fahrenheit");
    return "--";
  } else {
    Serial.print("temp fahrenheit: ");
    Serial.println(tempF);
  }
  return String(tempF);
}

// KELVIN
String readDSTemperatureK() {

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  float tempK = tempC + 273.15;

  if (int(tempK) == 146){
  Serial.println("Eroare citire senzor temperatura Kelvin");
  return "--";
  } else {
  Serial.print("temp kelvin: ");
  Serial.println(tempK);
  }
  return String(tempK);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">

   <script src="https://code.highcharts.com/highcharts.js"></script> 
  <style>
  
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.5rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h>Server senzor temperatura ESP DS18B20</h>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
   <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 

    <span id="temperaturef">%TEMPERATUREF%</span>
    <sup class="units">&deg;F</sup>
  </p>

  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    
    <span id="temperaturek">%TEMPERATUREK%</span>
    <sup class="ds-labels">K</sup>
  </p>

  <div id="chart-temperature" class="container"></div>
</body>
<script>
 Highcharts.setOptions({
 global: {
 useUTC: false
 }
 });

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturef").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturef", true);
  xhttp.send();
}, 10000) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturek").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturek", true);
  xhttp.send();
}, 10000) ;

//GRAFIC

var chartT = new Highcharts.Chart({
  chart:{ renderTo : 'chart-temperature' },
  title: { text: 'Grafic ' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },
  xAxis: { type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Temperatura (C)' }
    
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartT.series[0].data.length > 40) {
        chartT.series[0].addPoint([x, y], true, true, true);
      } else {
        chartT.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000 ) ;


</script>
</html>)rawliteral";

// schimbare valori placeholder
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATUREC"){
    return temperatureC;
  }
  else if(var == "TEMPERATUREF"){
    return temperatureF;
  }
  else if(var == "TEMPERATUREK"){
    return temperatureK;
  }
  return String();
}

void setup(){
  // Serial port pt debug
  Serial.begin(115200);
  Serial.println();

  configTime(0, 0, ntpServer);

  
  // Start librarie DS
  sensors.begin();

  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

  temperatureC = readDSTemperatureC();
  temperatureF = readDSTemperatureF();
  temperatureK = readDSTemperatureK();

  // conectare wifi
  WiFi.begin(ssid, password);
  Serial.println("Se incearca conexiunea Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // print adresa IP
  Serial.println(WiFi.localIP());

   // callback pt token (pt debug)
    GSheet.setTokenCallback(tokenStatusCallback);

     // setare timp inainte de refresh-ul tokenului de autentificare (60 - 3540, default 300 sec)
    GSheet.setPrerefreshSeconds(10 * 60);

    // inceperea generarii unei chei API
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);


  // ruta webpage
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureC.c_str());
  });
  server.on("/temperaturef", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureF.c_str());
  });
  server.on("/temperaturek", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureK.c_str());
  });

  
  // Start server
  server.begin();
}
 
void loop(){

  bool ready = GSheet.ready();

  if ((millis() - lastTime) > timerDelay) {
    temperatureC = readDSTemperatureC();
    temperatureF = readDSTemperatureF();
    temperatureK = readDSTemperatureK();
    lastTime = millis();

    FirebaseJson response;

     Serial.println("\nAdaugare valori spreadsheet...");
        Serial.println("----------------------------");

        FirebaseJson valueRange;

        epochTime = getTime();

        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", epochTime);
        valueRange.set("values/[1]/[0]", temperatureC);

        // Adauga valori in spreadsheet
        bool success = GSheet.values.append(&response, spreadsheetId, "Sheet1!A1", &valueRange);
        if (success){
            response.toString(Serial, true);
            valueRange.clear();
        }
        else{
            Serial.println(GSheet.errorReason());
        }
        Serial.println();
        Serial.println(ESP.getFreeHeap());
    }
}

void tokenStatusCallback(TokenInfo info){
    if (info.status == token_status_error){
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
  }  
}