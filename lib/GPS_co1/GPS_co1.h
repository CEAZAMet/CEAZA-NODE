#ifndef GPS_co1_h
#define GPS_co1_h

#include <FechaHora.h>
// 2023.11.03 se incluye en GPS_gettime en la fecha que solo se tome la hora si viene sin decimales los segundos
class GPS_co1
{
public:
  float GPS_lat, GPS_lon, GPS_hdop, GPS_alt, GPS_vel, GPS_dir; // nsat>8 muy bien, [nsat 6~dop 1.9, nsat 12~dop 0.7]
  int GPS_nsat;
  unsigned long GPS_millis_last_dt;

  FechaHora GPS_datetime;
  FechaHora GPS_datetime_RTC;

  HardwareSerial *SerialGPSWIFI;

  GPS_co1(HardwareSerial *SerialGPSWIFIfn) : GPS_lat(0.0), GPS_lon(0.0), GPS_hdop(0), GPS_alt(-99.0), GPS_vel(-1), GPS_dir(-1), GPS_nsat(-1), GPS_millis_last_dt(0) { SerialGPSWIFI = SerialGPSWIFIfn; }

  void init(int velocidad, int pin_rx, int pin_tx)
  {
    SerialGPSWIFI->begin(velocidad, SERIAL_8N1, pin_rx, pin_tx);
    GPS_lat = 0.0;
    GPS_lon = 0.0;
    GPS_alt = -99.0;
    GPS_nsat = -1;
    GPS_vel = -1;
    GPS_dir = -1;
    GPS_millis_last_dt = 0;
  }

