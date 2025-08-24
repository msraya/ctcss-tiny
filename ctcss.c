/*
 * CTCSS GENERATOR EA7EE 2025
 * GPL v2 LICENSE
 */
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// Sinus one quadrant
const uint8_t sine64[64] = {
      0,  3,  6,  9,  12,  15,  18,  21,
     24,  27,  30,  33,  36,  39,  42,  45,
     48,  51,  54,  57,  60,  63,  66,  69,
     72,  75,  78,  81,  84,  87,  90,  93,
     96,  99, 102, 105, 108, 111, 114, 117,
    120, 123, 126, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127
};

// CTCSS Table standard
const uint32_t ctcss_table[] = {
    6260340u,  /* 0x005F8674 */ /* 67.0 Hz  */
    6475248u,  /* 0x0062CDF0 */ /* 69.3 Hz  */
    6718186u,  /* 0x006682EA */ /* 71.9 Hz  */
    6951781u,  /* 0x006A1365 */ /* 74.4 Hz  */
    7194720u,  /* 0x006DC860 */ /* 77.0 Hz  */
    7447002u,  /* 0x0071A1DA */ /* 79.7 Hz  */
    7708628u,  /* 0x00759FD4 */ /* 82.5 Hz  */
    7979598u,  /* 0x0079C24E */ /* 85.4 Hz  */
    8269256u,  /* 0x007E2DC8 */ /* 88.5 Hz  */
    8549569u,  /* 0x008274C1 */ /* 91.5 Hz  */
    8857915u,  /* 0x0087293B */ /* 94.8 Hz  */
    9100853u,  /* 0x008ADE35 */ /* 97.4 Hz  */
    9343792u,  /* 0x008E9330 */ /*100.0 Hz  */
    9670824u,  /* 0x009390A8 */ /*103.5 Hz  */
    10016545u, /* 0x0098D721 */ /*107.2 Hz  */
    10362265u, /* 0x009E1D99 */ /*110.9 Hz  */
    10726673u, /* 0x00A3AD11 */ /*114.8 Hz  */
    11100425u, /* 0x00A96109 */ /*118.8 Hz  */
    11492864u, /* 0x00AF5E00 */ /*123.0 Hz  */
    11894647u, /* 0x00B57F77 */ /*127.3 Hz  */
    12315117u, /* 0x00BBE9ED */ /*131.8 Hz  */
    12754276u, /* 0x00C29D64 */ /*136.5 Hz  */
    13202778u, /* 0x00C9755A */ /*141.3 Hz  */
    13660623u, /* 0x00D071CF */ /*146.2 Hz  */
    14146501u, /* 0x00D7DBC5 */ /*151.4 Hz  */
    14641722u, /* 0x00DF6A3A */ /*156.7 Hz  */
    14931379u, /* 0x00E3D5B3 */ /*159.8 Hz  */
    15155630u, /* 0x00E741AE */ /*162.2 Hz  */
    15688226u, /* 0x00EF6222 */ /*167.9 Hz  */
    16005915u, /* 0x00F43B1B */ /*171.3 Hz  */
    16239510u, /* 0x00F7CB96 */ /*173.8 Hz  */
    16566543u, /* 0x00FCC90F */ /*177.3 Hz  */
    16809481u, /* 0x01007E09 */ /*179.9 Hz  */
    17398140u, /* 0x0109797C */ /*186.2 Hz  */
    18014830u, /* 0x0112E26E */ /*192.8 Hz  */
    19014616u, /* 0x012223D8 */ /*203.5 Hz  */
    19687369u, /* 0x012C67C9 */ /*210.7 Hz  */
    20378810u, /* 0x0136F4BA */ /*218.1 Hz  */
    21088938u, /* 0x0141CAAA */ /*225.7 Hz  */
    21827097u, /* 0x014D0E19 */ /*233.6 Hz  */
    22593288u, /* 0x0158BF08 */ /*241.8 Hz  */
    23387511u, /* 0x0164DD77 */ /*250.3 Hz  */
    24293858u  /* 0x0172B1E2 */ /*260.0 Hz  */
};

// Acumulador de fase de 32 bits
volatile uint32_t phaccu = 0;
// Incremento de fase
volatile uint32_t tword_m = 0;

void setup(void) {
    OSCCAL = 218;  //Fine tune for CTCSS output, was 214
    // 0-255 Oscillator Frequency calibration value: 
	// 0 for ~3055KHz, 255 for ~11.9MHz, 214 for ~10MHz, default ~7789KHz
	// Internal 8MHz, prescaler 1
	CCP    = 0xD8;
	CLKMSR = 0x00;
	CCP    = 0xD8;
	CLKPSR = 0X00;

    // PB1 config sine output, PB2 enable at low input
    DDRB = (1 << PB1);
    PUEB = (1 << PB2);

    // Set PWM output 
    TCCR0A = _BV(COM0B1) | _BV(WGM00);              
    TCCR0B = _BV(WGM02) | _BV(CS00);   

    // Timer0 interrupt
    TIMSK0 = (1 << TOIE0);

	// ADC Setup
    ADMUX = 0;
    DIDR0 = (1<<ADC0D);
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);

    // Global Interrupt Enable
    sei();
}

// ISR Timer0 Overflow
ISR(TIM0_OVF_vect) {
    // update phase accu
    phaccu += tword_m;
    // Get MSB 8 bits of phaccu as base index
    uint8_t phase = phaccu >> 24;  // 0..255
    uint8_t quadrant = (phase >> 6) & 0x03; // 0..3 (each 64 samples)
    uint8_t idx = phase & 0x3F;    // index into table 0..63
    uint8_t val;

    switch (quadrant) {
        case 0:
            val = 128 + sine64[idx];
            break;
        case 1:
            val = 128 + sine64[63 - idx];
            break;
        case 2:
            val = 128 - sine64[idx];
            break;
        case 3:
            val = 128 - sine64[63 - idx];
            break;
    }   

    OCR0B = val;
}

// tword = fout * 2^32/45966;
int main(void) {

    tword_m = 22593288; // example
    
    setup();

    while (1) {
        if (PINB & (1<<PB2)) TCCR0A &= ~(_BV(COM0B1));
        else TCCR0A |= _BV(COM0B1);
        ADCSRA |= (1<<ADSC);
        while(ADCSRA & (1<<ADSC));
        uint16_t adc_val = ADCL;
        uint8_t code = (adc_val * 43) >> 8;  // 0..42
        cli();
        tword_m = ctcss_table[code];
        sei();
    }
}

