#include <Arduino.h>
#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>

#define TipoRTC 1 //Se for usar RTC_DS1307 colocar 1, se for usar o DS3132 coloca 0
#define LeitorTemp 1 //Se for usar um thermistor colocar 1, se for usar o DS18B20 no D5 deve colocar 0

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
//Endereço 30 ao 36 reservado para dias da semana
int diaSemanaA1[7]={60,61,62,53,64,65,66};
int diaSemanaA2[7]={67,68,69,70,71,72,73};
int diaSemanaA3[7]={74,75,76,77,78,79,80};
int diaSemanaA4[7]={81,82,83,84,85,86,87};
int alarmeEEPROM1 = 0, alarmeEEPROM2 = 1, alarmeEEPROM3 = 2, alarmeEEPROM4 = 3;
int HoraEEPROM_1A = 10, MinutoEEPROM_1A = 11, HoraEEPROM_1D = 12, MinutoEEPROM_1D = 13;
int HoraEEPROM_2A = 14, MinutoEEPROM_2A = 15, HoraEEPROM_2D = 16, MinutoEEPROM_2D = 17;
int HoraEEPROM_3A = 18, MinutoEEPROM_3A = 19, HoraEEPROM_3D = 20, MinutoEEPROM_3D = 21;
int HoraEEPROM_4A = 22, MinutoEEPROM_4A = 23, HoraEEPROM_4D = 24, MinutoEEPROM_4D = 25;
int statusRele[quantAlarms] = { 100, 101, 102, 103 };
int portas[] = { pinRele1, pinRele2, pinRele3, pinRele4 };
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
  //Serial.println(conteudo);
  return conteudo;
}
void enviaComando(String concatena){
  pegaDadosRelogio();
      String envia;
      if(concatena == "data"){
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
        envia += "\"}";
      }
      if(concatena == "stRl"){
        envia += "{";
        envia += "r1\":";
        envia += readEEPROM(statusRele[0]);
        envia += ",\"r2\":";
        envia += readEEPROM(statusRele[1]);
        envia += ",\"r3\":";
        envia += readEEPROM(statusRele[2]);
        envia += ",\"r4\":";
        envia += readEEPROM(statusRele[3]);
        envia += "}";
      }
      if(concatena == "stAs") {
        envia += "{\"a1\":";
        envia += String(readEEPROM(alarmeEEPROM1));
        envia += ",\"a2\":";
        envia += String(readEEPROM(alarmeEEPROM2));
        envia += ",\"a3\":";
        envia += String(readEEPROM(alarmeEEPROM3));
        envia += ",\"a4\":";
        envia += String(readEEPROM(alarmeEEPROM4));
        envia += "}";
      }
      if(concatena == "stA1") {
        envia += ",\"h1\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_1A));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_1A));
        envia += "\",\"l1\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_1D));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_1D));
        envia += "\"}";
      }
      if(concatena == "stA2") {
        envia += "{\"h2\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_2A));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_2A));
        envia += "\",\"l2\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_2D));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_2D));
        envia += "\"}";
      }
      if(concatena == "stA3") {
        envia += "{\"h3\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_3A));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_3A));
        envia += "\",\"l3\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_3D));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_3D));
        envia += "\"}";
      }
      if(concatena == "stA4") {
        envia += "{\"h4\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_4A));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_4A));
        envia += "\",\"l4\":\"";
        envia += printDigit(readEEPROM(HoraEEPROM_4D));
        envia += ":";
        envia += printDigit(readEEPROM(MinutoEEPROM_4D));
        envia += "\"}";
      }
      if(concatena == "week") {
        envia += "{\"w1\":\"";
        for(int i=0; i<7; i++){
          envia += readEEPROM(diaSemanaA1[i]);
        }
        envia += "\",\"w2\":\"";
        for(int i=0; i<7; i++){
          envia += readEEPROM(diaSemanaA2[i]);
        }
        envia += "\",\"w3\":\"";
        for(int i=0; i<7; i++){
          envia += readEEPROM(diaSemanaA3[i]);
        }
        envia += "\",\"w4\":\"";
        for(int i=0; i<7; i++){
          envia += readEEPROM(diaSemanaA4[i]);
        }
        envia += "\"}";
      }
      concatena ="";
      bluetooth.println(envia);
      Serial.println(envia);
      envia = "";
}
void novaPlaca() {
  for (int i = 0; i < 200; i++) {
    writeEEPROM(i, 0);
  }
}
/*criar array como função
int * createArray(uint8_t size){
  if (size > 0) 
  {
    int* p = (int*) malloc(size * sizeof(int));
    for (int i = 0; i< size; i++) *p++ = i*i;
    return p;
  }
  return null;
}
*/
void alarmar(int pinRele, int alarmeInicial, int alarmeFinal){
  int horaAgora = (hora*100)+minuto;
  int indexComparador = statusRele[pinRele]+50; //Index de comparador para não ficar gravando na eeproom sem necessidade a cada minuto
  int diaAlsem;
  switch (pinRele) {
  case 1:
    diaAlsem = readEEPROM(diaSemanaA1[diasemana]);
  break;
  case 2:
    diaAlsem = readEEPROM(diaSemanaA2[diasemana]);
  break;
  case 3:
    diaAlsem = readEEPROM(diaSemanaA3[diasemana]);
  break;
  case 4:
    diaAlsem = readEEPROM(diaSemanaA4[diasemana]);
  break;
  }
  if (readEEPROM(pinRele) == 1 && alarmeInicial != alarmeFinal && diaAlsem == 1) { //Se o alarme estiver ativado e hora inicial for diferente de hora final e dia da semana for dia ativo, executa.
    if(alarmeInicial < alarmeFinal){ //Se a hora de ligar for menor que a hora de desligar
      if(alarmeInicial <= horaAgora && alarmeFinal > horaAgora){ //Se a hora atual for maior ou igual hora do alarme
        if(readEEPROM(indexComparador)==0){
          writeEEPROM(statusRele[pinRele], 1);
          writeEEPROM(indexComparador, 1);
          ciWrite(pinRele, HIGH); 
        }
      }else{
        if(readEEPROM(indexComparador)==1){
          writeEEPROM(statusRele[pinRele], 0);
          writeEEPROM(indexComparador, 0);
          ciWrite(pinRele, LOW);
        }
      }
    }else{ //Se a hora de ligar for maior que a hora de desligar
      if(alarmeInicial <= horaAgora || alarmeFinal > horaAgora){
        if(readEEPROM(indexComparador)==0){
          writeEEPROM(statusRele[pinRele], 1);
          writeEEPROM(indexComparador, 1);
          ciWrite(pinRele, HIGH);
        }
      }else{
        if(readEEPROM(indexComparador)==1){
          writeEEPROM(statusRele[pinRele], 0);
          writeEEPROM(indexComparador, 0);
          ciWrite(pinRele, LOW);
        }
      }
    }
  }
}
String valorJson (String entTxt, String busca){
  String retornoValor = "";
 //limpa o json, removendo:{"}
  for (int i=0; i <entTxt.length();++i){
    char c = entTxt.charAt(i);
    if(c=='{'){
      entTxt.remove(i, 1);
      i--;
    }
    if(c=='}')entTxt.remove(i, 1);
    if(c=='"')entTxt.remove(i, 1);
  }
  String verificador = entTxt;
  if(verificador.substring(verificador.length()-1) == "}"){
    entTxt.remove(verificador.length()-1);
  }
  entTxt = entTxt+",";
  //Define variaveis e valores e seta os mesmo!!
  int goOn = 1; int pos1=0; int pos2 = entTxt.length();
  while( goOn == 1 ) {
    pos1 = entTxt.lastIndexOf(",", pos2);
    pos2 = entTxt.lastIndexOf(",", pos1 - 1);
    if( pos2 <= 0 ) goOn = 0;
    String tmp = entTxt.substring(pos2 + 1, pos1);
    String nome = tmp.substring(0,tmp.indexOf(":"));
    String valor = tmp.substring(tmp.indexOf(":")+1);

    if (nome == busca)
     retornoValor=valor;

    if( goOn != 1) break;
  }
  return retornoValor;
}
void(* resetFunc) (void) = 0; //Função para resetar o arduino rapidamente
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
  delay(100);
}
void loop() {
  wdt_reset();  //watch dog
  pegaDadosRelogio();
  if (Serial.available() > 0 || bluetooth.available() > 0) {
    String Comando = concatena();
    String entrada;
    if(Comando.indexOf('{') >= 0 && Comando.indexOf('}')>=0){
      entrada = valorJson(Comando, "ent");
    }

    if (entrada == "st") { //{"ent":"st","a":1,"s":1}
      int valrele = valorJson(Comando, "a").toInt();
      int valStatus = valorJson(Comando, "s").toInt();
      writeEEPROM(valrele-1, valStatus);
      enviaComando("stRl");
    }
    if (entrada == "dd") { //{"ent":"dd"}
      enviaComando("data");
      enviaComando("stRl");
    }
    if (entrada == "hr") { //{"ent":"hr","gmt":"-3","h":1716050337}
      uint32_t epoch = valorJson(Comando, "h").toInt();
      int gmt = valorJson(Comando, "gmt").toInt()*3600;
      epoch= epoch+(gmt);
      rtc.adjust(DateTime(epoch));
      enviaComando("data");
    }
    if (entrada == "dt") { //{"ent":"dt","h":3,"m":31,"s":0,"d":17,"M":8,"a":2023}
      rtc.adjust(DateTime(valorJson(Comando, "a").toInt(), valorJson(Comando, "m").toInt(), valorJson(Comando, "d").toInt(), valorJson(Comando, "h").toInt(), valorJson(Comando, "M").toInt(), valorJson(Comando, "a").toInt()));
      enviaComando("data");
    }
    if (entrada == "al") { //{"ent":"al","r":1,"ha":21,"ma":15,"hd":22,"md":15}
      int horaAtiva = valorJson(Comando, "ha").toInt();
      int minutoAtiva = valorJson(Comando, "ma").toInt();
      int horaDesativa = valorJson(Comando, "hd").toInt();
      int minutoDesativa = valorJson(Comando, "md").toInt();
      int relval = valorJson(Comando, "r").toInt();
      enviaComando("stRl");
      switch (relval) {
        case 1:
          writeEEPROM(HoraEEPROM_1A, horaAtiva);
          writeEEPROM(MinutoEEPROM_1A, minutoAtiva);
          writeEEPROM(HoraEEPROM_1D, horaDesativa);
          writeEEPROM(MinutoEEPROM_1D, minutoDesativa);
          enviaComando("stA1");
        break;
        case 2:
          writeEEPROM(HoraEEPROM_2A, horaAtiva);
          writeEEPROM(MinutoEEPROM_2A, minutoAtiva);
          writeEEPROM(HoraEEPROM_2D, horaDesativa);
          writeEEPROM(MinutoEEPROM_2D, minutoDesativa);
          enviaComando("stA2");
        break;
        case 3:
          writeEEPROM(HoraEEPROM_3A, horaAtiva);
          writeEEPROM(MinutoEEPROM_3A, minutoAtiva);
          writeEEPROM(HoraEEPROM_3D, horaDesativa);
          writeEEPROM(MinutoEEPROM_3D, minutoDesativa);
          enviaComando("stA3");
        break;
        case 4:
          writeEEPROM(HoraEEPROM_4A, horaAtiva);
          writeEEPROM(MinutoEEPROM_4A, minutoAtiva);
          writeEEPROM(HoraEEPROM_4D, horaDesativa);
          writeEEPROM(MinutoEEPROM_4D, minutoDesativa);
          enviaComando("stA4");
        break;
      }
    }
    if (entrada == "di") { //{"ent":"di","r":2,"s":1}
      alteraRele(valorJson(Comando, "r").toInt(), valorJson(Comando, "s").toInt());
      enviaComando("stRl");
    }
    if (entrada == "wk") { //{"ent":"wk","a":1,"d":"1000001"}
      String diasAlarme = valorJson(Comando, "d");
      if(diasAlarme != ""){
        for(int i=0; i<7; i++){
          char valDias = diasAlarme[i];
          int someInt = valDias - '0';
          switch (valorJson(Comando, "a").toInt()) {
          case 1:
            writeEEPROM(diaSemanaA1[i], someInt);
          break;
          case 2:
            writeEEPROM(diaSemanaA2[i], someInt);
          break;
          case 3:
            writeEEPROM(diaSemanaA3[i], someInt);
          break;
          case 4:
            writeEEPROM(diaSemanaA4[i], someInt);
          break;
          }
        }
      }
      enviaComando("week");
    }
    if (entrada == "rs") { //{"ent":"rs"}
      novaPlaca();
      delay(1000);
      resetFunc();
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

    enviaComando("data");
    enviaComando("stRl");
    novominuto = minuto;
  }
}
