# References


## Touchpad 4 Click

## OLED C CLICK

## SMART MIC CLICK

## THUMBSTICK CLICK
Datasheet zum A/D-Converter: http://ww1.microchip.com/downloads/en/devicedoc/21298e.pdf
![grafik](https://github.com/VectorInformatik/clickboard-examples/assets/136338757/0f322c96-dc77-45bc-84b7-314389b9e4e5)
![grafik](https://github.com/VectorInformatik/clickboard-examples/assets/136338757/a62aaf96-48f6-445b-9fe0-92962d6ceb10)

Thumbstick Layout:
![grafik](https://github.com/VectorInformatik/clickboard-examples/assets/136338757/7353b62a-c660-4fb1-bcbb-728389c9526b)

```c++
#include <SPI.h>

#define CS_PIN 10

int read_thumbstick(int channel) {
  int value = 0;
  
  // Select the MCP3204
  digitalWrite(CS_PIN, LOW);
  
  // Send the start bit, single-ended mode, and channel selection bits
  byte command = B11000000 | (channel << 3);
  SPI.transfer(command);
  
  // Read the 12-bit result (MSB first)
  value = SPI.transfer(0) & B00001111;
  value = (value << 8) + SPI.transfer(0);
  
  // Deselect the MCP3204
  digitalWrite(CS_PIN, HIGH);
  
  return value;
}

void get_user_input()
{
  int dir = 0;

  // Read value for both axes
  int value_0 = read_thumbstick(0);
  int value_1 = read_thumbstick(1);

  int vert = 0;
  int hor = 0;

  // Use thresholds to smooth input
  if (value_0 > 3500) vert = -1;
  else if (value_0 < 500) vert = 1;
  else vert = 0;

  if (value_1 > 3500) hor = 1;
  else if (value_1 < 500) hor = -1;
  else hor = 0;

  Serial.print("User input: vert: ");
  Serial.print(vert);
  Serial.print("  hor: ");
  Serial.print(hor);
  Serial.println();
}

void setup()
{
  Serial.begin(9600);

  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
}

void loop()
{
  get_user_input();
  delay(100);
}
```

## 8800 RETRO CLICK

```c++
#include <Wire.h>

byte characterOne[] = {0,0,0,1,1,0,0,0,
                       0,0,0,1,1,0,0,0,
                       0,0,0,1,1,0,0,0,
                       0,0,0,1,1,0,0,0,
                       0,0,0,1,1,0,0,0,
                       0,0,0,0,0,0,0,0,
                       0,0,0,1,1,0,0,0,
                       0,0,0,1,1,0,0,0};

 byte characterTwo[] = {0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0};

byte i2c_addr = 0x00;

// Write to 8x8 LED matrix
void show(byte data[]) {
  for (int x=0; x < 8; x++) {
    byte line = 0;
    for (int y=0;y < 8; y++) {
        line = line << 1;
        line = line | data[x*8+y];
    }
    Wire.beginTransmission(i2c_addr);
    Wire.write(x+1);  // select segment
    Wire.write(line); // select digit
    Wire.endTransmission();  
  }
}


void setup()
{
  Serial.begin(9600);

  // LED matrix
  Wire.begin(); // join i2c bus (address optional for master)

  // Wakeup from shutdown mode
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x0c); 
  Wire.write(0x81); 
  Wire.endTransmission();
  delay(1);

  // This device can change slave address dynamically.
  // see p.15 more detail

  // Change device address (p.15)
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x2d); // register
  Wire.write(0x01); // user-defined (0x03 if not connected any pins)
  Wire.endTransmission();  
  delay(1);

  i2c_addr = 0x03; // address changed from here

  // Intensity control (0x0A)
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x0a); // register
  //Wire.write(0x0f); // 15/16 (max)
  Wire.write(0x07); // 8/16 (median)
  //Wire.write(0x00); // 1/16 (min)
  Wire.endTransmission();  
  delay(1);

  // Scan-Limit Register (0x0B)
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x0b); 
  Wire.write(0x07); // display all digits 0:7
  Wire.endTransmission();  
  delay(1);

  // Non-decode mode (0x09) for 8x8 dot matrix
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x09); 
  Wire.write(0x00); // non-decode mode
  Wire.endTransmission();  
  delay(1);
}

void loop()
{
  show(characterOne);
  delay(400);
  show(characterTwo);
  delay(400);
}
```


## MPU 9DOF CLICK

## PROXIMITY 18 CLICK

## RADAR CLICK

## MikroSDK
https://github.com/MikroElektronika/mikrosdk_click_v2

## Adafruit Feather M4CAN Express 

![grafik](https://github.com/VectorInformatik/clickboard-examples/assets/136338757/4107adeb-8cf2-4803-934a-0be1f3d37a52)
