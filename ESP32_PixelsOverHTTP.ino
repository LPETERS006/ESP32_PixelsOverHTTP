#define ESP32

#include <SPI.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <FastLED.h>
#include <LinkedList.h>

#define NUM_LEDS 1170
#define DATA_PIN 23
CRGB leds[NUM_LEDS];

const char* ssid     = "XXX";
const char* password = "XXX";
const char* kHostname = "XXX";
int port = 3001;
const char* kPath = "/users/";
const int kNetworkTimeout = 30 * 1000;
const int kNetworkDelay = 1000;
int* ledsNew;

WiFiClient c;
HttpClient http(c, kHostname, port);
LinkedList<CRGB> myLinkedListNew = LinkedList<CRGB>();
LinkedList<CRGB> myLinkedListOld = LinkedList<CRGB>();
LinkedList<int> myLinkedListIndex = LinkedList<int>();

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);  Serial.print(".");
  }
  Serial.println(WiFi.localIP());
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ splitting by ...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ stringToLed ....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void response2LED(String dat)
{
  myLinkedListNew.clear();
  myLinkedListOld.clear();
  myLinkedListIndex.clear();
  for (int i = 0; i < NUM_LEDS; i++)
  {
    String LED = getValue(dat, ',' , i);
    Serial.println(LED);
    if (leds[i] == CRGB( LED.substring(1, 4).toInt(), LED.substring(4, 7).toInt(), LED.substring(7, 10).toInt())) {}
    else {
      Serial.println(".");
      myLinkedListIndex.add(i);
      myLinkedListOld.add(leds[i]);
      myLinkedListNew.add(CRGB( LED.substring(1, 4).toInt(), LED.substring(4, 7).toInt(), LED.substring(7, 10).toInt()));
      leds[i] = CRGB( LED.substring(1, 4).toInt(), LED.substring(4, 7).toInt(), LED.substring(7, 10).toInt());
    }
  }
  FastLED.show();
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ setup...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  initWiFi();
  LEDS.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.clear();
  LEDS.show();
  LEDS.setBrightness(10);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ http GET json...
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void getHttp()
{
  int err = 0;
  err = http.get(kPath);
  if (err == 0)
  {
    Serial.println("startedRequest ok");
    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);
      int bodyLen = http.contentLength();
      Serial.print("Content length is: ");
      Serial.println(bodyLen);
      unsigned long timeoutStart = millis();
      while ( (http.connected() || http.available()) && (!http.endOfBodyReached()) && ((millis() - timeoutStart) < kNetworkTimeout) )
      {
        if (http.available())
        {
          String response = http.responseBody();
          int offset = response.lastIndexOf('data');
          String data = response.substring(offset + 4, response.length() - 2);
          /*          String data = response.substring(29, response.length() - 2);    */
          response2LED(data);
          timeoutStart = millis();
        }
        else
        {
          delay(kNetworkDelay);
        }
      }
    }
    else
    {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ blink updated LEDs.....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void blink() {
  Serial.println("blink");
  Serial.println( "index size" + String(myLinkedListIndex.size()));
  for (int i = 0; i < myLinkedListIndex.size(); i++)
  {
    int l = myLinkedListIndex.get(i);
    Serial.println(String(i));
    leds[l]  = myLinkedListOld.get(i);
  }

  LEDS.show();
  delay(100);
  for (int i = 0; i < myLinkedListIndex.size(); i++) {
    int l = myLinkedListIndex.get(i);
    Serial.println(String(i));
    leds[l]  = myLinkedListNew.get(i);
  }
  LEDS.show();
  delay(500);
}

int count = 0;
unsigned long previousTime = 0;
unsigned long delayS = 20000;  // 20 seconds delay
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ++ .....
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void loop()
{
  getHttp();
  for (int i = 0; i < 10; i++ ) {
    blink();
    blink();
    blink();
    delay(500);
  };
  count++;
  if (count <= 10) {
    count = 0;
  }
  if ((WiFi.status() != WL_CONNECTED) && ((millis()-previousTime) >= delayS)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WIFI network");
    WiFi.disconnect();
    WiFi.reconnect();
    previousTime = millis();
  }
}
