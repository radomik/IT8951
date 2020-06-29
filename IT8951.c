#include "IT8951.h"
#include "measure.h"
#include <assert.h>

#define LCDWaitForReady() \
	while (bcm2835_gpio_lev(HRDY) == 0)

static void LCDWrite(uint16_t preamble, uint16_t value) {
	LCDWaitForReady();

	bcm2835_gpio_write(CS, LOW);

	uint8_t buf[] = {preamble >> 8, preamble, value >> 8, value};
    bcm2835_spi_transfern((char*)buf, sizeof(buf));

	bcm2835_gpio_write(CS, HIGH);
}

//-----------------------------------------------------------
//Host controller function 2---Write command code to host data Bus
//-----------------------------------------------------------
#define LCDWriteCmdCode(usCmdCode) \
	LCDWrite(0x6000, usCmdCode)

//-----------------------------------------------------------
//Host controller function 3---Write Data to host data Bus
//-----------------------------------------------------------
#define LCDWriteData(usData) \
	LCDWrite(0x0000, usData)

void LCDWriteNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt)
{
	uint32_t i;

	uint16_t wPreamble	= 0x0000;

	LCDWaitForReady();

	bcm2835_gpio_write(CS,LOW);

	bcm2835_spi_transfer(wPreamble>>8);
	bcm2835_spi_transfer(wPreamble);

	LCDWaitForReady();

	for(i=0;i<ulSizeWordCnt;i++)
	{
		bcm2835_spi_transfer(pwBuf[i]>>8);
		bcm2835_spi_transfer(pwBuf[i]);
	}

	bcm2835_gpio_write(CS,HIGH);
}

static void LCDWriteNData2(uint8_t *data, uint32_t len) {
	meas_start();
	LCDWaitForReady();
	bcm2835_gpio_write(CS, LOW);
	
	bcm2835_spi_transfer(0x00);
	bcm2835_spi_transfer(0x00);
	
	LCDWaitForReady();
	bcm2835_spi_transfern((char*)data, len);
	
	bcm2835_gpio_write(CS, HIGH);
	meas_end();
}

//-----------------------------------------------------------
//Host controller function 4---Read Data from host data Bus
//-----------------------------------------------------------
uint16_t LCDReadData()
{
	uint16_t wRData;

	uint16_t wPreamble = 0x1000;

	LCDWaitForReady();

	bcm2835_gpio_write(CS,LOW);

	bcm2835_spi_transfer(wPreamble>>8);
	bcm2835_spi_transfer(wPreamble);

	LCDWaitForReady();

	wRData=bcm2835_spi_transfer(0x00);//dummy
	wRData=bcm2835_spi_transfer(0x00);//dummy

	LCDWaitForReady();

	wRData = bcm2835_spi_transfer(0x00)<<8;
	wRData |= bcm2835_spi_transfer(0x00);

	bcm2835_gpio_write(CS,HIGH);

	return wRData;
}

//-----------------------------------------------------------
//  Read Burst N words Data
//-----------------------------------------------------------
void LCDReadNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt)
{
	uint32_t i;

	uint16_t wPreamble = 0x1000;

	LCDWaitForReady();

	bcm2835_gpio_write(CS,LOW);

	bcm2835_spi_transfer(wPreamble>>8);
	bcm2835_spi_transfer(wPreamble);

	LCDWaitForReady();

	pwBuf[0]=bcm2835_spi_transfer(0x00);//dummy
	pwBuf[0]=bcm2835_spi_transfer(0x00);//dummy

	LCDWaitForReady();

	for(i=0;i<ulSizeWordCnt;i++)
	{
		pwBuf[i] = bcm2835_spi_transfer(0x00)<<8;
		pwBuf[i] |= bcm2835_spi_transfer(0x00);
	}

	bcm2835_gpio_write(CS,HIGH);
}

//-----------------------------------------------------------
//Host controller function 5---Write command to host data Bus with aruments
//-----------------------------------------------------------
void LCDSendCmdArg(uint16_t usCmdCode,uint16_t* pArg, uint16_t usNumArg)
{
     uint16_t i;
     //Send Cmd code
     LCDWriteCmdCode(usCmdCode);
     //Send Data
     for(i=0;i<usNumArg;i++)
     {
         LCDWriteData(pArg[i]);
     }
}

