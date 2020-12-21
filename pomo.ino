#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <JC_Button.h>

#include "tomat.h"

/**
 * TODO:
 *  - Fixa tomat
 *  - Setup mode:
 *    - Editera tider
 *    - Spara tider till EEPROM
 *  - Nollställa tomaträknare
 *    - Skriv till EEPROM
 *  - Paus/play/stop-markör / mode-markör
 *
 *  - Programmera buzzer-alarm
 */

#define DISABLE_SOUND
#define DISABLE_TOMATOES

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);

const int BUTTON_LONG_PRESS_TIME_MS = 1000;
enum class ButtonEvent {
    NoEvent,
    ShortPressed,
    LongPressed
};

class MyButton {
public:
    MyButton(int pin) : long_pressed(false), button(pin) {}
    void begin() { button.begin(); }

    ButtonEvent process() {
        button.read();

        ButtonEvent button_event = ButtonEvent::NoEvent;
        if(button.pressedFor(BUTTON_LONG_PRESS_TIME_MS)) {
            if(!long_pressed) {
                button_event = ButtonEvent::LongPressed;
                long_pressed = true;
            }
        }
        if(button.wasReleased()) {
            if(!long_pressed) {
                button_event = ButtonEvent::ShortPressed;
            }
            long_pressed = false;
        }
        return button_event;
    }
private:
    bool long_pressed;
    Button button;
};

MyButton setup_button(10);
MyButton playpause_button(12);
MyButton plus_button(9);
MyButton minus_button(7);

#define BUZZER_PIN 2

#define TOMATO_COUNTER_MAX 99
#define TIME_MIN_MAX 99
#define TIME_SEC_MAX 59
#define TOMATO_COUNTER_ADDRESS 0
#define REST_SETUP_TIME_MIN_ADDRESS 1
#define REST_SETUP_TIME_SEC_ADDRESS 2
#define WORK_SETUP_TIME_MIN_ADDRESS 3
#define WORK_SETUP_TIME_SEC_ADDRESS 4

