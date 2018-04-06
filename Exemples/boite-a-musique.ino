/*
********************
**** Mon Réveil ****
********************
* Un réveil hackable contenant 4 alarmes et un réglage de luminosité
* 
* Deveopped by Stefan for shop.mchobby.be - CC-BY-SA
* Distributed as it, without warranties
*/

// Version du programme (Version : 0.1)
#define VERSION                1

#include "Adafruit_LEDBackpack.h"
#include <RTClib.h>
#include <EEPROM.h>
#include "pitches.h"  // Notes de musique
#include "melodies.h" // Playlists de mélodies

// Boutons Piggrl
#define BOUTON_ALARME_CONTROLE 2 // Digitale
#define BOUTON_PLUS            3 // Digitale
#define BOUTON_MOINS           4 // Digitale
#define BOUTON_LUMINOSITE      5 // Digitale

// Boutons Arcade
#define BOUTON_OK             14 // Analogue 0
#define BOUTON_SNOOZE         15 // Analogue 1
#define LED_BOUTON_OK         16 // Analogue 2
#define LED_BOUTON_SNOOZE     17 // Analogue 3

// Piezzo Buzzer
#define PIEZO_BUZZER          13 // Digitale

// Dernière action réalisé sur les alarmes
#define ACTION_STOP            1
#define ACTION_SNOOZE          2

// ================================================
// =============== Personnalisation ===============
// ================================================
const int SNOOZE_ATTENTE     = 10;               // Durant combien de temps l'utilisateur va t'il encore dormir ? (en secondes)
const int DUREE_ALARME       = 60;               // Durant combien de temps l'alarme va t'elle sonner (en secondes) 
const int BOUTON_ALARME[]    = {8, 7, 9, 10};    // Quelles pins pour activer/désactiver chaques alarmes
const float VITESSE_LECTURE  = 1;                // Vitesse sonore des alarmes (par défaut 1)
const int MELODIE[][ 2 ]     = MARIO;            // Sélectionner la musique que vous désirez pour vos alarmes (voir melodies.h)
// ================================================


// ### Alarmes ###
struct alarme{
    DateTime heureSonne = 0;      // A quelle heure l'alarme sonnera-t'elle ?
    DateTime heureStop = 0;       // A quelle heure l'alarme va t'elle s'arrêter automatiquement si elle est pas coupé
    DateTime snoozeSonne = 0;     // L'heure ou le snooze sonnera
    boolean programme = false;    // L'alarme a-t'elle été programmée par l'utilisateur ?
    boolean sonne = false;        // L'alarme est-elle en train de sonner ?
};
const int NBRALARMES = sizeof( BOUTON_ALARME ) / sizeof( int );  // Combien d'alarmes programables
struct alarme alarme[ NBRALARMES ];                              // Déclaration de la structure
byte alarmeDerniereAction = ACTION_STOP;                         // Dernière action effectué sur l'alarme

// ### Séparateur ###
boolean separateur = false;       // 2 points au milieu des afficheurs
unsigned long avantClignote = 0;  // Savoir la dernière fois (durée) que les 2 points ont changés d'état (allumé/éteind)

// ### Mélodie ###
unsigned long notePrecedente = 0; // Savoir la dernière fois (durée) que la note précédente a été démarrée
int notePosition = 0;  
         
// ### Autres ###
Adafruit_7segment afficheurs = Adafruit_7segment(); // Initialisation de l'afficheur 7 segments
RTC_DS1307 rtc = RTC_DS1307();                      // Initialisation de le RTC

// ### ZONE HACK ###

// -- Boite à musique --
#define BOITE_ENABLE  12
#define BOITE_INPUT   6

/******************************************************************
 *                            Hackable                            *
 ******************************************************************/

// -- Boite à musique --
void activerBoite(){
  digitalWrite( BOITE_ENABLE , HIGH );
}
void jouerBoite(){
  // Vitesse de 0 à 255
  analogWrite( BOITE_INPUT , 220 );
}
void stopBoite(){
  digitalWrite( BOITE_ENABLE , LOW );
}

