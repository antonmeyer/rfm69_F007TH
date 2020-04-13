
#include "RFM69.h"
#include <SPI.h>

boolean RFM69::init433(unsigned char PinNSS, unsigned char PinDIO0)
{

	_PinNSS = PinNSS;
	pinMode(_PinNSS, OUTPUT);
	digitalWrite(_PinNSS, HIGH);
	SPI.begin();
	_PinDIO0 = PinDIO0;
	pinMode(_PinDIO0, INPUT);

	delay(10);
	unsigned char Type = readSPI(RFM69_REG_10_VERSION);
	if (Type == 00 || Type == 0xFF)
		return (false);
	//init device mode
	_Mode = INITIALIZING;

	setModeStdby();
	//set DAGC continuous automatic gain control)
	//writeSPI(RFM69_REG_6F_TESTDAGC, RFM69_TESTDAGC_CONTINUOUSDAGC_IMPROVED_LOWBETAOFF);

	//RFM69_REG_02_DATAMODUL RFM69_DATAMODUL_DATAMODE_CONT_WITH_SYNC RFM69_DATAMODUL_MODULATIONTYPE_OOK 0x08

	//Channel Filter?? RxBw

	//set LNA by AGC and 200 Ohm 0x8 / 0x00 50 Ohm
	writeSPI(RFM69_REG_18_LNA, 0x08);

	setFrequency(433.92);
	//setModulation(OOK);
	//writeSPI(RFM69_REG_02_DATAMODUL, RFM69_DATAMODUL_DATAMODE_CONT_WITH_SYNC | RFM69_DATAMODUL_MODULATIONTYPE_OOK);
	writeSPI(RFM69_REG_02_DATAMODUL, RFM69_DATAMODUL_DATAMODE_PACKET | RFM69_DATAMODUL_MODULATIONTYPE_OOK);
	setBitRate(2048); // sampling rate for RSSI

	/*set sync words 	*/

	unsigned char SyncBytes[] = {0xAA, 0xA6, 0x65, 0x66}; // does this fit to raw?
	setSyncWords(SyncBytes, sizeof(SyncBytes));
	useSyncWords(true);

	//writeSPI(RFM69_REG_38_PAYLOADLENGTH, 0);	// unlimited
	writeSPI(RFM69_REG_38_PAYLOADLENGTH, FixPktSize);
	writeSPI(RFM69_REG_37_PACKETCONFIG1, RFM69_PACKETCONFIG1_DCFREE_MANCHESTER); // fixed length, noCRC, no address filter 0x4 to set interrupt without CRC???
	//writeSPI(RFM69_REG_37_PACKETCONFIG1, 0);
	//writeSPI(RFM69_REG_3C_FIFOTHRESH, FixPktSize);

	writeSPI(RFM69_REG_25_DIOMAPPING1, RFM69_DIOMAPPING1_DIO0MAPPING_01 | RFM69_DIOMAPPING1_DIO1MAPPING_10 | RFM69_DIOMAPPING1_DIO2MAPPING_01 | RFM69_DIOMAPPING1_DIO3MAPPING_10); //set DIO0 in Rx mode to PayloadReady, DIO1 10  FIFO not empty, DIO2 01 Data , DIO3 10 Sync
	writeSPI(RFM69_REG_26_DIOMAPPING2, RFM69_DIOMAPPING2_DIO5MAPPING_11);																										   // DIO5 11 ModeReady
	return (true);
}

void RFM69::setModulation(modulation Modulation)
{
	unsigned char RegVal;
	RegVal = readSPI(RFM69_REG_02_DATAMODUL);
	RegVal &= 0xE4;
	switch (Modulation)
	{
	case FSK:
		writeSPI(RFM69_REG_02_DATAMODUL, RegVal | 0x00);
		break;
	case GFSK:
		writeSPI(RFM69_REG_02_DATAMODUL, RegVal | 0x01);
		break;
	case OOK:
		writeSPI(RFM69_REG_02_DATAMODUL, RegVal | 0x08);
		break;
	}
	_Modulation = Modulation;
}

