#include "arduino_secrets.h"
#include "thingProperties.h"

// Definizione pin per ESP32 Lolin S2 Mini Slave
const uint8_t ANALOG_IN_PIN = 8;   // Pin di ingresso analogico (A0)
const uint8_t PIEZO_PIN = 3;       // Pin per il piezo

// Soglia per l'attivazione del piezo
const int ACTIVATION_THRESHOLD = 128;  // Metà del valore massimo (0-255)

// Variabili per il controllo del suono
unsigned long piezoStartTime = 0;
bool alarmActive = false;
bool alarmTriggered = false;  // Flag per evitare attivazioni multiple

void setup() {
  // Inizializzazione comunicazione seriale
  Serial.begin(9600);
  delay(1500); 
  
  // Definito in thingProperties.h
  initProperties();
  // Connessione a Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  
  Serial.println("Avvio del sistema di irrigazione - SLAVE");
  
  // Configurazione dei pin
  pinMode(ANALOG_IN_PIN, INPUT);
  pinMode(PIEZO_PIN, OUTPUT);
  
  // Inizializzazione stato iniziale del piezo
  digitalWrite(PIEZO_PIN, LOW);
}

void loop() {
  // Aggiorna la connessione a Arduino Cloud
  ArduinoCloud.update();
  
  // Leggi il valore analogico dal master
  int analogValue = analogRead(ANALOG_IN_PIN);
  
  // Converti la lettura analogica (0-4095) a un valore 0-255
  int scaledValue = map(analogValue, 0, 4095, 0, 255);
  
  // Log dei valori ogni 5 secondi
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime >= 5000) {
    Serial.print("Analog: ");
    Serial.print(analogValue);
    Serial.print(", Allarme: ");
    Serial.println(alarmActive ? "Si" : "No");
    lastDebugTime = millis();
  }
  
  // Controlla se il valore supera la soglia
  if (scaledValue > ACTIVATION_THRESHOLD) {
    if (!alarmActive && !alarmTriggered) {
      // Attiva l'allarme solo se non è già attivo
      alarmActive = true;
      alarmTriggered = true;
      piezoStartTime = millis();
      Serial.println("ALLARME: Livello acqua basso");
      
      // Aggiorna la variabile Arduino Cloud
      livello = false;
    }
  } else {
    // Livello acqua tornato OK
    if (!livello) {
      livello = true;
      Serial.println("Livello acqua OK");
    }
    
    // Resetta il flag di attivazione quando il livello è tornato normale
    alarmTriggered = false;
  }
  
  // Gestione del piezo quando l'allarme è attivo
  if (alarmActive) {
    unsigned long elapsedTime = millis() - piezoStartTime;
    
    if (elapsedTime < 5000) {
      // Sequenza di beep
      bool shouldBeOn = (elapsedTime % 200 < 100);
      digitalWrite(PIEZO_PIN, shouldBeOn ? HIGH : LOW);
    } else {
      // Terminato il ciclo di 5 secondi
      digitalWrite(PIEZO_PIN, LOW);
      alarmActive = false;
    }
  }
  
  // Comando di test via seriale
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 't') {
      // Attiva un test manuale del piezo
      Serial.println("Test piezo attivato");
      digitalWrite(PIEZO_PIN, HIGH);
      delay(2000);
      digitalWrite(PIEZO_PIN, LOW);
    } else if (cmd == 'r') {
      // Resetta i flag di allarme
      alarmActive = false;
      alarmTriggered = false;
      Serial.println("Flag di allarme resettati");
    }
  }
  
  delay(10); // Piccolo ritardo per la stabilità
}

void onLivelloChange() {
  //codice da eseguire per accendere la pompa sulla master ...
  if (livello) {
    Serial.println("Livello è stato acceso tramite Alexa!");
    
  }
}