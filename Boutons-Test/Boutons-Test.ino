/*
* ############################
* #   Réveil - Test Boutons  #
* ############################
* 
* Auteur:  MCHOBBY (Stefan G)
* Réalisé à partir du: 12/02/2018
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


const int BOUTON_ALARME[] = {8, 7, 9, 10};  // Quelles pin pour chaques alarmes
const int NBRALARMES = sizeof( BOUTONALARME ) / sizeof( int );  // Combien d'alarmes

void setup() {
  // Communication RS232
  Serial.begin( 9600 );
  
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
  Serial.println("Appuyez sur le bouton 'OK'");
  while(!estAppuye(boutonOk));
  // Test bouton snooze
  Serial.println("Appuyez sur le bouton 'SNOOZE'");
  while(!estAppuye(boutonSnooze));
  // Test bouton plus
  Serial.println("Appuyez sur le bouton 'Plus'");
  while(!estAppuye(boutonPlus));
  // Test bouton moins
  Serial.println("Appuyez sur le bouton 'Moins'");
  while(!estAppuye(boutonMoins));
  // Test bouton luminosité
  Serial.println("Appuyez sur le bouton 'Luminosité'");
  while(!estAppuye(boutonLuminosite));
  // Test bouton alarme contrôle
  Serial.println("Appuyez sur le bouton 'Alarmes Contrôles'");
  while(!estAppuye(boutonAlarmeControl));
  // Test bouton activer alarmes
  for(int i=0; i<NBRALARMES ; i++){
    Serial.print("Appuyez sur le bouton 'Alarmes Actiavation ");
    Serial.print(i+1);
    Serial.println("'");
    while(!estAppuye(BOUTONALARME[i]));
  }
  // Test led ok
  Serial.println("Appuyez sur le bouton 'Ok', si vous voyez la led du bouton ok allumée");
  digitalWrite(ledBoutonOK ,HIGH);
  while(!estAppuye(boutonOk));
  digitalWrite(ledBoutonOK ,LOW);
  // Test led Snooze
  Serial.println("Appuyez sur le bouton 'Snooze', si vous voyez la led du bouton snooze allumée");
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
