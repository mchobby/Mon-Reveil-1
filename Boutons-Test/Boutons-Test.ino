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
#define BOUTON_LUMINOSITE    2  // digital
#define BOUTON_ALARME_CONTROL 3  // digital
#define BOUTON_PLUS          9  // digital
#define BOUTON_MOINS         10 // digital
#define PIEZO_BUZZER         13 // digital
#define BOUTON_OK            14 // analog 0
#define BOUTON_SNOOZE        15 // analog 1

#define LED_BOUTON_OK         16 // analog 2
#define LED_BOUTON_SNOOZE     17 // analog 3


const int BOUTONALARME[] = {8, 7, 5, 4};  // Quelles pin pour chaques alarmes
const int NBRALARMES = sizeof( BOUTONALARME ) / sizeof( int );  // Combien d'alarmes

void setup() {
  // Communication RS232
  Serial.begin( 9600 );
  
  // Initialisation des boutons
  pinMode( BOUTON_OK, INPUT_PULLUP );
  pinMode( BOUTON_MOINS, INPUT_PULLUP );
  pinMode( BOUTON_PLUS, INPUT_PULLUP );
  pinMode( BOUTON_LUMINOSITE, INPUT_PULLUP );
  pinMode( BOUTON_ALARME_CONTROL, INPUT_PULLUP );
  pinMode( BOUTON_SNOOZE, INPUT_PULLUP );
  for(int i=0 ; i<NBRALARMES ;i++)
    pinMode( BOUTONALARME[i], INPUT_PULLUP );

  // Initialisation des leds et du piezo
  pinMode( LED_BOUTON_OK, OUTPUT );
  pinMode( LED_BOUTON_SNOOZE, OUTPUT );
  pinMode( PIEZO_BUZZER, OUTPUT );
}

void loop(){
  // Test bouton ok
  Serial.println("Appuyiez sur le bouton 'OK'");
  while(!estAppuye(BOUTON_OK));
  // Test bouton snooze
  Serial.println("Appuyez sur le bouton 'SNOOZE'");
  while(!estAppuye(BOUTON_SNOOZE));
  // Test bouton plus
  Serial.println("Appuyez sur le bouton 'Plus'");
  while(!estAppuye(BOUTON_PLUS));
  // Test bouton moins
  Serial.println("Appuyez sur le bouton 'Moins'");
  while(!estAppuye(BOUTON_MOINS));
  // Test bouton luminosité
  Serial.println("Appuyez sur le bouton 'Luminosité'");
  while(!estAppuye(BOUTON_LUMINOSITE));
  // Test bouton alarme contrôle
  Serial.println("Appuyez sur le bouton 'Alarmes Contrôles'");
  while(!estAppuye(BOUTON_ALARME_CONTROL));
  // Test bouton activer alarmes
  for(int i=0; i<NBRALARMES ; i++){
    Serial.print("Appuyez sur le bouton 'Alarmes Actiavation ");
    Serial.print(i+1);
    Serial.println("'");
    while(!estAppuye(BOUTONALARME[i]));
  }
  // Test led ok
  Serial.println("Appuyez sur le bouton 'Ok', si vous voyez la led du bouton ok allumée");
  digitalWrite(LED_BOUTON_OK ,HIGH);
  while(!estAppuye(BOUTON_OK));
  digitalWrite(LED_BOUTON_OK ,LOW);
  // Test led Snooze
  Serial.println("Appuyez sur le bouton 'Snooze', si vous voyez la led du bouton snooze allumée");
  digitalWrite(LED_BOUTON_SNOOZE ,HIGH);
  while(!estAppuye(BOUTON_SNOOZE));
  digitalWrite(LED_BOUTON_SNOOZE ,LOW);
  // Test piezzo buzzer
  Serial.println("Entendez-vous le piezo sonner ?");
  tone(PIEZO_BUZZER, 1000);
  while(!estAppuye(BOUTON_OK));
  noTone(PIEZO_BUZZER);
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
