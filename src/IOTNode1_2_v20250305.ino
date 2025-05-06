//seleccionar ESP32-WROOM-DA module (flash mode QIO/ flash size 4MB/ partition scheme default 4MB with spiff)
// v2024.03.20 carga de firmware en nodo candidato a produccion

// v2023.03.09 se agregan las consultas 485
// v2023.03.17 se apaga con pullup el gps
// v2023.03.20 fix calculo linea flash y fix lora (no enviaba, estaba async) [pero tenia error en GPS, a veces captaba la hora mal
// v2023.03.24 se agrega el CRC al final de la linea de datos
//             se agregan los ultimos 2 bytes de la MAC como ID, las lineas que produce son como: 'id,I9B48,dt,1970-01-01T00:01:25Z,vin,9.00,at,,sh,,sw,,swt,,C,28094'
// v2023.03.31 en el primer ciclo y cada 100 toma el GPS para ajustar el reloj, luego de eso ademas ahora guarda y envia una linea de datos con la posicion tipo: 'id,I9B80,dt,2023-03-31T13:13:19Z,lat,-29.914974,lon,-71.242218,alt,123.000000,C,36729'
// v2023.04.04 se agrega la posibilidad de agregar una 2da pesa (definiendo las ID con valores >0)
//             ahora las primeras 10 lecturas tienen un delay de 30seg
//             se agrega la opcion de incluir el modem SWARM CEAZA V1.0, al ciclo 0 le consulta si esta y si esta le empieza a agregar los mensajes en la cola (1 por hora)
// v2023.04.10 se hace un fix que potencialmente deja el nodo sin salir del modo wifi
// v2023.04.26 busca 2 pesas (142 y 142) por defecto tirando comandos AT. Se implementa funcion rs485_getLines, para recibir respuestas de comandos con varias lineas (como el get_config de las pesas).
// v2023.06.15 busca 4 pesas (142, 142, 143 y 144) por defecto tirando comandos AT.
// v2023.08.08 el GPS no actualiza el reloj a menos que hallan al menos 5 satelites en vista
// v2023.09.29 se sincroniza para que despierte al minuto xx:x0:00 menos 30 segundos y envie el paquete lora en el segundo X (00:xx)
// v2023.10.05 dado el drift de tiempo del reloj interno en todos los ciclos se prende el GPS y se obtiene de ahi la lectura
// v2023.10.06 en todos los ciclos se usa la hora del GPS, en los ciclos comunes la obtiene del RTC del GPS y cada 2h fuerza a que venga del GPS/SAT y 1 vez al dia actualiza la posicion/tiempo con al menos 10 satelites
//             en esta version se actualizo la libreria del GPS tambien GPS_co, para agregar una hora del RTC
// v2023.11.03 se agregan IDs de nodos nuevos, ademas la libreria del GPS incluye la busqueda del .000 en la marca de tiempo para asegurarse de que este correcta la hora
// v2023.11.28 se agrega el sleep del modem swarm si baja de voltaje
// v204.01.16 variable names changed to english, envio lora instantaneo y GPS solo se prende 1 vez al dia (1 vez c/144 ciclos), ademas no despierta en el segundo 0 (sino interferiria con lora instantaneo)
// v2024.02.05 se incorpora el hold del SW antes del deep sleep, si no se hacia el comportamiento del switched es errativo y puede no apagar los dispositivos generando un consumo que puede terminar con la bateria
// v2024.02.12 se prueba un FIX en la escritura flash (leer la linea antes de escribirla parece hacer que no se corrompan los datos)
// v2024.03.13 se agrega el modo gateway en los define, si esta en ese modo no se dormira y quedara esperando datos Lora y guardara todo en la flash, ademas procesara comandos 485
// v2024.03.15 new: en modo gateway, en envia orden de encendido al RUT955; fixed: los comandos recibidos por 485 se parsean y procesan correctamente (anteriormente, recibia newline ('\n') y carriage return ('\r'))
// v2024.04.21 added some serial output specifiying if its a node or a gateway (and fix from the cicle 1 line, just was in the previous fw)
// v2024.04.22 in the gateway mode The SW12 (router) the off and on each hour also beep every time a lora paquet arrives
// v2024.05.14 in the gateway mode the GPS will be on sometimes and the received packets will have a timestamp 
// v2024.06.04 fix in node mode, the clock was not beeing updated by gps 
// v2024.07.29 the GPS packet also include firmware version
// v2024.09.05 fix in the CRC calculation, last char in data array was replaced by \0 char so CRC was not correct in previous firmwares
// v2024.09.23 * added the opcion in defines for reading calypso ULP win sensor 
//             * deleted the swarm routines
// v2024.09.24 * sensors with no data will not be included in data lines, id,I9B80,dt,2024-09-24T12:47:00Z,vin,12.45,at,,sh,,ws,0.00,wd,0.00,C,43955 (this kind of lines wont be generated, instead will be id,I9B80,dt,2024-09-24T12:47:00Z,vin,12.45,ws,0.00,wd,0.00,C,43955)
//             * gateway will reset once each 24h (this is not a watchdog but could provide help with overflows and fragmentation halts)
// v2024.09.25 * Now if the time if obtained from the RTC on the 485 conversor it will use it
//             * If the time is aquired by the gps it will update the time in the 485 conversor
//v2024.09.30 removed the "not ok command" response from de RS485 line in client mode
//v2024.10.04 changes the rs485_sendCmd function, instead of sending a println just include a \n at the end of the command (println produce a \r\n ([13][10]))
//            the query order was changed, now the scales are queried before the conversor (the scales fails to answer if are after [to-do the fix])
//v2024.10.16 Added serial commands in gateway mode ( <1,getLinesFromTo,0,100,I,2024>)
//v2024.12.02 Serial and RS485 behaves the same now, so it will process the same comands
//            Now one can ask lines from the last using the -N  <1,getLinesFromTo,-10,99999,I,20>
//v2024.12.13 Added DELAY_SW_VIN_ON_ms and TIME_TO_SLEEP_SEC as vars so can be chagned by NODEID
//v2024.12.16 Changed function get_all_data, now it will pack 10 lines before sending so it will be faster
//v2025.01.03 Calypso sensor now is connected in SW12 line, so 10 seconds are added to readings to stabilize, also the serial buffer is discarded in this period (the sensors transmit msgs when energized)
//v2025.02.03 camera now has ID 150 and 
//v2025.03.04 now can use a 1200 or 1300 board, when version = 13 then external clock is used, also pin changed is configured in fw
//v2025.03.05 fix HALLSENS pin (in ver 1.3) cause the webserver was activaded, also now serial2 is initialized with pin numbers, without it, it fails to comunicate
//             gateway also saves rssi level from lora
// TODOs
//  Recepcion de paquetes de comando luego del envio (de forma que se puedan cambiar parametros de forma remota, como el segundo de la ventana lora)
//  en processCmd se debe agregar el comando para enviar series de datos acotados (inicialmente por posicion de puntero, IDEALMENTE por fechas) ~ Carlo

// known issues
//  si el nodo lleva un tiempo encendido, al conectar con telnet desde el cliente se abre la conexion pero no procesa comandos (queda ocioso, no hay respuesta)
// 
//Memoria: 52560 lineas/año si mide cada 10m
//Consumo: 0.5mA en sleep / 50-100 mA en activo (dependiendo que tenga instalado)
//rssi: si esta a 1m (-50, -60)

#define ISGATEWAY 1
#define BUZZER_ON 0
#define BOARD_VERSION 12 //Puede ser 1.2 y 1.3, la diferencia son los pines del buzzer y el hall

