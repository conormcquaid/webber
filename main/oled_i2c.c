

#include <stdio.h>
#include "oled_i2c.h"
#include "driver/i2c_master.h"
#include "logo.h"
#include "esp_log.h"
#include "stdbool.h"

#define CMD_CONTINUOUS_SCROLL_H_RIGHT       0x26
#define CMD_CONTINUOUS_SCROLL_H_LEFT        0x27
#define CMD_ONE_COLUMN_SCROLL_H_RIGHT       0x2C
#define CMD_ONE_COLUMN_SCROLL_H_LEFT        0x2D

//10.3.1 Set Fade Out and Blinking (23h) and 10.3.2 Set Zoom In (D6h) 

#define I2C_MASTER_SCL_IO    18    /*!< gpio number for I2C master clock */  /* 17 & 18 for esp32-s3 renata-tv */
#define I2C_MASTER_SDA_IO    17    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    400000     /*!< I2C master clock frequency */


#define OLED_ADDR 0x3C         /*!< ESP32 slave address, you can set any 7bit value */
#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

static const char* TAG = "oled_i2c";

i2c_master_dev_handle_t dev_handle;

uint8_t* i2c_oled_buffer;
uint8_t  oled_cmd_buffer[6];
bool inverse_mode;

void i2c_master_init()
{
    i2c_master_bus_config_t bus_config = {
		.intr_priority = 2,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
        
    };
    i2c_master_bus_handle_t bus_handle;
    
    ESP_ERROR_CHECK(i2c_new_master_bus( &bus_config , &bus_handle));

    const i2c_device_config_t dev_config={
        .scl_speed_hz = I2C_MASTER_FREQ_HZ, // Set SCL frequency
        .scl_wait_us = 1, 
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, // Use 7-bit address length
        .device_address = OLED_ADDR, // Set device address
        .flags = {
            .disable_ack_check = true, // Enable ACK check
        }
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

    i2c_oled_buffer = malloc(1 + 128 * 8); // 128 columns * 8 pages (64 rows / 8 bits per byte)

	inverse_mode = false;
  
    printf("i2c_master_bus_add_device success\n");
}


void command(uint8_t c){

	uint8_t buf[2];
    buf[0] = 0; buf[1] = c;
	ESP_ERROR_CHECK(i2c_master_transmit(dev_handle,   (uint8_t*)&buf, 2, 666));
	//ESP_LOGI(TAG, "Cmd: %02x", c);
}

// void OLED_fill(uint8_t start_line, uint8_t end_line, uint8_t* source){

// 	uint8_t cmds[] = {0x00, 0x21, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x22, 0x00, start_line, 0x00, end_line};
//     //ESP_ERROR_CHECK(i2c_master_transmit( dev_handle, cmds, sizeof(cmds), 666));

// 	// command( 0x21); // SSD1306_COLUMNADDR
// 	// command( 0);    // column start
// 	// command( 127);  // column end
// 	// command( 0x22); // SSD1306_PAGEADDR
// 	// command( start_line);    // page start
// 	// command( end_line);      // page end (8 pages for 64 rows OLED)
    
//     // henceforth comes the data...
//     uint8_t* buf = i2c_oled_buffer;
// 	*buf++ = 0x40; // data mode
    
	
//     if(!source){
// 		// make our own data buffer

//         for(int y=0; y < 128 * 8; y++){
//             *buf++ = 0x01;
//         }
//     }else{
// 		// use supplied data buffer
// 		for(int y = 0; y < 128 * 8; y++){
//             *buf++ = *source++;
// 		}

//     }
// //    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle,i2c_oled_buffer, (1 + 128 * 8), 666));
//     i2c_master_transmit_multi_buffer_info_t biff[2] = {
//         { .write_buffer = cmds,             .buffer_size = sizeof(cmds) },
//         { .write_buffer = i2c_oled_buffer,  .buffer_size = 1 + 128 * 8}
//     };
//     ESP_ERROR_CHECK(i2c_master_multi_buffer_transmit(dev_handle, biff, 2, 666));
// }


void OLED_fill(uint8_t start_line, uint8_t end_line, uint8_t* source, bool inverse){

    //uint8_t cmds[] = {0x00, 0x21, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x22, start_line, end_line};
    //ESP_ERROR_CHECK(i2c_master_transmit( dev_handle, cmds, sizeof(cmds), 666));

	command( 0x21); // SSD1306_COLUMNADDR
	command( 0);    // column start
	command( 127);  // column end
	command( 0x22); // SSD1306_PAGEADDR
	command( start_line);    // page start
	command( end_line);      // page end (8 pages for 64 rows OLED)
    
    // henceforth comes the data...
    uint8_t* buf = i2c_oled_buffer;
	*buf++ = 0x40; // data mode
    
	
    if(!source){
		// make our own data buffer

        for(int y=0; y < 128 * 8; y++){
            *buf++ = 0x01;
        }
    }else{
		// use supplied data buffer
		if(inverse){
			for(int y = 0; y < 128 * 8; y++){
				*buf++ = ~(*source++);
			}
			
		}else{
			for(int y = 0; y < 128 * 8; y++){
				*buf++ = *source++;
			}
		}

    }
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle,i2c_oled_buffer, (1 + 128 * 8), 666));
    // i2c_master_transmit_multi_buffer_info_t biff[2] = {
    //     { .write_buffer = cmds, .buffer_size = sizeof(cmds) },
    //     { .write_buffer = i2c_oled_buffer,  .buffer_size = 128 * 8}
    // };
    // ESP_ERROR_CHECK(i2c_master_multi_buffer_transmit(dev_handle, biff, 2, 666));
}

