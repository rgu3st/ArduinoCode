#include <IRremote.hpp>

#define IRPOWER 9
#define IRSIGNALPIN 10
#define NUMTONES 10
#define NUMBUTTONPINS 10

//IR remote stuffs:
IRrecv irrecv(IRSIGNALPIN);
decode_results irResults;

// States:
enum States { initial,
              playingSounds,
              awaitingUserButtonInput,
              userButtonInput,
              buttonInputSuccess,
              buttonInputFail };
States currentState = States::initial;
States stateLastLoop = States::buttonInputSuccess;

int speakerPin = 2;
// delay
int delayBetweenBeeps = 140;  // in ms
int delayBetweenNumbers = 900;
int delayBetweenPlays = 3000;
int shortDelay = 10;
//For timing:
unsigned long timeoutMillis = 0;
unsigned long awaitUserInputMillis = 7000;  
unsigned long maxUserInputMillis = 30000;

// Represents breaker state, false is breaker is off
bool breakersOn[NUMTONES] = { false };
bool breakersPressed[NUMTONES] = {false};
// Lowest pitch is 1, highest pitch
int tones[NUMTONES] = {
  100,
  200,
  300,
  400,
  700,
  900,
  1100,
  1400,
  1600,
  1800
};
int failTone = 220;
// Use pins 4,5,6,7 as binary input for which button was pressed
int buttonPins[NUMBUTTONPINS] = { 3, 4, 5, 6, 7, A4, A3, A2, A1, A0 };
int curButtonPressed = 0;
int numButtonInputs = 0;




// Randomly picks a number of breakers to turn on
int numBreakers = random(3, 6);
//int numBreakers = 2; // TODO: get rid of this line after debugging

/*****************************************************************
/ Setup Functions:
/*****************************************************************/
void RandomizeBreakersOn() {
  randomSeed(analogRead(0));  // Seed with noise from pin 0
  
  for (int i = 1; i <= NUMTONES; i++) {
      breakersOn[i] = false;
  }

  for (int i = 1; i <= numBreakers; i++) {
    breakersOn[random(0, NUMTONES - 1)] = true;
  }

  for (int i = 1; i <= NUMTONES; i++) {
      if(breakersOn[i-1]){
        Serial.print("Breaker on: ");
        Serial.println(i);
      }
  }

}

void InitButtonPins() {
  for (int p = 0; p < NUMBUTTONPINS; p++) {
    pinMode(buttonPins[p], INPUT_PULLUP);
  }
}

void InitRemoteControl() {
  pinMode(IRPOWER, OUTPUT);
  digitalWrite(IRPOWER, HIGH);
  irrecv.enableIRIn();
}

void InitSpeaker() {
  pinMode(speakerPin, OUTPUT);
}

/*****************************************************************
/ Sound Functions:
/*****************************************************************/
bool PlayErrorBeeps(int numErrorBeeps) {
  int errorBeepDuration = 70;
  int errorTone = 800;

  for (int i = 0; i < numErrorBeeps; i++) {
    tone(speakerPin, errorTone);
    delay(errorBeepDuration);
    noTone(speakerPin);
    delay(errorBeepDuration);
  }
}

bool beepNumberOfTimes(int numTimes) {
  if (numTimes < 1 || numTimes > NUMTONES) {
    return false;
  }
  int index = numTimes - 1;  // Adjusting for zero-indexed array

  for (int i = 1; i <= numTimes; i++) {
    tone(speakerPin, tones[index]);
    delay(delayBetweenBeeps);
    noToneDelay(speakerPin, delayBetweenBeeps);
  }

  return true;
}

void testAllTones() {
  for (int i = 0; i < NUMTONES; i++) {
    tone(speakerPin, tones[i]);
    delay(delayBetweenBeeps);
  }
  noTone(speakerPin);
}

void noToneDelay(int pin, int millisDelay) {
  noTone(pin);
  delay(millisDelay);
}

void ToneDuration(int frequency, int durationMillis){
  tone(speakerPin, frequency);
  delay(durationMillis);
  noTone(speakerPin);
}

/*****************************************************************
/ IO Functions:
/*****************************************************************/
bool CheckForSoundsStartSignal() {
  if (irrecv.decode_old(&irResults)) {
    //Serial.println(irResults.value, HEX);
    irrecv.resume();
    TransitionToPlayingSounds();
  }
}

int GetButtonNumberPressed() {
  int buttonPressed = 0;
  for (int p = 0; p < NUMBUTTONPINS; p++) {
    if (LOW == digitalRead(buttonPins[p])) {  
      buttonPressed = p+1;            
    }
  }
  return buttonPressed;
}

bool SetCurButtonPressed(int buttonPressed) {
  if (1 > buttonPressed || NUMTONES < buttonPressed) {  // 1 to 10
    return false;
  }
  curButtonPressed = buttonPressed;
  return true;
}

void CheckForButtonInput() {
  int buttonPressed = GetButtonNumberPressed();
  if (0 < buttonPressed) {
    TransitionToUserButtonInput();
    PlayErrorBeeps(1);
    SetCurButtonPressed(buttonPressed);
    
    if(currentState == States::userButtonInput){
      SetButtonPressed(buttonPressed);
    }
  }
}

