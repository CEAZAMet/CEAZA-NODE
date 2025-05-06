#ifndef IOT_WEBSERVER_INDEP_H
#define IOT_WEBSERVER_INDEP_H
//-------------------------------------- wifi portal  --------------------------------

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

/* la funcion del modo webserver */
void WebServerMode();

/* funciones para sacar fecha, lineas entre fechas y lineas desde memoria */
unsigned long getEpoch(String &datetime);
unsigned long getEpochFromLine(String &line);
String getLinesBetweenTimestamps(String &fecha_inicio, String &fecha_fin);
String returnLines(unsigned long primera_linea, unsigned long end_line);

/* NODE_CONFIG Struct */

struct NODE_CONFIG
{
    uint8_t mode;         // 0: nodo, 1: gateway
    char node_id[16];     // identificador nodo, se debe mantener el id en caso de reemplazarse el nodo
    char wifi_ssid[32];   // ssid de la red wifi
    char wifi_pass[32];   // password de la red wifi
    char remote_host[32]; // host remoto (servidor tunel)
    uint16_t remote_port; // puerto remoto (servidor tunel)
    // tam en bytes: 1+16+32+32+32+2 = 115
    /*
    // ejemplo de uso
    NODE_CONFIG config;
    EEPROM.get(0, config);

     */
} __attribute__((packed));

// eeprom tiene fallas en escritura, probar con preferences....

NODE_CONFIG config;

void print_eeprom_config();
int VaciarBufferSerial();
bool EstadoPuerto(int puerto);
float getVin(void);

void print_eeprom_config()
{
    EEPROM.get(0, config);
    Serial.println("node_mode: " + String(config.mode));
    Serial.println("node_id: " + String(config.node_id));
    Serial.println("wifi_ssid: " + String(config.wifi_ssid));
    Serial.println("wifi_pass: " + String(config.wifi_pass));
    Serial.println("remote_host: " + String(config.remote_host));
    Serial.println("remote_port: " + String(config.remote_port));
}

/* fin NODE_CONFIG Struct */

/* html */

String html_output = "<pre>%LINEAS%</pre>";
const static char html_head[] = R"====(<!DOCTYPE html>
<html lang='es'>

<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Portal IOT Node</title>
</head>
<style>
    :root {
        font-size: 14px;
        --primary-color: #0b7cbd;
        --secondary-color: #F2F2F2;
    }
    body {
        height: 100%;
        width: 100%;
        display: flex;
        flex-flow: column nowrap;
        justify-content: flex-start;
        align-items: center;
        gap: 10px;
        margin: 0;
    }
    p, a, label, span, h1, h2, h3, h4, h5 {
        font-family: helvetica;
        margin: 0;
    }
    a {
        text-decoration: none;
        text-align: center;
    }
    h1 {
        font-size: 1.8rem;
    }
    form {
        display: flex;
        flex-flow: column nowrap;
        justify-content: center;
        align-items: center;
        width: 80%;
        height: 80%;
        gap: 10px;
    }
    .btn-options {
        background-color: var(--primary-color);
        color: white;
        font-size: 2rem;
        width: 100%;
        border-radius: 10px;
        border: 2px solid white;
    }
    a.btn-options:hover {
        background-color: #09669B;
    }
    .titulos {
        height: 20%;
        display: flex;
        flex-flow: row wrap;
        justify-content: flex-start;
        align-items: flex-start;
    }
    .titulos h1, h2 {
        width: 100%;
        text-align: center;
    }
    .options {
        display: flex;
        flex-flow: column wrap;
        justify-content: center;
        align-items: center;
        width: 80%;
        gap: 5px;
        flex-basis: 300px;
        border: 1px dashed black;
        border-radius: 10px;
        padding: 1rem;
    }
    .ranges {
        display: flex;
        flex-flow: column wrap;
        justify-content: center;
        align-items: flex-start;
        width: 100%;
        border: 1px dashed black;
        border-radius: 10px;
        padding: 1rem;
    }
    .range {
        display: flex;
        flex-flow: column nowrap;
        justify-content: center;
        align-items: center;
        width: 100%;
        height: 45%;
    }
    .self-centered {
        height: 10%;
        align-self: center;
    }
    input[type=date],
    input[type=time] {
        width: auto;
        height: 100%;
        margin: 0.4rem 0;
        border: 0.1rem solid darkgray;
        border-radius: 10px;
        padding: 0.4rem 0.1rem;
    }