#if  (BOARD_VERSION==12)
 #define BUZZER_PIN 32
 #define HALL_SENS_PIN 33
 float VIN_factor = 1.00;
#endif


#if  (BOARD_VERSION==13)
 #define BUZZER_PIN 25
 #define HALL_SENS_PIN 26
 float VIN_factor = 0.55;
#endif

#define HAS_CALYPSO_ULP 0 //if it has a Ultra-Low-Power Ultrasonic wind meter (ULP Standard) vRS485 (NMEA0183) / MODBUS will readit
#define ID_CALYPSO 1      //by default calypso sensor (after configuration when bought) as ID 1, MODBUS 9600 and 8n1 serial configuration
// consumos:
#define ID_SNOWSCALE1 141
#define ID_SNOWSCALE2 0
#define ID_SNOWSCALE3 0
#define ID_SNOWSCALE4 0
#define ID_CAMARA 150

unsigned long DELAY_SW_VIN_ON_ms = 500;

unsigned long TIME_TO_SLEEP_SEC= 600;        /* Time ESP32 will go to sleep (in seconds) 60=1 minuto 600 = 10 minutos 3600 = 1h*/
//#define TIME_TO_SLEEP_SEC 3600        /* Time ESP32 will go to sleep (in seconds) 60=1 minuto 600 = 10 minutos 3600 = 1h*/
unsigned long LORA_SEND_SECOND = 10; /* Time ESP32 will go to sleep (in seconds) */

#include "CRC16.h" //se pueden chequear en https://crccalc.com/ CRC-16/XMODEM
// #define LOG_LOCAL_LEVEL ESP_LOG_INFO //gedit /home/cristian/.arduino15/packages/esp32/hardware/esp32/2.0.4/tools/sdk/esp32/include/log/include/esp_log.h
// #define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#include <GPS_co1.h>
#include <ESP32Time.h>
#include <SPI.h>
//#include <W25N.h>
 #include "WinbondW25N.h"
 #include "IOTnode1.2_calypso.h"

//float VIN_factor = 1.00;

ESP32Time rtc(0); // offset in seconds GMT0



#define BUZZER_PIN_ON HIGH


#define HALL_SENS_PIN_ON false
RTC_DATA_ATTR bool hallSensorFlag = false;
RTC_DATA_ATTR float gps_lat = 0.0;
RTC_DATA_ATTR float gps_lon = 0.0;
RTC_DATA_ATTR float gps_alt = 0.0;


void IRAM_ATTR ISR() { hallSensorFlag = true; }

#define STATUS_LED_PIN 0
#define SW_VIN_PIN 5
#define SW_VIN_PIN_ON HIGH
#define V_MONITOR_PIN 34
#define VmonitorDiv 151.5
#define VmonitorOffSet 1.4

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */

//---------- inicio GPS ----------
#define GPS_POW_PIN 2
#define GPS_POW_PIN_ON LOW
#define GPS_TX_PIN 14
#define GPS_RX_PIN 39
#define SerialGPS Serial1
//---------- fin GPS ----------

//---------- inicio 485 -------------------
#define TXD2_PIN 17
#define RXD2_PIN 16
#define SerialRS485 Serial2
uint8_t qtyInstrumentsFound = 0;
uint8_t instrumentAddress[12];

unsigned long start_millis = millis();
unsigned long end_millis = millis();
unsigned long cycle_millis = 10000;

String firmware_ver = "20250305";
const char* credits = "\nIOTnode1.2_v2025.03.05 by C.O, A.G, C.G CEAZA, 2025\n";
//"<200,get_hss>"->"160.1,255\n"|"<200,get_hs>"->"160.1\n" | <200,get_ta>->"20.5\n"

void(* resetFunc) (void) = 0; // create a standard reset function

void rs485_emptybuffer(unsigned long timeout = 250)
{
  unsigned long timer = millis();
  while (millis() - timer < timeout)
  {
   if (SerialRS485.available())
   {
     byte inChar = SerialRS485.read();
     if (inChar>=32 && inChar<=126)     Serial.print(char(inChar));
     else {Serial.print("[");Serial.print(int(inChar));Serial.print("]");}
   }
  } 
}



String rs485_getLine(unsigned long timeout = 1000)
{
  String msjIn = "";
  unsigned long timer = millis();
  int cant_bytes_leidos=0;
  bool fin_de_linea=false;
  while (millis() - timer < timeout&&!fin_de_linea)
  {
    if (SerialRS485.available())
    {
     byte inChar = SerialRS485.read();
     
     if (inChar>=32 && inChar<=126)  
     {
       msjIn +=String(char(inChar));
       cant_bytes_leidos++;//solo contar bytes leidos los no binarios
     }//do not include non text bytes 
     //else   msjIn =msjIn+"["+inChar+"]";//do not include non text bytes
     if (inChar == 10  && cant_bytes_leidos>1){fin_de_linea=true;}
    }
  }

  rs485_processCmd(msjIn);

  Serial.print("| RS485<-'" + msjIn + "' ["); Serial.print(millis() - timer); Serial.println("ms]");
  rs485_emptybuffer(250);
  

  return msjIn;
}


String rs485_getLines(unsigned long timeout = 1000)
{
  String msjIn = "";
  unsigned long timer = millis();
  while (millis() - timer < timeout)
  {
    if (SerialRS485.available())
    {
      char inChar = SerialRS485.read();
      if (inChar != '\n')
      {
        msjIn += inChar;
      }
      else
      {
        msjIn += "[13]";
      }
    }
  }
  rs485_processCmd(msjIn);
  Serial.println("| RS485<-'" + msjIn + "'");
  return msjIn;
}

void rs485_sendCmd(String comando)
{
  int char_descartado = VaciarBufferSerial();
  if (char_descartado)
  {
    Serial.print(" RS485 Warning: se descartaron ");
    Serial.print(char_descartado);
    Serial.print(" bytes desde el serial.");
  }
  SerialRS485.print(comando);
  SerialRS485.print('\n');
  
  SerialRS485.flush();
  Serial.print(" RS485->'");
  Serial.print(comando);
  
  Serial.flush();
}

//---------- fin 485 -------------------

//-------------- inicio 485 como cliente -------------------

