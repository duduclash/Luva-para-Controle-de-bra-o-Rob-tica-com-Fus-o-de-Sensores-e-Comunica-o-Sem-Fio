#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ESP32Servo.h>
#include <string.h> 

// ============================================
// DEFINIÇÕES E PINAGEM (Confira seus pinos!)
// ============================================
#define PIN_BASE      2   // Antigo Servo1
#define PIN_OMBRO     4   // Antigo Servo2
#define PIN_COTOVELO  26  // Antigo Servo3
#define PIN_PUNHO_R   25  // Antigo Servo4
#define PIN_PUNHO_P   33  // Antigo Servo5
#define PIN_GARRA     32  // Antigo Servo6

// ============================================
// OBJETOS SERVOS
// ============================================
Servo servoBase;
Servo servoOmbro;
Servo servoCotovelo;
Servo servoPunhoRoll;
Servo servoPunhoPitch;
Servo servoGarra;

// ============================================
// VARIÁVEIS DE POSIÇÃO (Iniciam em 90 para segurança)
// ============================================
int posBase = 90;
int posOmbro = 90;      // Antigo rollatual
int posCotovelo = 90;   // Antigo rollatual2
int posPunhoRoll = 90;  // Antigo garraroll
int posPunhoPitch = 90; // Antigo garrapitch

// Variáveis "Anteriores" para evitar escrita excessiva (Anti-Jitter)
int lastBase = -1;
int lastOmbro = -1;
int lastCotovelo = -1;
int lastPunhoRoll = -1;
int lastPunhoPitch = -1;
int lastGarraEstado = -1;

// Configurações de movimento
int step = 2;       // Velocidade do movimento (aumente para ir mais rápido)
int deadzone = 20;  // Zona morta do sensor (ignora inclinações pequenas)

// ============================================
// VARIÁVEIS DE ESTADO
// ============================================
int modoControle = 0; // 0 = Braço, 1 = Punho (Antigo 'saida')
int estadoGarra = 0;  // 0 = Fechada, 1 = Aberta (Antigo 'saida2')

// Variáveis para detecção de borda (botões)
int btn1_anterior = 0;
int btn2_anterior = 0;

// ============================================
// BLE - VARIÁVEIS GLOBAIS
// ============================================
static BLEUUID serviceUUID("12345678-1234-1234-1234-123456789abc");
static BLEUUID dataCharUUID("abcd1234-5678-90ab-cdef-123456789abc");

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pDataRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice; 
static BLEClient* pClient = nullptr;

// Dados recebidos
int16_t rxRoll = 0;
int16_t rxPitch = 0;
uint8_t rxBtn1 = 0;
uint8_t rxBtn2 = 0;

unsigned long lastScanTime = 0;
const int scanInterval = 5000;

// ============================================
// CALLBACK DE DADOS (RECEBE DO SERVIDOR)
// ============================================
static void dataNotifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
  // PACOTE 1: Roll, Pitch, Acelerômetro (16 bytes)
  if (length == 16) {
    memcpy(&rxRoll, pData, 2);      // Bytes 0-1
    memcpy(&rxPitch, pData + 2, 2); // Bytes 2-3
    // Ignorando Ax, Ay, Az por enquanto
  } 
  // PACOTE 2: Giroscópio e Botões (14 bytes)
  else if (length == 14) {
    // Ignorando Gx, Gy, Gz (12 bytes)
    rxBtn1 = pData[12]; // Byte 12
    rxBtn2 = pData[13]; // Byte 13
  }
}

// ============================================
// CALLBACKS DE CONEXÃO
// ============================================
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println(">>> CONECTADO! <<<");
    connected = true;
  }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println(">>> DESCONECTADO! <<<");
  }
};

// ============================================
// CONECTAR AO SERVIDOR
// ============================================
bool connectToServer() {
  Serial.print("Conectando a: ");
  Serial.println(myDevice->getAddress().toString().c_str());

  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
  }
  pClient->setClientCallbacks(new MyClientCallback());

  if (!pClient->connect(myDevice)) return false;

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }

  pDataRemoteCharacteristic = pRemoteService->getCharacteristic(dataCharUUID);
  if (pDataRemoteCharacteristic == nullptr) {
    pClient->disconnect();
    return false;
  }

  if (pDataRemoteCharacteristic->canNotify()) {
    pDataRemoteCharacteristic->registerForNotify(dataNotifyCallback);
  }

  return true;
}

