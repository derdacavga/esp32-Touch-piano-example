#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

struct Button {
  int16_t x, y, w, h;
  String id;
  String label_idle;
  String label_active;
  uint16_t color_idle;
  uint16_t color_active;
  uint16_t color_text;
  bool isActive = false;
};

Button recordButton;
Button playButton;
Button songButton;

int notes[] = { 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523 };
String noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C5" };
#define NOTE_COUNT (sizeof(notes) / sizeof(int))

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define BUTTON_AREA_Y 10
#define BUTTON_HEIGHT 50
#define PIANO_AREA_Y (SCREEN_HEIGHT - 150)
#define KEY_HEIGHT 150
#define KEY_WIDTH 24
#define KEY_SPACING 1
#define BUZZER_PIN 13
#define BACKLIGHT_PIN 5

struct NoteEvent {
  int noteIndex;
  unsigned long pauseBefore;
};

#define MAX_RECORDED_NOTES 9999
NoteEvent recordedMelody[MAX_RECORDED_NOTES];
int recordedNoteCount = 0;
bool isRecording = false;
unsigned long lastEventTime = 0;
unsigned long lastButtonPressTime = 0;
const unsigned long buttonDebounceDelay = 500;
int currentlyHeldKey = -1;

void initButtons();
void drawButton(Button &b);
void perform_calibration();
void drawPiano();
void playRecordedNote(int noteIndex, int duration);
void playMelody();
void playMissionImpossible(); 
void startNote(int noteIndex);
void stopNote(int noteIndex);

void setup() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  tft.init();
  tft.setRotation(1);
  perform_calibration();
  initButtons();
  drawPiano();
  drawButton(recordButton);
  drawButton(playButton);
  drawButton(songButton);
}

void loop() {
  uint16_t t_x, t_y;
  bool isTouching = tft.getTouch(&t_x, &t_y);

  if (isTouching) {
    bool onButton = false;
    if (t_y < BUTTON_AREA_Y + BUTTON_HEIGHT) {
      onButton = true;
      if ((t_x > recordButton.x) && (t_x < recordButton.x + recordButton.w)) {
        if (millis() - lastButtonPressTime > buttonDebounceDelay) {
          lastButtonPressTime = millis();
          recordButton.isActive = !recordButton.isActive;
          isRecording = recordButton.isActive;
          if (isRecording) { recordedNoteCount = 0; lastEventTime = millis(); }
          drawButton(recordButton);
        }
      } else if ((t_x > playButton.x) && (t_x < playButton.x + playButton.w)) {
        if (millis() - lastButtonPressTime > buttonDebounceDelay) {
          lastButtonPressTime = millis();
          if (!isRecording) { playMelody(); }
        }
      } else if ((t_x > songButton.x) && (t_x < songButton.x + songButton.w)) {
        if (millis() - lastButtonPressTime > buttonDebounceDelay) {
          lastButtonPressTime = millis();
          if (!isRecording) { playMissionImpossible(); } 
        }
      }
    }

    int touchedKey = -1;
    if (!onButton && t_y > PIANO_AREA_Y) {
      touchedKey = t_x / (KEY_WIDTH + KEY_SPACING);
      if (touchedKey >= NOTE_COUNT) touchedKey = -1;
    }

    if (touchedKey != -1 && touchedKey != currentlyHeldKey) {
      if (currentlyHeldKey != -1) { stopNote(currentlyHeldKey); }
      startNote(touchedKey);
      currentlyHeldKey = touchedKey;
      if (isRecording && recordedNoteCount < MAX_RECORDED_NOTES) {
        unsigned long currentTime = millis();
        recordedMelody[recordedNoteCount++] = { touchedKey, currentTime - lastEventTime };
        lastEventTime = currentTime;
      }
    } else if (touchedKey == -1 && currentlyHeldKey != -1) {
      stopNote(currentlyHeldKey);
      currentlyHeldKey = -1;
    }
  } else {
    if (currentlyHeldKey != -1) {
      stopNote(currentlyHeldKey);
      currentlyHeldKey = -1;
    }
  }
}

void playMissionImpossible() {
  songButton.isActive = true;
  drawButton(songButton);

  const int dot_time = 120; 
  const int dash_time = 250;
  const int pause_between_notes = 50;
  const int pause_between_phrases = 400;

  int phrase1[] = {7, 7, 10, 12}; // G, G, A#, C5
  int phrase2[] = {7, 7, 5, 6};   // G, G, F, F#

  for (int i = 0; i < 2; i++) {
    if (!songButton.isActive) break;

    playRecordedNote(phrase1[0], dash_time);
    delay(pause_between_notes);
    playRecordedNote(phrase1[1], dash_time);
    delay(pause_between_notes);
    playRecordedNote(phrase1[2], dot_time);
    delay(pause_between_notes);
    playRecordedNote(phrase1[3], dot_time);
    delay(pause_between_phrases);
    
    if (!songButton.isActive) break;

    playRecordedNote(phrase2[0], dash_time);
    delay(pause_between_notes);
    playRecordedNote(phrase2[1], dash_time);
    delay(pause_between_notes);
    playRecordedNote(phrase2[2], dot_time);
    delay(pause_between_notes);
    playRecordedNote(phrase2[3], dot_time);
    delay(pause_between_phrases);
  }

  songButton.isActive = false;
  drawButton(songButton);
}

