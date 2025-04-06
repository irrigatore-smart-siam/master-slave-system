#include "arduino_secrets.h"
#include "thingProperties.h"

const uint8_t ANALOG_OUT_PIN = 7;
bool comandoInviato = false;
int statoLivelloPrecedente = 0;
bool primaEsecuzione = true;

// Timer per l'auto-reset del CloudSwitch
unsigned long tempoInvioComando = 0;
const unsigned long TEMPO_AUTO_RESET = 5000; // 5 secondi dopo l'invio del comando

// Utilizzare un approccio hardware - La disconnessione fisica forzata
const uint8_t PIN_HARDWARE_RESET = 5; // Pin per forzare un reset hardware se necessario

void setup() {
  Serial.begin(9600);
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  pinMode(ANALOG_OUT_PIN, OUTPUT);
  analogWrite(ANALOG_OUT_PIN, 0);
  pinMode(PIN_HARDWARE_RESET, OUTPUT);
  digitalWrite(PIN_HARDWARE_RESET, HIGH); // Non attivo
  
  Serial.println("Slave LoLin S2 Mini - Sistema avviato con reset smart");
  
  // All'avvio, assicurati che livello sia 0
  if (livello != 0) {
    livello = 0;
  }
}

void loop() {
  ArduinoCloud.update();
  
  // Gestiamo la prima esecuzione per evitare invii non voluti all'avvio
  if (primaEsecuzione) {
    statoLivelloPrecedente = livello;
    primaEsecuzione = false;
    Serial.println("Inizializzazione completata, stato livello iniziale: " + String(livello));
    // Forza immediatamente un controllo del valore
    if (livello != 0) {
      livello = 0;
      ArduinoCloud.update();
      Serial.println("Livello forzato a 0 all'avvio");
    }
  }
  
  // Controllo timer per auto-reset del livello
  if (comandoInviato && millis() - tempoInvioComando > TEMPO_AUTO_RESET) {
    Serial.println("AUTO-RESET: Trascorsi " + String(TEMPO_AUTO_RESET/1000) + " secondi, tentativo di reset Smart");
    
    // NON usare solo livello = 0, poiché questo potrebbe non sincronizzarsi
    Serial.println("Reimpostazione di livello usando il ciclo completo di sincronizzazione...");
    
    // Imposta a 0 e forza un update
    livello = 0;
    ArduinoCloud.update();
    delay(100);
    ArduinoCloud.update();
    
    // Forzare l'aggiornamento usando un pulsante dell'Arduino IoT Cloud
    Serial.println("Simulo cambio valore per forzare sincronizzazione");
    // Simula cambiamenti di valore per forzare la sincronizzazione
    livello = 1;
    ArduinoCloud.update();
    delay(100);
    livello = 0;
    ArduinoCloud.update();
    delay(100);
    
    // Reset completo dello stato
    comandoInviato = false;
    Serial.println("Reset Smart completato");
  }
  
  delay(10);
}

void onLivelloChange() {
  Serial.print("onLivelloChange chiamata - Livello attuale: ");
  Serial.print(livello);
  Serial.print(", Livello precedente: ");
  Serial.println(statoLivelloPrecedente);
  
  // Invia il comando solo se c'è un cambiamento effettivo da 0 a 1
  if (livello == 1 && statoLivelloPrecedente == 0 && !comandoInviato) {
    Serial.println("RILEVATO CAMBIO DA 0 A 1 - COMANDO ATTIVAZIONE POMPA INVIATO");
    
    // Imposta il flag per evitare invii multipli
    comandoInviato = true;
    tempoInvioComando = millis();  // Memorizza il tempo di invio per l'auto-reset
    
    // Invia un singolo impulso
    analogWrite(ANALOG_OUT_PIN, 255);
    delay(300);
    analogWrite(ANALOG_OUT_PIN, 0);
    
    Serial.println("Comando inviato una sola volta - Auto-reset programmato tra " + 
                   String(TEMPO_AUTO_RESET/1000) + " secondi");
  }
  else if (livello == 0 && statoLivelloPrecedente == 1) {
    // Resetta il flag quando torniamo a livello 0
    comandoInviato = false;
    Serial.println("Livello tornato a 0 - Flag reset");
  }
  
  // Aggiorna lo stato precedente di livello
  statoLivelloPrecedente = livello;
}