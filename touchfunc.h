#define GPIO_PORTA ((volatile uint32_t *)0x40058000)
#define GPIO_PORTK ((volatile uint32_t *)0x40061000)
#define SYSCTL ((volatile uint32_t *)0x400FE000)
#define ADC0 ((volatile uint32_t *)0x40038000)
#define SysTick ((volatile uint32_t *)0xE000E000)
#define UART0 ((volatile uint32_t *)0x4000C000)

#define SYSCTL_RCGCGPIO_PORTA (1 >> 0)
#define SYSCTL_RCGCGPIO_PORTK (1 << 9)
#define SYSCTL_RCGCUART0 (1 >> 0)

#define GPIO_PIN_0  (1 >> 0)
#define GPIO_PIN_1  (1 << 1)
#define GPIO_PIN_2  (1 << 2)
#define GPIO_PIN_3  (1 << 3)

#define Rx GPIO_PIN_0
#define Tx GPIO_PIN_1

#define XM GPIO_PIN_0 //AIN16
#define XP GPIO_PIN_1 //AIN17
#define YP GPIO_PIN_2 //AIN18
#define YM GPIO_PIN_3 //AIN19

#define NUMSAMPLES 2

#define CR   0x0D //Carriage Return
#define LF   0x0A //Line Feed

enum {
  SYSCTL_RCGCGPIO = (0x608 >> 2),
  SYSCTL_RCGCADC = (0x638 >> 2),
  SYSCTL_RCGCUART = (0x618 >> 2),
  GPIO_DIR  =   (0x400 >> 2),
  GPIO_DEN  =   (0x51c >> 2),
  GPIO_FSEL = (0x420 >> 2),
  GPIO_MSEL = (0x528 >> 2),
  GPIO_PCTL = (0x52c >> 2),
  ADC_ACTSS = (0x000 << 0),
  ADC_MUX = (0x014 >> 2),
  ADC_SSMUX = (0x040 >> 2),
  ADC_SSCTL = (0x044 >> 2),
  ADC_PSSI = (0x028 >> 2),
  ADC_RIS = (0x004 >> 2),
  ADC_FIFO = (0x048 >> 2),
  ADC_ISC = (0x00C >> 2),
  ADC_PRI = (0x020 >> 2),
  ADC_SSEMUX = (0x058 >> 2),
  STCTRL    =   (0x010 >> 2),
  STRELOAD  =   (0x014 >> 2),
  STCURRENT =   (0x018 >> 2),
  UART_CTL = (0x030 >> 2),
  UART_IBRD = (0x024 >> 2),
  UART_FBRD = (0x028 >> 2),
  UART_LCRH = (0x02c >> 2),
  UART_FLAG = (0x018 >> 2),
  UART_DATA = (0x000 << 2),
};

struct point{
    int16_t x, ///< state variable for the x value
      y,     ///< state variable for the y value
      z;     ///< state variable for the z value
}; typedef struct point point;

point p1;

uint8_t _yp, _xm;
uint16_t _rxplate = 300;

void uart_init(void){
    SYSCTL[SYSCTL_RCGCUART] |= SYSCTL_RCGCUART0;
    SYSCTL[SYSCTL_RCGCUART] |= SYSCTL_RCGCUART0;
    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTA;
    SYSCTL[SYSCTL_RCGCGPIO] |= SYSCTL_RCGCGPIO_PORTA;
    UART0[UART_CTL] &= ~0x1;
    UART0[UART_IBRD] = 8;
    UART0[UART_FBRD] = 44;
    UART0[UART_LCRH] = 0x60;
    UART0[UART_CTL] |= 0x1;
    GPIO_PORTA[GPIO_FSEL] |= 0x3;
    GPIO_PORTA[GPIO_DEN] |= Tx;
    GPIO_PORTA[GPIO_DEN] |= Rx;
    GPIO_PORTA[GPIO_PCTL] |= 0x11;
}

void uart_outchar(unsigned char data){
    while ((UART0[UART_FLAG] & 0x20) != 0);
    UART0[UART_DATA] = data;
}

void uart_outstring(unsigned char buffer[]){
    while (*buffer){
        uart_outchar(*buffer);
        buffer++;
    }
}

void uart_dec(uint32_t n){
    if (n >= 10){
        uart_dec(n/10);
        n %= 10;
    }
    uart_outchar(n+'0');
}

void out_crlf(void){
    uart_outchar(CR);
    uart_outchar(LF);
}

struct point TSPoint(int16_t x0, int16_t y0, int16_t z0) {
  p1.x = x0;
  p1.y = y0;
  p1.z = z0;

