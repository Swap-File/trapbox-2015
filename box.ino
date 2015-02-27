#include <MFRC522.h>
#include <SPI.h>
#include <math.h>
#if defined(__AVR_ATtiny85__)
#error "This code is for ATmega boards, see other example for ATtiny."
#endif
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TiCoServo.h>
#include <Metro.h> //Include Metro library

#define N_LEDS       60
#define LED_PIN       7

#define SERVO_PIN    9
#define SERVO_MIN 1000 // 1 ms pulse
#define SERVO_MAX 2000 // 2 ms pulse

#define SAD 10
#define RST 8



MFRC522 nfc(SAD, RST);
Adafruit_NeoPixel  strip = Adafruit_NeoPixel(N_LEDS, LED_PIN);
Adafruit_TiCoServo servo;

byte card_data[11][4] = {  
  {    
    0x12, 0xf0, 0xd2, 0xb5                                                                          }
  ,   {        
    0x6d, 0x1e, 0x78, 0xe3                                                                          }
  ,   {    
    0x7d, 0xf5, 0x79, 0xe3                                                                          } 
  , {    
    0x3D, 0x4b, 0x79, 0xe3                                                                          }
  ,  {    
    0x9d, 0x6f, 0x7a, 0xe3                                                                          }
  ,  {    
    0x6d, 0xee, 0x79, 0xe3                                                                          }
  ,  {    
    0x1d, 0x8d, 0x73, 0xe3                                                                          }
  ,  {    
    0x2d, 0x52, 0x77, 0xe3                                                                          }
  ,  {    
    0x3d, 0xf2, 0x79, 0xe3                                                                          }
  ,  {    
    0xdd, 0x47, 0x79, 0xe3                                                                          }
  ,  {    
    0x0d, 0x02, 0x78, 0xe3                                                                          }
};


const float M_E =  2.71828;

const int effect_resolution = 256;
const int breathingrate = 2;
byte effect_array[effect_resolution];

boolean L_locked = true;
boolean R_locked = true;

Metro ledMetro = Metro(1); 

int run_mode = 0;
boolean light1 = false;
boolean light2 = false;
int breath_effect = 0;

void setup() {
  //lid switch
  pinMode(A0,INPUT);
  digitalWrite(A0, HIGH);

  //disarm switch
  pinMode(A1,INPUT);
  digitalWrite(A1, HIGH);

  //lock enable
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);
  //lock
  pinMode(3,OUTPUT);
  digitalWrite(3,LOW);
  //unlock left
  pinMode(4,OUTPUT);
  digitalWrite(4,LOW);
  //unlock right
  pinMode(5,OUTPUT);
  digitalWrite(5,LOW);

  digitalWrite(2,LOW);

  unlock_L();
  unlock_R(); 
  run_mode = 11;

  Serial.begin(115200);

  //effect array lookup table
  for ( int i = 0; i < effect_resolution; i++ ) { 
    effect_array[i] =  255*((exp(sin( M_PI/2  +(float(i)/(effect_resolution/breathingrate)*M_PI))) )/ (M_E));
    Serial.println( effect_array[i] );
  }

  //start servo and move to unlocked
  servo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
  servo.write(175);   

  //start LED strip empty
  strip.clear();
  strip.begin();

  //start NFC card reader
  SPI.begin();
  Serial.println("Looking for MFRC522.");
  nfc.begin();
  byte version = nfc.getFirmwareVersion();
  if (! version) {
    Serial.print("Didn't find MFRC522 board.");
    while(1); //halt
  }
  Serial.print("Found chip MFRC522 ");
  Serial.print("Firmware ver. 0x");
  Serial.print(version, HEX);
  Serial.println(".");

}





