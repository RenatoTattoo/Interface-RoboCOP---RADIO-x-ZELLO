/*
 Interface RoboCOP - GMRS Radio x ZELLO - by 2RUGE0007 Renato Druziani 2023

 This is the design of a controller for a GMRS radio gateway for the zello app.
 The idea is to make it simple to assemble without the need to open the radio or smartphone to collect signals,
 I use LDRs on the cell phone screen and LED tx/rx on the HT.
 Automated configuration steps and created constant self-tuning, reducing maintenance.

*/
 
// VERSÃO do Software --------------------------------------------------------------------------------------------------------------------------------
 #define Versao          "8.1.1"

// Bibliotecas ---------------------------------------------------------------------------------------------------------------------------------------
 #include <WiFiManager.h>         // por Tzapu     - Ver 2.0.16-rc.2
 #include <WiFi.h>                // por Arduino   - Ver 1.2.7
 #include <NTPClient.h>           // por Fabrice W - Ver 3.2.1
 #include <Adafruit_GFX.h>        // por Adafruit  - Ver 1.11.9
 #include <Adafruit_SSD1306.h>    // por Adafruit  - Ver 2.5.9
 #include <SoftwareSerial.h>      // por Dirk Kaar - Ver 8.1.0 (EspSoftwareSerial)
 #include <DFRobotDFPlayerMini.h> // por DFRobot   - Ver 1.0.6
 #include <EEPROM.h>   
           
// relação MENSAGEM  com  o  NÚMERO DO ARQUIVO no cartão SD-------------------------------------------------------------------------------------------
 #define RogerBEEPlocal   25      //Arquivo 01~24 correspondem aos áudios de HORA e MEIA: 01 = 00:30h ~ 24 = 23:30h
 #define RogerBEEPweb     26      //Arquivo 25~31 correspondem aos áudios OPERACIONAIS
 #define RogerBEEPerro    27               
 #define TelaAPAGOU       28
 #define LinkOFFLINE      29
 #define LinkONLINE       30
 #define ZelloFECHOU      31
 #define PrimeiraVinheta  32      //Arquivo 32 ou + correspondem aos áudios de VINHETAS diversas

// definições PORTAS----------------------------------------------------------------------------------------------------------------------------------
 #define BotaoBOOT        0
 #define LED              2
 #define TocandoMP3       4
 #define PTTzello         18
 #define PTTradio         23
 #define SinalLDRradio    34
 #define SinalLDRzello    35

// Definições dos endereços dos valores salvos na EEPROM----------------------------------------------------------------------------------------------
 #define EEPROM_SIZE      64
 #define eVolumeVOZ       10
 #define evonZ            20
 #define evtxZ            30
 #define edifonrxZ        40
 #define evtxR            50
 #define eHoraCerta       0
 #define eMensagens       1
 #define eStatus          2
 #define eRogerBEEPs      3
 #define eFuso            4

// Outras Definições ---------------------------------------------------------------------------------------------------------------------------------
 #define TempoDoPulso     150             //Define o tempo de acionamento para o PTT do Zello
 #define TempoDisplay     300000          //Define o tempo para que o LDC fique apagado depois da inatividade
 #define TempoSalvaAjuste 3600000         //Define de quanto em quanto tempo deve ser salvo os valores de Auto Ajuste
 #define TaxaAtualizacao  75              //Define de quantos em quantos milisegundos o Display é atualizado
 #define AntiTRAVA        300000000       //Define o tempo de detecção de travamento (em microsegundos) 300 mega = 5 minutos
 #define NAZ              250             //Número de amostragens para a média móvel da leitura do LDR do ZELLO
 #define NAAz             20              //Número de amostragens para a média móvel das correções de auto ajuste Zello
 #define NAR              200             //Número de amostragens para a média móvel da leitura do LDR do RADIO

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);
Adafruit_SSD1306 display(128, 64, &Wire);
hw_timer_t *CaoDeGuarda = NULL;
WiFiManager wm;
SoftwareSerial ComunicacaoSerial(16, 17);     // Pinos 16 RX, 17 TX para comunicação serial com o DFPlayer
DFRobotDFPlayerMini LOCUTORA;                 // nomeando DFPlayer como "LOCUTORA"
 
// Variáveis, arrays, funções, etc--------------------------------------------------------------------------------------------------------------------

 bool          wm_nonblocking    = false; 
 bool          WiFiOK            = false;
 bool          HC                = false;
 bool          DisplayAceso      = true;
 bool          FalarHORA         = false;
 bool          FalouHORA         = false;
 bool          FalarMensagens    = false;
 bool          FalouMensagem     = false;
 bool          FalarStatus       = false;
 bool          RogerBeeps        = false;
 bool          MensagemTA        = true;
 bool          MensagemTI        = true;
 bool          MensagemOFF       = true;
 bool          MensagemON        = true;
 byte          D                 = 30;
 byte          H                 = 0;
 byte          M                 = 0;
 byte          S                 = 0;
 byte          D2                = 0;
 byte          H2                = 0;
 byte          M2                = 0;
 byte          S2                = 0;
 byte          Volume            = 0;
 byte          Vinheta           = PrimeiraVinheta;
 byte          Narquivos         = 0;
 byte          TipoMOSTRADOR     = 1;

 int           LeituraZELLOatual = 0;
 int           LeituraRADIOatual = 0;
 int           LSz               = 0;
 int           VONz              = 0;
 int           VRXz              = 0;
 int           DifONRXz          = 0;
 int           L1z               = 0;
 int           LIz               = 0;
 int           LOFFz             = 0;
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
 unsigned long TimerAtividade    = millis();
 unsigned long TimerDeEntrada    = millis();
 unsigned long TimerAutoAjuste   = millis();
 unsigned long Crono             = millis();
 unsigned long CronoONz          = millis();
 unsigned long CronoRXz          = millis();
 int           MediaLDRzello[NAZ];
 int           MediaONz[NAAz];
 int           MediaLDRradio[NAR];

