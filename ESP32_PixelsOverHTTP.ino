#define ESP32
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_USE_PROGMEM 1
#define INTERRUPT_THRESHOLD 1
#define FASTLED_INTERRUPT_RETRY_COUNT 1

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <FastLED.h>
#include <LinkedList.h>

const uint8_t kMatrixWidth  = 30;
const uint8_t kMatrixHeight = 20;
bool kMatrixSerpentineLayout = true;
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
#define DATA_PIN 23
CRGB leds[NUM_LEDS];

const char* kSsid                  = "xxx";
const char* kPassword              = "xxx";
const char* kHostname              = "www.xxx.com";
const int kPort                    = 443;
const char* kPath                  = "/xxx/xxx/xxx/";
const int kNetworkTimeout          = 30 * 1000;
const int kNetworkDelay            = 1000;
unsigned long kPreviousTime        = 0;
unsigned long kReconnectDelay      = 20000;
const int kBlinkDelayMultiplicator = 5;
const int kBlinkRepeats            = 5;
int kLoopCount                     = 0;
String kDeviceMac;

WiFiClientSecure kClient;
HttpClient http(kClient, kHostname, kPort);
LinkedList<CRGB> kLinkedListNew = LinkedList<CRGB>();
LinkedList<CRGB> kLinkedListOld = LinkedList<CRGB>();
LinkedList<int> kLinkedListIndex = LinkedList<int>();


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ WiFi init ...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void initWiFi() 
{
  kClient.setInsecure();
  WiFi.mode(WIFI_STA); WiFi.begin(kSsid, kPassword);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
  Serial.println(WiFi.localIP());
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ NTP TimeSync ...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ splitting by ...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
String getValues(String vData, char vSeparator, int vIndex)
{
  int found = 0; int strIndex[] = { 0, -1 };
  int maxIndex = vData.length() - 1;
  for (int i = 0; i <= maxIndex && found <= vIndex; i++)
  {
    if (vData.charAt(i) == vSeparator || i == maxIndex)
    {
      found++; strIndex[0] = strIndex[1] + 1; strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  };
  return found > vIndex ? vData.substring(strIndex[0], strIndex[1]) : "";
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ stringToLed ....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void response2LED(String rData)
{
  kLinkedListNew.clear(); kLinkedListOld.clear(); kLinkedListIndex.clear();
  for (int i = 0; i < NUM_LEDS; i++)
  {
    String rLED = getValues(rData, ',' , i);
  Serial.println(rLED);
    if (leds[convert(i)] == CRGB( rLED.substring(1, 4).toInt(), rLED.substring(4, 7).toInt(), rLED.substring(7, 10).toInt())) {}
    else 
  {
      Serial.println('.');
      kLinkedListIndex.add(i); kLinkedListOld.add(leds[i]);
      kLinkedListNew.add(CRGB( rLED.substring(1, 4).toInt(), rLED.substring(4, 7).toInt(), rLED.substring(7, 10).toInt()));
      leds[convert(i)] = CRGB( rLED.substring(1, 4).toInt(), rLED.substring(4, 7).toInt(), rLED.substring(7, 10).toInt());
    }
  };
  FastLED.show();
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ Get Device MAC ...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void getDeviceMAC()
{
  kDeviceMac = WiFi.macAddress();
  kDeviceMac.replace(":", "");
  Serial.println(kDeviceMac);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ setup...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void setup()
{
  Serial.begin(115200); delay(10); Serial.println(); Serial.println();
  getDeviceMAC();
  initWiFi();
  setClock();
  LEDS.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.clear(); LEDS.setBrightness(255); LEDS.show();
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ http GET DATA...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void getHttp()
{
  int err = 0;
  http.beginRequest();
  err = http.get(kPath + kDeviceMac + "/");
  http.sendHeader("Content-Type","application/json");
  http.sendHeader("Accept","application/json");
  http.endRequest();
  if (err == 0)
  {
    Serial.println('startedRequest ok');
    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print('Got status code: '); Serial.println(err);
      int bodyLen = http.contentLength();
      Serial.print('Content length is: '); Serial.println(bodyLen);
      unsigned long timeoutStart = millis();
      while ( (http.connected() || http.available()) && (!http.endOfBodyReached()) && ((millis() - timeoutStart) < kNetworkTimeout) )
      {
        if (http.available())
        {
          String response = http.responseBody();
          int offset = response.lastIndexOf('data');
          String data = response.substring(offset + 4, response.length() - 2);
          response2LED(data);
          timeoutStart = millis();
        }
        else { delay(kNetworkDelay); }
      }
    }
    else { Serial.print('Getting response failed: '); Serial.println(err); }
  }
  else { Serial.print('Connect failed: '); Serial.println(err); }
  http.stop();
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ blink updated LEDs.....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void blink() 
{
  Serial.println('blink'); Serial.println( 'index size' + String(kLinkedListIndex.size()));
  for (int i = 0; i < kLinkedListIndex.size(); i++)
  {
    int l = kLinkedListIndex.get(i); Serial.println(String(i)); leds[convert(l)] = kLinkedListOld.get(i);
  };
  LEDS.show(); delay(100*kBlinkDelayMultiplicator);
  for (int i = 0; i < kLinkedListIndex.size(); i++) 
  {
    int l = kLinkedListIndex.get(i); Serial.println(String(i)); leds[convert(l)] = kLinkedListNew.get(i);
  };
  LEDS.show(); delay(500*kBlinkDelayMultiplicator);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ Convert 2 Matrix XY.....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
uint16_t convert(int i)
{
    uint8_t nr = i + 1;
    uint8_t x = floor(nr/kMatrixWidth);
    uint8_t y = nr % kMatrixWidth;
    return (XY(x,y))
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ Matrix LEDs.....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
uint16_t XY( uint8_t x, uint8_t y)
{
  if (kMatrixSerpentineLayout == true) {
    if ( y & 0x01) {
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      return (y * kMatrixWidth) + reverseX;
    }
    else {
      return (y * kMatrixWidth) + x;
    }
  }
  if (kMatrixSerpentineLayout == false) {
    return (y * kMatrixWidth) + x;
  }
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ .....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void loop()
{
  getHttp();
  for (int i = 0; i < 10; i++ ) 
  {
  for (int ii = 0; ii < kBlinkRepeats; ii++ ) { blink(); }
    delay(500*kBlinkDelayMultiplicator);
  };
  kLoopCount++; if (kLoopCount <= 10) { kLoopCount = 0; }
  if ((WiFi.status() != WL_CONNECTED) && ((millis()-kPreviousTime) >= kReconnectDelay)) 
  {
    Serial.print(millis()); Serial.println('Reconnecting to WIFI network');
    WiFi.disconnect(); WiFi.reconnect();
    kPreviousTime = millis();
  };
}
