// Wio Terminal 스마트 테라리움 UI - 색상 개선 버전
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Wire.h>
#include "Seeed_BME280.h"
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// Seeed BME280 센서 객체 생성
BME280 bme280;

// 스피커
SoftwareSerial mySerial(7,8); // Wio Terminal 핀에 맞게 수정
DFRobotDFPlayerMini myDFPlayer;

// 디스플레이 및 LVGL 버퍼 설정
TFT_eSPI tft;
static lv_disp_buf_t draw_buf;
static lv_color_t buf[320 * 10];
static lv_disp_drv_t disp_drv;

// --- 전역 변수 선언 ---
static lv_obj_t *screen_blank;        // 빈 화면
static lv_obj_t *screen_sensor;        // 온습도 화면
static lv_obj_t *screen_water;         // 물 분사 화면
static lv_obj_t *screen_sound;         // 소리 설정 화면

// UI 위젯 객체
lv_obj_t *label_temp;
lv_obj_t *label_humidity;
lv_obj_t *bar_water_level;
lv_obj_t *label_water_status;
lv_obj_t *bar_volume;
lv_obj_t *label_music_title;
lv_obj_t *label_music_status;

lv_obj_t *label_temp_arrow;
lv_obj_t *label_humidity_arrow;

// [추가] 온도와 습도 컨테이너 객체 (색상 변경용)
lv_obj_t *cont_temp;
lv_obj_t *cont_hum;

// 데이터 변수
bool misting_on = false;
int water_level = 80; // 남은 물의 양 (0-100%)
int volume_level = 5;
bool music_on = false;
const char *music_list[] = {"Rain ASMR", "Forest Sounds", "Ocean Waves"};
int current_track = 0;
unsigned long lastUpdateTime = 0;
const long updateInterval = 2000;
bool dfplayer_playing = false; //노래 재생중인지

// [추가] 물 분사 관련 변수
#define ATOMIZER_PIN A0 
int atomizer_level = 0; // 0~3 단계
int pwmLevels[4] = {0, 100, 180, 255};

// [추가] 색상 정의 (연한 색상)
#define LIGHT_GREEN   LV_COLOR_MAKE(144, 238, 144)  // 연한 초록색
#define LIGHT_BLUE    LV_COLOR_MAKE(173, 216, 230)  // 연한 파란색  
#define LIGHT_RED     LV_COLOR_MAKE(255, 182, 193)  // 연한 빨간색
#define DEFAULT_BG    LV_COLOR_WHITE                 // 기본 배경색

// 물 분사 제어 함수
void applyPower() {
    if (misting_on && atomizer_level > 0) {
        analogWrite(ATOMIZER_PIN, pwmLevels[atomizer_level]);
        Serial.print("💧 Atomization ON | Level ");
        Serial.print(atomizer_level);
        Serial.print(" → PWM=");
        Serial.println(pwmLevels[atomizer_level]);
    } else {
        analogWrite(ATOMIZER_PIN, 0);
        Serial.println("💤 Atomization OFF (Level 0)");
    }
}