void IRAM_ATTR ReiniciaRoboCOP() // função que o Cão de guarda irá chamar, para reiniciar o Sistema em caso de travamento
{
  ESP.restart(); // Reiniciando 
}

// SETUP =============================================================================================================================================
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
  if (!LOCUTORA.begin(ComunicacaoSerial))      //Use softwareSerial para comunicação com mp3Player.
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
  Volume = EEPROM.readByte(eVolumeVOZ);        // Ajustando o Volume armazenado
  Fuso   = EEPROM.readByte(eFuso);
  LOCUTORA.setTimeOut(500);                    // ajustando o timeout da comunicação serial 500ms
  LOCUTORA.volume(Volume);                     // Ajustando o volume (0~30).
  FalarHORA      = EEPROM.readBool(eHoraCerta);
  FalarMensagens = EEPROM.readBool(eMensagens);
  FalarStatus    = EEPROM.readBool(eStatus);  
  RogerBeeps     = EEPROM.readBool(eRogerBEEPs);
  if (LOCUTORA.begin(ComunicacaoSerial)) display.println(F(" OK!!!"));
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
  VONz     = EEPROM.readInt(evonZ);  // Lendo na EEPROM os dados gravados
  DifONRXz = EEPROM.readInt(edifonrxZ);
  VRXz     = VONz - DifONRXz;
  VTXr     = EEPROM.readInt(evtxR);
  for(int x=0; x<NAAz; x++) MediaONz[x] = VONz;
  display.print  (F(" VONz : "));
  display.println(VONz); 
  display.print  (F(" VRXz : "));
  display.println(VRXz);
  display.print  (F(" GMT  : -"));
  display.println(Fuso);
  display.display();
  display.clearDisplay();
  delay(1000);
}

// LOOP ==============================================================================================================================================
void loop()
{
  LeituraZELLOatual = mmLDRzello();
  LeituraRADIOatual = mmLDRradio();
  Pisca();
  if ((millis() - TimerLOOP) > 20)
  {
    ESTADOzello();
    ESTADOradio();
    Display();

    // LINK TRANSMITINDO sinal -----------------------------------------------------------------------------------------------------------------------------
    if ((EstadoZello == "Recebendo") & (EstadoRadio == "Stand by"))
    {
      LigarPTTradio();
      while (EstadoZello == "Recebendo")
      {
        LeituraZELLOatual = mmLDRzello();
        LeituraRADIOatual = mmLDRradio();
        ESTADOzello();
        Display();
      }
      if (RogerBeeps) TOCAR(RogerBEEPweb);
      DesligarPTTradio();
    }
    
    // LINK RECEBENDO sinal --------------------------------------------------------------------------------------------------------------------------------
    if ((EstadoRadio == "Recebendo") & (EstadoZello == "On-line"))
    {
      LigarPTTzello();
      bool QSOok = false;
      while ((EstadoRadio == "Recebendo") & (EstadoZello == "Transmitindo"))
      {
        LeituraZELLOatual = mmLDRzello();
        LeituraRADIOatual = mmLDRradio();
        QSOok = true;
        ESTADOradio();
        Display();
      }
      if (RogerBeeps)
      {
        LigarPTTradio();
        DesligarPTTzello();
        if (QSOok) TOCAR(RogerBEEPlocal);
        else       TOCAR(RogerBEEPerro);
        QSOok = false;
        DesligarPTTradio();
      }
      else DesligarPTTzello();
    }

    // STATUS DO LINK ---------------------------------------------------------------------
    if (FalarStatus)
    { 
      // LINK ON-LINE ---------------------------------------------------------------------
      if ((EstadoZello == "On-line") & (MensagemON))
      {
        LigarPTTradio();
        TOCAR(LinkONLINE);
        TOCAR(RogerBEEPlocal);
        DesligarPTTradio();
        MensagemON = false;
      }

      // LINK OFF-LINE ---------------------------------------------------------------------
      if (EstadoZello == "Off-line") 
      {
        while (EstadoRadio == "Recebendo")
        {
          LeituraZELLOatual = mmLDRzello();
          LeituraRADIOatual = mmLDRradio();
          ESTADOradio();
          Display();
          MensagemOFF = true;
        }
        if (MensagemOFF)
        {
          LigarPTTradio();
          TOCAR(LinkOFFLINE);
          TOCAR(RogerBEEPerro);
          DesligarPTTradio();
          MensagemOFF = false;
          MensagemON  = true;
        }
      }
      else MensagemOFF = true;

      // LINK TELA APAGOU ------------------------------------------------------------------
      if (EstadoZello == "Tela Apagada")
      {
        while (EstadoRadio == "Recebendo")
        {
          LeituraZELLOatual = mmLDRzello();
          LeituraRADIOatual = mmLDRradio();
          ESTADOradio();
          Display();
          MensagemTA = true;
        }
        if (MensagemTA)
        {
          LigarPTTradio();
          TOCAR(TelaAPAGOU);
          TOCAR(RogerBEEPerro);
          DesligarPTTradio();
          MensagemTA = false;
          MensagemON = true;
        }
      }
      else MensagemTA = true;

      // LINK ZELLO FECHOU ------------------------------------------------------------------
      if (EstadoZello == "Tela Inicial")
      {
        while (EstadoRadio == "Recebendo")
        {
          LeituraZELLOatual = mmLDRzello();
          LeituraRADIOatual = mmLDRradio();
          ESTADOradio();
          Display();
          MensagemTI = true;
        }
        if (MensagemTI)
        {
          LigarPTTradio();
          TOCAR(ZelloFECHOU);
          TOCAR(RogerBEEPerro);
          DesligarPTTradio();
          MensagemTI = false;
          MensagemON = true;
        }
      }
      else MensagemTI = true;
    }

    // LINK TX HORA E MEIA --------------------------------------------------------------------
    if (FalarHORA) 
    {
      if ((HC) & (M == 30) & (!FalouHORA) & (EstadoZello == "On-line") & (EstadoRadio == "Stand by"))
      {
        LigarPTTradio();
        TOCAR(H + 1);
        TOCAR(RogerBEEPlocal);
        DesligarPTTradio();
        FalouHORA = true;
      }
      if (M > 55) FalouHORA = false;
    }

    // LINK TX MENSAGENS 15 e 45 minutos ------------------------------------------------------
    if (FalarMensagens) 
    {
      if (((M == 10) | (M == 50)) & (S == 0)) FalouHORA = false;
      if ((!FalouMensagem) & (EstadoZello == "On-line") & (EstadoRadio == "Stand by") & ((millis() - TimerAtividade) > 65000))
      {
        LigarPTTradio();
        TOCAR(Vinheta);
        DesligarPTTradio();
        Vinheta++;
        if (Vinheta > Narquivos) Vinheta = PrimeiraVinheta;
        FalouMensagem = true;
      }
    }
    TimerLOOP = millis();
  }
}

