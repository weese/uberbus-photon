// RFM12B driver definitions
// http://opensource.org/licenses/mit-license.php
// 2012-12-12 (C) felix@lowpowerlab.com
// Based on the RFM12 driver from jeelabs.com (2009-02-09 <jc@wippler.nl>)

#include "ubrf12.h"
#include "Particle.h"

struct RF12_stati RF12_status;
volatile unsigned char RF12_Index = 0;
char RF12_Data[RF12_DataLength+10];	// +10 == packet overhead
unsigned char RF12_channel;

static void debug(String message, int value = 0) {
    char msg [250];
    sprintf(msg, message.c_str(), value);
    Particle.publish("DEBUG", msg);
}

unsigned int crcUpdate(unsigned int crc, unsigned char serialData)
{
    unsigned int tmp;
    unsigned char j;

	tmp = serialData << 8;
    for (j=0; j<8; j++){
        if((crc^tmp) & 0x8000)
            crc = (crc<<1) ^ 0x1021;
        else
            crc = crc << 1;
        tmp = tmp << 1;
    }
    return crc;
}

void ubleds_tx(void)
{
    RGB.control(true);
    RGB.color(255, 0, 0);
}

void ubleds_txend(void)
{
    RGB.control(true);
    RGB.color(0, 0, 0);
}

void ubleds_rx(void)
{
    RGB.control(true);
    RGB.color(0, 255, 0);
}

void ubleds_rxend(void)
{
    RGB.control(true);
    RGB.color(0, 0, 0);
}

// Log message to cloud, message is a printf-formatted string
void uart_puts(String message) {
    char msg [50];
    sprintf(msg, message.c_str());
    Particle.publish("DEBUG", msg);
}

unsigned isr_counter = 42;

void ubrf12_isr()
{
    ++isr_counter;
    if(RF12_status.Rx){
        if(RF12_Index < RF12_DataLength){
            ubleds_rx();
            unsigned char d  = ubrf12_trans(0xB000) & 0x00FF;
            if(RF12_Index == 0 && d > RF12_DataLength)
                d = RF12_DataLength;
            RF12_Data[RF12_Index++]=d;
        }else{
            ubrf12_trans(0x8208);
            ubleds_rxend();
            RF12_status.Rx = 0;
        }
        if(RF12_Index >= RF12_Data[0] + 3){		//EOT
            ubrf12_trans(0x8208);
            ubleds_rxend();
            RF12_status.Rx = 0;
            RF12_status.New = 1;
        }
    }else if(RF12_status.Tx){
        ubrf12_trans(0xB800 | RF12_Data[RF12_Index]);
        if(!RF12_Index){
            RF12_status.Tx = 0;
            ubrf12_trans(0x8208);		// TX off
            ubleds_txend();
        }else{
            RF12_Index--;
        }
    }else{
    }
}

void spi_init(void) {
    //set RFM12B SPI settings
    SPI.setClockDivider(SPI_CLOCK_DIV256);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    // SPI.begin(SPI_MODE_MASTER, A2);
    SPI.begin();
    digitalWrite(A2, HIGH);
}

/// Select the transceiver
void select() {
  noInterrupts();
  digitalWrite(A2, LOW);
}

/// UNselect the transceiver chip
void unselect() {
  digitalWrite(A2, HIGH);
  interrupts();
}

unsigned short ubrf12_trans(unsigned short d) {
    select();
    uint16_t retval = SPI.transfer(d >> 8) << 8;
    retval |= SPI.transfer(d & 0xff);
    unselect();
    return retval;
}


void ubrf12_init(unsigned char channel)
{
    unsigned char i;
    RF12_channel = channel;

    spi_init();
#ifdef RESET
    volatile unsigned long j;
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, HIGH);
    delay(10);
    digitalWrite(RESET, LOW);
    delay(100);
    // for(j=0;j<900000;j++)
    //     wdt_reset();
    digitalWrite(RESET, HIGH);
 #endif
    for (i=0; i<100; i++){
        delay(10);			// wait until POR done
        // wdt_reset();
    }

    ubrf12_trans(0xC0E0);			// AVR CLK: 10MHz
    ubrf12_trans(0x80D7);			// Enable FIFO
    ubrf12_trans(0xC2AB);			// Data Filter: internal
    ubrf12_trans(0xCA81);			// Set FIFO mode
    ubrf12_trans(0xE000);			// disable wakeuptimer
    ubrf12_trans(0xC800);			// disable low duty cycle
    ubrf12_trans(0xC4F7);			// AFC settings: autotuning: -10kHz...+7,5kHz

    ubrf12_trans(0x0000);

    RF12_status.Rx = 0;
    RF12_status.Tx = 0;
    RF12_status.New = 0;

    // picking a non-interrupt enabled pin will prevent proper initialization
    // code thanks to @ScruffR - https://community.particle.io/t/how-to-make-rfm69-work-on-photon-solved-new-library-rfm69-particle/26497/93?u=bloukingfisher
    pinMode(RF12_IRQ_PIN, INPUT_PULLUP);
    // pinMode(A4, INPUT_PULLUP);
    if (!attachInterrupt(RF12_IRQ_PIN, ubrf12_isr, FALLING)) {
        debug("Couldn't attach ISR");
    }
}

