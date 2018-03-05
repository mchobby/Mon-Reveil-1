/*
* ##############
* #   Réveil   #
* ##############
* 
* Auteur:  MCHOBBY (Stefan)
* Version:  1.0
*/
#include "Adafruit_LEDBackpack.h"
#include <RTClib.h>
#include <EEPROM.h>
#include "pitches.h"  // Notes de musique
#include "melodies.h" // Playlists de musiques

#define boutonLuminosite    2  // digital
#define boutonAlarmeControl 3  // digital
#define boutonPlus          9  // digital
#define boutonMoins         10 // digital
#define piezoBuzzer         13 // digital
#define boutonOk            14 // analog 0
#define boutonSnooze        15 // analog 1

#define ledBoutonOK         16 // analog 2
#define ledBoutonSnooze     17 // analog 3

#define ACTION_STOP         1
#define ACTION_SNOOZE       2

// ================================================
// ================= A configurer =================
// ================================================
const int SNOOZEATTENTE     = 10;               // Durant combien de minutes l'utilisateur va t'il encore dormir ? (en secondes)
const int DUREEALARME       = 20;               // Durant combien de temps l'alarme va t'elle sonner (en secondes) 
const int BOUTONALARME[]    = {8, 7, 5, 4};     // Quelles pin pour chaques alarmes
const float vitesseLecture  = 1;                // Vitesse sonore des alarmes (par défaut 1)
const int MELODIE[][2]      = ALARM1;           // Sélectionner la musique que vous désirez pour vos alarmes
// ================================================


// ### Alarmes ###
struct alarme{
    DateTime heureSonne;          // A quelle heure l'alarme sonnera-t'elle ?
    DateTime heureStop=0;           // A quelle heure l'alarme va t'elle s'arrêter automatiquement si elle est pas coupé
    DateTime snoozeSonne=0;         // L'heure ou le snooze sonnera
    boolean programme;            // L'alarme a-t'elle été programmée par l'utilisateur ?
    boolean sonne = false;        // L'alarme est-elle en train de sonner ?
};
const int NBRALARMES = sizeof( BOUTONALARME ) / sizeof( int );  // Combien d'alarmes
struct alarme alarme[NBRALARMES];            // Déclaration de la structure
byte alarmeDerniereAction = ACTION_STOP;   // Dernière action effectué sur l'alarme

// ### Séparateur ###
boolean separateur = false;      // 2 points au milieu des afficheurs
unsigned long avantClignote = 0; // Savoir la dernière fois que les 2 points on changé d'état

// ### Mélodie ###
unsigned long notePrecedente = 0;
int notePosition = 0;
// ### Autres ###
Adafruit_7segment afficheurs = Adafruit_7segment(); // Initialisation de l'afficheur 7 segments
RTC_DS1307 rtc = RTC_DS1307();                      // Initialisation de l'rtc
int version = 10; // Version 1.0
// ================================================

/*
 *  Configuration avant lancement de la routine
 */
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
  
  // Addresse I2C des afficheurs 
  afficheurs.begin( 0x70 );

  // Démarrer le lien avec l'RTC en I2C
  rtc.begin();
  
  // Configuration de l'heure par l'utilisateur si ce n'a pas été encore fait
  if ( !rtc.isrunning() ){
    int h = 0;
    int m = 0;
    changerHeureVisuel( &h, &m );
    rtc.adjust( DateTime(2018, 2, 20, h, m, 0) ); // Change l'heure de l'RTC 
  }
  
  // Formattage de l'EEPROM si rien dedans (vérification si égale à 255) et si la version est égale à 1 (1.0)
  if( EEPROM.read(0) != 255 && EEPROM.read(1) != version )
    eepromConfiguration();

  // Définir la luminosité des afficheurs
  afficheurs.setBrightness( EEPROM.read(2) );

  
  // Configuration des dates/heures sur les alarmes
  DateTime maintenant = rtc.now();
  int j = 0;
  for( int i=3 ; i<(NBRALARMES*2)+6 ; i+=3 ){
    alarme[j].heureSonne = DateTime( maintenant.year(), maintenant.month(), maintenant.day(), EEPROM.read(i), EEPROM.read(i+1), 0);
    alarme[j].programme = EEPROM.read(i+2);
    j++;
  }
  
  //   affichageEEpromContenu();
}

