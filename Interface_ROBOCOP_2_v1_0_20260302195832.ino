#include "Arduino.h"
#include "SoftwareSerial.h"
#include "BluetoothSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "Wire.h"
#include "Adafruit_TCS34725.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ST7735.h"
#include "SPI.h"
#include "EEPROM.h"

//DENIFIÇÕES============================================================================================================================================
#define PinBOOT 0
#define LIGA 1
#define DESLIGA 0
#define PinoRxRadio 34
#define PinoPTTR 25
#define BusyDFp 4

// relação MENSAGEM  com  o  NÚMERO DO ARQUIVO no cartão SD-------------------------------------------------------------------------------
 #define RogerBEEPlocal   25  //Arquivo 01~24 correspondem aos áudios de HORA e MEIA: 01 = 00:30h ~ 24 = 23:30h
 #define RogerBEEPweb     26  //Arquivo 25~31 correspondem aos áudios OPERACIONAIS
 #define RogerBEEPerro    27               
 #define TelaAPAGOU       28
 #define LinkOFFLINE      29
 #define LinkONLINE       30
 #define ZelloFECHOU      31
 #define PrimeiraVinheta  32  //Arquivo 32 ou + correspondem aos áudios de VINHETAS diversas

// TELA ST7735 PINOUT
//      VCC 3.3~5V
//      LED 3.3V
#define Display_A0 2    //A0 ou DC
#define Display_CS 32   //CS
#define Display_SDA 23  //SDA ou MOSI
#define Display_SCK 18  //SCK ou CLK
#define Display_RST 14
//#define Display_MISO 0

//Definições dos endereços dos valores salvos na EEPROM----------------------------------------------------------------------------------------------
#define EEPROM_SIZE 64
// Endereços de 0 a 11 são os valores de Zon[0,1,2,3], Ztx[4,5,6,7] e Zrx[8,9,10,11]
// 12 = Volume
// 13 = MENSAGEMativada
// 14 = BEEPativado
// 15 = HORAativada

Adafruit_ST7735 Display = Adafruit_ST7735(Display_CS, Display_A0, Display_SDA, Display_SCK, Display_RST);

SoftwareSerial mySoftwareSerial(16, 17);  // RX, TX
DFRobotDFPlayerMini DFPlayer;

BluetoothSerial SerialBT;

int Zon[4] = { 0, 0, 0, 0 };
int Zof[4] = { 0, 0, 0, 0 };
int Ztx[4] = { 0, 0, 0, 0 };
int Zrx[4] = { 0, 0, 0, 0 };
int Zti[4] = { 0, 0, 0, 0 };

char caractere;

String OPCAO       = "";
String ESTADOz     = "teste";
String ESTADOza    = "";
String ESTADOr     = "teste";
String ESTADOra    = "";
String ESTADOlink  = "teste";
String ESTADOlinka = "";

byte H  = 0;
byte Ha = 0;
byte M  = 0;
byte Ma = 0;
byte S  = 0;

int R  = 0;
int Ra = 0;
int G  = 0;
int Ga = 0;
int B  = 0;
int Ba = 0;
int C  = 0;
int Ca = 0;
int SinalRadio  = 0;
int SinalRadioA = 0;

byte Narquivos  = 0;
byte Volume     = 0;
byte ExibirTELA = 1;
byte Vinheta    = PrimeiraVinheta;

unsigned long timer    = millis();
unsigned long timer1   = millis();
unsigned long timerFPS = millis();
unsigned long timerSCR = millis();
unsigned long Gatilho  = millis();
unsigned long segundo  = millis();

bool HORAcerta       = false;
bool FalarHORA       = false;
bool BTconectado     = false;
bool CarregarTELA    = true;
bool Preencher       = true;
bool IconeZELLO      = false;
bool PTTRacionado    = false;
bool MENSAGEMativada = false;
bool FalarMENSAGEM   = false;
bool HORAativada     = false;
bool BEEPativado     = false;
bool NotificaOF      = false;
bool NotificaTI      = false;
bool NotificaTA      = false;
bool NotificaNC      = false;
bool NotificaON      = false;
bool TocouNotON      = false;
bool Recebeu         = false;

/* SENSOR RGB PINOUT 
   Connect SCL    to analog 22
   Connect SDA    to analog 21
   Connect VDD    to 3.3V DC
   Connect GROUND to common ground */

/* Initialise with default values (int time = 2.4ms, gain = 1x) */
Adafruit_TCS34725 SensorZello = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

//INICIALIZAÇÃO==========================================================================================================================================

