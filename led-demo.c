/* light show for APA102 RGB LED strip */

#include "greatfet_core.h"
#include <spi_bus.h>
#include <pins.h>
#include <math.h>
#include <stdlib.h>

/* number of LEDs in LED strip */
#define LEN (56)

/* length of SPI buffer: (start frame + number of leds) * 4 bytes per frame */
#define BLEN ((1 + LEN) * 4)

/* 'shellie' in morse code */
const uint8_t code[] = {
	1, 0, 1, 0, 1, 0, 0, 0,             // S ...
	1, 0, 1, 0, 1, 0, 1, 0, 0, 0,       // H ....
	1, 0, 0, 0,                         // E .
	1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, // L .-..
	1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, // L .-..
	1, 0, 1, 0, 0, 0,                   // I ..
	1, 0, 0, 0,                         // E .
};

uint8_t spi_buffer[BLEN];

static struct gpio_t dfu_button = GPIO(5, 7);

static spi_target_t spi1_target = {
	.bus = &spi_bus_ssp1,
};

void spi1_init(spi_target_t* const target) {
	/* configure SSP pins */
	scu_pinmux(SCU_SSP1_MISO, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_MOSI, (SCU_SSP_IO | SCU_CONF_FUNCTION5));
	scu_pinmux(SCU_SSP1_SCK,  (SCU_SSP_IO | SCU_CONF_FUNCTION1));
	scu_pinmux(SCU_SSP1_SSEL, (SCU_GPIO_FAST | SCU_CONF_FUNCTION0));
	(void) target;
}

static spiflash_driver_t spi1_target_drv = {
	.target = &spi1_target,
	.target_init = spi1_init,
};

void long_delay(const uint32_t duration) {
	uint32_t i;
	for(i = 0; duration > i; i++){
		delay(65535);
	}
}

void write_start_frame() {
	spi_buffer[0] = 0;
	spi_buffer[1] = 0;
	spi_buffer[2] = 0;
	spi_buffer[3] = 0;
}

void set_led(uint8_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
	if(LEN > index) {
		spi_buffer[4*(index+1)] = 0xe0 | (brightness & 0x1f);
		spi_buffer[4*(index+1)+1] = blue;
		spi_buffer[4*(index+1)+2] = green;
		spi_buffer[4*(index+1)+3] = red;
	}
}

void name_tag() {
	uint32_t i, j;
	uint8_t brightness = 1;

	while(1) {
		for(j = 0; LEN > j; j++){
			write_start_frame();
			for(i = 0; LEN > i; i++){
				if(code[(i+j) % LEN]) {
					set_led(i, 0, 0, 0xff, brightness);
				} else {
					set_led(i, 0, 0, 0, 0);
				}
			}
			spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
			long_delay(100);
			if(gpio_read(&dfu_button)) {
				return;
			}
		}
	}
}

void purple_pulse() {
	uint32_t i;
	uint8_t brightness = 1;
	uint8_t phase = 0;

	while(1) {
		for(phase = 1; 255 > phase; phase++) {
			write_start_frame();
			for(i = 0; i < LEN; i++){
				set_led(i, phase, 0, phase, brightness);
			}
			spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
			long_delay(64/phase);
		}
		if(gpio_read(&dfu_button)) {
			break;
		}
		for(phase = 255; 1 < phase; phase--) {
			write_start_frame();
			for(i = 0; LEN > i; i++){
				set_led(i, phase, 0, phase, brightness);
			}
			spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
			long_delay(64/phase);
		}
		if(gpio_read(&dfu_button)) {
			break;
		}
	}
}

void green_chase() {
	uint32_t i, j;
	uint8_t brightness = 1;

	while(1) {
		for(j = 0; LEN > j; j++){
			write_start_frame();
			for(i = 0; LEN > i; i++){
				if(i == j) {
					set_led(i, 0, 0xff, 0, brightness);
				} else {
					set_led(i, 0, 0, 0, 0);
				}
			}
			spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
			long_delay(10);
		}
		if(gpio_read(&dfu_button)) {
			break;
		}
	}
}

void sparkle() {
	uint32_t i;
	uint8_t brightness = 1;
	uint8_t active_led = 0;
	uint8_t button_delay = 1;

	while(1) {
		active_led += rand();
		active_led %= LEN;
		write_start_frame();
		for(i = 0; LEN > i; i++){
			if(i == active_led) {
				set_led(i, 0xff, 0xff, 0, brightness);
			} else {
				set_led(i, 0, 0, 0, 0);
			}
		}
		spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
		long_delay(2);
		if(!button_delay++ && gpio_read(&dfu_button)) {
			break;
		}
	}
}

void rainbow() {
	uint32_t i, j;
	uint8_t brightness = 1;

	while(1) {
		for(j = 0; LEN > j; j++){
			write_start_frame();
			for(i = 0; LEN > i; i++){
				set_led(i, (j+i+37) % LEN, (j+i+19) % LEN, (j+i) % LEN, brightness);
			}
			spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
			long_delay(10);
		}
		if(gpio_read(&dfu_button)) {
			break;
		}
	}
}

int main(void)
{
	//rtc_init();
	pin_setup();

	/* set up DFU button for input */
	scu_pinmux(SCU_PINMUX_BOOT2, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
	gpio_input(&dfu_button);

	ssp_config_t *config;
	config = (ssp_config_t *)&ssp1_config_spi;
	spi_bus_start(spi1_target_drv.target, config);
	spi1_init(spi1_target_drv.target);

	while(1){
		name_tag();
		purple_pulse();
		green_chase();
		rainbow();
		sparkle();
	}

	return 0;
}