void initButtons() {
  recordButton.x = 10;
  recordButton.y = BUTTON_AREA_Y;
  recordButton.w = 95;
  recordButton.h = BUTTON_HEIGHT;
  recordButton.id = "record";
  recordButton.label_idle = "REC";
  recordButton.label_active = "REC..";
  recordButton.color_idle = TFT_DARKGREEN;
  recordButton.color_active = TFT_RED;
  recordButton.color_text = TFT_WHITE;
  recordButton.isActive = false;

  playButton.x = 115;
  playButton.y = BUTTON_AREA_Y;
  playButton.w = 95;
  playButton.h = BUTTON_HEIGHT;
  playButton.id = "play";
  playButton.label_idle = "PLAY";
  playButton.label_active = "PLAY..";
  playButton.color_idle = TFT_BLUE;
  playButton.color_active = TFT_ORANGE;
  playButton.color_text = TFT_WHITE;
  playButton.isActive = false;
  
  songButton.x = 220;
  songButton.y = BUTTON_AREA_Y;
  songButton.w = 95;
  songButton.h = BUTTON_HEIGHT;
  songButton.id = "song";
  songButton.label_idle = "SONG";
  songButton.label_active = "SONG..";
  songButton.color_idle = TFT_PURPLE;
  songButton.color_active = TFT_MAGENTA;
  songButton.color_text = TFT_WHITE;
  songButton.isActive = false;
}

void drawButton(Button &b) {
  uint16_t current_color = b.isActive ? b.color_active : b.color_idle;
  String current_label = b.isActive ? b.label_active : b.label_idle;
  uint16_t highlight_color = TFT_WHITE;
  uint16_t shadow_color = TFT_DARKGREY;
  uint8_t corner_radius = 8;
  
  tft.fillRoundRect(b.x, b.y, b.w, b.h, corner_radius, current_color);

  if (!b.isActive) {
    tft.drawFastHLine(b.x + corner_radius, b.y, b.w - (2*corner_radius), highlight_color);
    tft.drawFastVLine(b.x, b.y + corner_radius, b.h - (2*corner_radius), highlight_color);
  } else {
    tft.drawFastHLine(b.x + corner_radius, b.y, b.w - (2*corner_radius), shadow_color);
    tft.drawFastVLine(b.x, b.y + corner_radius, b.h - (2*corner_radius), shadow_color);
  }

  tft.setTextColor(b.color_text, current_color);
  tft.setTextSize(2);
  int16_t text_x = b.x + (b.w - tft.textWidth(current_label)) / 2;
  int16_t text_y = b.y + (b.h - tft.fontHeight()) / 2;

  if (b.isActive) {
    text_x += 1;
    text_y += 1;
  }
  
  tft.drawString(current_label, text_x, text_y);
}

void drawPiano() {
  tft.fillRect(0, PIANO_AREA_Y, SCREEN_WIDTH, KEY_HEIGHT, TFT_BLACK);
  for (int i = 0; i < NOTE_COUNT; i++) {
    int keyX = i * (KEY_WIDTH + KEY_SPACING);
    bool isBlackKey = (noteNames[i].indexOf('#') != -1);
    
    if(isBlackKey) {
        tft.fillRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT * 0.6, TFT_BLACK);
        tft.drawRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT * 0.6, TFT_WHITE);
    } else {
        tft.fillRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT, TFT_WHITE);
        tft.drawRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT, TFT_BLACK);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(2);
        tft.drawString(noteNames[i], keyX + 5, PIANO_AREA_Y + KEY_HEIGHT - 30);
    }
  }
}

void startNote(int noteIndex) {
  if (noteIndex < 0 || noteIndex >= NOTE_COUNT) return;
  int keyX = noteIndex * (KEY_WIDTH + KEY_SPACING);
  bool isBlackKey = (noteNames[noteIndex].indexOf('#') != -1);

  if(isBlackKey) {
      tft.fillRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT * 0.6, TFT_YELLOW);
  } else {
      tft.fillRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT, TFT_YELLOW);
      tft.setTextColor(TFT_BLACK);
      tft.drawString(noteNames[noteIndex], keyX + 5, PIANO_AREA_Y + KEY_HEIGHT - 30);
  }
  tone(BUZZER_PIN, notes[noteIndex]);
}

void stopNote(int noteIndex) {
  if (noteIndex < 0 || noteIndex >= NOTE_COUNT) return;
  noTone(BUZZER_PIN);
  int keyX = noteIndex * (KEY_WIDTH + KEY_SPACING);
  bool isBlackKey = (noteNames[noteIndex].indexOf('#') != -1);

  if(isBlackKey) {
      tft.fillRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT * 0.6, TFT_BLACK);
      tft.drawRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT * 0.6, TFT_WHITE);
  } else {
      tft.fillRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT, TFT_WHITE);
      tft.drawRect(keyX, PIANO_AREA_Y, KEY_WIDTH, KEY_HEIGHT, TFT_BLACK);
      tft.setTextColor(TFT_BLACK);
      tft.drawString(noteNames[noteIndex], keyX + 5, PIANO_AREA_Y + KEY_HEIGHT - 30);
  }
}

void playRecordedNote(int noteIndex, int duration) {
  if (noteIndex < 0 || noteIndex >= NOTE_COUNT) return;
  startNote(noteIndex);
  delay(duration);
  stopNote(noteIndex);
}

void playMelody() {
  playButton.isActive = true;
  drawButton(playButton);
  for (int i = 0; i < recordedNoteCount; i++) {
    if (!playButton.isActive) break;
    delay(recordedMelody[i].pauseBefore);
    if (!playButton.isActive) break;
    playRecordedNote(recordedMelody[i].noteIndex, 200);
  }
  playButton.isActive = false;
  drawButton(playButton);
}

void perform_calibration() {
  uint16_t calData[5];
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Touch calibration needed");
  tft.setTextFont(2);
  tft.println();
  tft.println("Press corners as directed");
  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
  tft.setTouch(calData);
  tft.fillScreen(TFT_BLACK);
}