void setup() 
{
  pinMode(PinoRxRadio, INPUT);
  pinMode(PinBOOT, INPUT_PULLUP);
  pinMode(PinoPTTR, OUTPUT);

  SinalRadio = analogRead(PinoRxRadio);
  digitalWrite(PinoPTTR, false);

  Serial.begin(115200);
  if (!EEPROM.begin(EEPROM_SIZE)) Serial.println(F("Falha ao iniciar EEPROM"));
  delay(100);
  mySoftwareSerial.begin(9600);
  delay(100);

  Serial.println(F("INICIALIZANDO"));
  Serial.println(F("Testando Display ST7735"));
  Display.initR(INITR_BLACKTAB);
  Display.setRotation(3);
  Display.setTextSize(2);
  Display.fillScreen(ST77XX_BLACK);
  Display.setTextWrap(true);
  Display.setCursor(0, 0);
  Display.setTextColor(ST77XX_YELLOW);
  Display.print(F("RoboCOP"));
  Display.setTextSize(1);
  Display.setCursor(80, 8);
  Display.println(F(" iniciando"));
  delay(1000);
  Display.setCursor(0, 20);
  Display.setTextColor(ST77XX_GREEN);
  Display.println(F("Display OK!!!"));
  delay(250);

  SerialBT.register_callback(callback);
  SerialBT.begin("PTT");
  delay(500);
  if(SerialBT.begin()) 
  {
    Serial.println(F("Bluetooth Iniciado!"));
    Display.println(F("Bluetooth OK!!!"));
  }
  
  DFPlayer.begin(mySoftwareSerial);
  Display.setTextColor(ST77XX_YELLOW);
  Display.println(F("Iniciando DFPlayer!!!"));
  delay(100);
  while (!DFPlayer.begin(mySoftwareSerial)) 
  {
    int x=0;
    DFPlayer.begin(mySoftwareSerial);
    Display.print  (F("."));
    if (x>5)
    {
      Display.setTextColor(ST77XX_RED);
      Display.println(F("DFPlayer nao encontrado"));
      Display.setTextColor(ST77XX_YELLOW);
      Display.println(F("Insira o MicroSD,"));
      Display.println(F("verifique as conexoes"));
      Display.println(F("e reinicie a CTRL"));
      while (1);
    }
    delay(100);
  }
  Display.setTextColor(ST77XX_YELLOW);
  DFPlayer.setTimeOut(500);
  delay(500);
  Display.print  (F("."));
  DFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  delay(500);
  Display.print  (F("."));
  DFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  delay(550);
  Narquivos = DFPlayer.readFileCounts();
  delay(500);
  Display.setTextColor(ST77XX_ORANGE);
  
  while ((Narquivos < 1) || (Narquivos > 254)) 
  {
    Narquivos = DFPlayer.readFileCounts();
    Display.print  (F("."));
    delay(500);
  }
  
  Display.setTextColor(ST77XX_BLUE);
  Display.println(F("."));
  Display.setTextColor(ST77XX_BLUE);
  Display.print  (F("Num arq = "));
  Display.println(Narquivos);
  delay(100);
  Display.setTextColor(ST77XX_GREEN);
  Display.println(F("DFPlayer OK!!!"));
  delay(100);

  Display.setTextColor(ST77XX_WHITE);
  Display.print(F("Sensor RGB "));
  SensorZello.begin();
  delay(100);
  SensorZello.setIntegrationTime(TCS34725_INTEGRATIONTIME_50MS);
  delay(100);
  if (SensorZello.begin()) {
    Display.setTextColor(ST77XX_GREEN);
    Display.println(F("OK!!!"));
    delay(500);
  } else {
    Display.setTextColor(ST77XX_RED);
    Display.println(F("nao encontrado!"));
    Display.setTextColor(ST77XX_YELLOW);
    Display.println(F("verifique as conexoes"));
    Display.println(F("e reinicie a CTRL"));
    while (1)
      ;
  }

  // Lendo valores na EEPROM
  Display.setTextColor(ST77XX_WHITE);
  Display.println(F("Carregando dados..."));
  int addr = 0;
  for (int i = 0; i < 4; i++)
  {
    Zon[i] = EEPROM.read(addr++);
    Display.print(Zon[i]);
    delay(40);
  }
  for (int i = 0; i < 4; i++)
  {
    Ztx[i] = EEPROM.read(addr++);
    Display.print(Ztx[i]);
    delay(40);
  }
  for (int i = 0; i < 4; i++)
  {
    Zrx[i] = EEPROM.read(addr++);
    Display.print(Zrx[i]);
    delay(40);
  }
  Volume          = EEPROM.read(12);
  MENSAGEMativada = EEPROM.read(13);
  BEEPativado     = EEPROM.read(14);
  HORAativada     = EEPROM.read(15);
  DFPlayer.volume(Volume);
  delay(500);
  DFPlayer.volume(Volume);
  delay(500);
  Display.print(DFPlayer.readVolume());
  Display.println(Volume);
  Display.setTextColor(ST77XX_GREEN);
  Display.println(F("OK!!!"));
  delay(100);
  Display.setTextColor(ST77XX_GREEN);
  Display.println();
  Display.println(F("INICIALIZACAO CONCLUIDA!!!"));
  delay(500);
  Display.fillScreen(ST77XX_BLACK);
  Display.setTextSize(1);
}

//===========================================================================================

void loop() 
{
  ESTADORADIO();
  ESTADOZELLO();
  TELA();
  RELOGIO();
  
  if (!digitalRead(PinBOOT)) 
  {
    BotaoPRESS();
    if (OPCAO == "PROXIMO")
    {
      ExibirTELA++;
      if (ExibirTELA > 4) ExibirTELA = 1;
    }
    else 
    if (OPCAO == "SELECIONA")
    {
      OPCAO = "";
      MENU1();
    }
    OPCAO = "";
    CarregarTELA = true;
    Preencher = true;
  }

  if (ESTADOlink == "Inoperante")
  {
    if ((ESTADOr == "SB") && (BEEPativado))
    {
      if ((Recebeu) && (!BTconectado))
      {
        delay(250);
        PTTradio(LIGA);
        TOCAR(RogerBEEPerro);
        PTTradio(DESLIGA);
        NotificaNC = false;
        Recebeu = false;
      }
      else if ((Recebeu) && (ESTADOz== "TA"))
      {
        delay(250);
        PTTradio(LIGA);
        TOCAR(TelaAPAGOU);
        PTTradio(DESLIGA);
        NotificaTA = false;
        Recebeu = false;
      }
      else if ((Recebeu) && (ESTADOz== "OF"))
      {
        delay(250);
        PTTradio(LIGA);
        TOCAR(LinkOFFLINE);
        PTTradio(DESLIGA);
        NotificaOF = false;
        Recebeu = false;
      }
      else if ((Recebeu) && (ESTADOz== "TI"))
      {
        delay(250);
        PTTradio(LIGA);
        TOCAR(ZelloFECHOU);
        PTTradio(DESLIGA);
        NotificaTI = false;
        Recebeu = false;
      }
      TocouNotON = false;
    }
  }

  if (ESTADOlink != "Inoperante")
  {

    if ((!TocouNotON) && (BEEPativado) && (NotificaON) && (ESTADOr == "SB") && (ESTADOz == "ON"))
    {
      PTTradio(LIGA);
      TOCAR(LinkONLINE);
      PTTradio(DESLIGA);
      NotificaON = false;
      TocouNotON = true;
    }
    
    
    if ((HORAativada) && (FalarHORA))
    {
      PTTradio(LIGA);
      TOCAR(H + 1);
      PTTradio(DESLIGA);
      FalarHORA = false;
    }

    if ((FalarMENSAGEM) && (MENSAGEMativada) && (M == 30))
    {
      PTTradio(LIGA);
      TOCAR(Vinheta);
      Vinheta++;
      if (Vinheta > Narquivos) Vinheta = PrimeiraVinheta;
      PTTradio(DESLIGA);
      FalarMENSAGEM = false;
    }

    // ZELLO recebendo
    if ((ESTADOz == "RX") && (ESTADOr == "SB"))
    {
      while (ESTADOz == "RX")
      {
        if (ESTADOr == "SB") PTTradio(LIGA);
        ESTADORADIO();
        ESTADOZELLO();
        TELA();
      }
      if (BEEPativado) TOCAR(26);
      PTTradio(DESLIGA);
    }

    //RADIO recebendo
    if ((ESTADOr == "RX") && (ESTADOz == "ON"))
    {
      while (ESTADOr == "RX")
      {
        ESTADORADIO();
        ESTADOZELLO();
        if (ESTADOz == "ON") PTTzello(LIGA);
        ESTADOLINK();
        TELA();
      }
      PTTzello(DESLIGA);
      if (BEEPativado) 
      {
        PTTradio(LIGA);
        TOCAR(25);
        PTTradio(DESLIGA);
      }
    }
  }
}