  bool GPS_gettime(unsigned long timeout_ms, bool valid_required = true)
  {
    bool ok = false;
    bool rtc_updated = false;
    int nro_lineas = 0;
    int conta_clock = 0;
    if (timeout_ms > 600000)
    {
      Serial.print(F("Se solicito GPS por demasiados ms:"));
      Serial.print(timeout_ms);
      timeout_ms = 180000;
    } // maximo tiempo en gps son 3 minutos
    // int min_sat=2;
    unsigned long ts_inicio = millis();
    while (millis() - ts_inicio < timeout_ms && ok == false)
    {
      if (SerialGPSWIFI->available() > 0)
      {
        Serial.print(".");
        // Serial.print(millis()-ts_inicio);     Serial.print("<");     Serial.println(timeout_ms);
        char buffer_in_gps[200];
        memset(buffer_in_gps, '\0', sizeof(buffer_in_gps));
        int cant_bytes_gps = SerialGPSWIFI->readBytesUntil('\n', buffer_in_gps, sizeof(buffer_in_gps));
        nro_lineas++;
        if (memcmp(&buffer_in_gps[0], "$GPTXT,01,01,01,ANTENNA OPEN*25", strlen("$GPTXT,01,01,01,ANTENNA OPEN*25")) == 0)
          Serial.println("Error GPS: ANTENNA OPEN");

        if (memcmp(&buffer_in_gps[0], "$GPRMC", strlen("$GPRMC")) == 0 || memcmp(&buffer_in_gps[0], "$GNRMC", strlen("$GNRMC")) == 0)
        {
          //     $GNRMC,124120.000,V,,,,,,,061023,,,M*52

          // Serial.print("GPS: ");      Serial.println (buffer_in_gps);
          //      Serial.flush();
          String timeHMSs = getValueCSV(String(buffer_in_gps), ',', 1);
          String valid_time = getValueCSV(String(buffer_in_gps), ',', 2);
          String dateDMY = getValueCSV(String(buffer_in_gps), ',', 9);
          int hora = timeHMSs.substring(0, 2).toInt();
          int minuto = timeHMSs.substring(2, 4).toInt();
          int segundo = timeHMSs.substring(4, 6).toInt();
          int milli_segundo = timeHMSs.substring(7, 10).toInt();
          // 01234567890123456789
          //$GNRMC,130838.000,A,2954.89981,S,07114.53160,W,0.00,0.00,031123,,,A*75

          //		Serial.print(timeHMSs.substring(7, 10));
          int dia = dateDMY.substring(0, 2).toInt();
          int mes = dateDMY.substring(2, 4).toInt();
          int ano = dateDMY.substring(4, 6).toInt() + 2000;

          // Serial.println(buffer_in_gps);		 Serial.print("->date:"+dateDMY); Serial.println(", time:"+timeHMSs);
          if (timeHMSs.length() > 4 && dateDMY.length() > 4 && ano >= 2023 && ano <= 2030 && milli_segundo == 0)
          {

            GPS_millis_last_dt = millis(); // 400ms extra entre la llegada al serial del gps y la lectura del dato
            if (valid_time == "A" && conta_clock > 0)
            {
              GPS_datetime.SetFechaHora(ano, mes, dia, hora, minuto, segundo);
              Serial.print("TA");
              GPS_datetime_RTC.SetFechaHora(ano, mes, dia, hora, minuto, segundo);
              return true;
            }
            if (valid_time == "V" && !valid_required && conta_clock > 0)
            {
              GPS_datetime_RTC.SetFechaHora(ano, mes, dia, hora, minuto, segundo);
              Serial.print("TV");
              return true;
            }
            conta_clock++;
          }
          else
            GPS_datetime.SetFechaHora(2000, 1, 1, 0, 0, 0);
        }

        if ((nro_lineas - 1) % 20 == 0 && nro_lineas > 0)
          Serial.print(".");
      } // fin if (SerialGPSWIFI->available()>0)
    } // fin while(tiempo&&notok)
    return false; // si llego hasta aqui es que fallo la actualizacion de la hora
  }
  bool GPS_getdata(unsigned long timeout_ms, int minsat = 0)
  {
    bool ok = false;
    bool rtc_updated = false;
    int nro_lineas = 0;
    if (timeout_ms > 600000)
    {
      Serial.print(F("Se solicito GPS por demasiados ms:"));
      Serial.print(timeout_ms);
      timeout_ms = 180000;
    } // maximo tiempo en gps son 3 minutos
    // int min_sat=2;
    unsigned long ts_inicio = millis();
    while (millis() - ts_inicio < timeout_ms && ok == false)
    {
      if (SerialGPSWIFI->available() > 0)
      {
        Serial.print(".");
        // Serial.print(millis()-ts_inicio);     Serial.print("<");     Serial.println(timeout_ms);
        char buffer_in_gps[200];
        memset(buffer_in_gps, '\0', sizeof(buffer_in_gps));
        int cant_bytes_gps = SerialGPSWIFI->readBytesUntil('\n', buffer_in_gps, sizeof(buffer_in_gps));
        nro_lineas++;
        // Serial.println(buffer_in_gps);
        /*
        GPS: $GPRMC,131853.00,V,,,,,,,150917,,,N*7B
        GPS: $GPVTG,,,,,,,,,N*30
        GPS: $GPGGA,131853.00,,,,,0,00,99.99,,,,,,*6B
        GPS: $GPGSA,A,1,,,,,,,,,,,9.99*30
        GPS: $GPGSV,3,1,09,02,27,246,,03,21,053,293,23*7F
        GPS: $GPGSV,3,2,09,09,6,23,29,115,15,28,06,002,*7C
        GPS: $GPGSV,3,3,09,30,,,,131853.00,V,N*47

        cuando esta funcionando y obtuvo un fix tira algo como (puede demorar mas de 5mins la primera vez):
        0       1         2         3 4           5 6 7  8    9     1 11   234
        $GPGGA,132413.00,2954.88733,S,07114.53107,W,1,04,2.07,112.3,M,33.4,M,,*5F
        *
        $GNGGA,200202.00,2955.51476,S,07112.56402,W,1,05,2.58,140.7,M,33.5,M,,*48 //de esta viene el lat/lon pero no el datetime

        $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
        * $GNRMC,203756.00,V,,,,,,,200820,,,N*6E
         1    = UTC of position fix
         2    = Data status (V=navigation receiver warning)
         3    = Latitude of fix
         4    = N or S
         5    = Longitude of fix
         6    = E or W
         7    = Speed over ground in knots
         8    = Track made good in degrees True
         9    = UT date
        10   = Magnetic variation degrees (Easterly var. subtracts from true course)
        11   = E or W
        12   = Checksum
        */

        //      float GPS_lat,GPS_lon, GPS_hdop, GPS_nsat;
        //      FechaHora GPS_datetime;
        //     $GPTXT,01,01,01,ANTENNA OPEN*25

        if (memcmp(&buffer_in_gps[0], "$GPTXT,01,01,01,ANTENNA OPEN*25", strlen("$GPTXT,01,01,01,ANTENNA OPEN*25")) == 0)
          Serial.println("Error GPS: ANTENNA OPEN");

        // 0_$GNGGA,1_200202.00,2_2955.51476,3_S,4_07112.56402,5_W,6_1,7_05,8_2.58,9_140.7,10_M,11_33.5,12_M,13_,14_*48
        if (memcmp(&buffer_in_gps[0], "$GPGGA", strlen("$GPGGA")) == 0 || memcmp(&buffer_in_gps[0], "$GNGGA", strlen("$GPGGA")) == 0)
        {
          // Serial.println(buffer_in_gps);
          // Serial.print("GPS: ");//      Serial.println (buffer_in_gps);
          //      Serial.flush();

          String gps_tiempo_str = getValueCSV(String(buffer_in_gps), ',', 1);
          String gps_lat_str = getValueCSV(String(buffer_in_gps), ',', 2);
          String gps_NS = getValueCSV(String(buffer_in_gps), ',', 3);
          String gps_lon_str = getValueCSV(String(buffer_in_gps), ',', 4);
          String gps_EO = getValueCSV(String(buffer_in_gps), ',', 5);
          String gps_cant_sat_str = getValueCSV(String(buffer_in_gps), ',', 7);
          String gps_hdop_str = getValueCSV(String(buffer_in_gps), ',', 8);
          String gps_altitud_str = getValueCSV(String(buffer_in_gps), ',', 9);

          GPS_lat = StrToFloat(gps_lat_str) / 100;
          GPS_lon = StrToFloat(gps_lon_str) / 100;
          GPS_hdop = StrToFloat(gps_hdop_str);
          GPS_nsat = gps_cant_sat_str.toInt();
          GPS_alt = StrToFloat(gps_altitud_str);
          ;
          if (GPS_nsat > 30)
          {
            GPS_lat = 0;
            GPS_lon = 0;
            GPS_alt = 0;
          }
          GPS_lat = floor(GPS_lat) + (GPS_lat - floor(GPS_lat)) * 100 / 60;
          GPS_lon = floor(GPS_lon) + (GPS_lon - floor(GPS_lon)) * 100 / 60;

          if (gps_NS == "S")
            GPS_lat = -GPS_lat;
          if (gps_EO == "W")
            GPS_lon = -GPS_lon;
          if (GPS_lat != 0 && GPS_nsat > minsat)
            ok = true;
          if (GPS_lat != 0)
          {
            Serial.print("P");
            Serial.print(GPS_nsat);
          }
        } // fin if ( memcmp(&buffer_in_gps[0],"$GPGGA",strlen("$GPGGA")) == 0)

        if (memcmp(&buffer_in_gps[0], "$GPRMC", strlen("$GPRMC")) == 0 || memcmp(&buffer_in_gps[0], "$GNRMC", strlen("$GNRMC")) == 0)
        {
          // ejemplo: $GNRMC,121541.000,A,2954.89596,S,07114.53464,W,0.00,0.00,031123,,,A*7D
          //          $GNRMC,122110.001,A,2954.89073,S,07114.53278,W,0.31,319.09,031123,,,A*7A (cuando viene con decimales el tiempo es que no esta ok)

          //      Serial.flush();
          // Serial.print("GPS: ");      Serial.println (buffer_in_gps);
          String timeHMSs = getValueCSV(String(buffer_in_gps), ',', 1);
          String valid_time = getValueCSV(String(buffer_in_gps), ',', 2);
          String dateDMY = getValueCSV(String(buffer_in_gps), ',', 9);
          //                 Serial.print("HMS: ");      Serial.println (timeHMSs);
          if (GPS_nsat > 3)
          {
            String velknot = getValueCSV(String(buffer_in_gps), ',', 7);
            String gpsdir = getValueCSV(String(buffer_in_gps), ',', 8);
            GPS_dir = StrToFloat(gpsdir);
            GPS_vel = StrToFloat(velknot) * 1.852;
          }
          else
          {
            GPS_dir = 0;
            GPS_vel = 0;
          }

          int hora = timeHMSs.substring(0, 2).toInt();
          int minuto = timeHMSs.substring(2, 4).toInt();
          int segundo = timeHMSs.substring(4, 6).toInt();

          int milli_segundo = timeHMSs.substring(7, 10).toInt();

          int dia = dateDMY.substring(0, 2).toInt();
          int mes = dateDMY.substring(2, 4).toInt();
          int ano = dateDMY.substring(4, 6).toInt() + 2000;
          if (timeHMSs.length() > 4 && dateDMY.length() > 4 && valid_time != "V" && ano >= 2023 && milli_segundo == 0)
          {

            GPS_datetime.SetFechaHora(ano, mes, dia, hora, minuto, segundo);
            GPS_millis_last_dt = millis();
            // Serial.println(buffer_in_gps);		 Serial.print("->date:"+dateDMY); Serial.println(", time:"+timeHMSs);
            Serial.print("T");
          }
          else
            GPS_datetime.SetFechaHora(2000, 1, 1, 0, 0, 0);
          if (GPS_datetime.EsValida() && GPS_datetime.ano() >= 2023)
          {
            // if (rtc_updated==false){rtc.SetFechaHora(GPS_datetime);rtc_updated=true;}//si llega una fecha valida por gps actualizar la hora del reloj, pero solo 1 vez por fix
            if (minsat == 0)
              ok = true; // si se solicitan 0 sat significa que solo se requiere la hora
            // Serial.print("GPS_datetime: ");Serial.println(GPS_datetime.GetISO());
          }
        }

        if ((nro_lineas - 1) % 20 == 0 && nro_lineas > 0)
          Serial.print(".");
      } // fin if (SerialGPSWIFI->available()>0)
    } // fin while(tiempo&&notok)
    return ok;
  } // fin GPS_getdata(char *bufferout)

