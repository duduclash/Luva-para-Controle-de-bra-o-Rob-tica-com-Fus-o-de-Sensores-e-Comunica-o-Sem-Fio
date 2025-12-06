# Luva para Controle de Braço Robótico com Fusão de Sensores e Comunicação Sem Fio

[cite_start]Este repositório contém o código-fonte e a documentação do Trabalho de Conclusão de Curso (TCC) do curso Técnico em Eletrônica da ETEC Rubens de Faria e Souza [cite: 39-40]. O projeto consiste em uma luva sensorial capaz de captar movimentos inerciais e transmiti-los via Bluetooth Low Energy (BLE) para o controle em tempo real de um braço robótico impresso em 3D.

## 📋 Resumo

O projeto aborda o desenvolvimento de um sistema de teleoperação acessível. Utilizando um sensor inercial (LSM6DS3) integrado a um microcontrolador ESP32-C3 (na luva), os dados de aceleração e giroscópio são processados através de um **Filtro Complementar** para garantir estabilidade. Os dados tratados são enviados via BLE para um segundo ESP32 (no braço), que converte os sinais em movimentos precisos nos servomotores.

### ✨ Principais Funcionalidades

* **Comunicação Sem Fio (BLE):** Conexão estável e de baixo consumo entre a luva (Servidor) e o braço (Cliente).
* [cite_start]**Fusão de Sensores:** Implementação de Filtro Complementar (Acelerômetro + Giroscópio) para eliminar ruídos e deriva [cite: 184-187].
* **Interface Visual:** Display OLED na luva exibindo status da conexão, nível de bateria e ângulos (Roll/Pitch).
* **Modos de Controle:** Alternância entre controle do *Braço* (Base/Ombro/Cotovelo) e *Punho/Garra* através de botões físicos.
* **Segurança:** Limites de software para impedir movimentos que danifiquem a estrutura mecânica.

## 🛠️ Hardware Utilizado

### Luva Transmissora
* Microcontrolador: **Seeed Studio XIAO ESP32-C3**
* Sensor Inercial: **LSM6DS3** (Acelerômetro e Giroscópio)
* Display: **OLED 1.3" I2C (SH1106)**
* Bateria: LiPo 3.7V 750mAh
* Componentes extras: 2 Botões táteis, Chave HH, Resistores.

### Braço Receptor
* Microcontrolador: **ESP32 DevKit V1** (ou similar)
* Servomotores: 3x **MG996R** (Base/Ombro) e 3x **SG90** (Punho/Garra)
* Estrutura: Braço Robótico impresso em 3D.
    * **Modelo:** Robotic Arm 4.0
    * [cite_start]**Créditos do Design:** [Fabri Creator](https://fabricreator.com)[cite: 111].
* Fonte de Alimentação: 5V.

## 📂 Estrutura do Código

O projeto está dividido em dois módulos principais:

### 1. Luva (Transmissor)
Responsável pela leitura dos sensores, filtro complementar e servidor BLE.
* **Arquivo Principal:** `luvatcccorinthiansbimundial_copy_2025052621_copy_20250908191331.ino`
* **Funções Auxiliares:** `funcoes.ino` (Contém lógica de leitura, display e configuração BLE)

### 2. Braço (Receptor)
Responsável por receber os pacotes BLE e controlar os servos motores.
* **Arquivo Principal:** `sketch_cliente.ino`

## 👥 Autores

Trabalho desenvolvido pelos alunos da ETEC Rubens de Faria e Souza (Sorocaba/SP):

* **Bruno Machado Pires**
* **Cauã Alves do Amaral**
* **Eduardo Henrique Belmiro**
* **Enzo Israel Domingues**
* **Richard Henrique Pereira Siqueira**

Orientador: Prof. Diego Bianchi Macedo.

## 📄 Licença e Créditos

* **Código:** Distribuído sob a licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.
* **Design do Braço 3D:** Todos os créditos do modelo mecânico (Robotic Arm 4.0) pertencem ao **Fabri Creator**.
