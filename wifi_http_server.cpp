/************************** Configuration ***********************************/

// edit the config.h tab and enter your configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config_const.h"
#include "config_wifi.h"
#include "config_twilio.h" // for the incorporation of telephone menus

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
/************************ Example Starts Here *******************************/

#include "timers.h"
#include "door.h"
#include "wifi_http_server.h"
#include "door_http_connector.h"

// Set web server port number to 80
// WiFiServer server(80);
ESP8266WebServer server(80);

const int led = 0; // blue wifi light

void handleRoot() {
    digitalWrite(led, 1);
    String html = "";
    String string_door = "";

    /*

    You are here:
    - need to incorporate the response of the overall state requests into the angular model... translate data into action
    - allow override to the door operating mode (master state) (drop-down selection w/on_change PUT master_state=new value

    */

    html += "<!DOCTYPE html>\n<html>\n";
    html += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    html += "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>\n";
    //html += "<link rel=\"icon\" href=\"data:,\">\n";
    html += "<style>\nhtml { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    html += ".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;\n";
    html += "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\n";
    html += ".open {background-color: #e60c0c;}</style></head>\n";
    html += "<body><h1>ESP8266 Web Server - Garage Door</h1>\n";
    html += "<div><table width=\"100%\">\n";
    html += "<tr>\n";
    for (int d = 1; d <= MAX_DOORS; d++) {
        string_door = String(d);
        html += "<td>";
        html += "<center>";
        html += "<table>";

        html += "<tr><td>";
        html += "  <p><button id=\"door_"+string_door+"\" class=\"button\">DOOR "+string_door+"</button></p>";
        html += "</td></tr>";
        html += "<tr><td>";
        html += "  <p>Door "+string_door+" is ";
        // html += "<span id=\"door_master_state_"+string_door+"\">"+String(get_door_master_state(d))+"</span> ";

        html += "<select id='door_master_state_"+string_door+"'>";
        html += "  <option>NORMAL</option>";
        html += "  <option>AUTO_SHUT</option>";
        html += "  <option>DISABLED</option>";
        html += "</select>";

        html += "[<span id=\"door_state_"+string_door+"\">"+String(get_door_state(d))+"</span>]";
        html += "</p>";
        html += "  <p>Timeout: <input type='text' id='door_timeout_"+string_door+"' value='"+ String(get_door_timeout(d)) +"'></p>";
        html += "</td></tr>";

        html += "</table>";
        html += "</center>";
        html += "</td>\n";
        // TODO: make the button require confirmation if the door is closed
    }
    html += "</tr>";
    html += "</table></div>\n";

    html += "<script>\n"; //--------------------------------------- SCRIPTS -------------
    html += "var timeoutID;\n";

    html += "function parseDoorStatus(data, status) {\n";
    for (int d = 1; d <= MAX_DOORS; d++) {
        html += "  $('#door_master_state_"+String(d)+"').val(data["+String(d)+"].master_state);\n";
        html += "  $('#door_state_"+String(d)+"').html(data["+String(d)+"].state);\n";
        html += "  if (! ($('#door_timeout_"+String(d)+"').is(':focus'))) {\n";
        html += "    $('#door_timeout_"+String(d)+"').val(data["+String(d)+"].timeout);\n"; // .val because it's an input
        html += "  }\n";
    }
    html += "}\n";

    html += "function TimerFired() {\n";
    html += "  console.log(\"TimerFired\");\n";
    html += "  $.get(\"/doors\", parseDoorStatus);\n";
    html += "  timeoutID = window.setTimeout(TimerFired, "+String(GUI_CHECK_DOOR_MS)+");\n";
    html += "}\n";

    for (int d = 1; d <= MAX_DOORS; d++) {
        string_door = String(d);
        html += "$('#door_"+string_door+"').click(function(){\n";
        html += "  $.post(\"/door/"+string_door+"/pulse\", {}, parseDoorStatus);\n";
        html += "});\n";

        html += "$('#door_timeout_"+string_door+"').change(function(){\n";
        html += "  $.ajax({url:\"/door/"+string_door+"\", type:'PUT', data: \"timeout=\"+$( this ).val(), success: parseDoorStatus});\n";
        html += "});\n";

        html += "$('#door_master_state_"+string_door+"').change(function(){\n";
        html += "  console.log($(this)[0].selectedOptions[0].value); ";
        html += "  $.ajax({url:\"/door/"+string_door+"\", type:'PUT', data: \"master_state=\"+ $(this)[0].selectedOptions[0].value, success: parseDoorStatus});\n";
        html += "});\n";
    }

    html += "timeoutID = window.setTimeout(TimerFired, "+String(GUI_CHECK_DOOR_MS)+");\n";
    html += "</script>\n"; //-------------------------------------- SCRIPTS -------------

    html += "</body></html>\n";

    server.send(200, "text/html", html);
    digitalWrite(led, 0);
}