void RFM69::setBitRate(unsigned long BitRate)
{
	unsigned int RateVal;
	float Fosc = RFM69_FXOSC;
	/*if (BitRate < 1200)
		BitRate = 1200; */
	if (BitRate > 300000)
		BitRate = 300000;
	if (_Modulation == OOK && BitRate > 19200)
		BitRate = 19200;
	RateVal = (unsigned int)(round(Fosc / BitRate));
	writeSPI(RFM69_REG_03_BITRATEMSB, highByte(RateVal));
	writeSPI(RFM69_REG_04_BITRATELSB, lowByte(RateVal));
}

void RFM69::setFDEV(unsigned long fdev)
{
	unsigned int Val;
	float Fstep = RFM69_FSTEP;
	Val = round(float(fdev) / Fstep);
	writeSPI(RFM69_REG_05_FDEVMSB, highByte(Val));
	writeSPI(RFM69_REG_06_FDEVLSB, lowByte(Val));
}

void RFM69::setFrequency(float Frequency)
{
	//if (Frequency < 424.0) Frequency = 424.0;
	//if (Frequency > 510.0) Frequency = 510.0;
	unsigned long Frf = (unsigned long)((Frequency * 1000000.0) / RFM69_FSTEP);
	writeSPI(RFM69_REG_07_FRFMSB, (Frf >> 16) & 0xff);
	writeSPI(RFM69_REG_08_FRFMID, (Frf >> 8) & 0xff);
	writeSPI(RFM69_REG_09_FRFLSB, Frf & 0xff);
}


void RFM69::setSyncWords(unsigned char *SyncWords, unsigned char Len)
{
	unsigned char SyncConfig;
	if (Len > 8)
		Len = 8;
	if (Len > 0)
		writeSPIBurst(RFM69_REG_2F_SYNCVALUE1, SyncWords, Len);
	SyncConfig = readSPI(RFM69_REG_2E_SYNCCONFIG);
	SyncConfig &= 0x7F;
	SyncConfig &= ~RFM69_SYNCCONFIG_SYNCSIZE;
	SyncConfig |= (Len - 1) << 3;
	writeSPI(RFM69_REG_2E_SYNCCONFIG, SyncConfig);
}

void RFM69::useSyncWords(boolean Using)
{
	unsigned char SyncConfig;
	SyncConfig = readSPI(RFM69_REG_2E_SYNCCONFIG);
	if (Using)
		SyncConfig |= 0x80;
	else
		SyncConfig &= 0x7F;
	writeSPI(RFM69_REG_2E_SYNCCONFIG, SyncConfig);
}

unsigned char RFM69::getLastRSSI()
{
	return _RSSILast;
}

float RFM69::convertRSSIToRSSIdBm(unsigned char RSSI)
{
	return -(float)(RSSI) / 2.;
}

int RFM69::readTemperature()
{
	int Temp;
	unsigned char LastMode;
	LastMode = _Mode;
	setModeStdby();
	writeSPI(RFM69_REG_4E_TEMP1, RFM69_TEMP1_TEMPMEASSTART);
	while (readSPI(RFM69_REG_4E_TEMP1) & RFM69_TEMP1_TEMPMEASRUNNING)
		;
	Temp = _CalibrationTempVal - readSPI(RFM69_REG_4F_TEMP2);
	switch (LastMode)
	{
	case rfm69RX:
		setModeRx();
		break;
	}
	return (Temp);
}

void RFM69::setCalibrationTemp(unsigned char Val)
{
	_CalibrationTempVal = Val;
}

void RFM69::setModeSleep()
{
	if (_Mode != SLEEP)
	{
		setOpMode(RFM69_OPMODE_MODE_SLEEP);
		_Mode = SLEEP;
	}
}

void RFM69::setModeStdby()
{
	if (_Mode != STDBY)
	{
		setOpMode(RFM69_OPMODE_MODE_STDBY);
		_Mode = STDBY;
	}
}

void RFM69::setModeRx()
{
	if (_Mode != rfm69RX)
	{
		memset(_RxBuffer, 0, sizeof(_RxBuffer));
		setOpMode(RFM69_OPMODE_MODE_RX); // Clears FIFO
		_Mode = rfm69RX;
	}
}

