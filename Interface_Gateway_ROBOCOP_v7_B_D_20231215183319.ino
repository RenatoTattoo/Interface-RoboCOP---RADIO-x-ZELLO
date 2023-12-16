/*
 Interface RoboCOP - GMRS Radio x ZELLO - by 2RUGE0007 Renato Druziani 2023

 This is the design of a controller for a GMRS radio gateway for the zello app.
 The idea is to make it simple to assemble without the need to open the radio or smartphone to collect signals,
 I use LDRs on the cell phone screen and LED tx/rx on the HT.
 Automated configuration steps and created constant self-tuning, reducing maintenance.

*/
 
// VERSÃO do Software -------------------------------------------------------------------------------------------------------------------
 #define Versao          "7.b.d"

// Bibliotecas --------------------------------------------------------------------------------------------------------------------------
 #include <WiFiManager.h>         // por Tzapu    - Ver 2.0.16
 #include <WiFi.h>                // por Arduino  - Ver 1.2.7
 #include <NTPClient.h>           // por Fabrice  - Ver 3.2.1
 #include <Adafruit_GFX.h>        // por Adafruit - Ver 1.11.9
 #include <Adafruit_SSD1306.h>    // por Adafruit - Ver 2.5.9
 #include <SoftwareSerial.h>
 #include <DFRobotDFPlayerMini.h> // por DFRobot  - Ver 1.0.6
 #include <EEPROM.h>   
           
// relação MENSAGEM  com  o  NÚMERO DO ARQUIVO no cartão SD-----------------------------------------------------------------------------
 #define RogerBEEPlocal   25               //Arquivo 01~24 correspondem aos áudios de HORA e MEIA: 01 = 00:30h ~ 24 = 23:30h
 #define RogerBEEPweb     26               //Arquivo 25~31 correspondem aos áudios OPERACIONAIS
 #define RogerBEEPerro    27               
 #define TelaAPAGOU       28
 #define LinkOFFLINE      29
 #define LinkONLINE       30
 #define ZelloFECHOU      31
 #define PrimeiraVinheta  32                //Arquivo 32 ou + correspondem aos áudios de VINHETAS diversas

// definições PORTAS--------------------------------------------------------------------------------------------------------------------
 #define BotaoBOOT        0
 #define LED              2
 #define TocandoMP3       4
 #define PTTzello         18
 #define PTTradio         23
 #define SinalLDRradio    34
 #define SinalLDRzello    35

// Definições dos endereços dos valores salvos na EEPROM--------------------------------------------------------------------------------
 #define EEPROM_SIZE      64
 #define evolumeVOZ       10
 #define evonZ            20
 #define evtxZ            30
 #define evrxZ            40
 #define evtxR            50
 #define eHoraCerta       0
 #define eMensagens       1
 #define eRogerBEEPs      2
 #define eFuso            3

// Outras Definições -------------------------------------------------------------------------------------------------------------------
 #define TempoDoPulso     150
 #define TempoDisplay     300000          //Define o tempo para que o LDC fique apagado depois da inatividade
 #define TempoSalvaAjuste 3600000         //Define de quanto em quanto tempo deve ser salvo os valores de Auto Ajuste
 #define TaxaAtualizacao  75              //Define de quantos em quantos milisegundos o Display é atualizado
 #define AntiTRAVA        300000000       //Define o tempo de detecção de travamento (em microsegundos) 300 mega = 5 minutos
 #define NAZ              200             //Número de amostragens para a média móvel da leitura do LDR do ZELLO
 #define NAAz             20              //Número de amostragens para a média móvel das correções de auto ajuste
 #define NAR              100             //Número de amostragens para a média móvel da leitura do LDR do RADIO

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);
Adafruit_SSD1306 display(128, 64, &Wire);
hw_timer_t *CaoDeGuarda = NULL;
WiFiManager wm;
SoftwareSerial ComunicacaoSerial(16, 17);     // Pinos 16 RX, 17 TX para comunicação serial com o DFPlayer
DFRobotDFPlayerMini LOCUTORA;                 // nomeando DFPlayer como "LOCUTORA"
 

// Variáveis, arrays, funções, etc------------------------------------------------------------------------------------------------------

 bool          wm_nonblocking    = false; 
 bool          WiFiOK            = false;
 bool          HC                = false;
 bool          DisplayAceso      = true;
 bool          MensagemTA        = true;
 bool          MensagemTI        = true;
 bool          MensagemOFF       = true;
 bool          MensagemON        = false;
 byte          D                 = 30;
 byte          H                 = 0;
 byte          M                 = 0;
 byte          S                 = 0;
 byte          Volume            = 0;
 byte          Vinheta           = PrimeiraVinheta;
 byte          Narquivos         = 0;
 byte          TipoMOSTRADOR     = 2;

 int           LeituraZELLOatual = 0;
 int           LeituraRADIOatual = 0;
 int           LSz               = 0;
 int           VONz              = 0;
 int           VRXz              = 0;
 int           VTXz              = 0;
 int           DifONRXz          = 0;
 int           L1z               = 0;
 int           LIz               = 0;
 String        EstadoZello       = "XX";
 
 int           VTXr              = 0;
 int           LSr               = 0;
 int           L1r               = 0;
 String        EstadoRadio       = "XX";

 String        EstadoLINK        = "XX";

 byte          Fuso              = 0;      //Fernando de Noronha(GMT-2), Brasília(GMT-3), Manaus - AM(GMT-4), Rio Branco - AC(GMT-5)
 unsigned long Relogio           = millis();
 unsigned long TimerLCD          = millis();
 unsigned long TimerFPS          = millis();
 unsigned long TimerLOOP         = millis();
 unsigned long TimerPisca        = millis();
 unsigned long TimerDeEntrada    = millis();
 unsigned long TimerAutoAjuste   = millis();
 unsigned long Crono             = millis();
 unsigned long CronoONz          = millis();
 unsigned long CronoRXz          = millis();
 int           MediaLDRzello[NAZ];
 int           MediaONz[NAAz];
 int           MediaRXz[NAAz];
 int           MediaLDRradio[NAR];




