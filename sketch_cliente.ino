

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ESP32Servo.h>
#include <string.h> //para a função memcpy

  int pitchatual = 90;
  int rollatual = 90;
  int valorAnterior = 0;  // valor anterior do valorAtual
  int saida = 0;  // variável que deve se manter no 1 quando valorAtual vai para 1

// ============================================
// ESTRUTURA DE DADOS - REMOVIDA PARA USAR SERIALIZAÇÃO MANUAL
// ============================================

// ============================================
// VARIÁVEIS GLOBAIS
// ============================================
int valorpitch = 0;
int valorroll = 0;
Servo Servo1;
Servo Servo2;
Servo Servo3;
Servo Servo4;
Servo Servo5;
Servo Servo6;

// UUIDs - Devem ser iguais ao servidor
static BLEUUID serviceUUID("12345678-1234-1234-1234-123456789abc");
static BLEUUID dataCharUUID("abcd1234-5678-90ab-cdef-123456789abc");

static boolean doConnect = false;
static boolean connected = false;

// Ponteiro BLE
static BLERemoteCharacteristic* pDataRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static BLEClient* pClient = nullptr;

// Variáveis para armazenar os dados recebidos
int receivedRoll = 0;
int receivedPitch = 0;
float receivedAx = 0.00;
float receivedAy = 0.00;
float receivedAz = 0.00;
float receivedGx = 0.00;
float receivedGy = 0.00;
float receivedGz = 0.00;
uint8_t receivedBotao1 = 0;
uint8_t receivedBotao2 = 0;

unsigned long lastScanTime = 0;
const int scanInterval = 5000;

int ondas = 0;
unsigned long tempo = 0;


}
// ============================================
// CALLBACK - RECEBE TODOS OS DADOS DE UMA VEZ (CORRIGIDO)
// ============================================
static void dataNotifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
  // Verifica se é o PACOTE 1 (Roll, Pitch, Acelerômetro)
  if (length == 16) {
    int offset = 0;

    // 1. Roll (2 bytes - int16_t) - CORREÇÃO DE SINAL
    int16_t tempRoll;
    memcpy(&tempRoll, pData + offset, 2);
    receivedRoll = tempRoll; // Atribuição de int16_t para int (32 bits) faz a extensão de sinal
    offset += 2;
    
    // 2. Pitch (2 bytes - int16_t) - CORREÇÃO DE SINAL
    int16_t tempPitch;
    memcpy(&tempPitch, pData + offset, 2);
    receivedPitch = tempPitch; // Atribuição de int16_t para int (32 bits) faz a extensão de sinal
    offset += 2;
    
    // 3. Acelerômetro X, Y, Z (3 * 4 = 12 bytes)
    memcpy(&receivedAx, pData + offset, 4);
    offset += 4;
    memcpy(&receivedAy, pData + offset, 4);
    offset += 4;
    memcpy(&receivedAz, pData + offset, 4);
    offset += 4;
    
    // Agora, esperamos o PACOTE 2 para imprimir os dados completos
    return;
  } 
  
  // Verifica se é o PACOTE 2 (Giroscópio e Botões)
  else if (length == 14) {
    int offset = 0;
    
    // 4. Giroscópio X, Y, Z (3 * 4 = 12 bytes)
    memcpy(&receivedGx, pData + offset, 4);
    offset += 4;
    memcpy(&receivedGy, pData + offset, 4);
    offset += 4;
    memcpy(&receivedGz, pData + offset, 4);
    offset += 4;
    
    // 5. Botão 1 (1 byte - uint8_t)
    receivedBotao1 = pData[offset++];
    
    // 6. Botão 2 (1 byte - uint8_t)
    receivedBotao2 = pData[offset++];

    // Debug - mostra todos os dados recebidos (somente após o segundo pacote)
    Serial.printf("Roll:%d Pitch:%d | Ax:%.2f Ay:%.2f Az:%.2f | Gx:%.2f Gy:%.2f Gz:%.2f | B1:%d B2:%d\n",
                  receivedRoll, receivedPitch, 
                  receivedAx, receivedAy, receivedAz,
                  receivedGx, receivedGy, receivedGz,
                  receivedBotao1, receivedBotao2);
    return;
  }
  
  // Se o tamanho não for 16 nem 14, reporta o erro de tamanho
  Serial.print(" Tamanho incorreto! Esperado: 16 ou 14 | Recebido: ");
  Serial.println(length);
}

// ============================================
// CALLBACKS DE CONEXÃO
// ============================================
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("\n✓✓✓ CONECTADO AO ESP32-C3! ✓✓✓");
    connected = true;
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("\n✗✗✗ DESCONECTADO DO ESP32-C3 ✗✗✗");
    Serial.println("Reiniciando busca...\n");
  }
};