/*
 * Routine du code
 */
void loop() {
  // ============================================================== 
  // ================ L'utilisateur peut intéragir ================ 
  // ============================================================== 
  // Changer la luminosité
  changeLuminosite();
  // Changer heure
  changerHeure();
  // Changer les alarmes
  changerHeureAlarme();
  // On active/désactive une alarme ?
  changerEtatAlarmes();
  // Arrêter l'alarme ou la mettre en snooze
  controlerAlarme();
  
  // ============================================================== 
  // =================== Affichage de l'heure ===================== 
  // ============================================================== 
  // Affichage de l'heure
  afficherTemps();
  // Afficher ou pas le séparateur (2 points)
  afficherSeparateur();
  
  // ============================================================== 
  // ============== Gestion automatique de l'alarme ===============
  // ============================================================== 
  // Vérifie si une alarme sonne
  alarmeGestionAutomatique();
}
/*
 * Est-ce qu'on joue la mélodie ?
 */
void jouerMelodie(){
    if(prochaineNote()){

      int dureeNote = 2000 / MELODIE[notePosition][1];
      tone(piezoBuzzer,MELODIE[notePosition][0], dureeNote * vitesseLecture); 
      Serial.print(MELODIE[notePosition][0]);
      Serial.print("-");
      Serial.println(MELODIE[notePosition][1]);
      
      int nbrLignesTableauMelodie = (sizeof( MELODIE ) / sizeof( int ) / 2) -1;
      if( notePosition >= nbrLignesTableauMelodie )
        notePosition=0;
      else
        notePosition++;
    }   
}
/*
 * Est-ce qu'on joue la prochaine note de la mélodie ?
 */
boolean prochaineNote(){
  unsigned long maintenant = millis();

  int dureeNote = 2000 / MELODIE[notePosition-1][1];
  
  if(maintenant - notePrecedente >= dureeNote * vitesseLecture){
    notePrecedente = maintenant;
    return true;
  }
  return false;
}  

/*
 * Alarme qui commence à sonner
 */
void alarmeStart(int alarmePos){
  alarme[alarmePos].sonne = true;
  digitalWrite(ledBoutonOK, HIGH);
  digitalWrite(ledBoutonSnooze, HIGH);
  Serial.println("Alarme Start");
  notePrecedente = 0;
}
/*
 * Alarme qui cesse de sonner
 */
void alarmeStop(int alarmePos){
  alarme[alarmePos].sonne = false;
  noTone(piezoBuzzer);
  digitalWrite(ledBoutonOK, LOW);
  digitalWrite(ledBoutonSnooze, LOW);
  Serial.println("Alarme Stop");
}
/*
 * Pendant que l'alarme sonne, répéter une étape
 */
void alarmePulse(){
   
}

/*
 * Vérifier si une alarme sonne
 */
