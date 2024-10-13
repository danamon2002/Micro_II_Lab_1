#include <Keypad.h>
#include <string.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {52, 50, 48, 46}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {44, 42, 40, 38}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//Global var required for tracking index in command string
// Command storage
int commandIndex = 0;
char command[6] = "";  // Command should in theory never need to exceed 5 characters: ie 'A100#'
// Traffic Light Timer Storage
int redTimer = -1;
int greenTimer = -1;
int countdown = 1;
int lastState = 2;
// Light State Storage
enum light {off = 0, red = 13, yellow = 12, green = 11};
light trafficLight = off;
bool initialState = true;
boolean initialRedBlinkState = 0; // Used by ISR to check if light has been initialized.

void setup(){

  // Set up output pins
  pinMode(10, OUTPUT); //buzzer
  pinMode(11, OUTPUT); //green
  pinMode(12, OUTPUT); //yellow
  pinMode(13, OUTPUT); //red

  // init serial connection for debug
  Serial.begin(9600);

  // =================== INITIALIZE INTERRUPTS ===================
  cli();//stop interrupts

  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
  
}

// =================== INTERRUPT SERVICE ROUTINE ===================
ISR(TIMER1_COMPA_vect){ //timer1 interrupt

  if (initialState == true){  // if the light is in the initial state, blink red light.
      if (initialRedBlinkState){
        digitalWrite(13, HIGH);
        initialRedBlinkState = 0;
      }
      else{
        digitalWrite(13, LOW);
        initialRedBlinkState = 1;
      }
    }

  if(countdown > 0){
    countdown--;
  }
}

void readCommand(){
  uint seconds = 0;
  // Read and reset the global command variable to be empty.
  // Read:
  switch(command[0]){
    case 'A': // Red Light Set
      Serial.println("Red light command.");
      sscanf(command, "%*[^0-9]%d", &seconds); // write integer info from command to seconds int via its address
      redTimer = seconds * 2;
      break;
    case 'B': // Green  Light Set
      Serial.println("Green light command.");
      sscanf(command, "%*[^0-9]%d", &seconds); // write integer info from command to seconds int via its address
      greenTimer = seconds * 2;
      break;
    default: // Other commands
      Serial.println("Invalid command! Start command with \'A\' or \'B\'.");
  }

  // Reset:
  for(int i = 0; i < 5; i++){
          command[i] = ' ';
          command[5] = '\0';
        }
  commandIndex = 0;
}

  bool beginLight() {
    Serial.println("STAR PRESSED");
    if (redTimer > 0 && greenTimer > 0){
      return true;
    }
    else {
      return false;
    }
  }

char toggleLight(){
  // determine which light is on and toggle it.
  if (countdown % 2 == 1) {
    Serial.println("Light on!");
    digitalWrite(10, HIGH);
    return 0x1;
  } else if (countdown % 2 == 0){
    Serial.println("Light off!");
    digitalWrite(10, LOW);
    return 0x0;
  }
}

void readTimers(){
  // Print the timer values, in half-seconds, for the red and green lights.
  // Yellow lights are always 3 seconds, per the project manual.
  Serial.println("Red Timer: ");
  Serial.println(redTimer);
  Serial.println("Green Timer: ");
  Serial.println(greenTimer);
}

void printLights(){
  // Print the state of the traffic light.
  if (trafficLight == red){
    Serial.println("Light is red\n");
  } else if (trafficLight == yellow){
    Serial.println("Light is yellow\n");
  } else if (trafficLight == green){
    Serial.println("Light is green\n");
  } 
}

void loop(){

// CODE FOR MAIN LOOP: SETS STATES OF THE TRAFFIC LIGHT.

  char customKey = customKeypad.getKey();
  
  if (customKey){  // When key pressed...
    //Serial.println(customKey); // Print current key press.

    if((customKey != '#') && (customKey != '*')){  // For all keys except '#'...
      command[commandIndex] = customKey;
      commandIndex += 1;
      if(commandIndex > 5){
        readCommand();
      }
    }
    else if (customKey == '#') {                // for '#' pressed:
        Serial.println(command);
        readCommand();
        readTimers();
    }
    else if (customKey == '*') {
        if (beginLight()){
          Serial.println("Light is ready\n");
          trafficLight = yellow;
          initialState = false;
          OCR1A = 15624 / 2;

        }
        else {
          Serial.println("Still need to enter real commands\n");
        }
    }
  }


  if (countdown <= 6 && lastState != countdown){
    if (trafficLight == red){
      digitalWrite(trafficLight, toggleLight());
    } else if(trafficLight == green){
      digitalWrite(trafficLight, toggleLight());
    }
    lastState = countdown;
  }
  
  if(countdown <= 0){ // Logic for when light has completed its time cycle.
      //Serial.println("Counter over!\n");
      if (trafficLight == red){
          // set light green
          countdown = greenTimer;
          trafficLight = green;
          digitalWrite(10, LOW);
          digitalWrite(12, LOW);
          digitalWrite(13, LOW);
          digitalWrite(11, HIGH);
      } else if (trafficLight == yellow) {
        // set light red
          countdown = redTimer;
          trafficLight = red;
          digitalWrite(10, LOW);
          digitalWrite(12, LOW);
          digitalWrite(13, HIGH);
          digitalWrite(11, LOW);
      } else if (trafficLight == green) {
          // set light yellow
          countdown = 3 * 2;
          trafficLight = yellow;
          digitalWrite(10, LOW);  // comment out this line if you want buzzer on during yellow light.
          digitalWrite(12, HIGH);
          digitalWrite(13, LOW);
          digitalWrite(11, LOW);
      }
      printLights();  
    }
}