// MP3 e ACIONAMENTOS PTTs ===========================================================================================================================
void TOCAR(byte Arquivo)
{
  LOCUTORA.play (Arquivo);
  while (digitalRead(TocandoMP3))
  {
    LeituraZELLOatual = mmLDRzello();
    LeituraRADIOatual = mmLDRradio();
    ValoresLimites();
    Display();
  }
  while (!digitalRead(TocandoMP3))
  {
    LeituraZELLOatual = mmLDRzello();
    LeituraRADIOatual = mmLDRradio();
    ValoresLimites();
    Display();
  }
}

void LigarPTTzello()
{
  if (EstadoZello == "On-line")
  {
    digitalWrite (PTTzello, HIGH);
    delay (TempoDoPulso);
    digitalWrite (PTTzello, LOW);
    while ((LeituraZELLOatual > L1z) & (LeituraZELLOatual < LSz))
    {
      LeituraRADIOatual = mmLDRradio();
      LeituraZELLOatual = mmLDRzello();
      ValoresLimites();
      Display();
    }
    if ((LeituraZELLOatual > LIz) & (LeituraZELLOatual <= L1z)) EstadoZello = "Transmitindo";
    else ESTADOzello();
  }
  
}

void DesligarPTTzello()
{
  LeituraZELLOatual = mmLDRzello();
  if (EstadoZello == "Transmitindo")
  {
    digitalWrite (PTTzello, HIGH);
    delay (TempoDoPulso);
    digitalWrite (PTTzello, LOW);
    while ((LeituraZELLOatual > LIz) & (LeituraZELLOatual < L1z))
    {
      LeituraRADIOatual = mmLDRradio();
      LeituraZELLOatual = mmLDRzello();
      ValoresLimites();
      Display();
    }
    Crono = millis();
    while ((millis() - Crono) < 300)
    {
      LeituraRADIOatual = mmLDRradio();
      LeituraZELLOatual = mmLDRzello();
      ValoresLimites();
      ESTADOzello();
      Display();
      TimerAtividade = millis();
    }
  }
  
}

void LigarPTTradio()
{
  digitalWrite (PTTradio, HIGH);
  Crono = millis();
  while ((LeituraRADIOatual < L1r) & ((millis() - Crono) <= 1500))
  {
    LeituraRADIOatual = mmLDRradio();
    LeituraZELLOatual = mmLDRzello();
    ValoresLimites();
    Display();
  }
  if ((millis() - Crono) > 1500)
  {
    digitalWrite (PTTradio, LOW);
    EstadoRadio = "FALHOU!!!!";
  }
  else EstadoRadio = "Transmitindo";
}

