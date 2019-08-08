#include "greatfet_core.h"
#include <spi_bus.h>
#include <pins.h>
#include <math.h>
#include <stdlib.h>

/* length of LED string */
#define LEN (56)

/* length of buffer (56 leds + start frame) */
#define BLEN ((LEN + 1) * 4)

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
	for(i = 0; i < duration; i++){
		delay(65535);
	}
}

void write_start_frame() {
	spi_buffer[0] = 0;
	spi_buffer[1] = 0;
	spi_buffer[2] = 0;
	spi_buffer[3] = 0;
}

void purple_pulse() {
	uint32_t i;
	uint8_t brightness = 1;
	uint8_t phase = 0;

	while(1) {
		for(phase = 1; phase < 255; phase++) {
			write_start_frame();
			for(i = 4; i < BLEN; i+=4){
				spi_buffer[i] = 0xe0 | (brightness & 0x1f);
				spi_buffer[i+1] = phase; //blue
				spi_buffer[i+2] = 0; //green
				spi_buffer[i+3] = phase; //red
			}
			spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
			//delay(65535);
			long_delay(64/phase);
		}
		if(gpio_read(&dfu_button)) {
			break;
		}
		for(phase = 255; phase > 1; phase--) {
			write_start_frame();
			for(i = 4; i < BLEN; i+=4){
				spi_buffer[i] = 0xe0 | (brightness & 0x1f);
				spi_buffer[i+1] = phase; //blue
				spi_buffer[i+2] = 0; //green
				spi_buffer[i+3] = phase; //red
			}
			spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
			//delay(65535);
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
		for(j = 0; j < LEN; j++){
			write_start_frame();
			for(i = 0; i < LEN; i++){
				spi_buffer[4*(i+1)] = 0xe0 | (brightness & 0x1f);
				if(i == j) {
					spi_buffer[4*(i+1)+1] = 0; //blue
					spi_buffer[4*(i+1)+2] = 0xff; //green
					spi_buffer[4*(i+1)+3] = 0; //red
				} else {
					spi_buffer[4*(i+1)+1] = 0; //blue
					spi_buffer[4*(i+1)+2] = 0; //green
					spi_buffer[4*(i+1)+3] = 0; //red
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
	uint8_t count = 1;

	while(1) {
		active_led += rand();
		active_led %= LEN;
		write_start_frame();
		for(i = 0; i < LEN; i++){
			spi_buffer[4*(i+1)] = 0xe0 | (brightness & 0x1f);
			if(i == active_led) {
				spi_buffer[4*(i+1)+1] = 0; //blue
				spi_buffer[4*(i+1)+2] = 0xff; //green
				spi_buffer[4*(i+1)+3] = 0xff; //red
			} else {
				spi_buffer[4*(i+1)+1] = 0; //blue
				spi_buffer[4*(i+1)+2] = 0; //green
				spi_buffer[4*(i+1)+3] = 0; //red
			}
		}
		spi_bus_transfer(&spi1_target, spi_buffer, BLEN);
		long_delay(2);
		if(!count++ && gpio_read(&dfu_button)) {
			break;
		}
	}
}

void rainbow() {
	uint32_t i, j;
	uint8_t brightness = 1;

	while(1) {
		for(j = 0; j < LEN; j++){
			write_start_frame();
			for(i = 0; i < LEN; i++){
				spi_buffer[4*(i+1)] = 0xe0 | (brightness & 0x1f);
				spi_buffer[4*(i+1)+1] = ((j+i) % LEN); //blue
				spi_buffer[4*(i+1)+2] = ((j+i+19) % LEN); //green
				spi_buffer[4*(i+1)+3] = ((j+i+37) % LEN); //red
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
		purple_pulse();
		green_chase();
		rainbow();
		sparkle();
	}

	return 0;
}
