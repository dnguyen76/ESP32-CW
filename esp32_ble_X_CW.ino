
/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Creation d'un serveur BlueTooth that, qui une fois la connexion établie, envoie des annonces periodiques
   Ce serveur s'annonce en Bluetooth avec l'identifiant : 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   L'identifiant : 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - est utilisé pour recevoir des données ( mode "WRITE")
   L'identifiant : 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - est utilisé pour envoyer des données  ( mode "NOTIFY")

  Pour créer des services Bluetooth il faut:
  
   1. Creer un Serveur BLE (Bluetooth Low Energy)
   2. Creer un Service BLE
   3. Creer une Caracteristique BLE du Service
   4. Creer un descripteur BLE pour ce service
   5. Démarrer le service.
   6. Démarrer l'envoi de l'annonce du service

   rxValue contient les données reçues (only accessible inside that function).
   et txValue contient les données à envoyer .
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ===============================MORSE Declare ==============
const char* TableMorse[] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  de 0x0 à 0x07
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  de 0x08 à 0x0F
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  de 0x10 à 0x17
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, //  de 0x18 à 0x1F
  // space, !, ", #, $, %, &, '
  "s", "-.-.--", ".-..-.", NULL, NULL, NULL, NULL, ".----.",
  // ( ) * + , - . /
  "-.--.", "-.--.-", NULL, ".-.-.", "--..--", "-....-", ".-.-.-", "-..-.",
  // 0 1 2 3 4 5 6 7
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...",
  // 8 9 : ; < = > ?
  "---..", "----.", "---...", "-.-.-.", NULL, "-...-", NULL, "..--..",
  // @ A B C D E F G
  ".--.-.", ".-", "-...", "-.-.", "-..", ".", "..-.", "--.",
  // H I J K L M N O
  "....", "..", ".---", "-.-", ".-..", "--", "-.", "---",
  // P Q R S T U V W
  ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--",
  // X Y Z [ \ ] ^ _
  "-..-", "-.--", "--..", NULL, NULL, NULL, NULL, "..--.-",
  // ' a b c d e f g
  NULL, ".-", "-...", "-.-.", "-..", ".", "..-.", "--.",
  // h i j k l m n o
  "....", "..", ".---", "-.-", ".-..", "--", "-.", "---",
  // p q r s t u v w
  ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--",
  // x y z { | } ~ DEL
  "-..-", "-.--", "--..", NULL, NULL, NULL, NULL, NULL, // de 0x78 à 0x7F
};

const int LED = 2; // 2 For MHetLive Board;   25 For Heltec Wifi LoRa Board.
const int BUZZER = 22 ;

#define ALLUME LOW
#define ETEINT HIGH
#define VRAI 1
#define FAUX 0


int buflen ;
char buf[80] ;
int veille = VRAI;

//    Règles du Morse
// 1. Un Tiret est égal à trois Points.
// 2. L’espacement entre deux éléments d’une même lettre est égal à un Point.
// 3. L’espacement entre deux lettres est égal à trois Points.
// 4. L’espacement entre deux mots est égal à sept Points.

int Point_Duree = 30 ;
int Tiret_Duree = Point_Duree * 3;  // Le Tiret dure 3 fois la durée du Point
int k ;

void Point()   // génére le son point sur le buzzer
{
  digitalWrite(LED, ALLUME);   // La LED est allumé le temps de l'émission du Point

  for (k = 0; k < Point_Duree; k++) // Durée de l'emission d'un point
  {
    digitalWrite(BUZZER, HIGH); // Emission d'un son à 500 Hz avec 1 ms haut pour la sortie Buzzer 
    delay(1);//delay 1 ms
    digitalWrite(BUZZER, LOW); //   et 1 ms bas pour la sortie Buzzer 
    delay(1);// delay 1 ms
  }
  digitalWrite(LED, ETEINT);   // La LED est eteinte à la fin de l'emission
 
}

void Tiret()
{
  digitalWrite(LED, ALLUME);

  for (k = 0; k < Tiret_Duree; k++) // Durée de l'emission d'un tiret
  {
    digitalWrite(BUZZER, HIGH); // Emission d'un son à 500 Hz avec 1 ms haut pour la sortie Buzzer 
    delay(1);//delay 1 ms
    digitalWrite(BUZZER, LOW); // et 1 ms bas pour la sortie Buzzer 
    delay(1);// delay 1 ms
  }
  digitalWrite(LED, ETEINT);
 
}



