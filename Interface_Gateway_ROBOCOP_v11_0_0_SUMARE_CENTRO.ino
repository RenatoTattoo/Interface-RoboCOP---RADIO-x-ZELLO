/*                 Interface RoboCOP - RUGE x ZELLO - por 2RUGE0007 Renato Druziani 2023

 Este é o programa para a Interface RoboCOP, uma controladora para GATEWAY ZELLO x RÁDIO.
 A ideia era fazer algo de fácil instalação sem a necessidade de abrir o Rádio ou Celular para aquisição de algum sinal. 
 Usando um LDR na tela do celular e o sinal V no fone de ouvido do HT foi possível informar al programa o estado de cada
 um para o programa gerenciar a ativação dos PTTs permitindo a comunicação. A interface também monitora a temperatura do 
 rádio e aciona a ventilação para evitar o superaquecimento do mesmo durante longas transmissões.
 O gateway montado com esta interface além de seu funcionamento básico também permite que o gateway transmita:
 - MENSAGENS PERIÓDICAS:       Podem ser mensagens informativas que se queira.
 - INFORMAR A HORA CERTA:      Como no canal Talkabout Brasil a assistente do canal transmite a HORA CHEIA, o gateway 
                               transmite a HORA E MEIA.
 - INFORMAÇÕES DO ESTADO:      Quando há qualquer problema o gateway informa o estado quando se tenta a comunicação por 
                               ele, EX: caiu a internet e o Zello ficou off-line, se vc tenta falar com o seu rádio ao
                               soltar o PTT o gateway transmitira uma mensagem informando que o Zello está off-line. O
                               gateway tabém detecta e informa várias outras situações que podem impredir a comunicação.
 - INFORMAÇÕES DO CLIMA LOCAL: Se configurado corretamente o gateway transmitirá após a hora cheia as informações de
                               temperatura, umidade, velocidade e direção do vento.
 O Esquema elétrico e outros arquivos necessários para a confecção do gateway com a interface RoboCOP estão disponíveis
 no GitHUB:  https://github.com/RenatoTattoo/Interface-RoboCOP---RADIO-x-ZELLO

*/
 
// VERSÃO do Software --------------------------------------------------------------------------------------------------------------------------------
 #define Versao          "11.0.0"
 /*
  Histórico das modificações nas versões:
  - 10.2.1 - Adicionada a leitura do TX no LDR do Zello para fazer o Auto Ajuste durante o TX, essa é uma tentativa para resolver
             que aparece quando o link recebe longas mensagens através do HT. E deu certo!!!
  - 10.2.2 - Modificada a leitura do LDR do Radio, para um comportamento mais semelhande ao uma leitura DIGITAL que ANALOGICA
  - 10.2.3 - Modificado o registro de Temperatura MAX e MIN para se mostrar a MAX e a MIN dentro do PERIODO de Registro.
             Modificados os mecanismos de acionamento dos PTTs rádio e zello
             Modificado a configuração do Volume das mensagens, removido a locução em cada nível de volume.
             Modificado o sistema de anti travamento para não reiniciar durante longos TX ou RX.
             Modificado posição do ponteiro dos menus de configurações no retorno.   
  - 10.3.0 - Removido o Cão de Guarda.  
  - 11.0.0 - Adicionado o Boletim Meteorologico.
             Alteração das opções de layouts do display.
 */

// Bibliotecas ---------------------------------------------------------------------------------------------------------------------------------------
 #include <WiFiManager.h>         // por Tzapu     - Ver 2.0.17
 #include <WiFi.h>                // por Arduino   - Ver 1.2.7
 #include <ArduinoJson.h>         // por Benoit Bl - Ver 7.1.0
 #include <NTPClient.h>           // por Fabrice W - Ver 3.2.1
 #include <Adafruit_GFX.h>        // por Adafruit  - Ver 1.11.9
 #include <Adafruit_SSD1306.h>    // por Adafruit  - Ver 2.5.10
 #include <SoftwareSerial.h>      // por Dirk Kaar - Ver 8.1.0 (pesquise por EspSoftwareSerial)
 #include <DFRobotDFPlayerMini.h> // por DFRobot   - Ver 1.0.6
 #include <EEPROM.h> 
 //#include <Wire.h>  
           
// relação MENSAGEM  com  o  NÚMERO DO ARQUIVO no cartão SD-------------------------------------------------------------------------------------------
 #define RogerBEEPlocal   25      // Arquivo 01~24 correspondem aos áudios de HORA e MEIA: 01 = 00:30h ~ 24 = 23:30h
 #define RogerBEEPweb     26      // Arquivo 25~31 correspondem aos áudios ROGER e OPERACIONAIS
 #define RogerBEEPerro    27               
 #define TelaAPAGOU       28
 #define LinkOFFLINE      29
 #define LinkONLINE       30
 #define ZelloFECHOU      31
 #define TEMP             38      // Arquivo 33~88    correspondem aos áudios de Temperatura -5°C a 50°C
 #define UMID             84      // Arquivo 89~184   correspondem aos áudios de Umidade de   5% a 100%
 #define VVento          185      // Arquivo 185~197  correspondem aos áudios da Velocidade do vento
 #define DVento          198      // Arquivo 198~205  correspondem aos áudios da Direção do vento
 #define PrimeiraVinheta 206      // Arquivo 206 ou + correspondem aos áudios de VINHETAS diversas

// definições PORTAS----------------------------------------------------------------------------------------------------------------------------------
 #define BotaoBOOT        0
 #define LED              2
 #define TocandoMP3       4
 #define PinPTTzello      18
 #define FANradio         19
 #define PinPTTradio      23
 #define SinalRadio       34
 #define SinalLDRzello    35
 #define NTCradio         39

// Definições dos endereços dos valores salvos na EEPROM----------------------------------------------------------------------------------------------
 #define EEPROM_SIZE      64
 #define eVolumeVOZ       10
 #define evonZ            20
 #define edifrxonZ        30
 #define ediftxonZ        45
 #define eTMPlimite       15
 #define eTMPctrl         25
 #define ePeriodoTMP      35
 #define eHoraCerta       0
 #define eMensagens       1
 #define eStatus          2
 #define eRogerBEEPs      3
 #define eFuso            4
 #define eClima           5

// Outras Definições ---------------------------------------------------------------------------------------------------------------------------------
 #define TempoDoPulso    100             // Define o tempo de acionamento para o PTT do Zello - Limite mínimo no Galaxy Pocket2 foi 75ms
 #define TempoDisplay    300000          // Define o tempo para que o LDC fique apagado depois da inatividade
 #define UmaHora         3600000         // Define de quanto em quanto tempo deve ser salvo os valores de Auto Ajuste
 #define DezMinutos      600000          // Define um tempo de 10 minutos
 #define UmMinuto        60000           // Define um tempo de 1  minuto
 #define DezSegundos     10000           // Define um tempo de 10 segundos
 #define QuinzeSegundos  15000           // Define um tempo de 15 segundos
 #define CincoMinutos    300000          // Define um tempo de  5 minutos
 #define CincoSegundos   5000            // Define um tempo de  5 segundos
 #define TaxaAtualizacao 75              // Define de quantos em quantos milisegundos o Display é atualizado
 #define NAZ             350             // Número de amostragens para a média móvel da leitura do LDR do ZELLO
 #define NAAz            75              // Número de amostragens para a média móvel das correções de auto ajuste Zello
 #define NAT             10000           // Número de amostragens para a média móvel da leitura do NTC do RADIO
 #define FANligado       0              
 #define FANdesligado    1
 #define ligado          1              
 #define desligado       0