void IRAM_ATTR ReiniciaRoboCOP() // função que o Cão de guarda irá chamar, para reiniciar o Sistema em caso de travamento
{
  ESP.restart(); // Reiniciando 
}

void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  ComunicacaoSerial.begin(9600);
  
  CaoDeGuarda = timerBegin (0, 80, true);
  timerAttachInterrupt(CaoDeGuarda, &ReiniciaRoboCOP, true); 
  timerAlarmWrite(CaoDeGuarda, AntiTRAVA, true);             // Ajustando timer do cão de guarda        
  timerAlarmEnable(CaoDeGuarda);

  wm.setDebugOutput(false);

  // DEFININDO PINOS --------------------------------------------------------------------------------------------------------------------
  pinMode(TocandoMP3,    INPUT);
  pinMode(SinalLDRzello, INPUT_PULLDOWN);
  pinMode(SinalLDRradio, INPUT_PULLDOWN);
  pinMode(LED,           OUTPUT);
  pinMode(PTTzello,      OUTPUT);
  pinMode(PTTradio,      OUTPUT);
  pinMode(BotaoBOOT,     INPUT_PULLUP);

  digitalWrite (LED, 1);
  Serial.println(F("Iniciando DISPLAY"));
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  while(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    delay (50);
    digitalWrite (LED, 1);
    Serial.print(F("-"));
    display.display();
    delay (50);
    digitalWrite (LED, 0);
  }
  display.display();
  delay(500);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor (0, 57);
  display.print(F("Versao"));
  display.setCursor (95, 57);
  display.print(Versao);
  display.display();
  display.clearDisplay();
  delay(2000);
  display.setCursor (0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(F("LCD OK!!"));
  Serial.println (F("LCD OK!!"));
  display.display();
  Serial.end();
  digitalWrite (LED, 0);

  display.print  (F("MP3..."));
  if (!LOCUTORA.begin(ComunicacaoSerial))      //Use softwareSerial to communicate with mp3.
  {
    display.clearDisplay();
    display.setCursor (0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.println(F("    MP3 FALHOU!!!"));
    display.println(F("---------------------"));
    display.println(F(" IMPOSSIVEL INICIAR "));
    display.println(F(" !!! Por  favor !!!"));
    display.println(F("  Cheque CARTAO SD"));
    display.display();
    delay(200);
    while(!LOCUTORA.begin(ComunicacaoSerial))
    {
      digitalWrite (LED, 1);
      display.fillRect(0, 50,   128, 64, SSD1306_BLACK);
      display.display();
      delay (100);
      digitalWrite (LED, 0);
      display.setCursor (0, 50);
      display.println(F("tentando novamente.."));
      display.display();
      delay (100);
    }
    display.clearDisplay();
    display.setCursor (0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
  }
  LOCUTORA.outputDevice(DFPLAYER_DEVICE_SD);   // configurando a media usada = SDcard
  LOCUTORA.EQ(DFPLAYER_EQ_BASS);               // configurando equalizador = BASS
  Narquivos = LOCUTORA.readFileCounts();       // contando número de arquivos no SD
  while (Narquivos == 255)
    {
      Narquivos = LOCUTORA.readFileCounts();
      delay (50);
    }
  Volume = EEPROM.readByte(evolumeVOZ);        // Ajustando o Volume armazenado
  LOCUTORA.setTimeOut(500);                    // ajustando o timeout da comunicação serial 500ms
  LOCUTORA.volume(Volume);                     // Ajustando o volume (0~30).
  display.println(F(" OK!!!"));
  display.display();


  delay(500);
  display.println(F("Checando WiFi..."));
  display.display();
  wm.setConfigPortalTimeout(1);
  WiFiOK = wm.autoConnect();
  if (WiFiOK)
  {
    display.println(F("WiFi OK!!"));
    display.display();
    delay(500);
    display.println(F("Sincronizando relogio"));
    display.display();
    delay(500);
    SincronizarNTP();
    if (HC) 
    {
      display.setTextSize(2);
      display.setCursor (15, 50);
      if (H<10) display.print (F("0"));
      display.print   (H);
      display.print   (F(":"));
      if (M<10) display.print (F("0"));
      display.print   (M);
      display.print   (F(":"));
      if (S<10) display.print (F("0"));
      display.println (S);
      display.display();
      delay(500);
    }
    else
    {
      display.println(F("Sincronizacao FALHOU!"));
      display.display();
      delay(500);
    }
  }
  else display.println(F("WiFi FORA DE ALCANCE!"));
  display.display();
  display.clearDisplay();
  delay(1000);
  // RECUPERAR DADOS
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("    LENDO VALORES"));
  display.println(F("---------------------"));
  VONz = EEPROM.readInt  (evonZ);  // Lendo na EEPROM os dados gravados
  VTXz = EEPROM.readInt  (evtxZ);
  VRXz = EEPROM.readInt  (evrxZ);
  VTXr = EEPROM.readInt  (evtxR);
  Fuso = EEPROM.readByte (eFuso);
  for(int x=0; x<NAAz; x++) MediaONz[x] = VONz;
  for(int x=0; x<NAAz; x++) MediaRXz[x] = VRXz;
  display.print  (F(" VONz : "));
  display.println(VONz); 
  display.print  (F(" VTXz : "));               
  display.println(VTXz);
  display.print  (F(" VRXz : "));
  display.println(VRXz);
  display.print  (F(" VTXr : "));               
  display.println(VTXr);
  display.print  (F(" GMT  : -"));
  display.println(Fuso);
  display.display();
  display.clearDisplay();
  delay(1000);
}

void loop()
{
  mmLDRzello();
  mmLDRradio();
  if ((millis() - TimerLOOP) > 20)
  {
    
    LeituraZELLOatual = mmLDRzello();
    LeituraRADIOatual = mmLDRradio();
    RELOGIO();
    ESTADOzello();
    ESTADOradio();
    Display();
    timerWrite(CaoDeGuarda, 0);                  // Alimentando o cachorrinho


    if ((EstadoZello == "Recebendo") & (EstadoRadio == "Stand by"))
    {
      digitalWrite (PTTradio, HIGH);
      EstadoRadio = "Transmitindo";
      while (EstadoZello == "Recebendo")
      {
        LeituraZELLOatual = mmLDRzello();
        LeituraRADIOatual = mmLDRradio();
        RELOGIO();
        ESTADOzello();
        Display();
      }

      TOCAR(RogerBEEPweb);

      digitalWrite (PTTradio, LOW);
      while (LeituraRADIOatual > L1r)
      {
        LeituraRADIOatual = mmLDRradio();
        LeituraZELLOatual = mmLDRzello();
        ValoresLimites();
        Display();
      }
    
    }

    if ((EstadoRadio == "Recebendo") & (EstadoZello == "On-line"))
    {
      digitalWrite (PTTzello, HIGH);
      delay (TempoDoPulso);
      digitalWrite (PTTzello, LOW);
      EstadoZello = "Transmitindo";
      while (EstadoRadio == "Recebendo")
      {
        LeituraZELLOatual = mmLDRzello();
        LeituraRADIOatual = mmLDRradio();
        RELOGIO();
        ESTADOradio();
        Display();
      }
      digitalWrite (PTTradio, HIGH);
      digitalWrite (PTTzello, HIGH);
      delay (TempoDoPulso);
      digitalWrite (PTTzello, LOW);

      TOCAR(RogerBEEPlocal);
      
      digitalWrite (PTTradio, LOW);
      while (LeituraZELLOatual < L1z)
      {
        LeituraZELLOatual = mmLDRzello();
        LeituraRADIOatual = mmLDRradio();
        ValoresLimites();
        Display();
      }
    }


    Crono = millis();
    if (!digitalRead(BotaoBOOT)) 
    {
      delay (75);
      while (!digitalRead(BotaoBOOT)) 
      {
        Display();
        if ((millis() - Crono) > 3000) MenuSETUPinicio();
      }
      delay (100);
      if ((millis() - TimerLCD) < TempoDisplay) TipoMOSTRADOR++;
      if (TipoMOSTRADOR > 2) TipoMOSTRADOR = 1;
      TimerLCD = millis();
    }

    TimerLOOP = millis();
  }
}

void TOCAR(byte Arquivo)
{
  LOCUTORA.play (Arquivo);
  while (!digitalRead(TocandoMP3))
  {
    LeituraZELLOatual = mmLDRzello();
    LeituraRADIOatual = mmLDRradio();
    ValoresLimites();
    Display();
  }
  while (digitalRead(TocandoMP3)) 
  {
    LeituraZELLOatual = mmLDRzello();
    LeituraRADIOatual = mmLDRradio();
    ValoresLimites();
    Display();
  }
}


// LDR - FUNÇÕES MEDIA MOVEL ------------------------------------------------------------------------------------------------------------
long mmLDRzello()
{
  for(int x=NAZ-1; x>0; x--) MediaLDRzello[x] = MediaLDRzello[x-1];
  MediaLDRzello[0] = analogRead (SinalLDRzello);
  delay(1);  
  long contador = 0;
  for (int x=0; x<NAZ; x++) contador += MediaLDRzello[x];
  return (contador/NAZ);
}

long mmONz()
{
  for(int x= NAAz-1; x>0; x--) MediaONz[x] = MediaONz[x-1];
  MediaONz[0] = LeituraZELLOatual;
  long contador = 0;
  for (int x=0; x<NAAz; x++) contador += MediaONz[x];
  return (contador/NAAz);
}


long mmRXz()
{
  for(int x= NAAz-1; x>0; x--) MediaRXz[x] = MediaRXz[x-1];
  MediaRXz[0] = LeituraZELLOatual;
  long contador = 0;
  for (int x=0; x<NAAz; x++) contador += MediaRXz[x];
  return (contador/NAAz);
}


long mmLDRradio()
{
  for(int x=NAR-1; x>0; x--) MediaLDRradio[x] = MediaLDRradio[x-1];
  MediaLDRradio[0] = analogRead (SinalLDRradio);
  delay(1);  
  long contador = 0;
  for (int x=0; x<NAR; x++) contador += MediaLDRradio[x];
  return (contador/NAR);
}

void RELOGIO()
{
  Pisca();
  if ((!HC) & (D>29) & (M==2) & (S==0) & (WiFiOK)) HoraCerta();
  // Cronometro do RELOGIO
  if (millis() > Relogio)
  { 
    S++;
    Relogio+=1000;
    if (H > 23) { H=0; D++; }
    if (M > 59) { H++; M=0; }
    if (S > 59) { M++; S=0; }
  }
}


void ValoresLimites()
{
  // Limites ZELLO
  DifONRXz = (VONz - VRXz) / 2;
  LSz = VONz + DifONRXz;
  L1z = VONz - DifONRXz;
  LIz = VRXz - DifONRXz;
  
  // Limites RADIO
  if (LSr == 0) LSr = VTXr;
  if (LeituraRADIOatual > LSr) LSr = (LeituraRADIOatual * 1.1);
  L1r = LSr / 2;

}

void ESTADOzello()
{
  ValoresLimites();
  int Porcentagem = ((NAZ / 100) * 10);
  if ((MediaLDRzello[NAZ-1] > (MediaLDRzello[0] - Porcentagem)) & (MediaLDRzello[NAZ-1] < (MediaLDRzello[0] + Porcentagem)))
  {
   // if ((millis() - TimerDeEntrada) > 25) //Aguardar a leitura se estabilizar por este tempo
   // {
      if  (LeituraZELLOatual >  LSz)                                 EstadoZello = "Tela Inicial";
      if ((LeituraZELLOatual > (L1z)) & (LeituraZELLOatual <  LSz))  EstadoZello = "On-line";
      if ((LeituraZELLOatual >  LIz)  & (LeituraZELLOatual < (L1z))) EstadoZello = "Recebendo";
      if ((LeituraZELLOatual >  100)  & (LeituraZELLOatual <  LIz))  EstadoZello = "Off-line";
      if  (LeituraZELLOatual <   99)                                 EstadoZello = "Tela Apagada";
   // } 
  }
  //else TimerDeEntrada = millis();

  AutoAjuste();
}

void ESTADOradio()
{
  ValoresLimites();
  int Porcentagem = ((NAR / 100) * 25);
  if ((MediaLDRradio[NAR-1] > (MediaLDRradio[0] - Porcentagem)) & (MediaLDRradio[NAR-1] < (MediaLDRradio[0] + Porcentagem)))
  {
    if (LeituraRADIOatual >= L1r) EstadoRadio = "Recebendo";
    if (LeituraRADIOatual <  L1r) EstadoRadio = "Stand by";
  }
}

void AutoAjuste()
{
  if (EstadoZello != "On-line") CronoONz = millis();
  if (EstadoZello != "Recebendo") CronoRXz = millis(); 

  if ((EstadoZello == "On-line") & ((millis() - CronoONz) > 30000))
  {  
    if ((LeituraZELLOatual > L1z) & (LeituraZELLOatual < LSz)) VONz = mmONz();    
    CronoONz = millis() - 29000; 
  }

      
  if ((EstadoZello == "Recebendo") & ((millis() - CronoRXz) > 15000))
  { 
    if ((LeituraZELLOatual > LIz) & (LeituraZELLOatual < L1z)) VRXz = mmRXz();     
    CronoRXz = millis() - 14000; 
  }   
  
  if (millis() - TimerAutoAjuste > TempoSalvaAjuste)     // Hora em hora verifica e salva na EEPROM os dados ajustados
  {
    if ((EEPROM.readInt (evonZ) != VONz) | (EEPROM.readInt (evrxZ) != VRXz) | (EEPROM.readInt (evtxZ) != VTXz))
    {
      EEPROM.writeInt (evonZ, VONz);   
      EEPROM.writeInt (evrxZ, VRXz);
      EEPROM.commit(); 
    }     
    TimerAutoAjuste = millis();
  }
}


void Display()
{
  if (((millis() - TimerLCD) > TempoDisplay) & ((EstadoZello != "On-line") | (EstadoRadio != "Stand by")))
  {
    TimerLCD = (millis() - 290000); //Ativando Display por 10 segundos durante inatividade
  }
  if ((millis() - TimerFPS) > TaxaAtualizacao)
  {
    if ((millis() - TimerLCD) < TempoDisplay)
    {
      display.clearDisplay();
      int LeituraZ  = map (LeituraZELLOatual, LIz, LSz, 0, 95);
      int ColunaONz = map (VONz, LIz, LSz, 31, 128);
      int ColunaL1z = map ( L1z, LIz, LSz, 31, 128);
      int ColunaRXz = map (VRXz, LIz, LSz, 31, 128);
      
      display.fillRect ( 31, 0, LeituraZ,  7, SSD1306_WHITE);
      if ((LeituraZ + 31) < ColunaL1z) display.drawLine ( ColunaL1z, 0, ColunaL1z,  7, SSD1306_WHITE);
      else                             display.drawLine ( ColunaL1z, 0, ColunaL1z,  7, SSD1306_BLACK);
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor (0, 0);
      display.print  (F("ZELLO"));

      display.setCursor ((ColunaRXz - 5) , 0);
      if ((LeituraZ + 31) > (ColunaRXz - 2)) display.setTextColor(SSD1306_BLACK);
      else                              display.setTextColor(SSD1306_WHITE);
      display.print  (F("R"));
      if ((LeituraZ + 31) > (ColunaRXz + 2)) display.setTextColor(SSD1306_BLACK);
      else                              display.setTextColor(SSD1306_WHITE);
      display.print  (F("X"));

      display.setCursor ((ColunaONz - 5), 0);
      if ((LeituraZ + 31) > (ColunaONz - 2)) display.setTextColor(SSD1306_BLACK);
      else                              display.setTextColor(SSD1306_WHITE);
      display.print  (F("O"));
      if ((LeituraZ + 31) > (ColunaONz + 2)) display.setTextColor(SSD1306_BLACK);
      else                              display.setTextColor(SSD1306_WHITE);
      display.print  (F("N"));
      
      
      
      int LeituraR  = map (LeituraRADIOatual, 0, LSr, 0, 95);
      int ColunaL1r = map ( L1r, 0, LSr, 31, 128);
      //display.drawRect ( 31,  9, 97     ,  7, SSD1306_WHITE);
      display.fillRect ( 31, 9, LeituraR,  7, SSD1306_WHITE);
      if ((LeituraR + 31) < ColunaL1r) display.drawLine ( ColunaL1r, 9, ColunaL1r, 15, SSD1306_WHITE);
      else                             display.drawLine ( ColunaL1r, 9, ColunaL1r, 15, SSD1306_BLACK);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor (0, 9); 
      display.print   (F("RADIO"));

      if (TipoMOSTRADOR == 1)
      {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (0, 32);
        if (HC) 
          display.println(F("     HORA  CERTA     "));
        else
          display.println(F("     CRONOMETRO      "));
        display.println(F("---------------------"));
        display.setTextSize(2);
        display.setCursor (15, 50);
        if (H<10) display.print (F("0"));
        display.print   (H);
        display.print   (F(":"));
        if (M<10) display.print (F("0"));
        display.print   (M);
        display.print   (F(":"));
        if (S<10) display.print (F("0"));
        display.println (S);
      }

      if (TipoMOSTRADOR == 2)
      {
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (0, 16);
        display.print  (F("     "));
        display.print  (VRXz);
        display.print  (F("        "));
        display.println(VONz);
        display.print  (F("           "));
        display.print  (L1z);

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (0, 33);
        display.print  (F("Zello: "));
        display.println(EstadoZello);
        display.print  (F("LDRz : "));
        display.println(LeituraZELLOatual);
        display.print  (F("Radio: "));
        display.println(EstadoRadio);
        display.print  (F("LDRr : "));
        display.print  (LeituraRADIOatual);
      }
    }
  TimerFPS = millis();
  display.display();
  display.clearDisplay();
  }
}




// MENU INICIAL CONFIGURAÇÕES -------------------------------------------------------------------------------------------------------------
void MenuSETUPinicio()
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
  bool Sair  = true;
  byte opcao = 0;
  while (Sair)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (0, 0);
                    display.println(F("     CONFIGURACAO    "));
                    display.println(F("---------------------")); 
    if (opcao == 1) display.println(F(">CONFIG BASICA"));
    else            display.println(F(" Config basica"));  
    if (opcao == 2) display.println(F(">CONFIG MENSAGENS"));
    else            display.println(F(" Config mensagens"));
    if (opcao == 3) display.println(F(">CONFIG WiFi"));
    else            display.println(F(" Config WiFi"));
    if (opcao == 4) display.println(F(">ACERTAR RELOGIO"));
    else            display.println(F(" Acertar Relogio"));
    if (opcao == 5) display.println(F(">SAIR"));
    else            display.println(F(" Sair"));
    if (opcao == 6) display.println(F(">RESET"));
    else            display.println(F(" Reset"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(10);
      digitalWrite (LED, 0);
      delay(200);
      TimerPisca = millis();
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        Pisca();
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
              opcao = 0;
              MenuSETUPbasico();
              break;
            case 2:
              opcao = 0;
              //MenuSETUPmensagens(); 
              break;
            case 3:
              opcao = 0;
              MenuSETUPwifi();  
              break;  
            case 4:
              opcao = 0;
              MenuSETUPrelogio();  
              break;
            case 5:
              Sair = false; 
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
  opcao = 0;
  delay(200);
  while (!digitalRead(BotaoBOOT)) Pisca();
  delay(1000);
}

void MenuSETUPrelogio()
{
  
  bool Voltar  = false;
  byte opcao = 0;
  while (!Voltar)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (0, 0);
                    display.println(F("   ACERTAR RELOGIO   "));
                    display.println(F("---------------------")); 
    if (opcao == 1) display.println(F(">SINCRONIZAR COM NTP"));
    else            display.println(F(" Sincronizar com NTP"));  
    if (opcao == 2) display.println(F(">AJUSTAR FUSO HORARIO"));
    else            display.println(F(" Ajustar Fuso Horario"));
    if (opcao == 3) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(10);
      digitalWrite (LED, 0);
      delay(200);
      TimerPisca = millis();
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        Pisca();
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
              HoraCerta();
            break;
            case 2:
              FusoHorario();
            break;
            case 3:
              Voltar = true; 
            break;
          }
        }
      }
      opcao++;
      if (opcao > 3) opcao = 1;      
    }
    display.display();
  }
  delay(200);
  while (!digitalRead(BotaoBOOT)) Pisca();
  delay(1000);
}

