/*

Interface RoboCOP - GMRS Radio x ZELLO - by 2RUGE0007 Renato Druziani 2023

This is the design of a controller for a GMRS radio gateway for the zello app.
The idea is to make it simple to assemble without the need to open the radio or smartphone to collect signals,
I use LDRs on the cell phone screen and LED tx/rx on the HT.
Automated configuration steps and created constant self-tuning, reducing maintenance.

*/
 
// VERSÃO do Software --------------------------------------------------------------------------------------------------------------------
 #define Versao          "5.b.0"

// Incluindo bibliotecas utilizadas-----------------------------------------------------------------------------------------------------
 #include <Arduino.h>
 #include <WiFi.h>
 #include <DNSServer.h>
 #include <WebServer.h>
 #include <WiFiManager.h> 
 #include <NTPClient.h>
 #include <Adafruit_SSD1306.h>
 #include <SoftwareSerial.h>
 #include <EEPROM.h>
 #include <DFRobotDFPlayerMini.h>
 
// relação MENSAGEM  com  o  NÚMERO DO ARQUIVO no cartão SD-------------------------------------------------------------------------------
 #define RogerBEEPlocal   25               //Arquivo 01~24 correspondem aos áudios de HORA e MEIA: 01 = 00:30h ~ 24 = 23:30h
 #define RogerBEEPweb     26               //Arquivo 25~31 correspondem aos áudios OPERACIONAIS
 #define RogerBEEPerro    27               
 #define TelaAPAGOU       28
 #define LinkOFFLINE      29
 #define LinkONLINE       30
 #define ZelloFECHOU      31
 #define PrimeiraVinheta  32                //Arquivo 32 ou + correspondem aos áudios de VINHETAS diversas
// definições PORTAS-----------------------------------------------------------------------------------------------------------------------
 #define BotaoBOOT        0
 #define LED              2
 #define TocandoMP3       4
 #define PTTradio         23
 #define PTTzello         18
 #define SinalLDRradio    34
 #define SinalLDRzello    35
// Definições dos endereços dos valores salvos na EEPROM-----------------------------------------------------------------------------------
 #define EEPROM_SIZE      64
 #define evolumeVOZ       10
 #define evon             20
 #define evtx             30
 #define evrx             40
// OUTRAS definições-----------------------------------------------------------------------------------------------------------------------
 #define NCo              20
 #define NCc              1250
 #define TempoDoPulso     300
 #define TempoDisplay     600000                    // tempo de display aceso
 #define TempoSalvaAjuste 3600000
 #define CincoMinutos     300000
 
// Configurando VOZ------------------------------------------------------------------------------------------------------------------------
 SoftwareSerial ComunicacaoSerial(16, 17);     // Pinos 16 RX, 17 TX para comunicação serial com o DFPlayer
 DFRobotDFPlayerMini LOCUTORA;                 // nomeando DFPlayer como "LOCUTORA"
 
// Configurando Display--------------------------------------------------------------------------------------------------------------------
 Adafruit_SSD1306 display(128, 64, &Wire);
 
// Definindo Servidor de HORA -------------------------------------------------------------------------------------------------------------
 WiFiUDP ntpUDP;
 NTPClient ntp(ntpUDP);
 
// Declarando Gerenciador de WIFI --------------------------------------------------------------------------------------------------------
 WiFiManager wifiManager;
 
