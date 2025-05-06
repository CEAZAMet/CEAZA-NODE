//Ultra-Low-Power Ultrasonic wind meter (ULP Standard) vRS485 (NMEA0183) / MODBUS
//https://calypsoinstruments.com/shop/product/ultra-low-power-ultrasonic-wind-meter-ulp-standard-6?search=Wind#attr=76,56,65
//manual https://calypsoinstruments.com/web/content/135200?access_token=abd7e7b1-1fc6-42f8-ac47-96356973ff0d&unique=a1aa24ff7632d746a358318137570f8ab5f8d4c3&download=true

#define VSPI_EN_PIN 13 //lora, flash, RS485, sens_t internal
#define VSPI_EN_PIN_ON LOW //electronica inversa


// Define modbus sensor constants
const int CALYPSO_START_ADDR = 0x00C9; // Starting address of holding registers to read
const int CALYPSO_NUM_REGS = 13;       // Number of holding registers to read
uint16_t CALYPSO_windSensorData[12]; // {WS_3s, WD_3s, WS_2min, WD_2min, WS_10m, WD_10min, WGS, WGD, WS_5min, WD_5min, WGS_5min,WGS_5min} all data is multiplied x10
const String varName[12] = {"WS_3s", "WD_3s", "WS_2min", "WD_2min", "WS_10m", "WD_10min", "WGS", "WGD", "WS_5min", "WD_5min", "WGS_5min", "WGS_5min"};

unsigned int calculateCRC16(unsigned char *buf, int len)
{
  unsigned int crc = 0xFFFF; // Initialize CRC16 to 0xFFFF

  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos]; // XOR byte into CRC

    for (int i = 8; i != 0; i--) { // Loop over each bit
      if ((crc & 0x0001) != 0) {   // If the LSB is set
        crc >>= 1;                // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else {                       // If the LSB is not set
        crc >>= 1;                // Shift right
      }
    }
  }

  return crc; // Return calculated CRC16
}

struct calypso_data
{
  float ws_3s;
  float wd_3s;
  float ws_2m;
  float wd_2m;
  float ws_10m;
  float wd_10m;
  float wgs;
  float wgd;
  float ws_5m;
  float wd_5m;
  float wgs_5m;
  float wgd_5m;
};


void generateModbusRTUQuery03(unsigned char* query, unsigned char slaveID, unsigned int startAddr, unsigned int numRegs)
{
  query[0] = slaveID;           // Slave ID
  query[1] = 0x03;              // Function code 03 (Read Holding Registers)
  query[2] = (startAddr >> 8);  // Starting address MSB
  query[3] = (startAddr & 0xFF);// Starting address LSB
  query[4] = (numRegs >> 8);    // Number of registers to read MSB
  query[5] = (numRegs & 0xFF);  // Number of registers to read LSB
  
  // Calculate CRC16 checksum and append it to query
  unsigned int crc = calculateCRC16(query, 6);
  query[6] = (crc & 0xFF);      // CRC LSB
  query[7] = ((crc >> 8) & 0xFF);// CRC MSB
}

//***********************************************************************************
bool Calypso_GetData( calypso_data &cd_result, unsigned char slave_id,unsigned long timeOut=100, int modbus_baudrate=9600){
  Serial2.begin(modbus_baudrate);
  digitalWrite(VSPI_EN_PIN,VSPI_EN_PIN_ON);delay(50);
  delay(100);
  

  

  // Define query buffer
  unsigned char query[8];

  // Generate Modbus RTU query
  generateModbusRTUQuery03(query, slave_id, CALYPSO_START_ADDR, CALYPSO_NUM_REGS);

  const int modbusInBufferLength = 64;
  byte modbusInBuffer[modbusInBufferLength];
 memset(&modbusInBuffer,0, modbusInBufferLength);
  

  // Clear UART in buffer
  delay(20);
  while(Serial2.available()){    byte inByte = Serial2.read();  }

  // Send modbus query
  Serial2.write(query,8);
  Serial2.flush();

 char query_hex[64];
 sprintf(query_hex, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", query[0], query[1], query[2],query[3],query[4],query[5],query[6],query[7]);
 Serial.print(" RS485->'");
 Serial.print(query_hex);
 Serial.print("'");
  // Wait for any answer
  int qtyInByte = 0;
  unsigned long startPoint= millis();
  
 Serial.print("| RS485<-'");
  while((millis() - startPoint < timeOut)){
    if(Serial2.available()){
      byte inByte = Serial2.read();
      modbusInBuffer[qtyInByte] = inByte;
      qtyInByte++;
      
      char query_hex[5];
      sprintf(query_hex, "0x%02x",inByte);

      if (qtyInByte>1) Serial.print(" ");
      Serial.print(query_hex);
     }
  }
   Serial.println("'");
//  digitalWrite(VSPI_EN_PIN,!VSPI_EN_PIN_ON);//apagar linea de voltaje


  memset(&CALYPSO_windSensorData,0, 12);
  int dataCount = 0;
  for(int i = 3; i < 28 ; i = i+2)
  {
    if(i != 19){      CALYPSO_windSensorData[dataCount] = (uint16_t(modbusInBuffer[i]) << 8) + uint16_t(modbusInBuffer[i+1]);      dataCount++;    }
  }
    if (qtyInByte>0)
    {
    cd_result.ws_3s = float(CALYPSO_windSensorData[0])/10;
    cd_result.wd_3s = float(CALYPSO_windSensorData[1])/10;
    cd_result.ws_10m = float(CALYPSO_windSensorData[4])/10;
    cd_result.wd_10m = float(CALYPSO_windSensorData[5])/10;
    }
    else
    {
     cd_result.ws_3s =-999;
     cd_result.wd_3s =-999;
     cd_result.ws_10m =-999;
     cd_result.wd_10m =-999;
    }
    
  if (qtyInByte==0) return false;
  return true;
}//fin Calypso_GetData


/*
 Ejemplo de uso:
 #define VSPI_EN_PIN 13 //lora, flash, RS485, sens_t internal
#define VSPI_EN_PIN_ON LOW //electronica inversa
#define STATUS_LED_PIN  0
#include "IOTnode1.2_calypso.h"
 
#define HAS_CALYPSO_ULP 1
//***********************************************************************************
void setup() {
  pinMode(VSPI_EN_PIN,OUTPUT);digitalWrite(VSPI_EN_PIN,!VSPI_EN_PIN_ON);
  pinMode(STATUS_LED_PIN,OUTPUT);digitalWrite(STATUS_LED_PIN,HIGH);
  
  Serial.begin(115200);
  delay(100);
  Serial.println("\nIOTnode1.2_calypso_v20230525\n");

}

//***********************************************************************************
void loop() {
  if (HAS_CALYPSO_ULP)
  {
   calypso_data ws_data;
   Calypso_GetData(ws_data, 2,100,9600);//Calypso_GetData(result, id_sensor,timeout_ms,baudrate_485)
   Serial.print("ws_3s,");Serial.print(ws_data.ws_3s);
   Serial.print(",wd_3s,");Serial.print(ws_data.wd_3s);
   Serial.print(",ws_10m,");Serial.print(ws_data.ws_10m);
   Serial.print(",wd_10m,");Serial.print(ws_data.wd_10m);
   Serial.println("");
  }
  
  delay(2800);
}
*/
