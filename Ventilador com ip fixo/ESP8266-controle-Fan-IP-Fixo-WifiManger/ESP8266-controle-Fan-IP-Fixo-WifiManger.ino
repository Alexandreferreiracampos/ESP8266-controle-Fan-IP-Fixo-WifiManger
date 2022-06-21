#include <FS.h>                           //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>                  //https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <WiFiManager.h>                  //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>                  //https://github.com/bblanchon/ArduinoJson

char static_ip[16] = "192.168.0.218";         //default custom static IP
char static_gw[16] = "192.168.0.1";
char static_sn[16] = "255.255.255.0";
bool shouldSaveConfig = false;            //flag for saving data

int ventilador = 2; 
int bt = 1;
boolean buttonPress = false;                      

ESP8266WebServer server(80);                                // Instancia server
//  Strings com HTML
String Q_1 = "<!DOCTYPE HTML><html><head><meta http-equiv='refresh' content='1;URL=/Controle'/></head><h1><center>AlexandreDev Automacao / Ventilador-Quarto</center>";
String Q_2 = "</p></center><h3><BR></h3><html>\r\n";
String Ql = "";                                             // Quarto ligado
String Qd = "";                                             // Quarto desligado
String Qil = "";
String Qid = "";
String FanQuarto;                                           // String para controle
//float temp_c;                                               // Variavel para temperatura
//int humidity;                                               // Variavel para umidade
//#define botReset 13                                         // Entrada botao reset do ssid e pw
//#define LedRst 5                                            // Led indicativo de AP mode
//#define Saida1 2                                            // GPIO2 Port para ligar Controle do triac (Pino 2 do MOC3023)
//#define Switch1 4                                           // GPIO4 Port para ligar o interruptor
//byte Switch1_atual = 0;                                     // Variavel para staus de GPIO5 (Status do interruptor)
//unsigned long previousMillis = 0;                           // Variavel para medir periodos
//const long interval = 2000;                                 // Periodo de leitura da temperatura e umidade
//---------------------------------------
/*void gettemperature()                                       // Funcoa para ler temperatura e umidade
{
  if (millis() - previousMillis >= interval)                // Se passou o intervalo
  {
    previousMillis = millis();                              // Restaura valor de previousMillis
    humidity = sht1x.readHumidity();                        // Le umidade em SHT10
    temp_c = sht1x.readTemperatureC();                      // Le temperatura em SHT10
  }
}*/
//-------------------------------
void saveConfigCallback ()                //callback notifying us of the need to save config
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
//-------------------------------
void setup()
{
  Ql += Q_1;                                                // Monta tela pra informar que a luz
  Ql += "<p><center>Luz</p><p><a href=\"/Controle?FanQuarto=on \"><button style=\"background-color: rgb(0, 255,   0);height: 100px; width: 200px;\"><h1>Ventilador</h1></button></a>";
  Qd += Q_1;                                                // Monta tela pra informar que a luz
  //Qd += "<p><center>Luz</p><p><a href=\"/Controle?LuzQuarto=on \"><button style=\"background-color: rgb(0, 255,   0);height: 100px; width: 200px;\"><h1>Ventilador Desligado</h1></button></a>";

pinMode(ventilador, OUTPUT);
pinMode(bt, INPUT_PULLUP);
digitalWrite(ventilador, LOW);
//pinMode(botReset, INPUT_PULLUP);                          // botReset como entrada e pullup
//pinMode(LedRst, OUTPUT);                                  // LedRst como saida
//digitalWrite(LedRst, LOW);                                // Apaga o LED
//pinMode(Switch1, INPUT_PULLUP);                           // Switch1 como entrada e liga o resistor de pullup
// Switch1_atual = digitalRead(Switch1);                     // Atualisa status de GPIO5
//pinMode(Saida1, OUTPUT);                                  // Saida1 como saida
// digitalWrite(Saida1, LOW);                                // Liga saída

  //SPIFFS.format();                                        //clean FS, for testing
               //read configuration from FS json
  if (SPIFFS.begin())
  {
  
    if (SPIFFS.exists("/config.json"))                      // if file exists, reading and loading
    {
      
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);        // Allocate a buffer to store contents of the file.
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          
          if (json["ip"])
          {
            
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
           
          }
          
        }
        
      }
    }
  }
  
 // Serial.println(static_ip);                                  // end read
  WiFiManager wifiManager;

 /* if (digitalRead(botReset) == LOW)                           // Se reset foi pressionado
  {
    delay(3000);                                              // Aguarda 3 minutos segundos
    if (digitalRead(botReset) == LOW)                         // Se reset continua pressionado
    {
      wifiManager.resetSettings();                            // Reseta SSID e PW
    }
  }
  digitalWrite(LedRst, HIGH);                                 // Acende o Led indicativo de AP
*/
  wifiManager.setSaveConfigCallback(saveConfigCallback);      //set config save notify callback
  IPAddress _ip, _gw, _sn;                                    //set static ip
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  // wifiManager.resetSettings();                               //reset settings - for testing

  wifiManager.setMinimumSignalQuality();                        //set minimu quality of signal so it ignores AP's under that quality    //defaults to 8%
  if (!wifiManager.autoConnect("AlexandreDev-Ventilador-Quarto", "91906245"))
  {
   // Serial.println("failed to connect and hit timeout");
    delay(3000);

    ESP.reset();                                                //reset and try again, or maybe put it to deep sleep
    delay(5000);
  }
