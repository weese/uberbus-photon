/*
 *
 *  RFM12 Library
 *
 *  Developer:
 *     Benedikt K.
 *     Juergen Eckert
 *
 *  Version: 2.0.1
 *
 */



#ifndef __RFM12_H
#define __RFM12_H

#define RF12_RESET_PIN A0
#define RF12_IRQ_PIN A1

/* config */
#define RF12_DataLength	50		//RF12 Buffer length
					//max length 244

/* ------ */

#define RxBW400		1
#define RxBW340		2
#define RxBW270		3
#define RxBW200		4
#define RxBW134		5
#define RxBW67		6

#define TxBW15		0
#define TxBW30		1
#define TxBW45		2
#define TxBW60		3
#define TxBW75		4
#define TxBW90		5
#define TxBW105		6
#define TxBW120		7

#define LNA_0		0
#define LNA_6		1
#define LNA_14		2
#define LNA_20		3

#define RSSI_103	0
#define RSSI_97		1
#define RSSI_91		2
#define RSSI_85		3
#define RSSI_79		4
#define RSSI_73		5
#define RSSI_67		6
#define	RSSI_61		7

#define PWRdB_0		0
#define PWRdB_3		1
#define PWRdB_6		2
#define PWRdB_9		3
#define PWRdB_12	4
#define PWRdB_15	5
#define PWRdB_18	6
#define PWRdB_21	7

#define RF12TxBDW(kfrq)	((unsigned char)(kfrq/15)-1)
#define RF12FREQ(freq)	((freq-430.0)/0.0025)	// macro for calculating frequency value out of frequency in MHz

struct RF12_stati
{
    unsigned char Rx:1;
    unsigned char Tx:1;
    unsigned char New:1;
};

extern struct RF12_stati RF12_status;

void ubleds_rx(void);
void ubleds_rxend(void);
void ubleds_tx(void);
void ubleds_txend(void);

// transfer 1 word to/from module
unsigned short ubrf12_trans(unsigned short wert);

// initialize module
void ubrf12_init(unsigned char channel);

// set center frequency
void ubrf12_setfreq(unsigned short freq);

// set baudrate
void ubrf12_setbaud(unsigned short baud);

// set transmission settings
void ubrf12_setpower(unsigned char power, unsigned char mod);

// set receiver settings
void ubrf12_setbandwidth(unsigned char bandwidth, unsigned char gain, unsigned char drssi);


// start receiving a package
unsigned char ubrf12_rxstart(void);

// readout the package, if one arrived
unsigned char ubrf12_rxfinish(void *data);

// start transmitting a package of size size
void ubrf12_txstart(const void *data, unsigned char size);

// check whether the package is already transmitted
unsigned char ubrf12_txfinished(void);

// stop all Rx and Tx operations
void ubrf12_allstop(void);

unsigned char ubrf12_free(void);

#endif
