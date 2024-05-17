#include <Arduino.h>
#include <CAN.h>

// Defines
#define BACKUP_SIZE 5
#define CENTER_OFFSET_PADDLE 10
#define CENTER_OFFSET_BALL 3
#define FIELD_WIDTH 255
#define FIELD_HEIGHT 149
#define PADDLE_HEIGHT 20
#define PADDLE_WIDTH 5
#define PADDLE_POS_DEFAULT 65


// Variables
// Player 1 = 0x02, Player 2 = 0x03
long PLAYER = 0x03;
long OPPONENT = (PLAYER == 0x02) ? 0x03 : 0x02;

// Array of the last 5 received CAN data sets containing x, y, and state
uint8_t dataBackup[BACKUP_SIZE][2];

// Index to keep track of the current position in the circular buffer
int dataIndex = 0;

/* GAME_STATES:
0 	Game start. This is only sent once to indicate the start of a new game.
1 	The game is running.
2 	Player 1 scored a point. The ball will get reset.
3 	Player 2 scored a point. The ball will get reset.
4 	No player scored a point due to timeout. The ball will get reset.
5 	Game over. Player 1 won the game (score of 5 reached). The entire game is reset (including the initial paddle positions).
6 	Game over. Player 2 won the game (score of 5 reached). The entire game is reset (including the initial paddle positions).
7 	Game over. No winner. The entire game is reset (including the initial paddle positions).
*/
uint8_t GAME_STATE = 0;

// Paddle Position
uint8_t PADDLE_POS = PADDLE_POS_DEFAULT;

// Paddle Postion Opponent
uint8_t PADDLE_POS_OPPONENT = PADDLE_POS_DEFAULT;

// Predicted Ball Position
uint16_t predictedY = 300;

// Prototypes
void onReceiveFunction(int packetSize);
void sendUpdate(int8_t direction);
void resetGame();
void reactToUpdate();
void handleScore();
void movePaddle();
void resetBuffer();
void moveToCenter();
void predictBallPosition(int x, int y, int v_x, int v_y);
void dumpBuffer();
void dumpData(uint8_t (&data)[3], int i);

// Setup
void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  Serial.println("CAN Receiver Callback");

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false); // turn off STANDBY
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true); // turn on booster

  // start the CAN bus at 250 kbps
  if (!CAN.begin(250000)) {
    Serial.println("Starting CAN failed!");
    while (1) delay(10);
  }
  Serial.println("Starting CAN!");

  // register the receive callback
  CAN.onReceive(onReceiveFunction);
}

void loop() {

}


// Receive Packets
void onReceiveFunction(int packetSize) {

  if (packetSize) {

    // If it's not from the player, ignore it
    if (CAN.packetId() == PLAYER) return;

    // Track Paddle Position of Opponent
    if (CAN.packetId() == OPPONENT){
      uint8_t direction = CAN.read();
      if (PADDLE_POS_OPPONENT + direction > 130 || PADDLE_POS_OPPONENT + direction < 0) return;
      PADDLE_POS_OPPONENT += direction;
      // Serial.print("Paddle Position Opponent: ");
      // Serial.println(PADDLE_POS_OPPONENT);
      return;
    }

    // Array to store received data
    uint8_t data[3];  

    // Read the data from the CAN packet
    int i = 0;
    while (CAN.available() && i < 3) {
      data[i] = CAN.read();
      i++;
    }
    
    // Update GAME_STATE
    GAME_STATE = data[2];
    // if (data[2] != GAME_STATE) {
    //   Serial.print("GAME_STATE: ");
    //   Serial.println(GAME_STATE);
    // }

    if (GAME_STATE != 1){
      resetBuffer();
    }

    // Store the received data in the circular buffer
    for (int j = 0; j < 2; j++) {
      dataBackup[dataIndex][j] = data[j];
    }
    

    // Print the received data
    //dumpData(data, i);
    //dumpBuffer();

    // React to Update
    reactToUpdate();
    // Update the index for circular buffer
    dataIndex = (dataIndex + 1) % BACKUP_SIZE;
  }
}