/*
 * Alarme qui commence à sonner
 */
void alarmeStart( int alarmePos ){
  if( alarmePos == 0 )
    activerBoite();
  else 
    activerMelodie();
}

/*
 * Pendant que l'alarme sonne, répéter une étape
 */
void alarmePulse( int alarmePos ){
  if( alarmePos == 0 )
    jouerBoite();
  else
    jouerMelodie();
}

/*
 * Alarme qui cesse de sonner
 */
void alarmeStop(int alarmePos){
  if( alarmePos == 0 )
    stopBoite();
  else
    arreterMelodie();
}
/*
 * Vérifier si c'est le moment d'effectuer l'action
 * 
 * ARGUMENTS
 * tempsAvant est temps précédent en milisecondes
 * dureeAttente est le temps avant de relancer l'exécution
 * 
 * RETURN
 * 0 Signifie pas maintenant
 * x valeur (unsigned long) : effectuer l'action + changer le tempsAvant (définie dans l'entête du code)
 */
unsigned long effectuerAction( unsigned long tempsAvant, int dureeAttente ){
  unsigned long maintenant = millis();

  if( maintenant - tempsAvant >= dureeAttente )
    return maintenant;
    
  return 0;
}   
/*
 * Affichage du point désignant si l'alarme est active ou pas
 * Clignote si sonne
 */
boolean affichagePoint(int position){
  switch(position){
    case 1:
      // Retourne l'état de l'alarme
      if( alarme[0].sonne && clignote( false ) )
        return false;
      return alarme[0].programme;
    case 2:
      // Retourne l'état de l'alarme
      if( alarme[1].sonne && clignote( false ) )
        return false;
      return alarme[1].programme;
    case 3 :
      // Retourne l'état de l'alarme
      if( alarme[2].sonne && clignote( false ) )
        return false;
      return alarme[2].programme;
    case 4:
      // Retourne l'état de l'alarme
      if( alarme[3].sonne && clignote( false ) )
        return false;
      return alarme[3].programme;
    default:
      return false;
  }
}

/******************************************************************
 *                           Setup + loop                         *
 ******************************************************************/
/*
 *  Configuration avant lancement de la routine
 */
void setup() {
  
  // Communication RS232
  Serial.begin( 9600 );

  // Initialisation des boutons
  pinMode( BOUTON_OK, INPUT_PULLUP );
  pinMode( BOUTON_MOINS, INPUT_PULLUP );
  pinMode( BOUTON_PLUS, INPUT_PULLUP );
  pinMode( BOUTON_LUMINOSITE, INPUT_PULLUP );
  pinMode( BOUTON_ALARME_CONTROLE, INPUT_PULLUP );
  pinMode( BOUTON_SNOOZE, INPUT_PULLUP );
  // Boucles pour les broches utilisées pour activer/désactiver les alarmes
  for( int i=0 ; i<NBRALARMES ; i++ )
    pinMode( BOUTON_ALARME[i], INPUT_PULLUP );

  // Initialisation des leds et du piezo
  pinMode( LED_BOUTON_OK, OUTPUT );
  pinMode( LED_BOUTON_SNOOZE, OUTPUT );
  pinMode( PIEZO_BUZZER, OUTPUT );
  
  // Adresse I2C des afficheurs 
  afficheurs.begin( 0x70 );
  affichageHeures( 0, true);
  affichageMinutes( 0, true);
  delay( 1000 );

  // Démarrer le lien avec le RTC en I2C
  rtc.begin();
  
  // Configuration de l'heure par l'utilisateur si ce n'a pas été encore fait
  if ( !rtc.isrunning() ){
    int h = 0;
    int m = 0;
    changerHeureVisuel( &h, &m );
    rtc.adjust( DateTime(2018, 2, 20, h, m, 0) ); // Change l'heure de le RTC 
  }

  // Formattage de l'EEPROM si rien dedans (vérification si fanion égale à 255) et si la version est égale à ce qu'il y a dans l'EEPROM
  if( EEPROM.read(0) != 255 || EEPROM.read(1) != VERSION )
    eepromConfiguration();
    
  // Configuration de l'heure par l'utilisateur si ce n'a pas été encore fait
  if ( !rtc.isrunning() ){
    int h = 0;
    int m = 0;
    changerHeureVisuel( &h, &m );
    rtc.adjust( DateTime(2018, 2, 20, h, m, 0) ); // Change l'heure de le RTC 
  }

  // Définir la luminosité des afficheurs
  afficheurs.setBrightness( EEPROM.read(2) );

  // Configuration des dates/heures sur les alarmes
  DateTime maintenant = rtc.now();
  int j = 0;
  for( int i=3 ; i< ( NBRALARMES*2 )+6 ; i+=3 ){
    alarme[j].heureSonne = DateTime( maintenant.year(), maintenant.month(), maintenant.day(), EEPROM.read(i), EEPROM.read(i+1), 0);
    alarme[j].programme = EEPROM.read(i+2);
    j++;
  }

  // ### ZONE HACK ###

  pinMode( BOITE_ENABLE, OUTPUT );
  pinMode( BOITE_INPUT, OUTPUT );
  digitalWrite( BOITE_ENABLE , LOW );
  
}

