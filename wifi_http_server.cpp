/************************** Configuration ***********************************/

// edit the config.h tab and enter your configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config_const.h"
#include "config_wifi.h"

// Load Wi-Fi library
#include <ESP8266WiFi.h>
/************************ Example Starts Here *******************************/

#include "timers.h"
#include "door.h"
#include "wifi_http_server.h"
#include "door_http_connector.h"

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliary variables to store the current output state

void setup_server() {
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("~");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
    server.begin();
}

int parse_number(String s, int idx) {
    char c;
    int i = idx;
    int n = 0;
    while ((i < s.length()) && (c = s[i++])) {
        Serial.print(c);
        if ((c >= '0') && (c <= '9')) {
            n *= 10;                       // shift the existing value
            n += ((int)(c) - (int)('0'));  // add the ones digit
        } else {
            Serial.print("Not sure what to do with: '");
            Serial.print(c);
            Serial.println("'.");
            break; // no more numbers to parse
        }
    }

    return n;
}



void print_page(WiFiClient *client) {
    String string_door;

    // ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display the HTML web page
    client->println("<!DOCTYPE html><html>");
    client->println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

    client->println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");

    client->println("<link rel=\"icon\" href=\"data:,\">");
    // CSS to style the on/off buttons
    client->println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    client->println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
    client->println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
    client->println(".open {background-color: #e60c0c;}</style></head>");
    // 0ce659 green     e60c0c red

    // Web Page Heading
    client->println("<body><h1>ESP8266 Web Server - Garage Door</h1>");


    client->println("<div><table width=\"100%\">");
    client->println("<tr>");
    for (int d = 1; d <= MAX_DOORS; d++) {
        string_door = String(d);
        client->println("<td>");
        client->println("<center>");
        client->println("<table>");

        client->println("<tr><td>");
        client->println("  <p><button id=\"door_"+string_door+"\" class=\"button\">DOOR "+string_door+"</button></p>");
        client->println("</td></tr>");
        client->println("<tr><td>");
        client->println("  <p>Door "+string_door+" is <span id=\"door_state_"+string_door+"\">"+String(get_door_state(d))+"</span></p>");
        client->println("  <p>Timeout: <input type='text' id='door_timeout_"+string_door+"' value='"+ String(get_door_timeout(d)) +"'></p>");
        client->println("</td></tr>");

        client->println("</table>");
        client->println("</center>");
        client->println("</td>");
        // TODO: make the button require confirmation if the door is closed
    }
    client->println("</tr>");
    client->println("</table></div>\n");

    client->println("<script>"); //--------------------------------------- SCRIPTS -------------
    client->println("var timeoutID;");

    client->println("function TimerFired() {");
    for (int d = 1; d <= MAX_DOORS; d++) {
        string_door = String(d);
        client->println("  $.get(\"/door/"+string_door+"/state\", {}, function(data, status) {");
        client->println("    $(\"#door_state_"+string_door+"\").text(data)");
        client->println("  });");
        client->println("  $.get(\"/door/"+string_door+"/timeout\", {}, function(data, status) {");
        client->println("    console.log(`${data} and status is ${status}`);");
        client->println("    $(\"#door_timeout_"+string_door+"\").text(data)");
        client->println("  });");
    }
    client->println("  timeoutID = window.setTimeout(TimerFired, "+String(GUI_CHECK_DOOR_MS)+");");
    client->println("}");

    for (int d = 1; d <= MAX_DOORS; d++) {
        string_door = String(d);
        client->println("$('#door_"+string_door+"').click(function(){");
        client->println("  $.post(\"/door/"+string_door+"/pulse\", {}, function(data, status) {");
        client->println("    console.log(`${data} and status is ${status}`);");
        client->println("  });");
        client->println("});");
    }

    client->println("timeoutID = window.setTimeout(TimerFired, "+String(GUI_CHECK_DOOR_MS)+");");
    client->println("</script>"); //-------------------------------------- SCRIPTS -------------

    client->println("</body></html>");
}

void parse_request(WiFiClient *client) {
    String body = "";
    int i = 0;
    int n = 0;
    char c;

    // uses global String header

    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client->println("HTTP/1.1 200 OK");
    client->println("Content-type:text/html");
    client->println("Connection: close");
    client->println();

    //                  1234567890123456789  -> 16 char
    i = header.indexOf("Content-Length: "); // where to find the numbers
    if (i >= 0) { // Content-Length found
        // interpret the content length
        n = parse_number(header, i + 16);   // 16 characters... where the numbers start

        if (n > 0) {
            for (i = 0; i<n; i++) {
                char c = client->read();
                Serial.write(c);            // print it out the serial monitor
                body += c;
            }
        }
    }

    // handle the various urls/methods
    /////////////////////////////////////////////////////////////////////////////////////////

    // DOOR.toggle
    if (header.indexOf("POST /door/1/pulse") >= 0) {
        pulse_door_relay(1);
    } else if (header.indexOf("POST /door/2/pulse") >= 0) {
        pulse_door_relay(2);

    // DOOR.state
    } else if (header.indexOf("GET /door/1/state") >= 0) {
        client->println(String(get_door_state(1)));
    } else if (header.indexOf("GET /door/2/state") >= 0) {
        client->println(String(get_door_state(2)));

    // DOOR.state
    } else if (header.indexOf("GET /door/1/timeout") >= 0) {
        client->println(String(get_door_timeout(1)));
    } else if (header.indexOf("GET /door/2/timeout") >= 0) {
        client->println(String(get_door_timeout(2)));

    } else if ((header.indexOf("PUT /door/") >= 0) && (header.indexOf("/timeout") >= 0)) {
        // the whole 'body' is a number
        n = parse_number(body, 0);

        if (header.indexOf("PUT /door/1/timeout") >= 0) {
            i = 1; // door 1
        } else if (header.indexOf("PUT /door/2/timeout") >= 0) {
            i = 2; // door 2
        } else {
            i = -1;
            Serial.println("unknown door...");
        }

        Serial.print("set_door_timeout(");
        Serial.print(i);
        Serial.print(", ");
        Serial.print(n);
        Serial.println(")");
        if (i > 0) {
            set_door_timeout(i, n);
        }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // main page :-)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    } else {
        print_page(client); // this client is really already a pointer
    }

    // The HTTP response ends with another blank line
    client->println();
}

void serve_loop() {
    WiFiClient client = server.available(); // Listen for incoming clients
    int bail = 100; // in lieu of a real timer, in case we get stuck

    if (client) {                           // If a new client connects,
        Serial.println("New Client.");      // print a message out in the serial port
        String currentLine = "";            // make a String to hold incoming data from the client
        while (client.connected()) {        // loop while the client's connected through each character
            if (client.available()) {       // if there's bytes to read from the client,
                char c = client.read();     // read a byte, then
                Serial.write(c);            // print it out the serial monitor
                header += c;
                if (c == '\n') {            // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0) { // END of HTTP headers found (blank line)
                        parse_request(&client);
                        break;
                    } else { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                } else if (c != '\r') {    // if you got anything else but a carriage return character,
                    currentLine += c;            // add it to the end of the currentLine
                }
            } else {
                Serial.println("How long do we idle while waiting for the client to be available?");
                if (--bail <= 0) {
                    break;
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}
