#ifndef _PIC32_GPIO_H
#define _PIC32_GPIO_H

struct gpioreg {
    volatile unsigned ansel;            /* Analog select */
    volatile unsigned anselclr;
    volatile unsigned anselset;
    volatile unsigned anselinv;
    volatile unsigned tris;             /* Mask of inputs */
    volatile unsigned trisclr;
    volatile unsigned trisset;
    volatile unsigned trisinv;
    volatile unsigned port;             /* Read inputs, write outputs */
    volatile unsigned portclr;
    volatile unsigned portset;
    volatile unsigned portinv;
    volatile unsigned lat;              /* Read/write outputs */
    volatile unsigned latclr;
    volatile unsigned latset;
    volatile unsigned latinv;
    volatile unsigned odc;              /* Open drain configuration */
    volatile unsigned odcclr;
    volatile unsigned odcset;
    volatile unsigned odcinv;
    volatile unsigned cnpu;             /* Input pin pull-up enable */
    volatile unsigned cnpuclr;
    volatile unsigned cnpuset;
    volatile unsigned cnpuinv;
    volatile unsigned cnpd;             /* Input pin pull-down enable */
    volatile unsigned cnpdclr;
    volatile unsigned cnpdset;
    volatile unsigned cnpdinv;
    volatile unsigned cncon;            /* Interrupt-on-change control */
    volatile unsigned cnconclr;
    volatile unsigned cnconset;
    volatile unsigned cnconinv;
    volatile unsigned cnen;             /* Input change interrupt enable */
    volatile unsigned cnenclr;
    volatile unsigned cnenset;
    volatile unsigned cneninv;
    volatile unsigned cnstat;           /* Change notification status */
    volatile unsigned cnstatclr;
    volatile unsigned cnstatset;
    volatile unsigned cnstatinv;
    volatile unsigned unused[6*4];
};

/* Convert port name/signal into a pin number. */
#define RP(x,n) (((x)-'A'+1) << 4 | (n))

int gpio_input_map1(int);
int gpio_input_map2(int);
int gpio_input_map3(int);
int gpio_input_map4(int);

void gpio_set_input(int pin);
void gpio_set_output(int pin);
void gpio_set_analog(int pin);
void gpio_set(int pin);
void gpio_clr(int pin);
int gpio_get(int pin);

char gpio_portname(int pin);
int gpio_pinno(int pin);

#endif
