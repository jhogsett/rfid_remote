#ifndef HT16K33Disp_h
#define HT16K33Disp_h
// some code borrowed from https://github.com/akuzechie/HT16K33-Display-Library

#include <Wire.h>

#define DEFAULT_ADDRESS_ 0x70
#define DEFAULT_NUM_DISPLAYS 1
#define DECIMAL_PT_SEGMENT 0x4000

#define NUM_DIGITS_PER_DISPLAY 4

#define DEFAULT_SHOW_DELAY 750
#define DEFAULT_SCROLL_DELAY 200
#define SEGMENT_TEST_DELAY 10

#define HT16K33Disp_USEPROGMEM

#ifdef HT16K33Disp_USEPROGMEM
const uint16_t HT16K33Disp_FourteenSegmentASCII[96] PROGMEM = {
#else
const uint16_t HT16K33Disp_FourteenSegmentASCII[96] = {
#endif
    0b000000000000000, /* (space) */
    0b100000000000110, /* ! */
    0b000001000000010, /* " */
    0b001001011001110, /* # */
    0b001001011101101, /* $ */
    0b011111111100100, /* % */
    0b010001101011001, /* & */
    0b000001000000000, /* ' */
    0b010010000000000, /* ( */
    0b000100100000000, /* ) */
    0b011111111000000, /* * */
    0b001001011000000, /* + */
    0b000100000000000, /* , */
    0b000000011000000, /* - */
    0b100000000000000, /* . */
    0b000110000000000, /* / */
    0b000110000111111, /* 0 */
    0b000010000000110, /* 1 */
    0b000000011011011, /* 2 */
    0b000000010001111, /* 3 */
    0b000000011100110, /* 4 */
    0b010000001101001, /* 5 */
    0b000000011111101, /* 6 */
    0b000000000000111, /* 7 */
    0b000000011111111, /* 8 */
    0b000000011101111, /* 9 */
    0b001001000000000, /* : */
    0b000101000000000, /* ; */
    0b010010001000000, /* < */
    0b000000011001000, /* = */
    0b000100110000000, /* > */
    0b101000010000011, /* ? */
    0b000001010111011, /* @ */
    0b000000011110111, /* A */
    0b001001010001111, /* B */
    0b000000000111001, /* C */
    0b001001000001111, /* D */
    0b000000001111001, /* E */
    0b000000001110001, /* F */
    0b000000010111101, /* G */
    0b000000011110110, /* H */
    0b001001000001001, /* I */
    0b000000000011110, /* J */
    0b010010001110000, /* K */
    0b000000000111000, /* L */
    0b000010100110110, /* M */
    0b010000100110110, /* N */
    0b000000000111111, /* O */
    0b000000011110011, /* P */
    0b010000000111111, /* Q */
    0b010000011110011, /* R */
    0b000000011101101, /* S */
    0b001001000000001, /* T */
    0b000000000111110, /* U */
    0b000110000110000, /* V */
    0b010100000110110, /* W */
    0b010110100000000, /* X */
    0b000000011101110, /* Y */
    0b000110000001001, /* Z */
    0b000000000111001, /* [ */
    0b010000100000000, /* \ */
    0b000000000001111, /* ] */
    0b010100000000000, /* ^ */
    0b000000000001000, /* _ */
    0b000000100000000, /* ` */
    0b001000001011000, /* a */
    0b010000001111000, /* b */
    0b000000011011000, /* c */
    0b000100010001110, /* d */
    0b000100001011000, /* e */
    0b001010011000000, /* f */
    0b000010010001110, /* g */
    0b001000001110000, /* h */
    0b001000000000000, /* i */
    0b000101000010000, /* j */
    0b011011000000000, /* k */
    0b000000000110000, /* l */
    0b001000011010100, /* m */
    0b001000001010000, /* n */
    0b000000011011100, /* o */
    0b000000101110000, /* p */
    0b000010010000110, /* q */
    0b000000001010000, /* r */
    0b010000010001000, /* s */
    0b000000001111000, /* t */
    0b000000000011100, /* u */
    0b000100000010000, /* v */
    0b010100000010100, /* w */
    0b010110100000000, /* x */
    0b000001010001110, /* y */
    0b000100001001000, /* z */
    0b000100101001001, /* { */
    0b001001000000000, /* | */
    0b010010010001001, /* } */
    0b000110011000000, /* ~ */
    0b000000000000000, /* (del) */
};

class HT16K33Disp
{
public:
    HT16K33Disp(byte address = 0, byte num_displays = 1);
    void set_address(byte address, byte num_displays);

    void write(byte digit, unsigned int data);
    void segments_test();
    void clear();
    int string_length(char * string);
    void show_string(char * string, bool pad_blanks = true, bool right_justify = false);
    void simple_show_string(char * string);

    void scroll_string(char * string, int show_delay = 0, int scroll_delay = 0);
    int begin_scroll_string(char * string, int show_delay = 0, int scroll_delay = 0);
    bool step_scroll_string(unsigned long time);

    void begin_scroll_loop(int times=-1);
    bool loop_scroll_string(unsigned long time, char * string, int show_delay = 0, int scroll_delay = 0);

    uint16_t char_to_segments(char c, bool decimal_point = false);
    uint16_t convertdp(int);

    void init(byte *brightLevels);

    static const byte DEFAULT_ADDRESS = DEFAULT_ADDRESS_;

private:
    byte _address;
    byte _num_displays;
    byte _num_digits;
    int _frames;
    int _frame;
    char * _string;
    int _show_delay;
    int _scroll_delay;
    unsigned long _next_frame;
    int _scrollpos;
    bool _short_string;
    bool _loop_running;
    int _loop_times;

    /*
    *  Project     Segmented LED Display - ASCII Library
    *  @author     David Madison
    *  @link       github.com/dmadison/Segmented-LED-Display-ASCII
    *  @license    MIT - Copyright (c) 2017 David Madison
    *
    * Permission is hereby granted, free of charge, to any person obtaining a copy
    * of this software and associated documentation files (the "Software"), to deal
    * in the Software without restriction, including without limitation the rights
    * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    * copies of the Software, and to permit persons to whom the Software is
    * furnished to do so, subject to the following conditions:
    *
    * The above copyright notice and this permission notice shall be included in
    * all copies or substantial portions of the Software.
    */
};
#endif