#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <time.h>

// Wi-Fi credentials
const char* ssid = "test_wifi";
const char* password = "esp32css";

// Email setup (SMTP server and credentials)
SMTPSession smtp;
ESP_Mail_Session mailSession;  
const char* smtpServer = "smtp.gmail.com";
const int smtpPort = 465;
const char* emailSender = "example1@gmail.com";
const char* emailPassword = "okcmlmdwbbqqzsag";  // App password
const char* emailRecipient = "example2@gmail.com";

// Ultrasonic sensor setup
const int trigPin = 5;
const int echoPin = 18;
long duration;
int distance;

//Time setup
const char* ntpServer = "time.google.com";
const long gmtOffset_sec = -8*3600;//-8*3600
const int daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Configure the email session settings
  mailSession.server.host_name = smtpServer;
  mailSession.server.port = smtpPort;
  mailSession.login.email = emailSender;
  mailSession.login.password = emailPassword;
  mailSession.login.user_domain = "";

  //configure time objects
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  smtp.callback(smtpCallback);  // Get status of email sending
  Serial.println("Setup complete. Ready to measure distance.");
}

void loop() {
  // Measure distance using ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;  //cm

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // If an object is detected within 10 cm, send an email alert
  if (distance < 50) {
    sendEmailAlert();
    delay(120000);  // Wait 30 seconds before checking again
  }

  delay(1000);  
}

void sendEmailAlert() {
  
  struct tm timeInfo; //create timeinfo structure
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char timeStamp[64];
  strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", &timeInfo); //set timestamp with proper format

  SMTP_Message message;

  // Configure the email content
  message.sender.name = "ESP32 Alert";
  message.sender.email = emailSender;
  message.subject = "Ultrasonic Sensor Alert!";
  message.addRecipient("Recipient", emailRecipient);

  String emailContent = "Warning: Intrusion detected at window 1!\n";
  emailContent += "Time of detection: ";
  emailContent += timeStamp;

  message.text.content = emailContent.c_str();

  // Send the email
  if (!smtp.connect(&mailSession)) {
    Serial.println("Failed to connect to SMTP server.");
    return;
  }

  if (MailClient.sendMail(&smtp, &message)) {
    Serial.println("Email sent successfully!");
  } else {
    Serial.println("Error sending email: " + smtp.errorReason());
  }

  smtp.closeSession();  // Close the SMTP session
}

// Callback function to monitor the email status
void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
}