WiFiUDP ntpUDP;
WiFiClient client;
NTPClient ntp(ntpUDP);
Adafruit_SSD1306 display(128, 64, &Wire);

WiFiManager wm;
SoftwareSerial ComunicacaoMP3(16, 17);        // Pinos 16 RX, 17 TX para comunicação serial com o DFPlayer
DFRobotDFPlayerMini LOCUTORA;                 // nomeando DFPlayer como "LOCUTORA"
 
// Variáveis, arrays, funções, etc--------------------------------------------------------------------------------------------------------------------
 int           TempLOCAL         = 0;
 byte          UmidLOCAL         = 0;
 float         PresLOCAL         = 0;
 float         VelVENTO          = 0;
 int           DirVENTO          = 0;
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
 bool          FalarClima        = false;
 bool          FalouBOLETIM      = false;
 bool          ColetouBOLETIM    = false;
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
 byte          opcao             = 1;
 byte          Volume            = 0;
 byte          Vinheta           = PrimeiraVinheta;
 byte          Narquivos         = 0;
 char          PeriodoTMP        = 0;
 byte          TipoMOSTRADOR     = 1;
 int           LeituraZELLOatual = 0;
 int           LSz               = 0;
 int           LONTIz            = 0;
 int           VONz              = 0;
 int           VRXz              = 0;
 int           VTXz              = 0;
 int           DifRXONz          = 0;
 int           DifTXONz          = 0;
 int           LOFFz             = 0;
 float         LAz               = 0;
 int           LIz               = 0;
 int           LTAOFFz           = 0;
 String        EstadoZELLO       = "XX";
 String        EstadoRADIO       = "XX";
 String        EstadoLINK        = "XX";
 String        result;
 bool          PTTradioON        = false;
 bool          PTTzelloON        = false;
 byte          Fuso              = 0;      // Fernando de Noronha(GMT-2), Brasília(GMT-3), Manaus - AM(GMT-4), Rio Branco - AC(GMT-5)
 unsigned long Relogio           = millis();
 unsigned long TimerLCD          = millis();
 unsigned long TimerFPS          = millis();
 unsigned long TimerTMP          = millis();
 unsigned long TimerLOOP         = millis();
 unsigned long TimerPisca        = millis();
 unsigned long TimerAtividade    = millis();
 unsigned long TimerESTADOzello  = millis();
 unsigned long TimerESTADOradio  = millis();
 unsigned long TimerAutoAjuste   = millis();
 unsigned long PausaAutoAjuste   = millis();

 int           MediaLDRzello[NAZ];
 int           MediaONz[NAAz];

 int           MediaNTC[NAT];               // variáveis para os cálculo de temperatura
 float         RegistroTMP[128];
 float         Vout              = 0; 
 float         Rt                = 0;
 float         TF                = 0;
 float         TMPradio          = 0;
 int           LeituraNTC        = 0;
 float         TMPradioMAX       = 0;
 float         TMPradioMIN       = 100;
 byte          TMPlimite         = 0;
 const float   Vs                = 3.3;           // Tensão 3.3V de saída no ESP32 
 const int     R1                = 10000;         // Resistor 10k usado no PULLUP
 const int     Beta              = 3380;          // Valor de BETA, cada tipo de termistor tem seu BETA, vide datasheet
 const float   To                = 298.15;        // Valor em °K referênte a 25°C
 const int     Ro                = 10000;         // Referente a resistência do termistor à 25°C que é 10k
 const float   LeituraMax        = 4095.0;        // Leitura máxima de resolução, a resolução do ADC noESP32 é 4095.0
 const String  CityID            = "3447259";                          // id da cidade no OpenWeather 
 const String  APIKEY            = "c6bcf019678bdecfa0a4ade221ba80e9"; // API Código de acesso a informações de clima OpenWeather
 char          servername[]      = "api.openweathermap.org";           // Nome do servidor de dados do clima
 const char    CIDADE[]          = "SUMARE";       // Coloque o NOME ou ABREVIAÇÃO de sua cidade com no MAXIMO 6 LETRAS