bool Serial_processCmd(Stream &serial_port, String comando)//comandos esperados <1,getLinesFromTo,0,100,I,2024>|<1,flash_format>
{
  if (!comando.length() > 0) return false;
  if (comando.indexOf('<')==-1 || comando.indexOf('>')==-1) return false;
  int ind_inicio=comando.indexOf('<');
  int ind_fin=comando.indexOf('>');

  int ind_comma1 = comando.indexOf(',');  //finds location of first ,
  String param1 = comando.substring(ind_inicio+1, ind_comma1);   //captures first data String
  
  int ind_comma2 = comando.indexOf(',', ind_comma1+1 );   //finds location of second ,
  String param2 = comando.substring(ind_comma1+1, ind_comma2);   //captures second data String

  int ind_comma3 = comando.indexOf(',', ind_comma2+1 );   //finds location of second ,
  String param3 = comando.substring(ind_comma2+1, ind_comma3);   //captures second data String

  int ind_comma4 = comando.indexOf(',', ind_comma3+1 );   //finds location of second ,
  String param4 = comando.substring(ind_comma3+1, ind_comma4);   //captures second data String

  int ind_comma5 = comando.indexOf(',', ind_comma4+1 );   //finds location of second ,
  String param5 = comando.substring(ind_comma4+1, ind_comma5);   //captures second data String

  String param6 = comando.substring(ind_comma5+1, ind_fin);   //captures second data String      
 
  if (param1!="1") return false;//no procesar comando si el parametro 1 no es 1 (id del gateway)

  if (param2=="getLinesFromTo" &&  param4.toInt()>=0 && param4.toInt()>= param3.toInt())
  {
    long start_line=param3.toInt();
    long end_line=param4.toInt();
    long last_line_in_memory = flash_getnextline() - 1;
    if (start_line>last_line_in_memory) start_line = last_line_in_memory;
    if (end_line>last_line_in_memory) end_line = last_line_in_memory;
    if (start_line<0) start_line=end_line+start_line;
    if (start_line<0) start_line=0;
    
    char buf[256];
    for (unsigned long q = start_line; q <= end_line; q++)
      {
        flash_get256(q, &buf[0]);
        String linea=String(buf);
        if (linea.indexOf(param5)!=-1&&linea.indexOf(param6)!=-1) serial_port.println(buf);
      }
  //serial_port.println("start_line:'"+String(start_line)+"'");
  //serial_port.println("end_line:'"+String(end_line)+"'");
  //serial_port.println("last_line_in_memory:'"+String(last_line_in_memory)+"'");

  }
  if (param2=="flash_format")
  {
   flash_format();
   serial_port.print("Flash Erased");serial_port.print("\n");
  }
  /*serial_port.println("Param 1:'"+param1+"'");
  serial_port.println("Param 2:'"+param2+"'");
  serial_port.println("Param 3:'"+param3+"'");
  serial_port.println("Param 4:'"+param4+"'");
  serial_port.println("Param 5:'"+param5+"'");
  serial_port.println("Param 6:'"+param6+"'");*/


  return true;
}


void rs485_processCmd(String comando)
{
  if (!ISGATEWAY)  return;//si no es gateway no procesa comandos
  if (comando.length() > 0)
  {
    if (comando == "get_data")
    {
      SerialRS485.println("get_data ACK");
      SerialRS485.println("get_data BEGIN");
      unsigned long ultima_linea = flash_getnextline() - 1;
      char buf[256];
      unsigned long start_line;
      if (ultima_linea > 48)
      {
        start_line = ultima_linea - 48UL;
      }
      else
      {
        start_line = 0;
      }
      for (unsigned long q = start_line; q < ultima_linea - 1; q++)
      {
        flash_get256(q, &buf[0]);
      }
      SerialRS485.println(buf);
      SerialRS485.println("get_data END");
    }
    else if (comando == "get_all_data")
    {
      SerialRS485.println("Serial: get_all_data ACK");
      SerialRS485.println("Serial: get_all_data BEGIN");
      char buf[255];
      unsigned long ultima_linea = flash_getnextline();

      for (unsigned long q = 0; q < ultima_linea - 1; q++)
      {
        flash_get256(q, &buf[0]);
        if (q % 100 == 0)
        {
          SerialRS485.print("flash[");
          SerialRS485.print(q);
          SerialRS485.print("] = ");
          SerialRS485.println(buf);
        }
        SerialRS485.println(buf);
      }
      SerialRS485.println("Serial: get_all_data END");
    }
    else if (comando == "get_all_data_raw")
    {
      SerialRS485.println("Serial: get_all_data_raw ACK");
      SerialRS485.println("Serial: get_all_data_raw BEGIN");
      char buf[255];
      // unsigned long ultima_linea = flash_getnextline() - 1;

      for (unsigned long q = 0; q < 60000; q++)
      {
        flash_get256(q, &buf[0]);
        if (q % 100 == 0)
        {
          SerialRS485.print("flash[");
          SerialRS485.print(q);
          SerialRS485.print("] = ");
          SerialRS485.println(buf);
        }
        SerialRS485.println(buf);
      }
      SerialRS485.println("Serial: get_all_data_raw END");
    }
    else if (comando == "get_last_240")
    {
      SerialRS485.println("Serial: get_last_240 ACK");
      SerialRS485.println("Serial: get_last_240 BEGIN");
      char buf[255];
      unsigned long ultima_linea = flash_getnextline() - 1UL;
      unsigned long start_line = 0;
      if (ultima_linea > 240)
        start_line = ultima_linea - 240UL;

      for (unsigned long q = start_line; q < ultima_linea; q++)
      {
        flash_get256(q, &buf[0]);
        if (q % 100 == 0)
        {
          SerialRS485.print("flash[");
          SerialRS485.print(q);
          SerialRS485.print("] = ");
          SerialRS485.println(buf);
        }
        SerialRS485.println(buf);
      }
      SerialRS485.println("Serial: get_last_240 END");
    }
    // else if (comando == "get_lines_range $1 $2") // pedir inicio y fin, enviar set de lineas segun inicio y fin
    // {
    //   SerialRS485.println("Serial: get_last_240 ACK");
    //   SerialRS485.println("Serial: get_last_240 BEGIN");
    //   char buf[255];
    //   unsigned long ultima_linea = flash_getnextline() - 1UL;
    //   unsigned long start_line = 0;
    //   if (ultima_linea > 240)
    //     start_line = ultima_linea - 240UL;

    //   for (unsigned long q = start_line; q < ultima_linea; q++)
    //   {
    //     flash_get256(q, &buf[0]);
    //     if (q % 100 == 0)
    //     {
    //       SerialRS485.print("flash[");
    //       SerialRS485.print(q);
    //       SerialRS485.print("] = ");
    //       SerialRS485.println(buf);
    //     }
    //     SerialRS485.println(buf);
    //   }
    //   SerialRS485.println("Serial: get_last_240 END");
    // }
    else if (comando == "help")
    {
      SerialRS485.println("Serial: lista de comandos reconocidos:");
      SerialRS485.println("Serial: get_data -- envia la ultima linea");
      SerialRS485.println("Serial: get_all_data -- envia todas las lineas");
      // SerialRS485.println("Serial: get_all_data_raw -- envia toda la informacion en la memoria flash");
      SerialRS485.println("Serial: get_last_240 -- envia las ultimas 240 lineas");
      SerialRS485.println("Serial: help -- muestra este mensaje");
    }
    /*else
    {
      SerialRS485.println("comando " + comando + " no reconocido");
      SerialRS485.println("Serial: get_data -- envia la ultima linea");
      SerialRS485.println("Serial: get_all_data -- envia todas las lineas");
      // SerialRS485.println("Serial: get_all_data_raw -- envia toda la informacion en la memoria flash");
      SerialRS485.println("Serial: get_last_240 -- envia las ultimas 240 lineas");
      SerialRS485.println("Serial: help -- muestra este mensaje");
    }*/
  }
}

//--------------- fin 485 como cliente ---------------------

int VaciarBufferSerial()
{
  int char_descartados = 0;
  while (Serial.available())
  {
    Serial.read();
    char_descartados++;
  }
  return char_descartados;
}

bool  EstadoPuerto(int puerto)
{
  return (0 != (*portOutputRegister(digitalPinToPort(puerto)) & digitalPinToBitMask(puerto)));
}

RTC_DATA_ATTR unsigned int counter = 0;
RTC_DATA_ATTR String NODEID = "CN000";

//--------- inicio flash -------------
#define FLASH_SS_PIN 15
#define LORA_SS_PIN 12
#define V_VSPI_CTRL_PIN 13
#define V_VSPI_CTRL_PIN_ON LOW
unsigned long FLASH_MAX_LINES = 400000; // se producen 60.000 lineas por año guardando cada 10minutos
W25N flash = W25N();
RTC_DATA_ATTR uint32_t flash_next_line = 0;