// Variáveis, arrays, funções, etc--------------------------------------------------------------------------------------------------------
 byte          H                = 0;
 byte          M                = 0;
 byte          S                = 0;
 byte          c                = 0;
 byte          l                = 0;
 byte          sa               = 0;
 byte          opcao            = 1;
 byte          Volume           = 0;
 byte          Vinheta          = PrimeiraVinheta;
 byte          Narquivos        = 0;
 byte          TipoMOSTRADOR    = 1;
 int           LeituraLDRzello  = 0;
 int           VON              = 0;
 int           L1               = 0;
 String        S1               = "XX";
 int           VRX              = 0; 
 int           L2               = 0;
 String        S2               = "XX";
 int           VTX              = 0;
 int           DifMedia         = 0;
 bool          shouldSaveConfig = false;
 bool          WIFIok           = false;
 bool          LocutoraOK       = false;
 bool          DisplayAceso     = true;
 bool          MensagemTA       = true;
 bool          MensagemTI       = true;
 bool          MensagemOFF      = true;
 bool          MensagemON       = false;
 bool          LEDligado        = true;
 bool          sobe             = true;
 bool          lado             = true;
 bool          FalouAsHORAS     = false;
 bool          BOOTacionado     = false;
 bool          RADIO            = false;
 unsigned long Time             = millis();
 unsigned long Crono            = millis();
 unsigned long Crono1           = millis();
 unsigned long Crono2           = millis();
 unsigned long Crono3           = millis();
 unsigned long CronoON          = millis();
 unsigned long CronoRX          = millis();
 unsigned long CronoTX          = millis();
 unsigned long CronoPPToff      = millis();
 unsigned long AtualizaDisplay  = millis();
 unsigned long TimerPisca       = millis();
 unsigned long TimerLedHT       = millis();
 unsigned long TimerDisplay     = millis();
 unsigned long TimerAutoAjuste  = millis();
 String        statusRADIO      = "XX";
 String        statusZELLO      = "XX";
 String        statusLINK       = "XX";
 String        HC               = "-";
 int           MediaLDR[NCc];
 int           MediaON[NCo];
 int           MediaRX[NCo];
 int           MediaTX[NCo];
 long          mmLDR();
 long          mmON();
 long          mmRX();
 long          mmTX();
 long          ESTADOzello();
 long          PTTzelloON();
 long          PTTzelloOFF();
 long          PTTradioON();
 long          PTTradioOFF();
 long          Display();
 long          Pisca();
 long          AjustarRELOGIO();
 long          MenuSETUP1();
 long          WiFiconfig();
 long          ADDPTTZello();
 long          PosicionarLDRZello(); 
 long          CalibrarLDRZello(); 
 long          AjusteVOLUMEvoz(); 


  // ---------------------------------------------------------------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  ComunicacaoSerial.begin(9600);
  
 // Gerenciador de WIFI -------------------------------------------------------------------------------------------------------------------
  wifiManager.setDebugOutput(false);              // True = mensagens de Debug do wifi vão aparecer no monitor serial
  wifiManager.setConfigPortalTimeout(60);
  wifiManager.setAPCallback(configModeCallback); //callback para quando entra em modo de configuração AP
  WiFi.begin();//wifiManager.setSaveConfigCallback(saveConfigCallback);
  //wifiManager.autoConnect("LINK-ROBOCOP-setup","12345678");
  
 // DEFININDO PINOS -----------------------------------------------------------------------------------------------------------------------
  pinMode(TocandoMP3,    INPUT);
  pinMode(SinalLDRzello, INPUT_PULLDOWN);
  pinMode(SinalLDRradio, INPUT_PULLDOWN);
  pinMode(LED,           OUTPUT);
  pinMode(PTTzello,      OUTPUT);
  pinMode(PTTradio,      OUTPUT);
  pinMode(BotaoBOOT,     INPUT_PULLUP);
  
 // INICIANDO DISPLAY ---------------------------------------------------------------------------------------------------------------------
  int INICIO = 50;
  Serial.print(F("Controladora de Gateway ROBOCOP v"));
  Serial.println(Versao);
  Serial.println(F("-"));
  Serial.println(F("Iniciando DISPLAY"));
  while(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    delay (50);
    digitalWrite (LED, 0);
    Serial.print(F("-"));
    display.display();
    delay (50);
    digitalWrite (LED, 1);
  }
  display.display();
  display.clearDisplay();
  delay(2000);
  Serial.end();
  digitalWrite (LED, 1);
  display.setCursor (0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.print  (F(" CTRL RoboCOP v"));
  display.println(Versao);
  display.display();
  display.println(F("---------------------"));
  display.display();
  delay(INICIO * 10); 
  digitalWrite (LED, 0);
  display.println(F("DISPLAY  OK"));
  display.display();
  delay(INICIO * 4); 

 // INICIANDO MP3 -------------------------------------------------------------------------------------------------------------------------
  digitalWrite (LED, 1);
  display.println(F("INICIANDO VOZ..."));
  LocutoraOK = LOCUTORA.begin(ComunicacaoSerial);
  display.display();
  delay(INICIO * 10);
  digitalWrite (LED, 0);
  if (!LocutoraOK)      //Use softwareSerial to communicate with mp3.
  {
    display.clearDisplay();
    display.setCursor (0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.println(F("   INICIANDO VOZ..."));
    display.println(F("---------------------"));
    display.println(F(" IMPOSSIVEL INICIAR "));
    display.println(F(" !!! Por  favor !!!"));
    display.println(F("  Cheque CARTAO SD"));
    display.display();
    delay(INICIO);
    byte X = 1;    
    while ((!LocutoraOK) & (X != 6))
    {
      digitalWrite (LED, 1);
      LocutoraOK = LOCUTORA.begin(ComunicacaoSerial);
      display.fillRect(0, 50,   128, 64, SSD1306_BLACK);
      display.display();
      display.setCursor (0, 50);
      display.print  (F("Tentativa numero: "));
      display.print  (X);
      display.println(F("/5"));
      display.display();   
      delay (100);
      digitalWrite (LED, 0);   
      delay (100);   
      X++;   
    }
    display.clearDisplay();
    display.setCursor (0, 0);
    display.println(F("      ATENCAO!!      "));
    display.println(F("---------------------"));
    display.println(F("IMPOSSIVEL INICIAR DF"));
    display.println(F("Iniciando gateway sem"));
    display.println(F("Rogerbeep ou mensagem"));
    display.println(F("---------------------"));
    display.display();
    display.clearDisplay();
    delay(3000);
  }
  if (LocutoraOK)
  {
    LOCUTORA.outputDevice(DFPLAYER_DEVICE_SD);   // configurando a media usada = SDcard
    LOCUTORA.EQ(DFPLAYER_EQ_BASS);               // configurando equalizador = BASS
    Narquivos = LOCUTORA.readFileCounts();       // contando número de arquivos no SD
    while (Narquivos == 255)
    {
      Narquivos = LOCUTORA.readFileCounts();
      delay (500);
    }
    Volume = EEPROM.readByte(evolumeVOZ);        // Ajustando o Volume armazenado
    LOCUTORA.setTimeOut(500);                    // ajustando o timeout da comunicação serial 500ms
    LOCUTORA.volume(Volume);                     // Ajustando o volume (0~30).
    display.println(F("VOZ HABILITADA!"));
    display.println();
    display.display();
    delay(750);
    display.print(F("Arquivos no SD = "));
    display.println(Narquivos); 
    display.display();
    delay(500);
    display.print(F("Volume da VOZ  = "));
    display.println(Volume);
    display.display();
    display.clearDisplay();
    delay(2000);
  }
  
 // MODO CONFIGURAÇÃO ---------------------------------------------------------------------------------------------------------------------
  display.clearDisplay();
  display.setCursor (0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(F("     CONFIGURACAO    "));
  display.println(F("---------------------"));
  display.println();
  display.println(F("---------------------"));
  display.println(F("Para entrar  no SETUP"));
  display.println(F("   pressione BOOT    "));
  display.println(F("---------------------"));
  display.display();
  for (byte x=0; x<22; x++)
  { 
    if (!digitalRead(BotaoBOOT)) INICIO = 500;
    digitalWrite (LED, 1);
    delay (50);
    digitalWrite (LED, 0);
    delay (50);    
  }  
  display.display();
  delay(INICIO); 
  if (INICIO == 500) MenuSETUP1(); 
  delay(INICIO * 4);
  
 // CARREGANDO DADOS DA EEPROM ------------------------------------------------------------------------------------------------------------
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor (0, 0);
  display.println(F("  CARREGANDO DADOS"));
  display.println(F("---------------------"));
  display.println();
  VON = EEPROM.readInt (evon);
  VTX = EEPROM.readInt (evtx);
  VRX = EEPROM.readInt (evrx);
  display.print(F("Valor ON: "));
  display.println(VON);
  display.println();
  display.print(F("Valor TX: "));
  display.println(VTX);
  display.println();
  display.print(F("Valor RX: "));
  display.println(VRX);
  display.display();
  display.clearDisplay();
  for(int p=0; p<NCo; p++) MediaON[p] = VON;
  for(int p=0; p<NCo; p++) MediaRX[p] = VRX;
  for(int p=0; p<NCo; p++) MediaTX[p] = VTX;
  digitalWrite (LED, 1);
  delay (INICIO * 6);
  digitalWrite (LED, 0);


  delay(INICIO * 4);
  WiFiconfig();
 // AJUSTANDO O RELÓGIO -------------------------------------------------------------------------------------------------------------------
  if (WIFIok) AjustarRELOGIO();
  BOOTacionado = false;
}

//----------------------------------------------------------------------------------------------------------------------------------------
void loop()
{
  Display();
  Pisca();

  // TELA APAGADA -------------------------------------------------------------------------------------------------------------------------  
  if (statusZELLO == "TA")
  {
    if ((!MensagemTA) & (LocutoraOK))
    {
      PTTradioON();
      Crono = millis();
      while ((millis() - Crono) < 350) Display();
      LOCUTORA.play (TelaAPAGOU);
      Crono = millis();
      while ((millis() - Crono) < 1000) Display();
      while (!digitalRead(TocandoMP3)) Display();
      LOCUTORA.play (RogerBEEPerro);
      Crono = millis();
      while ((millis() - Crono) < 250) Display();
      while (!digitalRead(TocandoMP3)) Display();
      PTTradioOFF();
      MensagemTA  = true;
      MensagemOFF = false;
      MensagemON  = false;
      MensagemTI  = false;
    }
    while (RADIO)
    {
      Display();
      MensagemTA = false;
    }
  }  

  // ZELLO OFFLINE ------------------------------------------------------------------------------------------------------------------------
  if (statusZELLO == "OF")
  {
    if ((!MensagemOFF) & (LocutoraOK))
    {
      PTTradioON();
      Crono = millis();
      while ((millis() - Crono) < 350) Display();
      LOCUTORA.play (LinkOFFLINE);
      Crono = millis();
      while ((millis() - Crono) < 1000) Display();
      while (!digitalRead(TocandoMP3)) Display();
      LOCUTORA.play (RogerBEEPerro);
      Crono = millis();
      while ((millis() - Crono) < 250) Display();
      while (!digitalRead(TocandoMP3)) Display();
      PTTradioOFF();
      MensagemTA  = false;
      MensagemOFF = true;
      MensagemON  = false;
      MensagemTI  = false;
    }
    while (RADIO)
    {
      Display();
      MensagemOFF = false;
    }
  }  

  // ZELLO ONLINE -------------------------------------------------------------------------------------------------------------------------
  if (statusZELLO == "ON")
  {  
    if ((!MensagemON) & (LocutoraOK))
    {
      PTTradioON();
      Crono = millis();
      while ((millis() - Crono) < 200) Display();
      LOCUTORA.play (LinkONLINE);
      Crono = millis();
      while ( digitalRead(TocandoMP3)) Display();
      while (!digitalRead(TocandoMP3)) Display();
      PTTradioOFF();
      MensagemTA  = false; 
      MensagemOFF = false;
      MensagemON  = true;
      MensagemTI  = false;
    }
     
    // Aplicando HORA E MEIA ---------------------------------------------------------------------------------------------------------------
    if ((WIFIok) & (M == 30) & (!FalouAsHORAS) & (HC=="+") & (LocutoraOK))
    {
      PTTradioON();
      Crono = millis();
      while ((millis() - Crono) < 400) Display();
      LOCUTORA.play (H+1);
      while ( digitalRead(TocandoMP3)) Display();
      while (!digitalRead(TocandoMP3)) Display();
      LOCUTORA.play (RogerBEEPlocal);
      Crono = millis();
      while ( digitalRead(TocandoMP3)) Display();
      while (!digitalRead(TocandoMP3)) Display();
      PTTradioOFF();
      FalouAsHORAS = true;
    }   
    
    // Aplicando MENSAGENS REGULARES quando ocioso ----------------------------------------------------------------------------------------
    if ((((M == 15) | (M == 45)) & (S == 0) & (Narquivos >= PrimeiraVinheta)) | (BOOTacionado) & (LocutoraOK))
    {
      PTTradioON();
      Crono = millis();
      while ((millis() - Crono) < 200) Display();
      LOCUTORA.play (Vinheta);
      Vinheta++;
      if (Vinheta > (Narquivos)) Vinheta = PrimeiraVinheta;
      while ( digitalRead(TocandoMP3)) Display();
      while (!digitalRead(TocandoMP3)) Display();
      LOCUTORA.play (RogerBEEPlocal);
      while ( digitalRead(TocandoMP3)) Display();
      while (!digitalRead(TocandoMP3)) Display();
      PTTradioOFF();
      BOOTacionado = false;
    }  
      
    // LINK em recepção -------------------------------------------------------------------------------------------------------------------
    if ((RADIO) & ((millis() - TimerLedHT) > 2000))
    {
      PTTzelloON();
      String BEEP = "erro";
      Crono = millis();
      while (RADIO)
      {
        Display();
        if (statusZELLO == "TX") BEEP = "local";
        if ((millis() - Crono) > CincoMinutos) RADIO = false;
      }
      PTTzelloOFF();
      if (LocutoraOK)
      {
        PTTradioON();
        Crono = millis();
        while ((millis() - Crono) < 50) Display();
        if (BEEP == "local") LOCUTORA.play (RogerBEEPlocal);
        if (BEEP == "erro")  LOCUTORA.play (RogerBEEPerro);  
        while ( digitalRead(TocandoMP3)) Display();
        while (!digitalRead(TocandoMP3)) Display();
        PTTradioOFF();
      }
    } 
  }

  // ZELLO RECEBENDO TRANSMISSÃO ---------------------------------------------------------------------------------------------------------
  if (statusZELLO == "RX")
  {
    PTTradioON();
    while (statusZELLO == "RX") Display();
    if (LocutoraOK)
    {
      LOCUTORA.play (RogerBEEPweb);
      while ( digitalRead(TocandoMP3)) Display();
      while (!digitalRead(TocandoMP3)) Display(); 
    }
    PTTradioOFF();
  }   
    
  // ZELLO FECHADO - Tela Inicial -------------------------------------------------------------------------------------------------------- 
  if (statusZELLO == "TI")
  {
    if ((!MensagemTI) & (LocutoraOK))
    {
      PTTradioON();
      Crono = millis();
      while ((millis() - Crono) < 350) Display();
      LOCUTORA.play (ZelloFECHOU);
      Crono = millis();
      while ((millis() - Crono) < 1000) Display();
      while (!digitalRead(TocandoMP3)) Display();
      LOCUTORA.play (RogerBEEPerro);
      Crono = millis();
      while ((millis() - Crono) < 250) Display();
      while (!digitalRead(TocandoMP3)) Display();
      PTTradioOFF();
      MensagemTA  = false; 
      MensagemOFF = false;
      MensagemON  = false;
      MensagemTI  = true;
    }
    while (RADIO)
    {
      Display();
      MensagemTI = false;
    }
  }  

  // VERIFICANDO PTT ----------------------------------------------------------------------------------------------------------------------  
  if ((statusZELLO == "TX") & (!RADIO) & ((millis() - CronoPPToff) > 2000))
  {
    PTTzelloOFF();
  }    
  if (statusZELLO != "TX") CronoPPToff  = millis();
}

// PISCA PISCA-----------------------------------------------------------------------------------------------------------------------------
long Pisca()
{
  if (statusZELLO != "ON") LEDligado = 0;
  else LEDligado = 1;
  if ((millis() - TimerPisca) > 450)
  {
    digitalWrite(LED, LEDligado);
  }
  if ((millis() - TimerPisca) > 500) 
  {  
    digitalWrite(LED, !LEDligado);
    TimerPisca = millis();
  }
}

// ESTADOzello - Função que determina o estado atual do LINK-------------------------------------------------------------------------------
long ESTADOzello() // TelaAPAGOU TA - Offline OF - RX - TX - Esperando ON - TelaInicial TI
{
  LeituraLDRzello = mmLDR();
  
  if ( (analogRead(SinalLDRradio)) > 100 ) RADIO = true; 
  else RADIO = false;
  if (RADIO) statusRADIO = "RX";   
  if (VRX > VTX)          // Em telas IPS o valor obtido em RX é maior que o valor em TX 
  {
    S1 = "RX";
    S2 = "TX";
    L1 = VRX + ((VON - VRX) / 2);
    L2 = VTX + ((VRX - VTX) / 2);
    DifMedia =  (VON - VTX) / 2;
  } 
  else if (VTX > VRX)  // Em telas OLED o valor obtido em TX é maior que o valor em RX 
  {
    S1 = "TX";
    S2 = "RX";
    L1 = VTX + ((VON - VTX) / 2);
    L2 = VRX + ((VTX - VRX) / 2);
    DifMedia = ((VON - VRX) / 2);
  }   
  if (MediaLDR[1] == MediaLDR[NCc-1])
  {
   if (statusZELLO != "ON") CronoON = millis();
   if ((statusZELLO == "ON") & ((millis() - CronoON) > 30000))
   {  
      if ((mmON() > (VON - (VON / (100 * 5))))  &  (mmON() < (VON + (VON / (100 * 5))))) VON = mmON();     //escluindo discrepancias acima de 5% de diferença da amostragem anterior
      CronoON = millis(); 
   }
   if (statusZELLO != "RX") CronoRX = millis();     
   if ((statusZELLO == "RX") & ((millis() - CronoRX) > 15000))
   {  
      if ((mmRX() > (VRX - (VRX / (100 * 3))))  &  (mmRX() < (VRX + (VRX / (100 * 3))))) VRX = mmRX();     //escluindo discrepancias acima de 3% de diferença da amostragem anterior
      CronoRX = millis() - 12000; 
   } 
   if (statusZELLO != "TX") CronoTX = millis(); 
   if ((statusZELLO == "TX") & ((millis() - CronoTX) > 15000))
   {  
      if ((mmTX() > (VTX - (VTX / (100 * 3))))  &  (mmTX() < (VTX + (VTX / (100 * 3))))) VTX = mmTX();     //escluindo discrepancias acima de 3% de diferença da amostragem anterior
      CronoTX = millis() - 12000; 
   }
   if (millis() - TimerAutoAjuste > TempoSalvaAjuste)     // Hora em hora verifica e salva na EEPROM os dados ajustados
   {
      if (EEPROM.readInt (evon) != VON) EEPROM.writeInt (evon, VON);               
      if (EEPROM.readInt (evrx) != VRX) EEPROM.writeInt (evrx, VRX);
      if (EEPROM.readInt (evtx) != VTX) EEPROM.writeInt (evtx, VTX);
      EEPROM.commit();  
      TimerAutoAjuste = millis();
   }
   if ((LeituraLDRzello > L1) & (LeituraLDRzello < (L1 + (DifMedia * 1.1)))) statusZELLO = "ON";
   if ((LeituraLDRzello < L1) & (LeituraLDRzello > L2))                      statusZELLO = S1; // TX ou RX dependendo do caso
   if ((LeituraLDRzello < L2) & (LeituraLDRzello > (L2 - (DifMedia * 1.1)))) statusZELLO = S2; // TX ou RX dependendo do caso
   if ((LeituraLDRzello < (L2 - (DifMedia * 1.2))) & (LeituraLDRzello > 9))  statusZELLO = "OF";
   if  (LeituraLDRzello < 10)                                                statusZELLO = "TA";
   if  (LeituraLDRzello > (L1 + (DifMedia * 1.1)))                           statusZELLO = "TI";
   statusLINK = statusZELLO;
   if  (statusZELLO == "RX") { statusRADIO = "TX"; statusLINK  = "TX"; }
   if  (statusZELLO == "ON")   statusRADIO = "OF";
   if  (statusZELLO == "TX")   statusLINK  = "RX";
  }     
}

// LDR - FUNÇÃO MEDIA MOVEL ---------------------------------------------------------------------------------------------------------------
long mmLDR()
{
  for(int x=NCc-1; x>0; x--) MediaLDR[x] = MediaLDR[x-1];
  MediaLDR[0] = analogRead (SinalLDRzello);
  delay(0.9);  
  long contador = 0;
  for (int x=0; x<NCc; x++) contador += MediaLDR[x];
  return (contador/NCc);
}

// VON - FUNÇÃO MEDIA MOVEL ---------------------------------------------------------------------------------------------------------------
long mmON()
{
  for(int x= NCo-1; x>0; x--) MediaON[x] = MediaON[x-1];
  MediaON[0] = mmLDR();
  long contador = 0;
  for (int x=0; x<NCo; x++) contador += MediaON[x];
  return (contador/NCo);
}

// VRX - FUNÇÃO MEDIA MOVEL ---------------------------------------------------------------------------------------------------------------
long mmRX()
{
  for(int x= NCo-1; x>0; x--) MediaRX[x] = MediaRX[x-1];
  MediaRX[0] = mmLDR();
  long contador = 0;
  for (int x=0; x<NCo; x++) contador += MediaRX[x];
  return (contador/NCo);
}

// VTX - FUNÇÃO MEDIA MOVEL ---------------------------------------------------------------------------------------------------------------
long mmTX()
{
  for(int x= NCo-1; x>0; x--) MediaTX[x] = MediaTX[x-1];
  MediaTX[0] = mmLDR();
  long contador = 0;
  for (int x=0; x<NCo; x++) contador += MediaTX[x];
  return (contador/NCo);
}

// FUNÇÃO Liga o PTT ZELLO ----------------------------------------------------------------------------------------------------------------
long PTTzelloON()
{
  digitalWrite (PTTzello, HIGH);
  Crono = millis();
  while ((millis() - Crono) < TempoDoPulso) Display();
  digitalWrite (PTTzello, LOW);
  Crono = millis();
  while (statusZELLO != "TX")
  {
    Display();
    if ((millis() - Crono) > 4000) PTTzelloON();
  }
}

// FUNÇÃO Desliga o PTT ZELLO -------------------------------------------------------------------------------------------------------------
long PTTzelloOFF()
{
  digitalWrite (PTTzello, HIGH);
  Crono = millis();
  while ((millis() - Crono) < TempoDoPulso) Display();
  digitalWrite (PTTzello, LOW);
  while (statusZELLO == "TX")
  {
    Display();
    if ((millis() - Crono) > 4000) PTTzelloOFF();
  }
}

// FUNÇÃO Liga o PTT RADIO ----------------------------------------------------------------------------------------------------------------
long PTTradioON()
{
  digitalWrite (LED,      HIGH);
  digitalWrite (PTTradio, HIGH);
  statusRADIO = "TX";
}

// FUNÇÃO Desliga o PTT RADIO -------------------------------------------------------------------------------------------------------------
long PTTradioOFF()
{
  digitalWrite (LED,      LOW);
  digitalWrite (PTTradio, LOW);
  Crono = millis();
  while ((millis() - Crono) < 1000) Display();
  TimerLedHT = millis();
}

// FUNÇÃO AJUSTE HORA CERTA  --------------------------------------------------------------------------------------------------------------
long AjustarRELOGIO()
{  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F(" AJUSTANDO  A  HORA "));
  Crono = millis();
  while ((millis() - Crono) < 5000) 
  {
    delay(250);
    display.print(F("-"));
    display.display();
    if (WiFi.status() == WL_CONNECTED)
    {
      Crono = 0;
      display.println(F("-"));
      display.println(F("  WIFI  CONECTADO!  "));
      display.display();
      delay(500);
      display.println(F("COLETANDO HORA CERTA")); 
      display.display();
      ntp.begin();                               // OBTENDO HORA CERTA 
      Crono = millis();
      while ((millis() - Crono) < 150) Display;
      ntp.setTimeOffset(-10800);
      Crono = millis();
      while ((millis() - Crono) < 150) Display;
      if (ntp.update()) 
      {
        H  = ntp.getHours();
        M  = ntp.getMinutes();
        S  = ntp.getSeconds();
        HC = "+"; 
        display.println(F("-------SUCESSO!-----")); 
        display.display();
      }
      else
      {
        HC = "-"; 
        display.println(F("------FALHOU!-------"));
      } 
      Crono = 0;     
    }   
  }   
  display.display();
  Crono = millis();
  while ((millis() - Crono) < 1000) Display();
  display.clearDisplay();
  display.display();
}

// FUNÇÃO TELA LEITURAS EXIBIÇÃO ----------------------------------------------------------------------------------------------------------
long Display()
{
  ESTADOzello();
  // Cronometro RELOGIO
  if (millis() > Time) { S++; Time+=1000; }
  if (H > 23) { H=0; }
  if (M > 59) { H++; M=0; FalouAsHORAS=false; }
  if (S > 59) { M++; S=0; }
  if ((H==0) & (M==1) & (S==0) & (HC=="+")) HC="-";
  if ((M==2) & (S==0) & (HC=="-") & (WIFIok)) AjustarRELOGIO();

  // ACIONANDO O Botão de ação ------------------------------------------------------------------------------------------------------------
  if (!digitalRead(BotaoBOOT))
  {
    delay (200);
    Crono = millis();
    while (!digitalRead(BotaoBOOT))
    {
      if ((millis() - Crono) > 5000) ESP.restart(); 
    }
    if (millis() - TimerDisplay < TempoDisplay) TipoMOSTRADOR++;
    TimerDisplay = millis();
    if (TipoMOSTRADOR > 4) 
    {
      if (TipoMOSTRADOR == 5) BOOTacionado = true;
      TipoMOSTRADOR = 1;     
    }
  }

  // MOSTRADOR   
  if ((millis() - AtualizaDisplay) > 50)
  {    
    if ((millis() - TimerDisplay > TempoDisplay) & (statusZELLO !="ON")) TimerDisplay = (millis() - TempoDisplay + 4000);
    if ((millis() - TimerDisplay < TempoDisplay) | (statusZELLO !="ON"))   
    {   
      display.setTextColor(SSD1306_WHITE); 
      display.setTextSize(1);
      int Leitura  = map (LeituraLDRzello, (L2 - DifMedia), (L1 + DifMedia), 0, 128);
      int limite1  = map (L1             , (L2 - DifMedia), (L1 + DifMedia), 0, 128);
      int limite2  = map (L2             , (L2 - DifMedia), (L1 + DifMedia), 0, 128);
      int colunaON = map (VON            , (L2 - DifMedia), (L1 + DifMedia), 0, 128);
      display.drawRect ( 0,      0, 128    ,  8, SSD1306_WHITE);
      display.fillRect ( 2,      2, Leitura,  4, SSD1306_WHITE);
      display.drawLine (limite1, 0, limite1, 15, SSD1306_WHITE);
      display.drawLine (limite2, 0, limite2, 15, SSD1306_WHITE);
      display.setCursor(108, 9); 
      display.print(F("ON"));
      display.setCursor(limite1 - 15, 9); 
      display.print(S1);
      display.setCursor(limite2 - 15, 9); 
      display.print(S2);


  
      // Mostrador RELOGIO E STATUS
      if (TipoMOSTRADOR == 1)
      {
        display.setTextSize(2);
        display.setCursor(8, 19);
        if (H < 10) display.print(F("0")); 
        display.print(H);
        display.print(F("h"));
        if (M < 10) display.print(F("0"));
        display.print(M);
        display.print(F("m"));
        if (S < 10) display.print(F("0"));
        display.print(S);
        display.print(F("s"));
        display.setCursor(122, 26);
        display.setTextSize(1);
        display.print(HC);
        display.setTextSize(1);
        display.setCursor(0, 40); 
        display.print(F("RADIO: "));
        display.print(statusRADIO); 
        display.setCursor(0, 52);
        display.print(F("ZELLO: "));
        display.print(statusZELLO); 
        display.setCursor(82, 40); 
        display.setTextSize(3);
        display.print(statusLINK);
      }
      
      // Mostrador VALORES E LIMITES
      if (TipoMOSTRADOR == 2)
      {
        display.setTextColor(SSD1306_WHITE); 
        display.setTextSize(2);
        display.setCursor(40, 47);
        display.print(LeituraLDRzello);
        display.setTextSize(1); 
        display.drawLine (limite1, 15, 90, 30, SSD1306_WHITE);
        display.drawLine (limite2, 15, 39, 30, SSD1306_WHITE);       
        display.setCursor(0, 19); 
        if (VRX < VTX) display.print(VRX);
        else display.print(VTX); 
        display.setCursor(26, 33);
        display.print(L2);   
        display.setCursor(52, 19);
        if (VTX > VRX) display.print(VTX);
        else display.print(VRX);
        display.setCursor(78, 33);
        display.print(L1);   
        display.setCursor(104, 19);
        display.print(VON);
      }

      // Mostrador RELOGIO SIMPLES
      if (TipoMOSTRADOR == 3)
      {
        TimerDisplay = millis();
        display.clearDisplay();
        display.setTextSize(4);
        display.setCursor(c, l);
        if (H < 10) display.print(F("0")); 
        display.print(H);
        display.print(F(":"));
        if (M < 10) display.print(F("0"));
        display.print(M);
        //display.setTextSize(1);
        //display.setCursor(0, 0);
        //display.println(c);
        //display.println(l);
        if (sa != S)
        {
          if (lado) c++;
          else c--;
          if (sobe) l++;
          else l--;
          if (c > 7)  lado = false;
          if (c < 1)  lado = true;
          if (l > 36) sobe = false;
          if (l < 1)  sobe = true;
          sa = S;
        }        
      }

      // Mostrador APAGADO
      if (TipoMOSTRADOR == 4)
      {
        display.clearDisplay();
      }
           
      display.display();
      display.clearDisplay(); 
      DisplayAceso = true;
    }
    else 
    {
      delay (20);
      if (DisplayAceso)
      {
        display.clearDisplay();
        display.display();
        DisplayAceso = false;
      }
    }
    AtualizaDisplay = millis();
  }
}

// ESP MODO AP para configuração da rede ou nova rede WIFI --------------------------------------------------------------------------------
void configModeCallback(WiFiManager *myWiFiManager)
{  
  display.println(WiFi.softAPIP()); //imprime o IP do AP
  display.display();
  delay(2000);
}

// ESP SALVANDO CONFIGURAÇÃO DO WIFI ------------------------------------------------------------------------------------------------------
void saveConfigCallback()
{
  display.println(F("Configuração salva"));
  display.println(F("IP: "));
  display.println(WiFi.softAPIP()); //imprime o IP do AP
  display.display();
  delay(2000);  
}

// CONFIGURAÇÕES --------------------------------------------------------------------------------------------------------------------------
long MenuSETUP1()
{
  while (true)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (0, 0);
                    display.println(F("     CONFIGURACAO    "));
                    display.println(F("---------------------")); 
    if (opcao == 1) display.println(F(">ADD botao PTT Zello"));
    else            display.println(F(" ADD botao PTT Zello"));  
    if (opcao == 2) display.println(F(">Posicionar LDR Zello"));
    else            display.println(F(" Posicionar LDR Zello"));
    if (opcao == 3) display.println(F(">Calibrar LDR Zello"));
    else            display.println(F(" Calibrar LDR Zello"));
    if (opcao == 4) display.println(F(">Ajustar volume VOZ"));
    else            display.println(F(" Ajustar volume VOZ"));
    if (opcao == 5) display.println(F(">Configurar WiFi"));
    else            display.println(F(" Configurar WiFi"));
    if (opcao == 6) display.println(F(">RESET"));
    else            display.println(F(" RESET"));
    if (!digitalRead(BotaoBOOT))
    {
      delay(200);
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - Crono) > 2000) 
        {
          digitalWrite (LED, 1);
          display.clearDisplay();
          display.setTextColor(SSD1306_WHITE);
          display.setTextSize(2);
          display.setCursor (50, 25);
          display.println(F("OK"));
          display.display();
          delay(1000);
          digitalWrite (LED, 0);
          switch (opcao)
          {
            case 1: 
              ADDPTTZello();
              break;
            case 2:
              PosicionarLDRZello(); 
              break;
            case 3:
              CalibrarLDRZello(); 
              break;
            case 4:
              AjusteVOLUMEvoz(); 
              break;
            case 5:
              WiFiconfig();  
              break;       
            case 6:
              ESP.restart(); 
              break;
          }
        }
      }
      opcao++;
      if (opcao == 7) opcao = 1;      
    }
    display.display();
  }
}