void FusoHorario()
{
  Fuso = EEPROM.readByte (eFuso);
  bool Voltar  = false;
  byte opcao   = 0;
  while (!Voltar)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (0, 0);
    display.println(F("     FUSO ATUAL      "));
    switch (Fuso)
    {
      case 2:
        display.println(F("FERNANDO DE NORONHA"));
      break;
      case 3:
        display.println(F("      BRASILIA"));
      break;
      case 4:
        display.println(F("       MANAUS"));
      break;
      case 5:
        display.println(F("     RIO BRANCO"));
      break;
    } 
    display.setCursor (0, 17);
    if (opcao == 1) display.println(F(">FERNANDO DE NORONHA"));
    else            display.println(F(" Fernando de Noronha"));  
    if (opcao == 2) display.println(F(">BRASILIA"));
    else            display.println(F(" Brasilia"));
    if (opcao == 3) display.println(F(">MANAUS"));
    else            display.println(F(" Manaus"));
    if (opcao == 4) display.println(F(">RIO BRANCO"));
    else            display.println(F(" Rio Branco"));
    if (opcao == 5) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(10);
      digitalWrite (LED, 0);
      delay(200);
      TimerPisca = millis();
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        Pisca();
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
          if ((opcao > 0) & (opcao < 5)) Fuso = opcao + 1;
          if (opcao == 5) Voltar = true; 
        }
      }
      opcao++;
      if (opcao > 5) opcao = 1;      
    }
    display.display();
  }
  if (Fuso != EEPROM.readByte (eFuso))
  {
    EEPROM.writeByte (eFuso, Fuso);
    EEPROM.commit();
  }
  Voltar = true;
  delay(200);
}

