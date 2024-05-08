/*
 Interface RoboCOP - GMRS Radio x ZELLO - by 2RUGE0007 Renato Druziani 2023

 This is the design of a controller for a GMRS radio gateway for the zello app.
 The idea is to make it simple to assemble without the need to open the radio or smartphone to collect signals,
 I use LDRs on the cell phone screen and LED tx/rx on the HT.
 Automated configuration steps and created constant self-tuning, reducing maintenance.

*/
 
// VERSÃO do Software --------------------------------------------------------------------------------------------------------------------------------
 #define Versao          "10.0.2"

// Bibliotecas ---------------------------------------------------------------------------------------------------------------------------------------
 #include <WiFiManager.h>         // por Tzapu     - Ver 2.0.17
 #include <WiFi.h>                // por Arduino   - Ver 1.2.7
 #include <NTPClient.h>           // por Fabrice W - Ver 3.2.1
 #include <Adafruit_GFX.h>        // por Adafruit  - Ver 1.11.9
 #include <Adafruit_SSD1306.h>    // por Adafruit  - Ver 2.5.10
 #include <SoftwareSerial.h>      // por Dirk Kaar - Ver 8.1.0 (pesquise por EspSoftwareSerial)
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
 #define FANradio         19
 #define PTTradio         23
 #define SinalLDRradio    34
 #define SinalLDRzello    35
 #define NTCradio         39

// Definições dos endereços dos valores salvos na EEPROM----------------------------------------------------------------------------------------------
 #define EEPROM_SIZE      64
 #define eVolumeVOZ       10
 #define evonZ            20
 #define evrxZ            30
 #define edifrxonZ        40
 #define evtxR            50
 #define eTMPlimite       15
 #define eTMPctrl         25
 #define ePeriodoTMP      35
 #define eHoraCerta       0
 #define eMensagens       1
 #define eStatus          2
 #define eRogerBEEPs      3
 #define eFuso            4

