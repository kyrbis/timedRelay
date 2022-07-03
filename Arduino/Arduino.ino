/*
 * based on https://www.mischianti.org/2020/08/08/network-time-protocol-ntp-timezone-and-daylight-saving-time-dst-with-esp8266-esp32-or-arduino/
 * 
 * Simple NTP client
 * https://www.mischianti.org/
 *
 * The MIT License (MIT)
 * written by Renzo Mischianti <www.mischianti.org>
 */

#define RELAY_CONDITION timeInfo->tm_min<30
const char *ssid     = "Funkquelle Epsilon";
const char *password = "eisigesnetzwerk93138";
 
#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <time.h>
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
 
/**
 * Input time in epoch format and return tm time format
 * by Renzo Mischianti <www.mischianti.org> 
 */
static tm getDateTimeByParams(long time){
    struct tm *newtime;
    const time_t tim = time;
    newtime = localtime(&tim);
    return *newtime;
}
/**
 * Input tm time format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org>
 */
static String getDateTimeStringByParams(tm *newtime, char* pattern = (char *)"%d/%m/%Y %H:%M:%S"){
    char buffer[30];
    strftime(buffer, 30, pattern, newtime);
    return buffer;
}
 
/**
 * Input time in epoch format format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org> 
 */
static String getEpochStringByParams(long time, char* pattern = (char *)"%d/%m/%Y %H:%M:%S"){
//    struct tm *newtime;
    tm newtime;
    newtime = getDateTimeByParams(time);
    return getDateTimeStringByParams(&newtime, pattern);
}
 
// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

int timeUpdate()
{
  WiFiUDP ntpUDP;
 
  // By default 'pool.ntp.org' is used with 60 seconds update interval and
  // no offset
  // NTPClient timeClient(ntpUDP);
   
  // You can specify the time server pool and the offset, (in seconds)
  // additionaly you can specify the update interval (in milliseconds).
  int GTMOffset = 0; // SET TO UTC TIME
  NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GTMOffset*60*60, 60*60*1000);

  timeClient.begin();
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while ( WiFi.status() != WL_CONNECTED ) 
  {
    Serial.print ( "." );
    delay ( 420 );
  }
  Serial.println(WiFi.localIP());

  int ret = timeClient.update();
  if (ret){
    Serial.println( "adjust local clock to" );
    unsigned long epoch = timeClient.getEpochTime();
    setTime(epoch);
    Serial.print("UTC ");
    Serial.println(getEpochStringByParams(now()));

    Serial.print("CE(S)T ");
    Serial.println(getEpochStringByParams(CE.toLocal(now())));
  }
  else
  {
    Serial.print ( "NTP Update not WORK!!" );
  }

  timeClient.end();
  WiFi.disconnect();
  return ret;
}

int relayDisable()
{
  if (digitalRead(LED_BUILTIN) == 1) { return 0; }
  
  Serial.println("SWITCH relay OFF");
  if (timeUpdate() < 0) { return -1; }

  digitalWrite(LED_BUILTIN, 1);

  return 0;
}

int relayEnable()
{
  if (digitalRead(LED_BUILTIN) == 0) { return 0; }
  
  Serial.println("SWITCH relay ON");
  digitalWrite(LED_BUILTIN, 0);

  return 0;
}

int relaySetAction()
{
  time_t timeNowUTC;
  time_t timeNowLocal;
  struct tm * timeInfo;

  timeNowUTC = time(nullptr);
  timeNowLocal = CE.toLocal(time(nullptr));
  timeInfo = localtime(&timeNowLocal);

  if (RELAY_CONDITION)
  {
    if (relayEnable() == 0) return 1;
  }
  else
  {
    if (relayDisable() == 0) return 0;
  }

  return -1;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200); 

  relayDisable();
  relayEnable();
}

void loop() 
{
  Serial.print(getEpochStringByParams(CE.toLocal(now())));
  Serial.print(" relay state: ");
  Serial.println(relaySetAction());

  delay(32 * 1000);
}
