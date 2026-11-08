# IoT com ESP32 + RabbitMQ

Projeto de estudo de IoT usando **ESP32** como dispositivo e **RabbitMQ** como broker de mensagens (via protocolo MQTT).

## 📋 Sobre o projeto

O objetivo é aprender a integrar um microcontrolador ESP32 a um sistema de mensageria usando RabbitMQ. Como o ESP32 não fala AMQP nativamente, usamos o protocolo **MQTT** (suportado pelo RabbitMQ através do plugin `rabbitmq_mqtt`).

O primeiro experimento será **acender e apagar um LED** remotamente via mensagens MQTT.

## 📂 Estrutura do projeto

esp32_rabbitmq/
├── src/ # Firmware do ESP32 (PlatformIO)
│ └── main.cpp
├── docker/ # Infraestrutura do RabbitMQ
│ ├── docker-compose.yml
│ └── rabbitmq.conf
├── python/ # Scripts de teste (AMQP) — futuro
├── include/
├── lib/
├── test/
├── platformio.ini # Configuração do PlatformIO
└── README.md

text

- **`src/main.cpp`** — Firmware do ESP32 (Wi-Fi + MQTT via PubSubClient).
- **`docker/`** — Infraestrutura do RabbitMQ (Docker Compose + configuração).
- **`python/`** — (futuro) Scripts de teste via AMQP.
- **`platformio.ini`** — Configuração do projeto PlatformIO.

## 🛠️ Requisitos

- **ESP32** (qualquer modelo com Wi-Fi)
- **PlatformIO** (VS Code + extensão, ou CLI)
- **Docker** e **Docker Compose**
- **MQTTX** (CLI ou app desktop) — para testar publicação/consumo MQTT
- **Python 3** (opcional, para scripts AMQP)

## 🚀 Roadmap

- [x] Criar a estrutura de pastas do projeto
- [x] Subir o RabbitMQ com Docker (plugin MQTT habilitado)
- [x] Escrever o firmware do ESP32 (conexão Wi-Fi + MQTT)
- [x] Testar o controle remoto do LED
- [x] Publicar status do ESP32 de volta no broker (confirmação + heartbeat)
- [ ] Publicar dados de sensor do ESP32 no RabbitMQ
- [ ] (Extra) Criar dashboard para visualizar os dados

## 🔌 Arquitetura (visão geral)

```text
┌──────────┐                ┌──────────┐                ┌───────────┐
│  ESP32   │ ── MQTT:1883 ─►│ RabbitMQ │◄─ AMQP:5672 ── │ Aplicação │
│ (sensor) │◄─ (publish) ── │ (broker) │ ── (consume) ─►│  (Python) │
└──────────┘                └──────────┘                └───────────┘
```

- ESP32 ↔ RabbitMQ: **MQTT** (porta `1883`)
- Aplicações ↔ RabbitMQ: **AMQP** (porta `5672`) ou painel web (porta `15672`)

## 🚦 Como rodar

### 1. Subir o RabbitMQ

```bash
cd docker
docker compose up -d
```

Aguarde ~10s e acesse o painel: <http://localhost:15672>

- **Usuário:** `admin`
- **Senha:** `admin123`

### 2. Descobrir o IP da máquina na rede local

```powershell
ipconfig | Select-String "IPv4"
```

Use o IP do adaptador **Wi-Fi** (ex: `192.168.1.9`) no `mqtt_server` do `src/main.cpp`.

### 3. Gravar o firmware

```bash
pio run -t upload
pio device monitor
```

Saída esperada:

```text
WiFi conectado
IP: 192.168.1.x
Tentando conexao MQTT...conectado
```

### 4. Testar controle do LED

Com o ESP32 conectado e o monitor serial aberto:

```bash
# Acender
mqttx pub -h 192.168.1.9 -p 1883 -u admin -P admin123 \
  -t "casa/led/comando" -m "ligar"
```

```bash
# Apagar
mqttx pub -h 192.168.1.9 -p 1883 -u admin -P admin123 \
  -t "casa/led/comando" -m "desligar"
```

No monitor serial deve aparecer LED LIGADO / LED DESLIGADO e o LED do ESP32 (GPIO 2) reage.

## 📝 Anotações

### Comandos úteis

Verificar se o plugin MQTT está ativo:

```powershell
docker exec rabbitmq-iot rabbitmq-plugins list -e
```

Deve listar: `[E*] rabbitmq_mqtt` ✅

Verificar portas expostas:

```powershell
docker port rabbitmq-iot
# 1883/tcp -> 0.0.0.0:1883   (MQTT)
# 5672/tcp -> 0.0.0.0:5672   (AMQP)
# 15672/tcp -> 0.0.0.0:15672 (painel web)
```

Testar se o broker é acessível da rede local:

```powershell
Test-NetConnection -ComputerName 192.168.1.9 -Port 1883
```

Saída esperada:

```text
TcpTestSucceeded : True
```

## ⚠️ Pegadinhas aprendidas

- O RabbitMQ **não habilita** o plugin MQTT por padrão — precisa ser explicitamente ativado.
- O ESP32 **não fala AMQP** — a ponte é feita via MQTT.
- `mqtt_server` no firmware **não pode ser `localhost`** — precisa ser o IP da máquina na LAN.
- O plugin MQTT traduz `/` → `.` nas routing keys ao cruzar para AMQP (`casa/led/comando` → `casa.led.comando`).

## 📚 Referências

- [RabbitMQ MQTT Plugin](https://www.rabbitmq.com/mqtt.html)
- [PlatformIO — ESP32](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
- [PubSubClient (knolleary)](https://github.com/knolleary/pubsubclient)
- [MQTTX](https://mqttx.app)

---

## 🎯 O que mudou (resumo)

1. **`esp32/*.ino` → `src/main.cpp`** — você usa PlatformIO, não Arduino IDE
2. **Credenciais corrigidas** — `admin/admin123`, não `guest/guest`
3. **Adicionada seção "Como rodar"** — ordem completa do zero
4. **Adicionada seção "Pegadinhas aprendidas"** — o que você já descobriu na prática (isso vale ouro pra você no futuro)
5. **Referências com links reais**
6. **Nome da pasta corrigido** — `esp32_rabbitmq/`
7. **Removido "(futuro)"** de "subir o RabbitMQ" — já funciona
8. **Arquitetura ASCII com alinhamento melhor**

---

## 🤔 Sobre o "guest/guest" que ainda aparece

Você pode ter visto `guest/guest` em algum tutorial e mantido no README. Mas como você **configurou `admin/admin123` no `docker-compose.yml`**, o README precisa refletir isso — senão, quem clonar o projeto vai travar exatamente no ponto que **você já resolveu**.

---

## 💾 Como aplicar

1. Abra o `README.md` no VS Code
2. Substitua o conteúdo pela versão acima
3. Salve (`Ctrl+S`)
4. Commit:

```bash
git add README.md
git commit -m "docs: atualiza README com estado real do projeto e instruções"
```

Ficou bom assim? Se quiser, posso te ajudar no próximo passo — (A) ESP32 publicar status de volta ou (B) sensor DHT22. Qual prefere? 🚀