void dumpData(uint8_t (&data)[3], int i) {
      Serial.print("Data: ");
    for (int j = 0; j < i; j++) {
      Serial.print(data[j]);  // Print data in hexadecimal format
      Serial.print(" ");
    }
    Serial.println();
}


void dumpBuffer() {
  // Print the content of dataBackup
    Serial.println("Data Backup:");
    Serial.print("GAME_STATE: ");
    Serial.print(GAME_STATE, HEX);

    Serial.println();
    for (int j = 0; j < BACKUP_SIZE; j++) {
      Serial.print("Message ");
      Serial.print(j);
      Serial.print(": ");
      for (int k = 0; k < 2; k++) {
        Serial.print(dataBackup[j][k], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  Serial.println();
}

// Send Packets
void sendUpdate(int8_t direction) {
  // Check if paddle is in bounds
  if (PADDLE_POS + direction > 130 || PADDLE_POS + direction < 0) {
    return;
  }

  // Create a CAN packet
  CAN.beginPacket(PLAYER);
  CAN.write(direction);
  CAN.endPacket();
  PADDLE_POS += direction;
}

// React to Update
void reactToUpdate() {
  switch (GAME_STATE) {
    case 0:
      resetGame();
      break;
    case 1:
      movePaddle();
      break;
    case 2:
    case 3:
    case 4:
      handleScore();
      break;
    case 5:
    case 6:
    case 7:
      resetGame();
      break;
    default:
      break;
  }
}

// Reset Game
void resetGame() {
  //Serial.println("Called: ResetGame");
  PADDLE_POS = PADDLE_POS_DEFAULT;
  PADDLE_POS_OPPONENT = PADDLE_POS_DEFAULT;
  GAME_STATE = 0;
  predictedY = 300;
}

// Handle Score
void handleScore() {
  // Serial.println("Called: HandleScore");
  // Serial.print("Predicted Y: ");
  // Serial.println(predictedY);
  // Serial.print(", Actual Y: ");
  // Serial.print(dataBackup[dataIndex][1]);
  // Serial.print(", Paddle Pos: ");
  // Serial.println(PADDLE_POS);
  predictedY = 300;
  moveToCenter();
}

// Move Paddle
void movePaddle() {
  //Serial.println("Called: MovePaddle");
  // Calculate Vector components
  int prevIndex = (dataIndex - 1 + BACKUP_SIZE) % BACKUP_SIZE;
  int x = dataBackup[dataIndex][0];
  int y = dataBackup[dataIndex][1];
  int v_x =  x - dataBackup[prevIndex][0]; //v_x > 0 => move right, v_x < 0 => move left
  int v_y =  y - dataBackup[(dataIndex - 1) % BACKUP_SIZE][1]; //v_y > 0 => move up, v_y < 0 => move down

  // Output index & prevIndex
  // Serial.print("dataIndex: ");
  // Serial.print(dataIndex);
  // Serial.print(", prevIndex: ");
  // Serial.println(prevIndex);

  // Serial.print("v_x: ");
  // Serial.print(v_x);
  // Serial.print(", v_y: ");
  // Serial.println(v_y);

  int paddlePos_Offset = PADDLE_POS + CENTER_OFFSET_PADDLE;

  // nach rechts und Player i = Ball geht weg von uns
  if ((PLAYER == 0x02 && v_x > 0) || (PLAYER == 0x03 && v_x < 0)) {
    if (predictedY != 300) {
      // Compare predictedY and actualY 
      // Serial.println("Ball goes away from us: " + PLAYER);
      // Serial.print("predictedY: ");
      // Serial.print(predictedY + CENTER_OFFSET_BALL);
      // Serial.print(", actualY: ");
      // Serial.print(y + CENTER_OFFSET_BALL);
      // Serial.print(", paddlePos: ");
      // Serial.println(paddlePos_Offset);

      predictedY = 300;
    }
    moveToCenter();
  } else {
    
    // Ball goes towards us
    if (predictedY == 300){
      // Serial.println("Ball goes towards us: ");
      predictBallPosition(x, y, v_x, v_y);
    }
    if (PADDLE_POS + CENTER_OFFSET_PADDLE > predictedY - 10 || PADDLE_POS + CENTER_OFFSET_PADDLE < predictedY + 10) {
      // Serial.print("Paddle is at predicted Y: ");
      // Serial.println(predictedY);
      predictBallPosition(x, y, v_x, v_y);
    }
    if ((predictedY + CENTER_OFFSET_BALL) > paddlePos_Offset) {
      sendUpdate(1);
    } else if ((predictedY + CENTER_OFFSET_BALL) < paddlePos_Offset) {
      sendUpdate(-1);
    }
  }
}


void resetBuffer() {
  // Reset dataIndex
  dataIndex = 0;

  // Reset the data in the circular buffer
  for (int i = 0; i < BACKUP_SIZE; i++) {
    for (int j = 0; j < 2; j++) {
      dataBackup[i][j] = 0;
    }
  }
}

// Move to Center
void moveToCenter() {
  // Return Paddle to center (PADDLE_POS_DEFAULT)
  if (PADDLE_POS > PADDLE_POS_DEFAULT) {
    sendUpdate(-1);
  }
  else if (PADDLE_POS < PADDLE_POS_DEFAULT) {
    sendUpdate(1);
  }
}

// Predict Ball Position
void predictBallPosition(int x, int y, int v_x, int v_y) {
  // Print current and previous ball position
  //int prevIndex = (dataIndex - 1 + BACKUP_SIZE) % BACKUP_SIZE;
  // Serial.print("Previous Ball Position: ");
  // Serial.print(dataBackup[prevIndex][0]);
  // Serial.print(", ");
  // Serial.println(dataBackup[prevIndex][1]);

  // Serial.print("Current Ball Position: ");
  // Serial.print(x);
  // Serial.print(", ");
  // Serial.println(y);

  // // Print velocity
  // Serial.print("Velocity: ");
  // Serial.print(v_x);
  // Serial.print(", ");
  // Serial.println(v_y);

  // Ensure v_x is not zero to prevent division by zero
  if (v_x == 0) {
    predictedY = 300;
    return;
  }

  // Time to reach our paddle
  int timeToReachPaddle = (PLAYER == 0x02) ? (x - PADDLE_WIDTH) / abs(v_x) : (FIELD_WIDTH - PADDLE_WIDTH - x - 2*CENTER_OFFSET_BALL) / abs(v_x);

  // Print time to reach paddle
  // Serial.print("Time to reach paddle: ");
  // Serial.println(timeToReachPaddle);

  // Calculate the predicted Y position based on time and velocity
  int predictedYInt = y + v_y * timeToReachPaddle;

  // Print predicted Y position
  // Serial.print("Predicted Y RAW: ");
  // Serial.println(predictedYInt);

  // Calculate the number of bounces
  int bounces = predictedYInt < 0 ? 1+ abs(predictedYInt) / FIELD_HEIGHT : predictedYInt / FIELD_HEIGHT;
  // Serial.print("Bounces: ");
  // Serial.println(bounces);

  // Calculate top bounces
  // int topBounces = (bounces+1) / 2;
  // int bonceOffset = topBounces * 2 * CENTER_OFFSET_BALL;
  // predictedYInt = y + v_y * (timeToReachPaddle - bonceOffset);

  if (bounces % 2 == 0) {
    predictedYInt = predictedYInt < 0 ? FIELD_HEIGHT - abs(predictedYInt % FIELD_HEIGHT) : (predictedYInt % FIELD_HEIGHT);
  } else {
    predictedYInt =  predictedYInt < 0 ? abs(predictedYInt % FIELD_HEIGHT) : FIELD_HEIGHT - abs(predictedYInt % FIELD_HEIGHT);
  }

  predictedY = predictedYInt;
  // Serial.print("Predicted Y: ");
  // Serial.println(predictedY);
  // Serial.println();
  // Serial.println();
  
}