void alarmeGestionAutomatique(){

  // Maintenant avec les secondes
  DateTime maint = rtc.now();

  // Maintenant sans les secondes
  long maintenant = maint.unixtime() - maint.second();  // En secondes 
  
  for(int i=0 ; i<NBRALARMES ; i++){
    
    // Si l'alarme est programmée
    // (i+1) => Commence à 1
    // (i+1) * 3 => saut de postion par 3 (alarme heure, alarme minute, alarme programmé)
    // ((i+1) * 3) + 2 => On se positionne à la position 2 minimum (fanion[0], version[1], luminosité[2])
    if( EEPROM.read( ((i+1)*3)+2 ) ){
      
      // Alarme sonne (mode normal)
      if( alarme[i].heureSonne.unixtime() == maintenant && !alarme[i].sonne && alarmeDerniereAction != ACTION_SNOOZE ){
        alarmeStart(i);
        // A quelle heure se stop l'alarme
        alarme[i].heureStop = rtc.now().unixtime() + DUREEALARME;
        Serial.println("Alarme Start - normal");
      }
      // Alarme sonne (mode snooze)
      if( alarme[i].snoozeSonne.unixtime() == maint.unixtime() && !alarme[i].sonne && alarmeDerniereAction == ACTION_SNOOZE ){
        alarmeStart(i);
        alarme[i].heureStop = rtc.now().unixtime() + DUREEALARME;
        Serial.println("Alarme Start - Snooze");
      }
      // Alarme s'arrête après un délai (mode normal)
      if( alarme[i].heureStop.unixtime() <= maint.unixtime() && alarme[i].sonne && alarmeDerniereAction != ACTION_SNOOZE ){
        alarme[i].heureStop = 0;
        alarmeDerniereAction = ACTION_STOP;
        alarmeStop(i);
        // Posposer l'alarme à demain
        alarme[i].heureSonne = alarme[i].heureSonne.unixtime() + 86400;
        Serial.println("Alarme Stop - normal");
      }
      // Alarme s'arrête après un délai (mode snooze)
      if( alarme[i].heureStop.unixtime() == maint.unixtime() && alarme[i].sonne && alarmeDerniereAction == ACTION_SNOOZE ){
        alarme[i].heureStop = 0;
        alarmeDerniereAction = ACTION_STOP;
        alarmeStop(i);
        // Posposer l'alarme à demain
        alarme[i].heureSonne = alarme[i].heureSonne.unixtime() + 86400;
        Serial.println("Alarme Stop - snooze");
      }
      // Si l'alarme sonne
      if(alarme[i].sonne){
        alarmePulse();
        jouerMelodie();
      }
    }
  }
}
/*
 * Intéragir avec l'alarme
 */
void controlerAlarme(){
  // ### BOUTON OK ###
  if(estAppuye(boutonOk)){

    // Vérifie toutes les alarmes
    for(int i=0 ; i<NBRALARMES ; i++){
      
      // Si une alarme sonne
      if( alarme[i].sonne ){
        
        // L'alarme sonne plus
        alarmeStop(i);
        
        // Posposer l'alarme à demain
        alarme[i].heureSonne = alarme[i].heureSonne.unixtime() + 86400;
        
        // Changer la dernière action
        alarmeDerniereAction = ACTION_STOP;
        digitalWrite(ledBoutonSnooze, LOW);
      }
      else if( alarmeDerniereAction == ACTION_SNOOZE ){
        // Posposer l'alarme à demain
        alarme[i].heureSonne = alarme[i].heureSonne.unixtime() + 86400;
        
        // Changer la dernière action
        alarmeDerniereAction = ACTION_STOP;
        digitalWrite(ledBoutonSnooze, LOW);
      }
    }
  }
  
  // ### BOUTON SNOOZE ###
  if(estAppuye(boutonSnooze)){
    for(int i=0 ; i<NBRALARMES ; i++){
      if(alarme[i].sonne){
        // L'alarme sonne plus
        alarmeStop(i);
        
        // Définir quand faire sonner le snooze
        alarme[i].snoozeSonne = rtc.now().unixtime() + SNOOZEATTENTE;
        
        // Changer la dernière action 
        alarmeDerniereAction = ACTION_SNOOZE;
        digitalWrite(ledBoutonSnooze, HIGH);
      }
    }
  }
}
boolean affichagePoint(int position){
  switch(position){
    case 1:
      // Retournel'état de l'alarme
      return alarme[0].programme;
    case 2:
      // Retournel'état de l'alarme
      return alarme[1].programme;
    case 3 :
      // Retournel'état de l'alarme
      return alarme[2].programme;
    case 4:
      // Retournel'état de l'alarme
      return alarme[3].programme;
    default:
      return false;
  }
}
/*
 * Configuration lors de la première utilisation de l'EEPROM
 * - Formattage de l'EEPROM (mise à zéro des alarmes)
 * - Définir un fanion qui signifie si l'EEPROM est formatté
 * - Définir une version
 * - Définir une valeur pour la luminosité
 */
