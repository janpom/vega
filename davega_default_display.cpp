/*
    Copyright 2018 Jan Pomikalek <jan.pomikalek@gmail.com>

    This file is part of the DAVEga firmware.

    DAVEga firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DAVEga firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DAVEga firmware.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "davega_default_display.h"
#include "davega_display.h"
#include "vesc_comm.h"
#include <TFT_22_ILI9225.h>

#define BATTERY_INDICATOR_CELL_WIDTH 14
#define BATTERY_INDICATOR_CELL_HEIGHT 15

#define SPEED_INDICATOR_CELL_WIDTH 10
#define SPEED_INDICATOR_CELL_HEIGHT 11

#define KM_PER_MILE 0.621371

#define LEN(X) (sizeof(X) / sizeof(X[0]))

typedef struct {
    uint8_t x;
    uint8_t y;
} Point;

const Point PROGMEM BATTERY_INDICATOR_CELLS[] = {
    // left column
    {0, 204},  {0, 187},  {0, 170},  {0, 153},  {0, 136},
    {0, 119},  {0, 102},  {0, 85},  {0, 68},  {0, 51},
    {0, 34},  {0, 17},
    // top row
    {0, 0},  {16, 0},  {32, 0},  {48, 0},  {64, 0},
    {80, 0},  {96, 0},  {112, 0},  {128, 0},  {144, 0},
    {160, 0},
    // right column
    {160, 17},  {160, 34},  {160, 51},  {160, 68},  {160, 85},
    {160, 102},  {160, 119},  {160, 136},  {160, 153},  {160, 170},
    {160, 187},  {160, 204},
};

const Point PROGMEM SPEED_INDICATOR_CELLS[] = {
    // bottom-left row
    {57, 151},  {41, 151}, {25, 151},
    // left column
    {25, 134}, {25, 117}, {25, 100}, {25, 84},
    // top row
    {25, 66}, {41, 66}, {57, 66}, {73, 66}, {90, 66},
    {106, 66}, {122, 66}, {138, 66},
    // right column
    {138, 83}, {138, 100}, {138, 117}, {138, 134},
    // bottom-right row
    {138, 151}, {122, 151}, {106, 151},
};

// TODO: PROGMEM
const bool FONT_DIGITS_3x5[10][5][3] = {
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    }
};

DavegaDefaultDisplay::DavegaDefaultDisplay(TFT_22_ILI9225* tft, t_davega_display_config* config) {
    _tft = tft;
    _config = config;

    if (_config->imperial_units)
        _distance_speed_multiplier = KM_PER_MILE;
    else
        _distance_speed_multiplier = 1.0;
}

void DavegaDefaultDisplay::reset() {
    _tft->fillRectangle(0, 0, _tft->maxX() - 1, _tft->maxY() - 1, COLOR_BLACK);
    _draw_labels();
}

void
DavegaDefaultDisplay::_draw_digit(uint8_t digit, uint8_t x, uint8_t y, uint16_t fg_color, uint16_t bg_color, uint8_t magnify = 1) {
    for (int xx = 0; xx < 3; xx++) {
        for (int yy = 0; yy < 5; yy++) {
            uint16_t color = FONT_DIGITS_3x5[digit][yy][xx] ? fg_color : bg_color;
            int x1 = x + xx * magnify;
            int y1 = y + yy * magnify;
            _tft->fillRectangle(x1, y1, x1 + magnify - 1, y1 + magnify - 1, color);
        }
    }
}

void
DavegaDefaultDisplay::_draw_number(char *number, uint8_t x, uint8_t y, uint16_t fg_color, uint16_t bg_color, uint8_t spacing, uint8_t magnify = 1) {
    int cursor_x = x;
    int number_len = strlen(number);
    for (int i=0; i < number_len; i++) {
        char ch = number[i];
        if (ch >= '0' and ch <= '9') {
            _draw_digit(ch - '0', cursor_x, y, fg_color, bg_color, magnify);
            cursor_x += 3 * magnify + spacing;
        } else if (ch == '.') {
            _tft->fillRectangle(cursor_x, y, cursor_x + magnify - 1, y + 5 * magnify - 1, bg_color);
            _tft->fillRectangle(cursor_x, y + 4 * magnify, cursor_x + magnify - 1, y + 5 * magnify - 1, fg_color);
            cursor_x += magnify + spacing;
        } else if (ch == '-') {
            _tft->fillRectangle(cursor_x, y, cursor_x + 3 * magnify - 1, y + 5 * magnify - 1, bg_color);
            _tft->fillRectangle(cursor_x, y + 2 * magnify, cursor_x + 3 * magnify - 1, y + 3 * magnify - 1, fg_color);
            cursor_x += 3 * magnify + spacing;
        } else if (ch == ' ') {
            _tft->fillRectangle(cursor_x, y, cursor_x + 3 * magnify - 1, y + 5 * magnify - 1, bg_color);
            cursor_x += 3 * magnify + spacing;
        }
    }
}

void DavegaDefaultDisplay::_draw_labels() {
    _tft->setFont(Terminal6x8);

    _tft->drawText(36, 48, "VOLTS", COLOR_WHITE);
    _tft->drawText(132, 48, "MAH", COLOR_WHITE);

    _tft->drawText(90, 131, _config->imperial_units ? "MPH" : "KPH", COLOR_WHITE);

    _tft->drawText(23, 175, "TRIP", COLOR_WHITE);
    _tft->drawText(97, 175, "TOTAL", COLOR_WHITE);

    if (_config->imperial_units) {
        _tft->drawText(50, 208, "MILES", COLOR_WHITE);
        _tft->drawText(119, 208, "MILES", COLOR_WHITE);
    }
    else {
        _tft->drawText(70, 208, "KM", COLOR_WHITE);
        _tft->drawText(139, 208, "KM", COLOR_WHITE);
    }
}

void DavegaDefaultDisplay::set_fw_version(char* fw_version) {
    _tft->setFont(Terminal6x8);
    _tft->drawText(40, 140, fw_version, COLOR_WHITE);
}

void DavegaDefaultDisplay::set_volts(float volts) {
    if (_config->per_cell_voltage)
        _draw_volts(volts / _config->battery_cells, 2);
    else
        _draw_volts(volts, 1);
}

void DavegaDefaultDisplay::_draw_volts(float volts, uint8_t decimals) {
    char fmt[5];
    dtostrf(volts, 4, decimals, fmt);
    _draw_number(fmt, 24, 25, COLOR_WHITE, COLOR_BLACK, 2, 4);
}

void DavegaDefaultDisplay::set_mah(int32_t mah) {
    _mah = mah;
    _draw_mah(_mah, COLOR_WHITE);
}

void DavegaDefaultDisplay::set_mah_reset_progress(float progress){
    float brightness = 255.0 * (1.0 - progress);
    uint16_t color = _tft->setColor(brightness, brightness, brightness);
    _draw_mah(_mah, color);
}

void DavegaDefaultDisplay::_draw_mah(int32_t mah, uint16_t color) {
    char fmt[6];
    dtostrf(mah, 5, 0, fmt);
    _draw_number(fmt, 84, 25, color, COLOR_BLACK, 2, 4);
}

void DavegaDefaultDisplay::set_trip_distance(uint32_t meters) {
    _trip_distance = meters;
    _draw_trip_distance(_trip_distance, COLOR_WHITE);
}

void DavegaDefaultDisplay::set_trip_reset_progress(float progress) {
    float brightness = 255.0 * (1.0 - progress);
    uint16_t color = _tft->setColor(brightness, brightness, brightness);
    _draw_trip_distance(_trip_distance, COLOR_WHITE);
}

void DavegaDefaultDisplay::_draw_trip_distance(uint32_t meters, uint16_t color) {
    char fmt[7];
    dtostrf(meters / 1000.0 * _distance_speed_multiplier, 5, 2, fmt);
    _draw_number(fmt, 24, 185, color, COLOR_BLACK, 2, 4);
}

void DavegaDefaultDisplay::set_total_distance(uint32_t meters) {
    char fmt[6];
    dtostrf(meters / 1000.0 * _distance_speed_multiplier, 4, 0, fmt);
    _draw_number(fmt, 98, 185, COLOR_WHITE, COLOR_BLACK, 2, 4);
}

void DavegaDefaultDisplay::set_speed(uint8_t kph) {
    uint8_t speed = round(kph * _distance_speed_multiplier);
    if (speed >= 10)
        _draw_digit(speed / 10, 60, 91, COLOR_WHITE, COLOR_BLACK, 7);
    else
        _tft->fillRectangle(60, 91, 82, 127, COLOR_BLACK);
    _draw_digit(speed % 10, 89, 91, COLOR_WHITE, COLOR_BLACK, 7);
}

bool DavegaDefaultDisplay::_draw_battery_cell(int index, bool filled, bool redraw = false) {
    uint16_t p_word = pgm_read_word_near(BATTERY_INDICATOR_CELLS + index);
    Point *p = (Point *) &p_word;
    if (filled || redraw) {
        uint8_t cell_count = LEN(BATTERY_INDICATOR_CELLS);
        uint8_t green = (uint8_t)(255.0 / (cell_count - 1) * index);
        uint8_t red = 255 - green;
        uint16_t color = _tft->setColor(red, green, 0);
        _tft->fillRectangle(
                p->x, p->y,
                p->x + BATTERY_INDICATOR_CELL_WIDTH, p->y + BATTERY_INDICATOR_CELL_HEIGHT,
                color
        );
    }
    if (!filled) {
        _tft->fillRectangle(
                p->x + 1, p->y + 1,
                p->x + BATTERY_INDICATOR_CELL_WIDTH - 1, p->y + BATTERY_INDICATOR_CELL_HEIGHT - 1,
                COLOR_BLACK
        );
    }
}

void DavegaDefaultDisplay::update_battery_indicator(float battery_percent, bool redraw = false) {
    int cells_to_fill = round(battery_percent * LEN(BATTERY_INDICATOR_CELLS));
    if (redraw) {
        for (int i = 0; i < LEN(BATTERY_INDICATOR_CELLS); i++)
            _draw_battery_cell(i, i < cells_to_fill, true);
    }
    else {
        if (cells_to_fill > _battery_cells_filled) {
            for (int i = _battery_cells_filled; i < cells_to_fill; i++)
                _draw_battery_cell(i, true);
        }
        if (cells_to_fill < _battery_cells_filled) {
            for (int i = _battery_cells_filled - 1; i >= cells_to_fill; i--)
                _draw_battery_cell(i, false);
        }
    }
    _battery_cells_filled = cells_to_fill;
}

void DavegaDefaultDisplay::_draw_speed_cell(int index, bool filled, bool redraw = false) {
    uint16_t p_word = pgm_read_word_near(SPEED_INDICATOR_CELLS + index);
    Point *p = (Point *) &p_word;
    if (filled || redraw) {
        _tft->fillRectangle(
                p->x, p->y,
                p->x + SPEED_INDICATOR_CELL_WIDTH, p->y + SPEED_INDICATOR_CELL_HEIGHT,
                _tft->setColor(150, 150, 255)
        );
    }
    if (!filled) {
        _tft->fillRectangle(
                p->x + 1, p->y + 1,
                p->x + SPEED_INDICATOR_CELL_WIDTH - 1, p->y + SPEED_INDICATOR_CELL_HEIGHT - 1,
                COLOR_BLACK
        );
    }
}

void DavegaDefaultDisplay::update_speed_indicator(float speed_percent, bool redraw = false) {
    int cells_to_fill = round(speed_percent * LEN(SPEED_INDICATOR_CELLS));
    if (redraw) {
        for (int i = 0; i < LEN(SPEED_INDICATOR_CELLS); i++)
            _draw_speed_cell(i, i < cells_to_fill, true);
    }
    else {
        if (cells_to_fill > _speed_cells_filled) {
            for (int i = _speed_cells_filled; i < cells_to_fill; i++)
                _draw_speed_cell(i, true);
        }
        if (cells_to_fill < _speed_cells_filled) {
            for (int i = _speed_cells_filled - 1; i >= cells_to_fill; i--)
                _draw_speed_cell(i, false);
        }
    }
    _speed_cells_filled = cells_to_fill;
}

void DavegaDefaultDisplay::indicate_read_success(uint32_t duration_ms) {
    _tft->fillRectangle(85, 155, 89, 159, _tft->setColor(0, 150, 0));
    delay(duration_ms);
    _tft->fillRectangle(85, 155, 89, 159, COLOR_BLACK);
}

void DavegaDefaultDisplay::indicate_read_failure(uint32_t duration_ms) {
    _tft->fillRectangle(85, 155, 89, 159, _tft->setColor(150, 0, 0));
    delay(duration_ms);
    _tft->fillRectangle(85, 155, 89, 159, COLOR_BLACK);
}

void DavegaDefaultDisplay::set_warning(char* warning) {
    uint16_t bg_color = _tft->setColor(150, 0, 0);
    _tft->fillRectangle(0, 60, 176, 83, bg_color);
    _tft->setFont(Terminal12x16);
    _tft->setBackgroundColor(bg_color);
    _tft->drawText(5, 65, warning, COLOR_BLACK);
    _tft->setBackgroundColor(COLOR_BLACK);
}