//===========================================================================================

void RELOGIO()
{
  if (millis() > segundo) 
  {
    S++;
    segundo = segundo + 1000; 
    if (S > 59) 
    { 
      S = 0;
      M++;
      if (M > 59) 
      { 
        M = 0;
        H++;
        if (H > 23) H = 0;
        FalarMENSAGEM  = true;
        if (HORAcerta) FalarHORA = true;        
      }
    }
  }
}

//===========================================================================================

void BotaoPRESS() 
{
  byte coluna = 0;
  OPCAO = "PROXIMO";
  while (!digitalRead(PinBOOT)) 
  {
    Display.fillRect(coluna, 127, 1, 1, ST77XX_RED);
    coluna++;
    if (coluna > 159) 
    {
      Display.fillRect(0, 127, 160, 1, ST77XX_GREEN);
      OPCAO = "SELECIONA";
      while (!digitalRead(PinBOOT));
    }
    delay(5);
  }
  Display.fillRect(0, 127, 159, 1, ST77XX_BLACK);
}

//===========================================================================================

void TOCAR(byte Arquivo)
{
  DFPlayer.play (Arquivo);
  while (digitalRead(BusyDFp))
  {
    ESTADORADIO();
    TELA();
  }
  while (!digitalRead(BusyDFp))
  {
    ESTADORADIO();
    TELA();
  }
}

//===========================================================================================

void TELA()
{
  if (ExibirTELA == 1) MOSTRADOR1();
  else
  if (ExibirTELA == 2) MOSTRADOR2();
  else
  if (ExibirTELA == 3) MOSTRADOR3();
  else
  if (ExibirTELA == 4) MOSTRADOR4();
}

void MOSTRADOR1()
{
  if (millis() > (timerFPS + 50))
  {
    if (CarregarTELA)
    {
      CarregarTELA = false;
      ESTADOz = "OF";
      Display.fillScreen(ST77XX_BLACK);
      Display.setTextSize(2);
      Display.setCursor(0, 0);
      Display.setTextColor(ST77XX_YELLOW);
      Display.println(F("RoboCOP"));
      Display.setTextSize(1);
      Display.setTextColor(ST77XX_WHITE);
      Display.setCursor(0, 119);
      Display.print(F("Link:"));
      if (BTconectado) Display.setTextColor(ST77XX_BLUE);
      else             Display.setTextColor(ST77XX_RED);
      Display.setCursor(148, 119);
      Display.print(F("BT"));
      //  Icone RADIO
      Display.drawRoundRect(20, 25,  6, 30, 3, ST77XX_WHITE);
      Display.fillRoundRect(20, 50, 30, 50, 5, ST77XX_BLACK);
      Display.drawRoundRect(20, 50, 30, 50, 5, ST77XX_WHITE);
      Display.drawRoundRect(24, 56, 22, 22, 3, ST77XX_WHITE);
      //  Icone ZELLO
      Display.drawRoundRect(105, 35, 41, 60, 3, ST77XX_WHITE);
      Display.setTextSize(2);
      Display.setTextColor(ST77XX_WHITE);
      Display.setCursor(120, 56);
      Display.print(F("z"));
    }
    // RELOGIO;
    if ((Ma != M) || (Preencher))
    {
      Display.setTextSize(2);
      Display.setCursor(100, 0);
      Display.setTextColor(ST77XX_BLACK);
      if (Ha<10) Display.print(F("0"));
      Display.print(Ha);
      Display.print(F(":"));
      if (Ma<10) Display.print(F("0"));
      Display.print(Ma);
      Display.setCursor(100, 0);
      if (HORAcerta) Display.setTextColor(ST77XX_WHITE);
      else           Display.setTextColor(ST77XX_MAGENTA);
      if (H<10) Display.print(F("0"));
      Display.print(H);
      Display.print(F(":"));
      if (M<10) Display.print(F("0"));
      Display.print(M);
      Display.setTextSize(1);
      Ma = M;
      Ha = M;
    }

    //  STATUS RADIO
    if ((ESTADOr != ESTADOra) || (Preencher))
    {
      if (ESTADOr == "SB") Display.fillRoundRect(25, 57, 20, 20, 3, ST77XX_BLACK);
      if (ESTADOr == "TX") Display.fillRoundRect(25, 57, 20, 20, 3, ST77XX_RED);
      if (ESTADOr == "RX") Display.fillRoundRect(25, 57, 20, 20, 3, ST77XX_GREEN);
      ESTADOra = ESTADOr;
    }
    //  STATUS ZELLO
    if ((ESTADOz != ESTADOza) || (Preencher))
    {
      if (IconeZELLO)
      {
        Display.fillRoundRect(105, 35, 41, 60, 3, ST77XX_BLACK);
        Display.drawRoundRect(105, 35, 41, 60, 3, ST77XX_WHITE);
        Display.setTextSize(2);
        Display.setTextColor(ST77XX_WHITE);
        Display.setCursor(120, 56);
        Display.print(F("z"));
        IconeZELLO = false;
      }
      if (ESTADOz == "ON") 
      {
        Display.drawCircle(125, 65, 16, ST77XX_ORANGE);
        Display.drawCircle(125, 65, 15, ST77XX_ORANGE);
      }
      if (ESTADOz == "TX")
      {
        Display.drawCircle(125, 65, 16, ST77XX_RED);
        Display.drawCircle(125, 65, 15, ST77XX_RED);
      }
      if (ESTADOz == "RX")
      {
        Display.drawCircle(125, 65, 16, ST77XX_GREEN);
        Display.drawCircle(125, 65, 15, ST77XX_GREEN);
      }
      if (ESTADOz == "OF")
      {
        Display.drawCircle(125, 65, 16, ST77XX_BLACK);
        Display.drawCircle(125, 65, 15, ST77XX_BLACK);
      }
      if (ESTADOz == "TA")
      {
        Display.fillRoundRect(105, 35, 41, 60, 3, ST77XX_BLACK);
        Display.drawRoundRect(105, 35, 41, 60, 3, ST77XX_WHITE);
        IconeZELLO = true;
      }
      if (ESTADOz == "TI")
      {
        Display.fillRoundRect(105, 35, 41, 60, 3, ST77XX_WHITE);
        IconeZELLO = true;
      }
      ESTADOza = ESTADOz;
    }

    // Seta INDICADORA Link RX ou TX
    if ((ESTADOlink != ESTADOlinka) || (Preencher))
    {
      Display.setTextSize(1);
      Display.setTextColor(ST77XX_BLACK);
      Display.setCursor(32, 119);
      Display.print(ESTADOlinka);
      if (ESTADOlink == "Stand By")
      {
        Display.drawTriangle(67, 54, 67, 74, 87, 64, ST77XX_BLACK);
        Display.drawTriangle(87, 54, 87, 74, 67, 64, ST77XX_BLACK);
        Display.setTextColor(ST77XX_ORANGE);
      }
      if (ESTADOlink == "Inoperante")
      {
        Display.drawTriangle(67, 54, 67, 74, 87, 64, ST77XX_BLACK);
        Display.drawTriangle(87, 54, 87, 74, 67, 64, ST77XX_BLACK);
        Display.setTextColor(ST77XX_YELLOW);
      }
      if (ESTADOlink == "Recebendo")
      {
        Display.drawTriangle(67, 54, 67, 74, 87, 64, ST77XX_GREEN);
        Display.setTextColor(ST77XX_GREEN);
      }
      if (ESTADOlink == "Transmitindo")
      {
        Display.drawTriangle(87, 54, 87, 74, 67, 64, ST77XX_RED); 
        Display.setTextColor(ST77XX_RED);
      }
      if (ESTADOlink == "DFPlayer")
      {
        Display.drawTriangle(87, 54, 87, 74, 67, 64, ST77XX_MAGENTA); 
        Display.setTextColor(ST77XX_MAGENTA);
      }
      Display.setCursor(32, 119);
      Display.print(ESTADOlink);
      ESTADOlinka = ESTADOlink;
    }
    timerFPS = millis();
    Preencher = false;
  }
}

