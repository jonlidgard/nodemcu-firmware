#include "driver/spi.h"


/******************************************************************************
 * FunctionName : spi_lcd_mode_init
 * Description  : SPI master initial function for driving LCD TM035PDZV36
 * Parameters   : uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
*******************************************************************************/
void spi_lcd_mode_init(uint8 spi_no)
{
	uint32 regvalue; 
	if(spi_no>1) 		return; //handle invalid input number
	//bit9 of PERIPHS_IO_MUX should be cleared when HSPI clock doesn't equal CPU clock
	//bit8 of PERIPHS_IO_MUX should be cleared when SPI clock doesn't equal CPU clock
	if(spi_no==SPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005); //clear bit9,and bit8
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);//configure io to spi mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);//configure io to spi mode	
	}else if(spi_no==HSPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); //clear bit9
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode	
	}			

	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD|SPI_USR_COMMAND);
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);
	// SPI clock=CPU clock/8
	WRITE_PERI_REG(SPI_CLOCK(spi_no), 
					((1&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)|
					((3&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
					((1&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
					((3&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S)); //clear bit 31,set SPI clock div
	
}
/******************************************************************************
 * FunctionName : spi_lcd_9bit_write
 * Description  : SPI 9bits transmission function for driving LCD TM035PDZV36
 * Parameters   : 	uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
 *				uint8 high_bit - first high bit of the data, 0 is for "0",the other value 1-255 is for "1"
 *				uint8 low_8bit- the rest 8bits of the data.
*******************************************************************************/
void spi_lcd_9bit_write(uint8 spi_no,uint8 high_bit,uint8 low_8bit)
{
	uint32 regvalue;
	uint8 bytetemp;
	if(spi_no>1) 		return; //handle invalid input number
	
	if(high_bit)		bytetemp=(low_8bit>>1)|0x80;
	else				bytetemp=(low_8bit>>1)&0x7f;
	
	regvalue= ((8&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S)|((uint32)bytetemp);		//configure transmission variable,9bit transmission length and first 8 command bit 
	if(low_8bit&0x01) 	regvalue|=BIT15;        //write the 9th bit
	while(READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR);		//waiting for spi module available
	WRITE_PERI_REG(SPI_USER2(spi_no), regvalue);				//write  command and command length into spi reg
	SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);		//transmission start
}

/******************************************************************************
 * FunctionName : spi_master_init
 * Description  : SPI master initial function for common byte units transmission
 * Parameters   : uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
*******************************************************************************/
void spi_master_init(uint8 spi_no, unsigned cpol, unsigned cpha, uint32_t clock_div)
{
	uint32 regvalue; 

	if(spi_no>1) 		return; //handle invalid input number

	if(spi_no==SPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005); 
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);//configure io to spi mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);//configure io to spi mode	
	}
	else if(spi_no==HSPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); 
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode	
	}

	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD|SPI_DOUTDIN|SPI_USR_MOSI|
			  SPI_RD_BYTE_ORDER|SPI_WR_BYTE_ORDER);

	//set clock polarity
	// TODO: This doesn't work
	//if (cpol == 1) {
	//    SET_PERI_REG_MASK(SPI_CTRL2(spi_no), (SPI_CK_OUT_HIGH_MODE<<SPI_CK_OUT_HIGH_MODE_S));
	//} else {
	//    SET_PERI_REG_MASK(SPI_CTRL2(spi_no), (SPI_CK_OUT_LOW_MODE<<SPI_CK_OUT_LOW_MODE_S));
	//}
	//os_printf("SPI_CTRL2 is %08x\n",READ_PERI_REG(SPI_CTRL2(spi_no)));

	//set clock phase
	if (cpha == 1) {
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE|SPI_CK_I_EDGE);
	} else {
    		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE|SPI_CK_I_EDGE);
	}

	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE|SPI_USR_MISO|
			    SPI_USR_ADDR|SPI_USR_COMMAND|SPI_USR_DUMMY);

	//clear Dual or Quad lines transmission mode
	CLEAR_PERI_REG_MASK(SPI_CTRL(spi_no), SPI_QIO_MODE|SPI_DIO_MODE|SPI_DOUT_MODE|SPI_QOUT_MODE);

	// SPI clock = CPU clock / clock_div
	// the divider needs to be a multiple of 2 to get a proper waveform shape
	if ((clock_div & 0x01) != 0) {
		// bump the divider to the next N*2
		clock_div += 0x02;
	}
	clock_div >>= 1;
	// clip to maximum possible CLKDIV_PRE
	clock_div = clock_div > SPI_CLKDIV_PRE ? SPI_CLKDIV_PRE : clock_div - 1;

	WRITE_PERI_REG(SPI_CLOCK(spi_no), 
					((clock_div&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)|
					((1&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
					((0&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
					((1&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S)); //clear bit 31,set SPI clock div

	//set 8bit output buffer length, the buffer is the low 8bit of register"SPI_FLASH_C0"
	WRITE_PERI_REG(SPI_USER1(spi_no), 
					((7&SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S)|
					((7&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S));
}

/******************************************************************************
 * FunctionName : spi_mast_set_mosi
 * Description  : Enter provided data into MOSI buffer.
 *                The data is regarded as a sequence of bits with length 'bitlen'.
 *                It will be written left-aligned starting from position 'offset'.
 * Parameters   :   uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
 *                  uint8 offset - offset into MOSI buffer (number of bits)
 *                  uint8 bitlen - valid number of bits in data
 *                  uint32 data  - data to be written into buffer
*******************************************************************************/
void spi_mast_set_mosi(uint8 spi_no, uint8 offset, uint8 bitlen, uint32 data)
{
    uint8  wn, wn_offset, wn_bitlen;
    uint32 wn_data;

    if (spi_no > 1)
        return; // handle invalid input number
    if (bitlen > 32)
        return; // handle invalid input number

    while(READ_PERI_REG(SPI_CMD(spi_no)) & SPI_USR);

    // determine which SPI_Wn register is addressed
    wn = offset >> 5;
    if (wn > 15)
        return; // out of range
    wn_offset = offset & 0x1f;
    if (32 - wn_offset < bitlen)
    {
        // splitting required
        wn_bitlen = 32 - wn_offset;
        wn_data   = data >> (bitlen - wn_bitlen);
    }
    else
    {
        wn_bitlen = bitlen;
        wn_data   = data;
    }

    do
    {
        // write payload data to SPI_Wn
        SET_PERI_REG_BITS(REG_SPI_BASE(spi_no) +0x40 + wn*4, BIT(wn_bitlen) - 1, wn_data, 32 - (wn_offset + wn_bitlen));

        // prepare writing of dangling data part
        wn += 1;
        wn_offset = 0;
        if (wn <= 15)
            bitlen -= wn_bitlen;
        else
            bitlen = 0; // force abort
        wn_bitlen = bitlen;
        wn_data   = data;
    } while (bitlen > 0);
}

/******************************************************************************
 * FunctionName : spi_mast_get_miso
 * Description  : Retrieve data from MISO buffer.
 *                The data is regarded as a sequence of bits with length 'bitlen'.
 *                It will be read starting left-aligned from position 'offset'.
 * Parameters   :   uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
 *                  uint8 offset - offset into MISO buffer (number of bits)
 *                  uint8 bitlen - requested number of bits in data
*******************************************************************************/
uint32 spi_mast_get_miso(uint8 spi_no, uint8 offset, uint8 bitlen)
{
    uint8  wn, wn_offset, wn_bitlen;
    uint32 wn_data = 0;

    if (spi_no > 1)
        return 0; // handle invalid input number

    // determine which SPI_Wn register is addressed
    wn = offset >> 5;
    if (wn > 15)
        return 0; // out of range
    wn_offset = offset & 0x1f;

    if (bitlen > (32 - wn_offset))
    {
        // splitting required
        wn_bitlen = 32 - wn_offset;
    }
    else
    {
        wn_bitlen = bitlen;
    }

    do
    {
        wn_data |= (READ_PERI_REG(REG_SPI_BASE(spi_no) +0x40 + wn*4) >> (32 - (wn_offset + wn_bitlen))) & (BIT(wn_bitlen) - 1);

        // prepare reading of dangling data part
        wn_data <<= bitlen - wn_bitlen;
        wn += 1;
        wn_offset = 0;
        if (wn <= 15)
            bitlen -= wn_bitlen;
        else
            bitlen = 0; // force abort
        wn_bitlen = bitlen;
    } while (bitlen > 0);

    return wn_data;
}

/******************************************************************************
 * FunctionName : spi_mast_transaction
 * Description  : Start a transaction and wait for completion.
 * Parameters   :   uint8  spi_no       - SPI module number, Only "SPI" and "HSPI" are valid
 *                  uint8  cmd_bitlen   - Valid number of bits in cmd_data.
 *                  uint16 cmd_data     - Command data.
 *                  uint8  addr_bitlen  - Valid number of bits in addr_data.
 *                  uint32 addr_data    - Address data.
 *                  uint8  mosi_bitlen  - Valid number of bits in MOSI buffer.
 *                  uint8  dummy_bitlen - Number of dummy cycles.
 *                  uint8  miso_bitlen  - number of bits to be captured in MISO buffer.
*******************************************************************************/
void spi_mast_transaction(uint8 spi_no, uint8 cmd_bitlen, uint16 cmd_data, uint8 addr_bitlen, uint32 addr_data,
                          uint8 mosi_bitlen, uint8 dummy_bitlen, uint8 miso_bitlen)
{
    if (spi_no > 1)
        return; // handle invalid input number

    while(READ_PERI_REG(SPI_CMD(spi_no)) & SPI_USR);

    // default disable COMMAND, ADDR, MOSI, DUMMY, MISO
    CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_MOSI|SPI_USR_DUMMY|SPI_USR_MISO);
    // default set bit lengths
    WRITE_PERI_REG(SPI_USER1(spi_no),
                   ((addr_bitlen - 1)  & SPI_USR_ADDR_BITLEN)    << SPI_USR_ADDR_BITLEN_S    |
                   ((mosi_bitlen - 1)  & SPI_USR_MOSI_BITLEN)    << SPI_USR_MOSI_BITLEN_S    |
                   ((dummy_bitlen - 1) & SPI_USR_DUMMY_CYCLELEN) << SPI_USR_DUMMY_CYCLELEN_S |
                   ((miso_bitlen - 1)  & SPI_USR_MISO_BITLEN)    << SPI_USR_MISO_BITLEN_S);

    // handle the transaction components
    if (cmd_bitlen > 0)
    {
        uint16 cmd = cmd_data << (16 - cmd_bitlen); // align to MSB
        cmd = (cmd >> 8) | (cmd << 8);              // swap byte order
        WRITE_PERI_REG(SPI_USER2(spi_no),
                       ((cmd_bitlen - 1 & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S) |
                       (cmd & SPI_USR_COMMAND_VALUE));
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND);
    }
    if (addr_bitlen > 0)
    {
        WRITE_PERI_REG(SPI_ADDR(spi_no), addr_data << (32 - addr_bitlen));
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_ADDR);
    }
    if (mosi_bitlen > 0)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI);
    }
    if (dummy_bitlen > 0)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_DUMMY);
    }
    if (miso_bitlen > 0)
    {
        SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MISO);
    }

    // start transaction
    SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);

    while(READ_PERI_REG(SPI_CMD(spi_no)) & SPI_USR);
}