void eepromConfiguration(){

  /*
   * EEPROM (1024 octets)
   * 
   * |POSITION| |VALEUR|   ~REFERENCE~
   *      0        255       Fanion
   *      1        10        Version
   *      2        15      Luminosité     
   *      3         0     Heure Alarme 1  (définit lors du formattage)
   *      4         0     Minute Alarme 1 (définit lors du formattage)
   *      5         0     Programmé Alarme 1 (définit lors du formattage)
   *      6         0     Heure Alarme 2  (définit lors du formattage)
   *      7         0     Minute Alarme 2 (définit lors du formattage)
   *      8         0     Programmé Alarme 2 (définit lors du formattage)
   *     ...       ...          ...
   */
    
    // Formatter l'EEPROM
    for (int i = 0 ; i < EEPROM.length() ; i++) 
       EEPROM.write(i, 0);
    Serial.println("EEPROM formatté !");
    
    // Valeur par défaut pour savoir si l'EEPROM est formatté
    EEPROM.write(0, 255); 
    Serial.println("Valeur par défaut définie !");

    // Définir une version
    EEPROM.write(1, version); 
    Serial.println("Version définie !");
    
    // Définir une luminosité 
    EEPROM.write(2, 15); 
    Serial.println("Valeur par défaut pour la luminosité est définie !");
}
/*
 * Affichage du contenu de l'eeprom
 */
void affichageEEpromContenu(){
  // Taille mémoire de l'EEPROM
  int octets = 1024;
  
  int j = 0;
  Serial.println("Zone mémoire");
  for(int i=0 ; i<octets ; i++){
    Serial.print("|");
    if( i == 0 )
      Serial.print("Fanion");
    else if( i == 1 )
      Serial.print("Version");
    else if( i == 2 )
      Serial.print("Luminosité");
    else if( j == 0 ){
      Serial.print("Alarme Heure");
      j++;
    }
    else if( j == 1 ){
      Serial.print("Alarme Minute");
      j++;
    }
    else{
      Serial.print("Alarme Prog");
      j=0;
    }
    Serial.print("|\t|");
    Serial.print(EEPROM.read(i));
    Serial.println("|");
    if ( i >= 2 && j == 0 )
      Serial.println();
  }
}

/*
 * Activer/désactiver une alarmes
 */
void changerEtatAlarmes(){
  for(int i=0 ; i<NBRALARMES ; i++){
    if(estAppuye(BOUTONALARME[i])){
      alarme[i].programme = !alarme[i].programme;

      // Si on active l'alarme
      if(alarme[i].programme){
        // Dessiner les heures et minutes 
        affichageMinutes(alarme[i].heureSonne.minute(), true);
        affichageHeures(alarme[i].heureSonne.hour(), true);
        delay(300);
      }
      else
        afficherTemps();
      EEPROM.write(((i+1)*3)+2, alarme[i].programme);  
      delay(1000);
    }
  }
}

/*
 * Changer l'heure d'une alarme
 */