/*
 * Routine du code
 */
void loop() {
  // ============================================================== 
  // ================ L'utilisateur peut intéragir ================ 
  // ============================================================== 
  // Changer la luminosité
  changerLuminosite();
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

/******************************************************************
 *                            Mélodie                             *
 ******************************************************************/
/*
 *  Activer la mélodie
 */
void activerMelodie(){
  // Remettre au début de la mélodie
  notePosition = 0;
}
/*
 * Jouer la mélodie
 */
void jouerMelodie(){

    // Depuis combien de temps en milisecondes la routine tourne
    unsigned long maintenant = millis();
    
    // Duree de la note précedente en milisecondes
    int dureeNote = 2000 / MELODIE[notePosition-1][1];

    // Prochaine note à jouer si la note précédente à fini d'être jouée
    if( maintenant - notePrecedente >= dureeNote * VITESSE_LECTURE ){
      
      // Redéfinir la dernière note (temps) à maintenant
      notePrecedente = maintenant;

      // On joue une note pendant un certain temps
      int dureeNote = 2000 / MELODIE[notePosition][1];
      tone( PIEZO_BUZZER , MELODIE[notePosition][0] , dureeNote * VITESSE_LECTURE ); 

      // Si la mélodie est finie, on recommence depuis le début la mélodie
      int nbrLignesTableauMelodie = (sizeof( MELODIE ) / sizeof( int ) / 2) -1;
      if( notePosition >= nbrLignesTableauMelodie )
        notePosition=0;
      else
        notePosition++;
    }   
}
/*
 * Arrèter la mélodie
 */
void arreterMelodie(){
    noTone( PIEZO_BUZZER );
    notePosition = 0;
}

/******************************************************************
 *                            Alarmes                             *
 ******************************************************************/
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
    // ((i+1) * 3) + 2 => On se positionne à la position 2 minimum (fanion[0], VERSION[1], luminosité[2])
    if( EEPROM.read( ((i+1)*3)+2 ) ){
      
      // Alarme sonne (mode normal-snooze)
      if( ( alarme[i].heureSonne.unixtime() == maintenant && !alarme[i].sonne && alarmeDerniereAction != ACTION_SNOOZE ) || ( alarme[i].snoozeSonne.unixtime() == maint.unixtime() && !alarme[i].sonne && alarmeDerniereAction == ACTION_SNOOZE ) ){
        
        // Changer l'état de l'alarme
        alarme[i].sonne = true;
  
        // Allumer les boutons d'arcade
        digitalWrite( LED_BOUTON_OK, HIGH );
        digitalWrite( LED_BOUTON_SNOOZE, HIGH );

        // Lancer les routines des alarmes
        alarmeStart(i);
        
        // A quelle heure se stop l'alarme
        alarme[i].heureStop = rtc.now().unixtime() + DUREE_ALARME;
      }
      // Alarme s'arrête après un délai (mode normal-snooze)
      if( ( alarme[i].heureStop.unixtime() <= maint.unixtime() && alarme[i].sonne && alarmeDerniereAction != ACTION_SNOOZE ) || ( alarme[i].heureStop.unixtime() == maint.unixtime() && alarme[i].sonne && alarmeDerniereAction == ACTION_SNOOZE ) ){

        // Changer l'état de l'alarme
        alarme[i].sonne = false;

        // Eteindre les boutons d'arcade
        digitalWrite( LED_BOUTON_OK, LOW );
        digitalWrite( LED_BOUTON_SNOOZE, LOW );
        
        // Remise à zéro du moment ou on a stopper la dernière fois l'alarme
        alarme[i].heureStop = 0;

        // Changer la dernière action effectué
        alarmeDerniereAction = ACTION_STOP;

        // Stopper les routines des alarmes
        alarmeStop(i);
        
        // Posposer l'alarme à demain
        alarme[i].heureSonne = alarme[i].heureSonne.unixtime() + 86400;
      }
      // Si l'alarme sonne
      if( alarme[i].sonne ){
        // Allumer les boutons d'arcade
        digitalWrite( LED_BOUTON_OK, HIGH );
        digitalWrite( LED_BOUTON_SNOOZE, HIGH );
        alarmePulse( i );
      }
    }
  }
}