void DesligarPTTradio()
{
  digitalWrite (PTTradio, LOW);
  while (LeituraRADIOatual > L1r)
  {
    LeituraRADIOatual = mmLDRradio();
    LeituraZELLOatual = mmLDRzello();
    ValoresLimites();
    Display();
    TimerAtividade = millis();
  }
}

// LDRs - FUNÇÕES MEDIA MOVEL de LEITURA e AUTO AJUSTE ===============================================================================================
long mmLDRzello()
{
  for(int x=NAZ-1; x>0; x--) MediaLDRzello[x] = MediaLDRzello[x-1];
  MediaLDRzello[0] = analogRead (SinalLDRzello);
  delay(1);  
  long contador = 0;
  for (int x=0; x<NAZ; x++) contador += MediaLDRzello[x];
  return (contador/NAZ);
}

long mmONRXz()
{
  if (EstadoZello == "On-line")
  {
    for(int x= NAAz-1; x>0; x--) MediaONz[x] = MediaONz[x-1];
    MediaONz[0] = LeituraZELLOatual;
    long contador = 0;
    for (int x=0; x<NAAz; x++) contador += MediaONz[x];
    return (contador/NAAz);
  }
  if (EstadoZello == "Recebendo")
  {
    for(int x= NAAz-1; x>0; x--) MediaONz[x] = MediaONz[x-1];
    MediaONz[0] = (LeituraZELLOatual + DifONRXz);
    long contador = 0;
    for (int x=0; x<NAAz; x++) contador += MediaONz[x];
    return (contador/NAAz);
  }
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

// RELOGIO, CÁLCULOS DOS LIMITES e STATUS do HT e CLIENTE ZELLO ======================================================================================
void RELOGIO()
{
  if ((!HC) & (D>29) & (M==2) & (S==0) & (WiFiOK)) HoraCerta(); // verificando QUANDO ajustar o relogio.
  if (millis() > Relogio)
  { 
    // Contagem do RELOGIO
    Relogio+=1000;                          // recarregando contador do relogio com 1 segundo.
    S++;                                    // Aumentando 1 segundo.
    if (S > 59) 
    { 
      M++; S=0;                             // Aumentando 1 minuto.
      timerWrite(CaoDeGuarda, 0);           // Alimentando o "cachorrinho" 5 minutos sem zerar este timer a RoboCOP reinicia.
    }
    if (M > 59) { H++; M=0; }               // Aumentando 1 hora.
    if (H > 23) { H=0; D++; }               // Aumentando 1 dia.
    // Contagem do TEMPO LIGADO
    S2++;                                   // recarregando contador do "tempo ligado" com 1 segundo.
    if (S2 > 59) { M2++; S2=0; }            // Aumentando 1 minuto.
    if (M2 > 59) { H2++; M2=0; }            // Aumentando 1 hora.
    if (H2 > 23) { H2=0; D2++; }            // Aumentando 1 dia.
  }
}

void ValoresLimites()
{
  //Limites ZELLO
  VRXz  = VONz -  DifONRXz;
  LSz   = VONz +  DifONRXz;            // calculando o limite superor da escala.
  L1z   = VONz - (DifONRXz / 2);       // calculando o limite entre os valores ON e RX.
  LIz   = VONz - (DifONRXz * 2);       // calculando o limite inferior da escala.
  LOFFz = VONz / 10;

  // Limites RADIO
  if (LSr == 0) LSr = VTXr;                                       // carregando valor inicial para o limite superior da escala do radio.
  if (LeituraRADIOatual > LSr) LSr = (LeituraRADIOatual * 1.1);   // se estrapolar aumenta em 10% o limite superior.
  L1r = LSr / 3;                                                  // calculando limite de ativação entre stand-by e RX/TX. 
}

void ESTADOzello()
{
  ValoresLimites();
  int Porcentagem = ((NAZ / 100) * 15);
  if ((MediaLDRzello[NAZ-1] > (MediaLDRzello[0] - Porcentagem)) & (MediaLDRzello[NAZ-1] < (MediaLDRzello[0] + Porcentagem)))
  {
    if ((millis() - TimerDeEntrada) > 10)                 //Aguardar a leitura se estabilizar por este tempo
    {
      if  (LeituraZELLOatual > LSz)                                EstadoZello = "Tela Inicial";
      if ((LeituraZELLOatual > L1z)   & (LeituraZELLOatual < LSz)) EstadoZello = "On-line";
      if ((LeituraZELLOatual > LIz)   & (LeituraZELLOatual < L1z)) EstadoZello = "Recebendo";
      if ((LeituraZELLOatual > LOFFz) & (LeituraZELLOatual < LIz)) EstadoZello = "Off-line";
      if  (LeituraZELLOatual < LOFFz)                              EstadoZello = "Tela Apagada";
    } 
  }
  else TimerDeEntrada = millis();
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
  if (EstadoZello != "On-line")   CronoONz = millis();
  if (EstadoZello != "Recebendo") CronoRXz = millis();

  if ((EstadoZello == "On-line") & ((millis() - CronoONz) > 10000))
  {  
    VONz = mmONRXz();    
    CronoONz = millis() - 9000; 
  }

  if ((EstadoZello == "Recebendo") & ((millis() - CronoRXz) > 10000))
  {  
    VONz = mmONRXz();    
    CronoRXz = millis() - 9000; 
  }

  if (millis() - TimerAutoAjuste > TempoSalvaAjuste)     // Hora em hora verifica e salva na EEPROM os dados ajustados
  {
    if (EEPROM.readInt (evonZ) != VONz)
    {
      EEPROM.writeInt (evonZ, VONz);
      EEPROM.commit(); 
    }     
    TimerAutoAjuste = millis();
  }
}

// DISPLAY ===========================================================================================================================================
void Display()
{
  RELOGIO();
  if (((millis() - TimerLCD) > TempoDisplay) & ((EstadoZello != "On-line") | (EstadoRadio != "Stand by")))
  {
    TimerLCD = (millis() - (TempoDisplay - 10000)); //Ativando Display por 10 segundos em caso de atividade
  }
  if ((millis() - TimerFPS) > TaxaAtualizacao)
  {
    if (!digitalRead(BotaoBOOT)) BootApertado();
    if ((millis() - TimerLCD) < TempoDisplay)
    {
      display.clearDisplay();
      int LeituraZ  = map (LeituraZELLOatual, LIz, LSz,  0,  95);
      int ColunaONz = map (             VONz, LIz, LSz, 31, 128);
      int ColunaL1z = map (              L1z, LIz, LSz, 31, 128);
      int ColunaRXz = map (             VRXz, LIz, LSz, 31, 128);
      
      // BARRA VU ZELLO -------------------------------------------------------------------------------
      display.fillRect ( 31, 0, LeituraZ,  7, SSD1306_WHITE);
      if ((LeituraZ + 31) < ColunaL1z) display.drawLine ( ColunaL1z, 0, ColunaL1z,  7, SSD1306_WHITE);
      else                             display.drawLine ( ColunaL1z, 0, ColunaL1z,  7, SSD1306_BLACK);
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor (0, 0);
      display.print  (F("ZELLO"));
      display.setCursor ((ColunaRXz - 5) , 0);
      if ((LeituraZ + 31) > (ColunaRXz - 2)) display.setTextColor(SSD1306_BLACK);
      else                                   display.setTextColor(SSD1306_WHITE);
      if (EstadoZello == "Transmitindo")     display.print  (F("T"));
      else                                   display.print  (F("R"));
      if ((LeituraZ + 31) > (ColunaRXz + 2)) display.setTextColor(SSD1306_BLACK);
      else                                   display.setTextColor(SSD1306_WHITE);
      display.print  (F("X"));
      display.setCursor ((ColunaONz - 5), 0);
      if ((LeituraZ + 31) > (ColunaONz - 2)) display.setTextColor(SSD1306_BLACK);
      else                                   display.setTextColor(SSD1306_WHITE);
      display.print  (F("O"));
      if ((LeituraZ + 31) > (ColunaONz + 2)) display.setTextColor(SSD1306_BLACK);
      else                                   display.setTextColor(SSD1306_WHITE);
      display.print  (F("N"));

      // BARRA VU RADIO -------------------------------------------------------------------------------
      int LeituraR  = map ( LeituraRADIOatual, 0, LSr,  0, 95);
      int ColunaL1r = map ( L1r,               0, LSr, 31, 128);
      display.fillRect ( 31, 9, LeituraR,  7, SSD1306_WHITE);
      if ((LeituraR + 31) < ColunaL1r) display.drawLine ( ColunaL1r, 9, ColunaL1r, 15, SSD1306_WHITE);
      else                             display.drawLine ( ColunaL1r, 9, ColunaL1r, 15, SSD1306_BLACK);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor (0, 9); 
      display.print   (F("RADIO"));
      
      // INFORMAÇÕES ----------------------------------------------------------------------------------
      if (TipoMOSTRADOR == 1)
      {
        display.drawLine ( 0, 17, 128, 17, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor (0, 21);
        display.print  (F("RoboCOP:"));
        display.setCursor (104, 20);
        if (EstadoZello == "Off-line")     display.print (F("OF"));
        if (EstadoZello == "On-line")      display.print (F("ON"));
        if (EstadoZello == "Recebendo")    display.print (F("TX"));
        if (EstadoZello == "Transmitindo") display.print (F("RX"));
        if (EstadoZello == "Tela Inicial") display.print (F("TI"));
        display.drawLine ( 0, 38, 128, 38, SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (33, 41);
        if (HC) 
          display.println(F("HORA CERTA"));
        else
          display.println(F("CRONOMETRO"));
        display.setTextSize(2);
        display.setCursor (15, 50);
        if (H<10) display.print (F("0"));
        display.print             (H);
        display.print           (F(":"));
        if (M<10) display.print (F("0"));
        display.print             (M);
        display.print           (F(":"));
        if (S<10) display.print (F("0"));
        display.println           (S);
      }
      // INFORMAAÇÔES ZELLO ---------------------------------------------------------------------------
      if (TipoMOSTRADOR == 2)
      {
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (36, 16);
        display.print  (VRXz);
        display.setCursor (96, 16);
        display.print  (VONz);
        display.setCursor (67, 16);
        display.print  (L1z);

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (0, 26);
        display.print  (F("Zello: "));
        display.print  (EstadoZello);
        display.setCursor (0, 36);
        display.print  (F("LDRz : "));
        display.print  (LeituraZELLOatual);
        display.setCursor (0, 46);
        display.print  (F("Radio: "));
        display.print  (EstadoRadio);
        display.setCursor (0, 56);
        display.print  (F("LDRr : "));
        display.print  (LeituraRADIOatual);
      }
      // INFORMAAÇÔES RÁDIO ---------------------------------------------------------------------------
      if (TipoMOSTRADOR == 3)
      {
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (96, 16);
        display.print  (VTXr);
        display.setCursor (67, 16);
        display.print  (L1r);

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (0, 26);
        display.print  (F("Radio: "));
        display.print  (EstadoRadio);
        display.setCursor (0, 36);
        display.print  (F("LDRr : "));
        display.print  (LeituraRADIOatual);
        display.setCursor (0, 46);
        display.print  (F("Zello: "));
        display.print  (EstadoZello);
        display.setCursor (0, 56);
        display.print  (F("LDRz : "));
        display.print  (LeituraZELLOatual);
      }
      // TEMPO LIGADO -------------------------------------------------------------------------------
      if (TipoMOSTRADOR == 4)
      {
        display.drawLine ( 0, 28, 128, 28, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (27, 18);
        display.println(F("TEMPO LIGADO"));
        display.setTextSize(2);
        display.setCursor (0, 33);
        if (D2<10) display.print (F("0"));
        display.print              (D2);
        if (D2>1) display.println (F(" Dias "));
        else      display.println (F(" Dia  "));
        if (H2<10) display.print  (F("0"));
        display.print               (H2);
        display.print             (F(":"));
        if (M2<10) display.print  (F("0"));
        display.print               (M2);
        display.print             (F(":"));
        if (S2<10) display.print  (F("0"));
        display.println             (S2);
      }
      // DISPLAY APAGADO ----------------------------------------------------------------------------
      if (TipoMOSTRADOR == 5)
      {
        display.clearDisplay();
      }
    }
  TimerFPS = millis();
  display.display();
  display.clearDisplay();
  }
}

// TELA OK -------------------------------------------------------------------------------------------------------------------------------------------
void telaOK()
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
}

// SENSOR DO BOTÃO -----------------------------------------------------------------------------------------------------------------------------------
void BootApertado()
{
  digitalWrite (LED, 1);
  delay (200);
  digitalWrite (LED, 0);
  Crono = millis();
  byte Z = 0;
  while (!digitalRead(BotaoBOOT)) 
  {
    if ((millis() - Crono) > 500)
    {
      digitalWrite (LED, 1);
      delay (100);
      digitalWrite (LED, 0);
      Crono = millis();
      Z++;
      if (Z > 3)
      {
        MenuINICIAL();
        TipoMOSTRADOR--;
      }
    } 
  }
  delay (100);
  if ((millis() - TimerLCD) < TempoDisplay) TipoMOSTRADOR++;
  if (TipoMOSTRADOR > 5) TipoMOSTRADOR = 1;
  TimerLCD = millis();
}

// HEART BEAT ----------------------------------------------------------------------------------------------------------------------------------------
void Pisca()
{
  if ((millis() - TimerPisca) < 999)
  {
    digitalWrite (LED, 0);
  }
  else
  {
    digitalWrite (LED, 1);
  }
  if ((millis() - TimerPisca) > 1000) TimerPisca = millis();
}

// CONFIGURAÇÕES MENU INICIAL ========================================================================================================================
void MenuINICIAL()
{
  digitalWrite (PTTradio, LOW);
  telaOK();
  bool Sair  = false;
  byte opcao = 1;
  while (!Sair)
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
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      byte Z = 0;
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - Crono) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          Crono = millis();
          Z++;
          if (Z > 3) 
          {
            switch (opcao)
            {
              case 1: 
                opcao = 0;
                MenuSETUPbasico();
              break;
              case 2:
                opcao = 0;
                MenuSETUPmensagens(); 
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
                telaOK();
                Sair = true; 
              break;      
              case 6:
                telaOK();
                ESP.restart(); 
              break;
            }
          }
        }
      }
      opcao++;
      if (opcao == 7) opcao = 1;      
    }
    display.display();
  }
}

