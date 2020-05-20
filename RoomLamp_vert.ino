/*Programm to drive the LED matrix with messages from a web server oder simple use as a lamp
 * 2020 Claus Rogisch
 * Funcions:
 *    Lamp
 *    Clock
 *    Enchant
 *    Enlighten
*/
//Fast LED initialzation
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN      D2
#define LED_TYPE      WS2812
#define COLOR_ORDER   GRB
#define NUM_LEDS      150
#define BRIGHTNESS    100
#define FRAMES_PER_SECOND  30
CRGB leds[NUM_LEDS];

//webserer initialzation
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdateServer;

const char* ssid = "11235813";
const char* password = "password";
IPAddress staticIP(192,168,2,90);
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);
IPAddress dns1(8,8,8,8);//dns 1
IPAddress dns2(8,8,4,4);//dns 2
const String deviceName="RoomMatrix";

#define ARDUINOJSON_DECODE_UNICODE 1
#include <ArduinoJson.h>
#define CHAR_HEIGHT   6
#define CHAR_WIDTH    5
#define DISPLAY_Y     30
#define DISPLAY_X     5
#define SCROLL_TIME   150
#define SCROLL_DELAY  1000
#define CHECK_INTERVAL 30000

//clock initialization
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 7200;
NTPClient timeClient(ntpUDP,"pool.ntp.org",utcOffsetInSeconds);
char daysOfTheWeek[7][12] = 
  {"Sunday", 
  "Monday", 
  "Tuesday", 
  "Wednesday", 
  "Thursday", 
  "Friday", 
  "Saturday"};

bool checkServer;
unsigned long now;
unsigned long lastNow;
unsigned long frameTime;

bool    scroll;
bool    animating;
bool    reachedEnd;
int     frame;
int     modus;
bool    lampColorUpdate;
String  lampColorHex;
unsigned long lampColorInt;

byte    helperMatrix[DISPLAY_X][CHAR_HEIGHT*6];
String  text;
String  displayText;
int     stringOffSet;
int     charOffSet;

long    color;
long    bgColor;

const String enlightenURL = "http://clausrogisch.de/api/enlighten.php";
const String enchantURL = "http://clausrogisch.de/api/enchant.php";

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<title>RoomLamp</title>
</head>
<body>
<h1>ESP8266 Web Form Demo</h1>
<FORM action="/action_page" method="post">
Modus<br>
<INPUT type="radio" name="modus" value="0">Enchant<BR>
<INPUT type="radio" name="modus" value="1">Enlighten<BR>
<INPUT type="radio" name="modus" value="2">Lamp<BR>
<INPUT type="radio" name="modus" value="3">Clock<BR>

Lamp Color<br>
<INPUT type="text" name="color" value="ffffff">
<INPUT type="submit" value="Submit">
</FORM>
</body>
</html>
)=====";


//-----------------------------------------------------------------------------------------
typedef struct{
  const char key;
  byte matrix[CHAR_HEIGHT][CHAR_WIDTH];
}FONT;

