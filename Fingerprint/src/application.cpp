/*
Fingerprint and RFID reader with Digole display

Uses EDB  database library to store Username data, RFID tag numbers and Fingerprint addresses.

 */

#define _Digole_Serial_SPI_

#include "application.h"
#include "SD.h"
#include "EDB.h"
#include "DigoleSerialDisp.h"
#include "Adafruit_Fingerprint.h"
#include "spark_disable_wlan.h"
#include "spark_disable_cloud.h"



//Digole wires
//VCC to 3v3
//Data to A5 MOSI
//Clock to A3 Clock
//SS to A1 chip select(or change below)
//Gnd to Gnd

DigoleSerialDisp digole(A1); //SPI SS

// Fingerprint reader Wires
// RX on spark GREEN wire (Serial1)
// TX on  spark WHITE wire (Serial1)
// 3v3 on spark RED wire
// GND on spark BLACK wire

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

// SD Card Wiring
// From the angle part of SD card
// pin on the angle is NC
// Chip select
// MOSI
// GND
// VCC
// Clock
// GND
// MISO

const uint8_t chipSelect = A0;   
const uint8_t mosiPin = A5;
const uint8_t misoPin = A4;
const uint8_t clockPin = A3;

// Wifi cloud and time stuff
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define FIVE_MINUTES (1 * 60 * 1000) //Set at 1 min for testing
unsigned long lastSync = millis();
unsigned long lastWifi = millis();
WiFi_Status_TypeDef last_wifi_status;

//  File names for DB files and log file, Size of table to create.
char RFIDFile[] = "rfid.db";
char FingerFile[] = "Finger.db";
char UsernameFile[] = "User.db";
char LogFile[] = "log.txt";
File dbFile;  

// DB File Sizes 

#define USERNAME_TABLE_SIZE 4096
#define RFID_TABLE_SIZE 512
#define FINGER_TABLE_SIZE 512

// Log file codes
#define Delete 1
#define Add 2
#define Update 3
#define Access 4
#define RFID 1
#define Finger 2
#define User 3

// PC serial com port

#define BAUD_RATE 115200
unsigned long timer;
#define TIMEOUT 20000 // 20 seconds
unsigned long active;
#define INACTIVITY_TIME 120000 // 2 mins


boolean ProgMode = false;
boolean Menu = false;
uint8_t Option = 0;
boolean scanDisplay = false;

int recno = 1;
char recnoascii[2];
int pos = 0;
char inChar;

struct RFIDtagID {
  uint8_t Usernumber;
  uint8_t Tagnumber[5];
}
RFIDtagID;

int RFIDtagIDcount;
uint8_t RFIDTagSearch[5];

struct FingerprintID {
    uint8_t Usernumber;
    uint8_t Address;
}
FingerprintID;

int FingerprintIDcount;
uint8_t FingerprintAddressSearch;


struct Username {
    uint8_t Usernumber;
    char Firstname[20];
    char Surname[20];
    char Welcome[60];
    uint8_t Rights;
}
Username;
 
int Usernamecount;
uint8_t UsernumberSearch;

char inSong[400];
char *inSongptr;

void writer(unsigned long address, byte data)
{
  dbFile.seek(address);
  dbFile.write(data);
  dbFile.flush();
}
 
byte reader(unsigned long address)
{
  dbFile.seek(address);
  return dbFile.read();
}
 
EDB db(&writer, &reader);

// RTTTL Setup
char *denied = (char *)"NO:d=32,o=6,b=100:4a,a,2p";
char *allowed = (char *)"YES:d=8,o=5,b=112:c6,4f6";

// Select a Doorbell tune