</style>
<body>
    <div class='titulos'>
        <h1>Portal IOT Node 1.4</h1>
        <h2>&COPY;CEAZA, 2024</h2>
    </div>
)====";

const static char html_gateway_form[] = R"====(
    <div class='options'>
        <h3>Options</h3>
        <a class='btn-options' href='/get_all_data'>Get all data</a>
        <a class='btn-options' href='/get_fake_data'>Get simulated data</a>
        <a class='btn-options' href='/get_last_240'>Get last 240 rows of data</a>
        <a class='btn-options' href='/setup_wifi_form'>Setup local WiFi credentials</a>
        <a class='btn-options' href='/setup_remote_host_form'>Setup remote host credentials</a>
        <a class='btn-options' href='/setup_profile_form'>Setup device profile</a>
        <a class='btn-options' href='/restart'>Restart device</a>
    </div>
    <!-- get by datetime -->
    <h3>Get by datetime ranges</h3>
    <form method='GET' action='/get_by_datetime'>
        <div class='ranges'>
            <div class='range start-range'>
                <label for='fecha_inicio'>Start datetime</label>
                <div class='datetime'>
                    <input type='date' name='fecha_inicio' id='fecha_inicio' value='2023-01-01'>
                    <input type='time' name='hora_inicio' id='hora_inicio' value='00:00'>
                </div>
            </div>
            <div class='range end-range'>
                <label for='fecha_fin'>End datetime</label>
                <div class='datetime'>
                    <input type='date' name='fecha_fin' id='fecha_fin' value='2025-01-01'>
                    <input type='time' name='hora_fin' id='hora_fin' value='00:00'>
                </div>
            </div>
            <button type='submit' class='btn-options self-centered'>Get date-ranged data</button>
        </div>
    </form>
)====";

const static char html_node_form[] = R"====(
    <div class='options'>
        <h3>Options</h3>
        <a class='btn-options' href='/get_all_data'>Get all data</a>
        <a class='btn-options' href='/get_fake_data'>Get simulated data</a>
        <a class='btn-options' href='/get_last_240'>Get last 240 rows of data</a>
        <a class='btn-options' href='/setup_wifi_form'>Setup local WiFi credentials</a>
        <a class='btn-options' href='/setup_profile_form'>Setup device profile</a>
        <a class='btn-options' href='/restart'>Restart device</a>
    </div>
    <!-- get by datetime -->
    <h3>Get by datetime ranges</h3>
    <form method='GET' action='/get_by_datetime'>
        <div class='ranges'>
            <div class='range start-range'>
                <label for='fecha_inicio'>Start datetime</label>
                <div class='datetime'>
                    <input type='date' name='fecha_inicio' id='fecha_inicio' value='2023-01-01'>
                    <input type='time' name='hora_inicio' id='hora_inicio' value='00:00'>
                </div>
            </div>
            <div class='range end-range'>
                <label for='fecha_fin'>End datetime</label>
                <div class='datetime'>
                    <input type='date' name='fecha_fin' id='fecha_fin' value='2025-01-01'>
                    <input type='time' name='hora_fin' id='hora_fin' value='00:00'>
                </div>
            </div>
            <button type='submit' class='btn-options self-centered'>Get date-ranged data</button>
        </div>
    </form>
)====";

const static char html_foot[] = R"====(
</body>
</html>
)====";

const static char html_remote_host_setup[] = R"====(
    <form action='/save_remote_host_prefs' method='GET'>
        <div class='options'>
        <input type='text' name='host' id='host' placeholder='host url'>
        <input type='text' name='port' id='port' placeholder='host port'>
            <button type='submit' class='btn-options'>Save to Flash</button>
        </div>
    </form>
)====";