void HoraCerta()
{  
  digitalWrite (LED, 0);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor (0, 0);
  display.println(F("      HORA CERTA"));
  display.println(F("---------------------"));
  display.println();
  display.println(F("Sincronizando relogio"));
  display.println();
  display.display();
  SincronizarNTP();
  if (HC) display.println(F(" Sincronizacao    OK"));
  else   display.println(F("Sincronizacao FALHOU!"));
  display.setTextSize(2);
  display.setCursor (15, 50);
  if (H<10) display.print (F("0"));
  display.print   (H);
  display.print   (F(":"));
  if (M<10) display.print (F("0"));
  display.print   (M);
  display.print   (F(":"));
  if (S<10) display.print (F("0"));
  display.println (S);
  display.display();
  digitalWrite (LED, 0);
  delay(3000);
}

void SincronizarNTP()
{
  Fuso = EEPROM.readByte (eFuso);
  wm.setConfigPortalTimeout(1);
  WiFiOK = wm.autoConnect();
  delay(500);
  if (WiFiOK)
  {
    ntp.begin();
    delay(500);
    ntp.setTimeOffset(-3600 * Fuso);
    if (ntp.update()) 
    {
      D  = 0;
      H  = (ntp.getHours());
      M  = (ntp.getMinutes());
      S  = (ntp.getSeconds());
      HC = true;
      Relogio = millis();
    }
  }
  WiFi.disconnect();
  wm.disconnect();
}

