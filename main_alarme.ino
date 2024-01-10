#include <Arduino.h>
#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <ArduinoJson.h>

#define TipoRTC 1 //Se for usar RTC_DS1307 colocar 1, se for usar o DS3132 coloca 0
#define LeitorTemp 0 //Se for usar um thermistor colocar 1, se for usar o DS18B20 no D5, colocar 0
#define NovaPlaca 0 // Se a placa for nova recebe 1 (true) e depois de reinicializar muda pra 0 e grava novamente

#if TipoRTC == 1
  RTC_DS1307 rtc; //Objeto rtc da classe DS1307
  #define deviceaddress 0x50 //EEPROM 32k
#else
  RTC_DS3231 rtc; //Objeto rtc da classe DS3132
  #define deviceaddress 0x57 //EEPROM 32k
#endif

#if LeitorTemp == 1
  #include <Thermistor.h>
  #define ThermNTC A1
  Thermistor temp(ThermNTC);
#else
  #include <OneWire.h> //BIBLIOTECA NECESSÁRIA PARA O DS18B20
  #include <DallasTemperature.h> //BIBLIOTECA NECESSÁRIA PARA O DS18B20
  #define DS18B20 5
  OneWire ourWire(DS18B20);//CONFIGURA UMA INSTÂNCIA ONEWIRE PARA SE COMUNICAR COM DS18B20
  DallasTemperature sensors(&ourWire);
#endif

//Declaração da porta de comunicação do bluethoth e setando-o
#define txdpin 6
#define rxdpin 7
SoftwareSerial bluetooth(txdpin, rxdpin);

byte mes, dia, hora, minuto, segundo, diasemana, temperatura;
int ano;
byte novominuto = 0;

//Setando as portas dos reles
#define pinRele1 0
#define pinRele2 1
#define pinRele3 2
#define pinRele4 3

//Pinos 74HC595
#define pinSH_CP 4  //Pino Clock
#define pinST_CP 3  //Pino Latch
#define pinDS 2     //Pino Data
#define qtdeCI 1    //Quantidade de CI