void OLED_Clear(int start_line, int end_line){

    OLED_fill(start_line, end_line, NULL, false);
}

void OLED_init()
{

	i2c_master_init();

	
//{0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,0x81,
//0xCE,0xD9,0xF1,0xD8,0x40,0xA4,0xA6,0xAF};

	command( 0xae); // SSD1306_DISPLAYOFF
	command( 0xd5); // SSD1306_SETDISPLAYCLOCKDIV
	command( 0x80); // Suggested value 0x80
	command( 0xa8); // SSD1306_SETMULTIPLEX
	command( 0x3f); // 64px vertical resolution
//	command(address, 0x1f); // 32px vertical resolution
	command( 0xd3); // SSD1306_SETDISPLAYOFFSET
	command( 0x00); // 0 no offset
	command( 0x40); // SSD1306_SETSTARTLINE line #0
	command( 0x8d); // SSD1306_CHARGEPUMP
	command( 0x14); // Charge pump on
	command( 0x20); // SSD1306_MEMORYMODE
	command( 0x00); // 0x0 act like ks0108
	command( 0xa1); // SSD1306_SEGREMAP | 1
	command( 0xc8); // SSD1306_COMSCANDEC
	command( 0xda); // SSD1306_SETCOMPINS
	command( 0x12); // for 128064
//	command(address, 0x02); // for 128032
	command( 0x81); // SSD1306_SETCONTRAST
	
	//0xCE,0xD9,0xF1,0xD8,0x40,0xA4,0xA6,0xAF};0xCE,0xD9,0xF1,0xD8,0x40,0xA4,0xA6,0xAF};



//	command(address, 0x2f); // for 128032
	command( 0xff); // for 128064
	command( 0xd9); // SSD1306_SETPRECHARGE
	command( 0xf1);
	command( 0xdb); // SSD1306_SETVCOMDETECT
	command( 0x40);
	command( 0x2e); // SSD1306_DEACTIVATE_SCROLL
	command( 0xa4); // SSD1306_DISPLAYALLON_RESUME
	command( 0xa6); // SSD1306_NORMALDISPLAY
	
	vTaskDelay((200) / portTICK_PERIOD_MS);
	
	command( 0xaf); // SSD1306_DISPLAYON
	
	vTaskDelay((200) / portTICK_PERIOD_MS);
	
	OLED_Clear(0, 7);

	// OLED_WriteBig( "Hello!", 1, 1);
	// OLED_WriteBig( "X", 127 - 9, 0);
	// OLED_WriteBig( "X", 0, 6);
	// OLED_WriteBig( "X", 127 - 9, 6);
}

void OLED_WriteBig( char* s, uint8_t line, uint8_t charpos, bool invert){
	
	uint8_t xpos = charpos * 10;
	uint8_t c;
	uint8_t numchars = 0;
	uint8_t buffer[21];
	buffer[0] = 0x40;	
	
	while(*s && numchars < 16){
		
		numchars++;
		
		c = *s;
		if(c < 0x20) { c = 0x20; } // non-printable
		
		c = c - 0x20;	

        // uint8_t commands[] = {
        //    0x00, 0x21, 
		//     xpos, 
		//     xpos + 9,
        //    0x00, 0x22, 
		//     line, 
		//     line + 1,
		//    //0x40 // data start
        // };
	
		command( 0x21);
		command( xpos);
		command( xpos + 9);

		command( 0x22); 
		command( line);
		command( line+1);

        //ESP_ERROR_CHECK(i2c_master_transmit( dev_handle, commands, sizeof(commands), 666));
        
		for(int i = 0; i < 20; i++){
			uint8_t tmp = font10x16[ (c*20) + i] ;
			if(invert){ tmp = ~tmp; }
			buffer[i+1] =  tmp;
		}	
		ESP_ERROR_CHECK(i2c_master_transmit( dev_handle, buffer, sizeof(buffer), 666));
        //i2c_master_transmit_multi_buffer_info_t biff[2] = {
//{ .write_buffer = (uint8_t*)commands, .buffer_size = sizeof(commands) },
        //    { .write_buffer = buffer,             .buffer_size = sizeof(buffer)}
        //};
        //ESP_ERROR_CHECK(i2c_master_multi_buffer_transmit(dev_handle, biff, 2, 666));    

		xpos+=10;
		s++;
	}	

}