  void GPS_updateTimeWithMillis()
  {
    uint32_t last_gps_dt = GPS_datetime.GetDT32();
    int diff_gps_seconds = int(millis() - GPS_millis_last_dt) / 1000;
    GPS_datetime.SetDT32(last_gps_dt + diff_gps_seconds);

    last_gps_dt = GPS_datetime_RTC.GetDT32();
    diff_gps_seconds = int(millis() - GPS_millis_last_dt) / 1000;
    GPS_datetime_RTC.SetDT32(last_gps_dt + diff_gps_seconds);
  }
  void GPS_printactual()
  {
    Serial.print("dt,");
    Serial.print(GPS_datetime.GetISO());
    Serial.print(",lat,");
    Serial.print(GPS_lat, 6);
    Serial.print(",lon,");
    Serial.print(GPS_lon, 6);
    Serial.print(",hdop,");
    Serial.print(GPS_hdop, 2);
    Serial.print(",alt,");
    Serial.print(GPS_alt, 1);
    Serial.print(",vel,");
    Serial.print(GPS_vel, 1);
    Serial.print(",dir,");
    Serial.print(GPS_dir, 1);
    Serial.print(",nsat,");
    Serial.print(GPS_nsat);
    Serial.println();
  }

  String getValueCSV(String data, char separator, int index)
  {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
      if (data.charAt(i) == separator || i == maxIndex)
      {
        found++;
        strIndex[0] = strIndex[1] + 1;
        strIndex[1] = (i == maxIndex) ? i + 1 : i;
      }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
  }

  float StrToFloat(String str)
  {
    char buf[str.length()];
    str.toCharArray(buf, str.length());
    float float_val = atof(buf);
    return float_val;
  }
};

#endif
