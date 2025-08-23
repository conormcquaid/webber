/* i2c - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "oled_i2c.h"
//#include "driver/i2c_master.h"
#include "driver/i2c.h"
#include "logo.h"



#define I2C_MASTER_SCL_IO    18    /*!< gpio number for I2C master clock */  /* 17 & 18 for esp32-s3 renata-tv */
#define I2C_MASTER_SDA_IO    17    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency */

#define BH1750_SENSOR_ADDR  0x23    /*!< slave address for BH1750 sensor */
#define BH1750_CMD_START    0x23    /*!< Command to set measure mode */
#define OLED_ADDR 0x3C         /*!< ESP32 slave address, you can set any 7bit value */
#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */



const uint8_t cog2_0000[8*128] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,191,255,255,255,255,255,253,254,255,255,255,255,255,255,255,255,255,255,255,255,127,187,252,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,127,255,191,127,127,191,255,247,255,191,223,255,255,255,255,255,255,223,223,223,223,255,191,191,191,191,191,191,191,255,255,127,127,223,230,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,15,255,255,255,255,255,255,247,247,247,255,127,255,63,255,159,255,223,255,255,239,255,251,255,255,255,255,253,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,255,255,255,255,255,15,255,255,252,255,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,252,255,255,255,255,255,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,247,255,255,255,255,255,255,247,255,255,223,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,255,255,255,255,253,253,255,255,255,255,251,251,255,255,255,255,247,243,255,255,255,239,239,223,223,191,255,127,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,255,255,247,254,255,255,255,125,255,255,255,255,255,251,251,255,251,255,255,255,255,255,255,255,255,255,255,255,255,253,253,249,143,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};


SemaphoreHandle_t print_mux;



// esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config, i2c_master_bus_handle_t *ret_bus_handle);

// esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle, const i2c_device_config_t *dev_config, i2c_master_dev_handle_t *ret_handle);




/**
 * @brief test code to read esp-i2c-slave
 *        We need to fill the buffer of esp slave device, then master can read them out.
 *
 * _______________________________________________________________________________________
 * | start | slave_addr + rd_bit +ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|--------------------------|----------------------|--------------------|------|
 *
 */
esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t* data_rd, size_t size)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( OLED_ADDR << 1 ) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Test code to write esp-i2c-slave
 *        Master device write data to slave(both esp32),
 *        the data will be stored in slave buffer.
 *        We can read them out from slave buffer.
 *
 * ___________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write n bytes + ack  | stop |
 * --------|---------------------------|----------------------|------|
 *
 */
esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t* data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( OLED_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}



/**
 * @brief i2c master initialization
 */
void i2c_master_init()
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
	conf.clk_flags = 0; //!!!bugfix
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 25;//I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 26;//I2C_MASTER_SCL_IO;
	//ESP32S3: SDA on IO17 and SCL on IO18
#ifdef CONFIG_IDF_TARGET_ESP32S3
    conf.sda_io_num = 17;
    conf.scl_io_num = 18;
#endif
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, 
										conf.mode, 
										I2C_MASTER_RX_BUF_DISABLE, 
										I2C_MASTER_TX_BUF_DISABLE, 0));
	// i2c_master_bus_config_t i2c_bus_config = {
    //     .clk_source = I2C_CLK_SRC_DEFAULT,
    //     .i2c_port = -1,
    //     .scl_io_num = I2C_MASTER_SCL_IO,
    //     .sda_io_num = I2C_MASTER_SDA_IO,
    //     .glitch_ignore_cnt = 7,
    // };
    

    // ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

	// i2c_device_config_t i2c_dev_config;

	// i2c_dev_config.scl_speed_hz = I2C_MASTER_FREQ_HZ; // Set SCL frequency
	// i2c_dev_config.scl_wait_us = 0; // Use default wait time
	// i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7; // Use 7-bit address length
	// i2c_dev_config.device_address = OLED_ADDR; // Set device address

	// i2c_master_dev_handle_t handle;

	// ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &i2c_dev_config, &handle));


}

void command(uint8_t dummy, uint8_t c){

	uint8_t buf[2];
	buf[0] = 0x00;
	buf[1] = c;
	/////////////i2c_master_transmit(handle , buffer, len, -1 /*timeout*/);
	i2c_master_write_slave( I2C_MASTER_NUM, buf, 2);
}

void data(uint8_t c){

	uint8_t buf[2];
	buf[0] = 0x40;
	buf[1] = c;
	i2c_master_write_slave( I2C_MASTER_NUM, buf, 2);
}