void loop() {

  if (ledMetro.check() ){
    breath_effect++;
    if (breath_effect == effect_resolution){
      breath_effect = 0;
    }
  }

  byte status;
  byte data[MAX_LEN];
  byte serial[5];
  int i, j, pos;


  // Send a general request out into the aether. If there is a tag in
  // the area it will respond and the status will be MI_OK.
  status = nfc.requestTag(MF1_REQIDL, data);

  if (status == MI_OK) {
    // calculate the anti-collision value for the currently detected
    // tag and write the serial into the data array.
    status = nfc.antiCollision(data);
    memcpy(serial, data, 5);

    Serial.println("The serial nb of the tag is:");
    for (i = 0; i < 3; i++) {
      Serial.print(serial[i], HEX);
      Serial.print(", ");
    }
    Serial.println(serial[3], HEX);

    int card_number=0;
    for (card_number ;  card_number < 11;card_number++){
      if(memcmp(serial,card_data[card_number],3) == 0){
        break; 
      }
    }

    nfc.selectTag(serial);
    nfc.haltTag();

    switch (card_number) {
    case 0:
      {
        light1 = false;
        light2 = false;
        if(run_mode < 11){
          //go into service mode, first swipe
          //unlock the box, unlock the launchers
          unlock_L();
          unlock_R(); 
          run_mode = 11;
        }
        else if(run_mode == 11){
          servo.write(0); 
          run_mode = 12;
        } 
        else if(run_mode == 12){
          //lock the launchers
          servo.write(170); 
          run_mode = 13;
        }
        else if(run_mode == 13){
          //lock the doors
          lock_LR(); 
          run_mode = 0;
        }
        break;
      }
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      {
        if(run_mode < 10){
          light1 = true;
        }
        break;
      }
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
      {
        if(run_mode < 10){
          light2 = true;
        }
        break;
      }
    default: 
      Serial.println("No Match"); 
    }
  }

  strip.clear();

  if (run_mode > 10){
    if (digitalRead(A0)){
      debug_color(10);
    }
    if (digitalRead(A1)){
      debug_color(29);
    }
  }

  if (run_mode== 11){
    debug_color(16);
    debug_color(23); 
  }

  if (run_mode== 12){
    debug_color(17);
    debug_color(22); 
  }

  if (run_mode== 13){
    debug_color(18);
    debug_color(21); 
  }

  if (run_mode == 0 || run_mode == 1){

    if (light1){
      if (digitalRead(A1)){
        for (int i =0 ;  i < 13;i++){
          trap_color(i);
        }
      }
      else{
        for (int i =0 ;  i < 13;i++){
          safe_color(i);
        }
      }
      unlock_L();
    }

    if (light2){
      if (digitalRead(A1)){
        for (int i =27 ;  i < 40;i++){
          trap_color(i);
        }
      }
      else{
        for (int i =27 ;  i < 40;i++){
          safe_color(i);
        }
      }
      unlock_R();
    }
  }

  if (light2 && light1 && run_mode == 0){
    run_mode = 1; 
    unlock_L();
    unlock_R();
  }

  if(run_mode == 0){
    if (digitalRead(A1)){
      trap_color(19);
      trap_color(20);
    }
    else{
      safe_color(19);
      safe_color(20);
    }
  }
  else if (run_mode == 1){
    unlocked_color(19);
    unlocked_color(20);
  }


  if (run_mode == 1){
    if (digitalRead(A0) == 0 ){
      if (digitalRead(A1)){
        strip.clear();
        strip.show();   
        servo.write(0);  
        delay(500);
        servo.write(170);  
      }
      run_mode = 2;   
    }
  }
  strip.show();   
}

void safe_color(int index){
  strip.setPixelColor(index, effect_array[breath_effect], 0 , effect_array[breath_effect]); 
}

void trap_color(int index){
  strip.setPixelColor(index, effect_array[breath_effect], 0 ,0); 
}

void unlocked_color(int index){
  strip.setPixelColor(index, 0, effect_array[breath_effect] , 0); 
}

void debug_color(int index){
  strip.setPixelColor(index, effect_array[breath_effect] >>1, effect_array[breath_effect]>>1, effect_array[breath_effect]>>1);   
}

void lock_LR(void){
  if (L_locked == false || R_locked == false){
    digitalWrite(3,HIGH);
    delay(10);
    digitalWrite(3,LOW);
    L_locked = true;
    R_locked = true;
  }
}





void unlock_R(void){
  if (R_locked == true){
    digitalWrite(4,HIGH);
    delay(10);
    digitalWrite(4,LOW);
    R_locked=false;
  }

}


void unlock_L(void){
  if (L_locked == true){
    digitalWrite(5,HIGH);
    delay(10);
    digitalWrite(5,LOW);
  }
  L_locked=false;
}
