// CONFIGURAÇÕES BÁSICAS -----------------------------------------------------------------------------------------------------------------------------
void MenuSETUPbasico()
{
  telaOK();
  unsigned long Crono2 = millis();
  bool Voltar            = false;
  byte opcao           = 1;
  while (!Voltar)
  {
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
    if (opcao == 4) display.println(F(">CALIBRAR LDR RADIO"));
    else            display.println(F(" Calibrar LDR radio"));
    if (opcao == 5) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      byte Z = 0;
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - Crono) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          Crono = millis();
          Z++;
          if (Z > 3) 
          {
            switch (opcao)
            {
              case 1: 
                ADDPTTZello();
                break;
              case 2:
                PosicionarLDRZello(); 
                break;
              case 3:
                CalibrarLDRzello(); 
                break;
              case 4:
                CalibrarLDRradio(); 
                break;
              case 5:
                telaOK();
                Voltar = true; 
                break;
            }
          }
        }
      }
      opcao++;
      if (opcao > 5) opcao = 1;      
    }
    display.display();
  }
  opcao = 0;
}

void ADDPTTZello()
{
  telaOK();
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

void PosicionarLDRZello()
{
  telaOK();
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

void CalibrarLDRzello()
{
  telaOK();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("INSTRUCAO PARA AJUSTE"));
  display.println(F("---------------------"));
  display.println(F("Deixe o Zello on-line"));
  display.println(F("  e no usuario ECHO"));
  display.println(F("antes de continuar..."));
  display.println(F("---------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.display();
  while (digitalRead(BotaoBOOT)) {mmLDRzello();}
   
  // Procedimento 1/3 - Coletando valor ONLINE -----------------------------------------------------------------------------------------
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
      display.println(F("1.3 CALIBRANDO ONLINE"));
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
  
  // Procedimento 2/3 - Coletando valor TX ---------------------------------------------------------------------------------------------
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
      display.println(F("2.3 CALIBRANDO EM TX"));
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
    
  // Procedimento 3/3 - Coletando valor RX ---------------------------------------------------------------------------------------------
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
      display.println(F("3.3 CALIBRANDO EM RX"));
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

  // SALVAR DADOS NA EEPROM ---------------------------------------------------------------------------------------------------------------
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F(" SALVANDO CALIBRACAO "));
  display.println(F("---------------------"));
  display.println();
  VONz = PTTZlaranja;     
  int VTXz = PTTZverde;
  VRXz = PTTZvermelho;
  if (VRXz > VTXz) DifONRXz = (VONz - VRXz);
  if (VTXz > VRXz) DifONRXz = (VONz - VTXz);
  display.println(PTTZlaranja);                
  display.println(PTTZverde);
  display.println(PTTZvermelho);
  display.display();
  display.clearDisplay();
   
  EEPROM.writeInt (evonZ, VONz);  // Gravando valores na EEPROM
  EEPROM.writeInt (edifonrxZ, DifONRXz);
  EEPROM.commit();
  
  delay (2000);
  
  for(int x=0; x<NAAz; x++) MediaONz[x] = VONz;
}

void CalibrarLDRradio()
{
  telaOK();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("INSTRUCAO PARA AJUSTE"));
  display.println(F("---------------------"));
  display.println(F("Deixe o Radio ligado "));
  display.println(F("antes de continuar..."));
  display.println(F("---------------------"));
  display.println(F(" E PRESSIONE O BOOT"));
  display.display();
  while (digitalRead(BotaoBOOT)) {mmLDRradio();}
  int PTTRvermelho = mmLDRradio();
  digitalWrite (PTTradio, HIGH);
  Crono  = millis();
  unsigned long Crono2 = millis();      
  while ((millis() - Crono) < 7500)
  {
    mmLDRradio();
    if ((millis() - Crono2) > 100)      
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("  LENDO TX no RADIO"));
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

  // SALVAR DADOS NA EEPROM ---------------------------------------------------------------------------------------------------------------
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F(" SALVANDO CALIBRACAO "));
  display.println(F("---------------------"));
  display.println();
  display.println(PTTRvermelho); 
   
  EEPROM.writeInt (evtxR, PTTRvermelho);  // Gravando valores na EEPROM  
  EEPROM.commit();
  display.display();
  display.clearDisplay();
  VTXr = PTTRvermelho;
  delay (2000);
}