//char *song = (char *)"The Simpsons:d=4,o=5,b=168:c.6,e6,f#6,8a6,g.6,e6,c6,8a,8f#,8f#,8f#,2g,8p,8p,8f#,8f#,8f#,8g,a#.,8c6,8c6,8c6,c6";
//char *song = (char *)"Indiana:d=4,o=5,b=250:e,8p,8f,8g,8p,1c6,8p.,d,8p,8e,1f,p.,g,8p,8a,8b,8p,1f6,p,a,8p,8b,2c6,2d6,2e6,e,8p,8f,8g,8p,1c6,p,d6,8p,8e6,1f.6,g,8p,8g,e.6,8p,d6,8p,8g,e.6,8p,d6,8p,8g,f.6,8p,e6,8p,8d6,2c6";
//char *song = (char *)"TakeOnMe:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5,8f#5,8e5,8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5";
//char *song = (char *)"Entertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,2c.6,8c6,8d6,8d#6,8e6,8c6,8d6,e6,8b,d6,2c6,p,8d,8d#,8e,c6,8e,c6,8e,2c.6,8p,8a,8g,8f#,8a,8c6,e6,8d6,8c6,8a,2d6";
//char *song = (char *)"Muppets:d=4,o=5,b=250:c6,c6,a,b,8a,b,g,p,c6,c6,a,8b,8a,8p,g.,p,e,e,g,f,8e,f,8c6,8c,8d,e,8e,8e,8p,8e,g,2p,c6,c6,a,b,8a,b,g,p,c6,c6,a,8b,a,g.,p,e,e,g,f,8e,f,8c6,8c,8d,e,8e,d,8d,c";
//char *song = (char *)"Xfiles:d=4,o=5,b=125:e,b,a,b,d6,2b.,1p,e,b,a,b,e6,2b.,1p,g6,f#6,e6,d6,e6,2b.,1p,g6,f#6,e6,d6,f#6,2b.,1p,e,b,a,b,d6,2b.,1p,e,b,a,b,e6,2b.,1p,e6,2b.";
//char *song = (char *)"Looney:d=4,o=5,b=140:32p,c6,8f6,8e6,8d6,8c6,a.,8c6,8f6,8e6,8d6,8d#6,e.6,8e6,8e6,8c6,8d6,8c6,8e6,8c6,8d6,8a,8c6,8g,8a#,8a,8f";
//char *song = (char *)"20thCenFox:d=16,o=5,b=140:b,8p,b,b,2b,p,c6,32p,b,32p,c6,32p,b,32p,c6,32p,b,8p,b,b,b,32p,b,32p,b,32p,b,32p,b,32p,b,32p,b,32p,g#,32p,a,32p,b,8p,b,b,2b,4p,8e,8g#,8b,1c#6,8f#,8a,8c#6,1e6,8a,8c#6,8e6,1e6,8b,8g#,8a,2b";
//char *song = (char *)"Bond:d=4,o=5,b=80:32p,16c#6,32d#6,32d#6,16d#6,8d#6,16c#6,16c#6,16c#6,16c#6,32e6,32e6,16e6,8e6,16d#6,16d#6,16d#6,16c#6,32d#6,32d#6,16d#6,8d#6,16c#6,16c#6,16c#6,16c#6,32e6,32e6,16e6,8e6,16d#6,16d6,16c#6,16c#7,c.7,16g#6,16f#6,g#.6";
//char *song = (char *)"MASH:d=8,o=5,b=140:4a,4g,f#,g,p,f#,p,g,p,f#,p,2e.,p,f#,e,4f#,e,f#,p,e,p,4d.,p,f#,4e,d,e,p,d,p,e,p,d,p,2c#.,p,d,c#,4d,c#,d,p,e,p,4f#,p,a,p,4b,a,b,p,a,p,b,p,2a.,4p,a,b,a,4b,a,b,p,2a.,a,4f#,a,b,p,d6,p,4e.6,d6,b,p,a,p,2b";
//char *song = (char *)"StarWars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6";
//char *song = (char *)"UncleFuc:d=4,o=6,b=160:8e,8e,8e,8e,e,8d,8c,8a5,2c,8p,8c,8d,8e,8e,8e,8e,8e,8e,8d,8c,8a5,2c,8p,8c,8c,8d,8d,8d,8b5,8c,8d,8e,16p,16e,16f,16f,8e,8d,8c,8b5,8c,8d";
//char *song = (char *)"GoodBad:d=4,o=5,b=56:32p,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,d#,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,c#6,32a#,32d#6,32a#,32d#6,8a#.,16f#.,32f.,32d#.,c#,32a#,32d#6,32a#,32d#6,8a#.,16g#.,d#";
//char *song = (char *)"TopGun:d=4,o=4,b=31:32p,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,16f,d#,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,g#";
//char *song = (char *)"A-Team:d=8,o=5,b=125:4d#6,a#,2d#6,16p,g#,4a#,4d#.,p,16g,16a#,d#6,a#,f6,2d#6,16p,c#.6,16c6,16a#,g#.,2a#";
//char *song = (char *)"Flinstones:d=4,o=5,b=40:32p,16f6,16a#,16a#6,32g6,16f6,16a#.,16f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c6,d6,16f6,16a#.,16a#6,32g6,16f6,16a#.,32f6,32f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c6,a#,16a6,16d.6,16a#6,32a6,32a6,32g6,32f#6,32a6,8g6,16g6,16c.6,32a6,32a6,32g6,32g6,32f6,32e6,32g6,8f6,16f6,16a#.,16a#6,32g6,16f6,16a#.,16f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c.6,32d6,32d#6,32f6,16a#,16c.6,32d6,32d#6,32f6,16a#6,16c7,8a#.6";
//char *song = (char *)"Jeopardy:d=4,o=6,b=125:c,f,c,f5,c,f,2c,c,f,c,f,a.,8g,8f,8e,8d,8c#,c,f,c,f5,c,f,2c,f.,8d,c,a#5,a5,g5,f5,p,d#,g#,d#,g#5,d#,g#,2d#,d#,g#,d#,g#,c.7,8a#,8g#,8g,8f,8e,d#,g#,d#,g#5,d#,g#,2d#,g#.,8f,d#,c#,c,p,a#5,p,g#.5,d#,g#";
//char *song = (char *)"Gadget:d=16,o=5,b=50:32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,32d#,32f,32f#,32g#,a#,d#6,4d6,32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,8d#";
//char *song = (char *)"Smurfs:d=32,o=5,b=200:4c#6,16p,4f#6,p,16c#6,p,8d#6,p,8b,p,4g#,16p,4c#6,p,16a#,p,8f#,p,8a#,p,4g#,4p,g#,p,a#,p,b,p,c6,p,4c#6,16p,4f#6,p,16c#6,p,8d#6,p,8b,p,4g#,16p,4c#6,p,16a#,p,8b,p,8f,p,4f#";
//char *song = (char *)"MahnaMahna:d=16,o=6,b=125:c#,c.,b5,8a#.5,8f.,4g#,a#,g.,4d#,8p,c#,c.,b5,8a#.5,8f.,g#.,8a#.,4g,8p,c#,c.,b5,8a#.5,8f.,4g#,f,g.,8d#.,f,g.,8d#.,f,8g,8d#.,f,8g,d#,8c,a#5,8d#.,8d#.,4d#,8d#.";
//char *song = (char *)"LeisureSuit:d=16,o=6,b=56:f.5,f#.5,g.5,g#5,32a#5,f5,g#.5,a#.5,32f5,g#5,32a#5,g#5,8c#.,a#5,32c#,a5,a#.5,c#.,32a5,a#5,32c#,d#,8e,c#.,f.,f.,f.,f.,f,32e,d#,8d,a#.5,e,32f,e,32f,c#,d#.,c#";
//char *song = (char *)"Mission Impossible:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,";//32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d";
char *song = (char *)"CircusTh:d=32,o=6,b=65:16c#,16c,b5,c,b5,a#5,16a5,16g#5,16g5,16g#5,16a#5,16a5,g#5,a5,g#5,g5,16f#5,16f5,16e5,16f5,16g#5,f#5,f#5,16d5,16d#5,16g#5,f#5,f#5,16d5,16d#5,c,c#,d,d#,e,f,f#,g,g#,a,a#,b,16a#,16g#,16c#,16c,b5,c,b5,a#5,16a5,16g#5,16g5,16g#5,16a#5,16a5,g#5,a5,g#5,g5,16f#5,16f5,16e5,16f5,16e5,e5,e5,8g5,g#5,a#5,g#5,g5,16f5,16g#5,16c,c,c,16c,16c,c,c,c,c,c,c,c";
//char *song = (char *)"SMBunderground:d=16,o=6,b=100:c,c5,a5,a,a#5,a#,2p,8p,c,c5,a5,a,a#5,a#,2p,8p,f5,f,d5,d,d#5,d#,2p,8p,f5,f,d5,d,d#5,d#,2p,32d#,d,32c#,c,p,d#,p,d,p,g#5,p,g5,p,c#,p,32c,f#,32f,32e,a#,32a,g#,32p,d#,b5,32p,a#5,32p,a5,g#5";
//char *song = (char *)"SMBwater:d=8,o=6,b=225:4d5,4e5,4f#5,4g5,4a5,4a#5,b5,b5,b5,p,b5,p,2b5,p,g5,2e.,2d#.,2e.,p,g5,a5,b5,c,d,2e.,2d#,4f,2e.,2p,p,g5,2d.,2c#.,2d.,p,g5,a5,b5,c,c#,2d.,2g5,4f,2e.,2p,p,g5,2g.,2g.,2g.,4g,4a,p,g,2f.,2f.,2f.,4f,4g,p,f,2e.,4a5,4b5,4f,e,e,4e.,b5,2c.";
//char *song = (char *)"SMBdeath:d=4,o=5,b=90:32c6,32c6,32c6,8p,16b,16f6,16p,16f6,16f.6,16e.6,16d6,16c6,16p,16e,16p,16c";
//char *song = (char *)"RickRoll:d=4,o=5,b=200:8g,8a,8c6,8a,e6,8p,e6,8p,d6.,p,8p,8g,8a,8c6,8a,d6,8p,d6,8p,c6,8b,a.,8g,8a,8c6,8a,2c6,d6,b,a,g.,8p,g,2d6,2c6.,p,8g,8a,8c6,8a,e6,8p,e6,8p,d6.,p,8p,8g,8a,8c6,8a,2g6,b,c6.,8b,a,8g,8a,8c6,8a,2c6,d6,b,a,g.,8p,g,2d6,2c6.";
//char *song = (char *)"2.34kHzBeeps:d=4,o=7,b=240:d,p,d,p,d,p,d,p";
//char *song = (char *)"Satellite:d=8,o=6,b=112:g,e,16f,16e,d,c,g,c7,c";