void SetButtonPressed(int buttonPressed){
  Serial.print("Button pressed: ");
  Serial.println(buttonPressed);
  int breakerIndex = curButtonPressed - 1;
  if( true == breakersOn[ breakerIndex ] ){
    Serial.print("Turning off breaker ");
    Serial.println(curButtonPressed);
    breakersPressed[breakerIndex] = true;
    //breakersOn[breakerIndex] = false;
  }
  else{
    Serial.print("Wrong breaker: ");
    Serial.println(curButtonPressed);
    TransitionToButtonFailState();
  }
  
  if(YouWon()){
    TransitionToButtonSuccessState();
  }
  delay(shortDelay);
}

void PlayBreakerSounds() {
  for (int i = 1; i <= NUMTONES; i++) {
    if (breakersOn[i-1]) {
      beepNumberOfTimes(i);
      delay(delayBetweenNumbers);
    }
  }
  
}

void PlayWinSounds(){
  int eighth = 220;
  int quarter = 400;
  int c = 261;
  int bf = 233;
  int b = 246;
  int af = 208;
  ToneDuration(c, eighth);
  ToneDuration(c, eighth);
  ToneDuration(c, eighth);
  ToneDuration(c, quarter);
  ToneDuration(af, quarter);
  ToneDuration(bf, quarter);
  ToneDuration(c, eighth);
  ToneDuration(0, eighth-120);
  ToneDuration(bf, eighth-60);
  ToneDuration(c, quarter);
}

void PlayFailTones(){
  int eighth = 220;
  int quarter = 400;
  int c = 261;
  ToneDuration(c, eighth);
  ToneDuration(c, eighth);
  ToneDuration(c, eighth);
  ToneDuration(failTone, 1000);
  delay(delayBetweenNumbers);
}


/*****************************************************************
/ State Functions:
/*****************************************************************/
void TransitionToPlayingSounds() {
  // TODO: Check for bad transition criteria
  if (currentState == States::initial) {
    RandomizeBreakersOn();
  }
  currentState = States::playingSounds;
}

void TransitionToAwaitingUserInput(){
  currentState = States::awaitingUserButtonInput;
  timeoutMillis = millis() + awaitUserInputMillis;
} 

void TransitionToUserButtonInput() {
  if (currentState == States::awaitingUserButtonInput || currentState == States::userButtonInput ) {
    currentState = States::userButtonInput;
    timeoutMillis = millis() + maxUserInputMillis;
  }
  else{
    PlayErrorBeeps(3);
  }
}

void TransitionToButtonFailState(){
  // Maybe add some logic checks?
  //PlayErrorBeeps(3);
  currentState = States::buttonInputFail;
  
}

void TransitionToInitialState(){
  if( States::buttonInputSuccess == currentState){
    currentState = States::initial;
  }
  else{
    Serial.println("Can't transition to initial again until we win!");
  }
}

void TransitionToButtonSuccessState(){
  currentState = States::buttonInputSuccess;
}

bool Timedout(){
  if( millis() > timeoutMillis){
    return true;
  }
  else{
    return false;
  }
}

bool YouWon(){
  for (int i = 1; i <= NUMTONES; i++) {
    if( breakersPressed[i-1] != breakersOn[i-1] ){
      return false;
    }
  }
  return true; // All the breakers pressed match the breakers on. Win!
}

void LogTransition(){
  if(stateLastLoop != currentState){
    Serial.print("Transitioned from ");
    PrintStatename(stateLastLoop);
    Serial.print(" to ");
    PrintStatename(currentState);
    Serial.println("");
  }
}

void PrintStatename(States state){
  switch(state){
      case States::initial:
        Serial.print("initial");
        break;
      case States::awaitingUserButtonInput:
        Serial.print("awaitingUserButtonInput");
        break;
      case States::userButtonInput:
        Serial.println("userButtonInput");
        break;
      case States::playingSounds:
        Serial.print("playingSounds");
        break;
      case States::buttonInputFail:
        Serial.print("buttonInputFail");
        break;
      case States::buttonInputSuccess:
        Serial.print("buttonInputSuccess");
        break;
      default:
        Serial.print(" ?STATE? ");
    }
}

/*****************************************************************
/ Built-in Functions:
/*****************************************************************/
void setup() {
  InitSpeaker();
  InitRemoteControl();
  InitButtonPins();
  Serial.begin(9600);
  PlayErrorBeeps(1);
  Serial.println("\n\nStarting Fresh!\n-----------------------------------");
  currentState = States::initial;
  stateLastLoop = States::buttonInputSuccess;  
}

void loop() {
  switch (currentState) {
    case States::initial:
      CheckForSoundsStartSignal();
      CheckForButtonInput();
      LogTransition();
      break;
    case States::awaitingUserButtonInput:
      CheckForButtonInput();
      if(Timedout()){
        TransitionToPlayingSounds();
      }
      LogTransition();
      break;
    case States::userButtonInput:
      CheckForButtonInput();
      if(Timedout()){
        TransitionToPlayingSounds();
      }
      LogTransition();
      break;
    case States::playingSounds:
      PlayBreakerSounds();
      TransitionToAwaitingUserInput();
      LogTransition();
      break;
    case States::buttonInputFail:
      for(int i=0; i<NUMTONES; i++){
        breakersPressed[i] = false;
      } 
      PlayFailTones();
      TransitionToPlayingSounds();
      LogTransition();
      break;
    case States::buttonInputSuccess:
      PlayWinSounds();
      TransitionToInitialState();
      LogTransition();
      break;
    default:
      break;
      PlayErrorBeeps(3);
  }
  
  stateLastLoop = currentState;
}
