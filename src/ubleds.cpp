#include "ubleds.h"
#include "application.h"

void ubleds_init(void)
{
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