/*
 * Intéragir avec l'alarme
 */
void controlerAlarme(){
  
  // ### BOUTON OK ###
  if( estAppuye( BOUTON_OK ) ){

    // Vérifie toutes les alarmes
    for(int i=0 ; i<NBRALARMES ; i++){
      
      // Si une alarme sonne
      if( alarme[i].sonne ){

        // Changer l'état de l'alarme
        alarme[i].sonne = false;

        // Eteindre les boutons d'arcade
        digitalWrite( LED_BOUTON_OK, LOW );
        digitalWrite( LED_BOUTON_SNOOZE, LOW );

        // Changer la dernière action
        alarmeDerniereAction = ACTION_STOP;
        
        // Stopper les routines des alarmes
        alarmeStop(  i );
        
        // Posposer l'alarme à demain
        alarme[i].heureSonne = alarme[i].heureSonne.unixtime() + 86400;
      }
      // Si on est en snooze mais l'alarme ne sonne pas
      else if( alarmeDerniereAction == ACTION_SNOOZE ){
        
        alarme[i].sonne = false;
        
        // Posposer l'alarme à demain
        alarme[i].heureSonne = alarme[i].heureSonne.unixtime() + 86400;
        
        // Changer la dernière action
        alarmeDerniereAction = ACTION_STOP;
        
        // Désactiver la led du snooze
        digitalWrite( LED_BOUTON_SNOOZE, LOW );
      }
    }
  }
  
  // ### BOUTON SNOOZE ###
  if( estAppuye( BOUTON_SNOOZE ) ){
    for(int i=0 ; i<NBRALARMES ; i++){
      if( alarme[i].sonne ){
        
        // L'alarme sonne plus
        alarmeStop( i );

        alarme[i].sonne = false;
        
        // Définir quand faire sonner le snooze
        alarme[i].snoozeSonne = rtc.now().unixtime() + SNOOZE_ATTENTE;
        
        // Changer la dernière action 
        alarmeDerniereAction = ACTION_SNOOZE;

        // Activer la led du snooze
        digitalWrite( LED_BOUTON_OK, LOW );
        digitalWrite( LED_BOUTON_SNOOZE, HIGH );
      }
    }
  }
}

/*
 * Activer/désactiver une alarmes
 */