bool flash_init()
{

  pinMode(LORA_SS_PIN, OUTPUT);
  digitalWrite(LORA_SS_PIN, HIGH);
  pinMode(V_VSPI_CTRL_PIN, OUTPUT);
  digitalWrite(V_VSPI_CTRL_PIN, LOW);
  delay(50);
  if (flash.begin(FLASH_SS_PIN) == 0)
  {
    Serial.print("Flash init OK, ");

    // Muestra información de la memoria
    char jedec[5] = {W25N_JEDEC_ID, 0x00, 0x00, 0x00, 0x00};
    flash.sendData(jedec, sizeof(jedec));
    Serial.print(" MAN_ID:");
    Serial.print(jedec[2], HEX);
    Serial.print(" DEV_ID:");
    Serial.print(jedec[3], HEX);
    Serial.println(jedec[4], HEX);

    // NODEID=String(jedec[2], HEX)+String(jedec[3], HEX)+String(jedec[4], HEX);
  }
  else
  {
    Serial.println("Flash init Failed. HALT!");
    while (1)      ;
  }
  // SPI.setFrequency(20000000);
  return true;
}
unsigned long flash_getnextline()
{
  if (EstadoPuerto(V_VSPI_CTRL_PIN) == !V_VSPI_CTRL_PIN_ON)
  {
    digitalWrite(V_VSPI_CTRL_PIN, V_VSPI_CTRL_PIN_ON);
    delay(50);
  }
  char buf[256];
  memset((void *)&buf[0], 0, sizeof(buf));

  char buf1[256];
  char buf2[256];

  unsigned long indice1 = 0;
  unsigned long indice2 = 1;
  unsigned long last_known_line = 0;

  for (unsigned int k = 0; k < FLASH_MAX_LINES; k += 1000)
  {
    flash_get256(k, &buf1[0]);
    flash_get256(k + 1000, &buf2[0]);
    if (buf1[0] != 0xff && buf2[0] == 0xff)
    {
      last_known_line = k;
      break;
    }
  }

  for (unsigned int k = last_known_line; k < FLASH_MAX_LINES; k += 100)
  {
    flash_get256(k, &buf1[0]);
    flash_get256(k + 100, &buf2[0]);
    if (buf1[0] != 0xff && buf2[0] == 0xff)
    {
      last_known_line = k;
      break;
    }
  }
  for (int k = last_known_line; k < FLASH_MAX_LINES; k++)
  {
    flash_get256(k, &buf1[0]);
    if (buf1[0] == 0xff)
    {
      return k;
    }
  }
  return 0;
} // fin flash_getnextpage

void flash_format()
{
  Serial.print("Formating flash...");
  if (EstadoPuerto(V_VSPI_CTRL_PIN) == !V_VSPI_CTRL_PIN_ON)
  {
    digitalWrite(V_VSPI_CTRL_PIN, V_VSPI_CTRL_PIN_ON);
    delay(50);
  }
  flash.bulkErase();
  Serial.println("formated ok");
}
void flash_get256(uint32_t linea, char *buffer256)
{
  if (EstadoPuerto(V_VSPI_CTRL_PIN) == !V_VSPI_CTRL_PIN_ON)
  {
    digitalWrite(V_VSPI_CTRL_PIN, V_VSPI_CTRL_PIN_ON);
    delay(50);
  }
  if (linea < 0)
    linea = 0;
  uint32_t pagina = uint32_t(linea / 4);
  uint16_t offset = uint16_t(linea % 4) * 256;
  flash.pageDataRead(pagina);
  flash.read(offset, buffer256, 256);
}

void flash_write_srt(unsigned long linea, String linea_srt)
{
  if (EstadoPuerto(V_VSPI_CTRL_PIN) == !V_VSPI_CTRL_PIN_ON)
  {
    digitalWrite(V_VSPI_CTRL_PIN, V_VSPI_CTRL_PIN_ON);
    delay(100);
  }
  char buf[256];
  memset((void *)&buf[0], 0, sizeof(buf));
  linea_srt.toCharArray(buf, linea_srt.length() + 1);
  flash_write256(linea, &buf[0]);
}

void flash_write256(unsigned long linea, char *buffer256) // solo pide el ID de dispositivo para comprobar que el chip esta
{
  if (EstadoPuerto(V_VSPI_CTRL_PIN) == !V_VSPI_CTRL_PIN_ON)
    digitalWrite(V_VSPI_CTRL_PIN, V_VSPI_CTRL_PIN_ON);
  delay(100);
  char auxmem[256];
  uint32_t pagina = uint32_t(linea / 4);
  uint16_t offset = uint16_t(linea % 4) * 256;
  flash_get256(linea, &auxmem[0]); // al parecer leerla antes de escribirla evita la corrupcion de memoria (2024-02-12)
  Serial.print(" flash_write256[");
  Serial.print(linea);
  Serial.print("]<-");
  Serial.println(buffer256);
  memcpy(&auxmem[0], &buffer256[0], 256);          //<-la transferencia SPI vacia el buffer asi que se crea un temporal
  flash.loadRandProgData(offset, &auxmem[0], 256); // W25N::loadProgData    (uint16_t columnAdd, char* buf, uint32_t dataLen, uint32_t pageAdd)
  flash.ProgramExecute(pagina);
  if (flash.check_WIP())
  {
    delay(15);
  }          // si todavia esta escribiendo le da 15ms para terminar
  delay(10); //<--importante
  flash_next_line = flash_next_line + 1;
}
//----------------------------- fin flash -----------------------

//-------------inicio lora ---------------
#include <LoRa.h>
#define LORA_SS_PIN 12
#define LORA_RESET_PIN 4
#define LORA_DIO0_PIN 36
#define LORA_DIO1_PIN 21
#define LORA_DIO2_PIN 22
// LoRa setup definition
#define LORA_FREC 915E6      //(`433E6`, `866E6`, `915E6`)
#define LORA_SF 10           // 6 a 12
#define LORA_TX_POW 18       // 2 a 20
#define LORA_BANDWIDTH 125E3 //`7.8E3`, `10.4E3`, `15.6E3`, `20.8E3`, `31.25E3`, `41.7E3`, `62.5E3`, `125E3`, and `250E3`.

bool lora_init()
{
  digitalWrite(V_VSPI_CTRL_PIN, V_VSPI_CTRL_PIN_ON); // Turn on SPI devices
  delay(10);

  // Init Lora module
  LoRa.setPins(LORA_SS_PIN, LORA_RESET_PIN, LORA_DIO0_PIN);
  Serial.print("LORA.... ");
  if (!LoRa.begin(LORA_FREC))
  {
    Serial.println("Fail!!!");
    while (1)
      ;
  }
  else
  {
    Serial.println("OK");
  }
  LoRa.setTxPower(LORA_TX_POW);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BANDWIDTH);
  return true;
}
//-------------fin lora ---------------

float getVin(void)
{
  int adcRead = analogRead(V_MONITOR_PIN);
  float vin = VIN_factor*(float(adcRead) / VmonitorDiv + VmonitorOffSet);
  return vin;
}

