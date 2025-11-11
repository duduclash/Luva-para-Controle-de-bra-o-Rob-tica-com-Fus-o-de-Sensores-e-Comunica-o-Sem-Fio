#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-123456789abc"

void leitura() {
  Serial.print("AX: ");
  Serial.print(sensor.readFloatAccelX());
  Serial.print(" | AY: ");
  Serial.print(sensor.readFloatAccelY());
  Serial.print(" | AZ: ");
  Serial.println(sensor.readFloatAccelZ());

  Serial.print("GX: ");
  Serial.print(sensor.readFloatGyroX());
  Serial.print(" | GY: ");
  Serial.print(sensor.readFloatGyroY());
  Serial.print(" | GZ: ");
  Serial.println(sensor.readFloatGyroZ());

  Serial.println();
}



void configurarLSM6DS3() {
  sensor.settings.accelEnabled = 1;
  sensor.settings.accelSampleRate = 5;  // 416 Hz
  sensor.settings.accelRange = 1;       // ±2g (índice 0)

  sensor.settings.gyroEnabled = 1;
  sensor.settings.gyroSampleRate = 5;  // 416 Hz
  sensor.settings.gyroRange = 1;       // ±245 dps (índice 1)
}


void BLESETUP() {
  BLEDevice::init("ESP32-C3-SENSOR");
  BLEDevice::setMTU(517);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // APENAS 1 característica para todos os dados
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  pServer->getAdvertising()->start();

  Serial.println("✓ BLE iniciado - Transmissão otimizada com botões!");
}

void BLE(float ax, float ay, float az, float gx, float gy, float gz) {
  if (!deviceConnected) return;

  // Lê os botões (invertido porque usa INPUT_PULLUP)
  uint8_t btn1 = !digitalRead(BOTAO1_PIN);  // 1 = pressionado, 0 = solto
  uint8_t btn2 = !digitalRead(BOTAO2_PIN);

  // --- PACOTE 1: Roll, Pitch, Acelerômetro (16 bytes) ---
  uint8_t packet1[16];
  int offset = 0;
  
  // Roll (int16_t = 2 bytes)
  int16_t roll_val = (int16_t)roll2;
  memcpy(packet1 + offset, &roll_val, 2);
  offset += 2;
  
  // Pitch (int16_t = 2 bytes)
  int16_t pitch_val = (int16_t)pitch2;
  memcpy(packet1 + offset, &pitch_val, 2);
  offset += 2;
  
  // Acelerômetro X, Y, Z (3 * 4 = 12 bytes)
  memcpy(packet1 + offset, &ax, 4);
  offset += 4;
  memcpy(packet1 + offset, &ay, 4);
  offset += 4;
  memcpy(packet1 + offset, &az, 4);
  offset += 4;
  
  pCharacteristic->setValue(packet1, 16);
  pCharacteristic->notify();

  // --- PACOTE 2: Giroscópio e Botões (14 bytes) ---
  uint8_t packet2[14];
  offset = 0;
  
  // Giroscópio X, Y, Z (3 * 4 = 12 bytes)
  memcpy(packet2 + offset, &gx, 4);
  offset += 4;
  memcpy(packet2 + offset, &gy, 4);
  offset += 4;
  memcpy(packet2 + offset, &gz, 4);
  offset += 4;
  
  // Botão 1 (1 byte)
  packet2[offset++] = btn1;
  
  // Botão 2 (1 byte)
  packet2[offset++] = btn2;

  pCharacteristic->setValue(packet2, 14);
  pCharacteristic->notify();

  // Debug opcional (mantenha o seu debug original)
  Serial.printf("R:%d P:%d | Gx:%.2f Gy:%.2f Gz:%.2f | B1:%d B2:%d\n",
                roll2, pitch2, gx, gy, gz, btn1, btn2);
}




void calibracaodisplay() {

  display.clearDisplay();

  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(15, 38);
  display.print("MANTENHA O SENSOR");

  display.drawBitmap(47, 2, image_operation_warning_bits, 32, 32, 1);

  display.setCursor(48, 48);
  display.print("PARADO");

  display.display();
}


void exibirErroSensor() {

  display.clearDisplay();

  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(46, 14);
  display.print("SENSOR");

  display.setCursor(23, 26);
  display.print("NAO ENCONTRADO");

  display.setCursor(2, 38);
  display.print("VERIFIQUE AS CONEXOES");

  display.display();
}


void connectble() {

  display.clearDisplay();

  display.drawBitmap(50, 6, image_bluetooth_bits, 28, 32, 1);

  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(26, 41);
  display.print("CONECTANDO...");

  display.display();
}

void conectado() {
  display.clearDisplay();

  display.drawBitmap(50, 8, image_bluetooth_connected_bits, 28, 32, 1);

  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(38, 41);
  display.print("CONECTADO");

  display.setCursor(32, 51);
  display.print("COM SUCESSO");

  display.display();
}


void veriqBLE() {
  if (!deviceConnected) {

    ESP.restart();
  }
}

int battery() {
  // ... (todo o seu código de cálculo da tensão) ...

  uint32_t Vbatt = 0;
  for (int i = 0; i < 16; i++) {
    Vbatt += analogReadMilliVolts(A0);
  }

  float Vbattf = FATOR_DIVISOR * Vbatt / 16 / 1000.0;
  int percentagem = 100 * (Vbattf - VOLTAGEM_MINIMA) / (VOLTAGEM_MAXIMA - VOLTAGEM_MINIMA);
  percentagem = constrain(percentagem, 0, 100);

  tensaobatt = Vbattf;

  percentagemOld[indice_bateria] = percentagem;
  indice_bateria++;
  if (indice_bateria >= N_LEITURAS) {
    indice_bateria = 0;
  }

  float soma = 0;
  for (int j = 0; j < N_LEITURAS; j++) {
    soma += percentagemOld[j];
  }
  int mediaFinal = soma / N_LEITURAS;

  return mediaFinal;  // <-- Devolve o valor calculado e suavizado
}