// [수정] 센서 데이터 업데이트 함수 - 색상 변경 로직 추가
void update_sensor_data() {
    float temp = bme280.getTemperature();
    float hum = bme280.getHumidity();

    char temp_buf[10];
    char hum_buf[10];
    snprintf(temp_buf, sizeof(temp_buf), "%.1f C", temp);
    snprintf(hum_buf, sizeof(hum_buf), "%.1f %%", hum);
    
    if(lv_scr_act() == screen_sensor) {
        lv_label_set_text(label_temp, temp_buf);
        lv_label_set_text(label_humidity, hum_buf);
        
        // [추가] 온도에 따른 배경색 변경
        lv_color_t temp_color;
        if (temp < 18.0) {
            temp_color = LIGHT_BLUE;  // 추위 - 연한 파란색
        } else if (temp > 25.0) {
            temp_color = LIGHT_RED;   // 더위 - 연한 빨간색
        } else {
            temp_color = LIGHT_GREEN; // 정상 - 연한 초록색
        }
        
        // [추가] 습도에 따른 배경색 변경
        lv_color_t hum_color;
        if (hum < 65.0) {
            hum_color = LIGHT_BLUE;   // 건조 - 연한 파란색
        } else if (hum > 90.0) {
            hum_color = LIGHT_RED;    // 과습 - 연한 빨간색
        } else {
            hum_color = LIGHT_GREEN;  // 정상 - 연한 초록색
        }
        
        // 컨테이너 배경색 적용
        static lv_style_t style_temp, style_hum;
        
        lv_style_init(&style_temp);
        lv_style_set_bg_color(&style_temp, LV_STATE_DEFAULT, temp_color);
        lv_obj_add_style(cont_temp, LV_OBJ_PART_MAIN, &style_temp);
        
        lv_style_init(&style_hum);
        lv_style_set_bg_color(&style_hum, LV_STATE_DEFAULT, hum_color);
        lv_obj_add_style(cont_hum, LV_OBJ_PART_MAIN, &style_hum);
    }

    // 온도 화살표
    if (temp < 18.0) {
        lv_label_set_text(label_temp_arrow, "^");
    } else if (temp > 25.0) {
        lv_label_set_text(label_temp_arrow, "v");
    } else {
        lv_label_set_text(label_temp_arrow, "");
    }

    // 습도 화살표
    if (hum < 65.0) {
        lv_label_set_text(label_humidity_arrow, "^");
    } else if (hum > 90.0) {
        lv_label_set_text(label_humidity_arrow, "v");
    } else {
        lv_label_set_text(label_humidity_arrow, "");
    }
}

//노래
void update_music(bool volumeChanged = false) {
    if (volumeChanged) {
        myDFPlayer.volume(volume_level * 3); // 0~30
    }

    if (music_on) {
        if (!dfplayer_playing) {
            myDFPlayer.play(current_track + 1); // 1부터 시작
            dfplayer_playing = true;
        }
    } else {
        if (dfplayer_playing) {
            myDFPlayer.pause();
            dfplayer_playing = false;
        }
    }
}

// 디스플레이 드라이버 콜백
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

// [추가] 1. 빈 화면 생성
void create_blank_screen() {
    screen_blank = lv_obj_create(NULL, NULL);
}

