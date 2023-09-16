#include <Arduino.h>
#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <ArduinoJson.h>
/*
//deve descomentar a linha 1003, 222 e 223
#include <OneWire.h> //BIBLIOTECA NECESSÁRIA PARA O DS18B20
#include <DallasTemperature.h> //BIBLIOTECA NECESSÁRIA PARA O DS18B20
#define DS18B20 5
OneWire ourWire(DS18B20);//CONFIGURA UMA INSTÂNCIA ONEWIRE PARA SE COMUNICAR COM DS18B20
DallasTemperature sensors(&ourWire);
*/

//Declaração da porta de comunicação do bluethoth e setando-o
#define txdpin 6
#define rxdpin 7
SoftwareSerial bluetooth(txdpin, rxdpin);

//As linhas de codigo a seguir devem ser comentadas, ou descomentadas, de acordo com o modelo de RTC utilizado (DS1307 ou DS3132)
//RTC_DS1307 rtc; //Objeto rtc da classe DS1307
RTC_DS3231 rtc;  //Objeto rtc da classe DS3132

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

//Funções de escrita na EEPROM 32k
#define deviceaddress 0x57
//#define deviceaddress 0x50
//Variáveis Endereo EEPROM do alarme
//Quantidade de alarmes que podem ser configurados
#define quantAlarms 4
//Reservando endereços EEPOROM
int alarmeEEPROM1 = 0, alarmeEEPROM2 = 1, alarmeEEPROM3 = 2, alarmeEEPROM4 = 3, alarmeEEPROM5 = 4, alarmeEEPROM6 = 5, alarmeEEPROM7 = 6, alarmeEEPROM8 = 7;
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
void enviaDadosData(byte e_dia, byte e_mes, int e_ano) {
  rtc.adjust(DateTime(e_ano, e_mes, e_dia, hora, minuto, segundo));
}
void enviaDadosHora(byte e_hora, byte e_minuto, byte e_segundo) {
  rtc.adjust(DateTime(ano, mes, dia, e_hora, e_minuto, e_segundo));
}
//Funções para ativação dos Reles
void alteraRele(int pinRele, int estado, int statusAlarme, int StatusRele) {
  writeEEPROM(statusAlarme, 0);
  writeEEPROM(StatusRele, estado);
  ciWrite(pinRele1, estado);
}
void pegaDadosRelogio() {
  temperatura = rtc.getTemperature();  //temperadura do ds3231

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
void enviaComando() {
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

  out["1a"] = printDigit(readEEPROM(HoraEEPROM_1A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_1A));
  out["1d"] = printDigit(readEEPROM(HoraEEPROM_1D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_1D));

  out["2a"] = printDigit(readEEPROM(HoraEEPROM_2A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_2A));
  out["2d"] = printDigit(readEEPROM(HoraEEPROM_2D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_2D));

  out["3a"] = printDigit(readEEPROM(HoraEEPROM_3A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_3A));
  out["3d"] = printDigit(readEEPROM(HoraEEPROM_3D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_3D));

  out["4a"] = printDigit(readEEPROM(HoraEEPROM_4A)) + ":" + printDigit(readEEPROM(MinutoEEPROM_4A));
  out["4d"] = printDigit(readEEPROM(HoraEEPROM_4D)) + ":" + printDigit(readEEPROM(MinutoEEPROM_4D));

  String output;
  serializeJson(out, output);

  Serial.println(output);
  bluetooth.println(output);
}
void novaPlaca() {
  for (int i = 0; i < 200; i++) {
    writeEEPROM(i, 0);
  }
}
void alarmar(int StatusAlarme, int StatusRele, int pinRele, int alarmeInicial, int alarmeFinal, int horaAgora){
  if (readEEPROM(StatusAlarme) == 1 && alarmeInicial != alarmeFinal) { //Se o alarme estiver ativado e hora inicial for diferente de hora final, executa.
    if(alarmeInicial < alarmeFinal){ //Se a hora de ligar for menor que a hora de desligar
      if(alarmeInicial <= horaAgora && alarmeFinal > horaAgora){ //Se a hora atual for maior ou igual hora do alarme
        writeEEPROM(StatusRele, 1);
        ciWrite(pinRele, HIGH);
      }else{
        writeEEPROM(StatusRele, 0);
        ciWrite(pinRele, LOW);
      }
    }else{ //Se a hora de ligar for maior que a hora de desligar
      if(alarmeInicial <= horaAgora || alarmeFinal > horaAgora){
        writeEEPROM(StatusRele, 1);
        ciWrite(pinRele, HIGH);
      }else{
        writeEEPROM(StatusRele, 0);
        ciWrite(pinRele, LOW);
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

  pegaDadosRelogio();
  novominuto = minuto;
  //Seta todas as portas declaradas como portas de saída
  pinMode(pinSH_CP, OUTPUT);
  pinMode(pinST_CP, OUTPUT);
  pinMode(pinDS, OUTPUT);
  for (int nL = 0; nL < quantAlarms; nL++) {
    ciWrite(portas[nL], readEEPROM(statusRele[nL]));
  }
  //novaPlaca();
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

    if (entrada == "st") {
      int valrele = doc["r"];
      if (doc["df"] == 1) {
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
    if (entrada == "dd") {
      enviaComando();
    }
    if (entrada == "hr") {
      enviaDadosHora(doc["h"], doc["m"], 0);
    }
    if (entrada == "dt") {
      enviaDadosData(doc["d"], doc["m"], doc["a"]);
    }
    if (entrada == "al") {
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
    if (entrada == "di") {
      byte str = doc["r"];
      byte estado = doc["s"];
      switch (str) {
      case 1:
        alteraRele(1, estado, alarmeEEPROM1, statusRele[0]);
      break;
      case 2:
        alteraRele(1, estado, alarmeEEPROM2, statusRele[1]);
      break;
      case 3:
        alteraRele(1, estado, alarmeEEPROM3, statusRele[2]);
      break;
      case 4:
        alteraRele(1, estado, alarmeEEPROM4, statusRele[3]);
      break;
      }
    }
    Serial.println(Comando);
  }
  if (novominuto != minuto) {
    enviaComando();
    novominuto = minuto;

    int s1a = (readEEPROM(HoraEEPROM_1A)*100)+readEEPROM(MinutoEEPROM_1A);
    int s1d = (readEEPROM(HoraEEPROM_1D)*100)+readEEPROM(MinutoEEPROM_1D);

    int s2a = (readEEPROM(HoraEEPROM_2A)*100)+readEEPROM(MinutoEEPROM_2A);
    int s2d = (readEEPROM(HoraEEPROM_2D)*100)+readEEPROM(MinutoEEPROM_2D);

    int s3a = (readEEPROM(HoraEEPROM_3A)*100)+readEEPROM(MinutoEEPROM_3A);
    int s3d = (readEEPROM(HoraEEPROM_3D)*100)+readEEPROM(MinutoEEPROM_3D);

    int s4a = (readEEPROM(HoraEEPROM_4A)*100)+readEEPROM(MinutoEEPROM_4A);
    int s4d = (readEEPROM(HoraEEPROM_4D)*100)+readEEPROM(MinutoEEPROM_4D);
    int horaAtual = (hora*100)+minuto;

    alarmar(alarmeEEPROM1, statusRele[0], pinRele1, s1a, s1d, horaAtual);
  }
}