//-----------------------------------------------------------
//Host Cmd 1---SYS_RUN
//-----------------------------------------------------------
void IT8951SystemRun()
{
    LCDWriteCmdCode(IT8951_TCON_SYS_RUN);
}

//-----------------------------------------------------------
//Host Cmd 2---STANDBY
//-----------------------------------------------------------
void IT8951StandBy()
{
    LCDWriteCmdCode(IT8951_TCON_STANDBY);
}

//-----------------------------------------------------------
//Host Cmd 3---SLEEP
//-----------------------------------------------------------
void IT8951Sleep()
{
    LCDWriteCmdCode(IT8951_TCON_SLEEP);
}

//-----------------------------------------------------------
//Host Cmd 4---REG_RD
//-----------------------------------------------------------
static uint16_t IT8951ReadReg(uint16_t usRegAddr)
{
	uint16_t usData;

	//Send Cmd and Register Address
	LCDWriteCmdCode(IT8951_TCON_REG_RD);
	LCDWriteData(usRegAddr);
	//Read data from Host Data bus
	usData = LCDReadData();
	return usData;
}
//-----------------------------------------------------------
//Host Cmd 5---REG_WR
//-----------------------------------------------------------
static void IT8951WriteReg(uint16_t usRegAddr,uint16_t usValue)
{
	//Send Cmd , Register Address and Write Value
	LCDWriteCmdCode(IT8951_TCON_REG_WR);
	LCDWriteData(usRegAddr);
	LCDWriteData(usValue);
}

//-----------------------------------------------------------
//Host Cmd 6---MEM_BST_RD_T
//-----------------------------------------------------------
void IT8951MemBurstReadTrigger(uint32_t ulMemAddr , uint32_t ulReadSize)
{
    uint16_t usArg[4];
    //Setting Arguments for Memory Burst Read
    usArg[0] = (uint16_t)(ulMemAddr & 0x0000FFFF); //addr[15:0]
    usArg[1] = (uint16_t)( (ulMemAddr >> 16) & 0x0000FFFF ); //addr[25:16]
    usArg[2] = (uint16_t)(ulReadSize & 0x0000FFFF); //Cnt[15:0]
    usArg[3] = (uint16_t)( (ulReadSize >> 16) & 0x0000FFFF ); //Cnt[25:16]
    //Send Cmd and Arg
    LCDSendCmdArg(IT8951_TCON_MEM_BST_RD_T , usArg , 4);
}
//-----------------------------------------------------------
//Host Cmd 7---MEM_BST_RD_S
//-----------------------------------------------------------
void IT8951MemBurstReadStart()
{
    LCDWriteCmdCode(IT8951_TCON_MEM_BST_RD_S);
}
//-----------------------------------------------------------
//Host Cmd 8---MEM_BST_WR
//-----------------------------------------------------------
void IT8951MemBurstWrite(uint32_t ulMemAddr , uint32_t ulWriteSize)
{
    uint16_t usArg[4];
    //Setting Arguments for Memory Burst Write
    usArg[0] = (uint16_t)(ulMemAddr & 0x0000FFFF); //addr[15:0]
    usArg[1] = (uint16_t)( (ulMemAddr >> 16) & 0x0000FFFF ); //addr[25:16]
    usArg[2] = (uint16_t)(ulWriteSize & 0x0000FFFF); //Cnt[15:0]
    usArg[3] = (uint16_t)( (ulWriteSize >> 16) & 0x0000FFFF ); //Cnt[25:16]
    //Send Cmd and Arg
    LCDSendCmdArg(IT8951_TCON_MEM_BST_WR , usArg , 4);
}
//-----------------------------------------------------------
//Host Cmd 9---MEM_BST_END
//-----------------------------------------------------------
void IT8951MemBurstEnd(void)
{
    LCDWriteCmdCode(IT8951_TCON_MEM_BST_END);
}

//-----------------------------------------------------------
//Example of Memory Burst Write
//-----------------------------------------------------------
// ****************************************************************************************
// Function name: IT8951MemBurstWriteProc( )
//
// Description:
//   IT8951 Burst Write procedure
//
// Arguments:
//      uint32_t ulMemAddr: IT8951 Memory Target Address
//      uint32_t ulWriteSize: Write Size (Unit: Word)
//      uint8_t* pDestBuf - Buffer of Sent data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstWriteProc(uint32_t ulMemAddr , uint32_t ulWriteSize, uint16_t* pSrcBuf )
{

    uint32_t i;

    //Send Burst Write Start Cmd and Args
    IT8951MemBurstWrite(ulMemAddr , ulWriteSize);

    //Burst Write Data
    for(i=0;i<ulWriteSize;i++)
    {
        LCDWriteData(pSrcBuf[i]);
    }

    //Send Burst End Cmd
    IT8951MemBurstEnd();
}