/******************************************************************************
 * FunctionName : spi_byte_write_espslave
 * Description  : SPI master 1 byte transmission function for esp8266 slave,
 * 			transmit 1byte data to esp8266 slave buffer needs 16bit transmission ,
 * 			first byte is command 0x04 to write slave buffer, second byte is data
 * Parameters   : 	uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
 *				uint8 data- transmitted data
*******************************************************************************/
void spi_byte_write_espslave(uint8 spi_no,uint8 data)
 {
	uint32 regvalue;

	if(spi_no>1) 		return; //handle invalid input number

	while(READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR);
	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI);
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MISO|SPI_USR_ADDR|SPI_USR_DUMMY);

	//SPI_FLASH_USER2 bit28-31 is cmd length,cmd bit length is value(0-15)+1,
	// bit15-0 is cmd value.
	//0x70000000 is for 8bits cmd, 0x04 is eps8266 slave write cmd value
	WRITE_PERI_REG(SPI_USER2(spi_no), 
					((7&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S)|4);
	WRITE_PERI_REG(SPI_W0(spi_no), (uint32)(data));
	SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
 }
/******************************************************************************
 * FunctionName : spi_byte_read_espslave
 * Description  : SPI master 1 byte read function for esp8266 slave,
 * 			read 1byte data from esp8266 slave buffer needs 16bit transmission ,
 * 			first byte is command 0x06 to read slave buffer, second byte is recieved data
 * Parameters   : 	uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
 *				uint8* data- recieved data address
*******************************************************************************/
  void spi_byte_read_espslave(uint8 spi_no,uint8 *data)
 {
	uint32 regvalue;

	if(spi_no>1) 		return; //handle invalid input number

	while(READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR);

	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MISO);
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI|SPI_USR_ADDR|SPI_USR_DUMMY);
		//SPI_FLASH_USER2 bit28-31 is cmd length,cmd bit length is value(0-15)+1,
	// bit15-0 is cmd value.
	//0x70000000 is for 8bits cmd, 0x06 is eps8266 slave read cmd value
	WRITE_PERI_REG(SPI_USER2(spi_no), 
					((7&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S)|6);
	SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
	
	while(READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR);
	*data=(uint8)(READ_PERI_REG(SPI_W0(spi_no))&0xff);
 }