void MOSTRADOR2()
{
  if (millis() > (timerFPS + 50))
  {
    if (CarregarTELA)
    {
      Preencher = false;
      Display.fillScreen(ST77XX_BLACK);
      Display.setTextSize(2);
      Display.setCursor(0, 0);
      Display.setTextColor(ST77XX_YELLOW);
      Display.println(F("    INFOS"));
      Display.setTextSize(1);
      Display.setTextColor(ST77XX_WHITE);
      Display.setCursor(0, 25);
      Display.print(F("P: "));
      Display.setCursor(0, 35);
      Display.print(F("ESTADO RADIO: "));

      Display.setCursor(40, 50);
      Display.setTextColor(ST77XX_ORANGE);
      Display.print(F("ON"));
      Display.setCursor(60, 50);
      Display.setTextColor(ST77XX_RED);
      Display.print(F("TX"));
      Display.setCursor(80, 50);
      Display.setTextColor(ST77XX_GREEN);
      Display.print(F("RX"));

      Display.setCursor(0, 60);
      Display.setTextColor(ST77XX_WHITE);
      Display.print(F("R: "));
      Display.setCursor(40, 60);
      Display.print(Zon[0]);
      Display.setCursor(60, 60);
      Display.print(Ztx[0]);
      Display.setCursor(80, 60);
      Display.print(Zrx[0]);

      Display.setCursor(0, 70);
      Display.print(F("G: "));
      Display.setCursor(40, 70);
      Display.print(Zon[1]);
      Display.setCursor(60, 70);
      Display.print(Ztx[1]);
      Display.setCursor(80, 70);
      Display.print(Zrx[1]);

      Display.setCursor(0, 80);
      Display.print(F("B: "));
      Display.setCursor(40, 80);
      Display.print(Zon[2]);
      Display.setCursor(60, 80);
      Display.print(Ztx[2]);
      Display.setCursor(80, 80);
      Display.print(Zrx[2]);

      Display.setCursor(0, 90);
      Display.print(F("C: "));
      Display.setCursor(40, 90);
      Display.print(Zon[3]);
      Display.setCursor(60, 90);
      Display.print(Ztx[3]);
      Display.setCursor(80, 90);
      Display.print(Zrx[3]);

      Display.setCursor(0, 100);
      Display.print(F("ESTADO ZELLO: "));
      CarregarTELA = false;
    }
    if ((SinalRadio != SinalRadioA) || (Preencher))
    {
      Display.setCursor(20, 25);
      Display.setTextColor(ST77XX_BLACK);
      Display.print(SinalRadioA);
      Display.setCursor(20, 25);
      Display.setTextColor(ST77XX_ORANGE);
      Display.print(SinalRadio);
      SinalRadioA = SinalRadio;
    }
    if ((ESTADOr != ESTADOra) || (Preencher))
    {
      Display.setCursor(80, 35);
      Display.setTextColor(ST77XX_BLACK);
      Display.print(ESTADOra);
      Display.setCursor(80, 35);
      Display.setTextColor(ST77XX_WHITE);
      if (ESTADOr == "SB") Display.setTextColor(ST77XX_ORANGE);
      if (ESTADOr == "TX") Display.setTextColor(ST77XX_RED);
      if (ESTADOr == "RX") Display.setTextColor(ST77XX_GREEN);
      Display.print(ESTADOr);
      ESTADOra = ESTADOr;
    } 
    if ((R != Ra) || (Preencher))
    {
      Display.setCursor(20, 60);
      Display.setTextColor(ST77XX_BLACK);
      Display.print(Ra);
      Display.setCursor(20, 60);
      Display.setTextColor(ST77XX_RED);
      Display.print(R);
      Ra = R;
    }
    if ((G != Ga) || (Preencher))
    {
      Display.setCursor(20, 70);
      Display.setTextColor(ST77XX_BLACK);
      Display.print(Ga);
      Display.setCursor(20, 70);
      Display.setTextColor(ST77XX_GREEN);
      Display.print(G);
      Ga = G;
    }
    if ((B != Ba) || (Preencher))
    {
      Display.setCursor(20, 80);
      Display.setTextColor(ST77XX_BLACK);
      Display.print(Ba);
      Display.setCursor(20, 80);
      Display.setTextColor(ST77XX_BLUE);
      Display.print(B);
      Ba = B;
    }
    if ((C != Ca) || (Preencher))
    {
      Display.setCursor(20, 90);
      Display.setTextColor(ST77XX_BLACK);
      Display.print(Ca);
      Display.setCursor(20, 90);
      Display.setTextColor(ST77XX_YELLOW);
      Display.print(C);
      Ca = C;
    }
    if ((ESTADOz != ESTADOza) || (Preencher))
    {
      Display.setCursor(80, 100);
      Display.setTextColor(ST77XX_BLACK);
      Display.print(ESTADOza);
      Display.setTextColor(ST77XX_WHITE);
      Display.setCursor(80, 100);
      if (ESTADOz == "ON") Display.setTextColor(ST77XX_ORANGE);
      if (ESTADOz == "TX") Display.setTextColor(ST77XX_RED);
      if (ESTADOz == "RX") Display.setTextColor(ST77XX_GREEN);
      Display.print(ESTADOz);
      ESTADOza = ESTADOz;
    }
    Preencher = false;
    timerFPS = millis();
  }
}