// ****************************************************************************************
// Function name: IT8951MemBurstReadProc( )
//
// Description:
//   IT8951 Burst Read procedure
//
// Arguments:
//      uint32_t ulMemAddr: IT8951 Read Memory Address
//      uint32_t ulReadSize: Read Size (Unit: Word)
//      uint8_t* pDestBuf - Buffer for storing Read data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstReadProc(uint32_t ulMemAddr , uint32_t ulReadSize, uint16_t* pDestBuf )
{
    //Send Burst Read Start Cmd and Args
    IT8951MemBurstReadTrigger(ulMemAddr , ulReadSize);

    //Burst Read Fire
    IT8951MemBurstReadStart();

    //Burst Read Request for SPI interface only
    LCDReadNData(pDestBuf, ulReadSize);

    //Send Burst End Cmd
    IT8951MemBurstEnd(); //the same with IT8951MemBurstEnd()
}

//-----------------------------------------------------------
//Host Cmd 10---LD_IMG
//-----------------------------------------------------------
void IT8951LoadImgStart(IT8951LdImgInfo* pstLdImgInfo)
{
    uint16_t usArg;
    //Setting Argument for Load image start
    usArg = (pstLdImgInfo->usEndianType << 8 )
    |(pstLdImgInfo->usPixelFormat << 4)
    |(pstLdImgInfo->usRotate);
    //Send Cmd
    LCDWriteCmdCode(IT8951_TCON_LD_IMG);
    //Send Arg
    LCDWriteData(usArg);
}
//-----------------------------------------------------------
//Host Cmd 11---LD_IMG_AREA
//-----------------------------------------------------------
void IT8951LoadImgAreaStart(
  const IT8951LdImgInfo* pstLdImgInfo,
  const IT8951AreaImgInfo* pstAreaImgInfo)
{
    uint16_t usArg[5];
    //Setting Argument for Load image start
    usArg[0] = (pstLdImgInfo->usEndianType << 8 )
    |(pstLdImgInfo->usPixelFormat << 4)
    |(pstLdImgInfo->usRotate);
    usArg[1] = pstAreaImgInfo->usX;
    usArg[2] = pstAreaImgInfo->usY;
    usArg[3] = pstAreaImgInfo->usWidth;
    usArg[4] = pstAreaImgInfo->usHeight;
    //Send Cmd and Args
    LCDSendCmdArg(IT8951_TCON_LD_IMG_AREA , usArg , 5);
}
//-----------------------------------------------------------
//Host Cmd 12---LD_IMG_END
//-----------------------------------------------------------
#define IT8951LoadImgEnd() \
    LCDWriteCmdCode(IT8951_TCON_LD_IMG_END)

void GetIT8951SystemInfo(IT8951DevInfo* pstDevInfo)
{
	uint16_t* pusWord = (uint16_t*)pstDevInfo;

	//Send I80 CMD
	LCDWriteCmdCode(USDEF_I80_CMD_GET_DEV_INFO);

	//Burst Read Request for SPI interface only
	LCDReadNData(pusWord, sizeof(IT8951DevInfo)/2);//Polling HRDY for each words(2-bytes) if possible

	//Show Device information of IT8951
	printf("Panel(W,H) = (%d,%d)\r\n",
	pstDevInfo->usPanelW, pstDevInfo->usPanelH );
	printf("Image Buffer Address = %X\r\n",
	pstDevInfo->usImgBufAddrL | (pstDevInfo->usImgBufAddrH << 16));
	//Show Firmware and LUT Version
	printf("FW Version = %s\r\n", (uint8_t*)pstDevInfo->usFWVersion);
	printf("LUT Version = %s\r\n", (uint8_t*)pstDevInfo->usLUTVersion);
}

