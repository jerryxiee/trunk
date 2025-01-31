#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "rc522.h"
#include "tq2440.h"

//int fd = 0;
static uint8_t bits = 8;
static uint32_t speed = 80000;
static uint16_t delay = 0;

static unsigned char SPIReadByte(unsigned char cmd);
static void SPIWriteByte(unsigned char cmd, unsigned char data);
static unsigned char ReadRawRC(unsigned char Address);
static void WriteRawRC(unsigned char Address, unsigned char value);
static void SetBitMask(unsigned char reg, unsigned char mask);
static void ClearBitMask(unsigned char reg, unsigned char mask);
static void CalculateCRC(unsigned char* pIndata, unsigned char len, unsigned char* pOutData);
static char PcdComMF522(unsigned char Command, unsigned char* pInData, unsigned char InLenByte, unsigned char* pOutData, unsigned int* pOutLenBit);

void sysfs_write(const char* fname, const char* dat, int len);

void sysfs_write(const char* fname, const char* dat, int len)
{
    int fd = open(fname, O_RDWR);

    if (fd > 0) {
        write(fd, dat, len);
        close(fd);
    } else {
        printf("can't open device:%s\n", fname);
    }
}

static void delay_ns(unsigned int ns)
{
    unsigned int i, j = 0;

    for (i = 0; i < ns; i++) {
        j++;
    }
}

static void pabort(const char* s)
{
    perror(s);
    abort();
}

/////////////////////////////////////////////////////////////////////
//功    能：读SPI数据
//参数说明：cmd[IN]:命令字
//返    回：SPI数据
/////////////////////////////////////////////////////////////////////
static unsigned char SPIReadByte(unsigned char cmd)
{
    int ret;
    uint8_t rx = 0x00;
    uint8_t tx = cmd;
    struct spi_ioc_transfer xfer[2] = {
        {
            .tx_buf = (unsigned long)& tx,
            .rx_buf = (unsigned long)NULL,  //
            .len = 1,
            .delay_usecs = delay,
            .speed_hz = speed,
            .bits_per_word = bits,
            .cs_change = 0,
        },
        {
            .tx_buf = (unsigned long)NULL,
            .rx_buf = (unsigned long)& rx, //
            .len = 1,
            .delay_usecs = delay,
            .speed_hz = speed,
            .bits_per_word = bits,
            .cs_change = 0,
        }
    };

#if 1
    int fd = open(TQ2440_SPI, O_RDWR);

    if (fd < 0) {
        printf("can't Read:%s\n", TQ2440_SPI);
        pabort("can't open spi device");
    }

#endif
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &xfer);
#if 1
    close(fd);
#endif

    if (ret == 1) {
        pabort("can't send spi message");
    }

    return rx;
}

/////////////////////////////////////////////////////////////////////
//功    能：写SPI数据
//参数说明：cmd[IN]:命令字
//          data[IN]:SPI数据
/////////////////////////////////////////////////////////////////////
static void SPIWriteByte(unsigned char cmd, unsigned char data)
{
    int ret;
    uint8_t tx[2];
    tx[0] = cmd;
    tx[1] = data;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)NULL,  //
        .len = 2,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
        .cs_change = 0,
    };
#if 1
    int fd = open(TQ2440_SPI, O_RDWR);

    if (fd < 0) {
        printf("can't write :%s\n", TQ2440_SPI);
        pabort("can't open spi device");
    }

#endif
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    close(fd);

    if (ret == 1) {
        pabort("can't send spi message");
    }

    delay_ns(10);
}

/////////////////////////////////////////////////////////////////////
//功    能：读RC632寄存器
//参数说明：Address[IN]:寄存器地址
//返    回：读出的值
/////////////////////////////////////////////////////////////////////
static unsigned char ReadRawRC(unsigned char Address)
{
    unsigned char ucAddr;
    unsigned char ucResult = 0;

    ucAddr = ((Address << 1) & 0x7E) | 0x80;    //read cmd
    ucResult = SPIReadByte(ucAddr);
    return ucResult;
}

/////////////////////////////////////////////////////////////////////
//功    能：写RC632寄存器
//参数说明：Address[IN]:寄存器地址
//          value[IN]:写入的值
/////////////////////////////////////////////////////////////////////
static void WriteRawRC(unsigned char Address, unsigned char value)
{
    unsigned char ucAddr;

    ucAddr = ((Address << 1) & 0x7E);//write cmd

    SPIWriteByte(ucAddr, value);
}

