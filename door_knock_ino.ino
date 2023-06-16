/* PaniQ Escape
 * Wizarding Room Entry Door Knocking Puzzle Controller
 * Riley Waite
 * June '23
 * riley.s.waite@gmail.com
 */

// If set 'true', debugging statements will be printed over serial
#define DEBUG true

// Puzzle passcode
const int passcodeLength = 3; // number of digits in the passcode
int passcode[passcodeLength] = {9,3,4}; // each number is a distinct "instance" of knocking. e.g. {3,4} means "three knocks" then a pause, followed by "four knocks"

// IO
const int microphonePin = A0 ;  // microphone signal
const int relayPin = A1; // relay output

// Timing constants
const int globalLoopMs = 1; // duration of global loop delay
const int pollPeriodMs = 20; // duration of microphone polling period (ms of signal which is analyzed)
const int postKnockDelayMs = 150; // ms of delay following a knock detection. necessary to reduce multi-knock "ringing". minimum time between knocks to count. 
const int unlockHoldMs = 5000; // ms that system will hold maglock unlocked after correct passcode solve
const int maxPasscodeKnockPeriodMs = 500; // time window within which the knocks must come. knocks slower than this will be counted as seperate 'numbers' 

// Clocks
int periodClockMs = 0; // period measuring clock
int passcodeKnockPeriodClockMs = 0; // clock for passcode window

// Mic signal processing
int micInputVal = 0;    // microphone signal value (updated every global loop)
int maxVolumeLastPeriod = 0; // per-period maximum microphone value cache (updated every poll period)
const int knockThreshold = 100; // microphone threshold for knock detection
const int micBufferLen = pollPeriodMs/globalLoopMs; // number of values stored in microphone signal buffer
const int micBuffer[pollPeriodMs/globalLoopMs]; // buffer for microphone measurements

// Puzzle logic
int knockCount = 0; // number of knocks detected during the last passcode window
int bufferedKnockCodes[passcodeLength]; // sliding buffer of knocked codes

// Initialize pins, serial
void setup() 
{
  if (DEBUG) {
    Serial.begin(9600);
    Serial.println("\nNew Session:");
  }
  // Setup relay DIO
  pinMode(relayPin, OUTPUT);
} 

// Global loop
void loop() 
{
  // Mic reading
  micInputVal = analogRead(microphonePin); // read microphone level
  insertAndShiftLeft(micBuffer, micBufferLen, micInputVal); // insert measurement into buffer

  // Loop Delay and Clock Increments
  delay(globalLoopMs);  // delay global loop
  periodClockMs += globalLoopMs; // increment clock
  passcodeKnockPeriodClockMs += globalLoopMs; // increment clock

  // Mic-polling window completion checks
  if (periodClockMs >= pollPeriodMs) { // if a period has passed,

    // Check if knock happened
    if (checkKnock()) {
      knockCount += 1; // increment knock count 
      passcodeKnockPeriodClockMs = 0; // reset passcode knock period window

      if (DEBUG) {
        Serial.print("Knock ");
        Serial.println(knockCount);
      }
      
      delay(postKnockDelayMs); // delay to avoid multi-knock ringing
    }

    // Reset window values
    maxVolumeLastPeriod = 0;
    periodClockMs = 0;
  }

  // Knock code window completion checks
  if (passcodeKnockPeriodClockMs >= maxPasscodeKnockPeriodMs) {
    if (knockCount != 0) { // if any knocks were detected last knock period,
      insertAndShiftLeft(bufferedKnockCodes, passcodeLength, knockCount); // insert knocks into passcode buffer
    }

    // reset counts and period clock for new knock period
    knockCount = 0;
    passcodeKnockPeriodClockMs = 0;

    if (DEBUG) {
      Serial.println("Reseting passcode knock period clock after inactivity.");
      Serial.print("Current passcode buffer is: ");
      printArr(bufferedKnockCodes, passcodeLength);
    }

    // passcode checking
    if (checkPasscode()) { // if the code in the knock buffer matches prescribed passcode,
      if (DEBUG) {
        Serial.println("Win!");
      }
      digitalWrite(relayPin, HIGH); // unlock magloc
      delay(unlockHoldMs); // hold in this paused+unlocked state for some time to allow players to open and enter
      insertAndShiftLeft(bufferedKnockCodes, passcodeLength, -1); // insert a bogus value to "reset" the system
    } else digitalWrite(relayPin, LOW); // if code doesn't match, make sure relay is still closed (maglock engaged)
  }
} 

// prints all values in an array
void printArr(int* array, int len) {
  for (int i = 0; i < len; i++) {
      Serial.print(*(array+i));
      Serial.print(" ");
  }
  Serial.println("");
}

// returns max value of array
int maxVal(int* array, int len) {
  int max = INT16_MIN;
  for (int i = 0; i < len; i++) 
    if (*(array+i) > max)
      max = *(array+i);
  return max;
}

// checks if maximum mic value in buffer exceeds threshold
bool checkKnock() {
  return maxVal(micBuffer, micBufferLen) >= knockThreshold;
}

// inserts value into end of array, shifting all others left and "forgetting" the first value
void insertAndShiftLeft(int* array, int len, int knockCodeValue) {
  for (int i = 0; i < len-1; i++) 
    *(array + i) = *(array + i + 1);
  *(array + len - 1) = knockCodeValue;
}

// checks if knock code stored in buffer matches prescribed code
bool checkPasscode() {
  for (int i = 0; i < passcodeLength; i++) 
    if (bufferedKnockCodes[i] != passcode[i])
      return false;
  return true;
}