void MOSTRADOR3()
{
  if (millis() > (timerFPS + 50))
  {
    byte lin  = 63;
    byte col  = 80;
    byte raio = 25;
    if (CarregarTELA)
    {
      CarregarTELA = false;
      ESTADOz = "OF";
      Display.fillScreen(ST77XX_BLACK);
      Display.fillCircle(col, lin, raio, ST77XX_WHITE);
      delay (20);
      Display.fillCircle(col, lin, raio, ST77XX_BLACK);
    }
    
    //  STATUS LINK
    if ((ESTADOlink != ESTADOlinka) || (Preencher))
    {
      if (ESTADOlink == "Stand By")
      {
        Display.fillCircle(col, lin, raio, ST77XX_BLACK);
      }
      if (ESTADOlink == "Inoperante")
      {
         Display.fillCircle(col, lin, raio, ST77XX_YELLOW);
      }
      if (ESTADOlink == "Recebendo")
      {
         Display.fillCircle(col, lin, raio, ST77XX_GREEN);
      }
      if (ESTADOlink == "Transmitindo")
      {
         Display.fillCircle(col, lin, raio, ST77XX_RED);
      }
      if (ESTADOlink == "DFPlayer")
      {
         Display.fillCircle(col, lin, raio, ST77XX_MAGENTA);
      }
      ESTADOlinka = ESTADOlink;
    }
    timerFPS = millis();
    Preencher = false;
  }
}

void MOSTRADOR4()
{
  if (millis() > (timerFPS + 50))
  {
    if (CarregarTELA)
    {
      CarregarTELA = false;
      Display.fillScreen(ST77XX_BLACK);
      Display.setTextSize(2);
      Display.setCursor(0, 45);
      Display.setTextColor(ST77XX_YELLOW);
      Display.println(F("Tela apagada!"));
      delay(600);
    }
    if (Preencher)
    {
      Preencher = false;
      Display.fillScreen(ST77XX_BLACK);
    }
    Serial.print(F(" "));
    Serial.print(F("R:   "));
    Serial.print(R);
    Serial.print(F(" "));
    Serial.print(F("G:   "));
    Serial.print(G);
    Serial.print(F(" "));
    Serial.print(F("B:   "));
    Serial.print(B);
    Serial.print(F(" "));
    Serial.print(F("C:   "));
    Serial.print(C);
    Serial.print(F(" "));
    Serial.print(F("PIN: "));
    Serial.print(SinalRadio);
    Serial.print(F(" "));
    Serial.print(ESTADOz);
    Serial.print(F(" "));
    Serial.print(ESTADOr);
    Serial.println(F(" "));
    Preencher = false;
    timerFPS = millis();
  }
}

//===========================================================================================

void VALORES() 
{
  uint16_t r, g, b, c;
  SensorZello.getRawData(&r, &g, &b, &c);
  R = (r);
  G = (g);
  B = (b);
  C = (c);
}

//===========================================================================================

void ESTADORADIO()
{
  if (PTTRacionado) ESTADOr = "TX";
  else
  {
    SinalRadio = analogRead(PinoRxRadio);
    if (SinalRadio > 100) 
      {
        Gatilho = millis();
        ESTADOr = "RX";
        if (ESTADOlink == "Inoperante") Recebeu = true;
     }
    else 
    if ((SinalRadio < 2) && (millis() > (Gatilho + 200))) ESTADOr = "SB";
  }
  ESTADOLINK();
}

//===========================================================================================

