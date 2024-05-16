#include <Arduino.h>
#include <CAN.h>

#define BACKUP_SIZE 5

// Variables

// Player 1 = 0x02, Player 2 = 0x03
long PLAYER = 0x02;
long OPPONENT = PLAYER = 0x02 ? 0x03 : 0x02;

// Array of the last 5 received CAN data sets containing x, y, and state
uint8_t dataBackup[BACKUP_SIZE][2];
int dataIndex = 0;  // Index to keep track of the current position in the circular buffer


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
uint8_t PADDLE_POS = 65;

// Paddle Postion Opponent
uint8_t PADDLE_POS_OPPONENT = 65;

// Prototype
void onReceiveFunction(int packetSize);
void sendUpdate(int8_t direction);
void resetGame();
void reactToUpdate();
void handleScore();
void movePaddle();
void resetBuffer();
void moveToCenter();

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
  // do nothing
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
      Serial.print("Paddle Position Opponent: ");
      Serial.println(PADDLE_POS_OPPONENT);
      return;
    }

    uint8_t data[3];  // Array to store received data

    Serial.print("Received packet with size: ");
    Serial.println(packetSize);

    // Read the data from the CAN packet
    int i = 0;
    while (CAN.available() && i < 3) {
      data[i] = CAN.read();
      i++;
    }
    
    // Update GAME_STATE
    GAME_STATE = data[2];

    if (GAME_STATE != 1){
      resetBuffer();
    }

    // Store the received data in the circular buffer
    for (int j = 0; j < 2; j++) {
      dataBackup[dataIndex][j] = data[j];
    }
    dataIndex = (dataIndex + 1) % BACKUP_SIZE;  // Update the index for circular buffer
    
    reactToUpdate();

    // Print the received data
    Serial.print("Data: ");
    for (int j = 0; j < i; j++) {
      Serial.print(data[j], HEX);  // Print data in hexadecimal format
      Serial.print(" ");
    }
    Serial.println();
    Serial.println();

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
  }
}

// Send Packets
void sendUpdate(int8_t direction) {

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
  switch (GAME_STATE)
  {
  case 0:
    resetGame();
    break;
  case 1:
    movePaddle();
    break;
  case 2:
    handleScore();
    break;
  case 3:
    handleScore();
    break;
  case 4:
    handleScore();
    break;
  case 5:
    resetGame();
    break;
  case 6:
    resetGame();
    break;
  case 7:
    resetGame();
  default:
    break;
  }
}

// Reset Game
void resetGame() {
  Serial.print("Called: ResetGame");
  PADDLE_POS = 65;
  PADDLE_POS_OPPONENT = 65;
  GAME_STATE = 0;
}

// Handle Score
void handleScore() {
  Serial.print("Called: HandleScore");
  moveToCenter();
}

// Move Paddle
void movePaddle() {
  Serial.print("Called: MovePaddle");
  // Calculate Vector
  int v_x =  dataBackup[dataIndex][0] - dataBackup[(dataIndex - 1) % BACKUP_SIZE][0]; //v_x > 0 => move right, v_x < 0 => move left
  int v_y =  dataBackup[dataIndex][1] - dataBackup[(dataIndex - 1) % BACKUP_SIZE][1];

  // nach rechts und Player i = Ball geht weg von uns
  if (PLAYER == 0x02) {
    if (v_x > 0) {
      Serial.println("Ball goes away from Player 1");
      moveToCenter();
    }
    else {
      Serial.println("Ball goes to Player 1");
      int y = dataBackup[dataIndex][1];
      if (y > PADDLE_POS) {
        sendUpdate(1);
      }
      else if (y < PADDLE_POS) {
        sendUpdate(-1);
      }
    }
  }
  else{
    if (v_x > 0) {
      Serial.println("Ball goes to Player 2");
      int y = dataBackup[dataIndex][1];
      if (y > PADDLE_POS_OPPONENT) {
        sendUpdate(1);
      }
      else if (y < PADDLE_POS_OPPONENT) {
        sendUpdate(-1);
      }
    }
    else {
      Serial.println("Ball goes away from Player 2");
      moveToCenter();
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
  // Return Paddle to center (65)
  if (PADDLE_POS > 65) {
    sendUpdate(-1);
  }
  else if (PADDLE_POS < 65) {
    sendUpdate(1);
  }
}
 