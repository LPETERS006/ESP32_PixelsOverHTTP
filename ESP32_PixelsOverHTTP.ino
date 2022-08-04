#define ESP32
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_USE_PROGMEM 0
#define INTERRUPT_THRESHOLD 1
#define FASTLED_INTERRUPT_RETRY_COUNT 1

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <FastLED.h>
#include <LinkedList.h>

#define NUM_LEDS 192
#define DATA_PIN 23
CRGB leds[NUM_LEDS];

const char* kSsid                  = "WIFI-SSID";				/*Enter your SSID*/
const char* kPassword              = "WIFI-PASS";				/*Enter your PASS*/
const char* kHostname              = "www.example.com";			/*Enter your HOST*/
const int kPort                    = 443;
const char* kPath                  = "/main/path/rest/iot/matrix/";	/*Enter your path to restservice (we automatic the DEVICE MAC as URL like /main/path/rest/iot/matrix/FF00...)*/
const int kNetworkTimeout          = 30 * 1000;
const int kNetworkDelay            = 1000;
unsigned long kPreviousTime        = 0;
unsigned long kReconnectDelay      = 20000;
const int kBlinkDelayMultiplicator = 3;
const int kBlinkRepeats            = 5;
int kLoopCount                     = 0;
String kDeviceMac;


/*LetsEncrypt Root X1 Cert for SSL*/

const char cert_XSRG_ROOT_X1[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)CERT";

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
  /*kClient.setInsecure();*/
  kClient.setCACert(cert_XSRG_ROOT_X1);
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
    if (leds[i] == CRGB( rLED.substring(1, 4).toInt(), rLED.substring(4, 7).toInt(), rLED.substring(7, 10).toInt())) {}
    else 
  {
      Serial.println('.');
      kLinkedListIndex.add(i); kLinkedListOld.add(leds[i]);
      kLinkedListNew.add(CRGB( rLED.substring(1, 4).toInt(), rLED.substring(4, 7).toInt(), rLED.substring(7, 10).toInt()));
      leds[i] = CRGB( rLED.substring(1, 4).toInt(), rLED.substring(4, 7).toInt(), rLED.substring(7, 10).toInt());
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
    int l = kLinkedListIndex.get(i); Serial.println(String(i)); leds[l] = kLinkedListOld.get(i);
  };
  LEDS.show(); delay(100*kBlinkDelayMultiplicator);
  for (int i = 0; i < kLinkedListIndex.size(); i++) 
  {
    int l = kLinkedListIndex.get(i); Serial.println(String(i)); leds[l] = kLinkedListNew.get(i);
  };
  LEDS.show(); delay(500*kBlinkDelayMultiplicator);
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