void ESTADOZELLO()
{
  VALORES();
  //ZELLO FECHADO
  if ((R > (Zon[0]+30)) && (C > (Zon[3]+30)))  ESTADOz = "TI";
  //ZELLO ONLINE
  if (((R > (Zon[0]-5)) && (R < (Zon[0]+5))) &
      ((G > (Zon[1]-5)) && (G < (Zon[1]+5))) &
      ((B > (Zon[2]-5)) && (B < (Zon[2]+5))) &
      ((C > (Zon[3]-5)) && (C < (Zon[3]+5))))  ESTADOz = "ON";
  //ZELLO TRANSMITINDO
  if (((R > (Ztx[0]-5)) && (R < (Ztx[0]+5))) &
      ((G > (Ztx[1]-5)) && (G < (Ztx[1]+5))) &
      ((B > (Ztx[2]-5)) && (B < (Ztx[2]+5))) &
      ((C > (Ztx[3]-5)) && (C < (Ztx[3]+5))))  ESTADOz = "TX";
  //ZELLO RECEBENDO
  if (((R > (Zrx[0]-5)) && (R < (Zrx[0]+5))) &
      ((G > (Zrx[1]-5)) && (G < (Zrx[1]+5))) &
      ((B > (Zrx[2]-5)) && (B < (Zrx[2]+5))) &
      ((C > (Zrx[3]-5)) && (C < (Zrx[3]+5))))  ESTADOz = "RX";
  //ZELLO OFFLINE
  if ((R < 5) && (G < 5) && (B < 5) && (C < 10)) ESTADOz = "OF";
  //Tela APAGADA
  if ((R < 2) && (G < 2) && (B < 2) && (C < 2))  ESTADOz = "TA";
  ESTADOLINK();
}

//===========================================================================================

void ESTADOLINK()
{
  ESTADOlink = "Stand By";
  if ((ESTADOz == "RX") && (ESTADOr == "TX"))                            ESTADOlink = "Transmitindo";
  if ((ESTADOr == "RX") && (ESTADOz == "TX"))                            ESTADOlink = "Recebendo";
  if ((ESTADOz == "ON") && (ESTADOr == "TX") && (!digitalRead(BusyDFp))) ESTADOlink = "DFPlayer";
  if ((ESTADOz == "TA") || (ESTADOz == "TI") || 
      (ESTADOz == "OF") || (!BTconectado)   )                            ESTADOlink = "Inoperante";
  
  // Ativando as notificações
  NotificaOF = false;
  NotificaTI = false;
  NotificaTA = false;
  NotificaNC = false;
  if (ESTADOlink == "Inoperante")
  {
    NotificaON = false;
    if      (!BTconectado)    NotificaNC = true;
    else if (ESTADOz == "TA") NotificaTA = true;
    else if (ESTADOz == "OF") NotificaOF = true;
    else if (ESTADOz == "TI") NotificaTI = true;
  }
  else NotificaON = true;
}

//===========================================================================================

void MENU1() 
{
  byte x1 = 1;
  bool voltar = false;
  Display.fillScreen(ST77XX_BLACK);
  while (!voltar) 
  {
    Display.setTextSize(2);
    Display.setCursor(0, 0);
    Display.setTextColor(ST77XX_YELLOW);
    Display.println(F("   CONFIG"));

    Display.setTextSize(1);
    Display.setCursor(0, 25);
    if (x1 == 1) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("1 - Calibrar"));
    if (x1 == 2) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("2 - Bluetooth PTT"));
    if (x1 == 3) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("3 - Mensagens"));
    if (x1 == 4) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("4 - Relogio"));
    if (x1 == 5) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("5 - Voltar"));
    while (digitalRead(PinBOOT)) delay(100);
    if (!digitalRead(PinBOOT))
    {
      BotaoPRESS();
      if (OPCAO == "SELECIONA")
      {
        Display.fillScreen(ST77XX_BLACK);
        if (x1 == 1) CALIBRA();
        if (x1 == 2) BLUETOOTH();
        if (x1 == 3) MENSAGENS();
        if (x1 == 4) ACERTARHORA();
        if (x1 == 5) voltar = true;
        x1 = 1;
        Display.fillScreen(ST77XX_BLACK);
      }
      else
      if (OPCAO == "PROXIMO") 
      {
        x1++;
        if (x1 > 5) x1 = 1; 
      }
    }
  }
  Display.fillScreen(ST77XX_BLACK);
  CarregarTELA = true;
  Preencher    = true;
}

//===========================================================================================

