#define DEBUG true

// IO
const int microphonePin = A0 ;  // define analog port A0
const int relayPin = A1;
int micInputVal = 0;    // set value to 0

// Timing
const int globalLoopMs = 1; // duration of loop delay
int periodClockMs = 0; // period measuring clock
const int pollPeriodMs = 20; // duration of polling period 
const int postKnockDelayMs = 150; // ms of delay following knock detection
const int winDelayMs = 5000; // ms of delay upon completion

// Mic Logic
int maxVolumeLastPeriod = 0; // per-period maximum value 
const int knockThreshold = 100; // microphone threshold for knock detection
const int pollValueLen = pollPeriodMs/globalLoopMs;
const int pollValues[pollPeriodMs/globalLoopMs];

// Puzzle 
int knockCount = 0; // number of knocks detected during the last passcode window
const int maxPasscodeKnockPeriodMs = 500; // time window within which the knocks must come
int passcodeKnockPeriodClockMs = 0; // clock for passcode window

const int passcodeLength = 3;
int passcode[passcodeLength] = {9,3,4};
int bufferedKnockCodes[passcodeLength];

void setup() 
{
  if (DEBUG) {
    Serial.begin(9600);
    Serial.println("\nNew Session:");
  }
  // Setup relay DIO
  pinMode(relayPin, OUTPUT);
} 

void loop() 
{
  // Read microphone level
  micInputVal = analogRead(microphonePin);
  insertAndShiftLeft(pollValues, pollValueLen, micInputVal);

  // Loop Delay and Clock Increments
  delay(globalLoopMs);  //delay 2ms
  periodClockMs += globalLoopMs;
  passcodeKnockPeriodClockMs += globalLoopMs;

  // Mic-polling window completion checks
  if (periodClockMs >= pollPeriodMs) {

    // Check if knock happened
    if (checkKnock()) {
      knockCount += 1;
      passcodeKnockPeriodClockMs = 0; // reset passcode knock period window

      if (DEBUG) {
        Serial.print("Knock ");
        Serial.println(knockCount);
      }
      
      delay(postKnockDelayMs);
    }

    // Reset window values
    maxVolumeLastPeriod = 0;
    periodClockMs = 0;
  }

  // Knock code window completion checks
  if (passcodeKnockPeriodClockMs >= maxPasscodeKnockPeriodMs) {
    if (knockCount != 0) {
      insertAndShiftLeft(bufferedKnockCodes, passcodeLength, knockCount);
    }

    knockCount = 0;
    passcodeKnockPeriodClockMs = 0;

    if (DEBUG) {
      Serial.println("Reseting passcode knock period clock after inactivity.");
      Serial.print("Current passcode buffer is: ");
      printArr(bufferedKnockCodes, passcodeLength);
    }

    if (checkPasscode()) {
      if (DEBUG) {
        Serial.println("Win!");
      }
      digitalWrite(relayPin, HIGH);
      delay(winDelayMs);
      insertAndShiftLeft(bufferedKnockCodes, passcodeLength, -1); // insert a bogus value to reset code buffer
    } else digitalWrite(relayPin, LOW);
  }
} 

void printArr(int* array, int len) {
  for (int i = 0; i < len; i++) {
      Serial.print(*(array+i));
      Serial.print(" ");
  }
  Serial.println("");
}

int maxVal(int* array, int len) {
  int max = INT16_MIN;
  for (int i = 0; i < len; i++) 
    if (*(array+i) > max)
      max = *(array+i);
  return max;
}

bool checkKnock() {
  return maxVal(pollValues, pollValueLen) >= knockThreshold;
}

void insertAndShiftLeft(int* array, int len, int knockCodeValue) {
  for (int i = 0; i < len-1; i++) 
    *(array + i) = *(array + i + 1);
  *(array + len - 1) = knockCodeValue;
}

bool checkPasscode() {
  for (int i = 0; i < passcodeLength; i++) 
    if (bufferedKnockCodes[i] != passcode[i])
      return false;
  return true;
}