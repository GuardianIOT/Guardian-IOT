// Coleção de bibliotecas
#include <WiFi.h> 
#include <time.h>
#include <ThingerESP32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Thinger
#define USERNAME "brunofbpaula"
#define DEVICE_ID "AeroIOT"
#define DEVICE_CREDENTIAL "MadeByGuardian"

// Credenciais WiFi Wokwi
const char* SSID = "Wokwi-GUEST";
const char* SSID_PASSWORD = "";

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// API da Guardian
const char* serverRaios = "https://guardian.com/api";
const char* serverClima = "https://guardian-b2d7eggyhgewctg7.canadacentral-01.azurewebsites.net/api/v1/Sensor/cadastraDadosSensor";
const char* serverLogin = "https://guardian-b2d7eggyhgewctg7.canadacentral-01.azurewebsites.net/api/v1/login";

// Credenciais de Usuário
const char* userEmail = "user@email.com";
const char* userPassword = "user@123";
String jwtToken;

// API da Xweather com parâmetros para coletar dados de Ventos do Araripe III
const char* xweatherLightning = "https://data.api.xweather.com/lightning/closest?p=-7.7690457,-40.6633014&format=json&radius=25mi&minradius=0mi&filter=cg&limit=10&client_id=9jSiXyDl4tCI5t3fLctsh&client_secret=XDDBUKJgfXfmUDiLXRHLEyshvv8H549mZb8IH9hF";
const char* xweatherConditions = "https://data.api.xweather.com/conditions/-7.7690457,-40.6633014?format=json&plimit=1&filter=1min&client_id=9jSiXyDl4tCI5t3fLctsh&client_secret=XDDBUKJgfXfmUDiLXRHLEyshvv8H549mZb8IH9hF";

// Buffers
String jsonBufferConditions;
String jsonBufferLightning;

// Dados do timer
unsigned long lastTime = 0;
unsigned long timerDelay = 500000;

// Variáveis do sensor de quedas de raios
String data_hora_raio;
String tipo_raio;
String amplitude_raio;
float latitude_raio;
float longitude_raio;

// Variáveis do sensor de dados climáticos
String timezone_clima;
String data_hora_clima;
float temperatura_clima;
float umidade_clima;
String direcao_vento;
float velocidade_vento;
String descricao_clima;
float id_torre;

void setup() {
  Serial.begin(115200);

  thing.add_wifi(SSID, SSID_PASSWORD);
  WiFi.begin(SSID, SSID_PASSWORD);
  Serial.println("Conectando...");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Quedas de raios
  thing["Queda do Raio"] >> outputValue(data_hora_raio); 
  thing["Tipo do Raio"] >> outputValue(tipo_raio); 
  thing["Amplitude Máxima"] >> outputValue(amplitude_raio); 
  thing["Latitude do Raio"] >> outputValue(latitude_raio); 
  thing["Longitude do Raio"] >> outputValue(longitude_raio);

  // Dados climáticos
  thing["Localização"] >> outputValue(timezone_clima); 
  thing["Medição do clima"] >> outputValue(data_hora_clima); 
  thing["Temperatura"] >> outputValue(temperatura_clima); 
  thing["Umidade"] >> outputValue(umidade_clima); 
  thing["Direção do Vento"] >> outputValue(direcao_vento);
  thing["Velocidade do Vento"] >> outputValue(velocidade_vento);
  thing["Descrição do Clima"] >> outputValue(descricao_clima);

  Serial.println("");
  Serial.print("Conectado à rede WiFi no endereço IP: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Sensores ajustados. Novos dados serão captados a cada cinco minutos.");
}