// ============================================
// BUSCA DE DISPOSITIVOS (SCAN)
// ============================================
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Verifica se é o nosso dispositivo pelo UUID ou Nome
    if (advertisedDevice.isAdvertisingService(serviceUUID) || 
        advertisedDevice.getName() == "ESP32-C3-SENSOR") {
            
      Serial.println("Dispositivo Encontrado!");
      BLEDevice::getScan()->stop();

      // CORREÇÃO DE MEMÓRIA: Deleta o antigo antes de criar novo
      if (myDevice != nullptr) {
        delete myDevice;
        myDevice = nullptr;
      }
      
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Braço Robótico ESP32 Client...");

  BLEDevice::init("ESP32-Arm-Client");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  // Configura Servos
  servoBase.attach(PIN_BASE);
  servoOmbro.attach(PIN_OMBRO);
  servoCotovelo.attach(PIN_COTOVELO);
  servoPunhoRoll.attach(PIN_PUNHO_R);
  servoPunhoPitch.attach(PIN_PUNHO_P);
  servoGarra.attach(PIN_GARRA);

  // Posição Inicial (para evitar pulos bruscos ao ligar)
  servoBase.write(posBase);
  servoOmbro.write(posOmbro);
  servoCotovelo.write(posCotovelo);
  servoPunhoRoll.write(posPunhoRoll);
  servoPunhoPitch.write(posPunhoPitch);
  servoGarra.write(0);
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void loop() {
  // 1. Gerenciamento de Conexão
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Conexão estabelecida.");
    } else {
      Serial.println("Falha na conexão.");
    }
    doConnect = false;
  }

  if (!connected) {
    if (millis() - lastScanTime > scanInterval) {
      Serial.println("Escaneando...");
      BLEDevice::getScan()->start(5, false);
      lastScanTime = millis();
    }
    return; // Se não conectado, não faz nada abaixo
  }

  // 2. Leitura dos Botões (Toggle)
  if (rxBtn1 == 1 && btn1_anterior == 0) {
    modoControle = !modoControle; // Alterna 0 <-> 1
    Serial.print("Modo alterado para: "); Serial.println(modoControle ? "PUNHO" : "BRAÇO");
  }
  btn1_anterior = rxBtn1;

  if (rxBtn2 == 1 && btn2_anterior == 0) {
    estadoGarra = !estadoGarra; // Alterna 0 <-> 1
    Serial.print("Garra: "); Serial.println(estadoGarra ? "ABERTA" : "FECHADA");
  }
  btn2_anterior = rxBtn2;

  // 3. Lógica de Movimento
  
  // --- MODO 0: BRAÇO (Base, Ombro, Cotovelo) ---
  if (modoControle == 0) {
    
    // CONTROLE DA BASE (Roll do sensor)
    if (rxRoll > deadzone) posBase += step;
    else if (rxRoll < -deadzone) posBase -= step;

    // CONTROLE DE ELEVAÇÃO (Pitch do sensor) - LÓGICA FLUIDA
    
    // >>>> MOVIMENTO PARA FRENTE (Esticar) <<<<
    if (rxPitch > deadzone) {
        // Sobe o ombro
        if (posOmbro < 180) posOmbro += step;

        // Se o ombro já subiu um pouco (> 60), o cotovelo começa a estender junto
        // Isso cria o efeito cascata SUAVE
        if (posOmbro > 60 && posCotovelo > 0) {
            posCotovelo -= step; 
        }
    }
    // >>>> MOVIMENTO PARA TRÁS (Recolher) <<<<
    else if (rxPitch < -deadzone) {
        // Recolhe o cotovelo primeiro/junto
        if (posCotovelo < 180) posCotovelo += step;

        // O ombro só desce se o cotovelo já recolheu um pouco (evita bater na mesa)
        if (posCotovelo > 45 && posOmbro > 0) {
            posOmbro -= step;
        }
    }
  } 
  
  // --- MODO 1: PUNHO ---
  else {
    if (rxPitch > deadzone) posPunhoRoll += step;
    else if (rxPitch < -deadzone) posPunhoRoll -= step;

    if (rxRoll > deadzone) posPunhoPitch += step;
    else if (rxRoll < -deadzone) posPunhoPitch -= step;
  }

  // 4. Limitação de Segurança (Constrain)
  posBase = constrain(posBase, 0, 180);
  posOmbro = constrain(posOmbro, 0, 180);
  posCotovelo = constrain(posCotovelo, 0, 180);
  posPunhoRoll = constrain(posPunhoRoll, 0, 180);
  posPunhoPitch = constrain(posPunhoPitch, 0, 180);

  // 5. Atualização dos Servos (Apenas se mudou o valor)
  if (posBase != lastBase) { servoBase.write(posBase); lastBase = posBase; }
  if (posOmbro != lastOmbro) { servoOmbro.write(posOmbro); lastOmbro = posOmbro; }
  if (posCotovelo != lastCotovelo) { servoCotovelo.write(posCotovelo); lastCotovelo = posCotovelo; }
  if (posPunhoRoll != lastPunhoRoll) { servoPunhoRoll.write(posPunhoRoll); lastPunhoRoll = posPunhoRoll; }
  if (posPunhoPitch != lastPunhoPitch) { servoPunhoPitch.write(posPunhoPitch); lastPunhoPitch = posPunhoPitch; }
  
  // Garra
  int posGarraAlvo = (estadoGarra == 1) ? 180 : 0; // 180 = Aberta, 0 = Fechada (ajuste conforme sua garra)
  if (posGarraAlvo != lastGarraEstado) {
      servoGarra.write(posGarraAlvo);
      lastGarraEstado = posGarraAlvo;
  }

  delay(10); // Pequeno delay para estabilidade do loop
}
