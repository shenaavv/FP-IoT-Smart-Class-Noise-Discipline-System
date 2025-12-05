#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= KONFIGURASI LAYAR OLED (HARDWARE SPI) =================
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

// Wiring OLED (KEMBALIKAN KE POSISI STANDAR):
// PENTING: Hardware SPI hanya bekerja di pin 18 & 23!
// D0 (CLK)  -> GPIO 18 (WAJIB)
// D1 (MOSI) -> GPIO 23 (WAJIB)
// RES       -> GPIO 19
// DC        -> GPIO 4
// CS        -> GPIO 5
#define OLED_DC     4
#define OLED_CS     5
#define OLED_RESET  19

// Perhatikan bedanya: Kita pakai '&SPI' di sini agar ESP32 pakai jalur ngebut.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ================= KONFIGURASI PIN LAINNYA =================
const int pinTrafficRed    = 13; // Lampu Merah
const int pinTrafficYellow = 12; // Lampu Kuning
const int pinTrafficGreen  = 14; // Lampu Hijau

const int pinMic = 27;     // Sensor Suara MAX4466
const int pinBuzzer = 26;  // Buzzer

// ================= LOGIKA DISIPLIN =================
const int sampleWindow = 50; 
unsigned int sample;
const int thresholdWaspada = 1500; 
const int thresholdBerisik = 2500; 

void setup() {
  Serial.begin(115200);

  // Setup Pin Mode
  pinMode(pinTrafficRed, OUTPUT);
  pinMode(pinTrafficYellow, OUTPUT);
  pinMode(pinTrafficGreen, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinMic, INPUT);

  // --- MANUAL RESET OLED (Wajib untuk Hardware SPI) ---
  pinMode(OLED_RESET, OUTPUT);
  digitalWrite(OLED_RESET, LOW);
  delay(50); // Delay agak lama dikit
  digitalWrite(OLED_RESET, HIGH);
  delay(50);
  // ----------------------------------------------

  // Inisialisasi OLED
  // Coba init dengan Hardware SPI
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("OLED Gagal! Pastikan D0->18 dan D1->23"));
  } else {
    Serial.println(F("OLED Berhasil!"));
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.cp437(true);
  }

  // Tampilan Pembuka
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("SISTEM DISIPLIN");
  display.setCursor(10, 25);
  display.println("KELAS PINTAR");
  display.display();
  
  // Tes Nyala Lampu & Buzzer Singkat
  digitalWrite(pinTrafficRed, HIGH); delay(200); digitalWrite(pinTrafficRed, LOW);
  digitalWrite(pinTrafficYellow, HIGH); delay(200); digitalWrite(pinTrafficYellow, LOW);
  digitalWrite(pinTrafficGreen, HIGH); delay(200); digitalWrite(pinTrafficGreen, LOW);
  
  // Tes Buzzer
  tone(pinBuzzer, 1500, 200); 
  delay(1000);
}

void loop() {
  unsigned long startMillis = millis(); 
  unsigned int peakToPeak = 0;   
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;

  // 1. BACA SENSOR SUARA
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(pinMic);
    if (sample < 4095) {
      if (sample > signalMax) signalMax = sample;
      if (sample < signalMin) signalMin = sample;
    }
  }
  peakToPeak = signalMax - signalMin; 
  
  Serial.print("Suara: "); Serial.println(peakToPeak);

  // 2. UPDATE LAYAR OLED
  display.clearDisplay();
  int barLength = map(peakToPeak, 0, 4095, 0, SCREEN_WIDTH);
  display.fillRect(0, 55, barLength, 8, WHITE); 

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Noise Level: "); display.println(peakToPeak);
  
  display.setTextSize(2);
  display.setCursor(0, 20);

  // 3. LOGIKA LAMPU & ALARM
  if (peakToPeak > thresholdBerisik) {
    digitalWrite(pinTrafficRed, HIGH);
    digitalWrite(pinTrafficYellow, LOW);
    digitalWrite(pinTrafficGreen, LOW);
    tone(pinBuzzer, 2000); 
    display.println("BERISIK!!");
    display.setTextSize(1); display.setCursor(0, 40); display.println("MOHON TENANG!");
  } 
  else if (peakToPeak > thresholdWaspada) {
    digitalWrite(pinTrafficRed, LOW);
    digitalWrite(pinTrafficYellow, HIGH);
    digitalWrite(pinTrafficGreen, LOW);
    noTone(pinBuzzer); 
    display.println("WASPADA");
    display.setTextSize(1); display.setCursor(0, 40); display.println("Suara Meningkat");
  } 
  else {
    digitalWrite(pinTrafficRed, LOW);
    digitalWrite(pinTrafficYellow, LOW);
    digitalWrite(pinTrafficGreen, HIGH);
    noTone(pinBuzzer);
    display.println("AMAN");
    display.setTextSize(1); display.setCursor(0, 40); display.println("Kelas Kondusif");
  }

  display.display(); 
}