void changerEtatAlarmes(){
  
  for(int i=0 ; i<NBRALARMES ; i++){

    if( estAppuye( BOUTON_ALARME[i] ) ){
      
      // Changer l'état
      alarme[i].programme = !alarme[i].programme;

      // Si on active l'alarme
      if( alarme[i].programme ){

        DateTime maintenant = rtc.now();
        
        // Définir l'alarme aujourd'hui si l'heure n'est pas encore dépassée
        if( ( maintenant.minute() +  maintenant.hour() * 60 ) <= ( alarme[i].heureSonne.minute() +  alarme[i].heureSonne.hour() * 60 ) )
          alarme[i].heureSonne = maintenant.unixtime() - maintenant.second();
        
        // Dessiner les heures et minutes 
        affichageMinutes( alarme[i].heureSonne.minute(), true );
        affichageHeures( alarme[i].heureSonne.hour(), true );
        
        // Délai pour afficher l'heure
        delay( 300 );
      }
      // Si on désactive l'alarme
      else
        afficherTemps();

      
      // Si cette alarme sonne, éteindre les boutons et stopper l'alarme
      if( alarme[i].sonne ){
        digitalWrite( LED_BOUTON_OK, LOW );
        digitalWrite( LED_BOUTON_SNOOZE, LOW );
        alarmeStop( i );
      }

      // Réecrire l'état de l'alarme dans l'EEPROM
      EEPROM.write( (( i+1 )*3)+2 , alarme[i].programme);  

      // Délai pour éviter les rebons du bouton
      delay( 1000 );
      
    }
  }
}

/*
 * Changer l'heure d'une alarme
 */
void changerHeureAlarme(){
  if( estAppuye( BOUTON_ALARME_CONTROLE ) && !estAppuye( BOUTON_OK ) ){
    
    boolean alarmeBoucle = true;
    int alarmeNb = 1;

    // Affichage A1 (Alarme 1)
    affichageHeures( 1, false ); 
    afficheurs.drawColon( false ); // 2 points au centrer de l'afficheur
    afficheurs.writeDigitNum( 3, 10, affichagePoint(3) );
    afficheurs.writeDigitNum( 4, alarmeNb, affichagePoint(4) );
    afficheurs.writeDisplay(); // Dessiner

    // Eviter les répétition du bouton
    delay( 500 ); 
    
    while( alarmeBoucle ){
      // Sélectionner une alarme
      if( estAppuye( BOUTON_PLUS ) ){
        if( alarmeNb < NBRALARMES ){ 
          alarmeNb++;
          afficheurs.writeDigitNum( 4, alarmeNb, affichagePoint(4) );
          afficheurs.writeDisplay(); // Dessiner
          delay( 200 );
        }
      }
      // Sélectionner une alarme
      if( estAppuye( BOUTON_MOINS ) ){
        if( alarmeNb > 1 ){ 
          alarmeNb--;
          afficheurs.writeDigitNum( 4, alarmeNb, affichagePoint(4) );
          afficheurs.writeDisplay(); // Dessiner
          delay( 200 );
        }
      }
      // Annuler le mode alarme
      if( estAppuye( BOUTON_ALARME_CONTROLE ) ){
        alarmeBoucle = false;
      }
      // Configurer l'alarm
      digitalWrite( LED_BOUTON_OK, HIGH );
      if( estAppuye( BOUTON_OK ) ){
        digitalWrite( LED_BOUTON_OK, LOW );
        // Remplacer l'alarme (insertion de son heure précédente)
        int h = alarme[alarmeNb -1].heureSonne.hour(); 
        int m = alarme[alarmeNb -1].heureSonne.minute();
        
        changerHeureVisuel( &h, &m );
        
        // Changement de son heure
        DateTime maintenant = rtc.now();
        alarme[alarmeNb -1].heureSonne = DateTime(maintenant.year(), maintenant.month(), maintenant.day(), h, m, 0);

        // Changer l'heure dans l'EEPROM
        EEPROM.write( alarmeNb*3, h );
        // Changer la minute dans l'EEPROM
        EEPROM.write( (alarmeNb*3)+1, m );

        alarmeBoucle = false;

      }
    }
    digitalWrite( LED_BOUTON_OK, LOW );
    
    //Eviter les répétition du bouton
    delay( 500 );
  }
}
/******************************************************************
 *                     EEPROM Fromattage                          *
 ******************************************************************/
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
       EEPROM.write( i, 0 );
    Serial.println( "EEPROM formatté !" );
    
    // Valeur par défaut pour savoir si l'EEPROM est formatté
    EEPROM.write( 0, 255 ); 
    Serial.println( "Valeur par défaut définie !" );

    // Définir une version
    EEPROM.write( 1, VERSION ); 
    Serial.print( "Version " );
    Serial.print( VERSION/10 );
    Serial.println( " définie !" );
    
    // Définir une luminosité 
    EEPROM.write( 2, 15 ); 
    Serial.println( "Valeur par défaut pour la luminosité est définie !" );
}
/******************************************************************
 *                     Changer la luminosité                      *
 ******************************************************************/