void changerHeureAlarme(){
  if(estAppuye(boutonAlarmeControl) && !estAppuye(boutonOk)){
    
    boolean alarmeBoucle = true;
    int alarmeNb = 1;

    // Affichage A1 (Alarme 1)
    affichageHeures(1, false); 
    afficheurs.drawColon(false); // 2 points au centrer de l'afficheur
    afficheurs.writeDigitNum(3, 10, affichagePoint(3));
    afficheurs.writeDigitNum(4, alarmeNb, affichagePoint(4));
    afficheurs.writeDisplay(); // Dessiner

    delay(1000); //Eviter les répétition du bouton
    while(alarmeBoucle){
      // Sélectionner une alarme
      if(estAppuye(boutonPlus)){
        if(alarmeNb < NBRALARMES){ 
          alarmeNb++;
          afficheurs.writeDigitNum(4, alarmeNb, affichagePoint(4));
          afficheurs.writeDisplay(); // Dessiner
        }
      }
      // Sélectionner une alarme
      if(estAppuye(boutonMoins)){
        if(alarmeNb > 1){ 
          alarmeNb--;
          afficheurs.writeDigitNum(4, alarmeNb, affichagePoint(4));
          afficheurs.writeDisplay(); // Dessiner
        }
      }
      // Annuler le mode alarme
      if(estAppuye(boutonAlarmeControl)){
        alarmeBoucle = false;
        delay(1000); //Eviter les répétition du bouton
      }
      // Configurer l'alarm
      digitalWrite(ledBoutonOK, HIGH);
      if(estAppuye(boutonOk)){
        digitalWrite(ledBoutonOK, LOW);
        // Remplacer l'alarme (insertion de son heure précédente)
        int h = alarme[alarmeNb -1].heureSonne.hour(); 
        int m = alarme[alarmeNb -1].heureSonne.minute();
        
        changerHeureVisuel( &h, &m);
        
        // Changement de son heure
        DateTime maintenant = rtc.now();
        alarme[alarmeNb -1].heureSonne = DateTime(maintenant.year(), maintenant.month(), maintenant.day(), h, m, 0);

        // Changer l'heure dans l'EEPROM
        EEPROM.write(alarmeNb*3, h);
        // Changer la minute dans l'EEPROM
        EEPROM.write((alarmeNb*3)+1, m);

        alarmeBoucle = false;
        delay(1000); //Eviter les répétition du bouton
      }
      delay(150);
    }
    digitalWrite(ledBoutonOK, LOW);
    delay(1000);
  }
}
/*
 *  Changer la luminostié
 */