void ubrf12_setbandwidth(unsigned char bandwidth, unsigned char gain, unsigned char drssi)
{
    ubrf12_trans(0x9400|((bandwidth&7)<<5)|((gain&3)<<3)|(drssi&7));
}

void ubrf12_setfreq(unsigned short freq)
{	
    if (freq<96)				// 430,2400MHz
        freq=96;
    else if (freq>3903)			// 439,7575MHz
        freq=3903;
    ubrf12_trans(0xA000|freq);
}

void ubrf12_setbaud(unsigned short baud)
{
/*    if (baud<663)
        return;
    if (baud<5400)					// Baudrate= 344827,58621/(R+1)/(1+CS*7)
        ubrf12_trans(0xC680|((43104/baud)-1));
    else
        ubrf12_trans(0xC600|((344828UL/baud)-1));*/
    //19200:
    ubrf12_trans(0xC600|16);
}

void ubrf12_setpower(unsigned char power, unsigned char mod)
{	
    ubrf12_trans(0x9800|(power&7)|((mod&15)<<4));
}

unsigned char ubrf12_rxstart(void)
{
    if(RF12_status.New)
        return(1);			//buffer not yet empty
    if(RF12_status.Tx)
        return(2);			//tx in action
    if(RF12_status.Rx)
        return(3);			//rx already in action

    ubrf12_trans(0x82C8);			// RX on
    ubrf12_trans(0xCA81);			// set FIFO mode
    ubrf12_trans(0xCA83);			// enable FIFO
	
    RF12_Index = 0;
    RF12_status.Rx = 1;
    return(0);				//all went fine
}

unsigned char ubrf12_rxfinish(void *data)
{
    unsigned char i;
    char *cdata = (char*)data;
    //not finished yet or old buffer
    if( RF12_status.Rx || !RF12_status.New ){
        return 255;
    }
    RF12_status.New = 0;

    unsigned char len = RF12_Data[0];
    if (len > RF12_DataLength || RF12_Data[len + 1] != RF12_channel) {
        return 255;
    }
    
    for (i = 0; i < len; ++i)
        cdata[i] = RF12_Data[i + 1];
    return len;			//strsize
}

void ubrf12_txstart(const void *data, unsigned char size)
{	
    char *cdata = (char*)data;
    ubleds_tx();
    uint8_t i, l;

    ubrf12_allstop();
    if(size > RF12_DataLength)
        return;          //to big to transmit, ignore
	
    RF12_status.Tx = 1;
    RF12_Index = i = size + 9;      //act -10 

    RF12_Data[i--] = 0xAA;
    RF12_Data[i--] = 0xAA;
    RF12_Data[i--] = 0xAA;
    RF12_Data[i--] = 0x2D;
    RF12_Data[i--] = 0xD4;
    RF12_Data[i--] = size;
    for(l=0; l<size; l++){
        RF12_Data[i--] = cdata[l];
    }
    //TODO: remove these two bytes
    RF12_Data[i--] = RF12_channel;
    RF12_Data[i--] = 0;
    RF12_Data[i--] = 0xAA;
    RF12_Data[i--] = 0xAA;

    ubrf12_trans(0x8238);         // TX on
    return;
}

unsigned char ubrf12_txfinished(void)
{
    if(RF12_status.Tx){
        return 0;        //not yet finished
    }
    return 1;
}

void ubrf12_allstop(void)
{
    ubrf12_trans(0x8208);     //shutdown all
    RF12_status.Rx = 0;
    RF12_status.Tx = 0;
    RF12_status.New = 0;
    ubrf12_trans(0x0000);     //dummy read
}

unsigned char ubrf12_free(void)
{
    if( (ubrf12_trans(0x0000)>>8) & (1<<0) )
        return 0;
    return 1;
}