bool CheckProcesaLora()
{

//------------- recepcion de paquetes LoRa  ----------------      
      int packetSize = LoRa.parsePacket();
      if (packetSize)
      {
        String LoRaData = LoRa.readString();
        int rssi = LoRa.packetRssi();
        Serial.print("Lora in:");
        Serial.println(LoRaData);
        digitalWrite(FLASH_SS_PIN, LOW);
        digitalWrite(LORA_SS_PIN, HIGH);
        delay(50);
        LoRaData += ",dtgw,"; LoRaData += String(rtc.getTime("%Y-%m-%dT%H:%M:%SZ"));
        LoRaData += ",rssi,"; LoRaData += String(rssi);

        flash_write_srt(flash_next_line, LoRaData);
        digitalWrite(FLASH_SS_PIN, HIGH);
        digitalWrite(LORA_SS_PIN, LOW);
        
        digitalWrite(BUZZER_PIN, BUZZER_PIN_ON);    delay(10);    digitalWrite(BUZZER_PIN, !BUZZER_PIN_ON);// beep when lora paquet arrive
        return true;
      }
      //------------- recepcion de comandos 485 LoRa  ----------------      
  return false;
}

// void apagarGps()
// {
//   digitalWrite(GPS_POW_PIN, GPS_POW_PIN_ON);
// }