int16_t tonePin = D6; //Doorbell Sound Output 

int notes[] =
{0,
3817,3597,3401,3205,3030,2857,2703,2551,2404,2273,2146,2024,
1908,1805,1701,1608,1515,1433,1351,1276,1205,1136,1073,1012,
956,903,852,804,759,716,676,638,602,568,536,506,
478,451,426,402,379,358,338,319,301,284,268,253,
239,226,213,201,190,179,169,159,151,142,134,127};

#define OCTAVE_OFFSET 0 

boolean playing = false;
byte default_dur = 4;
byte default_oct = 6;
byte lowest_oct = 3;
int bpm = 63;
int num;
long wholenote;
long duration;
byte note;
byte scale;
bool songDone = false;
char *songPtr;


// Doobell Stuff

int doorbell = D0; // Doorbell button connects to D0 on the core
int LED = D7; // Built in LED
volatile boolean doorbellPressed = false;
boolean DEBUG = false; // enable or disable Debug
int Retrys = 5; // Number of times to retry connecting

// XBMC Setup
char xbmcServer[] = "Server"; // XBMC Server
int xbmcPort = 10085; // XBMC Port
char xbmcScript[] = "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Addons.ExecuteAddon%22,%22params%22:{%22addonid%22:%22script.securitycam%22},%22id%22:%221%22}}"; // Script to run
char xbmcLogon[] = "Username:Password"; // Username:Password base64 encoded


// Pushing Box Setup

char serverName[] = "api.pushingbox.com"; // Pushingbox Server
char deviceID[] = "/pushingbox?devid=YourDEVID"; // Get request 

void doorbellpressed(void);
void playDoorbell(void);
void checkInactivity(void);
void checkWifi(void);
void syncTime(void);
void printmenu(void);
int toggleProgMode(void);
void readString(char *ptr, int length);
char readSelection(char);
char readSelection(void);
void createDB(void);
void Log(char LogText[]);
void Log(uint8_t Action, uint8_t database);
void PrintLog(void);
void printDB(void);
void printRFIDFile(void);
void printFingerFile(void);
void printUsernameFile(void);
boolean getUsername(void);
void adduser(void);
void addrfidcard(void);
void addfingerprint(void);
int getFingerprintID(void);
uint8_t getFingerprintEnroll(uint8_t id);
int searchDB(void);
void setMaster(void);
void begin_rtttl(char *p);
bool next_rtttl();
void tones(int pin, int16_t note, int16_t duration);
void playSong(int a);
void inputSong(void);
void runXbmcScript(char *hostname, int port, char *url, char *xbmclogon, int retry);
void sendToPushingBox(char *serverpbName, char *devid, int retry);


void setup() {
    
    pinMode(tonePin,OUTPUT);
    pinMode(doorbell, INPUT_PULLUP); // other side of doorbell will connect to ground to pull low
    attachInterrupt(doorbell, doorbellpressed, FALLING);
    pinMode(LED, OUTPUT);
    
    Time.zone(+8);
    
    digole.begin();
    delay(1500);
    digole.disableCursor();
    digole.clearScreen();
    delay(50);
    digole.setRot90();
    digole.setFont(10);
    digole.println("Booting Up");
        
    inSongptr = inSong;
    
    if (ProgMode) {
        digole.println("Opening serial port..");
        timer = millis();
        Serial.begin(BAUD_RATE);
        while (!Serial.available() && millis() <= timer + TIMEOUT);
        if (millis() >= timer + TIMEOUT) {
                ProgMode = false;
                digole.println("Timed Out");
                Serial.end();
            }
        while (Serial.available()) {Serial.read();}//throw it away
        if (ProgMode) {
            Serial.println("Serial port started");
            digole.println("Opened");
            active = millis();
        }
    }
    
    if (ProgMode) Serial.print("Attempting to connect to the internet...");
    digole.println("WiFi Connecting..");
    timer = millis();
    WiFi.on();
    while (WiFi.status() != WIFI_ON && millis() <= timer + TIMEOUT) SPARK_WLAN_Loop();
    if (millis() >= timer + TIMEOUT) {
        WiFi.off();
        if (ProgMode) Serial.println("Unable to Connect WiFi");
        digole.println("..no WiFi");
        lastWifi = millis();
        
    }
    if (WiFi.status() == WIFI_ON) {
        Spark.connect();
        while (!Spark.connected() && millis() <= timer + TIMEOUT) SPARK_WLAN_Loop();
            if (millis() >= timer + TIMEOUT) { //Timer now covers both to stop a 40 sec boot-up
                WiFi.off();
                if (ProgMode) Serial.println("Unable to connect to the Spark Cloud");
                digole.println("..no Internet");
                lastWifi = millis();
            }
        
    }
    
    if (Spark.connected()) {
        if (ProgMode) Serial.println("Connected to the Cloud");
        digole.println("..Connected");
        digole.print("SSID: ");
        digole.println(Network.SSID());
        digole.print("IP: ");
        digole.println(Network.localIP());
        digole.print("GW: ");
        digole.println(Network.gatewayIP());
    }
        
    if (ProgMode) Serial.print("Initializing SD card...");
    digole.print("Starting SDcard..");
    delay(200);
    if (!SD.begin(chipSelect)) {
        if (ProgMode) Serial.println("SD initialization failed!");
        digole.print("Fail");
    }
    else{
    if (ProgMode) Serial.println("SD initialization done.");
    
    dbFile = SD.open(LogFile, FILE_WRITE);
    if (dbFile) {  
        dbFile.print('>');
                
        if (Time.now() >= 1400000000) {
            dbFile.printUntil(Time.timeStr(), 0x0a); 
        } else {
            dbFile.print("Clock Not Set");
        }
        dbFile.print(" - ");
        dbFile.println("*** Spark Core Booted up ***");
        dbFile.flush();
        dbFile.close();
    } else {
        if (ProgMode) Serial.println("Unable to open or create LogFile");
    }
    }
    digole.println("Done");
    
    digole.print("Starting fingerprint reader...");
    // set the data rate for the fingerprint sensor serial port
    finger.begin(57600);
	
    if (finger.verifyPassword()) {
	if (ProgMode) Serial.println("Found fingerprint sensor!");
        digole.println("Done");
    } else {
	if (ProgMode) Serial.println("Did not find fingerprint sensor :(");
        digole.print("Failed");
    }
        
    delay(1000);
    digole.clearScreen();
    

}

