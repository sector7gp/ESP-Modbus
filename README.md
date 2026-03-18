# Control de Motor vía Modbus con ESP32-C3

Este proyecto permite controlar un driver de motor mediante protocolo Modbus RTU utilizando un microcontrolador ESP32-C3 SuperMini. Dispone de entradas digitales para controles físicos y una interfaz web en red local configurada como Punto de Acceso (AP).

## 🔌 Conexión ESP32-C3 + MAX485 (Modbus RTU)
El módulo MAX485 (TTL ↔ RS485) toma el rol vital de convertir la comunicación UART del ESP32 en una señal diferencial robusta para el bus RS485.

### 📐 Conexiones

🔹 **ESP32-C3 → MAX485**

| ESP32-C3 | MAX485 | Descripción |
| :--- | :--- | :--- |
| **3.3V** | **VCC** | Alimentación |
| **GND** | **GND** | Tierra |
| **TX (GPIO 21)** | **DI** | Datos a transmitir |
| **RX (GPIO 20)** | **RO** | Datos recibidos |
| **GPIO 10** | **DE + RE** | Control dirección |

**🔁 Punto CLAVE: DE y RE**
En el módulo MAX485:
- **DE** (Driver Enable) → Habilita la transmisión.
- **RE** (Receiver Enable) → Habilita la recepción (activo en LOW, es decir su valor lógico está negado).

👉 **Lo que se hace en la práctica:**
Se unen físicamente los pines DE y RE.
```text
DE ─┬─ GPIO 10 (PIN_DE_RE)
    └─ RE
```

**🔄 Lógica de funcionamiento (Control Direccional)**
| GPIO 10 | Modo del MAX485 |
| :--- | :--- |
| **LOW** | Recepción (`Serial1.read`) |
| **HIGH** | Transmisión (`Serial1.write`) |

En nuestro código, el control `HIGH`/`LOW` de este pin lo maneja automáticamente la librería `ModbusMaster` gracias a las sentencias `node.preTransmission()` y `node.postTransmission()` en `src/main.cpp`.

🔹 **Conexión al driver (RS485)**
| MAX485 | Driver Motor |
| :--- | :--- |
| **A** | **A** (Data +) |
| **B** | **B** (Data -) |

> 👉 **Si el sistema no comunica:** Prueba **invertir** A y B en los terminales del driver. Es el típico problema RS-485.

### 📊 Diagrama de conexión completo
```text
        +-------------------+
        |     ESP32-C3      |
        |                   |
        |   TX (21) --> DI  |
        |   RX (20) <-- RO  |
        |   GPIO 10 --> DE/RE
        +--------|----------+
                 |
          +------v------+
          |   MAX485    |
          |             |
          |   A -------+------------------> A (Driver)
          |   B -------+------------------> B (Driver)
          +-------------+
```

### ⚠️ Alimentación (MUY importante)
Muchos módulos genéricos MAX485 dicen 5V, pero en la práctica pueden funcionar con 3.3V, o bien requieren de manera estricta que les des los 5V. 
👉 **Lo ideal:**
- Si el módulo tiene un regulador y permite lógica 3.3V, aliméntalo con la salida de 3.3V del ESP32.
- **⚠️ Si el módulo es estrictamente de 5V pero la lógica TTL de tu ESP32 maneja 3.3V:** Podrías requerir un _Level Shifter_ (conversor de nivel lógico) en las líneas TX y RX para proteger el chip ESP32 (los pines del C3 toleran en general niveles hasta la alimentación, 3.3V, y 5V podría quemarlos, especialmente por la vía desde `RO` al ESP32). Revisa las especificaciones exactas de la versión de hardware MAX485 que tengas en mano.

### ⚙️ Configuración UART en Firmware
En `src/main.cpp` configuramos el bus serial a través de `Serial1` usando los pines específicos del C3:
```cpp
Serial1.begin(modbusBaudrate, SERIAL_8N1, PIN_RX, PIN_TX);
```

### ⚠️ Buenas prácticas
1. **Terminación (si el cable es largo):** Resistencia 120Ω entre las borneras A y B en el extremo de la línea.
2. **Masa común:** Siempre conectar GND del circuito ESP32+MAX485 con el borne GND señalizado del Driver Motor, para establecer un potencial de referencia idéntico.
3. **Cable:** Usar cable de par trenzado (ej. UTP Cat5/6 para red) para A y B, esto reduce profundamente el ruido inducido.

---

### 🚀 Resumen corto para un rápido ensamble

✔️ **TX (21)** → DI  
✔️ **RX (20)** → RO  
✔️ **GPIO (10)** → DE + RE (juntos)  
✔️ **A/B** → Borneras A/B driver  
✔️ **HIGH** = transmite / **LOW** = recibe (Lo gestiona el software automáticamente)