void MenuSETUPwifi()
{
  bool Voltar  = true;
  byte opcao2 = 0;
  unsigned long Crono2 = millis();
  while (Voltar)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (0, 0);
                     display.println(F("        Wi-Fi"));
                     display.println(F("---------------------")); 
    if (opcao2 == 1) display.println(F(">LIMPAR CONFIGURACAO"));
    else             display.println(F(" Limpar Configuracao"));  
    if (opcao2 == 2) display.println(F(">CONFIGURAR NOVA REDE"));
    else             display.println(F(" Configurar Nova Rede"));
    if (opcao2 == 3) display.println(F(">VOLTAR"));
    else             display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(10);
      digitalWrite (LED, 0);
      delay(200);
      TimerPisca = millis();
      Crono2 = millis();
      while (!digitalRead(BotaoBOOT))
      {
        Pisca();
        if ((millis() - Crono2) > 2000) 
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
          switch (opcao2)
          {
            case 1: 
            {
              opcao2 = 0;
              display.clearDisplay();
              display.setCursor (0, 0);
              display.setTextColor(SSD1306_WHITE);
              display.setTextSize(1);
              display.println(F("        Wi-Fi"));
              display.println(F("---------------------"));
              display.println(F("   APAGANDO CONFIGS"));
              display.println(F("       do WiFi"));
              display.println(F("---------------------"));
              display.display();
              wm.resetSettings();  // apaga configs do wifi
              delay(3000);
            }
            break;
            case 2:
            {
              opcao2 = 0;
              WiFi.mode(WIFI_STA); // Definindo o modo do WiFi, esp padrão para STA+AP 
              display.clearDisplay();
              display.setCursor (0, 0);
              display.setTextColor(SSD1306_WHITE);
              display.setTextSize(1);
              display.println(F("   INICIANDO WiFi"));
              display.println(F("---------------------"));
              display.display();
              if (wm_nonblocking) wm.setConfigPortalBlocking(false);
              delay(100);
              wm.setSaveParamsCallback(saveParamCallback);
              std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
              wm.setMenu(menu);
              wm.setClass("invert");
              wm.setConfigPortalTimeout(1); // Fechar portal de configurações após N segundos
              digitalWrite (LED, 1);
              WiFiOK = wm.autoConnect(); // NOME e SENHA do AP
              if(WiFiOK) 
              {
                display.println(F("   CONECTADO!! :)")); 
              }
              else 
              {  
                digitalWrite (LED, 1);
                display.clearDisplay();
                display.setCursor (0, 0); 
                display.println(F(" ENTRE NA REDE WIFI"));
                display.println(F("---------------------"));
                display.println(F(" RoboCOP-WiFi-Config"));
                display.println(F("   Senha 12345678"));
                display.println(F("  Para inserir uma "));
                display.println(F("   nova rede WiFi."));    
                display.display();
                wm.setConfigPortalTimeout(180);
                WiFiOK = wm.autoConnect("RoboCOP-WiFi-Config","12345678");
              }
              display.display();
              WiFi.disconnect();
              wm.disconnect();
              digitalWrite (LED, 0);
            }
            break; 
            case 3:
            {
              Voltar = false; 
            }
            break;     
          }
        }
      }
      opcao2++;
      if (opcao2 == 4) opcao2 = 1;      
    }
    display.display();
  }
  opcao2 = 0;
  delay(1000);
}