const static char html_wifi_setup[] = R"====(
    <form action='/save_wifi_prefs' method='GET'>
        <div class='options'>
        <input type='text' name='ssid' id='ssid' placeholder='WiFi SSID'>
        <input type='text' name='password' id='password' placeholder='WiFi password'>
            <button type='submit' class='btn-options'>Save to Flash</button>
        </div>
    </form>
)====";

const static char html_profile_setup[] = R"====(
    <form action='/save_profile_prefs'' method='GET'>
        <div class='options'>
        <input type='text' name='node_id' id='node_id' placeholder='CN000'>
        <div class='row'>
        <input type='radio' name='profile' value='gateway'><label for="gateway">Gateway</label>
        <input type='radio' name='profile' value='node'><label for="node">Node</label>
        </div>
            <button type='submit' class='btn-options'>Save to Flash</button>
        </div>
    </form>
)====";

/* fin html */

/* webservermode */

void WebServerMode()
{

    hallSensorFlag = false;
    String html = "";
    DNSServer dnsServer;

    unsigned long start_time = millis();

    Serial.println("\nWeb Portal Active (enter in http://192.168.1.2/get_all_data to get data)");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 1, 2), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0), IPAddress(192, 168, 1, 10)); // 858909 bytes
    // WiFi.softAPConfig(node_IP, gateway_IP, subnet_mask, dhcp_lease); //858945 bytes

    WiFi.softAP("ceaza_node_" + NODEID); // cambiar por prefs
    // (IPAddress node_IP, IPAddress gateway, IPAddress subnet, IPAddress dhcp_lease_start)
    dnsServer.start(53U, "*", IPAddress(192, 168, 1, 2));
    // setup de archivos..

    WebServer server(80);
    server.begin();

    String html_data = "";
    unsigned long primera_linea;
    unsigned long ultima_linea;

    server.on("/", [&]()
              {
                ultima_linea = flash_getnextline() - 1;
                primera_linea = (ultima_linea > 48) ? ultima_linea - 48UL : 0;
                String html_data = returnLines(primera_linea, ultima_linea);
                html_output = "<pre>" + html_data + "</pre>";
                if(config.mode == 1)
                    html = String(html_head) + String(html_gateway_form) + html_output + String(html_foot);
                else
                    html = String(html_head) + String(html_node_form) + html_output + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/index.html", [&]()
              {
                ultima_linea = flash_getnextline() - 1;
                primera_linea = (ultima_linea > 48) ? ultima_linea - 48UL : 0;
                html_data = returnLines(primera_linea, ultima_linea);
                html_output = "<pre>" + html_data + "</pre>";
                html = String(html_head) + String(html_gateway_form) + html_output + String(html_foot);

                server.send(200, "text/html", html); });

    server.on("/get_all_data_old", [&]()
              {
                ultima_linea = flash_getnextline();
                html_data = returnLines(0, ultima_linea);
                server.send(200, "text/csv", html_data); });

    server.on("/get_all_data", [&]()
              {
                unsigned long ultima_linea;
                char buf[256];
                int chunk = 600;
                int limite = 65536;
                ultima_linea = flash_getnextline();
                html_data = "";

                server.setContentLength(CONTENT_LENGTH_UNKNOWN);
                server.sendHeader("Content-Type", "text/csv");
                server.sendHeader("Cache-Control", "no-cache");
                server.send(200);

                for (unsigned long q = 0; q < ultima_linea; q++)
                {
                    flash_get256(q, &buf[0]);
                    html_data += String(buf) + "\n";

                    if (q % chunk == 0 || q == (limite - 1))
                    {
                        server.sendContent(html_data);
                        html_data = "";
                    }
                }
                server.sendContent(""); });

    server.on("/get_slow_data", [&]()
              {
                server.setContentLength(CONTENT_LENGTH_UNKNOWN);
                server.sendHeader("Content-Type", "text/csv");
                server.sendHeader("Cache-Control", "no-cache");
                server.send(200);

                for (unsigned long q = 0; q < 65536; q++)
                {
                html_data = "";
                html_data +="flash[";
                html_data += q;
                html_data += "] = id,I9C7C,dt,1970-01-01T00:00:00Z,vin,12.74,at,,sh,,C,6335\n";
                server.sendContent(html_data);
                }

                server.sendContent(""); });
    server.on("/get_fake_data", [&]()
              {
                int limite = 65536;
                int chunk = 600;
                server.setContentLength(CONTENT_LENGTH_UNKNOWN);
                server.sendHeader("Content-Type", "text/csv");
                server.sendHeader("Cache-Control", "no-cache");
                server.send(200);

                for (unsigned long q = 0; q < limite; q++)
                {
                html_data +="flash[";
                html_data += q;
                html_data += "] = id,I9C7C,dt,1970-01-01T00:00:00Z,vin,12.74,at,,sh,,C,6335\n";
                if(q % 800 == 0 || q == (limite - 1)){
                server.sendContent(html_data);
                html_data = "";
                }
                }

                server.sendContent(""); });
    // String html_data = processCmd("get_fake_data"); // esta funcion corta el stream
    // Serial.println(html_data); //tira los 923

    server.on("/get_last_240", [&]()
              {
                ultima_linea = flash_getnextline() - 1UL;
                primera_linea = 0;
                if (ultima_linea > 240)
                    primera_linea = ultima_linea - 240UL;
                html_data = returnLines(primera_linea, ultima_linea);
                // html = html_data;
                // html = String(html_head) + html_output + String(html_foot);

                  server.send(200, "text/csv", html_data); });

    // server.on("/upload", [&]()
    //           {
    //                 String html = "<h1>Upload page</h1>";
    //                 server.send(200, "text/html", html); });

    server.on("/get_by_datetime", [&]()
              {
                String html = "";
                String fecha_inicio = "";
                String fecha_fin = "";
                String hora_inicio = "";
                String hora_fin = "";
                
                server.setContentLength(CONTENT_LENGTH_UNKNOWN);
                server.sendHeader("Content-Type", "text/csv");
                server.sendHeader("Cache-Control", "no-cache");

                if (server.hasArg("fecha_inicio") && server.hasArg("fecha_fin"))
                {
                    fecha_inicio = server.arg("fecha_inicio");
                    fecha_fin = server.arg("fecha_fin");
                    if (server.hasArg("hora_inicio") && server.hasArg("hora_fin")){
                        hora_inicio = server.arg("hora_inicio");
                        hora_fin = server.arg("hora_fin");
                        } else {
                        hora_inicio = "00:00:00";
                        hora_fin = "23:59:59";
                    }
                    fecha_inicio += " " + hora_inicio + ":00";
                    fecha_fin += " " + hora_fin + ":00";
                    html_data = getLinesBetweenTimestamps(fecha_inicio, fecha_fin);
                    // Serial.println("lineas: ");
                    // Serial.println(html_data);
                    html_output = html_data;
                    // html_output = "<pre>" + html_data + "</pre>";

                    html = html_output;
                    // html = html_head + html_output + html_foot;

                      server.send(200, "text/csv", html);
                  }
                  else
                  {
                      html = "<p>Error: faltan fechas de inicio y fin</p>";
                      server.send(200, "text/html", html);
                  } });

    server.on("/format", [&]()
              {
                flash_format();
                html_data = "<h1>Flash memory has been formatted</h1>";
                html_data += "<a href='/'>Back to main page</a>";
                html_output = "<pre>" + html_data + "</pre>";
                html = String(html_head) + html_output + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/setup_wifi_form", [&]()
              {
                
                String html_wifi_placeholders = "<p><span>SSID:</span><span>" + String(config.wifi_ssid) + "</span></p><p><span>password:</span><span>" + String(config.wifi_pass) + "</span></p>";
                html = String(html_head) + html_wifi_placeholders + String(html_wifi_setup) + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/save_wifi_prefs", [&]()
              {
                if (server.hasArg("ssid") && server.hasArg("password"))
                {
                    String wifi_ssid = server.arg("ssid");
                    String wifi_pass = server.arg("password");
                    strncpy(config.wifi_ssid,wifi_ssid.c_str(),wifi_ssid.length());
                    strncpy(config.wifi_pass,wifi_pass.c_str(),wifi_pass.length());

                    EEPROM.put(0, config);
                    EEPROM.commit();

                    html_output = "<p>WiFi SSID and password saved to flash.</p><a href='/'>Back to main page</a>";
                } else if (server.hasArg("ssid") && !server.hasArg("password"))
                {
                    html = "<p>Error: password has not been defined.</p>";
                    /* code */
                } else if(!server.hasArg("ssid") && server.hasArg("password")){
                    html = "<p>Error: SSID has not been defined.</p>";
                } else
                 {
                    html_output = "<p>Error: SSID and password have not been defined.</p>";
                }
                print_eeprom_config();
                html = String(html_head) + html_output + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/setup_remote_host_form", [&]()
              {
                uint16_t port = config.remote_port;
                String html_remote_host_placeholders = "<p><span>Host:</span><span>" + String(config.remote_host) + "</span></p><p><span>port:</span><span>" + String(port) + "</span></p>";
                html = String(html_head) + html_remote_host_placeholders + String(html_remote_host_setup) + String(html_foot);
                // html = String(html_head) + String(html_remote_host_setup) + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/save_remote_host_prefs", [&]()
              {
                if (server.hasArg("host") && server.hasArg("port"))
                {
                    String host_str = server.arg("host");
                    strncpy(config.remote_host,host_str.c_str(),host_str.length());
                    config.remote_port = server.arg("port").toInt();
                    EEPROM.put(0, config);
                    EEPROM.commit();
                    html_output = "<p>Remote host and port saved to flash.</p><a href='/'>Back to main page</a>";
                } else if (server.hasArg("host") && !server.hasArg("port"))
                {
                    html = "<p>Error: port has not been defined.</p>";
                    /* code */
                } else if(!server.hasArg("host") && server.hasArg("port")){
                    html = "<p>Error: host has not been defined.</p>";
                } else
                 {
                    html_output = "<p>Error: host and port have not been defined.</p>";
                }
                print_eeprom_config();

                html = String(html_head) + html_output + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/setup_profile_form", [&]()
              {
                uint8_t profile = config.mode;
                String lit_profile;
                if (profile == 1) lit_profile ="Gateway"; else lit_profile = "Node";
                String html_profile_placeholders = "<p>Node name: " + String(config.node_id) + "</p><p>Current profile: " + String(lit_profile) + "</p>";
                html = String(html_head) + html_profile_placeholders + String(html_profile_setup) + String(html_foot);
                // html = String(html_head) + String(html_profile_setup) + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/save_profile_prefs", [&]()
              {
                if (server.hasArg("profile") && server.hasArg("node_id"))
                {
                    uint8_t mode;

                    String id_str = server.arg("node_id");
                    strncpy(config.node_id,id_str.c_str(),id_str.length());

                    if (server.arg("profile") == "gateway")
                    {
                        config.mode = 1;
                        EEPROM.put(0, config);
                        EEPROM.commit();
                        html_output = "<p>Gateway profile saved to flash.</p><a href='/'>Back to main page</a>";
                    }
                    else if (server.arg("profile") == "node")
                    {
                        config.mode = 0;
                        EEPROM.put(0, config);
                        EEPROM.commit();
                        html_output = "<p>Node profile saved to flash.</p><a href='/'>Back to main page</a>";
                    } else {
                        mode = 2;
                        html_output = "<p>Error: invalid device profile.</p>";
                    }
                    print_eeprom_config();

                } else
                 {
                    html_output = "<p>Error: device profile has not been defined.</p>";
                }
                html = String(html_head) + html_output + String(html_foot);
                server.send(200, "text/html", html); });

    server.on("/restart", [&]()
              {
                html_output = "<p> Node is restarting...</p>";
                html = String(html_head) + html_output + String(html_foot);
                server.send(200, "text/html", html);
                delay(1000);
                ESP.restart(); });

    // while (millis()-start_time < 300000000UL)
    while (millis() - start_time < 300000UL) // maximo 5 minutos prendido el web server
    {
        dnsServer.processNextRequest();
        server.handleClient();
        CheckProcesaLora();
        // if (hallSensorFlag)
        // {
        //     break;
        // }
    }
    server.stop();
}

/* fin webservermode */

//-------------------------------------- wifi portal  --------------------------------

// funciones de data_process

String returnLines(unsigned long primera_linea, unsigned long end_line)
{
    String string_salida;
    string_salida = "";
    char buf[256];

    for (unsigned long q = primera_linea; q < end_line; q++)
    {
        flash_get256(q, &buf[0]);
        string_salida += String(buf) + "\n";
    }
    return string_salida;
}

String getLinesBetweenTimestamps(String &fecha_inicio, String &fecha_fin)
{
    unsigned long fecha_inicio_epoch = getEpoch(fecha_inicio);
    unsigned long fecha_fin_epoch = getEpoch(fecha_fin);
    unsigned long epoch_temp;

    Serial.println("fecha_inicio: " + fecha_inicio);
    Serial.println("fecha_fin: " + fecha_fin);

    Serial.println("fecha_inicio_epoch: " + String(fecha_inicio_epoch));
    Serial.println("fecha_fin_epoch: " + String(fecha_fin_epoch));
    // 2147483647
    if (fecha_inicio_epoch < 0 && fecha_inicio_epoch > 4294967295)
    {
        fecha_inicio_epoch = 0;
    }
    if (fecha_fin_epoch < 0 && fecha_fin_epoch > 4294967295)
    {
        fecha_fin_epoch = fecha_inicio_epoch + 86400;
    }
    if (fecha_inicio_epoch > fecha_fin_epoch)
    {
        epoch_temp = fecha_inicio_epoch;
        fecha_inicio_epoch = fecha_fin_epoch;
        fecha_fin_epoch = epoch_temp;
    }
    String string_salida;
    String buffer;
    string_salida = "";
    char buf[256];
    unsigned long ultima_linea;
    ultima_linea = flash_getnextline() - 1;

    for (unsigned long q = 0; q < ultima_linea; q++)
    {
        flash_get256(q, &buf[0]);
        buffer = String(buf);
        /* validar crc antes */
        if (buffer.indexOf("id,") != -1)
        {
            unsigned long epoch = getEpochFromLine(buffer);
            if (epoch != 0 && (epoch >= fecha_inicio_epoch && epoch <= fecha_fin_epoch))
            {
                Serial.println("\nlinea: " + buffer);
                Serial.print("epoch: ");
                Serial.print(String(epoch));
                string_salida += buffer + "\n";
            }
        }
    }
    return string_salida;
}

unsigned long getEpochFromLine(String &line)
{
    // String linea_prueba = "id,I9C7C,dt,2018-10-06T13:05:00Z,vin,12.70,at,,sh,,sw,,swt,,C,45711";

    int dt_start = line.indexOf("dt,") + 3;
    int dt_end = line.indexOf(",vin");
    if (dt_start == -1 || dt_end == -1)
    {
        return 0;
    }
    String dateTime = line.substring(int(dt_start), int(dt_end));
    dateTime.replace("T", " ");
    dateTime.replace("Z", "");
    unsigned long epoch = getEpoch(dateTime);
    // Serial.println("epoch: " + String(t));
    // epoch: 1538831100
    return epoch;
}

unsigned long getEpoch(String &datetime)
{
    // String html_datetime = "2018-10-06 13:15:00";
    struct tm tm = {0};
    time_t t;
    unsigned long epoch;

    char datetime_arr[255];
    datetime.toCharArray(datetime_arr, datetime.length() + 1);

    // Serial.println("datetime c_str: ");    Serial.println(datetime_arr);
    if (strptime(datetime_arr, "%Y-%m-%d %H:%M:00", &tm) == NULL)
    {
        // Serial.println("strptime failed, string passed: " + datetime);

        return 0;
    }

    t = mktime(&tm);

    epoch = t;
    return epoch;
}

#endif