FONT font[70] ={
  {'a',
  {{0,1,1,1,0},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,1,1,1,1},
  {1,0,0,0,1},
  {0,0,0,0,0}}
  },
  {'b',
  {
  {1,1,1,1,0},
  {1,0,0,0,1},
  {1,1,1,1,0},
  {1,0,0,0,1},
  {1,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'c',
  {
  {0,1,1,1,1},
  {1,0,0,0,0},
  {1,0,0,0,0},
  {1,0,0,0,0},
  {0,1,1,1,1},
  {0,0,0,0,0},
  }
  },
  {'d',
  {
  {1,1,1,1,0},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'e',
  {
  {1,1,1,1,1},
  {1,0,0,0,0},
  {1,1,1,1,0},
  {1,0,0,0,0},
  {1,1,1,1,1},
  {0,0,0,0,0},
  }
  },
  {'f',
  {
  {1,1,1,1,1},
  {1,0,0,0,0},
  {1,1,1,0,0},
  {1,0,0,0,0},
  {1,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'g',
  {
  {0,1,1,1,0},
  {1,0,0,0,0},
  {1,0,1,1,0},
  {1,0,0,0,1},
  {1,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'h',
  {
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,1,1,1,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'i',
  {
  {1,1,1,1,1},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {1,1,1,1,1},
  {0,0,0,0,0},
  }
  },
  {'j',
  {
  {1,1,1,1,1},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {1,1,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'k',
  {
  {1,0,0,0,1},
  {1,0,0,1,0},
  {1,1,1,0,0},
  {1,0,0,1,0},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'l',
  {
  {1,0,0,0,0},
  {1,0,0,0,0},
  {1,0,0,0,0},
  {1,0,0,0,0},
  {1,1,1,1,1},
  {0,0,0,0,0},
  }
  },
  {'m',
  {
  {1,0,0,0,1},
  {1,1,0,1,1},
  {1,0,1,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'n',
  {
  {1,0,0,0,1},
  {1,1,0,0,1},
  {1,0,1,0,1},
  {1,0,0,1,1},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'o',
  {
  {0,1,1,1,0},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'p',
  {
  {1,1,1,1,0},
  {1,0,0,0,1},
  {1,1,1,1,0},
  {1,0,0,0,0},
  {1,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'q',
  {
  {0,1,1,1,0},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'r',
  {
  {1,1,1,1,0},
  {1,0,0,0,1},
  {1,1,1,1,0},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'s',
  {
  {0,1,1,1,1},
  {1,0,0,0,0},
  {0,1,1,1,0},
  {0,0,0,0,1},
  {1,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'t',
  {
  {1,1,1,1,1},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'u',
  {
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'v',
  {
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,1,0,1,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'w',
  {
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,1,0,1},
  {1,1,0,1,1},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'x',
  {
  {1,0,0,0,1},
  {0,1,0,1,0},
  {0,0,1,0,0},
  {0,1,0,1,0},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'y',
  {
  {1,0,0,0,1},
  {0,1,0,1,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'z',
  {
  {1,1,1,1,1},
  {0,0,0,1,0},
  {0,0,1,0,0},
  {0,1,0,0,0},
  {1,1,1,1,1},
  {0,0,0,0,0},
  }
  },
  {'ü',
  {
  {0,1,0,1,0},
  {0,0,0,0,0},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'ä',
  {
  {1,0,1,0,1},
  {0,1,0,1,0},
  {1,0,0,0,1},
  {1,1,1,1,1},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'ö',
  {
  {0,1,0,1,0},
  {0,0,0,0,0},
  {0,1,1,1,0},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'?',
  {
  {0,1,1,1,0},
  {1,0,0,0,1},
  {0,0,1,1,0},
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'!',
  {
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'.',
  {
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {',',
  {
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,1,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {34,
  {
  {0,1,0,1,0},
  {0,1,0,1,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {39,
  {
  {0,1,0,0,0},
  {0,1,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'ß',
  {
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'0',
  {
  {0,1,1,1,0},
  {1,0,0,0,1},
  {1,0,1,0,1},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'1',
  {
  {0,0,1,0,0},
  {0,1,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'2',
  {
  {0,1,1,1,0},
  {1,0,0,0,1},
  {0,0,1,1,0},
  {0,1,0,0,0},
  {1,1,1,1,1},
  {0,0,0,0,0},
  }
  },
  {'3',
  {
  {1,1,1,1,0},
  {0,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,1},
  {1,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'4',
  {
  {0,0,0,1,0},
  {0,0,1,1,0},
  {0,1,0,1,0},
  {1,1,1,1,0},
  {0,0,0,1,0},
  {0,0,0,0,0},
  }
  },
  {'5',
  {
  {1,1,1,1,1},
  {1,0,0,0,0},
  {1,1,1,1,1},
  {0,0,0,0,1},
  {1,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'6',
  {
  {0,1,1,1,1},
  {1,0,0,0,0},
  {1,1,1,1,0},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'7',
  {
  {1,1,1,1,1},
  {0,0,0,0,1},
  {0,0,0,1,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'8',
  {
  {0,1,1,1,0},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {1,0,0,0,1},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'9',
  {
  {0,1,1,1,0},
  {1,0,0,0,1},
  {0,1,1,1,1},
  {0,0,0,0,1},
  {1,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'/',
  {
  {0,0,0,0,1},
  {0,0,0,1,0},
  {0,0,1,0,0},
  {0,1,0,0,0},
  {1,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {92, //backslash____________________________________________w____
  {
  {1,0,0,0,0},
  {0,1,0,0,0},
  {0,0,1,0,0},
  {0,0,0,1,0},
  {0,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'#',
  {
  {0,1,0,1,0},
  {1,1,1,1,1},
  {0,1,0,1,0},
  {1,1,1,1,1},
  {0,1,0,1,0},
  {0,0,0,0,0},
  }
  },
  {'%',
  {
  {1,0,0,0,1},
  {0,0,0,1,0},
  {0,0,1,0,0},
  {0,1,0,0,0},
  {1,0,0,0,1},
  {0,0,0,0,0},
  }
  },
  {'(',
  {
  {0,0,0,1,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,0,1,0},
  {0,0,0,0,0},
  }
  },
  {')',
  {
  {0,1,0,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,1,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'[',
  {
  {0,0,1,1,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,1,0},
  {0,0,0,0,0},
  }
  },
  {']',
  {
  {0,1,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,1,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'{',
  {
  {0,0,1,1,0},
  {0,0,1,0,0},
  {0,1,1,0,0},
  {0,0,1,0,0},
  {0,0,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'}',
  {
  {0,1,1,0,0},
  {0,0,1,0,0},
  {0,0,1,1,0},
  {0,0,1,0,0},
  {0,1,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'*',
  {
  {0,1,0,1,0},
  {0,0,1,0,0},
  {0,1,0,1,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {':',
  {
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'+',
  {
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,1,1,1,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'-',
  {
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,1,1,1,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'<',
  {
  {0,0,0,0,0},
  {0,0,1,1,0},
  {0,1,0,0,0},
  {0,0,1,1,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'>',
  {
  {0,0,0,0,0},
  {0,1,1,0,0},
  {0,0,0,1,0},
  {0,1,1,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {';',
  {
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {' ',
  {
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'~',
  {
  {0,0,0,0,0},
  {0,1,0,0,0},
  {1,0,1,0,1},
  {0,0,0,1,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'=',
  {
  {0,0,0,0,0},
  {0,1,1,1,0},
  {0,0,0,0,0},
  {0,1,1,1,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  }
  },
  {'$',
  {
  {0,0,1,0,0},
  {0,0,1,1,0},
  {0,1,1,1,0},
  {0,1,1,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'@',
  {
  {0,1,1,1,0},
  {1,0,1,0,1},
  {1,0,1,1,0},
  {1,0,0,0,0},
  {0,1,1,1,0},
  {0,0,0,0,0},
  }
  },
  {'|',
  {
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,1,0,0},
  {0,0,0,0,0},
  }
  },
  {'_',
  {
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {0,0,0,0,0},
  {1,1,1,1,1},
  {0,0,0,0,0},
  }
  }
};
int mLength;

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);

  //wifi setup
  WiFi.mode(WIFI_STA);
  WiFi.config(staticIP,gateway,subnet,dns1,dns2);
  WiFi.hostname(deviceName);
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, HIGH);
  }
  Serial.println("connected");
  digitalWrite(LED_BUILTIN, LOW);
  
  //Clock setups
  timeClient.begin();
  
  //FAST_LED setup___________________________________
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  checkServer   = true;
  scroll        = false;
  animating     = false;
  reachedEnd    = false;
  now           = millis();
  lastNow       = 0;
  frameTime     = 0;
  frame         = 0;
  stringOffSet  = 0;
  charOffSet    = 0;
  mLength       = 0;
  
  //web server setup
  server.on("/", handleRoot);
  server.on("/action_page",handleForm);
  server.begin();
  Serial.println("HTTP server started");

  modus         = 3;
  lampColorInt  = 0;
  lampColorUpdate=false;
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
void loop() {
    server.handleClient();
    now = millis();
    
    //MODUS CODE---------------------------------------------
    if(modus==0){
        if(!animating){
            checkServer_interval();
        }
        //filling text and color with data from db when it is time to check
        if (WiFi.status() == WL_CONNECTED && checkServer && !animating)  //Check WiFi connection status
        {
            getEnchantData();
            Serial.println("getting enchant data");
            Serial.println(text);
            mLength=text.length();
            if(mLength>6){
                Serial.println(scroll);
                scroll=true;
            }else{
                //generate matrix and display pixels if message fits on display
                scroll=false;
                setHelperMatrixAllZero();
                generateHelperMatrix(text);
                FastLED.clear();
                setLEDSfromHelperMatrix(0);
                FastLED.show();
            }
            charOffSet=0;
            stringOffSet=0;
            checkServer=false; 
            reachedEnd=false; 
        }

        
        if(scroll){
            animating=true;
            
            
            if(now-frameTime>SCROLL_TIME){

                if(charOffSet==0 && !reachedEnd){
                    String sub="";
                    Serial.print("CHarOfSet: ");
                    Serial.print(charOffSet);
                    Serial.print(" Frametime: ");
                    Serial.print(frameTime);
                    Serial.print(" Stringoffset: ");
                    Serial.println(stringOffSet);
                    Serial.println(mLength);
                    setHelperMatrixAllZero();
                    if(stringOffSet+6>text.length()){
                        sub=text.substring(stringOffSet);
                        generateHelperMatrix(sub);
                        Serial.println("too short substring");
                        Serial.println(sub);
                        reachedEnd=true;
                    }else{
                        sub=text.substring(stringOffSet,stringOffSet+6);
                        generateHelperMatrix(sub);
                        Serial.println("normal substring");
                        Serial.println(sub);
                    }
                    stringOffSet++;
                }
                
                animating=true;
                Serial.println("muss scrollen");
                Serial.print("CHarOfSet: ");
                Serial.print(charOffSet);
                Serial.print(" Frametime: ");
                Serial.println(frameTime);
                Serial.println(millis());
                frameTime=now;
                FastLED.clear();
                
                setLEDSfromHelperMatrix(charOffSet);
                charOffSet++;
                FastLED.show();
                if(stringOffSet==1 && charOffSet==1){
                  delay(SCROLL_DELAY); 
                }
            }
            if(charOffSet==CHAR_HEIGHT && reachedEnd){
                
                animating=false;
                reachedEnd=false;
                Serial.println("durchgescrolled");
                frameTime=0;
                frame=0;
                charOffSet=0;
                stringOffSet=0;
                delay(SCROLL_DELAY);
            }else if(charOffSet==CHAR_HEIGHT){
                charOffSet=0;
            }
        }
    }else if(modus==1){
        checkServer_interval();
        //ENLIGHTEN ME--------------------------------------------
        if (WiFi.status() == WL_CONNECTED && checkServer ){
            checkServer=false;
            setEnlightenPixel();
            Serial.println("enlighten me");
        }
    }else if(modus==2){
        //LAMP MODE----------------------------------------------
        if(lampColorUpdate){
            lamp(lampColorInt);
            lampColorUpdate=false;
            Serial.println("lampUpdated");
        }
    }else if(modus==3){
        //CLOCK--------------------------------------------------
        timeClient.update();
        displayTimePixel();
        delay(1000);
    }else{
        Serial.println("no viable modus selection");
    }
}

void checkServer_interval(){
    if (now-lastNow>CHECK_INTERVAL && !animating){
        checkServer=true;
        lastNow=now;
    }
}

void setHelperMatrixAllZero(){
    for(int x=0;x<DISPLAY_X;x++){
        for(int y=0;y<CHAR_HEIGHT*6;y++){
            helperMatrix[x][y]=0;
        }
    }
}

void replaceUmlauteAndLowerCase(){
    text.replace("Ü","ue");
    text.replace("ü","ue");
    text.replace("Ö","oe");
    text.replace("ö","oe");
    text.replace("Ä","ae");
    text.replace("ä","ae");
    text.replace("ß","ss");
    text.replace("\\","");
    text.toLowerCase();
}

void generateHelperMatrix(String s){
  for(int i=0;i<s.length();i++){
     char c = s[i];
     for(int j=0; j<69;j++){
        if(c==font[j].key){
            for(int h=0;h<CHAR_HEIGHT;h++){
                for(int w=0;w<CHAR_WIDTH;w++){
                    helperMatrix[w][h+(i*CHAR_HEIGHT)]=font[j].matrix[h][w];
                }
            }
            break;
        }
     }
  }
}

void setLEDSfromHelperMatrix(int offSet){
    for(int y=0;y<DISPLAY_Y;y++){
        for(int x=0;x<DISPLAY_X;x++){
            if(helperMatrix[x][y+offSet]==1){
                leds[ledIndex(x,29-y)]=color; 
            }
        }
    }
}

void handleRoot(){
    server.send(200, "text/html", INDEX_HTML);
}

void handleForm(){
    String formModus= server.arg("modus");
    String formColor= server.arg("color");
    modus=server.arg("modus").toInt();
    lampColorHex=server.arg("color");
    lampColorUpdate=true;
    lampColorHex.replace("#","");
    lampColorInt=strtol(lampColorHex.c_str(), NULL, 16);
    String s = "<a href='/'> Go Back </a>";
    server.send(200, "text/html", s);
    checkServer=true;
    scroll=false;
    animating=false;
    stringOffSet=0;
    charOffSet=0;
    
}

void allOff(){
    FastLED.clear();
    FastLED.show();
    modus=2;//lamp and no pixel shown
    animating=false;
    checkServer=false;
}

void lamp(unsigned long color){
    FastLED.clear();
    for(int i=0;i<NUM_LEDS;i++){
        leds[i]= color;
    }
    FastLED.show();
}
void getEnchantData(){
    HTTPClient http;
    http.begin("http://clausrogisch.de/api/enchant.php");  //Specify request destination
    int httpCode = http.GET();
    if (httpCode > 0) {
        const size_t capacity_enc = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(4) + 1400;
        DynamicJsonDocument doc(capacity_enc);
        deserializeJson(doc,http.getString());
        text = doc[0]["Message"].as<String>();
        replaceUmlauteAndLowerCase();
        mLength=text.length();
        displayText=text;
        String colorString = doc[0]["Farbe"].as<String>().substring(1); // remove Q:
        color = strtol(colorString.c_str(), NULL, 16);
        Serial.println("http enchant");
        doc.clear();   
    }
    http.end();   //Close connection
    
}
void setEnlightenPixel(){
    HTTPClient http;
    http.begin("http://clausrogisch.de/api/enlighten.php");
    int httpCode = http.GET();
    if (httpCode > 0) { 
        const size_t capacity_enl = JSON_ARRAY_SIZE(150) + 150*JSON_OBJECT_SIZE(1) + 2340;
        DynamicJsonDocument doc(capacity_enl);
        deserializeJson(doc,http.getString());
        Serial.println("http enlighten");
        for (int i=0;i<doc.size() && i<NUM_LEDS;i++) {     
            String colorString = doc[i]["Farbe"].as<String>().substring(1); // remove #:
            color = strtol(colorString.c_str(), NULL, 16);      
            leds[i]=color;
        }
        //code for big latest pixel
        String colorString = doc[0]["Farbe"].as<String>().substring(1); // remove Q:
        color = strtol(colorString.c_str(), NULL, 16);
        for(int j=0;j<3;j++){
            for(int k=0;k<6;k++){
                leds[ledIndex(j+1,12+k)]=color;
            }
        }
        doc.clear();
    }
    http.end();
    FastLED.show();
}
void displayTimePixel(){ 
    FastLED.clear();
    for(int i=0;i<=timeClient.getHours();i++){
        if(i==0){    
        }else if(i%5==0){
            leds[i-1]=CRGB::DarkRed;
        }else{
            leds[i-1]=CRGB::DarkOrange;
        }   
    }
    for(int m=0;m<=timeClient.getMinutes();m++){
        if(m==0){
          
        }else if(m<=30){
            if(m%5==0){
              leds[ledIndex(1,m-1)]=CRGB::Brown;
            }else{
              leds[ledIndex(1,m-1)]=CRGB::Beige;
            }
        }else{
            if(m%5==0){
              leds[ledIndex(2,m-31)]=CRGB::Brown;
            }else{
              leds[ledIndex(2,m-31)]=CRGB::Beige;
            }
        }  
    }
    for(int s=0;s<=timeClient.getSeconds();s++){
        if(s==0){
        }else if(s<=30){
            if(s%15==0){
              leds[ledIndex(3,s-1)]=CRGB::DarkCyan;
            }else{
              leds[ledIndex(3,s-1)]=CRGB::CadetBlue;
            }
        }else{
            if(s%15==0){
            leds[ledIndex(4,s-31)]=CRGB::DarkCyan;
            }else{
            leds[ledIndex(4,s-31)]=CRGB::CadetBlue;
            }
        }
    }
    FastLED.show();
}

int ledIndex(int x, int y){//convert x,y position on matrix to led index
    int index;
    if(x%2==0){//even row
        index=x*30+y;
    }else{ //uneven row
        index=x*30+29-y;
    }
    return index;
}