/******************************************************************************
 * FunctionName : spi_slave_init
 * Description  : SPI slave mode initial funtion, including mode setting,
 * 			IO setting, transmission interrupt opening, interrupt function registration
 * Parameters   : 	uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
*******************************************************************************/
void spi_slave_init(uint8 spi_no)
{
    uint32 regvalue; 
    if(spi_no>1)
        return; //handle invalid input number

    //clear bit9,bit8 of reg PERIPHS_IO_MUX
    //bit9 should be cleared when HSPI clock doesn't equal CPU clock
    //bit8 should be cleared when SPI clock doesn't equal CPU clock
    ////WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105); //clear bit9//TEST
    if(spi_no==SPI){
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);//configure io to spi mode
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);//configure io to spi mode	
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);//configure io to spi mode	
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);//configure io to spi mode	
    }else if(spi_no==HSPI){
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode	
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode	
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode	
    }

    //regvalue=READ_PERI_REG(SPI_FLASH_SLAVE(spi_no));
    //slave mode,slave use buffers which are register "SPI_FLASH_C0~C15", enable trans done isr
    //set bit 30 bit 29 bit9,bit9 is trans done isr mask
    SET_PERI_REG_MASK(	SPI_SLAVE(spi_no), 
    						SPI_SLAVE_MODE|SPI_SLV_WR_RD_BUF_EN|
                                         	SPI_SLV_WR_BUF_DONE_EN|SPI_SLV_RD_BUF_DONE_EN|
                                         	SPI_SLV_WR_STA_DONE_EN|SPI_SLV_RD_STA_DONE_EN|
                                         	SPI_TRANS_DONE_EN);
    //disable general trans intr 
    //CLEAR_PERI_REG_MASK(SPI_SLAVE(spi_no),SPI_TRANS_DONE_EN);

    CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);//disable flash operation mode
    SET_PERI_REG_MASK(SPI_USER(spi_no),SPI_USR_MISO_HIGHPART);//SLAVE SEND DATA BUFFER IN C8-C15 