/////////////////////////////////////////////////////////////////////
//功    能：置RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:置位值
/////////////////////////////////////////////////////////////////////
static void SetBitMask(unsigned char reg, unsigned char mask)
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp | mask);  // set bit mask
}

/////////////////////////////////////////////////////////////////////
//功    能：清RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:清位值
/////////////////////////////////////////////////////////////////////
static void ClearBitMask(unsigned char reg, unsigned char mask)
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & ~mask);  // clear bit mask
}

/////////////////////////////////////////////////////////////////////
//功    能：通过RC522和ISO14443卡通讯
//参数说明：Command[IN]:RC522命令字
//          pInData[IN]:通过RC522发送到卡片的数据
//          InLenByte[IN]:发送数据的字节长度
//          pOutData[OUT]:接收到的卡片返回数据
//          pOutLenBit[OUT]:返回数据的位长度
/////////////////////////////////////////////////////////////////////
static char PcdComMF522( unsigned char Command,
                         unsigned char* pInData,
                         unsigned char InLenByte,
                         unsigned char* pOutData,
                         unsigned int* pOutLenBit)
{
    char status = MI_ERR;
    unsigned char irqEn   = 0x00;
    unsigned char waitFor = 0x00;
    unsigned char lastBits;
    unsigned char n;
    unsigned int i;

    switch (Command) {
        case PCD_AUTHENT:
            irqEn   = 0x12;
            waitFor = 0x10;
            break;

        case PCD_TRANSCEIVE:
            irqEn   = 0x77;
            waitFor = 0x30;
            break;

        default:
            break;
    }

    WriteRawRC(ComIEnReg, irqEn | 0x80);
	ClearBitMask(ComIrqReg, 0x80); //定时器立刻停止运行 
    WriteRawRC(CommandReg, PCD_IDLE);
	SetBitMask(FIFOLevelReg, 0x80); //内部FIFO缓冲区的读和写指针以及寄存器ErrReg的BufferOvfl标志立刻被清除

    for (i = 0; i < InLenByte; i++) {
        WriteRawRC(FIFODataReg, pInData[i]); // 向内部fifo写入数据
    }

    WriteRawRC(CommandReg, Command);  // 写命令寄存器


    if (Command == PCD_TRANSCEIVE) {
        SetBitMask(BitFramingReg, 0x80);
    }

    //i = 600;//根据时钟频率调整，操作M1卡最大等待时间25ms
    i = 2000;

    do {
        n = ReadRawRC(ComIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitFor));

    ClearBitMask(BitFramingReg, 0x80);


    if (i != 0) {

        if (!(ReadRawRC(ErrorReg) & 0x1B)) {
            status = MI_OK;

            if (n & irqEn & 0x01) {
                status = MI_OK;
                // printf("MI_OK NEED SOLUTION!!!\n");
            }

            if (Command == PCD_TRANSCEIVE) {
                n = ReadRawRC(FIFOLevelReg);
                lastBits = ReadRawRC(ControlReg) & 0x07;

                if (lastBits) {
                    *pOutLenBit = (n - 1) * 8 + lastBits;
                } else {
                    *pOutLenBit = n * 8;
                }

                if (n == 0) {
                    n = 1;
                }

                if (n > MAXRLEN) {
                    n = MAXRLEN;
                }

                for (i = 0; i < n; i++) {
                    pOutData[i] = ReadRawRC(FIFODataReg);
                }
            }
        } else {
            status = MI_ERR;
        }

    }

    SetBitMask(ControlReg, 0x80);           // stop timer now
    WriteRawRC(CommandReg, PCD_IDLE);


    return status;
}