void saveParamCallback()
{
  // apenas callback
}

// CONFIGURAÇÕES BÁSICAS --------------------------------------------------------------------------------------------------------------
void MenuSETUPbasico()
{
  digitalWrite (LED, 0);
  unsigned long Crono2 = millis();
  bool Sair            = false;
  byte opcao           = 0;
  while (!Sair)
  {
    timerWrite(CaoDeGuarda, 0); // Alimentando o cachorrinho
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (0, 0);
                    display.println(F(" CONFIGURACAO BASICA "));
                    display.println(F("---------------------")); 
    if (opcao == 1) display.println(F(">ADD BOTAO PTT ZELLO"));
    else            display.println(F(" Add botao PTT Zello"));  
    if (opcao == 2) display.println(F(">POSICIONAR LDR ZELLO"));
    else            display.println(F(" Posicionar LDR Zello"));
    if (opcao == 3) display.println(F(">CALIBRAR LDR ZELLO"));
    else            display.println(F(" Calibrar LDR Zello"));
    if (opcao == 4) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(10);
      digitalWrite (LED, 0);
      delay(200);
      TimerPisca = millis();
      Crono2 = millis();
      while (!digitalRead(BotaoBOOT))
      {
        Pisca();
        if ((millis() - Crono2) > 2000) 
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
              CalibrarLDRs(); 
              break;
            case 4:
              Sair = true; 
              break;
          }
        }
      }
      opcao++;
      if (opcao == 5) opcao = 1;      
    }
    display.display();
  }
  opcao = 0;
}