//////**************RUN WHEN SLAVE RECIEVE*******************///////
   //tow lines below is to configure spi timing.
    SET_PERI_REG_MASK(SPI_CTRL2(spi_no),(0x2&SPI_MOSI_DELAY_NUM)<<SPI_MOSI_DELAY_NUM_S) ;//delay num
    os_printf("SPI_CTRL2 is %08x\n",READ_PERI_REG(SPI_CTRL2(spi_no)));
    WRITE_PERI_REG(SPI_CLOCK(spi_no), 0);


    
/////***************************************************//////	

    //set 8 bit slave command length, because slave must have at least one bit addr, 
    //8 bit slave+8bit addr, so master device first 2 bytes can be regarded as a command 
    //and the  following bytes are datas, 
    //32 bytes input wil be stored in SPI_FLASH_C0-C7
    //32 bytes output data should be set to SPI_FLASH_C8-C15
    WRITE_PERI_REG(SPI_USER2(spi_no), (0x7&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S); //0x70000000

    //set 8 bit slave recieve buffer length, the buffer is SPI_FLASH_C0-C7
    //set 8 bit slave status register, which is the low 8 bit of register "SPI_FLASH_STATUS"
    SET_PERI_REG_MASK(SPI_SLAVE1(spi_no),  ((0xff&SPI_SLV_BUF_BITLEN)<< SPI_SLV_BUF_BITLEN_S)|
                                                                                        ((0x7&SPI_SLV_STATUS_BITLEN)<<SPI_SLV_STATUS_BITLEN_S)|
                                                                                       ((0x7&SPI_SLV_WR_ADDR_BITLEN)<<SPI_SLV_WR_ADDR_BITLEN_S)|
                                                                                       ((0x7&SPI_SLV_RD_ADDR_BITLEN)<<SPI_SLV_RD_ADDR_BITLEN_S));
    
    SET_PERI_REG_MASK(SPI_PIN(spi_no),BIT19);//BIT19   

    //maybe enable slave transmission liston 
    SET_PERI_REG_MASK(SPI_CMD(spi_no),SPI_USR);
    //register level2 isr function, which contains spi, hspi and i2s events
    ETS_SPI_INTR_ATTACH(spi_slave_isr_handler,NULL);
    //enable level2 isr, which contains spi, hspi and i2s events
    ETS_SPI_INTR_ENABLE(); 
}