// [수정] 2. 온습도 표시 화면 생성 - 컨테이너 전역변수로 변경
void create_sensor_screen() {
    screen_sensor = lv_obj_create(NULL, NULL);
    
    lv_obj_t *label_title = lv_label_create(screen_sensor, NULL);
    lv_label_set_text(label_title, "Sensor Status");
    lv_obj_align(label_title, NULL, LV_ALIGN_IN_TOP_MID, 0, 15);

    // [수정] 온도 컨테이너를 전역변수로 변경
    cont_temp = lv_obj_create(screen_sensor, NULL);
    lv_obj_set_size(cont_temp, 140, 100);
    lv_obj_align(cont_temp, NULL, LV_ALIGN_CENTER, -75, 10);
    
    label_temp = lv_label_create(cont_temp, NULL);
    lv_label_set_text(label_temp, "N/A C");
    lv_obj_align(label_temp, NULL, LV_ALIGN_CENTER, 0, 0);
    
    // 온도 화살표
    label_temp_arrow = lv_label_create(cont_temp, NULL);
    lv_label_set_text(label_temp_arrow, "");
    lv_obj_align(label_temp_arrow, label_temp, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // [수정] 습도 컨테이너를 전역변수로 변경
    cont_hum = lv_obj_create(screen_sensor, NULL);
    lv_obj_set_size(cont_hum, 140, 100);
    lv_obj_align(cont_hum, NULL, LV_ALIGN_CENTER, 75, 10);
    
    label_humidity = lv_label_create(cont_hum, NULL);
    lv_label_set_text(label_humidity, "N/A %");
    lv_obj_align(label_humidity, NULL, LV_ALIGN_CENTER, 0, 0);

    // 습도 화살표
    label_humidity_arrow = lv_label_create(cont_hum, NULL);
    lv_label_set_text(label_humidity_arrow, "");
    lv_obj_align(label_humidity_arrow, label_humidity, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

// [추가] 3. 물 분사 제어 화면 생성
void create_water_screen() {
    screen_water = lv_obj_create(NULL, NULL);
    
    lv_obj_t *label_title = lv_label_create(screen_water, NULL);
    lv_label_set_text(label_title, "Water Control");
    lv_obj_align(label_title, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);

    label_water_status = lv_label_create(screen_water, NULL);
    lv_label_set_text(label_water_status, "Misting: OFF");
    lv_obj_align(label_water_status, NULL, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t * water_level_label = lv_label_create(screen_water, NULL);
    lv_label_set_text(water_level_label, "Water Level");
    lv_obj_align(water_level_label, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -70);

    bar_water_level = lv_bar_create(screen_water, NULL);
    lv_obj_set_size(bar_water_level, 200, 30);
    lv_obj_align(bar_water_level, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -30);
    lv_bar_set_value(bar_water_level, water_level, LV_ANIM_OFF);
}

// [변경] 4. 소리 설정 화면 생성 (이전의 스피커 화면과 동일)
void create_sound_screen() {
    screen_sound = lv_obj_create(NULL, NULL);
    lv_obj_t *label_title = lv_label_create(screen_sound, NULL);
    lv_label_set_text(label_title, "Sound Control");
    lv_obj_align(label_title, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);
    bar_volume = lv_bar_create(screen_sound, NULL);
    lv_obj_set_size(bar_volume, 30, 120);
    lv_obj_align(bar_volume, NULL, LV_ALIGN_IN_LEFT_MID, 20, 10);
    lv_bar_set_range(bar_volume, 0, 10);
    lv_bar_set_value(bar_volume, volume_level, LV_ANIM_OFF);
    label_music_title = lv_label_create(screen_sound, NULL);
    lv_label_set_text(label_music_title, music_list[current_track]);
    lv_obj_align(label_music_title, NULL, LV_ALIGN_CENTER, 30, 0);
    label_music_status = lv_label_create(screen_sound, NULL);
    lv_label_set_text(label_music_status, LV_SYMBOL_PLAY " PAUSED");
    lv_obj_align(label_music_status, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -30);
}

// 버튼 핸들러 로직
void button_event_handler() {
    lv_obj_t * current_screen = lv_scr_act();
    if (digitalRead(WIO_KEY_A) == LOW) {
        if (current_screen == screen_sensor) {
            lv_scr_load(screen_water);
        } else if (current_screen == screen_water) {
            lv_scr_load(screen_sound);
        } else if (current_screen == screen_sound) {
            lv_scr_load(screen_sensor);
        }
        delay(200);
    }
    
    else if (digitalRead(WIO_KEY_B) == LOW) {
        lv_scr_load(screen_water);
        delay(200);
    }
    
    else if (digitalRead(WIO_KEY_C) == LOW) {
        lv_scr_load(screen_sensor);
        delay(200);
    }
}

// [변경] 5방향 스위치 핸들러에 물 분사 제어 추가
void dpad_event_handler() {
    lv_obj_t * current_screen = lv_scr_act();
    if (current_screen == screen_water) {
        if (digitalRead(WIO_5S_PRESS) == LOW) { 
            misting_on = !misting_on;
            if (misting_on) {
                atomizer_level = 3;
                lv_label_set_text(label_water_status, "Misting: ON");
            } else {
                atomizer_level = 0;
                lv_label_set_text(label_water_status, "Misting: OFF");
            }
            applyPower();
            lv_bar_set_value(bar_water_level, (int)(atomizer_level * (100.0 / 3.0)), LV_ANIM_ON);
            delay(200);
        }
        // 물 분사 세기 증가
        if (digitalRead(WIO_5S_UP) == LOW) {
            if (atomizer_level < 3) atomizer_level++;
            if (atomizer_level > 0) {
                misting_on = true;
                lv_label_set_text(label_water_status, "Misting: ON");
            }            
            applyPower();
            lv_bar_set_value(bar_water_level, (int)(atomizer_level * (100.0 / 3.0)), LV_ANIM_ON);
            if (atomizer_level > 0) {
                lv_label_set_text(label_water_status, "Misting: ON");
                misting_on = true;
            }
            delay(200);
        }
        // 물 분사 세기 감소
        if (digitalRead(WIO_5S_DOWN) == LOW) {
            if (atomizer_level > 0) atomizer_level--;
            if (atomizer_level == 0) {
                misting_on = false;
                lv_label_set_text(label_water_status, "Misting: OFF");
            }            
            applyPower();
            lv_bar_set_value(bar_water_level, (int)(atomizer_level * (100.0 / 3.0)), LV_ANIM_ON);
            if (atomizer_level == 0) {
                lv_label_set_text(label_water_status, "Misting: OFF");
                misting_on = false;
            }
            delay(200);
        }

    } 
     else if (current_screen == screen_sound) {
        if (digitalRead(WIO_5S_UP) == LOW) { 
    if(volume_level < 10) volume_level++; 
    update_music(true);   // 볼륨 바뀔 때만 true
    delay(100); 
}
else if (digitalRead(WIO_5S_DOWN) == LOW) { 
    if(volume_level > 0) volume_level--; 
    update_music(true);   // 볼륨 바뀔 때만 true
    delay(100); 
}
else if (digitalRead(WIO_5S_LEFT) == LOW) { 
    current_track = (current_track - 1 + 3) % 3; 
    lv_label_set_text(label_music_title, music_list[current_track]); 
    dfplayer_playing = false;  
    update_music();       // 트랙 바뀌면 바로 반영
    delay(200); 
}
else if (digitalRead(WIO_5S_RIGHT) == LOW) { 
    current_track = (current_track + 1) % 3; 
    lv_label_set_text(label_music_title, music_list[current_track]); 
    dfplayer_playing = false;  
    update_music();
    delay(200); 
}
else if (digitalRead(WIO_5S_PRESS) == LOW) { 
    music_on = !music_on; 
    update_music();       // 재생/정지 전환
    delay(200); 
}

        // UI 업데이트
        lv_bar_set_value(bar_volume, volume_level, LV_ANIM_ON);
        lv_label_set_text(label_music_status, music_on ? LV_SYMBOL_PAUSE " PLAYING" : LV_SYMBOL_PLAY " PAUSED");
    
     }
}

void setup() {
    Serial.begin(9600);
    mySerial.begin(9600); //스피커 용
    Serial.println("Smart Terrarium UI - Color Enhanced Version");
    
    if (!bme280.init()) {
        Serial.println("Device error! (BME280 not found)");
    }
    if (!myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini 초기화 실패!");
    
    }
    
    tft.begin();
    tft.setRotation(3);
    lv_init();
    lv_disp_buf_init(&draw_buf, buf, NULL, 320 * 10);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // 핀 설정
    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    pinMode(WIO_5S_UP, INPUT_PULLUP);
    pinMode(WIO_5S_DOWN, INPUT_PULLUP);
    pinMode(WIO_5S_LEFT, INPUT_PULLUP);
    pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
    pinMode(WIO_5S_PRESS, INPUT_PULLUP);

    // [추가] 물 분사기 핀을 OUTPUT으로 설정
    pinMode(ATOMIZER_PIN, OUTPUT);
    
    // 모든 화면을 생성
    create_blank_screen();
    create_sensor_screen();
    create_water_screen();
    create_sound_screen();
    
    // 시작 화면을 센서 화면으로 설정
    lv_scr_load(screen_sensor);
}

void loop() {
    lv_tick_inc(5);
    lv_task_handler(); 
    button_event_handler();
    dpad_event_handler();

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentMillis;
        update_sensor_data();
    }
    
    delay(5);
}