// SETUP =============================================================================================================================================
void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  ComunicacaoMP3.begin(9600);
  
  wm.setDebugOutput(false);

  // DEFININDO PINOS --------------------------------------------------------------------------------------------------------------------
  pinMode(TocandoMP3,    INPUT);
  pinMode(SinalLDRzello, INPUT_PULLDOWN);
  pinMode(SinalRadio,    INPUT_PULLDOWN);
  pinMode(NTCradio,      INPUT_PULLUP);
  pinMode(BotaoBOOT,     INPUT_PULLUP);
  pinMode(LED,           OUTPUT);
  pinMode(PinPTTzello,   OUTPUT);
  pinMode(PinPTTradio,   OUTPUT);
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
  Serial.println(F("------------------------- INICIALIZAÇÃO DO SISTEMA -------------------------"));  
  Serial.print  (F("Iniciando DISPLAY"));
  while(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    delay (50);
    digitalWrite (LED, 1);
    Serial.print (F("-"));
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
  Serial.println(F("Versao "));
  display.setCursor (90, 57);
  display.print(Versao);
  Serial.println(F(Versao));
  display.display();
  display.clearDisplay();
  delay(2000);
  display.setCursor (0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(F("LCD...  OK!"));
  Serial.println (F("- OK!"));
  display.display();
  //Serial.end();
  digitalWrite (LED, 0);
  MP3OK = LOCUTORA.begin(ComunicacaoMP3);
  display.print (F("MP3...  "));
  Serial.print (F("Iniciando MP3"));
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
      display.print (tentativas);
      Serial.print  (F(" "));
      Serial.print  (tentativas);
      display.display();
      MP3OK = LOCUTORA.begin(ComunicacaoMP3);
      delay (100);
    }
    display.clearDisplay();
    display.setCursor (0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.println (F("LCD...  OK!"));
    if (!MP3OK) 
    {
      display.println(F("MP3...  FALHOU!"));
      Serial.println(F("MP3...  FALHOU!"));
    }
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
    Serial.println  (F(" MP3...  OK!!!"));
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
    Serial.print  (F("Volume VOZ: "));
    Serial.println(Volume);
    Fuso   = EEPROM.readByte(eFuso);
    Serial.print  (F("FUSO Horário: "));
    Serial.println(Fuso);
    LOCUTORA.setTimeOut(500);                    // ajustando o timeout da comunicação serial 500ms
    LOCUTORA.volume(Volume);                     // Ajustando o volume (0~30).
    FalarHORA      = EEPROM.readBool(eHoraCerta);
    Serial.print  (F("Falar HORA: "));
    Serial.println(FalarHORA);
    FalarMensagens = EEPROM.readBool(eMensagens);
    Serial.print  (F("Falar MENSAGENS: "));
    Serial.println(FalarMensagens);
    FalarClima     = EEPROM.readBool(eClima);
    Serial.print  (F("Falar CLIMA: "));
    Serial.println(FalarClima);
    FalarStatus    = EEPROM.readBool(eStatus);  
    Serial.print  (F("Falar STATUS: "));
    Serial.println(FalarStatus);
    RogerBeeps     = EEPROM.readBool(eRogerBEEPs);
    Serial.print  (F("RogerBEEPS: "));
    Serial.println(RogerBeeps);
  }
  else
  {
    FalarHORA      = false;
    FalarMensagens = false;
    FalarClima     = false;
    FalarStatus    = false;  
    RogerBeeps     = false;
  }
  
  delay(500);
  display.print  (F("WiFi... "));
  Serial.println(F("Iniciando WiFi "));
  display.display();
  wm.setConfigPortalTimeout(1);
  WiFiOK = wm.autoConnect();
  delay(500);
  if (WiFiOK)
  {
    display.println(F("OK!!!"));
    Serial.println (F("OK!!!"));
    display.display();
    delay(500);
    display.println(F("Sincronizando relogio"));
    Serial.println(F("Sincronizando relogio"));
    display.display();
    SincronizarNTP();
    delay(500);
    if (FalarClima)
    {
      display.println(F("Verificando clima"));
      Serial.println(F("Verificando clima"));
      display.display();
      ClimaOpenWeather();
    }
    
    if (HC) 
    {
      Serial.println(F("Sincronizacao OK!!!"));
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
      Serial.println (F("Sincronizacao FALHOU!"));
      display.display();
      delay(500);
    }
  }
  else
  {
    display.println();
    display.println(F("FALHOU!"));
    Serial.println (F("FALHOU!"));
  }
  display.display();
  display.clearDisplay();
  delay(1000);
  
  // RECUPERAR DADOS
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  Serial.println (F("CARREGANDO  DADOS"));
  display.println(F("  CARREGANDO  DADOS"));
  display.println(F("---------------------"));
  VONz     = EEPROM.readInt(evonZ);  // Lendo na EEPROM os dados gravados
  DifRXONz = EEPROM.readInt(edifrxonZ);
  DifTXONz = EEPROM.readInt(ediftxonZ);
  for (int x = 0; x < NAZ;  x++) MediaLDRzello[x] = VONz;
  for (int x = 0; x < NAAz; x++) MediaONz[x]      = VONz;
  VRXz = VONz - DifRXONz;
  VTXz = VONz - DifTXONz;
  if (VRXz > VTXz)
  {
    LOFFz = (VTXz / 2);
    LAz   = VONz - (DifRXONz / 2);
  }
  if (VTXz > VRXz)
  {
    LOFFz = (VRXz / 2);
    LAz   = VONz - (DifTXONz / 2);
  }
  
  TMPctrl    = EEPROM.readBool(eTMPctrl);
  Serial.print  (F("Controle de Temperatura: "));
  Serial.println(TMPctrl);
  TMPlimite  = EEPROM.readByte(eTMPlimite);
  Serial.print  (F("Tmperatura LIMITE: "));
  Serial.println(TMPlimite);
  PeriodoTMP = EEPROM.readChar(ePeriodoTMP);
  Serial.print  (F("Periodo TEMP: "));
  Serial.println(PeriodoTMP);
  if (TMPctrl)
  {
    for (int x = 0; x < NAT; x++) MediaNTC[x] = analogRead(NTCradio);
    long contador = 0;
    for (int x = 0; x < NAT; x++) contador += MediaNTC[x];
    LeituraNTC = (contador / NAT);

    Vout = LeituraNTC * 3.3 / LeituraMax;
    Rt = 10000 * Vout / (3.3 - Vout);
    TF = 1 / ((1 / 298.15) + (log(Rt / 10000) / Beta));
    TMPradio = TF - 273.15;

    for (int x = 0; x < 128; x++) RegistroTMP[x] = TMPradio;
    TMPradioMAX = TMPradio;
    TMPradioMIN = TMPradio;
  }
  
  for(int x=0; x<NAAz; x++) MediaONz[x] = VONz;
  display.print  (F(" VONz : "));
  display.println(VONz); 
  display.print  (F(" VRXz : "));
  display.println(VRXz);
  display.print  (F(" VTXz : "));
  display.println(VTXz);
  display.display();
  display.clearDisplay();
  Serial.print  (F(" VONz : "));
  Serial.println(VONz); 
  Serial.print  (F(" VRXz : "));
  Serial.println(VRXz);
  Serial.print  (F(" VTXz : "));
  Serial.println(VTXz);
  Serial.println(F("------------------------- INICIALIZAÇÃO COMPLETADA -------------------------"));
  digitalWrite (FANradio, FANdesligado);
  delay(1000);
}