//Variáveis Endereo EEPROM do alarme
//Quantidade de alarmes que podem ser configurados
#define quantAlarms 4
//Reservando endereços EEPROM
int alarmeEEPROM1 = 0, alarmeEEPROM2 = 1, alarmeEEPROM3 = 2, alarmeEEPROM4 = 3;
int HoraEEPROM_1A = 10, MinutoEEPROM_1A = 11, HoraEEPROM_1D = 12, MinutoEEPROM_1D = 13;
int HoraEEPROM_2A = 14, MinutoEEPROM_2A = 15, HoraEEPROM_2D = 16, MinutoEEPROM_2D = 17;
int HoraEEPROM_3A = 18, MinutoEEPROM_3A = 19, HoraEEPROM_3D = 20, MinutoEEPROM_3D = 21;
int HoraEEPROM_4A = 22, MinutoEEPROM_4A = 23, HoraEEPROM_4D = 24, MinutoEEPROM_4D = 25;
//Função 74HC595
void ciWrite(byte pino, bool estado) {
  static byte ciBuffer[qtdeCI];
  bitWrite(ciBuffer[pino / 8], pino % 8, estado);
  digitalWrite(pinST_CP, LOW);  //Inicia a Transmissão
  digitalWrite(pinDS, LOW);     //Apaga Tudo para Preparar Transmissão
  digitalWrite(pinSH_CP, LOW);
  for (int nC = qtdeCI - 1; nC >= 0; nC--) {
    for (int nB = 7; nB >= 0; nB--) {
      digitalWrite(pinSH_CP, LOW);                     //Baixa o Clock
      digitalWrite(pinDS, bitRead(ciBuffer[nC], nB));  //Escreve o BIT
      digitalWrite(pinSH_CP, HIGH);                    //Eleva o Clock
      digitalWrite(pinDS, LOW);                        //Baixa o Data para Previnir Vazamento
    }
  }
  digitalWrite(pinST_CP, HIGH);  //Finaliza a Transmissão
}
void writeEEPROM(unsigned int eeaddress, byte data) {
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));    // Byte mais significativo
  Wire.write((int)(eeaddress & 0xFF));  // Byte menos significativo
  Wire.write(data);
  Wire.endTransmission();
  delay(5);
}
byte readEEPROM(unsigned int eeaddress) {
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));
  Wire.write((int)(eeaddress & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}
int statusRele[quantAlarms] = { 100, 101, 102, 103 };
int portas[] = { pinRele1, pinRele2, pinRele3, pinRele4 };
//Declara as portas do Time e inicializa o DS3231
void enviaDadosHora(uint32_t epoch) {
  rtc.adjust(DateTime(epoch));
}
void enviaDadosHora(byte e_hora, byte e_minuto, byte e_segundo, byte e_dia, byte e_mes, byte e_ano){
  rtc.adjust(DateTime(e_ano, e_mes, e_dia, e_hora, e_minuto, e_segundo));
}
//Funções para ativação dos Reles
void alteraRele(int pinRele, int estado) {
  pinRele = pinRele-1;
  writeEEPROM(pinRele, 0); //Status do alarme
  writeEEPROM(statusRele[pinRele], estado); //Estado atual do rele
  writeEEPROM(statusRele[pinRele]+50, estado); //index de comparador da função alarmar
  ciWrite(pinRele, estado);
}
void pegaDadosRelogio() {
  #if LeitorTemp == 1
    temperatura = temp.getTemp(); //Temperatura da biblioteca thermistor
  #else
    sensors.requestTemperatures();//REQUISITA A TEMPERATURA DO SENSOR
    temperatura = sensors.getTempCByIndex(0); //recebe temperatura do DS18B20 da biblioteca DallasTemperature
  #endif
  DateTime agora = rtc.now();
  segundo = agora.second();          //recebe segundo
  minuto = agora.minute();           //recebe minuto
  hora = agora.hour();               //recebe hora
  dia = agora.day();                 //recebe dia
  mes = agora.month();               //recebe mês
  ano = agora.year();                //recebe ano
  diasemana = agora.dayOfTheWeek();  //recebe o dia da semana em numeral
}
String printDigit(int digito) {
  String digitos = String(digito);
  if (digitos.length() == 1) {
    digitos = "0" + digitos;
  }
  return (digitos);
}
String concatena() {
  String conteudo = "";
  char leitura;
  while (Serial.available() > 0 || bluetooth.available() > 0) {
    //leitura = Serial.read();
    if (bluetooth.available())
      leitura = bluetooth.read();
    else
      leitura = Serial.read();

    if (leitura != '\n') {
      conteudo.concat(leitura);
    }
    delay(15);
  }
  return conteudo;
}
/*void enviaComando() {
  pegaDadosRelogio();
  DynamicJsonDocument out(256);
  out["t"] = temperatura;
  out["h"] = printDigit(hora) + ":" + printDigit(minuto);
  out["d"] = printDigit(dia) + "/" + printDigit(mes) + "/" + ano;
  out["r1"] = readEEPROM(statusRele[0]);
  out["r2"] = readEEPROM(statusRele[1]);
  out["r3"] = readEEPROM(statusRele[2]);
  out["r4"] = readEEPROM(statusRele[3]);

  out["a1"] = readEEPROM(alarmeEEPROM1);
  out["a2"] = readEEPROM(alarmeEEPROM2);
  out["a3"] = readEEPROM(alarmeEEPROM3);
  out["a4"] = readEEPROM(alarmeEEPROM4);

  out["h1"] = printDigit(readEEPROM(HoraEEPROM_1A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_1A));
  out["l1"] = printDigit(readEEPROM(HoraEEPROM_1D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_1D));

  out["h2"] = printDigit(readEEPROM(HoraEEPROM_2A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_2A));
  out["l2"] = printDigit(readEEPROM(HoraEEPROM_2D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_2D));

  out["h3"] = printDigit(readEEPROM(HoraEEPROM_3A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_3A));
  out["l3"] = printDigit(readEEPROM(HoraEEPROM_3D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_3D));

  out["h4"] = printDigit(readEEPROM(HoraEEPROM_4A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_4A));
  out["l4"] = printDigit(readEEPROM(HoraEEPROM_4D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_4D));

  String output;
  serializeJson(out, output);
  Serial.println(output);
  bluetooth.println(output);
}*/
void enviaComando(bool concatena){
  pegaDadosRelogio();
      String envia;
      envia += "{\"t\":";
      envia += printDigit(temperatura);
      envia += ",\"h\":\"";
      envia += printDigit(hora);
      envia += ":";
      envia += printDigit(minuto);
      envia += "\",\"d\":\"";
      envia += printDigit(dia);
      envia += "/";
      envia += printDigit(mes);
      envia += "/";
      envia += ano;
      envia += "\",\"r1\":";
      envia += readEEPROM(statusRele[0]);
      envia += ",\"r2\":";
      envia += readEEPROM(statusRele[1]);
      envia += ",\"r3\":";
      envia += readEEPROM(statusRele[2]);
      envia += ",\"r4\":";
      envia += readEEPROM(statusRele[3]);
      if (concatena == true) {
          envia += ",\"a1\":";
          envia += String(readEEPROM(alarmeEEPROM1));
          envia += ",\"a2\":";
          envia += String(readEEPROM(alarmeEEPROM2));
          envia += ",\"a3\":";
          envia += String(readEEPROM(alarmeEEPROM3));
          envia += ",\"a4\":";
          envia += String(readEEPROM(alarmeEEPROM4));
          envia += ",\"h1\":\"";
          
          envia += printDigit(readEEPROM(HoraEEPROM_1A));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_1A));
          envia += "\",\"l1\":\"";
          envia += printDigit(readEEPROM(HoraEEPROM_1D));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_1D));
          envia += "\",\"h2\":\"";

          envia += printDigit(readEEPROM(HoraEEPROM_2A));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_2A));
          envia += "\",\"l2\":\"";
          envia += printDigit(readEEPROM(HoraEEPROM_2D));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_2D));
          envia += "\",\"h3\":\"";

          envia += printDigit(readEEPROM(HoraEEPROM_3A));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_3A));
          envia += "\",\"l3\":\"";
          envia += printDigit(readEEPROM(HoraEEPROM_3D));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_3D));
          envia += "\",\"h4\":\"";

          envia += printDigit(readEEPROM(HoraEEPROM_4A));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_4A));
          envia += "\",\"l4\":\"";
          envia += printDigit(readEEPROM(HoraEEPROM_4D));
          envia += ":";
          envia += printDigit(readEEPROM(MinutoEEPROM_4D));
          envia += "\"}";

          concatena =false;
      }else{
        envia += "}";
      }
      bluetooth.println(envia);
      Serial.println(envia);
      envia = "";
}
void novaPlaca() {
  for (int i = 0; i < 200; i++) {
    writeEEPROM(i, 0);
  }
}
void alarmar(int pinRele, int alarmeInicial, int alarmeFinal){
  int horaAgora = (hora*100)+minuto;
  int indexComparador = statusRele[pinRele]+50; //Index de comparador para não ficar gravando na eeproom sem necessidade a cada minuto
  if (readEEPROM(pinRele) == 1 && alarmeInicial != alarmeFinal) { //Se o alarme estiver ativado e hora inicial for diferente de hora final, executa.
    Serial.print("Status do rele ");
    Serial.print(pinRele+1);
    if(alarmeInicial < alarmeFinal){ //Se a hora de ligar for menor que a hora de desligar
      if(alarmeInicial <= horaAgora && alarmeFinal > horaAgora){ //Se a hora atual for maior ou igual hora do alarme
        if(readEEPROM(indexComparador)==0){
          writeEEPROM(statusRele[pinRele], 1);
          writeEEPROM(indexComparador, 1);
          ciWrite(pinRele, HIGH); 
        }
        Serial.println(": Ligado");
      }else{
        if(readEEPROM(indexComparador)==1){
          writeEEPROM(statusRele[pinRele], 0);
          writeEEPROM(indexComparador, 0);
          ciWrite(pinRele, LOW);
        }
        Serial.println(": Desligado");
      }
    }else{ //Se a hora de ligar for maior que a hora de desligar
      if(alarmeInicial <= horaAgora || alarmeFinal > horaAgora){
        if(readEEPROM(indexComparador)==0){
          writeEEPROM(statusRele[pinRele], 1);
          writeEEPROM(indexComparador, 1);
          ciWrite(pinRele, HIGH);
        }
        Serial.println(": Ligado");
      }else{
        if(readEEPROM(indexComparador)==1){
          writeEEPROM(statusRele[pinRele], 0);
          writeEEPROM(indexComparador, 0);
          ciWrite(pinRele, LOW);
        }
        Serial.println(": Desligado");
      }
    }
  }
}
void setup() {
  delay(3000);  //caso precise reistalar o skatch, ele da tempo carregar o programa antes de carregar o watch dog
  //inicializa as bibliotecas
  wdt_enable(WDTO_2S);
  bluetooth.begin(9600);
  Wire.begin();
  Serial.begin(9600);
  rtc.begin();
  #if LeitorTemp == 0
    sensors.begin();
  #endif

  pegaDadosRelogio();
  novominuto = minuto;
  //Seta todas as portas declaradas como portas de saída
  pinMode(pinSH_CP, OUTPUT);
  pinMode(pinST_CP, OUTPUT);
  pinMode(pinDS, OUTPUT);
  for (int nL = 0; nL < quantAlarms; nL++) {
    ciWrite(portas[nL], readEEPROM(statusRele[nL]));
  }
  #if NovaPlaca == 1
    novaPlaca();
  #endif
  delay(100);
}