void loop() {
    
    checkWifi();
    syncTime();
    checkInactivity();

    
    if (doorbellPressed) playDoorbell();
    
    if (!ProgMode) searchDB();
    else {
        
        if (!Menu) printmenu();
        
        if (Serial.available()) {
            char c = readSelection();
            active = millis();
            switch (c)
            {
              case '0':
                Menu = false;
                createDB();
                break;
              case '1':
                Menu = false;
                printDB();
                break;
              case '2':
                Menu = false;
                adduser();
                break;
	      case '3':
                Menu = false;
                addrfidcard();
                break;
	      case '4':
                Menu = false;
                addfingerprint();
                break;
	      case '5':
                Menu = false;
                timer = millis();
                while (!searchDB() && millis() <= timer + TIMEOUT) SPARK_WLAN_Loop();
                Serial.println();
                break;
              case '6':
                Menu = false;
                inputSong();
                break;
              case '7':
                Menu = false;
                PrintLog();
                break;
              case '9':
                Menu = false;
                setMaster();
                break;
	      default:
                Serial.println("Unknown selection");
                break;
            }
	}
	
    }
    	
}

void doorbellpressed() {
    doorbellPressed = true;
    
}

void playDoorbell() {
    scanDisplay = false;
    Menu = false;
    if (ProgMode) Serial.println("Doorbell Pressed");
    digole.clearScreen();
    digole.setRot90();
    digole.setFont(18);
    digole.setTrueColor(255, 255, 255);
    digole.println("  ");
    digole.println("  ");
    digole.println("   Doorbell       Pressed");
        
    Log((char*)"Doorbell Pressed");
    RGB.control(true);
    RGB.color(0,255,0);
    playSong(3);
    RGB.control(false);
    if (Spark.connected()) {
        runXbmcScript(xbmcServer, xbmcPort, xbmcScript, xbmcLogon, Retrys);  // opens up a script in xbmc that displays an ip camera 
        sendToPushingBox(serverName, deviceID, Retrys); // sends push notification to mobile phone or sends email using pushingbox
    }
    doorbellPressed = false;
       
}

void checkInactivity() {
    
   if (ProgMode && millis() - active > INACTIVITY_TIME) {
    Log((char*)"Exit Program Mode - Inactivity");
    Serial.println("Inactive for 2 mins");
       toggleProgMode();
   }
}

void checkWifi() {
    last_wifi_status = WiFi.status();
    if (millis() - lastWifi > FIVE_MINUTES && !Spark.connected()) {
        
        if (ProgMode) Serial.print("Attempting to connect...");
        timer = millis();
        WiFi.on();
        while (WiFi.status() != WIFI_ON && millis() <= timer + TIMEOUT) SPARK_WLAN_Loop();
        if (millis() >= timer + TIMEOUT) {
                WiFi.off();
                if (ProgMode) Serial.println("Unable to Connect WiFi");
                lastWifi = millis();
                return;
        }
        Spark.connect();
        timer = millis();
        while (!Spark.connected() && millis() <= timer + TIMEOUT) SPARK_WLAN_Loop();
        if (millis() >= timer + TIMEOUT) {
                WiFi.off();
                if (ProgMode) Serial.println("Unable to connect to the Spark Cloud");
                lastWifi = millis();
                return;
        }
        if (ProgMode) Serial.println("Connected to the Cloud");
        
    }
    
}

void syncTime() {
    if (millis() - lastSync > ONE_DAY_MILLIS && Spark.connected()) {
    // Request time synchronization from the Spark Cloud
    Spark.syncTime();
    lastSync = millis();
    }
    
}

void printmenu() {
    
        Serial.print("WiFi: "); 
            if (Spark.connected()) Serial.println("Connected to Cloud"); 
            if (!Spark.connected()) Serial.println(" Off"); 
        Serial.println("  ");
        Serial.println("Main Menu");
        Serial.println("    Press 0 to Create a new DB");
        Serial.println("    Press 1 to print all DB"); 
        Serial.println("    Press 2 to add, delete or update a User");
        Serial.println("    Press 3 to add, delete or update a RFID Tag");
        Serial.println("    Press 4 to add, delete or update a Fingerprint");
        Serial.println("    Press 5 to search");
        Serial.println("    Press 6 to test song");
        Serial.println("    Press 7 to Print Log File");
    

digole.clearScreen();
digole.setRot90();
digole.setFont(10);
digole.setTrueColor(255, 255, 255);
if (WiFi.status() == WIFI_ON) digole.println("WiFi Connected");
if (WiFi.status() == WIFI_OFF) digole.println("No WiFi");
digole.println("  ");
    digole.setFont(18);
    digole.setTrueColor(0, 0 , 255);
    digole.println("  Programming      Mode");


Menu = true;
}

int toggleProgMode() {
        if (!ProgMode){ 
            playSong(1);
            ProgMode = true;
            timer = millis();
            active = millis();
            Log((char*)"Entered Program Mode");
            Serial.begin(BAUD_RATE);
            while (!Serial.available() && millis() <= timer + TIMEOUT);
            if (millis() >= timer + TIMEOUT) {
                ProgMode = false;
                Log((char*)"Exit Program Mode - Timed out");
                Serial.end();
                return 1;
            }
            while (Serial.available()) {Serial.read();}//throw it away
            Serial.println("Serial port started");
            Menu = false;
            
            return 1;
        } else {
            ProgMode = false;
            playSong(2);
            Log((char*)"Exit Program Mode");
            Serial.println("Serial port closed");
            Serial.end();
            
            return 1;
        }
    }

void readString(char *ptr, int length) {
    
    int pos = 0;
    
    while (!Serial.available()) SPARK_WLAN_Loop(); //wait for serial data to come in 
    active = millis();
    while (Serial.available()) {
        
        inChar = Serial.read();
        if (inChar == 0x0A || inChar == 0x0D)
            break;
        ptr[pos] = inChar;
        pos++;
        if (pos >= 50) delay(100);
        if (pos >= 100) delay(50);
        
        if (pos >= length-1)
            break;
    }
    ptr[pos] = '\0';

    while (Serial.available())
        (void)Serial.read(); //throw it away

    return;
}

char readSelection() {

	while(!Serial.available()) SPARK_WLAN_Loop();
	
	inChar = Serial.read();
        active = millis();
	while (Serial.available()) 
		(void)Serial.read();//throw it away

	return inChar;
}