//-----------------------------------------------------------
//Initial function 2---Set Image buffer base address
//-----------------------------------------------------------
void IT8951SetImgBufBaseAddr(uint32_t ulImgBufAddr)
{
	uint16_t usWordH = (uint16_t)((ulImgBufAddr >> 16) & 0x0000FFFF);
	uint16_t usWordL = (uint16_t)( ulImgBufAddr & 0x0000FFFF);
	//Write LISAR Reg
	IT8951WriteReg(LISAR + 2 ,usWordH);
	IT8951WriteReg(LISAR ,usWordL);
}

//-----------------------------------------------------------
// 3.6. Display Functions
//-----------------------------------------------------------

//-----------------------------------------------------------
//Display function 1---Wait for LUT Engine Finish
//                     Polling Display Engine Ready by LUTNo
//-----------------------------------------------------------
void IT8951WaitForDisplayReady()
{
	meas_start();
	//Check IT8951 Register LUTAFSR => NonZero Busy, 0 - Free
	while(IT8951ReadReg(LUTAFSR)) usleep(100);
	meas_end();
}

static void _convert_bpp_4(uint8_t* pusFrameBuf, uint32_t size) {
	meas_start();
	uint32_t i = 0, j = 0;

	while (i < size) {
		pusFrameBuf[j] = (pusFrameBuf[i] >> 4) | ((pusFrameBuf[i + 1] >> 4) << 4);
		j += 1;
		i += 2;
	}
	meas_end();
}

//-----------------------------------------------------------
//Display function 2---Load Image Area process
//-----------------------------------------------------------
void IT8951HostAreaPackedPixelWrite(
  const IT8951LdImgInfo* pstLdImgInfo,
  const IT8951AreaImgInfo* pstAreaImgInfo,
  uint8_t bpp_mode
) {
	meas_start();
	//uint32_t j;
	//Source buffer address of Host
	uint8_t* pusFrameBuf = (uint8_t*)pstLdImgInfo->ulStartFBAddr;
	uint32_t size = pstAreaImgInfo->usHeight * pstAreaImgInfo->usWidth;
	
	if (bpp_mode == 4) {
		_convert_bpp_4(pusFrameBuf, bpp_mode);
	}

	IT8951SetImgBufBaseAddr(pstLdImgInfo->ulImgBufBaseAddr);
	IT8951LoadImgAreaStart(pstLdImgInfo, pstAreaImgInfo);
	
	if (bpp_mode == 4) {
		size /= 2;
	}
	LCDWriteNData2(pusFrameBuf, size);
	IT8951LoadImgEnd();
	meas_end();
}

//-----------------------------------------------------------
//Display functions 3---Application for Display panel Area
//-----------------------------------------------------------
void IT8951DisplayArea(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode)
{
	meas_start();
	//Send I80 Display Command (User defined command of IT8951)
	LCDWriteCmdCode(USDEF_I80_CMD_DPY_AREA); //0x0034
	//Write arguments
	LCDWriteData(usX);
	LCDWriteData(usY);
	LCDWriteData(usW);
	LCDWriteData(usH);
	LCDWriteData(usDpyMode);
	meas_end();
}

//-----------------------------------------------------------
//Test function 1---Software Initial
//-----------------------------------------------------------
uint8_t IT8951_Init(Device *device)
{
	if (!bcm2835_init())
	{
		printf("bcm2835_init error \n");
		return 1;
	}

	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);   	//default
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);               		//default
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);		//default

	bcm2835_gpio_fsel(CS, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(HRDY, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(RESET, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_gpio_write(CS, HIGH);

	printf("****** IT8951 ******\n");

	bcm2835_gpio_write(RESET, LOW);
	bcm2835_delay(100);
	bcm2835_gpio_write(RESET, HIGH);

	//Get Device Info
	GetIT8951SystemInfo(&device->gstI80DevInfo);

	device->gpFrameBuf = malloc(Device_width(device) * Device_height(device));
	if (!device->gpFrameBuf)
	{
		perror("malloc error!\n");
		return 1;
	}

 	device->gulImgBufAddr = device->gstI80DevInfo.usImgBufAddrL | (device->gstI80DevInfo.usImgBufAddrH << 16);

 	//Set to Enable I80 Packed mode
 	IT8951WriteReg(I80CPCR, 0x0001);

	return 0;
}

void IT8951_Cancel(Device *device)
{
	free(device->gpFrameBuf);
    device->gpFrameBuf = NULL;

	bcm2835_spi_end();
	bcm2835_close();
}
