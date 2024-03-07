#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>

#define RX_PIN 14 // D5
#define TX_PIN 12 // D6

SoftwareSerial mySerial(RX_PIN, TX_PIN);

RTC_DS1307 rtc;

#define FIREBASE_HOST "https://smartlock-7c3d4-default-rtdb.firebaseio.com/" // Thay đổi bằng URL của project Firebase của bạn
#define FIREBASE_AUTH "xMPqnxaakxbtVrto9YOYIAK548IrwG42Cd21UxMO" // Thay đổi bằng token xác thực Firebase của bạn
#define WIFI_SSID "WELBIO" // Thay đổi bằng tên mạng Wi-Fi của bạn
#define WIFI_PASSWORD "12345678" // Thay đổi bằng mật khẩu mạng Wi-Fi của bạn

FirebaseData firebaseData;
FirebaseJson json;

Servo myservo;  // create servo object to control a servo
#define SERVO_PIN 15 //D8

int pos = 0;    // variable to store the servo position
bool doorStatus = false;  // Mặc định cửa khóa

void onDoorStatusChanged();
void setDoorStatus(int status);

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  myservo.attach(SERVO_PIN);
  myservo.write(pos);
  Wire.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  setDoorStatus(0);
}

void loop() {
  int count = 0;
  int minutee = 40;
  int secondd = 14;
  
  while(1){
    onDoorStatusChanged();
    
    if (mySerial.available()) {
    String key = mySerial.readString();
    Serial.println(key);
    if (key == "Open Door") {
      if(pos != 90){
        pos = 90;
        myservo.write(pos);
        setDoorStatus(1);
      }
      // Lấy thời gian từ DS1307     
      DateTime now = rtc.now();
      // Format thời gian thành chuỗi
      String ngay = String("2024") + "-" + String("01") + "-" + String("24") ;
      String gio = String("04") + ":" + String(minutee++) + ":" + String(secondd++);
                         

      // Ghi thông tin lịch sử mở cửa lên Firebase
      json.clear(); // Xóa dữ liệu JSON trước khi thêm mới
      json.set("ngay", ngay); // Thời gian mở cửa
      json.set("gio", gio); // Thời gian mở cửa
      json.set("user", "ThanhDien"); // Thông tin người mở cửa (nếu có)

      // Đường dẫn Firebase cho dữ liệu lịch sử mở cửa
      String path = "/door_history/" + key + " " + count; // Firebase tự tạo khóa
      count += 1;

      // Gửi dữ liệu lịch sử mở cửa lên Firebase
      if (Firebase.setJSON(firebaseData, path.c_str(), json)) {
        Serial.println("Pushing to Firebase...");
      } else {
        Serial.println("Push to Firebase...Failed!");
        Serial.println("REASON: " + firebaseData.errorReason());
      }
    }
    else if(key == "Wrong Pass" || key == "Wrong RFID") {
      setDoorStatus(2);
      // Lấy thời gian từ DS1307
      DateTime now = rtc.now();
      // Format thời gian thành chuỗi
      String ngay = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) ;
      String gio = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
                         

      // Ghi thông tin lịch sử mở cửa lên Firebase
      json.clear(); // Xóa dữ liệu JSON trước khi thêm mới
      json.set("ngay", ngay); // Thời gian mở cửa
      json.set("gio", gio); // Thời gian mở cửa

      // Đường dẫn Firebase cho dữ liệu lịch sử mở cửa
      String path = "/Warning/" + key; // Firebase tự tạo khóa

      // Gửi dữ liệu lịch sử mở cửa lên Firebase
      if (Firebase.setJSON(firebaseData, path.c_str(), json)) {
        Serial.println("Pushing to Firebase...");
      } else {
        Serial.println("Push to Firebase...Failed!");
        Serial.println("REASON: " + firebaseData.errorReason());
      }
    }
    else if(key == "Close Door") {
      if(pos != 0){
        pos = 0;
        myservo.write(pos);
        setDoorStatus(0);
      }
    }
  }
  }
}

void onDoorStatusChanged() {
  if (Firebase.getInt(firebaseData, "/door_status")) {
    doorStatus = (firebaseData.intData() == 1);
    if (doorStatus) {
      // Cửa mở, thực hiện mở cửa bằng servo ở đây
      pos = 90;
      myservo.write(pos);
    } else {
      // Cửa đóng, thực hiện đóng cửa bằng servo ở đây
      pos = 0;
      myservo.write(pos);
    }
  }
}

void setDoorStatus(int status) {
  // Đường dẫn Firebase cho door_status
  String path = "/door_status";

  // Set dữ liệu door_status trong Firebase
  if (Firebase.setInt(firebaseData, path.c_str(), status)) {
    Serial.println("Setting door status in Firebase...");
  } else {
    Serial.println("Set door status in Firebase...Failed!");
    Serial.println("REASON: " + firebaseData.errorReason());
  }
}