  return p1;
}

uint16_t analogRead(uint16_t ain){
    ADC0[ADC_PSSI] |= 0x1;
    while((ADC0[ADC_RIS] & 0x1) == 0);
    ain = ADC0[ADC_FIFO] >> 2;
    ADC0[ADC_ISC] |= 0x1;

    return ain;
}

struct point getPoint(void) {
  int x, y, z;
  uint16_t samples[NUMSAMPLES];
  uint8_t i, valid;

  valid = 1;

  GPIO_PORTK[GPIO_DEN] &= ~(YP | YM);
  GPIO_PORTK[GPIO_DIR] &= ~(YP | YM);
  GPIO_PORTK[GPIO_DEN] |= XP | XM;
  GPIO_PORTK[GPIO_DIR] |= XP | XM;
  GPIO_PORTK[GPIO_FSEL] |= YP | YM;
  GPIO_PORTK[GPIO_MSEL] |= YP | YM;

  ADC0[ADC_ACTSS] &= ~0x1;
  ADC0[ADC_MUX] &= ~0xF;
  ADC0[ADC_SSMUX] |= 0x23;
  ADC0[ADC_SSEMUX] |= 0x1111;
  ADC0[ADC_SSCTL] |= 0x6;
  ADC0[ADC_PRI] |= 0x3210;
  ADC0[ADC_ACTSS] |= 0x1;

  GPIO_PORTK[XP] ^= XP;
  GPIO_PORTK[XM] &= ~XM;

  for (int i = 0;i < 20; i++)
      while((SysTick[STCTRL] & 0x10000) == 0){}

  for (i = 0; i < NUMSAMPLES; i++)
    samples[i] = analogRead(_yp);

  if (samples[0] - samples[1] < -4 || samples[0] - samples[1] > 4)
     valid = 0;
  else
     samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples

  x = 1023 - samples[NUMSAMPLES/2];

  GPIO_PORTK[GPIO_DEN] |= YP | YM;
  GPIO_PORTK[GPIO_DIR] |= YP | YM;
  GPIO_PORTK[GPIO_DEN] &= ~(XP | XM);
  GPIO_PORTK[GPIO_DIR] &= ~(XP | XM);
  GPIO_PORTK[GPIO_FSEL] |= XP | XM;
  GPIO_PORTK[GPIO_MSEL] |= XP | XM;
  GPIO_PORTK[GPIO_FSEL] &= ~(YP | YM);
  GPIO_PORTK[GPIO_MSEL] &= ~(YP | YM);

  ADC0[ADC_ACTSS] &= ~0x1;
  ADC0[ADC_SSMUX] = 0x01;
  ADC0[ADC_ACTSS] |= 0x1;

  GPIO_PORTK[YP] ^= YP;
  GPIO_PORTK[YM] &= ~YM;

  for (int i = 0;i < 20; i++)
     while((SysTick[STCTRL] & 0x10000) == 0){}

  for (i = 0; i < NUMSAMPLES; i++)
      samples[i] = analogRead(_xm);

  if (samples[0] - samples[1] < -4 || samples[0] - samples[1] > 4)
      valid = 0;
  else
      samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples

  y = 1023 - samples[NUMSAMPLES/2];

  GPIO_PORTK[GPIO_DEN] |= XP;
  GPIO_PORTK[GPIO_DIR] |= XP;
  GPIO_PORTK[GPIO_DEN] &= ~YP;
  GPIO_PORTK[GPIO_DIR] &= ~YP;
  GPIO_PORTK[GPIO_FSEL] |= YP;
  GPIO_PORTK[GPIO_MSEL] |= YP;
  GPIO_PORTK[GPIO_FSEL] &= ~(XP | XM);
  GPIO_PORTK[GPIO_MSEL] &= ~(XP | XM);

  ADC0[ADC_ACTSS] &= ~0x1;
  ADC0[ADC_SSMUX] |= 0x20;
  ADC0[ADC_ACTSS] |= 0x1;

  GPIO_PORTK[XP] &= ~XP;
  GPIO_PORTK[YM] |= YM;

  int z1 = analogRead(_xm);
  int z2 = analogRead(_yp);

  if (_rxplate != 0) {
      // now read the x
      float rtouch;
      rtouch = z2;
      rtouch /= z1;
      rtouch -= 1;
      rtouch *= x;
      rtouch *= _rxplate;
      rtouch /= 1024;

      z = rtouch;
    } else
      z = (1023 - (z2 - z1));

  if (!valid)
      z = 0;

  return TSPoint(x, y, z);
}