// Outras Definições ---------------------------------------------------------------------------------------------------------------------------------
 #define TempoDoPulso     150             //Define o tempo de acionamento para o PTT do Zello
 #define TempoDisplay     300000          //Define o tempo para que o LDC fique apagado depois da inatividade
 #define TempoSalvaAjuste 3600000         //Define de quanto em quanto tempo deve ser salvo os valores de Auto Ajuste
 #define DezMinutos       600000          //Define um tempo de 10 minutos
 #define CincoMinutos     300000          //Define um tempo de  5 minutos
 #define CincoSegundos    5000            //Define um tempo de  5 segundos
 #define TaxaAtualizacao  75              //Define de quantos em quantos milisegundos o Display é atualizado
 #define AntiTRAVA        300000000       //Define o tempo de detecção de travamento (em microsegundos) 300 mega = 5 minutos
 #define NAZ              150             //Número de amostragens para a média móvel da leitura do LDR do ZELLO
 #define NAAz             30              //Número de amostragens para a média móvel das correções de auto ajuste Zello
 #define NAR              300             //Número de amostragens para a média móvel da leitura do LDR do RADIO
 #define NAT              1000            //Número de amostragens para a média móvel da leitura do NTC do RADIO
 #define Vs               3.3             //Tensão de saída no ESP32
 #define R1               10000           //Resistor usado no PULLUP
 #define Beta             3380            //Valor de BETA, cada tipo de termistor tem seu BETA, vide datasheet
 #define To               298.15          //Valor em °K referênte a 25°C
 #define Ro               10000           //Resistência do termistor à 25°C
 #define LeituraMax       4095.0          //Leitura máxima de resolução, a resolução do ADC noESP32 é 4095.0
 #define FANligado        0              
 #define FANdesligado     1

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
 bool          MP3OK             = false;
 bool          HC                = false;
 bool          FAN               = false;
 bool          RadioQUENTE       = false;
 bool          DisplayAceso      = true;
 bool          FalarHORA         = false;
 bool          FalouHORA         = false;
 bool          FalarMensagens    = false;
 bool          FalouMensagem     = false;
 bool          FalarStatus       = false;
 bool          RogerBeeps        = false;
 bool          TMPctrl           = false;
 bool          MensagemTA        = true;
 bool          MensagemTI        = true;
 bool          MensagemOFF       = true;
 bool          MensagemON        = true;
 bool          VUZ               = true;
 bool          VUR               = true;
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
 char          PeriodoTMP        = 0;
 byte          TipoMOSTRADOR     = 1;

 int           LeituraZELLOatual = 0;
 int           LeituraRADIOatual = 0;
 int           LSz               = 0;
 int           LONTIz            = 0;
 int           VONz              = 0;
 int           VRXz              = 0;
 int           DifRXONz          = 0;
 int           LOFFRXz           = 0;
 float         LAz               = 0;
 int           LIz               = 0;
 int           LTAOFFz           = 0;
 String        EstadoZELLO       = "XX";
 
 int           VTXr              = 0;
 int           LSr               = 0;
 int           LAr               = 0;
 String        EstadoRADIO       = "XX";

 String        EstadoLINK        = "XX";

 byte          Fuso              = 0;      //Fernando de Noronha(GMT-2), Brasília(GMT-3), Manaus - AM(GMT-4), Rio Branco - AC(GMT-5)
 unsigned long Relogio           = millis();
 unsigned long TimerLCD          = millis();
 unsigned long TimerFPS          = millis();
 unsigned long TimerTMP          = millis();
 unsigned long TimerLOOP         = millis();
 unsigned long TimerPisca        = millis();
 unsigned long TimerAtividade    = millis();
 unsigned long TimerESTADOzello = millis();
 unsigned long TimerESTADOradio = millis();
 unsigned long TimerAutoAjuste   = millis();
 unsigned long Crono             = millis();
 unsigned long CronoONz          = millis();
 unsigned long CronoRXz          = millis();
 int           MediaLDRzello[NAZ];
 int           MediaONz[NAAz];
 int           MediaRXz[NAAz];
 int           MediaLDRradio[NAR];

 int           MediaNTC[NAT];               // variáveis para os cálculo de temperatura
 float         RegistroTMP[128];
 float         Vout               = 0; 
 float         Rt                 = 0;
 float         TF                 = 0;
 float         TMPradio           = 0;
 int           LeituraNTC         = 0;
 float         TMPradioMAX        = 0;
 float         TMPradioMIN        = 200;
 byte          TMPlimite          = 0;

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
  pinMode(NTCradio,      INPUT_PULLUP);
  pinMode(BotaoBOOT,     INPUT_PULLUP);
  pinMode(LED,           OUTPUT);
  pinMode(PTTzello,      OUTPUT);
  pinMode(PTTradio,      OUTPUT);
  pinMode(FANradio,      OUTPUT);

  // PORTAS NÃO UTILIZADAS ---------------------------------------------------------------------------------------------------------------
  pinMode(5,  OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(36, INPUT_PULLDOWN);

  digitalWrite(5,  LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);
  digitalWrite(15, LOW);
  digitalWrite(21, LOW);
  digitalWrite(22, LOW);
  digitalWrite(23, LOW);
  digitalWrite(25, LOW);
  digitalWrite(26, LOW);
  digitalWrite(27, LOW);
  digitalWrite(32, LOW);
  digitalWrite(33, LOW);


  digitalWrite (LED, 1);
  digitalWrite (FANradio, FANligado);
    
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
  display.setCursor (90, 57);
  display.print(Versao);
  display.display();
  display.clearDisplay();
  delay(2000);
  display.setCursor (0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(F("LCD...  OK!"));
  Serial.println (F("LCD...  OK!"));
  display.display();
  Serial.end();
  digitalWrite (LED, 0);
  MP3OK = LOCUTORA.begin(ComunicacaoSerial);
  display.print  (F("MP3...  "));
  if (!MP3OK)      //Use softwareSerial para comunicação com mp3Player.
  {
    delay(500);
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
    byte tentativas = 10; 
    while ((!MP3OK) && (tentativas != 0))
    {
      digitalWrite (LED, 1);
      delay (10);
      digitalWrite (LED, 0);
      display.fillRect(0, 50,   128, 64, SSD1306_BLACK);
      display.setCursor (0, 50);
      display.print(F("tentando novamente.."));
      tentativas--;
      display.print  (tentativas);
      display.display();
      MP3OK = LOCUTORA.begin(ComunicacaoSerial);
      delay (100);
    }
    display.clearDisplay();
    display.setCursor (0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.println            (F("LCD...  OK!"));
    if (!MP3OK) display.println(F("MP3...  FALHOU!"));
    display.display();
    
  }
  if (MP3OK)
  {
    delay(500);
    display.clearDisplay();
    display.setCursor (0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.println (F("LCD...  OK!"));
    display.println (F("MP3...  OK!!"));
    display.display();
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
  }
  else
  {
    FalarHORA      = false;
    FalarMensagens = false;
    FalarStatus    = false;  
    RogerBeeps     = false;
  }
  
  delay(500);
  display.print  (F("WiFi... "));
  display.display();
  wm.setConfigPortalTimeout(1);
  WiFiOK = wm.autoConnect();
  delay(500);
  if (WiFiOK)
  {
    display.println(F("OK!!!"));
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
  else
  {
    display.println();
    display.println(F("FALHOU!"));
  }
  display.display();
  display.clearDisplay();
  delay(1000);
  
  // RECUPERAR DADOS
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("  CARREGANDO  DADOS"));
  display.println(F("---------------------"));
  VONz     = EEPROM.readInt(evonZ);  // Lendo na EEPROM os dados gravados
  VRXz     = EEPROM.readInt(evrxZ);
  for (int x = NAAz-1; x > 0; x--) MediaONz[x]      = VONz;
  for (int x = NAZ-1;  x > 0; x--) MediaLDRzello[x] = VONz;
  for (int x = NAAz-1; x > 0; x--) MediaRXz[x]      = VRXz;
  DifRXONz = (VONz - VRXz);
  LOFFRXz  = (VRXz / 2);
  LAz      = VONz - (DifRXONz / 2);
  VTXr     = EEPROM.readInt(evtxR);
  
  TMPctrl    = EEPROM.readBool(eTMPctrl);
  TMPlimite  = EEPROM.readByte(eTMPlimite);
  PeriodoTMP = EEPROM.readChar(ePeriodoTMP);
  if (TMPctrl)
  {
    for (int x = NAT; x > 0; x--) MediaNTC[x] = analogRead(NTCradio);
    for (int x = 0; x < 127; x++) RegistroTMP[x] = 0;
  }
  
  for(int x=0; x<NAAz; x++) MediaONz[x] = VONz;
  display.print  (F(" VONz : "));
  display.println(VONz); 
  display.print  (F(" VRXz : "));
  display.println(VRXz);
  display.print  (F(" GMT  : -"));
  display.println(Fuso);
  display.display();
  display.clearDisplay();
  digitalWrite (FANradio, FANdesligado);
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
    timerWrite(CaoDeGuarda, 0);           // Alimentando o "cachorrinho" 5 minutos sem zerar este timer a RoboCOP reinicia.

    // LINK TRANSMITINDO sinal -----------------------------------------------------------------------------------------------------------------------------
    if ((EstadoZELLO == "Recebendo") && (EstadoRADIO == "Stand by"))
    {
      LigarPTTradio();
      while (EstadoZELLO == "Recebendo")
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
    if ((EstadoRADIO == "Recebendo") && (EstadoZELLO == "On-line"))
    {
      LigarPTTzello();
      bool QSOok = false;
      while ((EstadoRADIO == "Recebendo") && (EstadoZELLO == "Transmitindo"))
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
      if ((EstadoZELLO == "On-line") && (MensagemON))
      {
        LigarPTTradio();
        TOCAR(LinkONLINE);
        TOCAR(RogerBEEPlocal);
        DesligarPTTradio();
        MensagemON = false;
      }

      // LINK OFF-LINE ---------------------------------------------------------------------
      if (EstadoZELLO == "Off-line") 
      {
        while (EstadoRADIO == "Recebendo")
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
      if (EstadoZELLO == "Tela Apagada")
      {
        while (EstadoRADIO == "Recebendo")
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
      if (EstadoZELLO == "Tela Inicial")
      {
        while (EstadoRADIO == "Recebendo")
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
      if ((HC) && (M == 30) && (!FalouHORA) && (EstadoZELLO == "On-line") && (EstadoRADIO == "Stand by"))
      {
        LigarPTTradio();
        TOCAR(H + 1);
        TOCAR(RogerBEEPlocal);
        DesligarPTTradio();
        FalouHORA = true;
      }
      if (M > 55) FalouHORA = false;
    }

    // LINK TX MENSAGENS 10 e 50 minutos ------------------------------------------------------
    if (FalarMensagens) 
    {
      if (((M == 10) | (M == 50)) && (S == 0)) FalouMensagem = false;
      if ((!FalouMensagem) && (EstadoZELLO == "On-line") && (EstadoRADIO == "Stand by") && ((millis() - TimerAtividade) > 65000))
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
  if (EstadoZELLO == "On-line")
  {
    digitalWrite (PTTzello, HIGH);
    delay (TempoDoPulso);
    digitalWrite (PTTzello, LOW);
    while ((LeituraZELLOatual > LAz) && (LeituraZELLOatual < LONTIz))
    {
      LeituraRADIOatual = mmLDRradio();
      LeituraZELLOatual = mmLDRzello();
      ValoresLimites();
      Display();
    }
    if ((LeituraZELLOatual > LOFFRXz) && (LeituraZELLOatual <= LAz)) EstadoZELLO = "Transmitindo";
    else ESTADOzello();
  }
  
}

void DesligarPTTzello()
{
  LeituraZELLOatual = mmLDRzello();
  if (EstadoZELLO == "Transmitindo")
  {
    digitalWrite (PTTzello, HIGH);
    delay (TempoDoPulso);
    digitalWrite (PTTzello, LOW);
    while ((LeituraZELLOatual > LOFFRXz) && (LeituraZELLOatual < LAz))
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
  if (!RadioQUENTE) digitalWrite (PTTradio, HIGH);
  Crono = millis();
  while ((LeituraRADIOatual < LAr) && ((millis() - Crono) <= 1500))
  {
    LeituraRADIOatual = mmLDRradio();
    LeituraZELLOatual = mmLDRzello();
    ValoresLimites();
    Display();
  }
  if ((millis() - Crono) > 1500)
  {
    digitalWrite (PTTradio, LOW);
    if (!RadioQUENTE) EstadoRADIO = "FALHOU!!!!";
    else              EstadoRADIO = "Radio QUENTE!";
  }
  else EstadoRADIO = "Transmitindo";
}

void DesligarPTTradio()
{
  digitalWrite (PTTradio, LOW);
  while (LeituraRADIOatual > LAr)
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

long mmONz()
{
  for(int x= NAAz-1; x>0; x--) MediaONz[x] =  MediaONz[x-1];
  MediaONz[0] = LeituraZELLOatual;
  long contador = 0;
  for (int x=0; x<NAAz; x++) contador += MediaONz[x];
  return (contador/NAAz);
}

long mmRXz()
{
  for(int x= NAAz-1; x>0; x--) MediaRXz[x] =  MediaRXz[x-1];
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

// RELOGIO, CÁLCULOS DOS LIMITES e STATUS do HT e CLIENTE ZELLO ======================================================================================
void RELOGIO()
{
  if ((!HC) && (D>29) && (M==2) && (S==0) && (WiFiOK)) HoraCerta(); // verificando QUANDO ajustar o relogio.
  if (millis() > Relogio)
  { 
    // Contagem do RELOGIO
    Relogio+=1000;                          // recarregando contador do relogio com 1 segundo.
    S++;                                    // Aumentando 1 segundo.
    if (S > 59) 
    { 
      M++; S=0;                             // Aumentando 1 minuto.
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
  AutoAjuste();
  DifRXONz = VONz - VRXz;          // Dif ON RX zello - calculando a diferença entre valor ONLINE e valor RX.
  LONTIz   = VONz + DifRXONz;      // LONTIz - estabelecendo o limite entre ONLINE e TELA INICIAL.
  LSz      = VONz + DifRXONz;      // estabelecendo o limite superior para usar na tela.
  if ((EstadoZELLO == "On-line")   && (LAz < (LeituraZELLOatual - ((DifRXONz / 100) * 66)))) LAz = LAz + 0.05;   // subindo  o limite quando em ON.
  if ((EstadoZELLO == "On-line")   && (LAz > (LeituraZELLOatual - ((DifRXONz / 100) * 66)))) LAz = LAz - 0.01;  // descendo o limite quando em ON.
  if ((EstadoZELLO == "Recebendo") && (LAz > (LeituraZELLOatual + ((DifRXONz / 100) * 40)))) LAz = LAz - 0.05;   // descendo o limite quando em RX.
  if ((EstadoZELLO == "Recebendo") && (LAz < (LeituraZELLOatual + ((DifRXONz / 100) * 40)))) LAz = LAz + 0.01;  // Subindo  o limite quando em RX.
  LIz     = VONz - (DifRXONz * 2); // estabelecendo o limite inferior para usar na tela.
  LOFFRXz = VRXz / 2;              // Estabelecendo o limite útil entre RX e OFF LINE.
  LTAOFFz = (VONz / 100) * 5;

  // Limites RADIO
  if (LSr == 0) LSr = VTXr;                                       // carregando valor inicial para o limite superior da escala do radio.
  if (LeituraRADIOatual > LSr) LSr = (LeituraRADIOatual * 1.1);   // se estrapolar aumenta em 10% o limite superior.
  LAr = LSr / 5;                                                  // calculando limite de ativação entre stand-by e RX/TX. 
  
}

void ESTADOzello()
{
  ValoresLimites();
  String EZA = EstadoZELLO;
    if ((millis() - TimerESTADOzello) > 750)     //Aguardar a leitura se estabilizar por este tempo
    {
      if  (LeituraZELLOatual > LONTIz)                                    EstadoZELLO = "Tela Inicial";
      if ((LeituraZELLOatual > LAz)     && (LeituraZELLOatual < LONTIz))  EstadoZELLO = "On-line";
      if ((LeituraZELLOatual > LOFFRXz) && (LeituraZELLOatual < LAz))     EstadoZELLO = "Recebendo";
      if ((LeituraZELLOatual > LTAOFFz) && (LeituraZELLOatual < LOFFRXz)) EstadoZELLO = "Off-line";
      if  (LeituraZELLOatual < LTAOFFz)                                   EstadoZELLO = "Tela Apagada";
    } 
  if (EZA != EstadoZELLO)
  {
    TimerESTADOzello = millis();
    CronoONz = millis();
    CronoRXz = millis();
  }
}

void ESTADOradio()
{
  ValoresLimites();
  String ERA = EstadoRADIO;
  if ((millis() - TimerESTADOradio) > 750)     //Aguardar a leitura se estabilizar por este tempo
  {
    if (LeituraRADIOatual >= LAr) EstadoRADIO = "Recebendo";
    if (LeituraRADIOatual <  LAr) EstadoRADIO = "Stand by";
  }
  if (ERA != EstadoRADIO)
  {
    TimerESTADOradio = millis();
  }
}

void AutoAjuste()
{
  if (EstadoZELLO != "On-line")   CronoONz = millis();
  if (EstadoZELLO != "Recebendo") CronoRXz = millis();
  Serial.println(F("AUTO AJUSTE")); 
  if ((EstadoZELLO == "On-line") && ((millis() - CronoONz) > 10000))
  {  
    VONz = mmONz();   
    Serial.println(F("mmONz")); 
    CronoONz = millis() - 9000; 
  }

  if ((EstadoZELLO == "Recebendo") && ((millis() - CronoRXz) > 10000))
  {  
    VRXz = mmRXz();   
    Serial.println(F("mmRXz")); 
    CronoRXz = millis() - 9000; 
  }

  if ((millis() - TimerAutoAjuste) > TempoSalvaAjuste)     // Hora em hora verifica e salva na EEPROM os dados ajustados
  {
    if ((EEPROM.readInt (evonZ)) != VONz)
    {
      EEPROM.writeInt (evonZ, VONz);
      EEPROM.commit(); 
    }   
    if ((EEPROM.readInt (evrxZ)) != VRXz)
    {
      EEPROM.writeInt (evrxZ, VRXz);
      EEPROM.commit(); 
    }    
    TimerAutoAjuste = millis();
  }
}

void TEMPERATURAradio()
{
  for (int x = NAT - 1; x > 0; x--) MediaNTC[x] = MediaNTC[x - 1];
  MediaNTC[0] = analogRead(NTCradio);
  long contador = 0;
  for (int x = 0; x < NAT; x++) contador += MediaNTC[x];
  LeituraNTC = (contador / NAT);

  Vout = LeituraNTC * 3.3 / LeituraMax;
  Rt = 10000 * Vout / (3.3 - Vout);
  TF = 1 / ((1 / 298.15) + (log(Rt / 10000) / Beta));
  TMPradio = TF - 273.15;

  if (TMPradio > TMPradioMAX) TMPradioMAX = TMPradio;
  if (TMPradio < TMPradioMIN) TMPradioMIN = TMPradio;

  int Periodo = 0;
  switch (PeriodoTMP)
  {
    case 1:
      Periodo = 1066;              // Gráfico de temperatura com 2 minutos de registro - a cada ~1s
    break; 
    case 2:
      Periodo = 56250;             // Gráfico de temperatura com 2 horas de registro - a cada ~55s
    break;
    case 3: 
      Periodo = 168750;            // Gráfico de temperatura com 6 horas de registro - a cada ~2m49s
    break;
    case 4:
      Periodo = 337500;            // Gráfico de temperatura com 12 horas de registro - a cada ~5m37s
    break;
    case 5:
      Periodo = 675000;            // Gráfico de temperatura com 24 horas de registro - a cada ~11m15s
    break;
  }
  if ((millis() - TimerTMP) > Periodo)
  {
    for (int x = 127; x > 0; x--) RegistroTMP[x] = RegistroTMP[x - 1];
    RegistroTMP[0] = TMPradio;
    TimerTMP = millis();
  }
  if (TMPradio >  TMPlimite)
  {
    digitalWrite (FANradio, FANligado);
    FAN = true;
  }
  else if (TMPradio < (TMPlimite - 1))
  {
    digitalWrite (FANradio, FANdesligado);
    FAN = false;
  }
  if (TMPradio > 49) RadioQUENTE = true;
  else               RadioQUENTE = false;
}

// DISPLAY ===========================================================================================================================================
void Display()
{
  RELOGIO();
  if (TMPctrl) TEMPERATURAradio();
  if (((millis() - TimerLCD) > TempoDisplay) && ((EstadoZELLO != "On-line") | (EstadoRADIO != "Stand by")))
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
      int ColunaLAz = map (              LAz, LIz, LSz, 31, 128);
      int ColunaRXz = map (             VRXz, LIz, LSz, 31, 128);
      
      // BARRA VU ZELLO -------------------------------------------------------------------------------
      if (VUZ)
      {
        display.fillRect ( 31, 0, LeituraZ,  7, SSD1306_WHITE);
        if ((LeituraZ + 31) < ColunaLAz) display.drawLine ( ColunaLAz, 0, ColunaLAz,  7, SSD1306_WHITE);
        else                             display.drawLine ( ColunaLAz, 0, ColunaLAz,  7, SSD1306_BLACK);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (0, 0);
        display.print  (F("ZELLO"));
        display.setCursor ((ColunaRXz - 5) , 0);
        if ((LeituraZ + 31) > (ColunaRXz - 2)) display.setTextColor(SSD1306_BLACK);
        else                                   display.setTextColor(SSD1306_WHITE);
        if (EstadoZELLO == "Transmitindo")     display.print  (F("T"));
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
      }

      // BARRA VU RADIO -------------------------------------------------------------------------------
      if (VUR)
      {
        int LeituraR  = map ( LeituraRADIOatual, 0, LSr,  0, 95);
        int ColunaLAr = map ( LAr,               0, LSr, 31, 128);
        display.fillRect ( 31, 9, LeituraR,  7, SSD1306_WHITE);
        if ((LeituraR + 31) < ColunaLAr) display.drawLine ( ColunaLAr, 9, ColunaLAr, 15, SSD1306_WHITE);
        else                             display.drawLine ( ColunaLAr, 9, ColunaLAr, 15, SSD1306_BLACK);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (0, 9); 
        display.print   (F("RADIO"));
      }
      
      
      // INFORMAÇÕES BASICAS ---------------------------------------------------------------------------
      if (TipoMOSTRADOR == 1)
      {
        VUZ = true;
        VUR = true;
        display.drawLine ( 0, 17, 128, 17, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor (0, 21);
        display.print  (F("RoboCOP:"));
        display.setCursor (104, 20);
        if (EstadoZELLO == "Off-line")     display.print (F("OF"));
        if (EstadoZELLO == "On-line")      display.print (F("ON"));
        if (EstadoZELLO == "Recebendo")    display.print (F("TX"));
        if (EstadoZELLO == "Transmitindo") display.print (F("RX"));
        if (EstadoZELLO == "Tela Inicial") display.print (F("TI"));
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
      // INFORMAÇÔES GERAIS ---------------------------------------------------------------------------
      if (TipoMOSTRADOR == 2)
      {
        VUZ = true;
        VUR = false;
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (0, 9);
        display.print  (F("LDRz: "));
        display.print  (LeituraZELLOatual);
        display.setCursor (67, 9);
        display.print  (F("LINK:  "));
        if (EstadoZELLO == "Tela Apagada") display.print (F("TA"));
        if (EstadoZELLO == "Off-line")     display.print (F("OF"));
        if (EstadoZELLO == "On-line")      display.print (F("ON"));
        if (EstadoZELLO == "Recebendo")    display.print (F("TX"));
        if (EstadoZELLO == "Transmitindo") display.print (F("RX"));
        if (EstadoZELLO == "Tela Inicial") display.print (F("TI"));

        display.setCursor (0, 16);
        display.print  (F("TI: "));
        display.println(LONTIz);
        display.print  (F("ON: "));
        display.print  (VONz);
        display.println(F("<"));
        display.print  (F("LA: "));
        display.println(LAz, 0);
        display.print  (F("RX: "));
        display.print  (VRXz);
        display.println(F("<"));
        display.print  (F("OF: "));
        display.println(LOFFRXz);
        display.print  (F("TA: "));
        display.println(LTAOFFz);

        display.setCursor (67, 18);
        display.print  (F("ZELLO: "));
        if (EstadoZELLO == "Tela Apagada") display.print (F("TA"));
        if (EstadoZELLO == "Off-line")     display.print (F("OF"));
        if (EstadoZELLO == "On-line")      display.print (F("ON"));
        if (EstadoZELLO == "Recebendo")    display.print (F("RX"));
        if (EstadoZELLO == "Transmitindo") display.print (F("TX"));
        if (EstadoZELLO == "Tela Inicial") display.print (F("TI"));

        display.setCursor (67, 28);
        display.print  (F("RADIO: "));
        if (EstadoRADIO == "Stand by")     display.print (F("SB"));
        if (EstadoRADIO == "Recebendo")    display.print (F("RX"));
        if (EstadoRADIO == "Transmitindo") display.print (F("TX"));
        if (EstadoRADIO == "FALHOU!!!!")   display.print (F("--"));
        if (EstadoRADIO == "Radio QUENTE!")display.print (F("!!"));

        if (TMPctrl)
        {
          display.setCursor (67, 46);
          display.print (F("TMP: "));
          display.print (TMPradio, 1);
          display.print (F("C"));

          display.setCursor (67, 56);
          display.print (F("FAN: "));
          if (FAN) display.print (F("Max"));
          else     display.print (F("Min"));
        }
        


      }
      // INFORMAÇÔES RÁDIO ---------------------------------------------------------------------------
      if (TipoMOSTRADOR == 3)
      {
        VUZ = true;
        VUR = true;
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (96, 16);
        display.print  (VTXr);
        display.setCursor (67, 16);
        display.print  (LAr);

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (0, 26);
        display.print  (F("Radio: "));
        display.print  (EstadoRADIO);

        display.setCursor (0, 36);
        display.print  (F("LDRr : "));
        display.print  (LeituraRADIOatual);
        display.setCursor (0, 46);
        display.print  (F("Zello: "));
        display.print  (EstadoZELLO);
        display.setCursor (0, 56);
        display.print  (F("LDRz : "));
        display.print  (LeituraZELLOatual);
      }
      // TEMPO LIGADO -------------------------------------------------------------------------------
      if (TipoMOSTRADOR == 4)
      {
        VUZ = true;
        VUR = true;
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
      // TEMPERATURA --------------------------------------------------------------------------------
      if ((TipoMOSTRADOR == 5) && (TMPctrl)) 
      {
        VUZ = false;
        VUR = false;
        byte marcadores = 0;
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor (0, 0); 
        display.print (F("COOLER: "));
        //display.print (LeituraNTC);
        if (FAN) display.print (F("Max"));
        else     display.print (F("Min"));
        display.setCursor (108, 0);
        switch (PeriodoTMP)
        {
          case 1: 
            display.print  (F("2m"));
            marcadores = 32;
          break; 
          case 2: 
            display.print  (F("2h"));
            marcadores = 16;
          break;
          case 3: 
            display.print  (F("6h"));
            marcadores = 21;
          break;
          case 4: 
            display.print  (F("12h"));
            marcadores = 11;
          break;
          case 5: 
            display.print  (F("24h"));
            marcadores = 5;
          break;
        }
        for (int col= 0; col< 127; col += marcadores)
        {
          int Tmax = TMPradioMAX;
          int Tmin = TMPradioMIN;
          for (int dot= Tmax; dot > Tmin; dot--)
          {
            int lin = map (dot, Tmax, Tmin, 63, 16);
            display.drawLine ( col, lin, col, lin, SSD1306_WHITE); 
          }
        }
        display.setCursor (0, 9);
        display.print (F("TEMP: "));
        display.print (TMPradio, 1);
        display.print (F("C  LMT: "));
        display.print (TMPlimite);
        display.print (F("C"));
        display.drawRect ( 0, 16, 128,  48, SSD1306_WHITE);
        float linha = 0;
        float linhaAnterior = 0;
        for (int x = 0; x < 127; x++)
        {
          linha = map ((RegistroTMP[x] * 10) , (TMPradioMIN * 10), (TMPradioMAX * 10), 53, 16);
          display.drawLine ( x, linha, x-1, linhaAnterior, SSD1306_WHITE); 
          linhaAnterior = linha;
        }
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (108, 19);
        display.print  (TMPradioMAX, 0);
        display.print  (F("C"));
        display.setCursor (108, 54);
        display.print  (TMPradioMIN, 0);
        display.print  (F("C"));
      }
      else if (TipoMOSTRADOR == 5) TipoMOSTRADOR = 6;
      // DISPLAY APAGADO ----------------------------------------------------------------------------
      if (TipoMOSTRADOR == 6)
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
  if (TipoMOSTRADOR > 6) TipoMOSTRADOR = 1;
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
    if (opcao == 5) display.println(F(">CTRL TEMPERATURA"));
    else            display.println(F(" Ctrl temperatura"));
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
                ControleTemperatura();  
                break;
              case 6:
                telaOK();
                Voltar = true; 
                break;
            }
          }
        }
      }
      opcao++;
      if (opcao > 6) opcao = 1;      
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
      digitalWrite (PTTradio, HIGH);
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
  digitalWrite (PTTradio, LOW);
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
  int VTXz = PTTZvermelho;
  VRXz = PTTZverde;
  
  display.println(PTTZlaranja);                
  display.println(PTTZverde);
  display.println(PTTZvermelho);
  display.display();
  display.clearDisplay();
   
  EEPROM.writeInt (evonZ, VONz);  // Gravando valores na EEPROM
  EEPROM.writeInt (evrxZ, VRXz);
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

void ControleTemperatura()
{
  telaOK();
  bool Voltar  = false;
  byte opcao = 1;
  while (!Voltar)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor (24, 0);
                    display.println(F("TEMPERATURA HT"));
                    display.println(F("---------------------")); 
    if (opcao == 1) display.print  (F(">HABILITAR CTRL  "));
    else            display.print  (F(" Habilitar ctrl  ")); 
    display.println                  (TMPctrl); 
    if (opcao == 2) display.print  (F(">TEMP LIMITE  "));
    else            display.print  (F(" Temp limite  "));
    display.print                    (TMPlimite);
    display.println                (F("C"));
    if (opcao == 3) display.print  (F(">GRAFICO: "));
    else            display.print  (F(" Grafico: "));
    if (PeriodoTMP == 1) display.println (F("2 minutos"));
    if (PeriodoTMP == 2) display.println (F("2 horas"));
    if (PeriodoTMP == 3) display.println (F("6 horas"));
    if (PeriodoTMP == 4) display.println (F("12 horas"));
    if (PeriodoTMP == 5) display.println (F("24 horas"));
    if (opcao == 4) display.println (F(">VOLTAR"));
    else            display.println (F(" Voltar"));
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
            if (opcao == 1) 
            {
              TMPctrl = !TMPctrl;
              EEPROM.writeBool(eTMPctrl, TMPctrl);
              EEPROM.commit();
              opcao--;
            }
            if (opcao == 2) 
            {
              TMPlimite++; 
              if ((TMPlimite > 50) | (TMPlimite < 30)) TMPlimite = 30;
              EEPROM.writeByte (eTMPlimite, TMPlimite);
              EEPROM.commit();
              opcao--;
            }
            if (opcao == 3)
            {
              PeriodoTMP++;
              if ((PeriodoTMP > 5) | (PeriodoTMP < 1)) PeriodoTMP = 1;
              EEPROM.writeChar(ePeriodoTMP, PeriodoTMP);
              EEPROM.commit();
              opcao--;
            }
            if (opcao == 4)
            {
              telaOK();
              Voltar = true;
              while (!digitalRead(BotaoBOOT)) {};
            }
          }
        }
      }
      opcao++;
      if (opcao == 5) opcao = 1;      
    }
    display.display();
  }
}

// CONFIGURAÇÕES MENSAGENS ---------------------------------------------------------------------------------------------------------------------------
void MenuSETUPmensagens()
{
  telaOK();
  bool Voltar  = false;
  byte opcao = 1;
  while (!Voltar)
  {
    if (MP3OK)
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
                  telaOK();
                  Voltar = true; 
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
    else
    {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor (0, 0);
      display.println(F("  !!! ATENCAO !!!"));
      display.println(F("---------------------")); 
      display.println(F("    DFPlayer esta"));
      display.println(F("    desabilitado!!"));
      display.display();
      delay (3000);
      Voltar = true;
    }
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
            if ((opcao > 1) && (opcao < 6)) Fuso = opcao;
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
