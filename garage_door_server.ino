// Adafruit IO IFTTT Door Detector
// 
// Learn Guide: https://learn.adafruit.com/using-ifttt-with-adafruit-io
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2018 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config_const.h"
#include "config_io.h"
#include "config_wifi.h"

// Load Wi-Fi library
#include <ESP8266WiFi.h>
/************************ Example Starts Here *******************************/



// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliary variables to store the current output state
String last_door_state_1 = "unknown";
String door_state_1 = "open";
int door_timeout_1 = DOOR_TIMEOUT;

void setup_server() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void setup_io() {
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(DOOR_CONTROL_PIN, OUTPUT);

  // Set outputs to LOW (initialize!!!)
  digitalWrite(DOOR_CONTROL_PIN, LOW);
}

void check_door() {
  if(digitalRead(DOOR_SENSOR_PIN) == DOOR_CLOSED) {
    Serial.println("Door closed");
    door_state_1  = "closed";
    door_timeout_1 = 0; // already closed, reset timer to 0
  }
  else {
    Serial.println("Door is open!");
    door_state_1  = "open";
  }

  // if the door_state has just now become open, last is not open,
  // then initialize the timeout
  if ((last_door_state_1 != "open") &&
      (door_state_1 == "open")) {
    door_timeout_1 = DOOR_TIMEOUT;
  }
  last_door_state_1 = door_state_1;
}

void logic_loop() {
    if (door_state_1 == "open") {
        if (door_timeout_1 == 0) {
            // timeout fired and door is not closed
            pulse_door_button();
            door_timeout_1 = DOOR_TIME_TO_CLOSE; // delay for a bit before re-attempting to close the door
        }
    }
}

// will call this every second by ISR
void update_timers(void *pArg) {
  if (door_timeout_1 > 0) {
    door_timeout_1 -= 1;
  }
}

extern "C" {
#include "user_interface.h"
}

os_timer_t myTimer;

void setup_timers(void) {
    /*
    os_timer_setfn - Define a function to be called when the timer fires

    void os_timer_setfn(
          os_timer_t *pTimer,
          os_timer_func_t *pFunction,
          void *pArg)

    Define the callback function that will be called when the timer reaches zero. The pTimer parameters is a pointer to the timer control structure.

    The pFunction parameters is a pointer to the callback function.

    The pArg parameter is a value that will be passed into the called back function. The callback function should have the signature:
    void (*functionName)(void *pArg)

    The pArg parameter is the value registered with the callback function.
    */

    os_timer_setfn(&myTimer, update_timers, NULL);

    /*
    os_timer_arm -  Enable a millisecond granularity timer.

    void os_timer_arm(
          os_timer_t *pTimer,
          uint32_t milliseconds,
          bool repeat)

    Arm a timer such that is starts ticking and fires when the clock reaches zero.

    The pTimer parameter is a pointed to a timer control structure.
    The milliseconds parameter is the duration of the timer measured in milliseconds. The repeat parameter is whether or not the timer will restart once it has reached zero.

    */

    os_timer_arm(&myTimer, 1000, true);
} // End of setup_timers

void pulse_door_button() {
  digitalWrite(DOOR_CONTROL_PIN, HIGH);
  delay(500); // long enough but not forever...
  digitalWrite(DOOR_CONTROL_PIN, LOW);
}

void serve_loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // handle the various urls/methods
            if (header.indexOf("POST /door/1/pulse") >= 0) {
              Serial.println("/door/1/pulse");
              pulse_door_button();
            } else if (header.indexOf("GET /door/1/state") >= 0) {
              check_door();
              client.println(door_state_1);

            } else if (header.indexOf("GET /door/1/timeout") >= 0) {
              client.println(String(door_timeout_1));
            } else if (header.indexOf("PUT /door/1/timeout") >= 0) {
              // TODO: implement PUT method, need to accept/coonvert args
              Serial.println("PUT /door/1/timeout is not yet implemented...");
              client.println("PUT /door/1/timeout is not yet implemented...");

            } else {

                // ///////////////////////////////////////////////////////////////////////////////////////////////////////
                // Display the HTML web page
                client.println("<!DOCTYPE html><html>");
                client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

                client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");

                client.println("<link rel=\"icon\" href=\"data:,\">");
                // CSS to style the on/off buttons
                // Feel free to change the background-color and font-size attributes to fit your preferences
                client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
                client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                client.println(".open {background-color: #77878A;}</style></head>");

                // Web Page Heading
                client.println("<body><h1>ESP8266 Web Server - Garage Door</h1>");


                // ///////////////////////////////////////////////////////////////////////////////////////////////////////

                check_door(); // i want fresh data...

                client.println("<div><table width=\"100%\"><tr><td>");
                client.println("<p>Door 1 is <span id=\"door_state_1\">"+door_state_1+"</span>; Timeout = <span id=\"door_timeout_1\">"+ door_timeout_1 +"</span></p>");
                // i want to see the status live, set a timer to update this value over and over

                client.println("</td><td>");

                client.println("<script>");
                client.println("var timeoutID;");
                client.println("function checkDoorLoop() {");
                client.println("    $.get(\"/door/1/state\", {}, function(data, status) {");
                client.println("        $(\"#door_state_1\").text(data)");
                client.println("        timeoutID = window.setTimeout(checkDoorLoop, "+String(GUI_CHECK_DOOR_MS)+");");
                client.println("    });");
                client.println("}");
                client.println("timeoutID = window.setTimeout(checkDoorLoop, "+String(GUI_CHECK_DOOR_MS)+");");
                client.println("</script>");

                client.println("</td><td>");

                client.println("<p><button id=\"door_1\" class=\"button\">DOOR 1</button></p>");
                client.println("<script>");
                client.println("$('#door_1').click(function(){");
                client.println("    $.post(\"/door/1/pulse\", {}, function(data, status) {");
                client.println("        console.log(`${data} and status is ${status}`);");
                client.println("    });");
                client.println("});");
                client.println("</script>");

                client.println("</td><td>");

                client.println("<script>");
                client.println("var timeoutID2;");
                client.println("function checkDoorTimerLoop() {");
                client.println("    $.get(\"/door/1/timeout\", {}, function(data, status) {");
                client.println("        console.log(`${data} and status is ${status}`);");
                client.println("        $(\"#door_timeout_1\").text(data)");
                client.println("        timeoutID2 = window.setTimeout(checkDoorTimerLoop, "+String(GUI_CHECK_DOOR_TIMER_MS)+");");
                client.println("    });");
                client.println("}");
                client.println("timeoutID2 = window.setTimeout(checkDoorTimerLoop, "+String(GUI_CHECK_DOOR_TIMER_MS)+");");
                client.println("</script>");

                client.println("</td></tr></table></div>");

                // ///////////////////////////////////////////////////////////////////////////////////////////////////////
                client.println("</body></html>");
            }
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
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

void setup() {

  // start the serial connection
  Serial.begin(115200);
  while (!Serial);

  setup_io();
  setup_timers();
  setup_server();
}

void loop() {
  serve_loop();

  logic_loop();

  Serial.println(".");
  delay(100);
}