bool is_digit(char c) {
    return ((c >= '0') && (c <= '9'));
}

int parse_int(String s, int idx) {
    char c;
    int i = idx;
    int n = 0;
    int sign = 0;
    while ((i < s.length()) && (c = s[i++])) {
        Serial.print(c);
        if (is_digit(c)) {
            n *= 10;                       // shift the existing value
            n += ((int)(c) - (int)('0'));  // add the ones digit
            if (sign == 0) {
               sign = 1;
            }
        } else {
            if ((sign == 0) && (c == '-')) {
                sign = -1;
            } else {
                Serial.print("Not sure what to do with: '");
                Serial.print(c);
                Serial.println("'.");
                break; // no more numbers to parse
            }
        }
    }

    return sign * n;
}

String get_json_door_data(String message) {
    String json_data = "";

    String comma = "";

    // which door?
    json_data = "{";
    for (int d = 1; d <= MAX_DOORS; d++) {
        json_data += comma;
        comma = ",";
        json_data += "\"" + String(d) + "\"" + ": {";
        json_data += "\"master_state\": \"" + String(get_door_master_state(d)) +"\"";
        json_data += ", \"state\": \"" + String(get_door_state(d)) + "\"";
        json_data += ", \"timeout\": " + String(get_door_timeout(d)) + "";
        json_data += "}";
    }

    json_data += "}";

    /*
    data = { "1": {"state" : "blah", "timeout": 123456},
             "2": {"state" : "blah", "timeout": 123456} }
     */

    return json_data;
}


void handleDoorData() {
    String message = "handleDoorData\n\n";

    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "not GET";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    String json_data = get_json_door_data(message);
    server.send(200, "text/json", json_data);
}

void handleDoorAttributes() {
    String json_data = "";
    String message = "handleDoorAttributes\n\n";

    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_PUT) ? "PUT" : "not PUT";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    if (server.method() != HTTP_PUT) {
        message += "\nOnly HTTP PUT supported for this uri";
        server.send(403, "text/plain", message);
    } else { // HTTP_PUT
        // good method
        String uri = server.uri();
        String which_door_str;
        String attribute;
        int i = 0;
        int which_door = -1;
        bool good_uri = false;
        char c;

        if (uri.indexOf("/door/") == 0) {
            // /door/
            which_door_str = "";
            // {number}
            i = 6;
            while ((i < uri.length()) && (c = uri[i++])) {
                if (is_digit(c)) {
                    which_door_str += c;
                    good_uri = true;
                } else {
                    good_uri = false;
                    break;
                }
            }
        }

        if (good_uri) {
            // good door ?
            which_door = parse_int(which_door_str, 0);
            good_uri = (0 < which_door < MAX_DOORS);
        }

        // good uri?
        if (good_uri) {
            String argName;
            String argValue;
            int intValue;

            // for each argument (should be attribute)
            // for which_door; PUTting the argName to argValue
            for (uint8_t i = 0; i < server.args(); i++) {
                argName = server.argName(i);
                argValue = server.arg(i);
                if (argName == "timeout") {
                    intValue = parse_int(argValue, 0);
                    // --------------------------------------------------------
                    Serial.print("HTTP -> set_door_timeout(");
                    Serial.print(which_door);
                    Serial.print(", ");
                    Serial.print(intValue);
                    Serial.println(")");
                    set_door_timeout(which_door, intValue);
                    // --------------------------------------------------------
                } else if (argName == "master_state") {
                    // --------------------------------------------------------
                    Serial.print("HTTP -> set_door_master_state(");
                    Serial.print(which_door);
                    Serial.print(", \"");
                    Serial.print(argValue);
                    Serial.println("\")");
                    set_door_master_state(which_door, argValue);
                    // --------------------------------------------------------
                } else {
                    message += "Unhandled argument: " + argName + "\n";
                }
            }

            json_data = get_json_door_data(message);
            server.send(200, "text/json", json_data);

        } else {
            message += "\nUnknown /door/# reference";
            server.send(404, "text/plain", message);
        }
    }
}