void CALIBRA() 
{
  while (!digitalRead(PinBOOT)) delay(10);
  if (!BTconectado) BLUETOOTH();
  bool voltar = false;
  Display.setTextSize(2);
  Display.setCursor(0, 0);
  Display.setTextColor(ST77XX_YELLOW);
  Display.println(F("   CALIBRA"));
  Display.setTextSize(1);
  Display.setCursor(0, 25);
  Display.setTextColor(ST77XX_WHITE);
  Display.println(F("Para efetuar  a calibracao"));
  Display.println(F("execute cada passo exibido"));
  Display.println(F("na tela e pressione o BOOT"));
  Display.println(F(" para o proximo passo ser"));
  Display.println(F("        exibido."));
  Display.println();
  Display.setTextColor(ST77XX_GREEN);
  Display.println(F("Pressione BOOT"));
  while (digitalRead(PinBOOT)) delay(100);
  Display.fillScreen(ST77XX_BLACK);
  Display.setTextSize(2);
  Display.setCursor(0, 0);
  Display.setTextColor(ST77XX_YELLOW);
  Display.println(F("   CALIBRA"));
  Display.setTextSize(1);
  Display.setCursor(0, 25);
  Display.setTextColor(ST77XX_WHITE);
  Display.println(F("Coloque o ZELLO no usuario"));
  Display.println(F(" ECHO, o botao PTT deve"));
  Display.println(F("    estar LARANJA..."));
  Display.println(F("entao pressione BOOT para"));
  Display.println(F(" inicar a calibracao dos"));
  Display.println(F("    estados do ZELLO"));
  Display.println();
  Display.setTextColor(ST77XX_GREEN);
  Display.println(F("Pressione BOOT"));
  while (digitalRead(PinBOOT)) delay(100);
  Display.fillScreen(ST77XX_BLACK);
  Display.setTextSize(2);
  Display.setCursor(0, 0);
  Display.setTextColor(ST77XX_YELLOW);
  Display.println(F("   CALIBRA"));
  Display.setTextSize(1);
  Display.setCursor(0, 25);
  Display.setTextColor(ST77XX_WHITE);
  Display.print(F("Coletando dados..."));
  VALORES();
  Zon[0] = R;
  Zon[1] = G;
  Zon[2] = B;
  Zon[3] = C;
  Display.println();
  Display.setTextColor(ST77XX_YELLOW);
  Display.print(F("Zon =  "));
  Display.setTextColor(ST77XX_ORANGE);
  Display.print(Zon[0]);
  Display.print(F("  "));
  Display.print(Zon[1]);
  Display.print(F("  "));
  Display.print(Zon[2]);
  Display.print(F("  "));
  Display.print(Zon[3]);
  delay(1000);
  PTTzello(LIGA);
  delay(3000);
  VALORES();
  Ztx[0] = R;
  Ztx[1] = G;
  Ztx[2] = B;
  Ztx[3] = C;
  Display.println();
  Display.setTextColor(ST77XX_YELLOW);
  Display.print(F("Ztx =  "));
  Display.setTextColor(ST77XX_RED);
  Display.print(Ztx[0]);
  Display.print(F("  "));
  Display.print(Ztx[1]);
  Display.print(F("  "));
  Display.print(Ztx[2]);
  Display.print(F("  "));
  Display.print(Ztx[3]);
  delay(2000);
  PTTzello(DESLIGA);
  delay(2000);
  VALORES();
  Zrx[0] = R;
  Zrx[1] = G;
  Zrx[2] = B;
  Zrx[3] = C;
  Display.println();
  Display.setTextColor(ST77XX_YELLOW);
  Display.print(F("Zrx =  "));
  Display.setTextColor(ST77XX_GREEN);
  Display.print(Zrx[0]);
  Display.print(F("  "));
  Display.print(Zrx[1]);
  Display.print(F("  "));
  Display.print(Zrx[2]);
  Display.print(F("  "));
  Display.println(Zrx[3]);

  Display.setTextColor(ST77XX_WHITE);
  Display.println(F(""));
  Display.println(F("Salvando dados..."));
  int addr = 0;
  Display.setTextColor(ST77XX_YELLOW);
  Display.print(F("Zon =  "));
  Display.setTextColor(ST77XX_ORANGE);
  for (int i = 0; i < 4; i++) 
  {
    EEPROM.write(addr++, Zon[i]);
    Display.print(Zon[i]);
    Display.print(F("  "));
    delay(40);
  }
  Display.println();
  Display.setTextColor(ST77XX_YELLOW);
  Display.print(F("Ztx =  "));
  Display.setTextColor(ST77XX_RED);
  for (int i = 0; i < 4; i++) 
  {
    EEPROM.write(addr++, Ztx[i]);
    Display.print(Ztx[i]);
    Display.print(F("  "));
    delay(40);
  }
  Display.println(F(""));
  Display.setTextColor(ST77XX_YELLOW);
  Display.print(F("Zrx =  "));
  Display.setTextColor(ST77XX_GREEN);
  for (int i = 0; i < 4; i++) 
  {
    EEPROM.write(addr++, Zrx[i]);
    Display.print(Zrx[i]);
    Display.print(F("  "));
    delay(40);
  }
  Display.println(F(""));
  EEPROM.commit(); 
  delay(40);
  Display.setTextColor(ST77XX_GREEN);
  Display.println(F(""));
  Display.print(F("Calibracao EFETUADA!!!"));
  while (digitalRead(PinBOOT)) delay(10);
  Display.fillScreen(ST77XX_BLACK);
}

//===========================================================================================

void BLUETOOTH() 
{
  bool voltar2 = false;
  unsigned long TimerBLE = millis();
  while (!voltar2) 
  {
    Display.fillScreen(ST77XX_BLACK);
    Display.setTextSize(2);
    Display.setCursor(0, 0);
    Display.setTextColor(ST77XX_YELLOW);
    Display.println(F("  BLUETOOTH"));
    Display.setTextSize(1);
    Display.setCursor(0, 25);
    Display.setTextColor(ST77XX_WHITE);
    Display.println(F("Conectando o PTT Bluetooth"));
    Display.println(F("  No app ZELLO toque nos"));
    Display.println(F("3 PONTOS no canto superior"));
    Display.println(F("direito da tela, depois em"));
    Display.println(F("OPCOES, e enfim BOTAO PTT."));
    Display.println(F("No canto inferior direito "));
    Display.println(F(" toque no + CIRCULO AZUL"));
    Display.println();
    Display.setTextColor(ST77XX_GREEN);
    Display.println(F("Aguarde..."));
    Display.println();
    while ((!BTconectado) || (millis() > TimerBLE + 30000)) delay(100);
    if (!BTconectado) 
    {
      Display.setTextColor(ST77XX_RED);
      Display.println(F("FALHOU!!!"));
    }
    delay(2500);
    voltar2 = true;
  }
  
  Display.fillScreen(ST77XX_BLACK);
}

//===========================================================================================