#include "WifiNode.h"
//-------------------------------------- wifi portal  --------------------------------
bool time_was_updated_by_gps = false;
void setup()
{
  time_was_updated_by_gps = false;
  start_millis = millis();
  delay(200);
  attachInterrupt(HALL_SENS_PIN, ISR, FALLING);
  delay(200); // Esto evita que se pegue el procesador al despertar del deep sleep
  gpio_hold_dis((gpio_num_t)SW_VIN_PIN);
  pinMode(SW_VIN_PIN, OUTPUT);
  digitalWrite(SW_VIN_PIN, !SW_VIN_PIN_ON);

  pinMode(FLASH_SS_PIN, OUTPUT);
  digitalWrite(FLASH_SS_PIN, HIGH);
  pinMode(LORA_SS_PIN, OUTPUT);
  digitalWrite(LORA_SS_PIN, HIGH);
  pinMode(V_VSPI_CTRL_PIN, OUTPUT);
  Serial.begin(115200);
  //Serial.begin(921600);
  
  if (ISGATEWAY) {
  SerialRS485.begin(115200,SERIAL_8N1, RXD2_PIN, TXD2_PIN);


  Serial.println("Starting the logger in GATEWAY Mode...");
  } else {
  SerialRS485.begin(9600,SERIAL_8N1, RXD2_PIN, TXD2_PIN);
  Serial.println("Starting the logger in Node Mode...");
  }

  delay(100);
  Serial.print("\n\nStarting cicle number: ");
  Serial.println(counter);
  if (counter == 0)
  {
    Serial.print(credits);
  }
  
  String devMAC = WiFi.macAddress();
  Serial.print("Chip MAC:");
  Serial.println(devMAC);
  
  NODEID = "I" + devMAC.substring(12, 14) + devMAC.substring(15, 17);

  if (NODEID == "I9BA0" || NODEID == "I9C8C")    LORA_SEND_SECOND = 15;
  if (NODEID == "I9BE4" || NODEID == "I9B88")    LORA_SEND_SECOND = 20;
  if (NODEID == "I9B98" || NODEID == "I9C68")    LORA_SEND_SECOND = 25;
  if (NODEID == "I9B80" || NODEID == "I9BXX")    LORA_SEND_SECOND = 30;
  if (NODEID == "I2314") {VIN_factor=1.013843648;}
  if (NODEID == "I9B20") {VIN_factor=1.017871018;}//2024.10.02
  if (NODEID == "I22F0") {VIN_factor=1.050847458;}//2024.10.02
  if (NODEID == "I9B24") {VIN_factor=1.034;}//2025.03.06

  if (NODEID == "I9AE8") {VIN_factor=1.028;}//2024.10.02
  if (NODEID == "I2310") {VIN_factor=0.992;}//2024.10.02
  if (NODEID == "I5DD8") {VIN_factor=1.047;}//
  if (NODEID == "I5E18") {VIN_factor=1.0087;}//2025-03-10
  if (NODEID == "I5DFC") {VIN_factor=1.053;}//2025-03-10
  if (NODEID == "I9B94") {VIN_factor=0.976;}//2025-03-10
  if (NODEID == "I9D80") {VIN_factor=0.9797;}//2025-03-10
  

  //zzzzzzzzzzzzzzzz
  if (NODEID == "I5EA4") {TIME_TO_SLEEP_SEC=3600;DELAY_SW_VIN_ON_ms=10000;VIN_factor=1.029315961;}//2024.12.13 Nodo Fernandoy Shelley portatil (100mA, 0.65mA sleep)
  if (HAS_CALYPSO_ULP==1) DELAY_SW_VIN_ON_ms=10000;

  if (BUZZER_ON == 1)
  {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, BUZZER_PIN_ON);
    delay(5);
    digitalWrite(BUZZER_PIN, !BUZZER_PIN_ON);
  }
  Serial.print("RTC:");
  Serial.println(rtc.getTime("%Y-%m-%dT%H:%M:%SZ"));

  //----------------------------- inicio flash -----------------------
  flash_init();
  flash_next_line = flash_getnextline();
  if (flash_next_line == 0)
  {
    flash_write_srt(0, credits);
    Serial.print("flash_next_line = ");
    Serial.println(flash_next_line);
  }
  if (counter == 0)
  {
    char buf[256];
    unsigned long start_line = 0;
    if (flash_next_line > 20)
      start_line = flash_next_line - 20;
    for (int q = start_line; q < flash_next_line; q++)
    {
      flash_get256(q, &buf[0]);
      Serial.print("flash[");
      Serial.print(q);
      Serial.print("] = ");
      Serial.println(buf);
    }
  }
  //----------------------------- fin flash -----------------------

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    WebServerMode();
  delay(100);

  //----------------------------- inicio GPS (actualizacion de hora) -----------------------
  //counter = 1;
  if (counter % 144 == 3 || (getVin()>13.0 && counter % 36 == 3)) // si o si una vez al dia se prendera el GPS
  {
    Serial.print("GPS ON [5m max]: ");
    pinMode(GPS_POW_PIN, OUTPUT);    digitalWrite(GPS_POW_PIN, GPS_POW_PIN_ON);    delay(500);
    GPS_co1 gps(&SerialGPS);    gps.init(9600, GPS_RX_PIN, GPS_TX_PIN);

    bool hora_ok = false;

    if (getVin() > 12.6)
    {
      gps.GPS_getdata(60000 * 5, 7);
      hora_ok = gps.GPS_gettime(60000 * 5, true);
    } // una vez al dia pide un fix de al menos 7 satelites
    else hora_ok = gps.GPS_gettime(60000 * 5, true); // cada 24 ciclos (1 vez por hora) RTC del GPS

    if (hora_ok)
    {
      uint32_t dt_rtc = rtc.getEpoch();
      uint32_t dt_gps = gps.GPS_datetime_RTC.GetDT32();
      int dt_drift = dt_rtc - dt_gps;
      Serial.print("\nInterval dift (seconds):");
      Serial.println(dt_drift);

   
      rtc.setTime(gps.GPS_datetime_RTC.segundo(), gps.GPS_datetime_RTC.minuto(), gps.GPS_datetime_RTC.hora(), gps.GPS_datetime_RTC.dia(), gps.GPS_datetime_RTC.mes(), gps.GPS_datetime_RTC.ano());
      Serial.print("RTC:");
      Serial.print(rtc.getTime("%Y-%m-%dT%H:%M:%SZ"));
      Serial.println(" updated by GPS");
      time_was_updated_by_gps=true;
      // gps.GPS_updateTimeWithMillis();
      // Serial.print("GPS:"); Serial.println(gps.GPS_datetime_RTC.GetISO());

      // Serial.println("clock was updated from GPS");
      if (BUZZER_ON == 1)
      {
        for (int h = 0; h < 10; h++)
        {
          digitalWrite(BUZZER_PIN, BUZZER_PIN_ON);          delay(30);          digitalWrite(BUZZER_PIN, !BUZZER_PIN_ON);          delay(30);
        }
      }

      if (gps.GPS_lat != 0)
        gps_lat = gps.GPS_lat;
      if (gps.GPS_lon != 0)
        gps_lon = gps.GPS_lon;
      if (gps.GPS_alt != 0)
        gps_alt = gps.GPS_alt;
    }
    else
    {
      Serial.println("time update from GPS failed!"); // Serial.print(i);Serial.println(" of 10");
    }

    digitalWrite(GPS_POW_PIN, !GPS_POW_PIN_ON);
    Serial.println("GPS OFF");

    gps.GPS_printactual();
  }
  //else    Serial.println("GPS Skipped by low Voltaje");

  //----------------------------- fin GPS -----------------------

  //------------------------- inicio lora-----------------
  lora_init();
  // ------------------------------- fin lora ----------------------------------

} // fin setup
void system_printPortStaus()
{
  Serial.print("Port status:");
  Serial.print("\n - SW_VIN_PIN [");
  Serial.print(SW_VIN_PIN);
  Serial.print("]:");
  Serial.print(EstadoPuerto(SW_VIN_PIN));
  Serial.print("\n - GPS_POW_PIN [");
  Serial.print(GPS_POW_PIN);
  Serial.print("]:");
  Serial.print(EstadoPuerto(GPS_POW_PIN));
  Serial.print("\n - V_VSPI_CTRL_PIN [");
  Serial.print(V_VSPI_CTRL_PIN);
  Serial.print("]:");
  Serial.print(EstadoPuerto(V_VSPI_CTRL_PIN));
  Serial.print("\n - LORA_SS_PIN [");
  Serial.print(LORA_SS_PIN);
  Serial.print("]:");
  Serial.print(EstadoPuerto(LORA_SS_PIN));
  Serial.print("\n - FLASH_SS_PIN [");
  Serial.print(FLASH_SS_PIN);
  Serial.print("]:");
  Serial.print(EstadoPuerto(FLASH_SS_PIN));
  Serial.print("\n - BUZZER_PIN [");
  Serial.print(BUZZER_PIN);
  Serial.print("]:");
  Serial.print(EstadoPuerto(BUZZER_PIN));
  Serial.print("\n - STATUS_LED_PIN [");
  Serial.print(STATUS_LED_PIN);
  Serial.print("]:");
  Serial.print(EstadoPuerto(STATUS_LED_PIN));
}
void loop()
{
  
  String cmd485 = "";
  String resp = "";
  String dataLine;


  Serial.print("Loop init:");
  struct tm timeinfo = rtc.getTimeStruct();
  Serial.println(rtc.getTime("%Y-%m-%dT%H:%M:%SZ"));
  
  if (!ISGATEWAY)
  {
    
    //-------------------inicio obtencion de datos de los sensores------------------------------------

    Serial.print("SWV:ON for: ");Serial.print(DELAY_SW_VIN_ON_ms);Serial.print(" [ms]");
    
    
    digitalWrite(SW_VIN_PIN, SW_VIN_PIN_ON);
    
    if (DELAY_SW_VIN_ON_ms>5000) 
    {
      // delay(DELAY_SW_VIN_ON_ms-5000);
       
       rs485_sendCmd("");       rs485_getLines(2000); 
       rs485_sendCmd("");       rs485_getLines(2000); 
       rs485_sendCmd("");       rs485_getLines(2000); 
       rs485_sendCmd("");       rs485_getLines(2000); 
       rs485_sendCmd("");       rs485_getLines(2000); 
       rs485_sendCmd("");       rs485_getLines(2000); 
       rs485_sendCmd("");       rs485_getLines(2000); 
      
    }
    else delay(DELAY_SW_VIN_ON_ms);
    
     
    
     Serial.println("Querying sensors (RS485) ");
    //query conversor (ID:200)
    rs485_sendCmd("<200,at>");
    resp = rs485_getLine(2000);
    if (resp != "OK")
    {
      Serial.println("Warning: Conversor is not responding");
    }
    if (resp == "OK")
    {
      if (time_was_updated_by_gps){String cmd_updtz="<200,SET_DTZ,"+ rtc.getTime("%Y-%m-%dT%H:%M:%SZ")+">";rs485_sendCmd(cmd_updtz);resp = rs485_getLine(500);}
    }
    
    rs485_sendCmd("<200,GET_DTZ>");
    String fecha_hora_conversor = rs485_getLine(1000);
      
      int year, month, dayOfMonth, hour, minute, second;
      // Convertir String a char array
      char dateTimeCharArray[25];
      fecha_hora_conversor.toCharArray(dateTimeCharArray, 25);
      if (sscanf(dateTimeCharArray, "%4d-%2d-%2dT%2d:%2d:%2dZ", &year, &month, &dayOfMonth, &hour, &minute, &second) == 6) 
      {
        int dayOfWeek = 1;//calculateDayOfWeek(year, month, dayOfMonth);
        //year=year-1900;
        if (year>2020)
        {
           rtc.setTime(second, minute, hour, dayOfMonth, month, year);
           Serial.println(rtc.getTime("UPD SYSTIME %Y-%m-%dT%H:%M:%SZ"));
        }
      }

     
    rs485_sendCmd("<200,get_hs>");

    String snow_height = rs485_getLine(2000);

    /// SerialRS485.print("<200,get_ta>");Serial.print(" RS485->'<200,get_ta>'"); SerialRS485.flush();
    rs485_sendCmd("<200,get_ta>");
    String air_temperature = rs485_getLine(2000);

    
delay(1500);//para que no pregunte tan rapido a la pesa // 20241004
String snow_weight = "";
    String snow_weight_temp = "";
    
    if (ID_SNOWSCALE1 > 0)
    {

      cmd485 = String("<") + String(ID_SNOWSCALE1) + String(",at>");
      //rs485_sendCmd(cmd485); resp = rs485_getLine(100);
      rs485_sendCmd(cmd485); resp = rs485_getLine(1000);
      if (resp == "OK")
      {
        cmd485 = String("<") + String(ID_SNOWSCALE1) + String(",get_w>");
        rs485_sendCmd(cmd485);
        snow_weight = rs485_getLine(10000);

        cmd485 = String("<") + String(ID_SNOWSCALE1) + String(",get_t>");
        rs485_sendCmd(cmd485);
        snow_weight_temp = rs485_getLine(2000);
      }
    }
    String snow_weight2 = "";
    String snow_weight_temp2 = "";
    if (ID_SNOWSCALE2 > 0)
    {
      cmd485 = String("<") + String(ID_SNOWSCALE2) + String(",at>");
      rs485_sendCmd(cmd485);
      resp = rs485_getLine(2000);
      if (resp == "OK")
      {
        cmd485 = String("<") + String(ID_SNOWSCALE2) + String(",get_w>");
        rs485_sendCmd(cmd485);
        snow_weight2 = rs485_getLine(10000);

        cmd485 = String("<") + String(ID_SNOWSCALE2) + String(",get_t>");
        rs485_sendCmd(cmd485);
        snow_weight_temp2 = rs485_getLine(2000);
      }
    }

    String snow_weight3 = "";
    String snow_weight_temp3 = "";
    if (ID_SNOWSCALE3 > 0)
    {
      cmd485 = String("<") + String(ID_SNOWSCALE3) + String(",at>");
      rs485_sendCmd(cmd485);
      resp = rs485_getLine(2000);
      if (resp == "OK")
      {
        cmd485 = String("<") + String(ID_SNOWSCALE3) + String(",get_w>");
        rs485_sendCmd(cmd485);
        snow_weight3 = rs485_getLine(10000);

        cmd485 = String("<") + String(ID_SNOWSCALE3) + String(",get_t>");
        rs485_sendCmd(cmd485);
        snow_weight_temp3 = rs485_getLine(2000);
      }
    }

    String snow_weight4 = "";
    String snow_weight_temp4 = "";
    if (ID_SNOWSCALE4 > 0)
    {
      cmd485 = String("<") + String(ID_SNOWSCALE4) + String(",at>");
      rs485_sendCmd(cmd485);
      resp = rs485_getLine(2000);
      if (resp == "OK")
      {
        cmd485 = String("<") + String(ID_SNOWSCALE4) + String(",get_w>");
        rs485_sendCmd(cmd485);
        snow_weight4 = rs485_getLine(10000);

        cmd485 = String("<") + String(ID_SNOWSCALE4) + String(",get_t>");
        rs485_sendCmd(cmd485);
        snow_weight_temp4 = rs485_getLine(2000);
      }
    }


   String camara_res_ok = "";
   String camara_freespace = "";
    if (ID_CAMARA > 0)
    {
      cmd485 = String("<") + String(ID_CAMARA) + String(",at>");
      rs485_sendCmd(cmd485);
      resp = rs485_getLine(2000);
      if (resp == "OK")
      {
        cmd485 = String("<") + String(ID_CAMARA) + String(",takephotoandsave, 2025031021>");
        rs485_sendCmd(cmd485);
        camara_res_ok = rs485_getLine(5000); //<- respuesta de la camara podria ser OK

        cmd485 = String("<") + String(ID_CAMARA) + String(",get_freespace, 2025031021>");
        rs485_sendCmd(cmd485);
        camara_freespace = rs485_getLine(5000); //<- respuesta de la camara podria ser OK

      }
    }


    calypso_data ws_data;
    bool res_calypso_ok=false;
    if (HAS_CALYPSO_ULP)
    {
      Serial.println("Querying Calypso Wind Sensor (9600bps/MODBUS485)");
      res_calypso_ok=Calypso_GetData(ws_data, ID_CALYPSO,100,9600);//Calypso_GetData(result, id_sensor,timeout_ms,baudrate_485)
      //Serial.print("| RS485<- [ws_3s,");Serial.print(ws_data.ws_3s);  Serial.print(",wd_3s,");Serial.print(ws_data.wd_3s);       Serial.println("] ");
    }

    // SerialRS485.print("<200,get_fail>");Serial.print(" RS485->'<200,get_fail>'"); SerialRS485.flush();
    // String conv485_falla = rs485_getLine(2000);

    digitalWrite(SW_VIN_PIN, !SW_VIN_PIN_ON);

    //-------------------fin obtencion de datos de los sensores------------------------------------

    //---------------------- inicio generar linea de datos ---------------------------------
    dataLine = "";
    dataLine += "id,";
    dataLine += String(NODEID);
    dataLine += ",dt,";
    dataLine += String(rtc.getTime("%Y-%m-%dT%H:%M:00Z"));
    dataLine += ",vin,";
    dataLine += String(getVin());
    if (air_temperature.length()>0) {dataLine += ",at,";    dataLine += String(air_temperature);}
    if (snow_height.length()>0) {dataLine += ",sh,";    dataLine += String(snow_height);}
    if (ID_SNOWSCALE1 > 0&& snow_weight.length() > 0)
    {
      dataLine += ",sw,";
      dataLine += String(snow_weight);
      dataLine += ",swt,";
      dataLine += String(snow_weight_temp);
    }
    if (ID_SNOWSCALE2 > 0 && snow_weight2.length() > 0)
    {
      dataLine += ",sw2,";
      dataLine += String(snow_weight2);
      dataLine += ",swt2,";
      dataLine += String(snow_weight_temp2);
    }
    if (ID_SNOWSCALE3 > 0 && snow_weight3.length() > 0)
    {
      dataLine += ",sw3,";
      dataLine += String(snow_weight3);
      dataLine += ",swt4,";
      dataLine += String(snow_weight_temp3);
    }
    if (ID_SNOWSCALE4 > 0 && snow_weight4.length() > 0)
    {
      dataLine += ",sw4,";
      dataLine += String(snow_weight4);
      dataLine += ",swt4,";
      dataLine += String(snow_weight_temp4);
    }
    if (HAS_CALYPSO_ULP && res_calypso_ok)
    {
     dataLine += ",ws,";  dataLine += String(ws_data.ws_3s);
     dataLine +=  ",wd,"; dataLine += String(ws_data.wd_3s);
    }
    
    if (HAS_CALYPSO_ULP && res_calypso_ok)
    {
     dataLine += ",ws,";  dataLine += String(ws_data.ws_3s);
     dataLine +=  ",wd,"; dataLine += String(ws_data.wd_3s);
    }

    if (ID_CAMARA>0&&camara_res_ok=="OK")
    {
      dataLine +=  ",cm,"; dataLine += String(camara_freespace);
    }
    

    //---------------------- generar linea de datos cada 100 registros ---------------------------------
    if (counter % 100 == 0 && gps_lat != 0)
    {
      char buff[12];

      dataLine = "id,";
      dataLine += String(NODEID);
      dataLine += ",dt,";
      dataLine += String(rtc.getTime("%Y-%m-%dT%H:%M:%SZ"));
      dtostrf(gps_lat, 4, 6, buff);
      dataLine += ",lat,";
      dataLine += String(buff);
      dtostrf(gps_lon, 4, 6, buff);
      dataLine += ",lon,";
      dataLine += String(buff);
      dtostrf(gps_alt, 3, 2, buff);
      dataLine += ",alt,";
      dataLine += String(buff);
      dataLine += ",fw,";
      dataLine += firmware_ver;
    }

    char dataLine_char[255];
    dataLine.toCharArray(dataLine_char, dataLine.length()+1);

    // CRC32 crc;crc.reset();crc.setPolynome(0x04C11DB7);
    CRC16 crc;
    crc.reset();
    crc.setPolynome(0x1021);
    for (int c = 0; c < dataLine.length(); c++)
      crc.add(dataLine_char[c]);
    // Serial.println(crc.getCRC());
    dataLine += ",C,";
    dataLine += String(crc.calc());

    Serial.print("Dataline:");
    Serial.println(dataLine);
    //--------- fin generar linea de datos ------------------------
    //------------------------------------------------------------- gpio_hold_dis((gpio_num_t) forwardLeft);
    flash_write_srt(flash_next_line, dataLine);

    //----------------------------------------------------------------------

    //-------------envio lora de la linea-----------------------------------
    if (EstadoPuerto(V_VSPI_CTRL_PIN) == !V_VSPI_CTRL_PIN_ON)
      digitalWrite(V_VSPI_CTRL_PIN, V_VSPI_CTRL_PIN_ON);
    delay(50);
    digitalWrite(LORA_SS_PIN, HIGH);
    digitalWrite(FLASH_SS_PIN, HIGH);

    uint64_t sleep_to_lora = uint64_t(60) - uint64_t(timeinfo.tm_sec) + LORA_SEND_SECOND;
    if (sleep_to_lora > 120)
      sleep_to_lora = 60 + LORA_SEND_SECOND;
    sleep_to_lora = 0;
    Serial.print("Waiting for LoRa window at:");
    Serial.print(String(rtc.getTime("%Y-%m-%dT%H:%M:%SZ")));
    Serial.print(" for ");
    Serial.print(sleep_to_lora);
    Serial.println(" seconds");
    Serial.flush();

    delay(sleep_to_lora * 1000);
    Serial.print("Sending LoRa packet at: ");
    Serial.print(String(rtc.getTime("%Y-%m-%dT%H:%M:%SZ")));
    LoRa.beginPacket();
    LoRa.print(dataLine);
    LoRa.endPacket(false); // false blocking/true async
    Serial.print(" Lora->");
    Serial.println(dataLine);

  } // fin if (!ISGATEWAY)
  ///------------------------ Inicio funcionamiento modo gateway ---------------------
  if (ISGATEWAY)
  {
    //-----------apagar Switches  -------
    digitalWrite(SW_VIN_PIN, SW_VIN_PIN_ON);
    digitalWrite(GPS_POW_PIN, !GPS_POW_PIN_ON);
    pinMode(GPS_POW_PIN, INPUT_PULLUP); // apagar el GPS debe ser con un INPUT_PULLUP (si no no lo apaga)
    // SerialRS485.end();
    // SerialRS485.begin(115200);
   Serial.println("Starting the radio/command listening...");
    unsigned long millis_router_on = millis();
    unsigned long current_millis = millis();
    bool rut_on_flag = false;
    GPS_co1 gps(&SerialGPS); 
    gps.init(9600, GPS_RX_PIN, GPS_TX_PIN);
    pinMode(GPS_POW_PIN, OUTPUT);   digitalWrite(GPS_POW_PIN, !GPS_POW_PIN_ON);  
    while (true)
    {
      // ---------------- GPS managment -------------------
      current_millis = millis();
      unsigned int current_seconds = (current_millis - (current_millis%1000))/1000;
      if (current_seconds>86400UL) resetFunc();//una vez al dia se reinicia 
      if (current_seconds % 3600 == 2 && EstadoPuerto(GPS_POW_PIN) == !GPS_POW_PIN_ON) 
      {
        Serial.print("GPS ON [5m max]: ");
        digitalWrite(GPS_POW_PIN, GPS_POW_PIN_ON);    
        //gps.init(9600, GPS_RX_PIN, GPS_TX_PIN);
      }
      if (current_seconds % 3600 < 300 && EstadoPuerto(GPS_POW_PIN) == GPS_POW_PIN_ON) 
      {
        if (gps.GPS_gettime(500, true))  
        {
          rtc.setTime(gps.GPS_datetime_RTC.segundo(), gps.GPS_datetime_RTC.minuto(), gps.GPS_datetime_RTC.hora(), gps.GPS_datetime_RTC.dia(), gps.GPS_datetime_RTC.mes(), gps.GPS_datetime_RTC.ano());
          Serial.print("\nRTC Updated by GPS: ");  Serial.print(String(rtc.getTime("%Y-%m-%dT%H:%M:%SZ")));
          digitalWrite(GPS_POW_PIN, !GPS_POW_PIN_ON); 
          Serial.println("...GPS OFF ");
        }
      }

    //------------- reseteo del router  c/1h ----------------
      // if ((millis() - millis_router_on > 60000UL) && !rut_on_flag ) //debug: cada 1 minuto...
       if (millis() - millis_router_on > (1000UL * 60UL * 60UL) ) // cada 1h resetear el router (que esta conectado a la linea SW12)
      {
        digitalWrite(SW_VIN_PIN, !SW_VIN_PIN_ON);
         millis_router_on = millis();
         delay(5000);
         digitalWrite(SW_VIN_PIN, SW_VIN_PIN_ON);
      }
      
      bool pack_recv= CheckProcesaLora();
      if (SerialRS485.available())   Serial_processCmd(SerialRS485,SerialRS485.readString());
      if (Serial.available())        Serial_processCmd(Serial,Serial.readString());

      //----------- chequeo del llamado al modo webserver --------------------
      if (hallSensorFlag)  WebServerMode();

      

    }//fin while true


  }//fin if es_gateway
  ///------------------------ fin funcionamiento modo gateway ---------------------

  if (EstadoPuerto(V_VSPI_CTRL_PIN) == !V_VSPI_CTRL_PIN_ON)
    digitalWrite(V_VSPI_CTRL_PIN, !V_VSPI_CTRL_PIN_ON);
  delay(50);

 

  if (hallSensorFlag)
    WebServerMode();

  //-----------apagar Switches  -------
  digitalWrite(SW_VIN_PIN, !SW_VIN_PIN_ON);
  digitalWrite(GPS_POW_PIN, !GPS_POW_PIN_ON);
  digitalWrite(LORA_SS_PIN, HIGH);
  digitalWrite(FLASH_SS_PIN, HIGH);
  digitalWrite(V_VSPI_CTRL_PIN, !V_VSPI_CTRL_PIN_ON); // Turn on SPI devices
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  pinMode(GPS_POW_PIN, INPUT_PULLUP); // apagar el GPS debe ser con un INPUT_PULLUP (si no no lo apaga)
  pinMode(V_VSPI_CTRL_PIN, INPUT_PULLUP);

  end_millis = millis();

  cycle_millis = end_millis - start_millis;
  uint64_t cycle_seconds = cycle_millis / 1000UL;

  timeinfo = rtc.getTimeStruct();
  uint64_t next_wake = TIME_TO_SLEEP_SEC - (uint64_t(timeinfo.tm_sec) + uint64_t(timeinfo.tm_min % 10) * 60); // al segundo 00
  if (next_wake > TIME_TO_SLEEP_SEC)    next_wake = TIME_TO_SLEEP_SEC;
  next_wake = TIME_TO_SLEEP_SEC; // no despertar en los segundos 0 exactamente
  if (counter < 3)    next_wake = 10; // los primeros 3 ciclos los hace durmiendo solo 10s

  // --------------- Sleep start ---------------
  Serial.print("Going to sleep at:");
  Serial.print(String(rtc.getTime("%Y-%m-%dT%H:%M:%SZ")));
  Serial.print(" for ");
  Serial.print(next_wake);
  Serial.println(" seconds");
  Serial.flush();

  // system_printPortStaus();

  counter++;

  // TIME_TO_SLEEP_SEC * uS_TO_S_FACTOR
  // #define TIME_TO_SLEEP_SEC  600        /* Time ESP32 will go to sleep (in seconds) */

  if (BUZZER_ON == 1)
  {
    digitalWrite(BUZZER_PIN, BUZZER_PIN_ON);
    delay(5);
    digitalWrite(BUZZER_PIN, !BUZZER_PIN_ON);
    delay(50);
    digitalWrite(BUZZER_PIN, BUZZER_PIN_ON);
    delay(5);
    digitalWrite(BUZZER_PIN, !BUZZER_PIN_ON);
    delay(50);
  }

  gpio_hold_en((gpio_num_t)SW_VIN_PIN); // GPIO5 debe quedar en hold durante el deep sleep
  gpio_deep_sleep_hold_en();
  esp_sleep_enable_timer_wakeup(next_wake * uS_TO_S_FACTOR);
  if (HALL_SENS_PIN==33) esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);
  if (HALL_SENS_PIN==26) esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_deep_sleep_start();
  // esp_light_sleep_start();

} // fin loop