void EmettreTiretPoint(const char * morseCode)
{
  int i = 0;
  while (morseCode[i] != 0)
  {
    if (morseCode[i] == '.') {
      Point();delay(Point_Duree);      // Règle 2
    } else if (morseCode[i] == '-') {
      Tiret();delay(Point_Duree);     // Règle 2
    }
    else if (morseCode[i] == 's') {  // espace entre 2 mots (caractère blanc), rien est émis par le Buzzer
      digitalWrite(LED, ETEINT);     // La LED est éteinte
      delay(Point_Duree * 7);    // Règle 4 
    }
    i++;
  }
}

// ================================================

BLECharacteristic *pCharacteristic;

bool deviceConnected = false;
int txValue = 0;

std::string rxValue; // Chaine de caractère C++ reçue

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



class Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      rxValue = pCharacteristic->getValue();


      if (rxValue.length() > 0) {
        Serial.println("");
        Serial.print("*** Received : ");

        //  Pour des besoins de tests, impression sur la console Arduino du texte reçu dans la variable rxValue
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();

        // Si le 1er caractère reçu est 0, le programme passe en veille
        if (rxValue[0] == char('0')) veille = VRAI ;
        else veille = FAUX;

      }
    }
};




void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
 

  pinMode(BUZZER, OUTPUT); // Mettre en mode sortie 


  // Creation du périphérique BLE 
  BLEDevice::init("BLE ESP32"); // création et initialisation avec BLE ESP32 comme nom de périphérique

  // Crée le Serveur BLE
  BLEServer *pServer = BLEDevice::createServer();

  if (pServer == 0)  Serial.println("Create server fails");

  pServer->setCallbacks(new ServerCallbacks());

  // Creation du Service BLE
  BLEService *pService = pServer->createService(SERVICE_UUID);
  if (pService == 0)  Serial.println("Create service fails");

  // Creation de la caracteristique BLE en envoi (TX) 
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  if (pService == 0)  Serial.println("Create characteristic TX fails");

  pCharacteristic->addDescriptor(new BLE2902());


  // Creation de la caracteristique BLE en récception (RX)
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  if (pService == 0)  Serial.println("Create characteristic RX fails");

  pCharacteristic->setCallbacks(new Callbacks());

  // Démarrer le service
  pService->start();

  // Commencer l'annonce du service
  pServer->getAdvertising()->start();

  Serial.println("V0.1 Attente de la connexion d'un client bluetooth");


  digitalWrite(LED, ETEINT);

}

// ==========================================================


void loop() {
  if (deviceConnected) {
    if (veille == FAUX ) {
      // iteration...
      txValue += 1 ; // txValue represente le nombre de message Morse émis par le Buzzer

      // Conversion de l’entier txValue en chaine de caractère ASCII 
      char txString[8]; // make sure this is big enuffz
      itoa(txValue, txString, 6); // 

      pCharacteristic->setValue(txString);

      pCharacteristic->notify(); // Envoie la valeur à l'application Smartphone.

      Serial.print("\n*** Sent Value: ");
      Serial.print(txValue);
      Serial.println(" ***");



      char ch; int index;


      strcpy(buf ,  rxValue.c_str());   // une copie des caractères reçus est faite dans buf
      buflen = strlen(buf);             // calcul de la longueur de la chaine de caractères

      //

      for ( index = 0 ; index < buflen ; index++ )    // Parcours caractère par caractère de la chaine reçue
      {
        ch = buf[index];                // le caractère à convertir en code Morse
        Serial.print(ch);
        if (ch == 0x85) ch = 'a';                   //  On remplace le caractère à par par le caractère a
        if ((ch == 0x82 || ch == 0x8A )) ch = 'e';    //  On remplace les caractères é et è par par le caractère e
        if (ch >= 0x7F) ch = '?';                  // Les caractères de code ASCII > ou = à 127 sont remplacés par  ?
        EmettreTiretPoint(TableMorse[ch]);         // Utilisation du tableau TableMorse
        delay(Point_Duree*3);
      }
      delay(Point_Duree * 7);        // fin du dernier mot reçu


    }
    else {
      delay(1000);

    }
  }
}