void RFM69::setOpMode(unsigned char Mode)
{
	unsigned char OpMode = readSPI(RFM69_REG_01_OPMODE);
	OpMode &= ~RFM69_OPMODE_MODE;		  //clear bits 4-2 (mode)
	OpMode |= (Mode & RFM69_OPMODE_MODE); //set bits 4-2 (mode)
	writeSPI(RFM69_REG_01_OPMODE, OpMode);
	while (!(readSPI(RFM69_REG_27_IRQFLAGS1) & RFM69_IRQFLAGS1_MODEREADY))
		; //wWait for mode to change.
}

void RFM69::readFifo(unsigned char *buffer, unsigned char length)
{
	digitalWrite(_PinNSS, LOW);
	SPI.transfer(RFM69_REG_00_FIFO); //send the start address with the write mask off
	//for (_RxBufferLen = 0; _RxBufferLen < length; _RxBufferLen++) _RxBuffer[_RxBufferLen] = SPI.transfer(0);
	for (unsigned char i = 0; i < length; i++)
	{
		buffer[i] = SPI.transfer(0);
	}
	digitalWrite(_PinNSS, HIGH);
}

unsigned char RFM69::readSPI(unsigned char Reg)
{
	unsigned char Val;
	digitalWrite(_PinNSS, LOW);
	SPI.transfer(Reg & SPI_READ);
	Val = SPI.transfer(0);
	digitalWrite(_PinNSS, HIGH);
	return (Val);
}

unsigned char RFM69::writeSPI(unsigned char Reg, unsigned char Val)
{
	unsigned char Status = 0;
	digitalWrite(_PinNSS, LOW);
	Status = SPI.transfer(Reg | SPI_WRITE);
	SPI.transfer(Val);
	digitalWrite(_PinNSS, HIGH);
	return (Status);
}

unsigned char RFM69::writeSPIBurst(unsigned char Reg, const unsigned char *Src, unsigned char Len)
{
	unsigned char Status = 0;
	digitalWrite(_PinNSS, LOW);
	Status = SPI.transfer(Reg | SPI_WRITE);
	while (Len--)
		SPI.transfer(*Src++);
	digitalWrite(_PinNSS, HIGH);
	return (Status);
}

void RFM69::printRegisters()
{
	unsigned char i;
	for (i = 0; i < 0x50; i++)
		printRegister(i);
	// Non-contiguous registers
	printRegister(RFM69_REG_58_TESTLNA);
	printRegister(RFM69_REG_5A_TESTPA1);
	printRegister(RFM69_REG_5C_TESTPA2);
	printRegister(RFM69_REG_6F_TESTDAGC);
	printRegister(RFM69_REG_71_TESTAFC);
}

void RFM69::printRegister(unsigned char Reg)
{
	Serial.print(Reg, HEX);
	Serial.print(" ");
	Serial.println(readSPI(Reg), HEX);
}

unsigned char RFM69::rxMsg()
{
	setModeRx();
	//if(digitalRead(_PinDIO0)) {
	if (readSPI(RFM69_REG_28_IRQFLAGS2) & RFM69_IRQFLAGS2_PAYLOADREADY)
	//if (readSPI(RFM69_REG_28_IRQFLAGS2) & RFM69_IRQFLAGS2_FIFOLEVEL)
	{
		setModeStdby();
		//Serial.println("rxMsg1");

		digitalWrite(_PinNSS, LOW);
		SPI.transfer(RFM69_REG_00_FIFO); //send the start address with the write mask off

		unsigned char cnt = 0;

		//while (((readSPI(RFM69_REG_28_IRQFLAGS2) & RFM69_IRQFLAGS2_FIFONOTEMPTY)) && (cnt < 100))
		for (cnt = 0; cnt < FixPktSize; cnt++)
		{
			_RxBuffer[cnt] = SPI.transfer(0);
			//cnt++;
			//Serial.println(cnt);
		}

		digitalWrite(_PinNSS, HIGH);
		return (cnt);
	}
	else
		return (0);
}