// ============================================
// FUNÇÃO DE CONEXÃO AO SERVIDOR
// ============================================
bool connectToServer() {
  Serial.println("\n========================================");
  Serial.print("Tentando conectar ao ESP32-C3: ");
  Serial.println(myDevice->getAddress().toString().c_str());
  Serial.println("========================================");

  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    Serial.println("✓ Cliente BLE criado");
  }

  pClient->setClientCallbacks(new MyClientCallback());

  Serial.println("Iniciando conexão física...");
  if (!pClient->connect(myDevice)) {
    Serial.println("❌ Falha na conexão inicial");
    return false;
  }

  Serial.println("✓ Conexão física estabelecida");
  Serial.println("Procurando serviço...");
  delay(1000);

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println(" Serviço não encontrado!");
    Serial.print("UUID procurado: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  Serial.println("✓ Serviço encontrado!");
  Serial.println("Procurando característica de dados...");

  // Busca a característica única de dados
  pDataRemoteCharacteristic = pRemoteService->getCharacteristic(dataCharUUID);
  if (pDataRemoteCharacteristic == nullptr) {
    Serial.println(" Característica não encontrada!");
    Serial.print("UUID procurado: ");
    Serial.println(dataCharUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  Serial.println("✓ Característica encontrada!");
  Serial.println("Configurando notificações...");

  // Registra para receber notificações
  if (pDataRemoteCharacteristic->canNotify()) {
    pDataRemoteCharacteristic->registerForNotify(dataNotifyCallback);
    Serial.println("✓ Notificação configurada com sucesso!");
  } else {
    Serial.println(" Característica não pode notificar!");
    pClient->disconnect();
    return false;
  }

  connected = true;
  Serial.println("\n========================================");
  Serial.println("===== CONECTADO COM SUCESSO! =====");
  Serial.println("===== RECEBENDO DADOS... =====");
  Serial.println("========================================\n");
  return true;
}

// ============================================
// BUSCA DE DISPOSITIVOS
// ============================================
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE encontrado: ");
    Serial.print(advertisedDevice.toString().c_str());

    // Verifica se é o nosso dispositivo pelo UUID do serviço
    if (advertisedDevice.haveServiceUUID() && 
        advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.println(" <- ✓ ESTE É O NOSSO DISPOSITIVO!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    } 
    // Ou verifica pelo nome
    else if (advertisedDevice.haveName() && 
             advertisedDevice.getName() == "ESP32-C3-SENSOR") {
      Serial.println(" <- ✓ ESTE É O NOSSO DISPOSITIVO!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    } 
    else {
      Serial.println(" (não é o que procuramos)");
    }
  }
};

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n========================================");
  Serial.println("  ESP32-WROOM-32 BLE Client");
  Serial.println("  Versão Otimizada - Baixa Latência");
  Serial.println("========================================");

  // Inicializa BLE
  BLEDevice::init("ESP32-WROOM32-Client");
  BLEDevice::setPower(ESP_PWR_LVL_P7);
  BLEDevice::setMTU(517);
  Serial.println("✓ BLE inicializado");
  Serial.println("✓ MTU configurado para 517");

  // Configura o scanner BLE
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  Serial.println("✓ Scanner BLE configurado");

  // Configura os servos
  Servo1.attach(2);
  Servo2.attach(4);
  Servo3.attach(25);
  Servo4.attach(27);
  Servo5.attach(32);
  Servo6.attach(33);


  Serial.println("✓ Servos configurados (Pinos 2 e 4)");
  
  Serial.println("\n✓✓✓ Setup completo! ✓✓✓");
  Serial.println("Iniciando busca pelo ESP32-C3...\n");
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void loop() {
  // Se desconectar, tenta conectar
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("✓ Sucesso na conexão ao servidor BLE!");
      lastScanTime = millis();
    } else {
      Serial.println("✗ Falha na conexão ao servidor BLE.");
    }
    doConnect = false;
  }

  // Se não está conectado, procura o dispositivo periodicamente
  if (!connected) {
    if (millis() - lastScanTime > scanInterval) {
      Serial.println("\n--- Procurando pelo ESP32-C3-SENSOR... ---");
      BLEDevice::getScan()->start(5, false);
      lastScanTime = millis();
    }
  }

  // Verifica se a conexão foi perdida
  if (connected && pClient != nullptr && !pClient->isConnected()) {
    Serial.println("\n Conexão perdida detectada!");
    connected = false;
  }

  // ========== CONTROLE DOS SERVOS ==========
  if (connected) {
    // Mapeia os valores de roll e pitch para os servos
    valorroll = map(receivedRoll, -90, 90, 0, 180);
    valorpitch = map(receivedPitch, -90, 90, 0, 180);
    
    // Limita os valores entre 0 e 180
    valorroll = constrain(valorroll, 0, 180);
    valorpitch = constrain(valorpitch, 0, 180);

    rollatual = constrain(rollatual,0,360);
    pitchatual= constrain(pitchatual,0,360);
    
    // Atualiza os servos
if (receivedRoll > 20) {
    // Incrementa o valor (pode mudar o '1' para ajustar a velocidade)
    rollatual = rollatual + 2; 
}

// Diminui Gradualmente 'rollatual' se 'receivedRoll' for baixo
else if (receivedRoll < -20) {
    // Decrementa o valor
    rollatual = rollatual - 2;
}

// Escreve a nova posição nos Servos 1 e 2
Servo3.write(rollatual);
Servo2.write(rollatual);

// Aumenta Gradualmente 'pitchatual'
if (receivedPitch > 20) {
    pitchatual = pitchatual + 2;
}

// Diminui Gradualmente 'pitchatual'
else if (receivedPitch < -20) {
    pitchatual = pitchatual - 2;
}

// Escreve a nova posição no Servo 3
Servo1.write(pitchatual);    
//-----------------------------------------------------------------------------------------------------------------
  if (receivedBotao1 == 1 && valorAnterior == 0) {
    // quando valorAtual vai para 1, inverte o estado de saida
    saida = 1 - saida;
  }

  valorAnterior = receivedBotao1
  ;

  // use a variável saida como necessário
  Serial.println(saida);


  // use a variável saida como necessário
   
    if (receivedBotao1 == 1) {
      Serial.println(">>> BOTÃO 1 PRESSIONADO! <<<");
    
    }
    
    
    if (receivedBotao2 == 1) {
      Serial.println(">>> BOTÃO 2 PRESSIONADO! <<<");
    
    }
    
  delay(10);
  }
  
}
