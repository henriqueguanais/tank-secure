# Tank Secure - Monitoramento de Reservatório com RFID e ThingSpeak

## 📌 Sobre o Projeto

O **Tank Secure** é um sistema de monitoramento do nível de um reservatório e sua temperatura, utilizando a **Raspberry Pi Pico W**. O acesso às informações exibidas no **display OLED** é protegido por **autenticação via RFID**.

Os dados do nível do reservatório, temperatura e os usuários autenticados são enviados periodicamente para um canal do **ThingSpeak**.

## 🚀 Como Rodar o Projeto

### **1️⃣ Clone o Repositório**
```sh
    git clone https://github.com/henriqueguanais/tank-secure.git
    cd tank-secure
```

### **2️⃣ Configure o Ambiente**
Certifique-se de que você tem a **SDK do Raspberry Pi Pico** configurada e compilada corretamente. Para isso, siga as instruções oficiais:

[Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

### **3️⃣ Compile e Faça o Upload**
```sh
    mkdir build && cd build
    cmake ..
    make
    picotool load firmware.uf2
```

### **4️⃣ Conecte-se à Rede Wi-Fi**
O dispositivo precisa estar conectado à internet para enviar os dados ao **ThingSpeak**. Certifique-se de configurar corretamente as credenciais no código.

## 📡 ThingSpeak - Canal de Dados

Os dados são enviados para o canal **ID: 2836789** no **ThingSpeak**, onde você pode visualizar:
- **Nível do reservatório**
- **Temperatura do ambiente**
- **IDs dos usuários autenticados**

## 🔌 Conexões dos Componentes

### **📍 RFID RC522**
| Sinal | Pino Raspberry Pi Pico W |
|-------|------------------------|
| SDA   | GP17                   |
| SCK   | GP18                   |
| MOSI  | GP19                   |
| MISO  | GP16                   |
| RST   | GP20                   |
| IRQ   | Não conectado          |

### **📍 Outros Componentes**
| Componente | Pino Raspberry Pi Pico W |
|------------|------------------------|
| Buzzer     | GP21                   |
| LED Vermelho | GP13                 |
| LED Verde  | GP11                   |
| Botão      | GP5                    |
| OLED 128x64 I2C SDA | GP14          |
| OLED 128x64 I2C SCL | GP15          |

## 🔔 Indicação Visual e Sonora

Para melhorar a experiência do usuário, o projeto conta com:
- **LED RGB** para indicar o status da autenticação
- **Buzzer** para alertas sonoros
- **Botão** de controle adicional

---
