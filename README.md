# Estación IoT de Monitorización de Temperatura

Proyecto de estación autónoma basada en **ESP32 + SIM7600** que mide temperatura y envía los datos a **Google Sheets mediante LTE**.

El sistema está diseñado para funcionar durante largos periodos con **baterías 18650** utilizando **deep sleep**.

---

# Hardware

Elementos principales utilizados:

- ESP32
- Módulo LTE **SIM7600**
- Sensor **PT100** con **MAX31865**
- Sensor **LM35** (temperatura interna)
- Baterías **18650**
- Circuito de carga **MCP73871**
- Load switch **TPS22918**
- PCB diseñada en **KiCad**

---

# Funcionamiento

El sistema funciona de la siguiente manera:

1. El ESP32 se despierta desde **deep sleep**
2. Enciende el **SIM7600**
3. Se conecta a la red LTE
4. Sincroniza la hora mediante **NTP**
5. Lee los sensores
6. Envía los datos a **Google Sheets**
7. Apaga el módem
8. Vuelve a **deep sleep**

Actualmente el sistema envía datos a:

- **08:45**
- **13:00**

---

# Firmware

El firmware está desarrollado en **C++ usando PlatformIO**.

Características:

- Comunicación LTE mediante comandos AT
- Sincronización de hora con NTP
- Lectura de sensores
- Envío HTTP a Google Sheets
- Gestión de batería
- Deep sleep

El código se encuentra en:

Firmware/EstacionIot/src/main.cpp


---

# Esquema eléctrico

El esquema del sistema se puede consultar aquí:

[Esquema del sistema](Hardware/Proyecto/Esquema_V1.pdf)

---

# PCB

La PCB fue diseñada con **KiCad**.

Los archivos del proyecto se encuentran en:


Hardware/Proyecto


---

# Autor

Proyecto desarrollado por:

**Jonathan Arizala**