// POSICIONAMENTO DO LDR ---------------------------------------------------------------------------------------------------------------
void PosicionarLDRZello()
{
  unsigned long Crono3 = millis();
  delay (200);
  digitalWrite (LED, 0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("POSICIONAR LDR ZELLO"));
  display.println(F("---------------------"));
  display.println(F("Deixar o ZELLO online"));
  display.println(F("  no usuario ECHO."));
  display.println(F(" O botao PTT na tela"));
  display.println(F(" deve estar LARANJA."));
  display.println(F("---------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.display();
  while (digitalRead(BotaoBOOT)) Pisca();
  digitalWrite (LED, 1);
  delay (200);
  digitalWrite (LED, 0);
  delay (500);
  int Vmax   = 0;
  int Maximo = 0;
  while (digitalRead(BotaoBOOT)) 
  {       
    mmLDRzello();
    if ((millis() - Crono3) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("     POSICIONANDO"));
      display.println();
      display.println(F("  Tente encontrar a"));
      display.println(F("    posicao com o"));
      display.println(F("     maior valor"));
      display.setTextSize(2);
      display.setCursor(40, 47);
      display.println(mmLDRzello());
      //display.println(Maximo);
      if (mmLDRzello() > Vmax) Vmax = (mmLDRzello() + ((mmLDRzello() / 100) * 10));
      Maximo = map (mmLDRzello(), 0, Vmax, 1, 124);
      display.drawRect ( 0,  9,    128, 7, SSD1306_WHITE);
      display.fillRect ( 2, 11, Maximo, 3, SSD1306_WHITE);
      display.display();
      Crono3 = millis();
    }
  }
  delay(200);
  while (!digitalRead(BotaoBOOT)) Pisca();
  delay(200);
}

// ADICIONANDO BOTÃO PTT DO ZELLO ------------------------------------------------------------------------------------------------------
void ADDPTTZello()
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

// CALIBRAÇÃO DO LDR ZELLO E RADIO ---------------------------------------------------------------------------------------------------- 
void CalibrarLDRs()
{
  digitalWrite (LED, 0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("INSTRUCAO PARA AJUSTE"));
  display.println(F("---------------------"));
  display.println(F("Ligar RADIO e deixe o"));
  display.println(F("Zello on-line no ECHO"));
  display.println(F("antes de continuar..."));
  display.println(F("---------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.display();
  while (digitalRead(BotaoBOOT)) {mmLDRzello();}
   
  // Procedimento 1/4 - Coletando valor ONLINE -----------------------------------------------------------------------------------------
   
  Crono                = millis();
  int PTTZlaranja      = mmLDRzello();
  unsigned long Crono2 = millis();      
  while ((millis() - Crono) < 15000)
  {
    mmLDRzello();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("1.4 CALIBRANDO ONLINE"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F("  LARANJA  =  "));
      display.print(mmLDRzello());
      int coluna = map((millis() - Crono), 0, 15000, 0, 126);
      display.drawRect( 0, 55,    128, 60, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 58, SSD1306_WHITE);
      display.display();
      Crono2 = millis();
    }
  }
  PTTZlaranja = mmLDRzello();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.println(PTTZlaranja);
  display.display();
  display.clearDisplay();  
  digitalWrite (PTTzello, HIGH);
  Crono = millis();
  while ((millis() - Crono) < TempoDoPulso) mmLDRzello();
  digitalWrite (PTTzello, LOW);    
  delay (1000);
  digitalWrite (LED, 0);
  
  // Procedimento 2/4 - Coletando valor TX ---------------------------------------------------------------------------------------------
   
  int PTTZvermelho = mmLDRzello();
  Crono     = millis();
  Crono2    = millis();      
  while ((millis() - Crono) < 15000)
  {
    mmLDRzello();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("2.4 CALIBRANDO EM TX"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F(" VERMELHO  =  "));
      display.print(mmLDRzello());
      int coluna = map((millis() - Crono), 0, 15000, 0, 126);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();           
    }
  }
  PTTZvermelho = mmLDRzello();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.println(PTTZvermelho);
  display.display();
  display.clearDisplay();  
  digitalWrite (PTTzello, HIGH);
  Crono = millis();
  while ((millis() - Crono) < TempoDoPulso) mmLDRzello();
  digitalWrite (PTTzello, LOW);    
  delay (1000);
  digitalWrite (LED, 0);
    
  // Procedimento 3/4 - Coletando valor RX ---------------------------------------------------------------------------------------------
    
  int PTTZverde = mmLDRzello();
  Crono         = millis();
  Crono2        = millis();      
  while ((millis() - Crono) < 15000)
  {
    mmLDRzello();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("3.4 CALIBRANDO EM RX"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F("   VERDE  =  "));
      display.print(mmLDRzello());
      int coluna = map((millis() - Crono), 0, 15000, 0, 126);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();           
    }
  }
  PTTZverde = mmLDRzello();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.println(PTTZverde);
  display.display();
  display.clearDisplay();      
  delay (1000);
  digitalWrite (LED, 0);
    

    // Procedimento 4/4 - Coletando do RADIO o valor TX -----------------------------------------------------------------------------------
    
  int PTTRvermelho = mmLDRradio();
  digitalWrite (PTTradio, HIGH);
  Crono     = millis();
  Crono2    = millis();      
  while ((millis() - Crono) < 7500)
  {
    mmLDRradio();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("4.4 LENDO TX no RADIO"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F(" VERMELHO  =  "));
      display.print(mmLDRradio());
      int coluna = map((millis() - Crono), 0, 7500, 0, 126);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();           
    }
  }
  PTTRvermelho = mmLDRradio();
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  display.println(PTTRvermelho);
  display.display();
  display.clearDisplay(); 
  digitalWrite (PTTradio, LOW);     
  delay (1000);
  digitalWrite (LED, 0);
  

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F(" SALVANDO CALIBRACAO "));
  display.println(F("---------------------"));
  display.println(PTTZlaranja);                
  display.println(PTTZverde);
  display.println(PTTZvermelho);
  display.println(PTTRvermelho); 
   
  EEPROM.writeInt (evonZ, PTTZlaranja);  // Gravando valores na EEPROM
  EEPROM.writeInt (evrxZ, PTTZverde);
  EEPROM.writeInt (evtxZ, PTTZvermelho);
  EEPROM.writeInt (evtxR, PTTRvermelho);
  EEPROM.commit();
  display.display();
  display.clearDisplay();
  delay (2000);
  VONz = EEPROM.readInt  (evonZ);  // Lendo na EEPROM os dados gravados
  VTXz = EEPROM.readInt  (evtxZ);
  VRXz = EEPROM.readInt  (evrxZ);
  VTXr = EEPROM.readInt  (evtxR);
  for(int x=0; x<NAAz; x++) MediaONz[x] = VONz;
  for(int x=0; x<NAAz; x++) MediaRXz[x] = VRXz;
}

// PISCA PISCA--------------------------------------------------------------------------------------------------------------------------
void Pisca()
{
  
  if ((millis() - TimerPisca) < 998)
  {
    digitalWrite (LED, 0);
  }
  else
  {
    digitalWrite (LED, 1);
  }
  if ((millis() - TimerPisca) > 1000) TimerPisca = millis();
}
