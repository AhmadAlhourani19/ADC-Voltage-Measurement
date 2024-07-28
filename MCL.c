#include <stdint.h>
#include <inc/tm4c1294ncpdt.h>


void configuration(void);
unsigned long readADCValue(void);
void displayDigits(unsigned long);
void waitWithTimer(void);


/**
 * main.c
 */
void main(void) {
    int updateDisplay = 1;
    unsigned long valueADC;
    configuration(); // configure all ports for inputs and outputs
    //int i = 0;

    while (1) {
        if (updateDisplay) { // if updateDisplay == 1, read and display new ADC value
            valueADC = readADCValue();
            displayDigits(valueADC);
        }

        // Check if button pressed
        if (GPIO_PORTD_AHB_DATA_R & 0x01) {
            updateDisplay ^= 1; // Toggle updateDisplay
            waitWithTimer(); // wait for 10ms debounce delay
        }
    }
}

unsigned long readADCValue(void) {
    ADC0_PSSI_R |= 0x01; // start ADC0_SS0
    while (ADC0_SSFSTAT0_R & (1 << 8)); // wait for FIFO non-empty

    // read ADC value from FIFO, convert in mV and display
    unsigned long ulData = (unsigned long)ADC0_SSFIFO0_R * 1.5 * 3300UL / 4095; // 1.5 factor due to voltage divider ratio
    return ulData;
}

void displayDigits(unsigned long value) {
    int thousands = value / 1000;
    int hundreds = (value % 1000) / 100;
    int tens = (value % 100) / 10;
    int ones = value % 10;

    GPIO_PORTL_DATA_R = 0x02; // enable left hand display for thousands and hundreds
    GPIO_PORTM_DATA_R = (thousands << 4) | (hundreds << 0);

    GPIO_PORTL_DATA_R = 0x01; // enable right hand display for tens and ones
    GPIO_PORTM_DATA_R = (tens << 4) | (ones << 0);
}

void configuration(void) {
    /* Configure Port L */
    SYSCTL_RCGCGPIO_R |= (1 << 10); // enable clock port L (number 10, 0x400)
    while (!(SYSCTL_PRGPIO_R & 0x400));
    GPIO_PORTL_DEN_R = 0x03; // enable L(1:0)
    GPIO_PORTL_DIR_R = 0x03; // set as output

    /* Configure Port M */
    SYSCTL_RCGCGPIO_R |= (1 << 11); // enable clock port M (number 11, 0x800)
    while (!(SYSCTL_PRGPIO_R & 0x800)); // wait before accessing port
    GPIO_PORTM_DEN_R = 0xFF; // enable M(7:0)
    GPIO_PORTM_DIR_R = 0xFF; // set as outputs

    /* Configure Port D */
    SYSCTL_RCGCGPIO_R |= (1 << 3); // enable clock port D (number 3, 0x008)
    while (!(SYSCTL_PRGPIO_R & 0x008));
    GPIO_PORTD_AHB_DEN_R = 0x01; // enable D0
    GPIO_PORTD_AHB_DIR_R = 0x00; // set D0 as input

    /* Configure Port E and connect to ADC */
    SYSCTL_RCGCGPIO_R |= (1 << 4); // PE (Port E(0) connects to AIN3
    while (!(SYSCTL_PRGPIO_R & 0x10)); // Ready?
    SYSCTL_RCGCADC_R |= (1 << 0); // ADC0 digital block
    while (!(SYSCTL_PRADC_R & 0x01)); // Ready?

    // configure AIN3 (= PE(0)) as analog input
    GPIO_PORTE_AHB_AFSEL_R |= 0x01; // PE(0) Alternative Pin Function enable
    GPIO_PORTE_AHB_DEN_R &= ~0x01; // PE(0) disable digital IO
    GPIO_PORTE_AHB_AMSEL_R |= 0x01; // PE(0) enable analog function
    GPIO_PORTE_AHB_DIR_R &= ~0x01; // Allow Input PE(0)

    // ADC0_SS0 configuration
    ADC0_ACTSS_R &= ~0x0F; // disable all 4 sequencers of ADC0
    SYSCTL_PLLFREQ0_R |= (1 << 23); // PLL Power
    while (!(SYSCTL_PLLSTAT_R & 0x01)); // until PLL has locked
    ADC0_CC_R |= 0x01; // PIOSC for ADC sampling clock

    ADC0_SSMUX0_R |= 0x00000003; // sequencer 0, channel AIN3
    ADC0_SSEMUX0_R |= 0x00000000; // no offset required for AIN3
    ADC0_SSCTL0_R |= 0x00000002; // END0 set, sequence length = 1
    ADC0_ACTSS_R |= 0x01; // enable sequencer 0 ADC0

    // Configure timer 0
    SYSCTL_RCGCTIMER_R |= (1 << 0); // system clock on timer 0
    while (!(SYSCTL_PRTIMER_R & 0x01)); // wait for timer 0 activation
    TIMER0_CTL_R &= ~0x0001; // disable timer 0
    TIMER0_CFG_R = 0x04; // 16 bit mode
    TIMER0_TAMR_R = 0x01; // count down, one-shot mode, match disabled
    TIMER0_TAPR_R = 3 - 1; // for 10ms timer duration
    TIMER0_TAILR_R = 53333 - 1; // for 10ms timer duration
}

void waitWithTimer(void)
{
    TIMER0_CTL_R |= 0x0001; // enable timer 0
    while ((TIMER0_RIS_R & (1 << 0)) == 0); // wait for time-out
    TIMER0_ICR_R |= (1 << 0); // clear time-out flag
    TIMER0_CTL_R &= ~0x0001; // disable timer 0 I do not know if this step is necessary though in one-shot mode
}