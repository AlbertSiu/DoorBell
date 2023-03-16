#include <Arduino.h>

#include <WiFi.h>
//#include <WiFiMulti.h>
#include <WebServer.h>
//#include <HTTPClient.h>
//#include <HTTPUpdate.h>
#include <Update.h> 

//WiFiMulti WiFiMulti;
WebServer server(80);

void(* resetFunc) (void) = 0;//declare reset function at address 0
long wake_up_time=0;

String indexhtml = String("") +
                   "<!DOCTYPE html>\n" +
                   "<head>\n" +
                   "    <meta charset=\"UTF-8\">\n" +
                   "    <title>Update Test</title>\n" +
                   "</head>\n" +
                   "<body>\n" +
                   "    <form method=\'POST\' action=\'/update\' enctype=\'multipart/form-data\'>\n" +
                   "        <input type=\'file\' name=\'firmware\'>\n" +
                   "        <input type=\'submit\'>\n" +
                   "    </form>\n" +
                   "</body>\n";

bool shouldreboot = false;

bool connect_before=false;

const int BUTTON_PIN = 4;   // Input pin for the pushbutton
const int BUZZER_PIN = 12;   // Output pin for the buzzer
bool isPlaying = false;     // Flag to check if the melody is already playing
#define LEDC_CHANNEL_0     0

void handleResponse() //回调函数
{
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
}

void handleFileupload() //回调函数
{
    HTTPUpload &upload = server.upload();   // 文件上传对象
    if (upload.status == UPLOAD_FILE_START) // 文件上传开始
    {
        Serial.printf("开始上传文件: %s\n", upload.filename.c_str());
        if (!Update.begin()) // 开始固件升级，检查空可用间大小，如果正在升级或空间不够则返回false
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) // 文件读写中
    {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) // 将文件数据写入到OTA区域
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) // 文件上传完成
    {
        Serial.println("写入" + String(upload.totalSize) + "字节到OTA区域");
        if (!Update.end(true)) // 完成数据写入，设置在系统重启后自动将OTA区域固件移动到Sketch区域 // Update.begin不指定大小时这里设置true
        {
            Update.printError(Serial);
        }
        Serial.println("升级操作完成，模块将在5秒后重启以完成固件升级");
        shouldreboot = true;
    }
    else
    {
        Serial.printf("固件上传失败: status=%d\n", upload.status);
    }
}


void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
 // pinMode(BUZZER_PIN, OUTPUT);
  pinMode(13,INPUT);
  ledcSetup(LEDC_CHANNEL_0, 5000, 12);
  ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL_0);
  ledcWriteTone(LEDC_CHANNEL_0, 0);

    Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin("AlbertKitty", "longlong");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", indexhtml); // 发送网页
    });

    server.on("/update", HTTP_POST, handleResponse, handleFileupload); // 绑定回调函数

    server.begin(); //启动服务器

    Serial.println("Web server started");

}

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}


void loop() {
  delay(1);
  wake_up_time++;
  if (wake_up_time>7200000) shouldreboot=true;
  
  if (digitalRead(BUTTON_PIN) == LOW && !isPlaying) {
    wake_up_time=0;
    delay(2);
    if (digitalRead(BUTTON_PIN) == LOW && !isPlaying) wake_up_time++;
    delay(2);
    if (digitalRead(BUTTON_PIN) == LOW && !isPlaying) wake_up_time++;
    if (wake_up_time>=2)  playDoorbellOnce();    // Play the doorbell melody once
    
  }

  //*************************
   server.handleClient(); //处理来自客户端的请求

    if (shouldreboot)
    {
        delay(5000);
        ESP.restart(); // 重启设备
    }
}

void playDoorbellOnce() {
  isPlaying = true;
  // Doorbell melody notes
  int melody[] = { 261, 294, 330, 294, 330, 392, 330, 392, 440, 392, 440, 523, 440, 523, 587, 523, 587, 659, 587, 659, 784 };
  //int melody[] = { 261, 294, 330 };

  // Doorbell melody duration
  int duration[] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };

  // Play each note in the melody for the corresponding duration
  for (int i = 0; i < sizeof(melody)/sizeof(int); i++) {
    //tone(BUZZER_PIN, melody[i], duration[i] * 200);   // Multiply duration by 200ms to convert to milliseconds
  ledcWriteTone(LEDC_CHANNEL_0,  melody[i]);
    delay(duration[i] * 130);   // Wait for the tone to finish playing
   // noTone(BUZZER_PIN);   // Stop playing the tone
   ledcWriteTone(LEDC_CHANNEL_0, 0);
    delay(50);   // Small delay between notes
  }
  isPlaying = false;
}