// LOOP ==============================================================================================================================================
void loop()
{
  ESTADOzello();
  ESTADOradio();
  Pisca();
  if ((millis() - TimerLOOP) > 10)
  {
    Display();

    // LINK TRANSMITINDO sinal -----------------------------------------------------------------------------------------------------------------------------
    if (( EstadoZELLO== "Recebendo") && (EstadoRADIO == "Stand by"))
    {
      Serial.println(F("LINK TRANSMITINDO SINAL"));
      PTTradio(ligado);
      while (EstadoZELLO == "Recebendo")
      {
        ESTADOzello();
        ESTADOradio();
        Display();
      }
      if (RogerBeeps) TOCAR(RogerBEEPweb);
      PTTradio(desligado);
    }
    
    // LINK RECEBENDO sinal --------------------------------------------------------------------------------------------------------------------------------
    if ((EstadoRADIO == "Recebendo") && (EstadoZELLO == "On-line"))
    {
      Serial.println(F("LINK RECEBENDO SINAL"));
      PTTzello(ligado);
      bool QSOok = false;
      while ((EstadoRADIO == "Recebendo") && (EstadoZELLO == "Transmitindo"))
      {
        QSOok = true;
        ESTADOzello();
        ESTADOradio();
        Display();
      }
      if (RogerBeeps)
      {
        PTTradio(ligado);
        if (QSOok) TOCAR(RogerBEEPlocal);
        else       TOCAR(RogerBEEPerro);
        PTTradio(desligado);
      }
      PTTzello(desligado);
      QSOok = false;
    }

    // STATUS DO LINK ---------------------------------------------------------------------
    if (FalarStatus)
    { 
      // LINK ON-LINE ---------------------------------------------------------------------
      if ((EstadoZELLO == "On-line") && (MensagemON))
      {
        Serial.println(F("LINK ON-LINE"));
        PTTradio(ligado);
        TOCAR(LinkONLINE);
        TOCAR(RogerBEEPlocal);
        PTTradio(desligado);
        MensagemON = false;
      }

      // LINK OFF-LINE ---------------------------------------------------------------------
      if (EstadoZELLO == "Off-line") 
      {
        while (EstadoRADIO == "Recebendo")
        {
          LeituraZELLOatual = mmLDRzello();
          ESTADOradio();
          Display();
          MensagemOFF = true;
        }
        if (MensagemOFF)
        {
          Serial.println(F("LINK OFF-LINE"));
          PTTradio(ligado);
          TOCAR(LinkOFFLINE);
          TOCAR(RogerBEEPerro);
          PTTradio(desligado);
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
          ESTADOradio();
          Display();
          MensagemTA = true;
        }
        if (MensagemTA)
        {
          Serial.println(F("A TELA APAGOU"));
          PTTradio(ligado);
          TOCAR(TelaAPAGOU);
          TOCAR(RogerBEEPerro);
          PTTradio(desligado);
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
          ESTADOradio();
          Display();
          MensagemTI = true;
        }
        if (MensagemTI)
        {
          Serial.println(F("ZELLO FECHOU"));
          PTTradio(ligado);
          TOCAR(ZelloFECHOU);
          TOCAR(RogerBEEPerro);
          PTTradio(desligado);
          MensagemTI = false;
          MensagemON = true;
        }
      }
      else MensagemTI = true;
    }

    bool SituacaoOK = false;
    if ((EstadoZELLO == "On-line") && (EstadoRADIO == "Stand by")) SituacaoOK = true;
    else SituacaoOK = false;
    // LINK TX HORA E MEIA --------------------------------------------------------------------
    if (FalarHORA) 
    {
      if ((HC) && (M == 30) && (!FalouHORA) && (SituacaoOK))
      {
        Serial.println(F("INFORMANDO HORA E MEIA"));
        PTTradio(ligado);
        delay(250);
        TOCAR(H + 1);
        TOCAR(RogerBEEPlocal);
        PTTradio(desligado);
        FalouHORA = true;
      }
      if (M > 55) FalouHORA = false;
    }

    // LINK TX MENSAGENS 10 e 50 minutos ------------------------------------------------------
    if (FalarMensagens) 
    {
      if (((M == 10) | (M == 50)) && (S == 0)) FalouMensagem = false;
      if ((!FalouMensagem) && (SituacaoOK) && ((millis() - TimerAtividade) > UmMinuto))
      {
        Serial.println(F("TOCANDO MENSAGEM PERIODICA"));
        PTTradio(ligado);
        TOCAR(Vinheta);
        PTTradio(desligado);
        Vinheta++;
        if (Vinheta > Narquivos) Vinheta = PrimeiraVinheta;
        FalouMensagem = true;
      }
    }
    
    // LINK TX BOLETIM METEOROLOGICO ---------------------------------------------------------
    if (FalarClima)
    {
      if (((M > 00) && (M < 10)) && (ColetouBOLETIM) && (!FalouBOLETIM) && (SituacaoOK) && ((millis() - TimerAtividade) > UmMinuto)) 
      {
        Serial.println();
        Serial.println(F("Transmitindo BOLETIM do CLIMA"));
        PTTradio(ligado);
        delay(250);
        TOCAR(RogerBEEPweb);
        TOCAR(32);
        Serial.println(F("TEMP"));
        TOCAR(TEMP + TempLOCAL);
        Serial.println(F("UMIDADE"));
        TOCAR(UMID + (UmidLOCAL));
        Serial.println(F("VEL VENTO"));
        if      ((VelVENTO >=    0) && (VelVENTO <  0.2)) TOCAR(VVento +  0);
        else if ((VelVENTO >=  0.3) && (VelVENTO <  1.5)) TOCAR(VVento +  1);
        else if ((VelVENTO >=  1.6) && (VelVENTO <  3.3)) TOCAR(VVento +  2);
        else if ((VelVENTO >=  3.4) && (VelVENTO <  5.4)) TOCAR(VVento +  3);
        else if ((VelVENTO >=  5.5) && (VelVENTO <  7.9)) TOCAR(VVento +  4);
        else if ((VelVENTO >=  8.0) && (VelVENTO < 10.7)) TOCAR(VVento +  5);
        else if ((VelVENTO >= 10.8) && (VelVENTO < 13.8)) TOCAR(VVento +  6);
        else if ((VelVENTO >= 13.9) && (VelVENTO < 17.1)) TOCAR(VVento +  7);
        else if ((VelVENTO >= 17.2) && (VelVENTO < 20.7)) TOCAR(VVento +  8);
        else if ((VelVENTO >= 20.8) && (VelVENTO < 24.4)) TOCAR(VVento +  9);
        else if ((VelVENTO >= 24.5) && (VelVENTO < 28.4)) TOCAR(VVento + 10);
        else if ((VelVENTO >= 28.5) && (VelVENTO < 32.6)) TOCAR(VVento + 11);
        else if  (VelVENTO >= 32.7)                       TOCAR(VVento + 12);
        
        if (VelVENTO > 0.2)
        {
          Serial.println(F("DIREÇÃO"));
          if      ((DirVENTO >= 337.5) || (DirVENTO <  22.4)) TOCAR(DVento + 0);
          else if ((DirVENTO >=  22.5) && (DirVENTO <  67.4)) TOCAR(DVento + 1);
          else if ((DirVENTO >=  67.5) && (DirVENTO < 112.4)) TOCAR(DVento + 2);
          else if ((DirVENTO >= 112.5) && (DirVENTO < 157.4)) TOCAR(DVento + 3);
          else if ((DirVENTO >= 157.5) && (DirVENTO < 202.4)) TOCAR(DVento + 4);
          else if ((DirVENTO >= 202.5) && (DirVENTO < 247.4)) TOCAR(DVento + 5);
          else if ((DirVENTO >= 247.5) && (DirVENTO < 292.4)) TOCAR(DVento + 6);
          else if ((DirVENTO >= 292.5) && (DirVENTO < 337.4)) TOCAR(DVento + 7);
        }
        Serial.println(F("ROGER BEEP"));
        TOCAR(RogerBEEPlocal);
        FalouBOLETIM   = true;
        Serial.println(F("FalouBOLETIM = true"));
        Serial.println(F("FIM"));
        Serial.println();
        PTTradio(desligado);
      }
      if  ((((M == 58) && (S == 0)) || ((M == 13) && (S == 0)) || ((M == 28) && (S == 0)) || ((M == 43) && (S == 0))) && (ColetouBOLETIM))
      {
        Serial.println(F("ColetouBOLETIM = false"));
        ColetouBOLETIM = false;
      }
      if ((((M == 59) && (S == 0)) || ((M == 14) && (S == 0)) || ((M == 29) && (S == 0)) || ((M == 44) && (S == 0))) && (!ColetouBOLETIM))
      {
        Serial.println(F("ATUALIZANDO DADOS CLIMA...."));
        ClimaOpenWeather();
        Serial.println(F("FalouBOLETIM = false"));
        FalouBOLETIM = false;
      }  
    }
    
    TimerLOOP = millis();
  }
}

// MP3 e ACIONAMENTOS PTTs ================================================================================================================

void TOCAR(byte Arquivo)
{
  Serial.print  (F("Tocando arquivo: "));
  Serial.print  (Arquivo);
  Serial.print  (F(" de "));
  Serial.println(Narquivos);
  LOCUTORA.play (Arquivo);
  while (digitalRead(TocandoMP3))
  {
    LeituraZELLOatual = mmLDRzello();
    ESTADOradio();
    Display();
  }
  while (!digitalRead(TocandoMP3))
  {
    LeituraZELLOatual = mmLDRzello();
    ESTADOradio();
    Display();
  }
  delay (50);
}

void PTTzello(bool acaoZ)
{
  if (acaoZ)                             // LIGAR O PTT do zello
  {
    if (EstadoZELLO == "On-line")
    {
      Serial.println(F("Acionando PTT do ZELLO"));
      digitalWrite (PinPTTzello, HIGH);
      delay (TempoDoPulso);
      digitalWrite (PinPTTzello, LOW);
      PTTzelloON = true;
      while (EstadoZELLO == "On-line")
      {
        ESTADOzello();
        ESTADOradio();
        Display();
      }
    }
  }
  else                                   // DESLIGAR O PTT do zello
  {
    LeituraZELLOatual = mmLDRzello();
    if (EstadoZELLO == "Transmitindo")
    {
      Serial.println(F("Desligando PTT do ZELLO"));
      digitalWrite (PinPTTzello, HIGH);
      delay (TempoDoPulso);
      digitalWrite (PinPTTzello, LOW);
      PTTzelloON = false;
      while (EstadoZELLO != "On-line")
      {
        ESTADOzello();
        Display();
      }
      TimerAtividade = millis();
    }
  }
}

