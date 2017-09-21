/*
 Project: ValBal EE Radio Receiver Code
 Created: 09/17/2017 12:33:25 PM
 Authors:  Aria Tedjarati
           Joan Creus-Costa

 Description:
 GFSK Radio Receiver

 Credits:
 Femtoloon.

 Status:
 09/17/2017 Test code for the range test is written.
 09/20/2017 Interface with Raspberry Pi

 TODO as of 09/20/2017:
 Write full flight code for entire system.
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RH_RF24.h>


const int MESSAGE_LENGTH = 35;
#define NPAR 25

#include "ecc.h"

// Pins
#define   GFSK_SDN           19
#define   GFSK_IRQ           16
#define   GFSK_GATE          22
#define   GFSK_GPIO_0        21
#define   GFSK_GPIO_1        20
#define   GFSK_GPIO_2        17
#define   GFSK_GPIO_3        18
#define   GFSK_CS            15
#define   SD_CS              23
#define   LED                6


RH_RF24                  rf24(GFSK_CS, GFSK_IRQ, GFSK_SDN);

typedef enum {
        GFSK_CONFIG,
        RECEIVE_GFSK
} states;

states        currentState = GFSK_CONFIG;
states        previousState;
states        nextState;


/* Generated with a fair dice. */
uint8_t RADIO_START_SEQUENCE[] = {204, 105, 119, 82};
uint8_t RADIO_END_SEQUENCE[] = {162, 98, 128, 161};



void GFSKOff(){
  digitalWrite(GFSK_GATE, HIGH);
}

void GFSKOn(){
  digitalWrite(GFSK_GATE, LOW);
}



int main() {
  pinMode(GFSK_GATE, OUTPUT);
  pinMode(GFSK_SDN,OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  GFSKOff();
  SPI.setSCK(13);       // SCK on pin 13
  SPI.setMOSI(11);      // MOSI on pin 11
  SPI.setMISO(12);      // MOSI on pin 11
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);  // Setting clock speed to 8mhz, as 10 is the max for the rfm22
  SPI.begin();
  Serial.begin(115200);
  Serial.println("ValBal 8.1 Radio Receiver Board, Summer 2017");
  analogReadResolution(12);
  analogReference(INTERNAL);
  Serial2.begin(57600);
  initialize_ecc();
  //setupSDCard();
  delay(3000);

  while (true) {
    switch (currentState) {
    case GFSK_CONFIG:
      GFSKOn();
      rf24.init(MESSAGE_LENGTH+NPAR);
      uint8_t buf[8];
      if (!rf24.command(RH_RF24_CMD_PART_INFO, 0, 0, buf, sizeof(buf))) {
        Serial.println("SPI ERROR");
      } else {
        Serial.println("SPI OK");
      }
      if (!rf24.setFrequency(433.5)) {
        Serial.println("setFrequency failed");
      } else {
        Serial.println("Frequency set to 433.5 MHz");
      }
      rf24.setModemConfig(rf24.GFSK_Rb0_5Fd1);   // GFSK 500 bps
      rf24.setTxPower(0x4f);
      nextState = RECEIVE_GFSK;
      break;
    case RECEIVE_GFSK:
      uint8_t data[256] = {0};
      uint8_t leng = MESSAGE_LENGTH + NPAR;
      boolean tru = rf24.recv(data, &leng);
      if (tru) {
        uint8_t lastRssi = (uint8_t)rf24.lastRssi();
        Serial.print("Got stuff at RSSI ");
        Serial.println(lastRssi);
        //for (int kk=0; kk<60; kk++){ if (data[kk] < 10) Serial.print(" "); if (data[kk] < 100) Serial.print(" "); Serial.print(data[kk], DEC); Serial.print(" ");}
        for (int kk=0; kk<60; kk++){ Serial.print((char)data[kk]); }

        Serial.println();
        unsigned char copied[256];
        memcpy(copied, data, 256);
        decode_data(copied, MESSAGE_LENGTH+NPAR);
        if (check_syndrome () != 0) {
          Serial.println("There were errors");

          correct_errors_erasures(copied, MESSAGE_LENGTH+NPAR, 0, NULL);
        } else {
          Serial.println("No errors");
        }

        Serial2.write(RADIO_START_SEQUENCE, 4);
        int lastRssiNum = lastRssi;
        Serial2.write(lastRssiNum);
        Serial2.write(",");
        Serial2.write(MESSAGE_LENGTH);
        Serial2.write(",");
        Serial2.write(copied, MESSAGE_LENGTH);
        Serial2.write(data, MESSAGE_LENGTH + NPAR);
        Serial2.write(RADIO_END_SEQUENCE, 4);
      }

      nextState = RECEIVE_GFSK;
      break;
    }
    previousState = currentState;
    currentState = nextState;
  }
}
