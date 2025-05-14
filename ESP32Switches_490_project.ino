#include "WiFi.h"
#include "ESPAsyncWebServer.h"
//#include <WireGuard-ESP32.h>
//static Wireguard wg
/*char private_key[] = "yMSTmStZpZhpgnosNxQ/KovzLmuZZTxmlS7skMnKd34=";  // [Interface] PrivateKey
IPAddress local_ip(192,168,1,201);            // [Interface] Address
char public_key[] = "GwHGi1b3tt1BuGTWYJ/XYMR6Ia2SvmBLW7KMR9K2v28=";     // [Peer] PublicKey
char endpoint_address[] = "172.217.22.14";    // [Peer] Endpoint
int endpoint_port = 51820; */

// Define Relay configuration
#define RELAY_NO true  

// Define GPIO for the relay
const int relayGPIO = 26;

// Replace with your network credentials
const char* ssid = "test_wifi";
const char* password = "esp32css";

const char* PARAM_INPUT_1 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2 {
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }

  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    user-select: none;
  }
  .button:active {
    background-color: #0f8b8d;
    box-shadow: 2 2px #CDCDCD;
    transform: translateY(2px);
  }
  .state {
    font-size: 1.5rem;
    color: #8c8c8c;
    font-weight: bold;
  }
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <div class = "topnav">
    <h1>CSS Webserver</h1>
  </div>
  <div class = "content">
    <div class = "card">
        <h2>Relay 1</h2>
        <label class="switch">
            <input type="checkbox" onchange="toggleCheckbox(this)" id="relay">
            <span class="slider"></span>
        </label>
    </div>
    <div class = "card">
        <h2>Live ESPCam Stream</h2>
        <p><button id="button1" class="button">Press Here</button></p>
    </div>
    <div class = "card">
        <h2>Log Email</h2>
        <h3>Read config file</h3>
        <p><button id="button2" class="button">Press Here</button></p>
    </div>
  </div>
<script>
   document.addEventListener("DOMContentLoaded", function() {
    document.getElementById("button1").addEventListener("click", function() {
        window.location.href = "http://192.168.60.174"; // IP for ESP32 Camera
    });
    document.getElementById("button2").addEventListener("click", function() {
        window.location.href = "https://mail.google.com"; // Link to email
      });
   });
    function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if(element.checked){ xhr.open("GET", "/update?state=1", true); }
    else { xhr.open("GET", "/update?state=0", true); }
    xhr.send();
    }
</script>
</body>
</html>
)rawliteral";

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Configure relay pin
  pinMode(relayGPIO, OUTPUT);
  digitalWrite(relayGPIO, RELAY_NO ? HIGH : LOW); // Ensure relay starts in the OFF state
  Serial.println("Relay initialized to OFF");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  //Wireguard Impementation
  /*Serial.println("Adjusting system time...");
    configTime(9 * 60 * 60, 0, "ntp.jst.mfeed.ad.jp", "ntp.nict.jp", "time.google.com");

    Serial.println("Connected. Initializing WireGuard...");
    wg.begin(
        local_ip,
        private_key,
        endpoint_address,
        public_key,
        endpoint_port);
  */
  // Print ESP32 Local IP Address
  Serial.println("Connected! IP Address: " + WiFi.localIP().toString());

  // Serve web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Handle relay updates
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1)) {
      String state = request->getParam(PARAM_INPUT_1)->value();
      Serial.println("Received state: " + state);


        digitalWrite(relayGPIO, state == "1" ? LOW : HIGH); // Active-low logic


      // Print current relay state to serial monitor
      Serial.print("Relay GPIO state: ");
      Serial.println(digitalRead(relayGPIO) == LOW ? "ON" : "OFF");

      // Respond to the HTTP request
      request->send(200, "text/plain", "OK");
    } else {
      Serial.println("No state parameter received");
      request->send(400, "text/plain", "Missing state parameter");
    }
  });

  // Start server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Nothing to do here
}
