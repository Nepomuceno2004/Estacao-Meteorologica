
# 🌦️ Estação Meteorológica IoT com Interface Web

Projeto desenvolvido para monitoramento climático em tempo real usando a plataforma **BitDogLab** com **RP2040**, sensores I2C, alertas visuais e sonoros, e interface web interativa.

## 📋 Descrição

Este projeto implementa uma estação meteorológica embarcada que monitora **temperatura**, **umidade** e **pressão atmosférica** utilizando sensores digitais. Os dados são exibidos localmente em um display OLED e disponibilizados via Wi-Fi em uma interface web responsiva, permitindo:

- Visualização em tempo real dos dados.
- Definição de limites mínimos e máximos para temperatura e umidade.
- Ajuste de offsets para calibração.
- Alertas visuais (LEDs e matriz) e sonoros (buzzer) em caso de anomalias.

## ⚙️ Funcionalidades

- 📡 Conexão Wi-Fi com servidor web embutido.
- 🌡️ Leitura contínua dos sensores AHT20 e BMP280.
- 🖥️ Interface web com gráficos dinâmicos (Chart.js) e formulários de configuração.
- 📟 Exibição local em display OLED SSD1306.
- 🚨 Alertas automáticos com LED RGB, matriz de LEDs e buzzer.
- 🔘 Navegação por botões físicos com lógica de debounce e modo BOOTSEL.

## 🧩 Componentes Utilizados

### 🔌 Hardware
- **Plataforma:** BitDogLab com RP2040
- **Sensor AHT20:** Temperatura e umidade
- **Sensor BMP280:** Pressão e temperatura
- **Display OLED SSD1306**
- **LEDs (vermelho, verde e matriz WS2812)**
- **Buzzer**
- **Botões físicos (botão A e B)**

### 🛠️ Software
- **Linguagem:** C
- **Bibliotecas:**
  - `pico/stdlib`, `hardware/i2c`, `lwip/tcp`
  - `aht20.h`, `bmp280.h`, `ssd1306.h`, `buzzer.h`, `matrizLed.h`
- **Front-end Web:**
  - HTML5, CSS3 responsivo
  - JavaScript + AJAX + Chart.js

## 🚀 Como Usar

### 🔧 Instalação e Configuração

1. Clone o repositório:
   ```bash
   git clone https://github.com/seu-usuario/nome-do-repositorio.git
   ```

2. Abra o projeto no seu ambiente com suporte ao SDK do Raspberry Pi Pico (ex: VSCode + CMake).

3. Conecte os componentes conforme os pinos definidos:
   - I2C sensores: SDA = GP0, SCL = GP1
   - I2C display: SDA = GP14, SCL = GP15
   - Matriz WS2812: GP7
   - LED RGB: GP12 e GP13
   - Buzzer: GP21
   - Botões: GP5 (A), GP6 (B)

4. Configure o Wi-Fi no código:
   ```c
   #define WIFI_SSID "SEU_WIFI"
   #define WIFI_PASS "SENHA_WIFI"
   ```

5. Compile e carregue o código para a placa RP2040.

## 🌐 Acessar Interface Web

- Após a conexão Wi-Fi, o display OLED exibirá o IP local.
- Acesse esse IP via navegador (PC ou celular).
- Utilize os formulários da interface para:
  - Definir limites de temperatura e umidade.
  - Ajustar os offsets de calibração.
- Veja os gráficos atualizados em tempo real.


## 📌 Observações

- Todos os componentes são utilizados com lógica de interrupções para maior precisão.
- A interface web é responsiva, compatível com dispositivos móveis.
- O código possui tratamento de debounce e lógica para evitar múltiplos disparos nos botões.
- Sistema com alertas automáticos visuais e sonoros caso os limites definidos sejam ultrapassados.

## 👨‍💻 Autor

**Matheus Nepomuceno Souza**  
BitDogLab | Projeto Individual - Estação Meteorológica