/*
* ############################
* #   Réveil - Test Boutons  #
* ############################
* 
* Auteur:  MCHOBBY (Stefan)
* Réalisé le: 12/02/2018
* Version:  1.0
* 
* 
*/
#define boutonLuminosite    2  // digital
#define boutonAlarmeControl 3  // digital
#define boutonPlus          9  // digital
#define boutonMoins         10 // digital
#define piezoBuzzer         13 // digital
#define boutonOk            14 // analog 0
#define boutonSnooze        15 // analog 1

#define ledBoutonOK         16 // analog 2
#define ledBoutonSnooze     17 // analog 3


const int BOUTONALARME[] = {8, 7, 5, 4};  // Quelles pin pour chaques alarmes
const int NBRALARMES = sizeof( BOUTONALARME ) / sizeof( int );  // Combien d'alarmes

void setup() {
  // Communication RS232
  Serial.begin( 9600 );
  
  // Flasher l'EEPROM
  eepromConfiguration();
  
  // Initialisation des boutons
  pinMode( boutonOk, INPUT_PULLUP );
  pinMode( boutonMoins, INPUT_PULLUP );
  pinMode( boutonPlus, INPUT_PULLUP );
  pinMode( boutonLuminosite, INPUT_PULLUP );
  pinMode( boutonAlarmeControl, INPUT_PULLUP );
  pinMode( boutonSnooze, INPUT_PULLUP );
  for(int i=0 ; i<NBRALARMES ;i++)
    pinMode( BOUTONALARME[i], INPUT_PULLUP );

  // Initialisation des leds et du piezo
  pinMode( ledBoutonOK, OUTPUT );
  pinMode( ledBoutonSnooze, OUTPUT );
  pinMode( piezoBuzzer, OUTPUT );
}

void loop(){
  // Test bouton ok
  Serial.println("Appuyiez sur le bouton 'OK'");
  while(!estAppuye(boutonOk));
  // Test bouton snooze
  Serial.println("Appuyiez sur le bouton 'SNOOZE'");
  while(!estAppuye(boutonSnooze));
  // Test bouton plus
  Serial.println("Appuyiez sur le bouton 'Plus'");
  while(!estAppuye(boutonPlus));
  // Test bouton moins
  Serial.println("Appuyiez sur le bouton 'Moins'");
  while(!estAppuye(boutonMoins));
  // Test bouton luminosité
  Serial.println("Appuyiez sur le bouton 'Luminosité'");
  while(!estAppuye(boutonLuminosite));
  // Test bouton alarme contrôle
  Serial.println("Appuyiez sur le bouton 'Alarmes Contrôles'");
  while(!estAppuye(boutonAlarmeControl));
  // Test bouton activer alarmes
  for(int i=0; i<NBRALARMES ; i++){
    Serial.print("Appuyiez sur le bouton 'Alarmes Actiavation ");
    Serial.print(i+1);
    Serial.println("'");
    while(!estAppuye(BOUTONALARME[i]));
  }
  // Test led ok
  Serial.println("Appuyiez sur le bouton 'Ok', si vous voyiez la led du bouton ok allumée");
  digitalWrite(ledBoutonOK ,HIGH);
  while(!estAppuye(boutonOk));
  digitalWrite(ledBoutonOK ,LOW);
  // Test led Snooze
  Serial.println("Appuyiez sur le bouton 'Snooze', si vous voyiez la led du bouton snooze allumée");
  digitalWrite(ledBoutonSnooze ,HIGH);
  while(!estAppuye(boutonSnooze));
  digitalWrite(ledBoutonSnooze ,LOW);
  // Test piezzo buzzer
  Serial.println("Entendez-vous le piezo sonner ?");
  tone(piezoBuzzer, 1000);
  while(!estAppuye(boutonOk));
  noTone(piezoBuzzer);
  Serial.println("Tout est bon !!!");
  delay(1000);
  return;
}
/*
 * Vérifie si le bouton est appuyé ou non
 */
boolean estAppuye(int pinAppuye){
  int val1 = digitalRead(pinAppuye);     
  delay(10);                         
  int val2 = digitalRead(pinAppuye);     
  if (val1 == val2 && val1 == 0) 
    return true;
  else 
    return false;
}   
/*
* Formatter l'EEPROM
*/
void eepromConfiguration(){
    
    // Formatter l'EEPROM
    for (int i = 0 ; i < EEPROM.length() ; i++) 
       EEPROM.write(i, 0);
    Serial.println("EEPROM formatté !");
    
    // Valeur par défaut pour savoir si l'EEPROM est formatté
    EEPROM.write(0, 255); 
    Serial.println("Valeur par défaut définie !");

    // Définir une version
    EEPROM.write(1, 10); 
    Serial.println("Version définie !");
    
    // Définir une luminosité 
    EEPROM.write(2, 15); 
    Serial.println("Valeur par défaut pour la luminosité est définie !");
}
