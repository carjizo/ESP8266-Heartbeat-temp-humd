////////////////
//LIBRARY
////////////////
#include <ESP8266WiFi.h>
#include <Ticker.h>                  // interrupts
#include "FirebaseESP8266.h"        // connection with firebase
#include <DHT.h>                    // temperature humidity module

#define DHTPIN 4   // Connect Data pin of DHT to D2

////////////////
//LOOP VARIABLES
////////////////
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false; 
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);


//////////////////////
//INTERRRUPTS VARIABLE
//////////////////////
Ticker flipper;
Ticker sender;
const int maxAvgSample = 10;
volatile int rate[maxAvgSample];                    // used to hold last ten IBI values
boolean sendok = false;
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find the inter beat interval
volatile int P =512;                      // used to find peak in pulse wave
volatile int T = 512;                     // used to find trough in pulse wave
volatile int thresh = 525;                // used to find instant moment of heart beat
volatile int amp = 100;                   // used to hold amplitude of pulse waveform
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = true;       // used to seed rate array so we startup with reasonable BPM
long interval = 2000; // interval at which to do something (milliseconds)
float temp=0;
unsigned long previousMillis = 0;
//#define myPeriodic 15


////////////////
//WiFi VARIABLES
////////////////
WiFiClient client;
const char* ssid = "wifi ssid";               // wifi ssid
const char* password = "wifi password";      // wifi password
const char* host = "184.106.153.149";  
const char* server = "api.thingspeak.com";  // web service hostname/ example "api.thingspeak.com"
const int httpPort = 80;                    // web service port number/ defaul "80"


////////////////
//FIREBASE VARIABLES
////////////////
FirebaseData firebaseData;
#define FIREBASE_HOST ""    // firebase host/ example "https://example-111e-default-efsd.firebaseio.com"
#define FIREBASE_AUTH ""            // firebase database secret key






void setup() {
  Serial.begin(115200);                   // we agree to talk fast!
  dht.begin();                             // we initialize the module
  delay(10);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
 
  Serial.print("connecting to ");
  Serial.println(host);

  
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  delay(10);
    
  Serial.println();
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  
  Serial.println("end Setup");
  
  flipper.attach_ms(2, Test);
  sender.attach(2, senderfunc);
}


void senderfunc()
{
  sendok=true;
}



void loop() {  
  unsigned long currentMillis = millis();

  

   if(sendok)
     {
        delay(100);
            WiFiClient client;
              const int httpPort = 80;// to port to send the data
              if (!client.connect("api.thingspeak.com", httpPort)) {
                Serial.println("connection failed");
                return;
              }
   float temp=0;
   
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  
  Serial.print(F("HEART RATE: "));
  Serial.print(BPM);
  Serial.print(F(" bpm  "));
  Serial.print(F("  TEMPERATURE: "));
  Serial.print(t);
  Serial.print(F(" °C  "));
  Serial.print(F("  HUMIDITY: "));
  Serial.print(h);
  Serial.println(F(" %  "));
  
 if(currentMillis - previousMillis > interval) {
     previousMillis = currentMillis;  
  }
  
    
    Firebase.setFloat(firebaseData, "/bpm", BPM);
    Firebase.setFloat(firebaseData, "/temp", t);
    Firebase.setFloat(firebaseData, "/hum", h);
   
   delay(10);
     
   sendok =false;
   flipper.attach_ms(2, Test);
    
              
     }

 delay(10);
}


void sendtcp()
{
            
}


int count = 0;
void Test()
{
  count++;
  if(count ==1000)
  {
    flipper.detach();
    count =0;
    //sendtcp();
    sendok=true;
    
  }
  
      Signal = analogRead(A0);              // read the Pulse Sensor 
        sampleCounter += 2;                         // keep track of the time in mS with this variable
    int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

      if(Signal < thresh && N > (IBI/5)*3){       // avoid dichrotic noise by waiting 3/5 of last IBI
        if (Signal < T){                        // T is the trough
            T = Signal;                         // keep track of lowest point in pulse wave 
         }
       }
      
    if(Signal > thresh && Signal > P){          // thresh condition helps avoid noise
        P = Signal;                             // P is the peak
       }                                        // keep track of highest point in pulse wave
    
  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
if (N > 250){                                   // avoid high frequency noise
  if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){        
    Pulse = true;                               // set the Pulse flag when we think there is a pulse
    //digitalWrite(blinkPin,HIGH);                // turn on pin 13 LED
    IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
    lastBeatTime = sampleCounter;               // keep track of time for next pulse
         
         if(firstBeat){                         // if it's the first time we found a beat, if firstBeat == TRUE
             firstBeat = false;                 // clear firstBeat flag
             return;                            // IBI value is unreliable so discard it
            }   
         if(secondBeat){                        // if this is the second beat, if secondBeat == TRUE
            secondBeat = false;                 // clear secondBeat flag
               for(int i=0; i<=maxAvgSample-1; i++){         // seed the running total to get a realisitic BPM at startup
                    rate[i] = IBI;                      
                    }
            }
          
    // keep a running total of the last 10 IBI values
    word runningTotal = 0;                   // clear the runningTotal variable    

    for(int i=0; i<=(maxAvgSample-2); i++){                // shift data in the rate array
          rate[i] = rate[i+1];              // and drop the oldest IBI value 
          runningTotal += rate[i];          // add up the 9 oldest IBI values
        }
        
    rate[maxAvgSample-1] = IBI;                          // add the latest IBI to the rate array
    runningTotal += rate[maxAvgSample-1];                // add the latest IBI to runningTotal
    runningTotal /= maxAvgSample;                     // average the last 10 IBI values 
    BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
    QS = true;                              // set Quantified Self flag 
    // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
}

  if (Signal < thresh && Pulse == true){     // when the values are going down, the beat is over
      //digitalWrite(blinkPin,LOW);            // turn off pin 13 LED
      Pulse = false;                         // reset the Pulse flag so we can do it again
      amp = P - T;                           // get amplitude of the pulse wave
      thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
      P = thresh;                            // reset these for next time
      T = thresh;
     }
  
  if (N > 2500){                             // if 2.5 seconds go by without a beat
      thresh = 512;                          // set thresh default
      P = 512;                               // set P default
      T = 512;                               // set T default
      lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
      firstBeat = true;                      // set these to avoid noise
      secondBeat = true;                     // when we get the heartbeat back
     }
  
  //sei();                                     // enable interrupts when youre done!
}// end isr