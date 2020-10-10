#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);

#define B1_PIN 6
#define B2_PIN 5
#define B3_PIN 2
#define B4_PIN 4

enum class ButtonEvent {
    NoEvent,
    ShortPressB1,
    ShortPressB2,
    ShortPressB3,
    ShortPressB4,
    LongPressB1,
    LongPressB2,
    LongPressB3,
    LongPressB4
};

enum class State {
    Idle,
    Running,
    Paused,
    Ringing,
    Setup
};

struct Time {
    uint8_t min, sec;
    static Time from_seconds(uint16_t sec) {
        return {sec/60, sec%60};
    }
    uint16_t to_seconds() const {
        return uint16_t(sec)*60 + sec;
    }
};

enum class TimerType {
    Work,
    Rest
};

uint32_t now() {
    return millis()/1000;
}

State state = State::Idle;
ButtonEvent button_event = ButtonEvent::NoEvent;
Time rest_setup_time = { 5, 0};
Time work_setup_time  = { 45, 0};

TimerType current_timer = TimerType::Work;

uint32_t timer_seconds_left = 0;
uint32_t timer_end = 0;

int8_t tomato_counter = 0;

void process_buttons() {

    char b1 = digitalRead(B1_PIN);
    char b2 = digitalRead(B2_PIN);
    char b3 = digitalRead(B3_PIN);
    char b4 = digitalRead(B4_PIN);

}

void process_timer() {

    if(state == State::Running) {
        timer_seconds_left = timer_end - now();
    }
}

void handle_events() {
    switch(state) {
    case State::Running: {
        if(timer_seconds_left <= 0) {
            state = State::Ringing;
        } else if(0) {
            state = State::Paused;
        } else if(0) {
            state = State::Idle;
        }
        break;
    }
    case State::Paused: {
        if(0) {
            state = State::Running;
            timer_end = now() + timer_seconds_left;
        } else if(0) {
            state = State::Idle;
        }
        break;
    }
    case State::Idle: {
        if(0) {
            uint16_t timer_length;
            if(current_timer == TimerType::Work) {
                timer_length = work_setup_time.to_seconds();
            } else {
                timer_length = rest_setup_time.to_seconds();
            }
            timer_seconds_left = timer_length;
            state = State::Running;
            timer_end = now() + timer_seconds_left;
        }
        break;
    }
    case State::Ringing: {
        if(0) {
            state = State::Idle;
            current_timer = current_timer==TimerType::Work ? TimerType::Rest : TimerType::Work;
        }
        break;
    }
    case State::Setup: {
        if(0) {
            state = State::Idle;
        }
        break;
    }
    }
}

void process_alarm() {
    if(state == State::Ringing) {
    }
}

void display_text(const char *txt, int8_t row, int8_t col, bool inverse=false, int8_t size=1) {
    display.setTextSize(size);
    if(!inverse) {
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    } else {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }
    display.setCursor(col,row);
    display.println(txt);
}

void display_time(Time t, int8_t row, int8_t col, bool m_inv, bool s_inv, int8_t size=1) {

    display.setTextSize(size);
    display.setCursor(col,row);
    if(m_inv) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    if(t.min < 10) {
        display.print("0");
    }
    display.print(t.min);

    if(m_inv) {
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    display.print(":");

    if(s_inv) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }
    if(t.sec < 10) {
        display.print("0");
    }
    display.print(t.sec);
}

void update_display() {

    display.clearDisplay();

    //    if(alarm_dim_display)

    display.fillCircle(25, 25, 12, SSD1306_WHITE);

    char buf[20];
    sprintf(buf, "%2d tomater", tomato_counter);
    display_text(buf, 50, 2);

    display_text("Jobba", 0, 66);

    Time work_time;
    if(state == State::Running && current_timer == TimerType::Work) {
        work_time = Time::from_seconds(timer_seconds_left);
    } else {
        work_time = work_setup_time;
    }
    display_time(work_time, 10, 66, false, false, 2);

    display_text("Vila", 32, 66);

    Time rest_time;
    if(state == State::Running && current_timer == TimerType::Rest) {
        rest_time = Time::from_seconds(timer_seconds_left);
    } else {
        rest_time = rest_setup_time;
    }
    display_time(rest_time, 32+10, 66, true, false, 2);

    //    if(state != State::
    // Draw indicator triangle
    int8_t indicator_col = 57;
    int8_t indicator_row = (current_timer == TimerType::Work ? 0 : 32)+12;
    int8_t triangle_height = 10;
    int8_t triangle_width  = 5;
    display.fillTriangle(indicator_col, indicator_row,
                         indicator_col+triangle_width-1, indicator_row+(triangle_height-1)/2,
                         indicator_col, indicator_row+triangle_height-1, SSD1306_WHITE);

    display.display();
}


void setup() {
    pinMode(B1_PIN, INPUT_PULLUP);
    pinMode(B2_PIN, INPUT_PULLUP);
    pinMode(B3_PIN, INPUT_PULLUP);
    pinMode(B4_PIN, INPUT_PULLUP);

    Serial.begin(115200);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
}


void loop() {

    process_buttons();

    process_timer();

    handle_events();

    process_alarm();

    update_display();

    //     display.dim(true);
    //    delay(250);
    //    display.dim(false);
    //    delay(250);
}
