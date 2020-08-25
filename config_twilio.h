/******************************* TWILIO **************************************/

#define TWILIO_URI_CALL_FILTER "/twilio/call/filter"

#define TWILIO_PH_NO ""
#define TWILIO_X_SIGNATURE "X-Twilio-Signature"
#define TWILIO_ACCOUNT_SID "AC5ff1ba9c7db4fd68118f407b1927ba27"

#define ALLOWED_1 "13198005696" /* Lucas */
#define ALLOWED_2 "13198005697" /* Anika*/
#define ALLOWED_3 "13195409753" /* Dad*/

#define TWILIO_FLOW_URL "https://webhooks.twilio.com/v1/Accounts/AC5ff1ba9c7db4fd68118f407b1927ba27/Flows/FW59e2fdd1eda72fcd24778f12815191c0"

#define TWILIO_URI_DOOR_1 "/twilio/menu/door/1"
#define TWILIO_URI_DOOR_2 "/twilio/menu/door/2"

String allowed_callers[] = [
        "13198005696", /* Lucas */
        "13198005697", /* Anika*/
        "13195409753"  /* Dad*/
    ];