/* =============================================================================================
 * code below is for spi slave r/w testcase with 2 r/w state lines connected to the spi master mcu
 * replace with your own process functions
 * find "add system_os_post here" in spi_slave_isr_handler.
 * =============================================================================================
 */







#ifdef SPI_SLAVE_DEBUG
 /******************************************************************************
 * FunctionName : hspi_master_readwrite_repeat
 * Description  : SPI master test  function for reading and writing esp8266 slave buffer,
 			the function uses HSPI module 
*******************************************************************************/
os_timer_t timer2;

void hspi_master_readwrite_repeat(void)
{
	static uint8 data=0;
	uint8 temp;

	os_timer_disarm(&timer2);
	spi_byte_read_espslave(HSPI,&temp);

	temp++;
	spi_byte_write_espslave(HSPI,temp);
       os_timer_setfn(&timer2, (os_timer_func_t *)hspi_master_readwrite_repeat, NULL);
       os_timer_arm(&timer2, 500, 0);
}
#endif


/******************************************************************************
 * FunctionName : spi_slave_isr_handler
 * Description  : SPI interrupt function, SPI HSPI and I2S interrupt can trig this function
 			   some basic operation like clear isr flag has been done, 
 			   and it is availible	for adding user coder in the funtion
 * Parameters  : void *para- function parameter address, which has been registered in function spi_slave_init
*******************************************************************************/
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
static uint8 spi_data[32] = {0};
static uint8 idx = 0;
static uint8 spi_flg = 0;
#define SPI_MISO
#define SPI_QUEUE_LEN 8
os_event_t * spiQueue;
#define MOSI  0
#define MISO  1
#define STATUS_R_IN_WR 2
#define STATUS_W  3
#define TR_DONE_ALONE  4
#define WR_RD 5
#define DATA_ERROR 6
#define STATUS_R_IN_RD 7
//init the two intr line of slave
//gpio0: wr_ready ,and  
//gpio2: rd_ready , controlled by slave
void ICACHE_FLASH_ATTR
    gpio_init()
{

    	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
    	GPIO_OUTPUT_SET(0, 1);
    	GPIO_OUTPUT_SET(2, 0);
	//GPIO_OUTPUT_SET(4, 1);
}



void spi_slave_isr_handler(void *para)
{
	uint32 regvalue,calvalue;
    	static uint8 state =0;
	uint32 recv_data,send_data;

	if(READ_PERI_REG(0x3ff00020)&BIT4){		
        //following 3 lines is to clear isr signal
        	CLEAR_PERI_REG_MASK(SPI_SLAVE(SPI), 0x3ff);
    	}else if(READ_PERI_REG(0x3ff00020)&BIT7){ //bit7 is for hspi isr,
        	regvalue=READ_PERI_REG(SPI_SLAVE(HSPI));
         	CLEAR_PERI_REG_MASK(SPI_SLAVE(HSPI),  
								SPI_TRANS_DONE_EN|
								SPI_SLV_WR_STA_DONE_EN|
								SPI_SLV_RD_STA_DONE_EN|
								SPI_SLV_WR_BUF_DONE_EN|
								SPI_SLV_RD_BUF_DONE_EN);
        	SET_PERI_REG_MASK(SPI_SLAVE(HSPI), SPI_SYNC_RESET);
        	CLEAR_PERI_REG_MASK(SPI_SLAVE(HSPI),  
								SPI_TRANS_DONE|
								SPI_SLV_WR_STA_DONE|
								SPI_SLV_RD_STA_DONE|
								SPI_SLV_WR_BUF_DONE|
								SPI_SLV_RD_BUF_DONE); 
		SET_PERI_REG_MASK(SPI_SLAVE(HSPI),  
								SPI_TRANS_DONE_EN|
								SPI_SLV_WR_STA_DONE_EN|
								SPI_SLV_RD_STA_DONE_EN|
								SPI_SLV_WR_BUF_DONE_EN|
								SPI_SLV_RD_BUF_DONE_EN);

		if(regvalue&SPI_SLV_WR_BUF_DONE){ 
            		GPIO_OUTPUT_SET(0, 0);
            		idx=0;
            		while(idx<8){
            			recv_data=READ_PERI_REG(SPI_W0(HSPI)+(idx<<2));
            			spi_data[idx<<2] = recv_data&0xff;
            			spi_data[(idx<<2)+1] = (recv_data>>8)&0xff;
            			spi_data[(idx<<2)+2] = (recv_data>>16)&0xff;
            			spi_data[(idx<<2)+3] = (recv_data>>24)&0xff;
            			idx++;
			}
			//add system_os_post here
            		GPIO_OUTPUT_SET(0, 1);
		}
        	if(regvalue&SPI_SLV_RD_BUF_DONE){
			//it is necessary to call GPIO_OUTPUT_SET(2, 1), when new data is preped in SPI_W8-15 and needs to be sended.
           		GPIO_OUTPUT_SET(2, 0);
			//add system_os_post here
			//system_os_post(USER_TASK_PRIO_1,WR_RD,regvalue);

        	}
    
    }else if(READ_PERI_REG(0x3ff00020)&BIT9){ //bit7 is for i2s isr,

    }
}