void handlePulse() {
    String json_data = "";
    String message = "handlePulse\n\n";

    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_POST) ? "POST" : "not POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    if (server.method() != HTTP_POST) {
        message += "\nOnly HTTP POST supported for this uri";
        server.send(403, "text/plain", message);
    } else { // HTTP_POST
        // good method
        String uri = server.uri();
        String which_door_str;
        String attribute;
        int i = 0;
        int which_door = -1;
        bool good_uri = false;
        char c;

        if (uri.indexOf("/door/") == 0) {
            // /door/
            which_door_str = "";
            // {number}
            i = 6;
            while ((i < uri.length()) && (c = uri[i++])) {
                if (is_digit(c)) {
                    which_door_str += c;
                    good_uri = true;
                }
            }
        }

        if (good_uri) {
            // good door ?
            which_door = parse_int(which_door_str, 0);
            good_uri = (0 < which_door < MAX_DOORS);
        }

        // good uri?
        if (good_uri) {
            pulse_door_relay(which_door);

            json_data = get_json_door_data(message);
            server.send(200, "text/json", json_data);

        } else {
            message += "\nUnknown /door/# reference";
            server.send(404, "text/plain", message);
        }
    }
}


void handleNotFound() {
  String message = "File Not Found\n\n";

  server.send(404, "text/plain", message);
}


/******************************************************/
// TWILIO wrappers to give "server" argument

/******************************************************/

bool is_twilio_allowed_number() {
    // check the headers and body for the details about the call that we expect
    bool is_ok = true;
    bool is_allowed = false;

    String allowed_callers[] = {
        ALLOWED_1,
        ALLOWED_2,
        ALLOWED_3
    };
    const int num_allowed_callers = 3;

    if (server.method() != HTTP_POST) {
        is_ok = false;
    }

    if (! ((server.hasArg("AccountSid")) && (server.arg("AccountSid") == TWILIO_ACCOUNT_SID))) {
        is_ok = false;
    }

    if (! ((server.hasArg("Called")) && (server.arg("Called") == TWILIO_PH_NO))) {
        is_ok = false;
    }

    if (! ((server.hasArg("To")) && (server.arg("To") == TWILIO_PH_NO))) {
        is_ok = false;
    }


    if (server.hasArg("Caller")) {
        String caller = server.arg("Caller");
        for (int i = 0; i < num_allowed_callers; i++) {
            Serial.print(caller);
            Serial.print(" ?== ");
            Serial.print(allowed_callers[i]);
            if ( caller == allowed_callers[i] ) {
                is_allowed = true;
                Serial.println(" ...yup!");
                break;
            }
            Serial.println(" ...nope :-( ");
        }
    }

    is_ok = is_ok && is_allowed;

    // /*
    String message = "is_twilio_allowed_number\n";

    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_POST) ? "POST" : "not POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    message += "Headers:";
    message += server.headers();
    message += "\n";

    for (uint8_t i = 0; i < server.headers(); i++) {
        message += " " + server.headerName(i) + ": " + server.header(i) + "\n";
    }

    Serial.println(message);

    // */

    return is_ok;
}

void handleTwilioCallFilter() {
    if (is_twilio_allowed_number()) {
        Serial.println("call accepted");
        server.send(200, "text/xml", TWILIO_XML_REDIRECT_TO_FLOW);
    } else {
        Serial.println("call rejected");
        server.send(200, "text/xml", TWILIO_XML_REJECT);
    }
}

void handleTwilioDoor1() {
    int which_door = 1;
    if (is_twilio_allowed_number()) {
        pulse_door_relay(which_door);
        Serial.println("OK");
        server.send(200, "text/plain", "OK");
    } else {
        Serial.println("reject");
        server.send(200, "text/xml", TWILIO_XML_REJECT);
    }
}

void handleTwilioDoor2() {
    int which_door = 2;
    if (is_twilio_allowed_number()) {
        pulse_door_relay(which_door);
        Serial.println("OK");
        server.send(200, "text/plain", "OK");
    } else {
        Serial.println("reject");
        server.send(200, "text/xml", TWILIO_XML_REJECT);
    }
}

/******************************************************/

void setup_server() {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
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

    // register handlers
    server.on("/", handleRoot);
    server.on("/doors", handleDoorData);
    server.on("/door/1", handleDoorAttributes);
    server.on("/door/2", handleDoorAttributes);
    server.on("/door/1/pulse", handlePulse);
    server.on("/door/2/pulse", handlePulse);
    server.onNotFound(handleNotFound);

    // TWILIO handlers
    server.on(TWILIO_URI_CALL_FILTER, handleTwilioCallFilter);
    server.on(TWILIO_URI_DOOR_1,      handleTwilioDoor1);
    server.on(TWILIO_URI_DOOR_2,      handleTwilioDoor2);

    server.begin();
}

void serve_loop() {
    server.handleClient();
}