enum class InputEvent {
    NoEvent,
    SetupShortPress,
    SetupLongPress,
    PlayPauseShortPress,
    PlayPauseLongPress,
    PlusShortPress,
    PlusLongPress,
    MinusShortPress,
    MinusLongPress,
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
        return uint16_t(min)*60 + sec;
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
InputEvent input_event = InputEvent::NoEvent;
Time rest_setup_time; // initialize from EEPROM
Time work_setup_time; // initialize from EEPROM

TimerType current_timer = TimerType::Work;

uint32_t timer_seconds_left = 0;
uint32_t timer_end = 0;

int8_t tomato_counter; // initialize from EEPROM

uint32_t alarm_blink_next_time = 0;
uint32_t alarm_blink_period_ms = 250;

bool display_dimmed = false;

void process_buttons() {
    input_event = InputEvent::NoEvent;

    auto setup_button_event = setup_button.process();
    switch(setup_button_event) {
    case ButtonEvent::LongPressed:
        input_event = InputEvent::SetupLongPress;
        break;
    case ButtonEvent::ShortPressed:
        input_event = InputEvent::SetupShortPress;
        break;
    }

    auto playpause_button_event = playpause_button.process();
    switch(playpause_button_event) {
    case ButtonEvent::LongPressed:
        input_event = InputEvent::PlayPauseLongPress;
        break;
    case ButtonEvent::ShortPressed:
        input_event = InputEvent::PlayPauseShortPress;
        break;
    }

    auto plus_button_event = plus_button.process();
    switch(plus_button_event) {
    case ButtonEvent::LongPressed:
        input_event = InputEvent::PlusLongPress;
        break;
    case ButtonEvent::ShortPressed:
        input_event = InputEvent::PlusShortPress;
        break;
    }

    auto minus_button_event = minus_button.process();
    switch(minus_button_event) {
    case ButtonEvent::LongPressed:
        input_event = InputEvent::MinusLongPress;
        break;
    case ButtonEvent::ShortPressed:
        input_event = InputEvent::MinusShortPress;
        break;
    }
}

void process_timer() {

    if(state == State::Running) {
        timer_seconds_left = timer_end - now();
    }
}

void handle_events() {
    if(state == State::Running) {
        if(timer_seconds_left <= 0) {
            timer_seconds_left = 0;
            state = State::Ringing;
            alarm_blink_next_time = millis() + alarm_blink_period_ms;
            if(current_timer == TimerType::Work) {
                 tomato_counter = (tomato_counter+1)%(TOMATO_COUNTER_MAX+1);
                 EEPROM.write(TOMATO_COUNTER_ADDRESS, tomato_counter);
            }
        } else if(input_event == InputEvent::PlayPauseShortPress) {
            Serial.print("Pausing. Timer_seconds_left: ");
            Serial.println(timer_seconds_left);
            state = State::Paused;
        } else if(input_event == InputEvent::PlayPauseLongPress) {
            state = State::Idle;
            Serial.println("Stopping. Going to Idle.");
        }
    } else if(state== State::Paused) {
        if(input_event == InputEvent::PlayPauseShortPress) {
            state = State::Running;
            timer_end = now() + timer_seconds_left;
            Serial.print("Resuming. Timer_seconds_left: ");
            Serial.print(timer_seconds_left);
            Serial.print(", timer_end: ");
            Serial.println(timer_end);
        } else if(input_event == InputEvent::PlayPauseLongPress) {
            state = State::Idle;
            Serial.println("Stopping. Going to Idle.");
        }
    } else if(state == State::Idle) {
        if(input_event == InputEvent::PlayPauseShortPress) {
            uint16_t timer_length;
            if(current_timer == TimerType::Work) {
                timer_length = work_setup_time.to_seconds();
            } else {
                timer_length = rest_setup_time.to_seconds();
            }
            timer_seconds_left = timer_length;
            state = State::Running;
            timer_end = now() + timer_seconds_left;
            Serial.print("Starting to run. timer_seconds_left: ");
            Serial.print(timer_seconds_left);
            Serial.print(", timer_end: ");
            Serial.println(timer_end);
        } else if(input_event == InputEvent::PlusShortPress) {
            current_timer = current_timer==TimerType::Work ? TimerType::Rest : TimerType::Work;
            Serial.print("Switching timer to ");
            Serial.println(current_timer==TimerType::Work ? "Work" : "Rest");
        } else if(input_event == InputEvent::MinusShortPress) {
            current_timer = current_timer==TimerType::Work ? TimerType::Rest : TimerType::Work;
            Serial.print("Switching timer to ");
            Serial.println(current_timer==TimerType::Work ? "Work" : "Rest");
            /* case InputEvent::SetupLongPress: */
            /* Serial.println("Switching to Setup state"); */
            /*     state = State::Setup; */
            /*     break; */
        }
    } else if(state == State::Ringing) {
        if(input_event != InputEvent::NoEvent) {
            state = State::Idle;
            current_timer = current_timer==TimerType::Work ? TimerType::Rest : TimerType::Work;
            Serial.println("Leaving ringing to Idle");
        }
    } else if(state == State::Setup) {
        if(input_event==InputEvent::SetupLongPress) {
    /*         state = State::Idle; */
            /* Serial.println("Going back to Idle from Setup"); */
        }
    }
}

void process_alarm() {
    if(state == State::Ringing) {
        uint32_t now = millis();
        if(now > alarm_blink_next_time) {
            display_dimmed = !display_dimmed;
            alarm_blink_next_time = now + alarm_blink_period_ms;
        }

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

    display.dim(state==State::Ringing && display_dimmed);

// draw tomato
#ifndef DISABLE_TOMATOES
    /* display.fillSquareCircle(25, 25, 12, SSD1306_WHITE); */
    display.drawBitmap(0, 0, tomato_bmp_data, tomato_bmp_width, tomato_bmp_height, SSD1306_WHITE);
#endif

    char buf[20];
#ifndef DISABLE_TOMATOES
    sprintf(buf, "%2d tomater", tomato_counter);
#else
    sprintf(buf, "%2d abcdefg", tomato_counter);
#endif
    display_text(buf, 56, 0);

    display_text("Jobba", 0, 66);

    Time work_time;
    if((state == State::Running || state == State::Paused || state == State::Ringing) && current_timer == TimerType::Work) {
        work_time = Time::from_seconds(timer_seconds_left);
    } else {
        work_time = work_setup_time;
    }
    display_time(work_time, 10, 66, false, false, 2);

    display_text("Rast", 30, 66);

    Time rest_time;
    if((state == State::Running || state == State::Paused || state == State::Ringing) && current_timer == TimerType::Rest) {
        rest_time = Time::from_seconds(timer_seconds_left);
    } else {
        rest_time = rest_setup_time;
    }
    display_time(rest_time, 30+10, 66, false, false, 2);

    //    if(state != State::
    // Draw indicator triangle
    int8_t indicator_col = 57;
    int8_t indicator_row = (current_timer == TimerType::Work ? 0 : 30)+12;
    int8_t triangle_height = 10;
    int8_t triangle_width  = 5;
    display.fillTriangle(indicator_col, indicator_row,
                         indicator_col+triangle_width-1, indicator_row+(triangle_height-1)/2,
                         indicator_col, indicator_row+triangle_height-1, SSD1306_WHITE);

    display.fillRect(66, 54, SCREEN_WIDTH-66, SCREEN_HEIGHT-54,
                        SSD1306_BLACK);
    display.display();
}

void setup() {
    setup_button.begin();
    playpause_button.begin();
    plus_button.begin();
    minus_button.begin();
#ifndef DISABLE_SOUND
    pinMode(BUZZER_PIN, OUTPUT);
#endif

    tomato_counter = min(EEPROM.read(TOMATO_COUNTER_ADDRESS), TOMATO_COUNTER_MAX);
    rest_setup_time.min = min(EEPROM.read(REST_SETUP_TIME_MIN_ADDRESS),TIME_MIN_MAX);
    rest_setup_time.sec = min(EEPROM.read(REST_SETUP_TIME_SEC_ADDRESS),TIME_SEC_MAX);
    work_setup_time.min = min(EEPROM.read(WORK_SETUP_TIME_MIN_ADDRESS),TIME_MIN_MAX);
    work_setup_time.sec = min(EEPROM.read(WORK_SETUP_TIME_SEC_ADDRESS),TIME_SEC_MAX);

   Serial.begin(115200);

   Serial.println("Trying to set up display...");
   if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        while(true) {}
    }

    Serial.println("Pomo starting up...");
    /* state=State::Running; */
}

void loop() {

    process_buttons();

    process_timer();

    handle_events();

    process_alarm();

    update_display();
}
