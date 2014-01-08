// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield
 
// sinewave.ino program creates a sinusoidal waveform adjustable in amplitude and frequency.
// potentiometer 0 controls the frequency.
// potentiometer 2 controls the amplitude.

// VERSION HISTORY:
// 5.1.2014 - j0uni : This version uses bitwise AND to get the "rolling" pointer to sinewave table.
 
int in_ADC0, in_ADC1;  //variables for 2 ADCs values (ADC0, ADC1)
int POT0, POT1, POT2, out_DAC0, out_DAC1; //variables for 3 pots (ADC8, ADC9, ADC10)
const int LED = 3;
const int FOOTSWITCH = 7; 
const int TOGGLE = 2; 
int accumulator, sample;
 
// Create a table to hold pre computed sinewave, the table has a resolution of 600 samples
#define no_samples 4096 // must be two's complement!     
uint16_t nSineTable[no_samples]; // storing 12 bit samples in 16 bit variable.
 
void createSineTable()
{
  for(uint32_t nIndex=0; nIndex<no_samples; nIndex++)
  {
    // normalised to 12 bit range 0-4095
    nSineTable[nIndex] = (uint16_t)  (((1+sin(((2.0*PI)/no_samples)*nIndex))*4095.0)/2);
  }
}
 
void setup()
{
  createSineTable();
 
  //turn on the timer clock in the power management controller
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);
 
  //we want wavesel 01 with RC 
  TC_Configure(TC1,1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK2);
  TC_SetRC(TC1, 1, 238); // sets <> 44.1 Khz interrupt rate
  TC_Start(TC1, 1);
 
  //enable timer interrupts on the timer
  TC1->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;
 
  //Enable the interrupt in the nested vector interrupt controller 
  //TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number 
  //(=(1*3)+1) for timer1 channel1 
  NVIC_EnableIRQ(TC4_IRQn);
 
  //ADC Configuration
  ADC->ADC_MR |= 0x80;   // DAC in free running mode.
  ADC->ADC_CR=2;         // Starts ADC conversion.
  ADC->ADC_CHER=0x1CC0;  // Enable ADC channels 0,1,8,9 and 10  
 
  //DAC Configuration
  analogWrite(DAC0,0);  // Enables DAC0
  analogWrite(DAC1,0);  // Enables DAC1
}

float pot = 0;
 
void loop()
{
  //Read the ADCs.
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0); // wait for ADC 0, 1, 8, 9, 10 conversion complete.
  in_ADC0=ADC->ADC_CDR[7];            // read data from ADC0
  in_ADC1=ADC->ADC_CDR[6];            // read data from ADC1  

  POT0=ADC->ADC_CDR[10];                  // read data from ADC8          
  POT1=ADC->ADC_CDR[11];                  // read data from ADC9   
  POT2=ADC->ADC_CDR[12];                  // read data from ADC10  
  
  
}
 
//Interrupt at 44.1KHz rate (every 22.6us)
void TC4_Handler()
{
  //Clear status allowing the interrupt to be fired again.
  TC_GetStatus(TC1, 1);
 
  //update the accumulator, from 0 to 511
  accumulator=POT0>>3;
  sample=sample+accumulator;
  
  // Generate the DAC output
  // AND with TWO'S COMPLEMENT value to get the bits we really want to use
  out_DAC0 = nSineTable[sample & (no_samples-1)];
  out_DAC1 = 4095 - nSineTable[sample & (no_samples-1)];
 
  //Add volume feature
  out_DAC0=map(out_DAC0,0,4095,0,POT2);
  out_DAC1=map(out_DAC1,0,4095,0,POT2);
 
  //Write the DACs.
  dacc_set_channel_selection(DACC_INTERFACE, 0);       //select DAC channel 0
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC0);//write on DAC
  dacc_set_channel_selection(DACC_INTERFACE, 1);       //select DAC channel 1
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC1);//write on DAC 
}