void changeLuminosite(){
  if(estAppuye(boutonLuminosite)) {
    boolean luminositeBoucle = true;
    int luminosite = EEPROM.read(2);
    
    // Cacher les afficheurs de l'heure
    affichageHeures(1, false);
    // Cacher 2 points au centrer de l'afficheur
    afficheurs.drawColon(false);
    // Afficher l'intensité lumineuse
    affichageMinutes(luminosite, true); 

    delay(1000); // Eviter les rebons

    while(luminositeBoucle){
      // Si on appuie de nouveau sur le bouton, on stop la modification d'intensité lumineuse
      digitalWrite(ledBoutonOK, HIGH);
      if(estAppuye(boutonLuminosite) || estAppuye(boutonOk)) {
        digitalWrite(ledBoutonOK, LOW);
        luminositeBoucle = false;
      }
      // Augmente la luminosité
      if(estAppuye(boutonPlus)) {
        if(luminosite < 15)     luminosite++;
        
        afficheurs.setBrightness(luminosite);
        affichageMinutes(luminosite, true); 
      }
      // Diminue la luminosité
      if(estAppuye(boutonMoins)) {
        if(luminosite > 0)      luminosite--;
        
        afficheurs.setBrightness(luminosite);
        affichageMinutes(luminosite, true); 
      }
      delay(150);
    }
    EEPROM.write(2, luminosite);
    delay(1000); 
  }
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
 * Vérifier si c'est le moment de faire clignoter
 */
boolean clignote(){
  unsigned long maintenant = millis();

  if(maintenant - avantClignote >= 1000){
    avantClignote = maintenant;
    return true;
  }
  return false;
}      
/*
 * L'utilisateur peut changer l'heure
 */
void changerHeure(){
  if(estAppuye(boutonOk) && estAppuye(boutonAlarmeControl)){
    
    // DateTime maintenant
    DateTime maintenant = rtc.now();
    // Affichage changement heure
    int h = maintenant.hour();
    int m = maintenant.minute();

    changerHeureVisuel(&h,&m);
    // Changer l'heure
    rtc.adjust(DateTime(maintenant.year(), maintenant.month(), maintenant.day(), h, m, 0)); // Change l'heure de l'RTC 
  }
}
 /*
  * Réglage visuel de l'heure - modifie les valeurs de heures et minutes
  */
void changerHeureVisuel(int* heures, int* minutes){
   boolean minuteBoucle = true;
   boolean heureBoucle  = true;

   boolean heureAffichage = true;
   boolean minuteAffichage = true;

   // Allumer les 2 points
   afficheurs.drawColon(true);
   // Afficher les heures et minutes
   affichageHeures(*heures, heureAffichage);
   affichageMinutes(*minutes, minuteAffichage);
   delay(1000);

   // --- Réglage heures ---
   while(heureBoucle){
    // Faire clignoter l'écran
    if(clignote()){
      heureAffichage = !heureAffichage;
    }
    // Bouton plus
    if(estAppuye(boutonPlus)) {
      *heures += 1;
      if(*heures > 23) *heures = 0;
      delay(250);
    }
    // Bouton moins
    if(estAppuye(boutonMoins)) {
      *heures -= 1;
      if(*heures < 0) *heures = 23;
      delay(250);
    }
    // Bouton ok
    digitalWrite(ledBoutonOK, HIGH);
    if(estAppuye(boutonOk)) {
      digitalWrite(ledBoutonOK, LOW);
      heureBoucle = false;
      delay(1000);
    }
    affichageHeures(*heures, heureAffichage);
   }
   affichageHeures(*heures, true);
   
   // --- Réglage Minutes ---
   while(minuteBoucle){
    // Faire clignoter l'écran
    if(clignote()){
      minuteAffichage = !minuteAffichage;
    }
    // Bouton plus
    if(estAppuye(boutonPlus)) {
      *minutes += 1;
      if(*minutes > 59) *minutes = 0;
      delay(250);
    }
    // Bouton moins
    if(estAppuye(boutonMoins)) {
      *minutes -= 1;
      if(*minutes < 0) *minutes = 59;
      delay(250);
    }
    // Bouton ok
    digitalWrite(ledBoutonOK, HIGH);
    if(estAppuye(boutonOk)) {
      digitalWrite(ledBoutonOK, LOW);
      minuteBoucle = false;
      delay(1000);
    }
    affichageMinutes(*minutes, minuteAffichage);
   }
   affichageMinutes(*minutes, true);
 }
 
/*
 * Afficher les minutes
 */
 void affichageMinutes(int minutesT, boolean visible){
  if(visible){
    afficheurs.writeDigitNum(3, minutesT/10, affichagePoint(3));
    afficheurs.writeDigitNum(4, minutesT - ((int)(minutesT/10))*10, affichagePoint(4));
  }
  else{
    afficheurs.writeDigitNum(3, 16, affichagePoint(3));
    afficheurs.writeDigitNum(4, 16, affichagePoint(4));
  }
  // Change les afficheurs
  afficheurs.writeDisplay();
 }
 
 /*
 * Afficher les heures
 */
 void affichageHeures(int heuresT, boolean visible){
  // On affiche les heures
  if(visible){
    afficheurs.writeDigitNum(0, heuresT/10, affichagePoint(1));
    afficheurs.writeDigitNum(1, heuresT - ((int)(heuresT/10))*10, affichagePoint(2));
  }
  // On cache les heures
  else{
    afficheurs.writeDigitNum(0, 16, affichagePoint(1));
    afficheurs.writeDigitNum(1, 16, affichagePoint(2));
  }
  // Change les afficheurs
  afficheurs.writeDisplay();
 }

 /*       
 * Ajouter une seconde et obtenir l'heure       
 */
void afficherTemps(){
   DateTime maintenant = rtc.now();
   // Dessiner minutes
   affichageMinutes(maintenant.minute(), true);
   // Dessiner heures
   affichageHeures(maintenant.hour(), true);
}
 /*
  * Afficher les séparateurs (2 points sur l'afficheur)
  */
void afficherSeparateur(){
  if(clignote()){
    // Changer l'état des 2 points
    separateur = !separateur;
    afficheurs.drawColon(separateur);
  }
}
