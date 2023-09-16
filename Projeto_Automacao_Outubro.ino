/*

SISTEMA DE ALARMES UTILIZANDO O TIMEALARMS - UTILIZA MAIS MEMORIA E SÓ ATIVA NA HORA CERTA

*/
#include <Arduino.h>
#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
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
int ano, temporizador = 0;
byte novominuto = 0, novosegundo = 0;

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
AlarmId id1, id2, id3, id4, id5, id6, id7, id8;
void enviaDadosData(byte e_dia, byte e_mes, int e_ano) {
  rtc.adjust(DateTime(e_ano, e_mes, e_dia, hora, minuto, segundo));
}
void enviaDadosHora(byte e_hora, byte e_minuto, byte e_segundo) {
  rtc.adjust(DateTime(ano, mes, dia, e_hora, e_minuto, e_segundo));
}
//Funções para ativação dos Reles
void ativaRele1() {
  if (readEEPROM(alarmeEEPROM1) == 1) {
  writeEEPROM(statusRele[0], 1);
  ciWrite(pinRele1, HIGH);
  }
}
void desativaRele1() {
  if (readEEPROM(alarmeEEPROM1) == 1) {
  writeEEPROM(statusRele[0], 0);
  ciWrite(pinRele1, LOW);
  }
}
void ativaRele2() {
  if (readEEPROM(alarmeEEPROM2) == 1) {
  writeEEPROM(statusRele[1], 1);
  ciWrite(pinRele2, HIGH);
  }
}
void desativaRele2() {
  if (readEEPROM(alarmeEEPROM2) == 1) {
  writeEEPROM(statusRele[1], 0);
  ciWrite(pinRele2, LOW);
  }
}
void ativaRele3() {
  if (readEEPROM(alarmeEEPROM3) == 1) {
  writeEEPROM(statusRele[2], 1);
  ciWrite(pinRele3, HIGH);
  }
}
void desativaRele3() {
  if (readEEPROM(alarmeEEPROM3) == 1) {
  writeEEPROM(statusRele[2], 0);
  ciWrite(pinRele3, LOW);
  }
}
void ativaRele4() {
  if (readEEPROM(alarmeEEPROM4) == 1) {
  writeEEPROM(statusRele[3], 1);
  ciWrite(pinRele4, HIGH);
  }
}
void desativaRele4() {
  if (readEEPROM(alarmeEEPROM4) == 1) {
  writeEEPROM(statusRele[3], 0);
  ciWrite(pinRele4, LOW);
  }
}
//Funçào para verificar alarmes ativados no sistema
void inicializaAlarmes() {
  id1 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_1A), readEEPROM(MinutoEEPROM_1A), 0, ativaRele1);
  id5 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_1D), readEEPROM(HoraEEPROM_1D), 0, desativaRele1);
  id2 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_2A), readEEPROM(MinutoEEPROM_2A), 0, ativaRele2);
  id6 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_2D), readEEPROM(HoraEEPROM_2D), 0, desativaRele2);
  id3 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_3A), readEEPROM(MinutoEEPROM_3A), 0, ativaRele3);
  id7 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_3D), readEEPROM(HoraEEPROM_3D), 0, desativaRele3);
  id4 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_4A), readEEPROM(MinutoEEPROM_4A), 0, ativaRele4);
  id8 = Alarm.alarmRepeat(readEEPROM(HoraEEPROM_4D), readEEPROM(HoraEEPROM_4D), 0, desativaRele4);
}