void MENSAGENS()
{
  byte x2               = 1;
  byte VolumeA          = Volume;
  bool gravar           = false;
  bool voltar           = false;
  bool MENSAGEMativadaA = MENSAGEMativada;
  bool BEEPativadoA     = BEEPativado;
  bool HORAativadaA     = HORAativada;

  Display.fillScreen(ST77XX_BLACK);
  while (!voltar) 
  {
    Display.setTextSize(2);
    Display.setCursor(0, 0);
    Display.setTextColor(ST77XX_YELLOW);
    Display.println(F("  MENSAGENS"));

    Display.setTextSize(1);
    Display.setCursor(0, 25);
    if (x2 == 1) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    if (MENSAGEMativada) Display.println(F("1 - Desativar Mensagens"));
    else                 Display.println(F("1 - Ativar Mensagens"));
    if (x2 == 2) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    if (BEEPativado)     Display.println(F("2 - Desativar Rogerbeep"));
    else                 Display.println(F("2 - Ativar Rogerbeep"));
    if (x2 == 3) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    if (HORAativada)     Display.println(F("3 - Desativar Horas"));
    else                 Display.println(F("3 - Ativar Horas"));
    Display.println(F(""));
    Display.setTextColor(ST77XX_YELLOW);
    Display.print  (F("    Volume atual: "));
    Display.println(Volume);
    if (x2 == 4) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("4 - Volume +"));
    if (x2 == 5) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("5 - Volume -"));
    Display.println(F(""));
    if (x2 == 6) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("6 - Voltar"));
    while (digitalRead(PinBOOT)) delay(100);

    if (!digitalRead(PinBOOT))
    {
      BotaoPRESS();
      if (OPCAO == "SELECIONA")
      {
        Display.fillScreen(ST77XX_BLACK);
        if (x2 == 1) MENSAGEMativada = !MENSAGEMativada;
        if (x2 == 2) BEEPativado     = !BEEPativado;
        if (x2 == 3) HORAativada     = !HORAativada;
        if (x2 == 4) 
        {
          Volume++;
          if (Volume > 30) Volume = 1;
        }
        if (x2 == 5)
        {
          Volume--;
          if (Volume < 1) Volume = 30;
        }
        if (x2 == 6) voltar = true;
      }
      else
      if (OPCAO == "PROXIMO") 
      {
        x2++;
        if (x2 > 6) x2 = 1; 
      }
    }
  }
  if (VolumeA != Volume)
  {
    EEPROM.write(12, Volume);
    gravar = true; 
    DFPlayer.volume(Volume);
    delay(500);
    DFPlayer.volume(Volume);
  }
  if (MENSAGEMativadaA != MENSAGEMativada)
  {
    EEPROM.write(13, MENSAGEMativada);
    gravar = true;
    MENSAGEMativadaA = MENSAGEMativada;
  }
  if (BEEPativadoA != BEEPativado)
  {
    EEPROM.write(14, BEEPativado);
    gravar = true;
    BEEPativadoA = BEEPativado;
  }
  if (HORAativadaA != HORAativada)
  {
    EEPROM.write(15, HORAativada);
    gravar = true; 
    HORAativadaA = HORAativada;
  }
  if (gravar) EEPROM.commit(); 
  gravar = false; 
  CarregarTELA = true;
  Display.fillScreen(ST77XX_BLACK);
}

//===========================================================================================

void ACERTARHORA()
{
  Ha             = H;
  Ma             = M;
  Preencher      = true;
  byte x2        = 1;
  bool voltar2   = false;
  Display.fillScreen(ST77XX_BLACK);
  while (!voltar2) 
  {
    Display.setTextSize(2);
    Display.setCursor(0, 0);
    Display.setTextColor(ST77XX_YELLOW);
    Display.println(F("RELOGIO"));
    if ((M != Ma) || (H != Ha) || (Preencher))
    {
      if (!Preencher) HORAcerta = true;
      Preencher = false;
      Display.setTextSize(2);
      Display.setCursor(100, 0);
      Display.setTextColor(ST77XX_BLACK);
      if (Ha<10) Display.print(F("0"));
      Display.print(Ha);
      Display.print(F(":"));
      if (Ma<10) Display.print(F("0"));
      Display.print(Ma);
      Display.setCursor(100, 0);
      if (HORAcerta) Display.setTextColor(ST77XX_WHITE);
      else           Display.setTextColor(ST77XX_MAGENTA);
      if (H<10) Display.print(F("0"));
      Display.print(H);
      Display.print(F(":"));
      if (M<10) Display.print(F("0"));
      Display.print(M);
      Ha = H;
      Ma = M;
    }
    Display.setTextSize(1);
    Display.setCursor(0, 25);
    if (x2 == 1) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("1 - Hora +5"));
    if (x2 == 2) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("2 - Hora +1"));
    Display.println(F(""));
    if (x2 == 3) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("3 - Minuto +10"));
    if (x2 == 4) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("4 - Minuto +1"));
    Display.println(F(""));
    if (x2 == 5) Display.setTextColor(ST77XX_GREEN);
    else Display.setTextColor(ST77XX_WHITE);
    Display.println(F("5 - Salvar e voltar"));
    while (digitalRead(PinBOOT)) delay(100);
    if (!digitalRead(PinBOOT))
    {
      BotaoPRESS();
      if (OPCAO == "SELECIONA")
      {
        if (x2 == 1) H = H + 5;
        if (x2 == 2) H = H + 1;
        if (H  > 23) H = 0;
        if (x2 == 3) M = M + 10;
        if (x2 == 4) M = M + 1;
        if (M  > 59) M = 0;
        if (x2 == 5) voltar2 = true;
        segundo = millis();
      }
      else
      if (OPCAO == "PROXIMO") 
      {
        x2++;
        if (x2 > 5) x2 = 1; 
      }
    }
  }
  Ha        = H-1;
  Ma        = M-1;
}

//===========================================================================================

void PTTzello(bool acaoZ) 
{
  if (acaoZ) SerialBT.print("+PTTS=P");
  else       SerialBT.print("+PTTS=R");
}

//===========================================================================================

void PTTradio(bool acaoR) 
{
  if (acaoR)
  {
    digitalWrite(PinoPTTR, true); 
    PTTRacionado = true;
  }
  else
  {
    digitalWrite(PinoPTTR, false);
    PTTRacionado = false;
  }
  ESTADOLINK();
}

//===========================================================================================

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) 
{
  if (event == ESP_SPP_SRV_OPEN_EVT) 
  {
    Display.setTextColor(ST77XX_BLUE);
    Display.println(F("PTT ZELLO conectado!"));
    for (int i = 0; i < 6; i++) 
    {
      Serial.printf("%02X", param->srv_open.rem_bda[i]);
      if (i < 5) 
      {
        Serial.println(F(":"));
        BTconectado = true;
        CarregarTELA = true;
      }
    }
  } else if (event == ESP_SPP_SRV_STOP_EVT) 
  {
    Serial.println(F("Servidor Serial parado!"));
  }
}

//===========================================================================================

String leStringSerial() 
{
  String conteudo = "";
  while (SerialBT.available() > 0)  // Enquanto receber algo pela serial
  {
    caractere = SerialBT.read();  // Lê byte da serial
    if (caractere != '\n')        // Ignora caractere de quebra de linha
    {
      conteudo.concat(caractere);  // Concatena valores
    }
    delay(10);  // Aguarda buffer serial ler próximo caractere
  }
  Serial.println(F("Recebi: "));
  Serial.println(conteudo);
  return conteudo;
}