/*
 *  Changer la luminostié
 */
void changerLuminosite(){
  
  if( estAppuye( BOUTON_LUMINOSITE ) ) {
    
    boolean luminositeBoucle = true;
    
    // Obtenir la luminosité venant de l'EEPROM
    int luminosite = EEPROM.read( 2 );
    
    // Cacher les afficheurs de l'heure
    affichageHeures( 1, false );
    // Cacher 2 points au centrer de l'afficheur
    afficheurs.drawColon( false );
    // Afficher l'intensité lumineuse
    affichageMinutes( luminosite, true ); 

    // Eviter les rebons
    delay( 1000 );

    while( luminositeBoucle ){
      // Si on appuie de nouveau sur le bouton, on stop la modification d'intensité lumineuse
      digitalWrite( LED_BOUTON_OK, HIGH );
      if( estAppuye( BOUTON_LUMINOSITE ) || estAppuye( BOUTON_OK ) ) {
        digitalWrite( LED_BOUTON_OK, LOW );
        luminositeBoucle = false;
      }
      // Augmente la luminosité
      if( estAppuye( BOUTON_PLUS ) ) {
        if( luminosite < 15 )     
          luminosite++;
        
        afficheurs.setBrightness( luminosite );
        affichageMinutes( luminosite, true ); 
      }
      // Diminue la luminosité
      if( estAppuye( BOUTON_MOINS ) ) {
        if( luminosite > 0 )      
          luminosite--;
        
        afficheurs.setBrightness( luminosite );
        affichageMinutes( luminosite, true ); 
      }
      delay( 150 );
    }
    EEPROM.write( 2, luminosite );
    delay( 1000 ); 
  }
}
/******************************************************************
 *                        Fonctions utiles                        *
 ******************************************************************/
/*
 * Vérifie si le bouton est appuyé ou non
 */
boolean estAppuye( int pinAppuye ){
  int val1 = digitalRead( pinAppuye );     
  delay( 10 );                         
  int val2 = digitalRead( pinAppuye );     
  if ( val1 == val2 && val1 == 0 ) 
    return true;
  else 
    return false;
}   
/*
 * Vérifier si c'est le moment de faire clignoter
 * changerPeriode signifie si on change la période précédente, il est important de le changer uniquement quand on fait clignoter uniquement les 2 points au centre de l'écran
 */
boolean clignote( boolean changerPeriode ){
  unsigned long maintenant = millis();

  if( maintenant - avantClignote >= 1000 ){
    if( changerPeriode )
      avantClignote = maintenant;
    return true;
  }
  return false;
}     
/******************************************************************
 *                           Affichage                            *
 ******************************************************************/ 
/*
 * L'utilisateur peut changer l'heure
 */
void changerHeure(){
  if( estAppuye( BOUTON_OK ) && estAppuye( BOUTON_ALARME_CONTROLE ) ){
    
    // DateTime maintenant
    DateTime maintenant = rtc.now();
    // Affichage changement heure
    int h = maintenant.hour();
    int m = maintenant.minute();

    changerHeureVisuel( &h,&m );
    // Changer l'heure
    rtc.adjust( DateTime(maintenant.year(), maintenant.month(), maintenant.day(), h, m, 0) ); // Change l'heure de le RTC 
  }
}
 /*
  * Réglage visuel de l'heure - modifie les valeurs de heures et minutes
  */