void loop() {
  // Envio a requisição HTTP

  if ((millis() - lastTime) > timerDelay) { // Verifica o timer de 10 segundos.
    // Verifica o status da conexão WiFi
    if(WiFi.status()== WL_CONNECTED){
      
      // Extraindo as informações da string JSON
      jsonBufferConditions = httpGETRequest(xweatherConditions);
      jsonBufferLightning = httpGETRequest(xweatherLightning);

      // Parse JSON response
      parseConditionsJSON(jsonBufferConditions);
      Serial.println("Dados enviados ao backend.");
      Serial.println();
      
      thing.handle();
      Serial.println("Dados enviados ao Dashboard.");

    }
    else {
      Serial.println("WiFi desconectado");
    }
    lastTime = millis();
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClientSecure client;
  HTTPClient http;
    
  // Dados do servidor
  client.setInsecure();
  http.begin(client, serverName);
  
  // Envia a requisição GET
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Código de erro: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();

  return payload;
}


String getToken(const char* endpoint) {

  WiFiClientSecure client;
  HTTPClient http;
    
  // Dados do servidor e requisição
  client.setInsecure();
  http.begin(client, endpoint);
  http.setTimeout(15000);  // Timeout aumentado

  
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  // Json payload
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["email"] = userEmail;
  jsonDoc["senha"] = userPassword;
  String requestBody;
  serializeJson(jsonDoc, requestBody);
  
  int httpResponseCode = http.POST(requestBody);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Response:");

    // Parse JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      if (doc.containsKey("token")) {
        jwtToken = doc["token"].as<String>();
        return jwtToken;
      } else {
        Serial.println("Error: Token not found in the response.");
      }
    } else {
      Serial.print("JSON Deserialization Error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("HTTP POST Request Failed. Error Code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }
    http.end();
    return "";

}

void parseConditionsJSON(const String& json) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("JSON Parsing Failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject response = doc["response"][0];
  JsonObject loc = response["loc"];
  JsonObject place = response["place"];
  JsonObject profile = response["profile"];
  JsonObject period = response["periods"][0];

  // Coordenadas
  float latitude = loc["lat"].as<float>();
  float longitude = loc["long"].as<float>();

  // Localização
  String cityName = place["name"].as<String>();
  String state = place["state"].as<String>();
  String country = place["country"].as<String>();

  // Dados climáticos
  String dateTimeISO = period["dateTimeISO"].as<String>();
  float tempC = period["tempC"].as<float>();
  float humidity = period["humidity"].as<float>();
  String windDir = period["windDir"].as<String>();
  float windSpeedKPH = period["windSpeedKPH"].as<float>();
  String weatherDesc = period["weather"].as<String>();

  // Timezone
  String timezone = profile["tz"].as<String>();

  // POST
  StaticJsonDocument<512> postDoc;
  postDoc["timezone"] = timezone + ": " + cityName + " - " + state + ", " + country;
  postDoc["data_hora"] = dateTimeISO;
  postDoc["temperatura"] = tempC;
  postDoc["umidade"] = humidity;
  postDoc["direcao_vento"] = windDir;
  postDoc["velocidade_vento"] = windSpeedKPH;
  postDoc["descricao_clima"] = weatherDesc;
  postDoc["idTorre"] = 1;

  // Thinger
  timezone_clima = timezone + ": " + cityName + " - " + state + ", " + country;
  data_hora_clima = dateTimeISO;
  temperatura_clima = tempC;
  umidade_clima = humidity;
  direcao_vento = windDir;
  velocidade_vento = windSpeedKPH;
  descricao_clima = weatherDesc;

  // String postPayload;
  // serializeJson(postDoc, postPayload);
  
  // postJSONData(postPayload, serverClima);

}

void postJSONData(const String& payload, const char* endpoint) {
  WiFiClientSecure client;
  client.setInsecure();
  String token;

  token = getToken(serverLogin);
  Serial.println(token);

  HTTPClient http;
  http.begin(client, endpoint);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", token);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.print("Dados enviados! Response Code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Mensagem da response:");
    Serial.println(response);
  } else {
    Serial.print("POST Request Failed. Error Code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }

  http.end();
}

void parseLightningJSON(const String& json) {

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (doc["error"]["code"]) {
    Serial.println("Dados não encontrados.");
    Serial.println("Gerando dados..");

    data_hora_raio = "2024-11-22T19:58:51+00:00";
    tipo_raio = "ic";
    amplitude_raio = 26000;
    latitude_raio = -7.7690457;
    longitude_raio = -40.6633014;
    
    StaticJsonDocument<512> postDoc;
    postDoc["data_hora"] = data_hora_raio;
    postDoc["tipo_raio"] = tipo_raio;
    postDoc["amplitude"] = amplitude_raio;
    postDoc["latitude"] = latitude_raio;
    postDoc["longitude"] = longitude_raio;
    postDoc["idTorre"] = 2;

    
    return;
  }

}