void loop() {
  wdt_reset();  //watch dog
  pegaDadosRelogio();
  if (Serial.available() > 0 || bluetooth.available() > 0) {
    String Comando = concatena();
    DynamicJsonDocument doc(256);
    deserializeJson(doc, Comando);
    String entrada = doc["ent"];

    if (entrada == "st") { //{"ent":"st","a":1,"s":1}
      int valrele = doc["a"];
      if (doc["s"] == 1) {
        switch (valrele) {
          case 1:
            writeEEPROM(alarmeEEPROM1, 1);
            break;
          case 2:
            writeEEPROM(alarmeEEPROM2, 1);
            break;
          case 3:
            writeEEPROM(alarmeEEPROM3, 1);
            break;
          case 4:
            writeEEPROM(alarmeEEPROM4, 1);
            break;
        }
      } else {
        switch (valrele) {
          case 1:
            writeEEPROM(alarmeEEPROM1, 0);
            break;
          case 2:
            writeEEPROM(alarmeEEPROM2, 0);
            break;
          case 3:
            writeEEPROM(alarmeEEPROM3, 0);
            break;
          case 4:
            writeEEPROM(alarmeEEPROM4, 0);
            break;
        }
      }
    }
    if (entrada == "dd") { //{"ent":"dd"}
      enviaComando(true);
    }
    if (entrada == "hr") { //{"ent":"hr","h":1695105003}
      uint32_t epoch = doc["h"];
      rtc.adjust(DateTime(epoch));
      enviaComando(false);
    }
    if (entrada == "dt") { //{"ent":"dt","h":3,"m":31,"s":0,"d":17,"m":8,"a":2023}
      rtc.adjust(DateTime(doc["a"], doc["m"], doc["d"], doc["h"], doc["m"], doc["s"]));
      enviaComando(false);
    }
    if (entrada == "al") { //{"ent":"al","r":1,"ha":21,"ma":15,"hd":22,"md":15}
      byte horaAtiva = doc["ha"];
      byte minutoAtiva = doc["ma"];
      byte horaDesativa = doc["hd"];
      byte minutoDesativa = doc["md"];
      byte relval = doc["r"];
      switch (relval) {
      case 1:
        writeEEPROM(HoraEEPROM_1A, horaAtiva);
        writeEEPROM(MinutoEEPROM_1A, minutoAtiva);
        writeEEPROM(HoraEEPROM_1D, horaDesativa);
        writeEEPROM(MinutoEEPROM_1D, minutoDesativa);
      break;
      case 2:
        writeEEPROM(HoraEEPROM_2A, horaAtiva);
        writeEEPROM(MinutoEEPROM_2A, minutoAtiva);
        writeEEPROM(HoraEEPROM_2D, horaDesativa);
        writeEEPROM(MinutoEEPROM_2D, minutoDesativa);
      break;
      case 3:
        writeEEPROM(HoraEEPROM_3A, horaAtiva);
        writeEEPROM(MinutoEEPROM_3A, minutoAtiva);
        writeEEPROM(HoraEEPROM_3D, horaDesativa);
        writeEEPROM(MinutoEEPROM_3D, minutoDesativa);
      break;
      case 4:
        writeEEPROM(HoraEEPROM_4A, horaAtiva);
        writeEEPROM(MinutoEEPROM_4A, minutoAtiva);
        writeEEPROM(HoraEEPROM_4D, horaDesativa);
        writeEEPROM(MinutoEEPROM_4D, minutoDesativa);
      break;
      }
    }
    if (entrada == "di") { //{"ent":"di","r":2,"s":1}
        alteraRele(doc["r"], doc["s"]);
    }
  }
  if (novominuto != minuto) {

    int s1a = (readEEPROM(HoraEEPROM_1A)*100)+readEEPROM(MinutoEEPROM_1A);
    int s1d = (readEEPROM(HoraEEPROM_1D)*100)+readEEPROM(MinutoEEPROM_1D);

    int s2a = (readEEPROM(HoraEEPROM_2A)*100)+readEEPROM(MinutoEEPROM_2A);
    int s2d = (readEEPROM(HoraEEPROM_2D)*100)+readEEPROM(MinutoEEPROM_2D);

    int s3a = (readEEPROM(HoraEEPROM_3A)*100)+readEEPROM(MinutoEEPROM_3A);
    int s3d = (readEEPROM(HoraEEPROM_3D)*100)+readEEPROM(MinutoEEPROM_3D);

    int s4a = (readEEPROM(HoraEEPROM_4A)*100)+readEEPROM(MinutoEEPROM_4A);
    int s4d = (readEEPROM(HoraEEPROM_4D)*100)+readEEPROM(MinutoEEPROM_4D);

    alarmar(pinRele1, s1a, s1d);
    alarmar(pinRele2, s2a, s2d);
    alarmar(pinRele3, s3a, s3d);
    alarmar(pinRele4, s4a, s4d);

    enviaComando(false);
    novominuto = minuto;
  }
}
