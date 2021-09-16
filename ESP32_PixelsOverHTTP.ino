#include <SPI.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <FastLED.h>
#include <LinkedList.h>

#define NUM_LEDS 1170
#define DATA_PIN 23
CRGB leds[NUM_LEDS];

const char* ssid     = "LS006";
const char* password = "i2a781r4o3u5yd869n";
const char* kHostname = "192.168.132.119";
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

void res2LED(String dat)
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

void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  LEDS.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.clear();
  LEDS.show();
  // LEDS.setBrightness(10);
}

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
      char c;
      while ( (http.connected() || http.available()) && (!http.endOfBodyReached()) && ((millis() - timeoutStart) < kNetworkTimeout) )
      {
        if (http.available())
        {
          String response = http.responseBody();
          String data = response.substring(29, response.length() - 2);
          res2LED(data);
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

boolean blink() {
  Serial.println("blink");
  Serial.println( "index size" + String(myLinkedListIndex.size()));
  for (int i = 0; i < myLinkedListIndex.size(); i++)
  {
    int l = myLinkedListIndex.get(i);
    Serial.println(String(i));
    leds[l]  = myLinkedListOld.get(i);
  }

  LEDS.show();
  delay(300);
  for (int i = 0; i < myLinkedListIndex.size(); i++) {
    int l = myLinkedListIndex.get(i);
    Serial.println(String(i));
    leds[l]  = myLinkedListNew.get(i);
  }
  LEDS.show();
  delay(1000);
}

int count = 0;
boolean blinkMarker = false;
void loop()
{
  getHttp();
  count++;
  for (int i = 0; i < 60; i++ ) {
    blink();
  };
}