void createDB() {
    
    Serial.println("\n\rCreating new DB files will Delete old files");
    Serial.println("Press 1 to continue or any other key to cancel");
	char c = readSelection();
	switch (c)
	{
	    case '1':
	    Serial.println("Creating new Files");
	    break;
	    default:
	    Serial.println("Aborted. Exiting now");
	    delay(1000);
	    return;
	    
	}
	SD.begin(chipSelect);
	if (SD.exists(RFIDFile)) {
    Serial.println("RFID file exists, Deleting now");
    SD.remove(RFIDFile);
	}
    Serial.print("Creating new RFID file now");
    dbFile = SD.open(RFIDFile, FILE_WRITE);
	if (dbFile) { 
	    Serial.println(", creating DB now");
		db.create(0, RFID_TABLE_SIZE, sizeof(RFIDtagID));
		Serial.print("Size of RFIDtagID: "); Serial.println(sizeof(RFIDtagID));	
	    Serial.print("Record Count: "); Serial.println(db.count());
		Serial.print("Maximum Records: "); Serial.println((RFID_TABLE_SIZE / sizeof(RFIDtagID)));
		dbFile.close();
	}
    
    
    if (SD.exists(FingerFile)) {
    Serial.println("Fingerprint file exists, Deleting now");
    SD.remove(FingerFile);
    }
    Serial.print("Creating new Fingerprint file now");
    dbFile = SD.open(FingerFile, FILE_WRITE);
	if (dbFile) { 
	    Serial.println(", creating DB now");
		db.create(0, FINGER_TABLE_SIZE, sizeof(FingerprintID));
		Serial.print("Size of FingerprintID: "); Serial.println(sizeof(FingerprintID));	
	    Serial.print("Record Count: "); Serial.println(db.count());
		Serial.print("Maximum Records: "); Serial.println((FINGER_TABLE_SIZE / sizeof(FingerprintID)));
		dbFile.close();
	}
    
    
    if (SD.exists(UsernameFile)) {
    Serial.println("Username file exists, Deleting now");
    SD.remove(UsernameFile);
    }
    Serial.print("Creating new Username file now");
    dbFile = SD.open(UsernameFile, FILE_WRITE);
	if (dbFile) { 
	    Serial.println(", creating DB now");
		db.create(0, USERNAME_TABLE_SIZE, sizeof(Username));
		Serial.print("Size of FingerprintID: "); Serial.println(sizeof(Username));	
	    Serial.print("Record Count: "); Serial.println(db.count());
		Serial.print("Maximum Records: "); Serial.println((USERNAME_TABLE_SIZE / sizeof(Username)));
		dbFile.close();
	}
    Serial.print("Wiping Fingerprint reader database.. "); Serial.println(finger.emptyDatabase());
    Log((char*)"New database created");
    Serial.println("Done creating Database Files, Returning to main menu");
       
    Menu = false;
    return;
    
}

void Log(char LogText[]){
    
    SD.begin(chipSelect);
    if (SD.exists(LogFile)) {
            dbFile = SD.open(LogFile, FILE_WRITE);
            if (dbFile) { 
                dbFile.print('>');
                
                if (Time.now() >= 1400000000) {
                    dbFile.printUntil(Time.timeStr(), 0x0a);
                } else {
                    dbFile.print("Clock Not Set");
                }
                dbFile.print(" - ");
                dbFile.println(LogText);
                
                dbFile.flush();
                dbFile.close();
                return;
            } else {
                if (ProgMode) Serial.println("Unable to open LogFile");
            }
        } else {
            if (ProgMode) Serial.println("Unable to find LogFile");
        } 
     
    return;
    
}

void Log(uint8_t Action, uint8_t database){
    
    SD.begin(chipSelect);
    if (SD.exists(LogFile)) {
        dbFile = SD.open(LogFile, FILE_WRITE);
        if (dbFile) {  
            dbFile.print('>');
            if (Time.now() >= 1400000000) {
                dbFile.printUntil(Time.timeStr(), 0x0a); 
            } else {
                dbFile.print("Clock Not Set");
            }
            dbFile.print(" - ");
            if (Action == 1) dbFile.print("Deleted");
            if (Action == 2) dbFile.print("Added");
            if (Action == 3) dbFile.print("Updated");
            if (Action == 4) dbFile.print("Access Granted");
    
            dbFile.print(" - ");
            if (database == RFID){
                dbFile.print("RFID to User number: ");
                dbFile.println(RFIDtagID.Usernumber);
        
            } else if (database == Finger){
                dbFile.print("Fingerprint to User number: ");
                dbFile.print(FingerprintID.Usernumber);
                dbFile.print(", Address: ");
                dbFile.println(FingerprintID.Address);
        
            } else if (database == User){
                dbFile.print("User Number: ");
                dbFile.print(Username.Usernumber);
                dbFile.print(" - ");
                dbFile.print(Username.Firstname);
                dbFile.print(' ');
                dbFile.println(Username.Surname);
        
            }
            
                dbFile.flush();        
                dbFile.close();
                return;
            } else {
                if (ProgMode) Serial.println("Unable to open LogFile");
            }
        } else {
            if (ProgMode) Serial.println("Unable to find LogFile");
        }
    
    return;   
}

void PrintLog() {
    SD.begin(chipSelect);
    Serial.println("Log File");
    Serial.println(" ");
    if (SD.exists(LogFile)) {
            dbFile = SD.open(LogFile);
            if (dbFile) { 
                while (dbFile.available()){
                Serial.print(dbFile.readStringUntil(0x0d));
                }
                
                dbFile.close();
                return;
            } else {
                Serial.println("Unable to open LogFile");
            }
        } else {
            Serial.println("Unable to find LogFile");
        }
     
}
  
void printDB() {
    printRFIDFile();
    printFingerFile();
    printUsernameFile();
    return;
}

void printRFIDFile() {
    SD.begin(chipSelect);
    if (SD.exists(RFIDFile)) {
    dbFile = SD.open(RFIDFile, FILE_WRITE);
    if (dbFile) {  
        db.open(0);
        Serial.print("RFIF Database     Record Count: "); Serial.println(db.count());
        RFIDtagIDcount = (db.count());
		unsigned int recordnumber;
		for (recordnumber = 1; recordnumber < (db.count() + 1); recordnumber++)
		{
			db.readRec(recordnumber, EDB_REC RFIDtagID);
			Serial.print("User ID Number: "); Serial.print(RFIDtagID.Usernumber);
			Serial.print("RFID Tag Number: "); 
			int i;
			for(i = 0; i <= 4; i++)
            {
            Serial.print(RFIDtagID.Tagnumber[i],HEX);
            Serial.print(" ");
			}
			Serial.println();
		}
		dbFile.close();
    }
    else {
        Serial.print("Unable to open RFIDFile");
    }
    }
    else {
        Serial.print("Unable to find RFIDFile");
    }
    return;
}

void printFingerFile() {
    SD.begin(chipSelect);
    if (SD.exists(FingerFile)) {
    dbFile = SD.open(FingerFile, FILE_WRITE);
    if (dbFile) {  
        db.open(0);
        Serial.print("Fingerprint Database     Record Count: "); Serial.println(db.count());
        FingerprintIDcount = (db.count());
		unsigned int recordnumber;
		for (recordnumber = 1; recordnumber < (db.count() + 1); recordnumber++)
		{
			db.readRec(recordnumber, EDB_REC FingerprintID);
			Serial.print("User ID Number: "); Serial.print(FingerprintID.Usernumber);
			Serial.print(" Fingerprint Address: "); Serial.println(FingerprintID.Address);
            
		}
		dbFile.close();
    }
    else {
        Serial.print("Unable to open FingerFile");
    }
    }
    else {
        Serial.print("Unable to find FingerFile");
    }
    return;
}