//  Serial.println("connected...yeey :)");                        //if you get here you have connected to the WiFi
  if (shouldSaveConfig)                                         //save the custom parameters to FS
  {
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
     // Serial.println("failed to open config file for writing");
    }
    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();    //end save
  }
  //Serial.println("local ip");
  //Serial.println(WiFi.localIP());
  //Serial.println(WiFi.gatewayIP());
 // Serial.println(WiFi.subnetMask());

  server.on("/", []()                                       // Ao request
  {
    server.send(200, "text/html", Ql);                      // Executa o HTML Ql (Quarto ligado)
  });
  server.on("/status",[](){
    if (digitalRead(ventilador) == HIGH)                        // Se a saida esta ligada, carrega a pagina "ligada"
    {
      server.send(200, "text/json", "{\"status\": \"Ligado\"}");                                    // Limpa valor de temperatura e umidade
    }
    if (digitalRead(ventilador) == LOW)                         // Se a saida esta desligada, carrega a pagina "desligada"
    {
     server.send(200, "text/json", "{\"status\": \"Desligado\"}");                                         // Limpa valor de temperatura e umidade
    }
    
    
  })
  server.on("/Controle", []()                               // Ao requeste
  {
    //    gettemperature();                                     // Le temperatura e umidade. Comentar se tiver sem sensor pois fica lento
    FanQuarto = server.arg("FanQuarto");                    // Recupera o valor do parametro luz enviado
    if ( FanQuarto == "on") digitalWrite(ventilador, !digitalRead(ventilador));      // Se o valor de luz e off desliga a saida
          // Se o valor de luz e on liga a saida

    if (digitalRead(ventilador) == HIGH)                        // Se a saida esta ligada, carrega a pagina "ligada"
    {
      Qil += Ql;                                            // Monta tela nova quarto ligado
      Qil +=  "<p><center>Ventilador Ligado</p>";
      server.send(200, "text/html", Qil);                   // Mostra Quarto ligado
      Qil = "";                                             // Limpa valor de temperatura e umidade
    }
    if (digitalRead(ventilador) == LOW)                         // Se a saida esta desligada, carrega a pagina "desligada"
    {
      Qid += Qd;                                            // Monta tela nova quarto desligado
      Qid += "<p><center>Ventilador Desligado</p>";
      server.send(200, "text/html", Qid);                   // Mostra Quarto desligado
      Qid = "";                                             // Limpa valor de temperatura e umidade
    }
    delay(100);                                             // Delay
  });
  server.begin();                                           // Inicaliza servidor
  //Serial.println("HTTP server started");                    // Imprime
}
//-------------------------------
void loop()
{
  WiFiManager wifiManager;
  server.handleClient();   
  
  digitalRead(bt) == HIGH;
   if(digitalRead(bt) == LOW && buttonPress == false){
    delay(50);
      if(digitalRead(bt) == LOW){
        digitalWrite(ventilador, !digitalRead(ventilador));
        buttonPress = true;
        delay(1000);
        buttonPress = false;
        }
      }

      if (digitalRead(bt) == LOW)                           // Se reset foi pressionado
     {
    delay(5000);                                              // Aguarda 3 minutos segundos
    if (digitalRead(bt) == LOW)                         // Se reset continua pressionado
    {
      wifiManager.resetSettings();                            // Reseta SSID e PW
    }      
     
  }// Executa instancia
  /*if (digitalRead(Switch1) !=  Switch1_atual)               // Se o valor do SW alterou
  {
    delay(40);                                              // Delay
    if (digitalRead(Switch1) !=  Switch1_atual)             // Se o valor do SW alterou
    {
      digitalWrite(Saida1, !digitalRead(Saida1));           // Inverte a saida lamp1
      Switch1_atual = digitalRead(Switch1);                 // Atualisa o Gpio5 atual
    }
  }*/
}


//http://192.168.0.106/Controle?FanQuarto=on