#ifdef SPI_SLAVE_DEBUG

void ICACHE_FLASH_ATTR
    set_miso_data()
{
    if(GPIO_INPUT_GET(2)==0){
        WRITE_PERI_REG(SPI_W8(HSPI),0x05040302);
        WRITE_PERI_REG(SPI_W9(HSPI),0x09080706);
        WRITE_PERI_REG(SPI_W10(HSPI),0x0d0c0b0a);
        WRITE_PERI_REG(SPI_W11(HSPI),0x11100f0e);

        WRITE_PERI_REG(SPI_W12(HSPI),0x15141312);
        WRITE_PERI_REG(SPI_W13(HSPI),0x19181716);
        WRITE_PERI_REG(SPI_W14(HSPI),0x1d1c1b1a);
        WRITE_PERI_REG(SPI_W15(HSPI),0x21201f1e);
        GPIO_OUTPUT_SET(2, 1);
    }
}



void ICACHE_FLASH_ATTR
    disp_spi_data()
{
    uint8 i = 0;
    for(i=0;i<32;i++){
        os_printf("data %d : 0x%02x\n\r",i,spi_data[i]);
    }
    //os_printf("d31:0x%02x\n\r",spi_data[31]);
}


void ICACHE_FLASH_ATTR
    spi_task(os_event_t *e)
{
    uint8 data;
    switch(e->sig){
       case MOSI:
            	disp_spi_data();
            	break;
	case STATUS_R_IN_WR :
		os_printf("SR ERR in WRPR,Reg:%08x \n",e->par);
		break;
	case STATUS_W:
		os_printf("SW ERR,Reg:%08x\n",e->par);
		break;	
	case TR_DONE_ALONE:
		os_printf("TD ALO ERR,Reg:%08x\n",e->par);
		break;	
	case WR_RD:
		os_printf("WR&RD ERR,Reg:%08x\n",e->par);
		break;	
	case DATA_ERROR:
		os_printf("Data ERR,Reg:%08x\n",e->par);
		break;
	case STATUS_R_IN_RD :
		os_printf("SR ERR in RDPR,Reg:%08x\n",e->par);
		break;	
        default:
            break;
    }
}

void ICACHE_FLASH_ATTR
    spi_task_init(void)
{
    spiQueue = (os_event_t*)os_malloc(sizeof(os_event_t)*SPI_QUEUE_LEN);
    system_os_task(spi_task,USER_TASK_PRIO_1,spiQueue,SPI_QUEUE_LEN);
}

os_timer_t spi_timer_test;

void ICACHE_FLASH_ATTR
    spi_test_init()
{
    os_printf("spi init\n\r");
    spi_slave_init(HSPI);
    os_printf("gpio init\n\r");
    gpio_init();
    os_printf("spi task init \n\r");
    spi_task_init();
#ifdef SPI_MISO
    os_printf("spi miso init\n\r");
    set_miso_data();
#endif
    
    //os_timer_disarm(&spi_timer_test);
    //os_timer_setfn(&spi_timer_test, (os_timer_func_t *)set_miso_data, NULL);//wjl
    //os_timer_arm(&spi_timer_test,50,1);
}

#endif


