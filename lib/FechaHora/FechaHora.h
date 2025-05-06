#ifndef FechaHora_h
#define FechaHora_h

#include "Arduino.h"
#include <stdlib.h>
#include <time.h>

// ano, mes, dia, hora, minuto y segundo son enteros
// uso FechaHora reloj;
// reloj.SetFechaHora(2017,2,5,10,50,0);//2017-02-05 10:50:00
// 2021.02.08 fix cuando año 2000 marcaba ok

// documentacion generada por copilot (16 sept-24) ~ carlo
/**
 * @class FechaHora
 * @brief A class to manage date and time in ISO format.
 *
 * This class provides functionalities to set, get, and validate date and time.
 * It stores the date and time in both a structured format and an ISO string format.
 *
 * @private
 * char fechahora_iso[20]; // ISO formatted date and time string
 * time_t dt32; // Stores the current time in time_t format
 * struct tm fecha_hora; // Structure to hold date and time components
 * unsigned long millis_last_update; // Last update time in milliseconds
 *
 * @public
 * FechaHora(int anof=0, int mesf=0, int diaf=0, int horaf=0, int minutof=0, int segundof=0);
 *
 * @brief Sets the date and time using the provided parameters.
 *
 * @param anof Year (default is 0)
 * @param mesf Month (1-12, default is 0)
 * @param diaf Day (1-31, default is 0)
 * @param horaf Hour (0-24, default is 0)
 * @param minutof Minute (0-60, default is 0)
 * @param segundof Second (0-60, default is 0)
 *
 * void SetFechaHora(int anof=0, int mes1a12f=0, int dia1a31f=0, int horaf=0, int minutof=0, int segundof=0);
 *
 * @brief Updates the date and time with validation.
 *
 * @param anof Year (default is 0)
 * @param mes1a12f Month (1-12, default is 0)
 * @param dia1a31f Day (1-31, default is 0)
 * @param horaf Hour (0-24, default is 0)
 * @param minutof Minute (0-60, default is 0)
 * @param segundof Second (0-60, default is 0)
 *
 * int ano(); // Returns the year
 * int mes(); // Returns the month
 * int dia(); // Returns the day
 * int hora(); // Returns the hour
 * int minuto(); // Returns the minute
 * int segundo(); // Returns the second
 *
 * char *GetISO(); // Returns the ISO formatted date and time string
 * char *GetISOsrt(); // Returns the ISO formatted date and time string
 * String GetFechaYMD(); // Returns the date in YYYY-MM-DD format
 * uint32_t GetDT32(); // Returns the time in time_t format
 * uint32_t GetDT32MillisUpdated(); // Returns the updated time in time_t format
 * void SetDT32(uint32_t dt32f); // Sets the time using time_t format
 * String GetHoraHMS(); // Returns the time in HH:MM:SS format
 * bool EsValida(); // Validates the date and time
 */
class FechaHora
{
private:
  char fechahora_iso[20];
  // String fechahora_iso_str;
public:
  // int ano, mes, dia, hora, minuto, segundo;

  time_t dt32; // Store the current time in time
  struct tm fecha_hora;
  unsigned long millis_last_update;

  FechaHora(int anof = 0, int mesf = 0, int diaf = 0, int horaf = 0, int minutof = 0, int segundof = 0)
  {
    this->SetFechaHora(anof, mesf, diaf, horaf, minutof, segundof);
    millis_last_update = millis();
  }