void printUsernameFile() {
    SD.begin(chipSelect);
    if (SD.exists(UsernameFile)) {
    dbFile = SD.open(UsernameFile, FILE_WRITE);
    if (dbFile) {  
        db.open(0);
        Serial.print("Username Database     Record Count: "); Serial.println(db.count());
        Usernamecount = (db.count());
		unsigned int recordnumber;
		for (recordnumber = 1; recordnumber < (db.count() + 1); recordnumber++)
		{
			db.readRec(recordnumber, EDB_REC Username);
			Serial.print("User ID: "); Serial.print(Username.Usernumber);
			Serial.print(" User name: "); Serial.print(Username.Firstname); Serial.print(" "); Serial.print(Username.Surname);
			Serial.print(" Welcome Message: "); Serial.println(Username.Welcome);
    
		}
		dbFile.close();
    }
    else {
        Serial.print("Unable to open UsernameFile");
    }
    }
    else {
        Serial.print("Unable to find UsernameFile");
    }
    return;
}

boolean getUsername() {
printUsernameFile();
    Serial.println("Select user from List:");
	readString(recnoascii, 2);
    recno = atoi(recnoascii);
        
        if ( recno >= 0 && recno <= Usernamecount) {
            if (SD.exists(UsernameFile)) {
                dbFile = SD.open(UsernameFile, FILE_WRITE);
                if (dbFile) {  
                    db.open(0);
                    db.readRec(recno, EDB_REC Username);
                    dbFile.close();
                    Serial.print("User ID: "); Serial.print(Username.Usernumber);
		    Serial.print(" User name: "); Serial.print(Username.Firstname);
                    Serial.print(" ");Serial.println(Username.Surname);
                }
                
            }
        }
        else {
            Serial.println("Invalid User");
            return false;
        } 
        
        Serial.print("Press 1 to confirm user or any other key to cancel...");
        char c = readSelection();
	    switch (c)
	    {
	        case '1':
	        Serial.println("Confirmed");
	        return true;
	        default:
	        Serial.println("Aborted. Exiting now");
			delay(1000);
    	    return false;
	    
	    }
}

void adduser() {
    printUsernameFile();
    Serial.println();
    Serial.println("Press 1 to Add a new User");
    Serial.println("Press 2 to Update a User");
    Serial.println("Press 3 to Delete a User");
    Serial.println("Any other key to cancel");
    char c = readSelection();
	switch (c)
	{
	    case '1':
	    Option = 1;
	    break;
	    case '2':
	    Option = 2;
	    break;
	    case '3':
	    Option = 3;
	    break;
	    default:
	    Serial.println("Aborted. Exiting now");
	    delay(1000);
	    return;
	    
	}
	if (Option >= 2) {
        Serial.println("Select user from List:");
		readString(recnoascii, 2);
        recno = atoi(recnoascii);
        Username.Usernumber = recno;
        if (Option == 3) Serial.print("Deleting User number:"); 
		if (Option == 2) Serial.print("Updating User number."); 
		Serial.println(recno);
        if ( recno >= 0 && recno <= Usernamecount) {
            if (SD.exists(UsernameFile)) {
                dbFile = SD.open(UsernameFile, FILE_WRITE);
                if (dbFile) {  
                    db.open(0);
                    db.readRec(recno, EDB_REC Username);
			        Serial.print("User ID: "); Serial.print(Username.Usernumber);
			        Serial.print(" User name: "); Serial.print(Username.Firstname); Serial.print(" "); Serial.print(Username.Surname);
			        Serial.print(" Welcome Message: "); Serial.println(Username.Welcome);
			        if (Option == 3) Serial.println("Press D to delete. ");
				if (Option == 2) Serial.println("Press 1 to confirm. "); 
				char c = readSelection();
			        if (c == 'D' && Option == 3) {
                                    db.deleteRec(recno);
                                    Serial.print("Deleting User number:"); Serial.println(recno);
                                    dbFile.close();
                                    Log(Delete, User);
                                } else {
				    if(c=='1' && Option == 2) {
				        dbFile.close();
                                    }
                                    else {
                                        Serial.println("Aborted. Exiting now");
                                        dbFile.close();
                                        delay(1000);
                                        return;
                                    }
				}
                }
            }
            else {
                Serial.print("Unable to find UsernameFile");
                return;
            }    
        }
        else {
            Serial.println("Invalid User");
            return;
        }
        
        
    }
	
    if (Option  <= 2) {
        
        Serial.print("Enter Users first name 20 char max: ");
		readString(Username.Firstname, 20);
        Serial.println(Username.Firstname);
        
        Serial.print("Enter Users Surname 20 char max: ");
		readString(Username.Surname, 20);
        Serial.println(Username.Surname);
        
        Serial.print("Enter a Welcome Message 60 char max: ");
		readString(Username.Welcome, 60);
        Serial.println(Username.Welcome);
        
        Serial.print("Saving...");
		
		if (Option == 1) Username.Usernumber = (Usernamecount + 1);
		if (Option == 2) Username.Usernumber = recno;
		
		if (SD.exists(UsernameFile)) {
            dbFile = SD.open(UsernameFile, FILE_WRITE);
            if (dbFile) {  
                db.open(0);
                if (Option == 1) db.appendRec(EDB_REC Username);
		if (Option == 2) db.updateRec(recno, EDB_REC Username);
                dbFile.close();
                if (Option == 1) Log( Add , User );
                if (Option == 2) Log( Update , User );
                Serial.println(" Saved");
            }
                
        }
		
    }
	else{
	Serial.print("Oops.. I've fucked something up");
	}
	
return;	
}

void addrfidcard() {

    if (!getUsername()) {
	return;
	}
	RFIDtagID.Usernumber = Username.Usernumber;
    Serial.print("Scan tag now...");
	while(!Serial.available()) SPARK_WLAN_Loop();
    pos = 0;
    while (Serial.available()){
            if (pos < 5){
                inChar = Serial.read();
                RFIDtagID.Tagnumber[pos] = inChar;
                pos++;
                RFIDtagID.Tagnumber[pos] = 0;
                if (inChar == 0x0A || inChar == 0x0D){
                      // End of line... Process things here...

                    pos = 5;
                }
            } else {
                Serial.flush();
            }
    }
    int i;
	for(i = 0; i <= 4; i++) {
        Serial.print(RFIDtagID.Tagnumber[i],HEX);
        Serial.print(" ");
	}
	Serial.println();
	Serial.print("Saving tag now...");
    
	    if (SD.exists(RFIDFile)) {
            dbFile = SD.open(RFIDFile, FILE_WRITE);
            if (dbFile) {  
                db.open(0);
                db.appendRec(EDB_REC RFIDtagID);
                dbFile.close();
                Log(Add, RFID);
                Serial.println("Done");
            }
                
        }
    return;
}