// CONFIGURAÇÕES MENSAGENS ---------------------------------------------------------------------------------------------------------------------------
void MenuSETUPmensagens()
{
  telaOK();
  bool Voltar  = false;
  byte opcao = 1;
  while (!Voltar)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (36, 0);
                    display.println(F("MENSAGENS"));
                    display.println(F("---------------------")); 
    if (opcao == 1) display.print  (F(">FALAR HORA  "));
    else            display.print  (F(" Falar Hora  ")); 
    display.println                  (FalarHORA); 
    if (opcao == 2) display.print  (F(">MENSAGENS   "));
    else            display.print  (F(" Mensagens   "));
    display.println                  (FalarMensagens);
    if (opcao == 3) display.print  (F(">ESTADO LINK "));
    else            display.print  (F(" Estado Link "));
    display.println                  (FalarStatus);
    if (opcao == 4) display.print  (F(">ROGER BEEP  "));
    else            display.print  (F(" Roger Beep  "));
    display.println                  (RogerBeeps);
    if (opcao == 5) display.print  (F(">VOLUME MP3  "));
    else            display.print  (F(" Volume mp3  "));
    display.println                  (Volume);
    if (opcao == 6) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      byte Z = 0;
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - Crono) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          Crono = millis();
          Z++;
          if (Z > 3)  
          {
            digitalWrite (LED, 1);
            delay(1000);
            digitalWrite (LED, 0);
            switch (opcao)
            {
              case 1: 
                FalarHORA = !FalarHORA;
                break;
              case 2:
                FalarMensagens = !FalarMensagens; 
                break;
              case 3:
                FalarStatus = !FalarStatus; 
                break;
              case 4:
                RogerBeeps = !RogerBeeps;
                break;  
              case 5:
                Volume = (Volume + 1); 
                if (Volume > 30) Volume = 5;
                LOCUTORA.volume(Volume);
                LigarPTTradio();
                TOCAR(LinkONLINE);
                DesligarPTTradio();
                opcao--;
                break;
              case 6:
                Voltar = true; 
                telaOK();
                while (!digitalRead(BotaoBOOT)) {};
                break;
            }
          }
        }
      }
      opcao++;
      if (opcao == 7) opcao = 1;      
    }
    display.display();
  }
  // Verificar e salvar os dados -------------------------------------------------------------------------------
  if (FalarHORA != EEPROM.readBool(eHoraCerta)) 
  {
    EEPROM.writeBool(eHoraCerta, FalarHORA);
    EEPROM.commit();
  }
  if (FalarMensagens != EEPROM.readBool(eMensagens)) 
  {
    EEPROM.writeBool(eMensagens, FalarMensagens);
    EEPROM.commit();
  }
  if (FalarStatus != EEPROM.readBool(eStatus)) 
  {
    EEPROM.writeBool(eStatus, FalarStatus);
    EEPROM.commit();
  }
  if (RogerBeeps != EEPROM.readBool(eRogerBEEPs)) 
  {
    EEPROM.writeBool(eRogerBEEPs, RogerBeeps);
    EEPROM.commit();
  }
  if (Volume != EEPROM.readByte(eVolumeVOZ)) 
  {
    EEPROM.writeByte (eVolumeVOZ, Volume);
    EEPROM.commit();
  }
}