// CONFIGURACAO WIFI ------------------------------------------------------------------------------------------------------------------
long WiFiconfig()
{
  // VERIFICANDO WIFI ----------------------------------------------------------------------------------------------------------------------
  if (!digitalRead(BotaoBOOT)) wifiManager.resetSettings();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor (0, 0);
  display.println(F("Verificando WIFI"));
  display.display();
  delay(1000);
  digitalWrite (LED, 0);
  BOOTacionado = false;
  if (WiFi.status() != WL_CONNECTED)
  {  
    display.clearDisplay();
    display.setCursor (0, 0);
    display.println(F(" WIFI fora de alcance"));
    display.println(F("---------------------"));
    display.println(F(" Para adicionar uma"));
    display.println(F(" rede aperte o BOOT"));
    display.display();
    Crono = millis();
    while ((millis() - Crono) < 5000) 
    {
      if ((millis() - TimerPisca) > 230) 
      {
        TimerPisca = millis();
        display.print(F("-"));
        display.display();
      }
      if (!digitalRead(BotaoBOOT)) 
      { 
        BOOTacionado = true;
        Crono =- 4000;
        delay (500);
      }
    }
    if (BOOTacionado)
    {
      BOOTacionado = false;
      digitalWrite (LED, 1);
      display.clearDisplay();
      display.setCursor (0, 0); 
      display.println(F(" ENTRE NA REDE WIFI"));
      display.println(F("---------------------"));
      display.println(F(" LINK-ROBOCOP-setup"));
      display.println(F("   Senha 12345678"));    
      display.display();
      delay(2000); 
      digitalWrite (LED, 0);     
      if(!wifiManager.startConfigPortal("LINK-ROBOCOP-setup", "12345678") )
      {
        display.println(F(" Falha ao conectar"));
        display.println(F("   REINICIANDO"));
        display.display();
        delay(2000);
        ESP.restart();
        delay(1000);
      }      
    }
    display.println(F(""));
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    WIFIok = true;
    display.println(F("WIFI OK"));
    display.display();
  }
  else
  {
    WIFIok = false;
    display.println(F("WIFI OFF"));
    display.display();
  }
  display.display();
  BOOTacionado = false;
}