void addfingerprint() {

if (!getUsername()) {
	return;
	}
	FingerprintID.Usernumber = Username.Usernumber;
		
	if (getFingerprintEnroll((FingerprintIDcount + 1)) == FINGERPRINT_OK) {
            FingerprintIDcount++;
            FingerprintID.Address = FingerprintIDcount;
            Serial.print("Saving Fingerprint to database now...");
               
	    if (SD.exists(FingerFile)) {
                dbFile = SD.open(FingerFile, FILE_WRITE);
                if (dbFile) {  
                    db.open(0);
                    db.appendRec(EDB_REC FingerprintID);
                    dbFile.close();
                    Log(Add, Finger);
                    Serial.println("Done");
                }
            }
	} else {
	Serial.println("Something went wrong");
	}
	
    return;
	
	}

int getFingerprintID() {
    
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      if (ProgMode) Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      if (ProgMode) Serial.print(".");
      return 0;
    case FINGERPRINT_PACKETRECIEVEERR:
      if (ProgMode) Serial.println("Communication error");
      return -1;
    case FINGERPRINT_IMAGEFAIL:
      if (ProgMode) Serial.println("Imaging error");
      return -1;
    default:
      if (ProgMode) Serial.println("Unknown error");
      return -1;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      if (ProgMode) Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      if (ProgMode) Serial.println("Image too messy");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
      if (ProgMode) Serial.println("Communication error");
      return -1;
    case FINGERPRINT_FEATUREFAIL:
      if (ProgMode) Serial.println("Could not find fingerprint features");
      return -1;
    case FINGERPRINT_INVALIDIMAGE:
      if (ProgMode) Serial.println("Could not find fingerprint features");
      return -1;
    default:
      if (ProgMode) Serial.println("Unknown error");
      return -1;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  switch (p) {
      case FINGERPRINT_OK:
          if (ProgMode) Serial.println("Found a print match!");
          break;
      case FINGERPRINT_PACKETRECIEVEERR:
          if (ProgMode) Serial.println("Communication error");
          return -1;
      case FINGERPRINT_NOTFOUND:
          if (ProgMode) Serial.println("Did not find a match");
          return 1;
      default:
          if (ProgMode) Serial.println("Unknown error");
          return -1;
  }
  
  
  // found a match!
  if (ProgMode) Serial.print("Found Fingerprint #"); 
  if (ProgMode) Serial.print(finger.fingerID); 
  if (ProgMode) Serial.print(" with confidence of "); 
  if (ProgMode) Serial.println(finger.confidence); 
  return 2; 
 
  
}

