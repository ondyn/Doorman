// ESP82166 D1 mini - china
// 5V power
// low button D3, GPIO0, active low, pullup
// piezo: D2 - R 50ohm - base MPSA42; emitor gnd; collector -> piezo; piezo - +5V
// high button D1, GPIO5, active los, pullup
// electronic opener D8 GPIO15


#include <Firebase_ESP_Client.h>
#include <Utils.h>
#include <common.h>
#include <ESP8266WiFi.h>
#include "pitches.h"

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "xx"
#define WIFI_PASSWORD "xx"

// Insert Firebase project API Key
#define API_KEY "xx"

#define USER_EMAIL "xx"
#define USER_PASSWORD "xx"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "xx" 

#define PIEZO = 2;// D4

#define BUTTON 0 // GPIO0 - D3
#define BUTTON2 5 // GPIO5 - D1
#define OPENER D8 //
//Define Firebase Data object
FirebaseData fbdo;
FirebaseData fbdo2;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;


byte openingState = 0;
unsigned long buttonPressTime = 0;
unsigned long openingStartTime = 0;
byte buttonPhase = 0;
bool buttonPressed = 0;


// note durations: 4 = quarter note, 8 = eighth note, etc.:
int melodies [4][30] = {
  {
    NOTE_E4, NOTE_C4, NOTE_E4, 0
  },
  { //bezi liska
    NOTE_C4, NOTE_E4, NOTE_C4, NOTE_E4, 
    NOTE_G4,          NOTE_G4, NOTE_G4,
    NOTE_C4, NOTE_E4, NOTE_C4, NOTE_E4,
    NOTE_D4,          NOTE_D4, NOTE_D4,
  
    NOTE_C4, NOTE_E4, NOTE_G4, NOTE_E4,
    NOTE_D4, NOTE_D4, NOTE_E4,
    NOTE_C4, NOTE_E4, NOTE_G4, NOTE_E4,
    NOTE_D4, NOTE_D4, NOTE_C4, 0
  },
  { //t-mobile jingle
    // NOTE_C4, NOTE_C4, NOTE_C4, NOTE_E4, NOTE_C4, 0
    NOTE_C4, NOTE_E4, NOTE_C4, 0
  },
  {// skakal pes
    NOTE_G4, NOTE_G4, NOTE_E4,
    NOTE_G4, NOTE_G4, NOTE_E4,
    NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4,
    NOTE_G4, NOTE_F4,

    NOTE_F4, NOTE_F4, NOTE_D4,
    NOTE_F4, NOTE_F4, NOTE_D4,
    NOTE_F4, NOTE_F4, NOTE_G4, NOTE_F4,
    NOTE_F4, NOTE_E4, 0
  }
};
int noteDurations [4][30] = {
  {
    16, 16, 16, 0
  }, 
  {
    4, 4, 4, 4,
    2,    4, 4,
    4, 4, 4, 4,
    2,    4, 4,
    4, 4, 4, 4,
    4, 4, 2,
    4, 4, 4, 4,
    4, 4, 2, 0
  },
  {
    16, 16, 16, 0
  },
  {
    4, 4, 2,
    4, 4, 2,
    4, 4, 4, 4,
    2,2,
    4, 4, 2,
    4, 4, 2,
    4, 4, 4, 4,
    2,2,0
  }
};

// declaring variables
const int tonePin = 4; //GPIO4 - D2
unsigned long tonePreviousMillis = 0;
boolean outputTone = false;                // Records current state
int melodyIndex = 0;
int selectedMelody = 0;
bool play = false;

void setup(){
  // button
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  
  // status led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(OPENER, OUTPUT);
  digitalWrite(OPENER, LOW);
  
  Serial.begin(115200);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  /* Assign the api key (required) */
  config.api_key = API_KEY;
  
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  
   /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.RTDB.setStreamCallback(&fbdo, streamCallback, streamTimeoutCallback);
  if (!Firebase.RTDB.beginStream(&fbdo, "/door/open"))
  {
    //Could not begin stream connection, then print out the error detail
    Serial.println(fbdo.errorReason());
  }
}

byte runIndex = 0;
int readPeriod = 50;
unsigned long pressTime = 0;
unsigned long playStarted = 0;
bool pressRun[25];
bool reqStopPlay = false;