void changerHeureVisuel( int* heures, int* minutes ){
   boolean minuteBoucle = true;
   boolean heureBoucle  = true;

   boolean heureAffichage = true;
   boolean minuteAffichage = true;

   // Allumer les 2 points
   afficheurs.drawColon( true );
   // Afficher les heures et minutes
   affichageHeures( *heures, heureAffichage );
   affichageMinutes( *minutes, minuteAffichage );
   delay( 500 );

   // --- Réglage heures ---
   while( heureBoucle ){
    // Faire clignoter l'écran
    if( clignote( true ) ){
      heureAffichage = !heureAffichage;
    }
    // Bouton plus
    if( estAppuye( BOUTON_PLUS ) ) {
      *heures += 1;
      
      if( *heures > 23 ) 
        *heures = 0;
        
      delay( 200 );
    }
    // Bouton moins
    if( estAppuye( BOUTON_MOINS ) ) {
      *heures -= 1;
      
      if( *heures < 0 ) 
        *heures = 23;
        
      delay( 250 );
    }
    // Bouton ok
    digitalWrite( LED_BOUTON_OK, HIGH );
    if( estAppuye( BOUTON_OK ) ) {
      digitalWrite( LED_BOUTON_OK, LOW );
      heureBoucle = false;
      delay( 800 );
    }
    affichageHeures( *heures, heureAffichage );
   }
   affichageHeures( *heures, true );
   
   // --- Réglage Minutes ---
   while( minuteBoucle ){
    // Faire clignoter l'écran
    if( clignote( true ) ){
      minuteAffichage = !minuteAffichage;
    }
    // Bouton plus
    if( estAppuye( BOUTON_PLUS ) ) {
      *minutes += 1;
      
      if( *minutes > 59 ) 
        *minutes = 0;
        
      delay( 200 );
    }
    // Bouton moins
    if(estAppuye(BOUTON_MOINS)) {
      *minutes -= 1;
      
      if( *minutes < 0 ) 
        *minutes = 59;
        
      delay( 200 );
    }
    // Bouton ok
    digitalWrite( LED_BOUTON_OK, HIGH );
    if( estAppuye(BOUTON_OK) ) {
      digitalWrite( LED_BOUTON_OK, LOW );
      minuteBoucle = false;
      delay( 800 );
    }
    affichageMinutes( *minutes, minuteAffichage );
   }
   affichageMinutes( *minutes, true );
 }
 
/*
 * Afficher les minutes
 */
 void affichageMinutes( int minutesT, boolean affichageNombre ){
  if( affichageNombre ){
    afficheurs.writeDigitNum( 3, minutesT/10, affichagePoint(3) );
    afficheurs.writeDigitNum( 4, minutesT - ((int)(minutesT/10))*10, affichagePoint(4) );
  }
  else{
    afficheurs.writeDigitNum( 3, 16, affichagePoint(3) );
    afficheurs.writeDigitNum( 4, 16, affichagePoint(4) );
  }
  // Change les afficheurs
  afficheurs.writeDisplay();
 }
 
 /*
 * Afficher les heures
 */
 void affichageHeures( int heuresT, boolean affichageNombre ){
  // On affiche les heures
  if( affichageNombre ){
    afficheurs.writeDigitNum( 0, heuresT/10, affichagePoint(1) );
    afficheurs.writeDigitNum( 1, heuresT - ((int)(heuresT/10))*10, affichagePoint(2) );
  }
  // On cache les heures
  else{
    afficheurs.writeDigitNum( 0, 16, affichagePoint(1) );
    afficheurs.writeDigitNum( 1, 16, affichagePoint(2) );
  }
  // Change les afficheurs
  afficheurs.writeDisplay();
 }

 /*       
 * Afficher l'heure 
 */
void afficherTemps(){
   DateTime maintenant = rtc.now();
   // Dessiner minutes
   affichageMinutes( maintenant.minute(), true );
   // Dessiner heures
   affichageHeures( maintenant.hour(), true );
}
 /*
  * Afficher les séparateurs (2 points sur l'afficheur)
  */
void afficherSeparateur(){
  if( clignote( true ) ){
    // Changer l'état des 2 points
    separateur = !separateur;
    afficheurs.drawColon( separateur );
    afficheurs.writeDisplay();
  }
}