uint8_t getFingerprintEnroll(uint8_t id) {
  uint8_t p = -1;
  Serial.println("Waiting for valid finger to enroll");
  while (p != FINGERPRINT_OK) {
      
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  p = finger.fingerFastSearch();
  switch (p) {
      case FINGERPRINT_OK:
          Serial.println("Print already exists !");
          return p;
      case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          return p;
      case FINGERPRINT_NOTFOUND:
          Serial.println("Confirmed not in Database");
          break;
      default:
          Serial.println("Unknown error");
          return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  
  // OK converted!
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored on reader OK");
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  return 0;
}
    
int searchDB(void) {
    if (!scanDisplay){
    if (ProgMode) Serial.println("Place finger on reader...");
    digole.clearScreen();
    delay(50);
    digole.setRot90();
    digole.setFont(18);
    digole.setTrueColor(0, 0, 255);
    digole.print("  Place your  Finger on the     reader");
    scanDisplay = true;
    }
    int p = 0;
    
    p = getFingerprintID();
    switch (p) {
        case (1):
                scanDisplay = false;
                digole.clearScreen();
                delay(50);
                digole.setRot90();
                digole.setFont(18);
                digole.setTrueColor(255, 0, 0);
                digole.print("  Get Fucked  you don't live     here");
                playSong(2);
                Log((char*)"Denied fingerprint Access - Unknown Finger");
                delay(100);
              return -1;
        case (-1):
              if (ProgMode) Serial.println("Returning to menu because of error!!!");
              scanDisplay = false;
              return -1;  
        case (0):
              return 0;
        default:
              break;
        }        
                
    scanDisplay = false;
    
    if (finger.fingerID == 99) {
        toggleProgMode();
        return -1;
    }
    SD.begin(chipSelect);
    if (SD.exists(FingerFile)) {
        dbFile = SD.open(FingerFile, FILE_WRITE);
        if (dbFile) {  
            db.open(0);
            unsigned int recordnumber;
            for (recordnumber = 1; recordnumber < (db.count() + 1); recordnumber++) {
		
                db.readRec(recordnumber, EDB_REC FingerprintID);

                if (FingerprintID.Address == finger.fingerID) {
                    if (ProgMode) Serial.println("Match Found.");
                    dbFile.close();
                    if (SD.exists(UsernameFile)) {
                        dbFile = SD.open(UsernameFile, FILE_WRITE);
                        if (dbFile) {  
                            db.open(0);
                            db.readRec(FingerprintID.Usernumber, EDB_REC Username);
                            if (ProgMode) Serial.print("User ID: "); if (ProgMode) Serial.print(Username.Usernumber);
                            if (ProgMode) Serial.print(" User name: "); if (ProgMode) Serial.print(Username.Firstname); 
                            if (ProgMode) Serial.print(" "); if (ProgMode) Serial.println(Username.Surname);
                            if (ProgMode) Serial.print(" Welcome Message: "); if (ProgMode) Serial.println(Username.Welcome);
                            dbFile.close();
                            Log(Access, User);
                            digole.clearScreen();
                            delay(50);
                            digole.setRot90();
                            digole.setFont(18);
                            digole.setTrueColor(0, 255, 0);
                            digole.print(Username.Welcome);
                            playSong(1);
                            delay(1000);
                            return 1;
                        }
                        else {
                            if (ProgMode) Serial.print("Unable to open FingerFile");
                            return -1;
                        }
                    }
                    else {
                        if (ProgMode) Serial.print("Unable to find FingerFile");
                        return -1;
                    }
                            
                }
            
            }
            if (ProgMode) Serial.println("No Match Found in Database!");
            dbFile.close();
    }
    else {
        if (ProgMode) Serial.print("Unable to open FingerFile");
        return -1;
    }
    }
    else {
        Serial.print("Unable to find FingerFile");
        return -1;
    }
    
    
    return -1;
    
}

void setMaster() {

	FingerprintID.Usernumber = 99;
		
	if (getFingerprintEnroll((99)) == FINGERPRINT_OK) {
            Serial.println("Saved");
            Log((char*)"Master Fingerprint added");
            
	} else {
	Serial.println("Something went wrong");
	}
	
    return;
	
	}

void begin_rtttl(char *p) {
    if (ProgMode) {
        digole.setFont(10);
        digole.setTrueColor(255, 255, 255);
        digole.nextTextLine();
        digole.nextTextLine();
        digole.println("Now Playing:");
        digole.print("   ");
    }
  while(*p != ':') {
     if (ProgMode) digole.print(*p);
     p++; // ignore name 
  }
    
  p++; // skip ':'
  if(*p == 'd')
  {
    p++; p++; // skip "d="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    if(num > 0) default_dur = num;
    p++; // skip comma
  }
  // get default octave
  if(*p == 'o')
  {
    p++; p++; // skip "o="
    num = *p++ - '0';
    if(num >= 3 && num <=7) default_oct = num;
    p++; // skip comma
  }
  // get BPM
  if(*p == 'b')
  {
    p++; p++; // skip "b="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    bpm = num;
    p++; // skip colon
  }
  // BPM usually expresses the number of quarter notes per minute
  wholenote = (60 * 1000L / (bpm * 1.1)) * 2; // this is the time for whole note (in milliseconds)
  // Save current song pointer...
  songPtr = p;
}

bool next_rtttl() {

  char *p = songPtr;
  // if notes remain, play next note
  if(*p)
  {
    // first, get note duration, if available
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    
    if(num) duration = wholenote / num;
    else duration = wholenote / default_dur; // we will need to check if we are a dotted note after

    // now get the note
    note = 0;

    switch(*p)
    {
      case 'c':
        note = 1;
        break;
      case 'd':
        note = 3;
        break;
      case 'e':
        note = 5;
        break;
      case 'f':
        note = 6;
        break;
      case 'g':
        note = 8;
        break;
      case 'a':
        note = 10;
        break;
      case 'b':
        note = 12;
        break;
      case 'p':
      default:
        note = 0;
    }
    p++;
    if(*p == '#') // now, get optional '#' sharp
    {
      note++;
      p++;
    }
    if(*p == '.' || *p == '_') // now, get optional '.' dotted note
    {
      duration += duration/2;
      p++;
    }
    if(isdigit(*p)) // now, get scale
    {
      scale = *p - '0';
      p++;
    }
    else {
      scale = default_oct;
    }
    scale += OCTAVE_OFFSET;
    if(*p == ',')
      p++; // skip comma for next note (or we may be at the end)
    songPtr = p; // Save current song pointer...
    if(note){ // now play the note
      tones(tonePin, notes[(scale - lowest_oct) * 12 + note], duration);
        
    }
    else {
      delay(duration);
    }
    return 1; // note played successfully.
  }
  else {
    return 0; // all done
  }
}

void tones(int pin, int16_t note, int16_t duration) {
  for(int16_t x=0;x<(duration*1000/note);x++) {
    PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin; // HIGH
    delayMicroseconds(note);
    PIN_MAP[pin].gpio_peripheral->BRR = PIN_MAP[pin].gpio_pin; // LOW
    delayMicroseconds(note);
  }
}

void playSong(int a) {
    if (a == 1) begin_rtttl(allowed);
    if (a == 2) begin_rtttl(denied);
    if (a == 3) begin_rtttl(song);
    
    while(next_rtttl()) next_rtttl(); 
    
    SPARK_WLAN_Loop();
}

void inputSong(void) {
    
    Serial.println("Copy Song now.. this will take a while!");
    readString(inSongptr , 400);
    Serial.println(inSongptr);
    begin_rtttl(inSongptr);
    while(next_rtttl()) next_rtttl(); 
     
    return;
}

void runXbmcScript(char *hostname, int port, char *url, char *xbmclogon, int retry) {
    
    
    TCPClient client;
    char line[255];
    client.stop();
    client.flush();
    
    
    if (ProgMode) {Serial.print("connecting... ");}
    if (client.connect(hostname, port)) {
        if (ProgMode) {
            Serial.print("connected to ");
            Serial.println(hostname);
        }
        delay(500);
        digitalWrite(LED, LOW);
        strcpy(line, "GET ");
        strcat(line, url);
        strcat(line, " HTTP/1.1\r\n");
        client.print(line);
        if (ProgMode) {Serial.print(line);}
        
        strcpy(line, "Host: ");
        strcat(line, hostname);
        strcat(line, "\r\n");
        client.print(line);
        if (ProgMode) {Serial.print(line);}
        
        strcpy(line, "Authorization: Basic ");
        strcat(line, xbmclogon);
        strcat(line, "\r\n");
        strcat(line, "Connection: close\r\n\r\n");
        client.print(line);
        delay(500);
        if (ProgMode) {
            Serial.print(line);
            while (!client.available()) SPARK_WLAN_Loop(); 
            while (client.available()) {
            char c = client.read();
            Serial.print(c);
            }
        Serial.println();
        }
        
        client.flush();

        delay(200);
        if (ProgMode) {Serial.println("closing...");}
        client.stop();
        digitalWrite(LED, LOW);
        retry = -1;
        
    }
    else{
        digitalWrite(LED, HIGH);
        client.flush();
        client.stop();
    
        if (ProgMode) Serial.println("connection failed");
        if (retry > 0) {
            retry--;
            delay(1000);
            digitalWrite(LED, LOW);
            runXbmcScript(hostname, port, url, xbmclogon, retry);
        } else if (retry == 0) {
            if (ProgMode) Serial.println("Gave up");  
            digitalWrite(LED, LOW);
        }
    }
}

void sendToPushingBox(char *serverpbName, char *devid, int retry) {

    TCPClient client;    
    char line[255];
    client.stop();
    client.flush();
    
    if (ProgMode) {Serial.print("connecting... ");}
    if (client.connect(serverpbName, 80)) {
        
        if (ProgMode) {
            Serial.print("connected to ");
            Serial.println(serverpbName);
        }
        delay(500);
        strcpy(line, "GET ");
        strcat(line, devid);
        strcat(line, " HTTP/1.1\r\n");
        client.print(line);
        if (ProgMode) {Serial.print(line);}
        //delay(1000);
        strcpy(line, "Host: ");
        strcat(line, serverpbName);
        strcat(line, "\r\n");
        client.print(line);
        if (ProgMode) {Serial.print(line);}
        //delay(1000);
        strcpy(line, "Connection: close\r\n");
        //strcat(line, "Accept: text/html, text/plain\r\n");
        //strcat(line, "Content-Length: 0\r\n");
        strcat(line, "User-Agent: Spark\r\n\r\n");
        client.print(line);
        delay(500);
        if (ProgMode) {
            Serial.print(line);
            while (!client.available()) SPARK_WLAN_Loop();
            while (client.available()) {
            char c = client.read();
            Serial.print(c);
            }
        Serial.println();
        }
        
        client.flush();
        delay(200);
        if (ProgMode) {Serial.println("closing...");}
        client.stop();
        digole.println("Email Sent");
        digitalWrite(LED, LOW);
        retry = -1;
        
    }
    else{
        client.flush();
        client.stop();
        digitalWrite(LED, HIGH); 
        if (ProgMode) Serial.println("connection failed");
        if (retry > 0) {
            retry--;
            delay(1000);
            digitalWrite(LED, LOW);
            sendToPushingBox(serverpbName, devid, retry);
        } else if (retry == 0) {
            if (ProgMode) Serial.println("Gave up");   
            digitalWrite(LED, LOW);
        }
    }
}