void loop(){
  if (play) PlayMelody();

  // tlačítko horních dveří - hraj vždy
  if(!digitalRead(BUTTON2)) {
    play = true;
    melodyIndex = 0;
    selectedMelody = 3;
  }

  buttonPressed = !digitalRead(BUTTON);

  if(play && !buttonPressed && ((millis() - playStarted) > 3000) && (selectedMelody == 1) ) {
    play = false;
    melodyIndex = 0;
    outputTone = false;
    noTone(tonePin);
  }

  if (openingState == 0) {
    // start sampling button press 
    if (runIndex == 0 && buttonPressed){
      pressTime = millis();
      pressRun[runIndex] = buttonPressed;
      ++runIndex;
    }

    // sampling
    if ((runIndex > 0) && ((millis() - pressTime) > 50)) {
      pressRun[runIndex] = buttonPressed;
      ++runIndex;
      pressTime = millis();
    }

    // sampling finished
    if (runIndex >= sizeof(pressRun)) {
      runIndex = 0;
      int i = 0;
      byte changes = 0;
      byte ones = 0;
      Serial.print("opening: array=");
      while (i < sizeof(pressRun)){
        if ((i > 0) && (pressRun[i-1] != pressRun[i])) {
          ++changes;
        }
        if (pressRun[i] == 1) {
          ++ones;
        }
        Serial.print(pressRun[i]);
        i++;
      }
      Serial.print(" changes founded=");
      Serial.println(changes);
      // if there is 5 changes => 3 button press then open door
      if (changes == 5) openingState = 2;
      else if (!play) {
        play = true;
        melodyIndex = 0;
        selectedMelody = 1;
        playStarted = millis();
      }
    }
  }

  // if door should be opened
  if (openingState == 2) {
    Serial.println("opening: door opened");
    openingStartTime = millis();
    openingState = 3; // opening state
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(OPENER, HIGH);
    Firebase.RTDB.setBool(&fbdo2, "/door/openState", true);
    
    play = true;
    melodyIndex = 0;
    selectedMelody = 0;
  }

  if ((openingState == 3) && (millis() - openingStartTime) > 2000) { // how long door will be in open state
    Serial.println("opening: door closed");
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(OPENER, LOW);
    Firebase.RTDB.setBool(&fbdo2, "/door/openState", false);
    openingState = 0;
  }
}

 //Global function that handles stream data
void streamCallback(FirebaseStream data)
{
  //Print out all information
  Serial.println("Stream Data...");
  Serial.println(data.streamPath());
  Serial.println(data.dataPath());
  Serial.println(data.dataType());

  //Print out the value
  //Stream data can be many types which can be determined from function dataType

  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_integer)
      Serial.println(data.to<int>());
  else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_float)
      Serial.println(data.to<float>(), 5);
  else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_double)
      printf("%.9lf\n", data.to<double>());
  else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_boolean)
      Serial.println(data.to<bool>()? "true" : "false");
  else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_string)
      Serial.println(data.to<String>());
  else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json)
  {
      FirebaseJson *json = data.to<FirebaseJson *>();
      Serial.println(json->raw());
  }
  else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_array)
  {
      FirebaseJsonArray *arr = data.to<FirebaseJsonArray *>();
      Serial.println(arr->raw());
  }

  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_boolean) {
    if (data.to<bool>()){
      if (!play) {
        play = true;
        selectedMelody = 2;
      }
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(OPENER, HIGH);
      Firebase.RTDB.setBool(&fbdo2, "/door/openState", true);
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(OPENER, LOW);
      Firebase.RTDB.setBool(&fbdo2, "/door/openState", false);
    }
  }
     

}

//Global function that notifies when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout)
{
  if(timeout){
    //Stream timeout occurred
    Serial.println("Stream timeout, resume streaming...");
  }  
}

void PlayMelody() {
  if( play ) {
    int noteDuration = 1000 / noteDurations[selectedMelody][melodyIndex];
    if (outputTone) {
      // We are currently outputting a tone
      // Check if it's been long enough and turn off if so
      
      if (millis() - tonePreviousMillis >= noteDuration) {
        tonePreviousMillis = millis();
        noTone(tonePin);
        outputTone = false;
        //Update to play the next tone, next time
        ++melodyIndex;
        if (melodies[selectedMelody][melodyIndex] == 0) {
          melodyIndex = 0;
          play = false;
        }
      }
    } else {
      // We are currently in a pause
      // Check if it's been long enough and turn on if so
      int pauseBetweenNotes = 50; //noteDuration * 0.30;
      if (millis() - tonePreviousMillis >= pauseBetweenNotes) {
        tonePreviousMillis = millis();
        tone(tonePin, melodies[selectedMelody][melodyIndex]);
        outputTone = true;
        
      }
    }
  }
}