/////////////////////////////////////////////////////////////////////
//用MF522计算CRC16函数
/////////////////////////////////////////////////////////////////////
static void CalculateCRC(unsigned char* pIndata, unsigned char len, unsigned char* pOutData)
{
    unsigned char i, n;
    ClearBitMask(DivIrqReg, 0x04);
    WriteRawRC(CommandReg, PCD_IDLE);
    SetBitMask(FIFOLevelReg, 0x80);

    for (i = 0; i < len; i++) {
        WriteRawRC(FIFODataReg, *(pIndata + i));
    }

    WriteRawRC(CommandReg, PCD_CALCCRC);
    i = 0xFF;

    do {
        n = ReadRawRC(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04));

    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/////////////////////////////////////////////////////////////////////
//功    能：硬件复位RC522，通过Reset引脚
/////////////////////////////////////////////////////////////////////
#define  RFID_GPIONAME_DIR   "/sys/class/gpio/gpio26/direction"
#define  RFID_GPIONAME_VALUE "/sys/class/gpio/gpio26/value"
void ResetRC522()
{
#if 0
    int fd = open(TQ2440_GPIO, O_RDWR);

    if (fd < 0) {
        pabort("can't open gpio device");
    }

    ioctl(fd, IOCTL_GPIO_HIGH, RC522_RESET);//H
    delay_ns(10);
    ioctl(fd, IOCTL_GPIO_LOW, RC522_RESET);//L
    delay_ns(10);
    ioctl(fd, IOCTL_GPIO_HIGH, RC522_RESET);//H
    delay_ns(10);
    close(fd);
#else
    sysfs_write(RFID_GPIONAME_DIR, "out", 3);

    sysfs_write(RFID_GPIONAME_VALUE, "1", 1);//H
    delay_ns(10);
    sysfs_write(RFID_GPIONAME_VALUE, "0", 1);//L
    delay_ns(10);
    sysfs_write(RFID_GPIONAME_VALUE, "1", 1);//H
#endif
}

/////////////////////////////////////////////////////////////////////
//功    能：软件复位RC522
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset()
{
    WriteRawRC(CommandReg, PCD_RESETPHASE);   // 复位MFRC522
    delay_ns(10);
    WriteRawRC(ModeReg, 0x3D);   //和Mifare卡通讯，CRC初始值0x6363

    WriteRawRC(TReloadRegL, 30);
    WriteRawRC(TReloadRegH, 0);
	WriteRawRC(TModeReg, 0x8D); // 定时器在所有速率的发送结束时自动启动。在接收到第一个数据位后定时器立刻停止运行。如果该位清零, 表明定时器不受通信协议的影响。
    WriteRawRC(TPrescalerReg, 0x3E);
    WriteRawRC(TxAutoReg, 0x40);

    return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//功    能：寻卡
//参数说明：req_code[IN]:寻卡方式
//                0x52 = 寻感应区内所有符合14443A标准的卡
//                0x26 = 寻未进入休眠状态的卡
//          pTagType[OUT]:卡片类型代码
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(unsigned char req_code, unsigned char* pTagType)
{
    char status;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x07);
	SetBitMask(TxControlReg, 0x03);  // tx1 tx2管脚的输出信号将传递经发送数据调制的13.56MHz的能量载波信号。


    ucComMF522Buf[0] = req_code;

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x10)) {
        *pTagType     = ucComMF522Buf[0];
        *(pTagType + 1) = ucComMF522Buf[1];
    } else {
        status = MI_ERR;
    }

    return status;
}

