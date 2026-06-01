/*
 * Generador de tren de pulsos con duty cycle fijo al 50%.
 * Salida: pin 9 (OC1A).
 * Control de frecuencia por consola serial (baudrate 115200).
 *
 * Rango soportado: ~0.24 Hz a 8 MHz (limitado por Timer1 de 16 bits).
 */

#define OUT_PIN  9
#define REF_PIN  8   // 5V fijo para verificar el divisor resistivo

static const struct {
    uint8_t  cs_bits;   // CS12:CS10 para TCCR1B
    uint32_t prescaler;
} PRESCALERS[] = {
    {0b001,     1},
    {0b010,     8},
    {0b011,    64},
    {0b100,   256},
    {0b101,  1024},
};

static uint32_t current_freq = 0;

// Devuelve false si la frecuencia está fuera de rango.
static bool set_frequency(uint32_t freq_hz) {
    if (freq_hz == 0) return false;

    for (uint8_t i = 0; i < 5; i++) {
        uint32_t ocr = F_CPU / (2UL * PRESCALERS[i].prescaler * freq_hz);
        if (ocr == 0) continue;          // frecuencia demasiado alta para este prescaler
        if (ocr > 65536UL) continue;     // desborda el registro de 16 bits

        // Modo CTC, toggle OC1A en compare match → duty cycle exacto 50 %
        TCCR1A = (1 << COM1A0);          // toggle OC1A, WGM11:WGM10 = 00
        TCCR1B = (1 << WGM12) | PRESCALERS[i].cs_bits;  // CTC (WGM13:WGM12 = 01)
        OCR1A  = (uint16_t)(ocr - 1);
        TCNT1  = 0;

        current_freq = freq_hz;
        return true;
    }
    return false;
}

static void print_status() {
    if (current_freq == 0) {
        Serial.println("Estado: detenido");
    } else {
        Serial.print("Frecuencia actual: ");
        Serial.print(current_freq);
        Serial.println(" Hz  |  Duty cycle: 50 %  |  Salida: pin 9");
    }
}

static void print_help() {
    Serial.println("=== Generador de pulsos (duty 50%) ===");
    Serial.println("Comandos:");
    Serial.println("  <numero>   Fijar frecuencia en Hz  (ej: 1000)");
    Serial.println("  +          Duplicar frecuencia");
    Serial.println("  -          Dividir frecuencia a la mitad");
    Serial.println("  s          Detener la salida");
    Serial.println("  ?          Mostrar esta ayuda");
    Serial.println("Rango valido: 1 Hz – 8000000 Hz");
}

void setup() {
    Serial.begin(115200);
    pinMode(OUT_PIN, OUTPUT);
    pinMode(REF_PIN, OUTPUT);
    digitalWrite(REF_PIN, HIGH);

    // Timer1 apagado hasta recibir un comando
    TCCR1A = 0;
    TCCR1B = 0;

    print_help();
    Serial.println();
}

void loop() {
    if (!Serial.available()) return;

    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    char cmd = input.charAt(0);

    if (cmd == '?') {
        print_help();

    } else if (cmd == 's' || cmd == 'S') {
        TCCR1B = 0;          // detiene el timer
        TCCR1A = 0;
        digitalWrite(OUT_PIN, LOW);
        current_freq = 0;
        Serial.println("Salida detenida.");

    } else if (cmd == '+') {
        if (current_freq == 0) {
            Serial.println("Error: no hay frecuencia activa. Ingrese un valor primero.");
        } else if (!set_frequency(current_freq * 2)) {
            Serial.println("Error: frecuencia fuera de rango.");
        } else {
            print_status();
        }

    } else if (cmd == '-') {
        if (current_freq == 0) {
            Serial.println("Error: no hay frecuencia activa. Ingrese un valor primero.");
        } else if (!set_frequency(current_freq / 2)) {
            Serial.println("Error: frecuencia fuera de rango.");
        } else {
            print_status();
        }

    } else if (isDigit(cmd)) {
        uint32_t freq = (uint32_t)input.toInt();
        if (!set_frequency(freq)) {
            Serial.print("Error: ");
            Serial.print(freq);
            Serial.println(" Hz fuera de rango (1 Hz – 8 MHz).");
        } else {
            print_status();
        }

    } else {
        Serial.print("Comando desconocido: '");
        Serial.print(input);
        Serial.println("'  (escriba ? para ayuda)");
    }
}