void OLED_fill(void){

	uint8_t address = 0;

	command(address, 0x21); // SSD1306_COLUMNADDR
	command(address, 0);    // column start
	command(address, 127);  // column end
	command(address, 0x22); // SSD1306_PAGEADDR
	command(address, 0);    // page start
	command(address, 7);    // page end (8 pages for 64 rows OLED)
	
	for(int y=0; y < 128; y++){
		for(int x = 0; x < 8; x++){
			data(0xAA);
		}
	}

}

void OLED_Clear(int start_line, int end_line){

	uint8_t address = 0;

	command(address, 0x21); // SSD1306_COLUMNADDR
	command(address, 0);    // column start
	command(address, 127);  // column end
	command(address, 0x22); // SSD1306_PAGEADDR
	command(address, start_line);    // page start
	command(address, end_line);    // page end (8 pages for 64 rows OLED)
	
	for(int y=0; y < 128; y++){
		for(int x = start_line; x < (end_line + 1); x++){
			data(0x00);
		}
	}

}

void OLED_init()
{
	//Heltec HTIT WB32 has reset line on 16
	gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_32, 0);
	
	i2c_master_init();

	gpio_set_level(GPIO_NUM_32, 1);
		
	uint8_t address = 0;
//{0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,0x81,
//0xCE,0xD9,0xF1,0xD8,0x40,0xA4,0xA6,0xAF};

	command(address, 0xae); // SSD1306_DISPLAYOFF
	command(address, 0xd5); // SSD1306_SETDISPLAYCLOCKDIV
	command(address, 0x80); // Suggested value 0x80
	command(address, 0xa8); // SSD1306_SETMULTIPLEX
	command(address, 0x3f); // 64px vertical resolution
//	command(address, 0x1f); // 32px vertical resolution
	command(address, 0xd3); // SSD1306_SETDISPLAYOFFSET
	command(address, 0x00); // 0 no offset
	command(address, 0x40); // SSD1306_SETSTARTLINE line #0
	command(address, 0x8d); // SSD1306_CHARGEPUMP
	command(address, 0x14); // Charge pump on
	command(address, 0x20); // SSD1306_MEMORYMODE
	command(address, 0x00); // 0x0 act like ks0108
	command(address, 0xa1); // SSD1306_SEGREMAP | 1
	command(address, 0xc8); // SSD1306_COMSCANDEC
	command(address, 0xda); // SSD1306_SETCOMPINS
	command(address, 0x12); // for 128064
//	command(address, 0x02); // for 128032
	command(address, 0x81); // SSD1306_SETCONTRAST
	
	//0xCE,0xD9,0xF1,0xD8,0x40,0xA4,0xA6,0xAF};0xCE,0xD9,0xF1,0xD8,0x40,0xA4,0xA6,0xAF};



//	command(address, 0x2f); // for 128032
	command(address, 0xff); // for 128064
	command(address, 0xd9); // SSD1306_SETPRECHARGE
	command(address, 0xf1);
	command(address, 0xdb); // SSD1306_SETVCOMDETECT
	command(address, 0x40);
	command(address, 0x2e); // SSD1306_DEACTIVATE_SCROLL
	command(address, 0xa4); // SSD1306_DISPLAYALLON_RESUME
	command(address, 0xa6); // SSD1306_NORMALDISPLAY
	
	vTaskDelay((200) / portTICK_PERIOD_MS);
	
	command(address, 0xaf); // SSD1306_DISPLAYON
	
	vTaskDelay((200) / portTICK_PERIOD_MS);
	
	OLED_fill();
}
void OLED_WriteBig( char* s, uint8_t line, uint8_t charpos){
	
	uint8_t xpos = charpos * 10;
	uint8_t c;
	uint8_t numchars = 0;
	uint8_t address = 0;
	uint8_t buffer[21];
	buffer[0] = 0x40;	
	
	while(*s && numchars < 16){
		
		numchars++;
		
		c = *s;
		if(c < 0x20) { c = 0x20; } // non-printable
		
		c = c - 0x20;	
	
		command(address, 0x21);
		command(address, xpos);
		command(address, xpos + 9);

		command(address, 0x22); 
		command(address, line);
		command(address, line+1);

		for(int i = 0; i < 20; i++){
			buffer[i+1] =  font10x16[ (c*20) + i] ;
		}	
		i2c_master_write_slave( I2C_MASTER_NUM, buffer, 21);

		xpos+=10;
		s++;
	}	

}