String comando;
byte diasemananterior = 0;
void pegaDadosRelogio() {
  //temperatura = temp.getTemp(); //temperatura do ntc
  temperatura = rtc.getTemperature();  //temperadura do ds3231

  DateTime agora = rtc.now();
  segundo = agora.second();          //recebe segundo
  minuto = agora.minute();           //recebe minuto
  hora = agora.hour();               //recebe hora
  dia = agora.day();                 //recebe dia
  mes = agora.month();               //recebe mês
  ano = agora.year();                //recebe ano
  diasemana = agora.dayOfTheWeek();  //recebe o dia da semana em numeral
  if (diasemananterior != diasemana) {
    setTime(hora, minuto, segundo, mes, dia, ano);  //Grava na bibloteca timealarms a hora atual uma vez por dia
    diasemananterior = diasemana;
  }
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
void setup() {
  delay(3000);  //caso precise reistalar o skatch, ele da tempo carregar o programa antes de carregar o watch dog
  //inicializa as bibliotecas
  wdt_enable(WDTO_2S);
  bluetooth.begin(9600);
  Wire.begin();
  Serial.begin(9600);
  rtc.begin();

  pegaDadosRelogio();
  setTime(hora, minuto, segundo, mes, dia, ano);
  diasemananterior = diasemana;
  novominuto = minuto;
  //Seta todas as portas declaradas como portas de saída
  pinMode(pinSH_CP, OUTPUT);
  pinMode(pinST_CP, OUTPUT);
  pinMode(pinDS, OUTPUT);
  for (int nL = 0; nL < quantAlarms; nL++) {
    ciWrite(portas[nL], readEEPROM(statusRele[nL]));
  }
  //novaPlaca();
  //carrega a verificação dos alarmes ativados
  inicializaAlarmes();
  delay(100);
}

void loop() {
  wdt_reset();  //watch dog
  pegaDadosRelogio();
  if (Serial.available() > 0 || bluetooth.available() > 0) {
    String Comando = concatena();
    DynamicJsonDocument doc(256);
    auto error = deserializeJson(doc, Comando);
    if (error) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return;
    }

    String entrada = doc["ent"];

    if (entrada == "st") {
      int valrele = doc["r"];
      if (doc["df"] == 1) {
        switch (valrele) {
          case 1:
            Alarm.enable(id1);
            Alarm.enable(id5);
            writeEEPROM(alarmeEEPROM1, 1);
            break;
          case 2:
            Alarm.enable(id2);
            Alarm.enable(id6);
            writeEEPROM(alarmeEEPROM2, 1);
            break;
          case 3:
            Alarm.enable(id3);
            Alarm.enable(id7);
            writeEEPROM(alarmeEEPROM3, 1);
            break;
          case 4:
            Alarm.enable(id4);
            Alarm.enable(id8);
            writeEEPROM(alarmeEEPROM4, 1);
            break;
        }
        /*
        if (doc["rele"] == 1){
          Alarm.enable(id1);
          Alarm.enable(id5);
          writeEEPROM(alarmeEEPROM1, 1);
        }
        if (doc["rele"] == 2){
          Alarm.enable(id2);
          Alarm.enable(id6);
          writeEEPROM(alarmeEEPROM2, 1);
        }
        if (doc["rele"] == 3){
          Alarm.enable(id3);
          Alarm.enable(id7);
          writeEEPROM(alarmeEEPROM3, 1);
        }
        if (doc["rele"] == 4){
          Alarm.enable(id4);
          Alarm.enable(id8);
          writeEEPROM(alarmeEEPROM4, 1);
        }*/
      } else {
        switch (valrele) {
          case 1:
            Alarm.free(id1);
            Alarm.free(id5);
            writeEEPROM(alarmeEEPROM1, 0);
            break;
          case 2:
            Alarm.free(id2);
            Alarm.free(id6);
            writeEEPROM(alarmeEEPROM2, 0);
            break;
          case 3:
            Alarm.free(id3);
            Alarm.free(id7);
            writeEEPROM(alarmeEEPROM3, 0);
            break;
          case 4:
            Alarm.free(id4);
            Alarm.free(id8);
            writeEEPROM(alarmeEEPROM4, 0);
            break;
        }
        /*
        if (doc["rele"] == 1){
          Alarm.free(id1);
          Alarm.free(id5);
          writeEEPROM(alarmeEEPROM1, 0);
        }
        if (doc["rele"] == 2){
          Alarm.free(id2);
          Alarm.free(id6);
          writeEEPROM(alarmeEEPROM2, 0);
        }
        if (doc["rele"] == 3){
          Alarm.free(id3);
          Alarm.free(id7);
          writeEEPROM(alarmeEEPROM3, 0);
        }
        if (doc["rele"] == 4){
          Alarm.free(id4);
          Alarm.free(id8);
          writeEEPROM(alarmeEEPROM4, 0);
        }*/
      }
    }
    if (entrada == "dd") {
      enviaComando();
    }
    if (entrada == "hr") {
      enviaDadosHora(doc["h"], doc["m"], 0);
      enviaComando();
    }
    if (entrada == "dt") {
      enviaDadosData(doc["d"], doc["m"], doc["a"]);
      enviaComando();
    }
    if (entrada == "al") {
      byte horaAtiva = doc["ha"];
      byte minutoAtiva = doc["ma"];
      byte horaDesativa = doc["hd"];
      byte minutoDesativa = doc["md"];
      byte relval = doc["r"];
      switch (relval) {
      case 1:
        Alarm.free(id1);
        Alarm.free(id5);
        writeEEPROM(HoraEEPROM_1A, horaAtiva);
        writeEEPROM(MinutoEEPROM_1A, minutoAtiva);
        writeEEPROM(HoraEEPROM_1D, horaDesativa);
        writeEEPROM(MinutoEEPROM_1D, minutoDesativa);
        id1 = Alarm.alarmRepeat(horaAtiva, minutoAtiva, 0, ativaRele1);
        id5 = Alarm.alarmRepeat(horaDesativa, minutoDesativa, 0, desativaRele1);
      break;
      case 2:
        Alarm.free(id2);
        Alarm.free(id6);
        writeEEPROM(HoraEEPROM_2A, horaAtiva);
        writeEEPROM(MinutoEEPROM_2A, minutoAtiva);
        writeEEPROM(HoraEEPROM_2D, horaDesativa);
        writeEEPROM(MinutoEEPROM_2D, minutoDesativa);
        id2 = Alarm.alarmRepeat(horaAtiva, minutoAtiva, 0, ativaRele2);
        id6 = Alarm.alarmRepeat(horaDesativa, minutoDesativa, 0, desativaRele2);
      break;
      case 3:
        Alarm.free(id3);
        Alarm.free(id7);
        writeEEPROM(HoraEEPROM_3A, horaAtiva);
        writeEEPROM(MinutoEEPROM_3A, minutoAtiva);
        writeEEPROM(HoraEEPROM_3D, horaDesativa);
        writeEEPROM(MinutoEEPROM_3D, minutoDesativa);
        id3 = Alarm.alarmRepeat(horaAtiva, minutoAtiva, 0, ativaRele3);
        id7 = Alarm.alarmRepeat(horaDesativa, minutoDesativa, 0, desativaRele3);
      break;
      case 4:
        Alarm.free(id4);
        Alarm.free(id8);
        writeEEPROM(HoraEEPROM_4A, horaAtiva);
        writeEEPROM(MinutoEEPROM_4A, minutoAtiva);
        writeEEPROM(HoraEEPROM_4D, horaDesativa);
        writeEEPROM(MinutoEEPROM_4D, minutoDesativa);
        id4 = Alarm.alarmRepeat(horaAtiva, minutoAtiva, 0, ativaRele4);
        id8 = Alarm.alarmRepeat(horaDesativa, minutoDesativa, 0, desativaRele4);
      break;
      }
    }
    if (entrada == "di") {
      byte str = doc["r"];
      switch (str) {
      case 1:
        if (doc["s"] == 1) {
          ativaRele1();
        } else {
          desativaRele1();
        }
      break;
      case 2:
        if (doc["s"] == 1) {
          ativaRele2();
        } else {
          desativaRele2();
        }
      break;
      case 3:
        if (doc["s"] == 1) {
          ativaRele3();
        } else {
          desativaRele3();
        }
      break;
      case 4:
        if (doc["s"] == 1) {
          ativaRele4();
        } else {
          desativaRele4();
        }
      break;
      }
    }
    Serial.println(Comando);
  }
  if (novominuto != minuto) {
    enviaComando();
    novominuto = minuto;
  }
  Alarm.delay(10);
}