//////////////////////////////////////////////////////////////////////
//设置RC632的工作方式
//////////////////////////////////////////////////////////////////////
char PcdConfigISOType(unsigned char type)
{
    if (type == 'A') {                   //ISO14443_A
        ClearBitMask(Status2Reg, 0x08);
        WriteRawRC(ModeReg, 0x3D);//3F
        WriteRawRC(RxSelReg, 0x86);//84
        WriteRawRC(RFCfgReg, 0x7F);   //4F
        WriteRawRC(TReloadRegL, 30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec)
        WriteRawRC(TReloadRegH, 0);
        WriteRawRC(TModeReg, 0x8D);
        WriteRawRC(TPrescalerReg, 0x3E);
        delay_ns(1000);
        PcdAntennaOn();
    } else {
        return MI_NOTAGERR;
    }

    return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//功    能：防冲撞
//参数说明：pSnr[OUT]:卡片序列号，4字节
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdAnticoll(unsigned char* pSnr)
{
    char status;
    unsigned char i, snr_check = 0;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];


    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x00);
    ClearBitMask(CollReg, 0x80);

    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen);

    if (status == MI_OK) {
        for (i = 0; i < 4; i++) {
            *(pSnr + i)  = ucComMF522Buf[i];
            snr_check ^= ucComMF522Buf[i];
        }

        if (snr_check != ucComMF522Buf[i]) {
            status = MI_ERR;
        }
    }

    SetBitMask(CollReg, 0x80);

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：选定卡片
//参数说明：pSnr[IN]:卡片序列号，4字节
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdSelect(unsigned char* pSnr)
{
    char status;
    unsigned char i;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;

    for (i = 0; i < 4; i++) {
        ucComMF522Buf[i + 2] = *(pSnr + i);
        ucComMF522Buf[6]  ^= *(pSnr + i);
    }

    CalculateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);

    ClearBitMask(Status2Reg, 0x08);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x18)) {
        status = MI_OK;
    } else {
        status = MI_ERR;
    }

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：验证卡片密码
//参数说明：auth_mode[IN]:密码验证模式
//                 0x60 = 验证A密钥
//                 0x61 = 验证B密钥
//          addr[IN]:块地址
//          pKey[IN]:密码
//          pSnr[IN]:卡片序列号，4字节
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdAuthState(unsigned char auth_mode, unsigned char addr, unsigned char* pKey, unsigned char* pSnr)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;

    for (i = 0; i < 6; i++) {
        ucComMF522Buf[i + 2] = *(pKey + i);
    }

    for (i = 0; i < 6; i++) {
        ucComMF522Buf[i + 8] = *(pSnr + i);
    }

    //   memcpy(&ucComMF522Buf[2], pKey, 6);
    //   memcpy(&ucComMF522Buf[8], pSnr, 4);

    status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08))) {
        status = MI_ERR;
    }

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：读取M1卡一块数据
//参数说明：addr[IN]：块地址
//          pData[OUT]：读出的数据，16字节
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRead(unsigned char addr, unsigned char* pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalculateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x90))
        //   {   memcpy(pData, ucComMF522Buf, 16);   }
    {
        for (i = 0; i < 16; i++) {
            *(pData + i) = ucComMF522Buf[i];
        }
    } else {
        status = MI_ERR;
    }

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：写数据到M1卡一块
//参数说明：addr[IN]：块地址
//          pData[IN]：写入的数据，16字节
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdWrite(unsigned char addr, unsigned char* pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalculateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {
        status = MI_ERR;
    }

    if (status == MI_OK) {
        //memcpy(ucComMF522Buf, pData, 16);
        for (i = 0; i < 16; i++) {
            ucComMF522Buf[i] = *(pData + i);
        }

        CalculateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {
            status = MI_ERR;
        }
    }

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：命令卡片进入休眠状态
//返    回：成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt()
{
    char status;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalculateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//开启天线
//每次启动或关闭天险发射之间应至少有1ms的间隔
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn()
{
    unsigned char i;
    i = ReadRawRC(TxControlReg);

    if (!(i & 0x03)) {
        SetBitMask(TxControlReg, 0x03);   ///关闭天线
    }
}

/////////////////////////////////////////////////////////////////////
//关闭天线
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff()
{
    ClearBitMask(TxControlReg, 0x03);   ///打开天线
}

char PcdSeek(unsigned char* pLastSelectedSnr)
{
    unsigned char i;
    char status;
    unsigned char RevBuffer[10];

    status = PcdReset();
    if (status != MI_OK) {
        return status;
    }

    status = PcdRequest(PICC_REQIDL, &RevBuffer[0]);//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节
    if (status != MI_OK) {
        return status;
    }

    status = PcdAnticoll(&RevBuffer[2]);//防冲突，返回4字节的卡序列号
    if (status != MI_OK) {
        return status;
    }

    for (i = 0; i < 4; i++) {
        pLastSelectedSnr[i] = RevBuffer[(2 + i)];
    }

    status = PcdSelect(pLastSelectedSnr);//选卡

    return status;
}


char PcdInit()
{
    char status;

    status = PcdReset();

    if (status != MI_OK) {
        return status;
    }

    PcdAntennaOff();
    PcdAntennaOn();
    status = PcdConfigISOType('A');

    int fd = open(TQ2440_SPI, O_RDWR);

    if (fd < 0) {
        printf("can't Read:%s\n", TQ2440_SPI);
        pabort("can't open spi device");
    }

    close(fd);

    return status;
}