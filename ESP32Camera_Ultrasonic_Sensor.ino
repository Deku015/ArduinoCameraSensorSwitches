#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <ESP_Mail_Client.h>
#include <FS.h>
#include <WiFi.h>
#include "soc/rtc_cntl_reg.h"  

#define FORMAT_LITTLEFS_IF_FAILED true

//wifi
const char* ssid = "test_wifi";
const char* password = "esp32css";

//Time setup
const char* ntpServer = "time.google.com";
const long gmtOffset_sec = -8*3600;//-8*3600
const int daylightOffset_sec = 3600;

//SMTP config and email config
#define emailSenderAccount    "example@gmail.com"
#define emailSenderPassword   "....."
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "ESP32-CAM Intrusion Captured"
#define emailRecipient        "exmple@gmail.com"

#define CAMERA_MODEL_AI_THINKER

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

// Photo File Name to save in LittleFS
#define FILE_PHOTO "photo.jpg"
#define FILE_PHOTO_PATH "/photo.jpg"

// Ultrasonic sensor config
#define TRIG_PIN 13
#define ECHO_PIN 14

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  pinMode(TRIG_PIN, OUTPUT); //config pins for sensor
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(115200);
  Serial.println();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  // Init filesystem
  ESP_MAIL_DEFAULT_FLASH_FS.begin();
   
  //camera pin config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  //frame size type
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
}

long measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 50000); // 50s timeout
    if (duration == 0) {
        return -1; // failed reading
    }
    long distance = duration * 0.034 / 2; // Convert to cm
    return distance;
}

void loop() {
    long distance = measureDistance();
    if (distance > 0) { // Check for valid reading
        Serial.print("Distance: ");
        Serial.println(distance);
        
        if (distance < 50) { 
            Serial.println("Object detected. Capturing photo...");
            capturePhotoSaveLittleFS();
            sendPhoto();
            delay(300000); // Wait for 5 minutes before next capture
        }
    } else {
        Serial.println("Invalid distance reading");
    }
    delay(1000); 
}


// Capture photo and save it to LittleFS
void capturePhotoSaveLittleFS( void ) {
  // Dispose first pictures because of bad quality
  camera_fb_t* fb = NULL;
  // Skip first 3 frames 
  for (int i = 0; i < 3; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }
    
  // Take photo
  fb = NULL;  
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }  

  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

  // Insert the data 
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO_PATH);
    Serial.print(" - Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
  }
  // Close the file
  file.close();
  esp_camera_fb_return(fb);
}

void sendPhoto( void ) {
  
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  char timeStamp[64];
  strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", &timeInfo); //set timestamp with proper format

  smtp.debug(1);
  //get sending results
  smtp.callback(smtpCallback);

  Session_Config config;
  
// Configure NTP for Los Angeles time
 // config.time.ntp_server = F("time.google.com");
  //config.time.gmt_offset = -8;  // Pacific Standard Time (UTC-8)
  //config.time.day_light_offset = 1; // Daylight Saving Time (add 1 hour)


  //config SMTP
  config.server.host_name = smtpServer;
  config.server.port = smtpServerPort;
  config.login.email = emailSenderAccount;
  config.login.password = emailSenderPassword;
  config.login.user_domain = "";

  SMTP_Message message;

  //Enable the chunked data transfer
  message.enable.chunking = true;
  message.sender.name = "ESP32-CAM";
  message.sender.email = emailSenderAccount;

  message.subject = emailSubject;
  message.addRecipient("Your_Name", emailRecipient);

  String htmlMsg = "Photo captured with ESP32-CAM and attached in this email @ ";
  htmlMsg += timeStamp;
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  // attachment class
  SMTP_Attachment att;

  /** Set the attachment info:
   * file name, MIME type, file path, file storage type,
   * transfer encoding and content encoding
  */
  att.descr.filename = FILE_PHOTO;
  att.descr.mime = "image/png"; 
  att.file.path = FILE_PHOTO_PATH;
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  // Add attachment 
  message.addAttachment(att);

  // Connect to server with config 
  if (!smtp.connect(&config)){
    Serial.println("SMTP connection failed, restarting...");
    delay(2000);
    ESP.restart();
  }
  // Start sending the Email
  if (!MailClient.sendMail(&smtp, &message, true)){
    Serial.println("Error sending Email, " + smtp.errorReason());
    delay(2000);
    ESP.restart();
  }
}

// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status){

  Serial.println(status.info());

  if (status.success())
  {
    Serial.println("----------------");
    Serial.printf("Message sent success: %d\n", status.completedCount());
    Serial.printf("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
   // struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      //time_t ts = (time_t)result.timestamp;
      //localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
     // ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

   // clear for memory
   smtp.sendingResult.clear();
   ESP.restart();
  }
}