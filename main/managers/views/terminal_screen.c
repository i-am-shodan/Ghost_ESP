#include "managers/views/terminal_screen.h"
#include "managers/views/main_menu_screen.h"
#include "core/serial_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

lv_obj_t *terminal_textarea = NULL;
uint32_t last_update = 0;
int closeCount = 0;
unsigned long lastPress = 0;
#define MAX_TEXT_LENGTH 4096

int custom_log_vprintf(const char *fmt, va_list args);
static int (*default_log_vprintf)(const char *, va_list) = NULL;

void terminal_view_create(void) {
    if (terminal_view.root != NULL) {
        return;
    }
   
    terminal_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(terminal_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(terminal_view.root, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(terminal_view.root, LV_SCROLLBAR_MODE_OFF);

    
    terminal_textarea = lv_textarea_create(terminal_view.root);
    lv_obj_set_style_bg_color(terminal_textarea, lv_color_black(), 0);
    lv_textarea_set_one_line(terminal_textarea, false);
    lv_textarea_set_text(terminal_textarea, "");
    lv_obj_set_size(terminal_textarea, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(terminal_textarea, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_color(terminal_textarea, lv_color_hex(0x00FF00), 0); 
    lv_obj_set_style_text_font(terminal_textarea, &lv_font_montserrat_10, 0);
    lv_obj_set_style_border_width(terminal_textarea, 0, 0);

    
    //default_log_vprintf = esp_log_set_vprintf(custom_log_vprintf); // This is very slow might have to find a alternative

    
    display_manager_add_status_bar("Terminal");

    lastPress = (unsigned long) (esp_timer_get_time() / 1000ULL);
    closeCount = 0;
}

void terminal_view_destroy(void) {
    //esp_log_set_vprintf(default_log_vprintf);
    default_log_vprintf = NULL;
    if (terminal_view.root != NULL) {
        lv_obj_del(terminal_view.root);
        terminal_view.root = NULL;
        terminal_textarea = NULL;
    }
}

void terminal_view_add_text(const char *text) {
    if (terminal_textarea == NULL) return;
    if (text == NULL) return;
    
    lv_textarea_add_text(terminal_textarea, text);
    bool isPacket = strlen(text) == 1 && text[0] == '.';

    if (isPacket && strlen(lv_textarea_get_text(terminal_textarea)) > 500)
    {
        lv_textarea_set_text(terminal_textarea, "");
    }

    if (!isPacket)
    {
        lv_textarea_add_text(terminal_textarea, "\n");
    }
    
    lv_textarea_set_cursor_pos(terminal_textarea, LV_TEXTAREA_CURSOR_LAST);
}

void terminal_view_hardwareinput_callback(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        unsigned long now = (unsigned long) (esp_timer_get_time() / 1000ULL);
        if (now - lastPress > 1500)
        {
            closeCount = 0;
        }
        if (now - lastPress < 150)
        {
            return;
        }
        lastPress = now;

        int touch_y = event->data.touch_data.point.y;

        if (touch_y < LV_VER_RES / 3) {
            closeCount = 0;
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_up(terminal_textarea);
            }
        } else if (touch_y > (LV_VER_RES * 2) / 3) {
            closeCount = 0;
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_down(terminal_textarea);
            }
        } else if (closeCount > 5) {
            display_manager_switch_view(&options_menu_view);
            simulateCommand("stop");
            simulateCommand("stopspam");
            simulateCommand("stopdeauth");
            simulateCommand("capture -stop");
            simulateCommand("stopportal");
            simulateCommand("gpsinfo -s");
            simulateCommand("blewardriving -s");
            return;
        }
        else
        {
            printf("Close count = %d\n", closeCount);
            closeCount++;
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 1) {
            display_manager_switch_view(&options_menu_view);
            simulateCommand("stop");
            simulateCommand("stopspam");
            simulateCommand("stopdeauth");
            simulateCommand("capture -stop");
            simulateCommand("stopportal");
            simulateCommand("gpsinfo -s");
            simulateCommand("blewardriving -s");
            return;
        }

        if (button == 2)
        {
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_up(terminal_textarea);
            }
        }

        if (button == 4)
        {
            for (int i = 0; i < 5; i++) {
                lv_textarea_cursor_down(terminal_textarea);
            }
        }
    }
}

void terminal_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)terminal_view_hardwareinput_callback;
    }
}

int custom_log_vprintf(const char *fmt, va_list args) {
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len < 0) {
        return len;
    }

    terminal_view_add_text(buf);

    return len;
}

View terminal_view = {
    .root = NULL,
    .create = terminal_view_create,
    .destroy = terminal_view_destroy,
    .input_callback = terminal_view_hardwareinput_callback,
    .name = "TerminalView",
    .get_hardwareinput_callback = terminal_view_get_hardwareinput_callback
};