  void SetFechaHora(int anof = 0, int mes1a12f = 0, int dia1a31f = 0, int horaf = 0, int minutof = 0, int segundof = 0)
  {
    millis_last_update = millis();
    //     Serial.println("\n"+ anof+" "+mes1a12f+" "+dia1a31f+" "+horaf+" "+minutof+" "+segundof+"\n");
    if (anof < 1900 || anof > 2050)
      anof = 1900;
    if (mes1a12f < 1 || mes1a12f > 12)
      mes1a12f = 1;
    if (dia1a31f < 1 || dia1a31f > 31)
      dia1a31f = 1;
    if (horaf < 0 || horaf > 24)
      horaf = 0;
    if (minutof < 0 || minutof > 60)
      minutof = 0;
    if (segundof < 0 || segundof > 60)
      segundof = 0;
    fecha_hora.tm_sec = segundof;
    fecha_hora.tm_min = minutof;
    fecha_hora.tm_hour = horaf;
    fecha_hora.tm_mday = dia1a31f;    // dias de 1 a 31 en tm
    fecha_hora.tm_mon = mes1a12f - 1; // meses de 0 a 11 en tm
    fecha_hora.tm_year = anof - 1900;
    fecha_hora.tm_isdst = 0;

    dt32 = mktime(&fecha_hora);
    memset(fechahora_iso, '\0', sizeof(fechahora_iso));
    sprintf(fechahora_iso, "%04d-%02d-%02d %02d:%02d:%02d", ano(), mes(), dia(), hora(), minuto(), segundo());
    fechahora_iso[19] = '\0'; // debe tener este char al final del string
    // fechahora_iso_str=String(fechahora_iso);
    // fechahora_iso_str="2001-02-03 04:05:06";
    // Serial.print("SetFechaHora(");Serial.println(fechahora_iso);
  }

  int ano() { return int(int(fecha_hora.tm_year) + 1900); }
  int mes() { return fecha_hora.tm_mon + 1; }
  int dia() { return fecha_hora.tm_mday; }
  int hora() { return fecha_hora.tm_hour; }
  int minuto() { return fecha_hora.tm_min; }
  int segundo() { return fecha_hora.tm_sec; }

  // da el tiempo en String, formato YYYY-MM-DD HH:MM:SS
  char *GetISO()
  {
    // fechahora_iso[19]='\0';//debe tener este char al final del string
    // memset(fechahora_iso, '\0', sizeof(fechahora_iso));
    // Serial.print(fechahora_iso);
    // return String (fechahora_iso);//<-- causo problemas alguna vez (mejor no usar)
    // return fechahora_iso_str;
    return &fechahora_iso[0];
  }
  char *GetISOsrt()
  {
    return &fechahora_iso[0];
  }
  String GetFechaYMD()
  {
    char temp[20];
    memset(temp, '\0', sizeof(temp));
    sprintf(temp, "%04d-%02d-%02d", ano(), mes(), dia());
    fechahora_iso[19] = '\0'; // debe tener este char al final del string
    return (String)temp;
  }

  uint32_t GetDT32()
  {
    // https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
    // dt32=segundo + minuto*60 + hora*3600 + diadelanio*86400 +    (ano-70)*31536000 + ((ano-69)/4)*86400 -    ((ano-1)/100)*86400 + ((ano+299)/400)*86400;
    return dt32;
  }

  uint32_t GetDT32MillisUpdated()
  {
    return dt32 + ((millis_last_update - millis()) / 1000);
  }

  void SetDT32(uint32_t dt32f)
  {

    dt32 = (time_t)dt32f;
    millis_last_update = millis();
    /*
    struct tm * ptm;
    ptm = gmtime ( &dt32 );
    fecha_hora=*ptm;
    SetFechaHora(ano(),mes(), dia(),hora(),minuto(),segundo());
    */

    //   struct tm ptm;
    fecha_hora = *gmtime(&dt32);
    //     fecha_hora=ptm;
    SetFechaHora(ano(), mes(), dia(), hora(), minuto(), segundo());
  }
  String GetHoraHMS()
  {
    char temp[20];
    memset(temp, '\0', sizeof(temp));
    sprintf(temp, "%02d:%02d:%02d", hora(), minuto(), segundo());
    fechahora_iso[19] = '\0'; // debe tener este char al final del string
    return (String)temp;
  }

  bool EsValida()
  {
    if (ano() < 2016 || ano() > 2050)
      return false;
    if (mes() < 1 || mes() > 12)
      return false;
    if (dia() < 1 || dia() > 31)
      return false;
    if (hora() < 0 || hora() > 24)
      return false;
    if (minuto() < 0 || minuto() > 60)
      return false;
    if (segundo() < 0 || segundo() > 60)
      return false;
    return true;
  }

private:
};
#endif