// CONFIGURAÇÕES WiFi --------------------------------------------------------------------------------------------------------------------------------
void MenuSETUPwifi()
{
  telaOK();
  bool Voltar = false;
  byte opcao  = 1;
  while (!Voltar)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (0, 0);
                    display.println(F("        Wi-Fi"));
                    display.println(F("---------------------")); 
    if (opcao == 1) display.println(F(">LIMPAR CONFIGURACAO"));
    else            display.println(F(" Limpar Configuracao"));  
    if (opcao == 2) display.println(F(">CONFIGURAR NOVA REDE"));
    else            display.println(F(" Configurar Nova Rede"));
    if (opcao == 3) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      byte Z = 0;
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - Crono) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          Crono = millis();
          Z++;
          if (Z > 3) 
          {
            switch (opcao)
            {
              case 1: 
              {
                telaOK();
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
                telaOK();
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
                Voltar = true; 
                telaOK();
                while (!digitalRead(BotaoBOOT)){}
              }
              break;     
            }
          }
        }
      }
      opcao++;
      if (opcao == 4) opcao = 1;      
    }
    display.display();
  }
}

void saveParamCallback()
{
  // apenas callback
}

// CONFIGURAÇÕES RELOGIO -----------------------------------------------------------------------------------------------------------------------------
void MenuSETUPrelogio()
{
  telaOK();
  bool Voltar  = false;
  byte opcao = 1;
  byte Z = 0;
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
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      byte Z = 0;
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - Crono) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          Crono = millis();
          Z++;
          if (Z > 3) 
          {
            switch (opcao)
            {
              case 1: 
                HoraCerta();
              break;
              case 2:
                FusoHorario();
              break;
              case 3:
                telaOK();
                Voltar = true; 
                while (!digitalRead(BotaoBOOT)){}
              break;
            }
          }
        }
      }
      opcao++;
      if (opcao > 3) opcao = 1;      
    }
    display.display();
  }
}