void PTTradio(bool acaoR)
{
  if (acaoR)                             // LIGAR O PTT do radio 
  {
    if (!RadioQUENTE)
    {
      Serial.println(F("Acionando PTT do RADIO"));
      digitalWrite (PinPTTradio, HIGH);
      PTTradioON = true;
    }
    else EstadoRADIO = "Radio QUENTE!";
  }
  else                                   // DESLIGAR O PTT do radio
  {
    Serial.println(F("Desligando PTT do RADIO"));
    digitalWrite (PinPTTradio, LOW);
    while (EstadoRADIO != "Stand by")
    {
      ESTADOradio();
      Display();
      TimerAtividade = millis();
    }
    PTTradioON = false;
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
  if (EstadoZELLO == "On-line")      MediaONz[0] = LeituraZELLOatual;
  if (EstadoZELLO == "Recebendo")    MediaONz[0] = LeituraZELLOatual + DifRXONz;
  if (EstadoZELLO == "Transmitindo") MediaONz[0] = LeituraZELLOatual + DifTXONz;
  long contador = 0;
  for (int x=0; x<NAAz; x++) contador += MediaONz[x];
  return (contador/NAAz);
}

// RELOGIO, CÁLCULOS DOS LIMITES e STATUS do HT e CLIENTE ZELLO ======================================================================================
void RELOGIO()
{
  if ((!HC) && (D>10) && (M==2) && (S==0) && (WiFiOK)) HoraCerta(); // verificando QUANDO ajustar o relogio.
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
  LeituraZELLOatual = mmLDRzello();
  AutoAjuste();
  VRXz = VONz - DifRXONz;
  VTXz = VONz - DifTXONz;

  if (VRXz > VTXz)
  {
    LONTIz =  VONz + (DifRXONz * 1.5);  // LONTIz - estabelecendo o limite entre ONLINE e TELA INICIAL.
    LSz    =  VONz +  DifRXONz;         // estabelecendo o limite superior para usar na tela.
    LIz    =  VONz -  DifTXONz;         // estabelecendo o limite inferior para usar na tela.
    LOFFz  = (VTXz / 2);                // Estabelecendo o limite útil entre TX e OFF LINE.
    if ((EstadoZELLO == "On-line")      && (LAz < (LeituraZELLOatual - ((DifRXONz / 100) * 66)))) LAz = LAz + 0.10;  // subindo  o limite quando em ON.
    if ((EstadoZELLO == "On-line")      && (LAz > (LeituraZELLOatual - ((DifRXONz / 100) * 66)))) LAz = LAz - 0.05;  // descendo o limite quando em ON.
    if ((EstadoZELLO == "Recebendo")    && (LAz > (LeituraZELLOatual + ((DifRXONz / 100) * 40)))) LAz = LAz - 0.10;  // descendo o limite quando em RX.
    if ((EstadoZELLO == "Recebendo")    && (LAz < (LeituraZELLOatual + ((DifRXONz / 100) * 40)))) LAz = LAz + 0.05;  // Subindo  o limite quando em RX.
    if ((EstadoZELLO == "Transmitindo") && (LAz > (LeituraZELLOatual + ((DifRXONz / 100) * 40)))) LAz = LAz - 0.10;  // descendo o limite quando em TX.
    if ((EstadoZELLO == "Transmitindo") && (LAz < (LeituraZELLOatual + ((DifRXONz / 100) * 40)))) LAz = LAz + 0.05;  // Subindo  o limite quando em TX.
  }
  if (VTXz > VRXz)
  {
    LONTIz =  VONz + (DifTXONz * 1.5);  // LONTIz - estabelecendo o limite entre ONLINE e TELA INICIAL.
    LSz    =  VONz +  DifTXONz;         // estabelecendo o limite superior para usar na tela.
    LIz    =  VONz -  DifRXONz;         // estabelecendo o limite inferior para usar na tela.
    LOFFz  = (VRXz / 2);                // Estabelecendo o limite útil entre RX e OFF LINE.
    if ((EstadoZELLO == "On-line")      && (LAz < (LeituraZELLOatual - ((DifTXONz / 100) * 66)))) LAz = LAz + 0.10;  // subindo  o limite quando em ON.
    if ((EstadoZELLO == "On-line")      && (LAz > (LeituraZELLOatual - ((DifTXONz / 100) * 66)))) LAz = LAz - 0.05;  // descendo o limite quando em ON.
    if ((EstadoZELLO == "Recebendo")    && (LAz > (LeituraZELLOatual + ((DifTXONz / 100) * 40)))) LAz = LAz - 0.10;  // descendo o limite quando em RX.
    if ((EstadoZELLO == "Recebendo")    && (LAz < (LeituraZELLOatual + ((DifTXONz / 100) * 40)))) LAz = LAz + 0.05;  // Subindo  o limite quando em RX.
    if ((EstadoZELLO == "Transmitindo") && (LAz > (LeituraZELLOatual + ((DifTXONz / 100) * 40)))) LAz = LAz - 0.10;  // descendo o limite quando em TX.
    if ((EstadoZELLO == "Transmitindo") && (LAz < (LeituraZELLOatual + ((DifTXONz / 100) * 40)))) LAz = LAz + 0.05;  // Subindo  o limite quando em TX.
  }
  LTAOFFz = (VONz / 100) * 2;        // Estabelecendo o limite útil entre OFF e Tela Apagada.
}

void ESTADOzello()
{
  ValoresLimites();
  String EZA = EstadoZELLO;
  if ((millis() - TimerESTADOzello) > 750)     //Aguardar a leitura se estabilizar por este tempo
  {
    if  (LeituraZELLOatual > LONTIz)                                                     EstadoZELLO = "Tela Inicial";
    if ((LeituraZELLOatual > LAz)     && (LeituraZELLOatual < LONTIz))                   EstadoZELLO = "On-line";
    if ((LeituraZELLOatual > LOFFz)   && (LeituraZELLOatual < LAz)     && (!PTTzelloON)) EstadoZELLO = "Recebendo";
    if ((LeituraZELLOatual > LOFFz)   && (LeituraZELLOatual < LAz)     && ( PTTzelloON)) EstadoZELLO = "Transmitindo";
    if ((LeituraZELLOatual > LTAOFFz) && (LeituraZELLOatual < LOFFz))                    EstadoZELLO = "Off-line";
    if  (LeituraZELLOatual < LTAOFFz)                                                    EstadoZELLO = "Tela Apagada";
  } 
  if  (EZA != EstadoZELLO)
  {
    TimerESTADOzello = millis();
    PausaAutoAjuste  = millis();
  }
}

void ESTADOradio()
{
  String ERA = EstadoRADIO;
  if ((millis() - TimerESTADOradio) > 750)     //Aguardar a leitura se estabilizar por este tempo
  {
    if ((analogRead(SinalRadio) > 10) && (!RadioQUENTE))
    {
      if (PTTradioON) EstadoRADIO = "Transmitindo";
      else            EstadoRADIO = "Recebendo";
    } 
    else
    {
      if (RadioQUENTE) EstadoRADIO = "Radio QUENTE!";
      else             EstadoRADIO = "Stand by";
    } 
  }
  if (ERA != EstadoRADIO)
  {
    TimerESTADOradio = millis();
    PausaAutoAjuste  = millis();
  }
}

void AutoAjuste()
{
  if ((EstadoZELLO == "On-line") | (EstadoZELLO == "Recebendo") | (EstadoZELLO == "Transmitindo"))
  {
    if ((millis() - PausaAutoAjuste) > DezSegundos)
    {  
      VONz = mmONz();   
      PausaAutoAjuste = millis() - 9000; 
    }
  }

  if ((millis() - TimerAutoAjuste) > UmaHora)     // Hora em hora verifica e salva na EEPROM os dados ajustados

  {
    int VONgravado = (EEPROM.readInt (evonZ));
    int Diferenca  = VONz-VONgravado;
    if (abs(Diferenca) > 5)
    {
      EEPROM.writeInt (evonZ, VONz);
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
    TMPradioMAX = 0;
    TMPradioMIN = 100;
    for (int x = 127; x > 0; x--)RegistroTMP[x] = RegistroTMP[x - 1];
    RegistroTMP[0] = TMPradio;
    for (int x = 127; x > 0; x--)
    {
      if (RegistroTMP[x] > TMPradioMAX) TMPradioMAX = RegistroTMP[x];
      if (RegistroTMP[x] < TMPradioMIN) TMPradioMIN = RegistroTMP[x];
    }
    TimerTMP = millis();
  }
  if  (TMPradio > TMPradioMAX) TMPradioMAX = TMPradio;
  if  (TMPradio < TMPradioMIN) TMPradioMIN = TMPradio;
  if ((TMPradio > TMPlimite) && (!FAN)) 
  {
    Serial.println(F("ACIONANDO ARREFECIMENTO DO RADIO"));
    digitalWrite (FANradio, FANligado);
    FAN = true;
  }
  else if ((TMPradio < (TMPlimite - 1)) && (FAN))
  {
    Serial.println(F("DESLIGANDO ARREFECIMENTO DO RADIO"));
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
    TimerLCD = (millis() - (TempoDisplay - DezSegundos)); //Ativando Display por 10 segundos em caso de atividade
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
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor (0, 9); 
        display.print   (F("RADIO"));
        if ((EstadoRADIO == "Recebendo") | (EstadoRADIO == "Transmitindo"))
        {
          display.fillRect ( 31, 9, 95,  7, SSD1306_WHITE);
          display.setTextColor(SSD1306_BLACK);
        }
        else display.setTextColor(SSD1306_WHITE);

        display.setCursor (48, 9);
        display.print   (EstadoRADIO);
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
      // TEMPERATURA RÁDIO --------------------------------------------------------------------------------
      if ((TipoMOSTRADOR == 2) && (TMPctrl)) 
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
      else if (TipoMOSTRADOR == 2) TipoMOSTRADOR = 3;
      // INFORMAÇÔES CLIMA ---------------------------------------------------------------------------
      if ((TipoMOSTRADOR == 3) && (FalarClima))
      {
        VUZ = false;
        VUR = false;
        display.clearDisplay();
        display.fillRect (  0, 0, 128, 16, SSD1306_WHITE);
        display.fillRect ( 74, 2,  52, 12, SSD1306_BLACK);
        display.setCursor(1, 1);        
        display.setTextColor(SSD1306_BLACK);
        display.setTextSize(2);
        display.print  (CIDADE);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(76, 5);
        if  (H < 10) display.print  (F("0"));
        display.print  (H);
        display.print  (F(":"));
        if  (M < 10) display.print  (F("0"));
        display.print  (M);
        display.print  (F(":"));
        if  (S < 10) display.print  (F("0"));
        display.print  (S);
        //display.println(F("---------------------"));
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 18); 
        display.setTextSize(1);
        display.print  ("Temperatura: ");
        display.print  (TempLOCAL);
        display.print  ((char)247);
        display.println("C");
        display.setCursor(0, 27);
        display.print  ("Humidade:    ");
        display.print  (UmidLOCAL);
        display.println("%");
        display.setCursor(0, 36);
        display.print  ("Pressao: ");
        display.print  (PresLOCAL);
        display.println("hpa");
        display.setCursor(0, 45);
        display.print  ("Vento:   ");
        display.print  (VelVENTO * 3.6);
        display.println("k/h ");
        display.setCursor(0, 54);
        display.print  ("Sentido: ");
        if      ((DirVENTO >= 337.5) || (DirVENTO <  22.4)) display.println (F("N->S"));
        else if ((DirVENTO >=  22.5) && (DirVENTO <  67.4)) display.println (F("NE->SO"));
        else if ((DirVENTO >=  67.5) && (DirVENTO < 112.4)) display.println (F("L->O"));
        else if ((DirVENTO >= 112.5) && (DirVENTO < 157.4)) display.println (F("SE->NO"));
        else if ((DirVENTO >= 157.5) && (DirVENTO < 202.4)) display.println (F("S->N"));
        else if ((DirVENTO >= 202.5) && (DirVENTO < 247.4)) display.println (F("SO->NE"));
        else if ((DirVENTO >= 247.5) && (DirVENTO < 292.4)) display.println (F("O->L"));
        else if ((DirVENTO >= 292.5) && (DirVENTO < 337.4)) display.println (F("NO->SE"));
      }
      else if (TipoMOSTRADOR == 3) TipoMOSTRADOR = 4;
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
      // INFORMAÇÔES GERAIS ---------------------------------------------------------------------------
      if (TipoMOSTRADOR == 5)
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
        display.println  (LONTIz);
        display.print  (F("ON: "));
        display.print    (VONz);
        display.println(F("<"));
        display.print  (F("LA: "));
        display.println  (LAz, 0);
        if (VRXz > VTXz)
        {
          display.print  (F("RX: "));
          display.print    (VRXz);
          display.println(F("<"));
          display.print  (F("TX: "));
          display.println  (VTXz);
        }
        else
        {
          display.print  (F("TX: "));
          display.println  (VTXz);
          display.print  (F("RX: "));
          display.print    (VRXz);
          display.println(F("<"));
        }
        display.print  (F("OF: "));
        display.print    (LOFFz);
        //display.print  (F(" TA: "));
        //display.print    (LTAOFFz);

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
  unsigned long CronoBotaoApertado = millis();
  byte Z = 0;
  while (!digitalRead(BotaoBOOT)) 
  {
    if ((millis() - CronoBotaoApertado) > 500)
    {
      digitalWrite (LED, 1);
      delay (100);
      digitalWrite (LED, 0);
      CronoBotaoApertado = millis();
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
  Serial.println(F("Entrou no menu INICIAL"));
  digitalWrite (PinPTTradio, LOW);
  telaOK();
  bool Sair  = false;
  opcao = 1;
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
      unsigned long CronoConfig = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - CronoConfig) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          CronoConfig = millis();
          Z++;
          if (Z > 3) 
          {
            switch (opcao)
            {
              case 1: 
                opcao = 1;
                MenuSETUPbasico();
                opcao = 4;
              break;
              case 2:
                opcao = 1;
                MenuSETUPmensagens();
                opcao = 4;
              break;
              case 3:
                opcao = 1;
                MenuSETUPwifi();  
                opcao = 4;
              break;  
              case 4:
                opcao = 1;
                MenuSETUPrelogio();  
                opcao = 4;
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
  Serial.println(F("Entrou no menu SETUP BASICO"));
  telaOK();
  unsigned long CronoBasico = millis();
  bool Voltar            = false;
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
    if (opcao == 4) display.println(F(">CTRL TEMPERATURA"));
    else            display.println(F(" Ctrl temperatura"));
    if (opcao == 5) display.println(F(">VOLTAR"));
    else            display.println(F(" Voltar"));
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      byte Z = 0;
      CronoBasico = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - CronoBasico) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          CronoBasico = millis();
          Z++;
          if (Z > 3) 
          {
            switch (opcao)
            {
              case 1: 
                ADDPTTZello();
                opcao = 4;
                break;
              case 2:
                PosicionarLDRZello(); 
                opcao = 4;
                break;
              case 3:
                CalibrarLDRzello(); 
                opcao = 4;
                break;
              case 4:
                opcao = 1;
                ControleTemperatura(); 
                opcao = 4; 
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
}

void ADDPTTZello()
{
  Serial.println(F("Entrou no ADD BOTAO PTT ZELLO"));
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
  Serial.println(F("ACIONOU O PTT DO ZELLO"));
  digitalWrite (PinPTTzello, HIGH);
  digitalWrite (LED,      HIGH);
  delay (2000);
  digitalWrite (PinPTTzello, LOW);
  digitalWrite (LED,      LOW);
  Serial.println(F("DESLIGOU O PTT DO ZELLO"));
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
  Serial.println(F("Entrou no menu POSICIONAR LDR"));
  telaOK();
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
  unsigned long CronoPosLDR = millis();
  while (digitalRead(BotaoBOOT)) 
  {       
    mmLDRzello();
    if ((millis() - CronoPosLDR) > 100)      
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
      CronoPosLDR = millis();
    }
  }
  delay(200);
  while (!digitalRead(BotaoBOOT)) Pisca();
  delay(200);
}

void CalibrarLDRzello()
{
  Serial.println(F("Entrou no menu CALIBRAR LDR ZELLO"));
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
  Serial.println(F("Iniciando CALIBRAÇÃO"));
  Serial.println(F("Coletando valor para o LARANJA"));
  unsigned long CronoCalibra = millis();
  int           PTTZlaranja  = mmLDRzello();
  unsigned long Crono2       = millis();   
  unsigned long SOMATORIA    = 0;
  unsigned int  NumVEZES     = 0;   
  while ((millis() - CronoCalibra) < QuinzeSegundos)
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
      display.print (F("  LARANJA  =  "));
      if ((millis() - CronoCalibra) < DezSegundos)
      {
        display.print  (mmLDRzello());
      }
      else
      {
        SOMATORIA = SOMATORIA + mmLDRzello();
        NumVEZES++;
        display.print (SOMATORIA / NumVEZES);
      }
      int coluna = map((millis() - CronoCalibra), 0, QuinzeSegundos, 0, 126);
      display.drawRect( 0, 55,    128, 60, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 58, SSD1306_WHITE);
      display.display();
      Crono2 = millis();
    }
  }
  PTTZlaranja = (SOMATORIA / NumVEZES);
  SOMATORIA = 0;
  NumVEZES  = 0; 
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  Serial.print   (F("LARANHA = "));
  display.println(PTTZlaranja);
  Serial.println (PTTZlaranja);
  display.display();
  display.clearDisplay();  
  digitalWrite (PinPTTzello, HIGH);
  CronoCalibra = millis();
  while ((millis() - CronoCalibra) < TempoDoPulso) mmLDRzello();
  digitalWrite (PinPTTzello, LOW);    
  delay (3000);
  digitalWrite (LED, 0);
  
  // Procedimento 2/3 - Coletando valor TX ---------------------------------------------------------------------------------------------
  Serial.println(F("Coletando valor para o VERMELHO"));
  int PTTZvermelho = mmLDRzello();
  CronoCalibra     = millis();
  Crono2           = millis();      
  while ((millis() - CronoCalibra) < QuinzeSegundos)
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
      if ((millis() - CronoCalibra) < DezSegundos)
      {
        display.print  (mmLDRzello());
      }
      else
      {
        SOMATORIA = SOMATORIA + mmLDRzello();
        NumVEZES++;
        display.print (SOMATORIA / NumVEZES);
      }
      int coluna = map((millis() - CronoCalibra), 0, QuinzeSegundos, 0, 126);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();           
    }
  }
  PTTZvermelho = (SOMATORIA / NumVEZES);
  SOMATORIA = 0;
  NumVEZES  = 0;
  digitalWrite (LED, 1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  Serial.print   (F("VERMELHO = "));
  display.println(PTTZvermelho);
  Serial.println (PTTZvermelho);
  display.display();
  display.clearDisplay();  
  digitalWrite (PinPTTzello, HIGH);
  CronoCalibra = millis();
  while ((millis() - CronoCalibra) < TempoDoPulso) mmLDRzello();
  digitalWrite (PinPTTzello, LOW);    
  delay (3000);
  digitalWrite (LED, 0);
    
  // Procedimento 3/3 - Coletando valor RX ---------------------------------------------------------------------------------------------
  Serial.println(F("Coletando valor para o VERDE"));
  int PTTZverde = mmLDRzello();
  CronoCalibra  = millis();
  Crono2        = millis();      
  while ((millis() - CronoCalibra) < 14000)
  {
    mmLDRzello();
    if ((millis() - Crono2) > 100)      
    {
      digitalWrite (PinPTTradio, HIGH);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("3.3 CALIBRANDO EM RX"));
      display.println(F("---------------------"));
      display.setCursor(0, 30);
      display.print(F("   VERDE  =  "));
      if ((millis() - CronoCalibra) < DezSegundos)
      {
        display.print  (mmLDRzello());
      }
      else
      {
        SOMATORIA = SOMATORIA + mmLDRzello();
        NumVEZES++;
        display.print (SOMATORIA / NumVEZES);
      }
      int coluna = map((millis() - CronoCalibra), 0, QuinzeSegundos, 0, 126);
      display.drawRect( 0, 55,    128, 61, SSD1306_WHITE);
      display.fillRect( 2, 57, coluna, 59, SSD1306_WHITE);
      display.display();
      Crono2 = millis();           
    }
  }
  PTTZverde = (SOMATORIA / NumVEZES);
  digitalWrite (LED, 1);
  digitalWrite (PinPTTradio, LOW);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(40, 25);
  Serial.print   (F("VERDE = "));
  display.println(PTTZverde);
  Serial.print   (PTTZverde);
  display.display();
  display.clearDisplay();      
  delay (1000);
  digitalWrite (LED, 0);

  // SALVAR DADOS NA EEPROM ---------------------------------------------------------------------------------------------------------------
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  Serial.println (F("SALVANDO CALIBRAÇÃO"));
  display.println(F(" SALVANDO CALIBRACAO "));
  display.println(F("---------------------"));
  display.println();
  VONz = PTTZlaranja;     
  VTXz = PTTZvermelho;
  VRXz = PTTZverde;

  DifRXONz = VONz - VRXz;
  DifTXONz = VONz - VTXz;
  
  display.println(PTTZlaranja);                
  display.println(PTTZverde);
  display.println(PTTZvermelho);
  display.display();
  display.clearDisplay();
   
  EEPROM.writeInt (evonZ, VONz);  // Gravando valores na EEPROM
  EEPROM.writeInt (edifrxonZ, DifRXONz);
  EEPROM.writeInt (ediftxonZ, DifTXONz);
  EEPROM.commit();
  for (int x = 0; x < NAZ;  x++) MediaLDRzello[x] = VONz;
  for (int x = 0; x < NAAz; x++) MediaONz[x]      = VONz;
  VRXz = VONz - DifRXONz;
  VTXz = VONz - DifTXONz;
  if (VRXz > VTXz)
  {
    LOFFz = (VTXz / 2);
    LAz   = VONz - (DifRXONz / 2);
  }
  if (VTXz > VRXz)
  {
    LOFFz = (VRXz / 2);
    LAz   = VONz - (DifTXONz / 2);
  }
  delay (2000); 
}

void ControleTemperatura()
{
  Serial.println(F("Entrou no menu CONTROLE DE TEMPERATURA"));
  telaOK();
  bool Voltar  = false;
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
      unsigned long CronoBotao = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - CronoBotao) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          CronoBotao = millis();
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
  Serial.println(F("Entrou no menu MENSAGENS"));
  telaOK();
  unsigned long CronoRetorno = millis();
  unsigned long CronoBotao   = millis();
  bool          Voltar       = false;
  while (!Voltar)
  {
    if (MP3OK)
    {
      if ((millis() - CronoRetorno) > DezSegundos) Voltar = true;
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
      if (opcao == 3) display.print  (F(">CLIMA       "));
      else            display.print  (F(" Clima       "));
      display.println                  (FalarClima);
      if (opcao == 4) display.print  (F(">ESTADO LINK "));
      else            display.print  (F(" Estado Link "));
      display.println                  (FalarStatus);
      if (opcao == 5) display.print  (F(">ROGER BEEP  "));
      else            display.print  (F(" Roger Beep  "));
      display.println                  (RogerBeeps);
      if (opcao == 6) display.print  (F(">VOLUME MP3  "));
      else            display.print  (F(" Volume mp3  "));
      display.println                  (Volume);
      if (!digitalRead(BotaoBOOT))
      {
        digitalWrite (LED, 1);
        delay(250);
        digitalWrite (LED, 0);
        delay(100);
        byte Z  = 0;
        CronoBotao   = millis();
        CronoRetorno = millis();
        while (!digitalRead(BotaoBOOT))
        {
          if ((millis() - CronoBotao) > 500)
          {
            digitalWrite (LED, 1);
            delay (20);
            digitalWrite (LED, 0);
            CronoBotao = millis();
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
                  FalarClima = !FalarClima; 
                  break;
                case 4:
                  FalarStatus = !FalarStatus; 
                  break;
                case 5:
                  RogerBeeps = !RogerBeeps;
                  break;  
                case 6:
                  Volume = (Volume + 1); 
                  if (Volume > 30) Volume = 5;
                  opcao--;
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
      if (FalarClima != EEPROM.readBool(eClima)) 
      {
        EEPROM.writeBool(eClima, FalarClima);
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
  telaOK();
}

// CONFIGURAÇÕES WiFi --------------------------------------------------------------------------------------------------------------------------------
void MenuSETUPwifi()
{
  Serial.println(F("Entrou no menu WiFi"));
  telaOK();
  unsigned long CronoBotao = millis();
  bool          Voltar = false;
  opcao = 1;
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
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - CronoBotao) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          CronoBotao = millis();
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
  Serial.println(F("Entrou no menu RELOGIO"));
  telaOK();
  unsigned long CronoBotao = millis();
  bool          Voltar     = false;
  byte          Z          = 0;
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
      CronoBotao = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - CronoBotao) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          CronoBotao = millis();
          Z++;
          if (Z > 3) 
          {
            switch (opcao)
            {
              case 1: 
                HoraCerta();
                opcao = 2;
              break;
              case 2:
                FusoHorario();
                opcao = 2;
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
  Serial.println(F("Conectando ao WiFi..."));
  wm.setConfigPortalTimeout(1); 
  WiFiOK = wm.autoConnect();
  delay(500);
  Fuso = EEPROM.readByte (eFuso);
  if (WiFiOK)
  {
    Serial.println(F("Conectando do servidor NTP"));
    ntp.begin();
    delay(500);
    Serial.println(F("Coletando dados NTP"));
    ntp.setTimeOffset(-3600 * Fuso);
    if (ntp.update()) 
    {
      Serial.println(F("DADOS NTP COLETADOS"));
      D  = 0;
      H  = (ntp.getHours());
      M  = (ntp.getMinutes());
      S  = (ntp.getSeconds());
      HC = true;
      Relogio = millis();
    }
    else
    {
      Serial.println(F("NTP FALHOU!!!"));
    }
    Serial.println(F("Desconectando do servidor NTP"));
    ntp.end();
  }
  else
  {
    Serial.println(F("WiFi FALHOU!!!"));
    return;
  }
  Serial.println(F("Desligando WiFi..."));
  WiFi.disconnect();
  wm.disconnect();
}

void HoraCerta()
{  
  Serial.println(F("Acentando a HORA do RELOGIO"));
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

void ClimaOpenWeather()
{
  wm.setConfigPortalTimeout(1);
  wm.autoConnect();
  Serial.println(F("Conectando WiFi..."));
  unsigned long CronoWiFiClima = millis();
  while ((WiFi.status() != WL_CONNECTED) && ((millis() - CronoWiFiClima) < 2000))
  {
    ESTADOzello();
    ESTADOradio();
    Display();
    Serial.print(".");
  }
  WiFiOK = WiFi.status() == WL_CONNECTED;
  if (WiFiOK)
  {
    Serial.println(F("WiFi OK..."));
    Serial.println(F("LOGANDO no OpenWeather.org."));
    if (client.connect(servername, 80))
    { 
      Serial.println(F("LOGIN OK..."));
      delay(50);
      client.println("GET /data/2.5/weather?id=" + CityID + "&units=metric&APPID=" + APIKEY); // Checando conexão e Logando cliente
      client.println("Host: api.openweathermap.org");
      client.println("User-Agent: ArduinoWiFi/1.1");
      Serial.println(F("AGUARANDO dados..."));
      result = "";
      while ((client.connected()) && (!client.available()))    // Aguardando por Dados de clima
        delay(1);  
      Serial.println(F("COLETANDO dados..."));                                        
      while ((client.connected()) || (client.available()))     // Conectado ou Dados disponíveis
      { 
        char c = client.read();                       // Obtendo dados do buffer de ethernet
        result = result + c;
      }
      Serial.println(result);
      client.println("Connection: close");
      Serial.println(F("LOGOFF..."));
      client.println();
    }
    else 
    {
      ColetouBOLETIM = false;
      Serial.println(F("ColetouBOLETIM = false;"));
      Serial.println(F("Desconectando WiFi..."));
      WiFi.disconnect();
      return;
    }
    client.stop();                                      //stop client
    char jsonArray [result.length() + 1];
    result.toCharArray(jsonArray, sizeof(jsonArray));
    jsonArray[result.length() + 1] = '\0';
    StaticJsonDocument<1024> doc;
    DeserializationError  error = deserializeJson(doc, jsonArray);
    Serial.println(F("Tratando Dados"));        //error message if no client connect
    if (error) 
    {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      Serial.println(F("ColetouBOLETIM = false;"));
      ColetouBOLETIM = false;
      Serial.println(F("Desconectando WiFi..."));
      WiFi.disconnect();
      return;
    }
    else
    {
      ColetouBOLETIM = true;
      Serial.println(F("ColetouBOLETIM = true;"));
    }
    int Temperature = doc["main"]["temp"];
    int Humidity    = doc["main"]["humidity"];
    float pressure  = doc["main"]["pressure"];
    float Speed     = doc["wind"]["speed"];
    int degree      = doc["wind"]["deg"];
    TempLOCAL       = Temperature;
    UmidLOCAL       = Humidity;
    PresLOCAL       = pressure;
    VelVENTO        = Speed;
    DirVENTO        = degree;
    Serial.println(F("Dados Coletados"));
    Serial.println(TempLOCAL);
    Serial.println(UmidLOCAL);
    Serial.println(PresLOCAL);
    Serial.println(VelVENTO * 3.6);
    Serial.println(DirVENTO);
    Serial.println(F("Tudo OK ! ! !"));
    delay(5);
  }
  else
  {
    ColetouBOLETIM = false;
    Serial.println(F("ColetouBOLETIM = false;"));
    Serial.println(F("WiFi FALHOU!!!"));
  }
  Serial.println(F("Desconectando WiFi..."));
  WiFi.disconnect();
}

void FusoHorario()
{
  Serial.println(F("Entrou no menu FUSO HORARIO"));
  telaOK();
  bool Voltar = false;
  opcao       = Fuso;
  byte Z      = 0;
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
    unsigned long CronoTimerFuso = millis();
    if (!digitalRead(BotaoBOOT))
    {
      digitalWrite (LED, 1);
      delay(250);
      digitalWrite (LED, 0);
      delay(100);
      CronoTimerFuso = millis();
      while (!digitalRead(BotaoBOOT))
      {
        if ((millis() - CronoTimerFuso) > 500)
        {
          digitalWrite (LED, 1);
          delay (20);
          digitalWrite (LED, 0);
          CronoTimerFuso = millis();
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