// ADICIONANDO BOTÃO PTT DO ZELLO -----------------------------------------------------------------------------------------------------
long ADDPTTZello()
{
  digitalWrite (LED, 0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F(" CONFIGURANDO PTT:"));
  display.println(F("---------------------"));
  display.println(F("Abra Configuracoes >"));
  display.println(F("OPCOES > Botao PTT"));
  display.println(F("Toque em + para add"));
  display.println(F("novo botao PTT..."));
  display.println(F("--------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.display();
  while (digitalRead(BotaoBOOT)) Pisca();
  digitalWrite (PTTzello, HIGH);
  digitalWrite (LED,      HIGH);
  delay (2000);
  digitalWrite (PTTzello, LOW);
  digitalWrite (LED,      LOW);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F(" CONFIGURANDO PTT:"));
  display.println(F("--------------------"));
  display.println(F("Agora toque no BOTAO"));
  display.println(F("criado e selecione >"));
  display.println(F("    > ALTERNAR <    "));
  display.println(F("em comportamento."));
  display.println(F("--------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.display();
  while (digitalRead(BotaoBOOT)) Pisca();
  delay(500);
  digitalWrite (LED, LOW);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("  PTT CONFIGURADO!"));
  display.println(F("--------------------"));
  display.println(F("Agora feche as confi"));
  display.println(F("guracoes do ZELLO..."));
  display.println(F("--------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.println(F("   para retornar"));
  display.display();
  display.clearDisplay();
  while (digitalRead(BotaoBOOT)) Pisca();
  digitalWrite (LED, LOW);
  delay(500);
}

// POSICIONAMENTO DO LDR ----------------------------------------------------------------------------------------------------------
long PosicionarLDRZello()
{
  delay (2000);
  Crono  = millis();
  while (!BOOTacionado) 
  {       
    mmLDR();
    if ((millis() - Crono) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("    POSICIONANDO    "));
      display.println(F("---------------------"));
      display.println(F("  Tente encontrar a"));
      display.println(F("    posicao com o"));
      display.println(F("     maior valor"));
      display.setTextSize(2);
      display.setCursor(40, 47);
      display.println(mmLDR());
      display.display();
      if (!digitalRead(BotaoBOOT)) BOOTacionado = true;
      Crono  = millis();
    } 
  }
}

// CONFIGURAÇÃO DO LDR ZELLO ---------------------------------------------------------------------------------------------------- 
long CalibrarLDRZello()
{
  digitalWrite (LED, 0);
  int LDRon   = mmLDR();
  int LDRtx   = mmLDR();
  int LDRrx   = mmLDR();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("INSTRUCAO PARA AJUSTE"));
  display.println(F("---------------------"));
  display.println(F(" Coloque o Zello no  "));
  display.println(F("usuario ECHO antes de"));
  display.println(F("    continuar..."));
  display.println(F("---------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.display();
  while (digitalRead(BotaoBOOT)) {mmLDR();}
   
  // Procedimento 1/3 - Coletando valor ONLINE -----------------------------------------------------------------------------------------
   
  Crono  = millis();
  Crono2 = millis();      
  while ((millis() - Crono) < 15000)
  {
    mmLDR();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F(" 1.3 CONFIG ZELLO ON"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F("  LARANJA  =  "));
      display.print(mmLDR());
      int coluna = map((millis() - Crono), 0, 15000, 0, 128);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();
    }
  }
  LDRon = mmLDR();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.println(LDRon);
  display.display();
  display.clearDisplay();  
  digitalWrite (PTTzello, HIGH);
  Crono = millis();
  while ((millis() - Crono) < TempoDoPulso) mmLDR();
  digitalWrite (PTTzello, LOW);    
  delay (1000);
  digitalWrite (LED, 0);
  
  // Procedimento 2/3 - Coletando valor TX ---------------------------------------------------------------------------------------------
   
  Crono  = millis();
  Crono2 = millis();      
  while ((millis() - Crono) < 15000)
  {
    mmLDR();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F(" 2.3 CONFIG ZELLO TX"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F(" VERMELHO  =  "));
      display.print(mmLDR());
      int coluna = map((millis() - Crono), 0, 15000, 0, 128);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();           
    }
  }
  LDRtx = mmLDR();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.println(LDRtx);
  display.display();
  display.clearDisplay();  
  digitalWrite (PTTzello, HIGH);
  Crono = millis();
  while ((millis() - Crono) < TempoDoPulso) mmLDR();
  digitalWrite (PTTzello, LOW);    
  delay (1000);
  digitalWrite (LED, 0);
    
  // Procedimento 3/3 - Coletando valor RX ---------------------------------------------------------------------------------------------
    
  Crono  = millis();
  Crono2 = millis();      
  while ((millis() - Crono) < 15000)
  {
    mmLDR();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F(" 3.3 CONFIG ZELLO RX"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F("   VERDE  =  "));
      display.print(mmLDR());
      int coluna = map((millis() - Crono), 0, 15000, 0, 128);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();           
    }
  }
  LDRrx = mmLDR();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.println(LDRrx);
  display.display();
  display.clearDisplay();      
  delay (1000);
  digitalWrite (LED, 0);
   
  // Gravando valores na EEPROM
   
  EEPROM.writeInt (evon, LDRon);                // Salvando na EEPROM os dados coletados
  EEPROM.writeInt (evrx, LDRrx);
  EEPROM.writeInt (evtx, LDRtx);
  EEPROM.commit();
}

// CONFIGURAÇÃO VOLUME VOZ ------------------------------------------------------------------------------------------------------------
long AjusteVOLUMEvoz()
{
  digitalWrite (LED, 0);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor (0, 0);
  Volume = EEPROM.readByte(evolumeVOZ);
  display.print  (F(" VOLUME ATUAL = "));
  display.println(Volume);
  display.println(F("---------------------"));
  display.println(F("Pressione para +1 ou"));  
  display.println(F("aguarde 5s para sair"));
  display.println(F("---------------------"));
  display.setCursor(0, 40);
  display.print(F("VOLUME = "));
  display.print(Volume);
  display.print(F(" 5~30"));
  display.display();
  delay (2000);
  Crono = millis();
  while ((millis() - Crono) < 5000)
    if (!digitalRead(BotaoBOOT)) 
    {
      Volume++;
      digitalWrite (LED, 1);
      if (Volume > 30) Volume = 5;
      display.fillRect  (0, 40,   128, 55, SSD1306_BLACK);
      display.setCursor (0, 40);
      display.print(F("VOLUME = "));
      display.print(Volume);
      display.print(F(" 5~30"));
      display.display(); 
      delay (500);
      Crono = millis();
      digitalWrite (LED, 0);
    }      
  if (Volume != EEPROM.readByte(evolumeVOZ))
  {
    EEPROM.writeByte (evolumeVOZ ,Volume);
    EEPROM.commit();  
  }        
  LOCUTORA.volume(Volume);                     // Ajustando o volume (0~30).
  display.display();
  delay(500);  
}