void SincronizarNTP()
{
  wm.setConfigPortalTimeout(1);
  WiFiOK = wm.autoConnect();
  delay(500);
  Fuso = EEPROM.readByte (eFuso);
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

void HoraCerta()
{  
  telaOK();
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

void FusoHorario()
{
  telaOK();
  bool Voltar  = false;
  byte opcao   = Fuso;
  byte Z       = 0;
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
    if (opcao == 2) display.println(F(">FERNANDO DE NORONHA"));
    else            display.println(F(" Fernando de Noronha"));  
    if (opcao == 3) display.println(F(">BRASILIA"));
    else            display.println(F(" Brasilia"));
    if (opcao == 4) display.println(F(">MANAUS"));
    else            display.println(F(" Manaus"));
    if (opcao == 5) display.println(F(">RIO BRANCO"));
    else            display.println(F(" Rio Branco"));
    if (opcao == 6) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    Crono = millis();
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      Crono = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - Crono) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          Crono = millis();
          Z++;
          if (Z > 3) 
          {
            if ((opcao > 1) & (opcao < 6)) Fuso = opcao;
            if (opcao == 6) 
            { 
              Voltar = true; 
              telaOK();
              while (!digitalRead(BotaoBOOT)) {}
            } 
          }
        }
      }
      opcao++;
      if (opcao > 6) opcao = 2;      
    }
    display.display();
  }
  if (Fuso != EEPROM.readByte (eFuso))
  {
    EEPROM.writeByte (eFuso, Fuso);
    EEPROM.commit();
  }
}
