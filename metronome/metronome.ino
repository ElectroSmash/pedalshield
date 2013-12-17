// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield
 
int in_ADC0, in_ADC1, out_DAC0, out_DAC1;  //variables for ADCs and DACs
int POT0, POT1, POT2;  //variables for pots (ADC8, ADC9, ADC10)
const int LED = 3;
const int FOOTSWITCH = 7; 
const int TOGGLE = 2; 
 
int accumulator,sample,time_on, time_off;
 
// Create a table to hold pre computed sinewave, the table has a resolution of 600 samples
#define no_samples 44100
// default int is 32 bit, in most cases its best to use uint32_t but for large arrays its better to use smaller
// data types if possible, here we are storing 12 bit samples in 16 bit ints
uint16_t nSineTable[no_samples];
 
// create the individual samples for our sinewave table
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
 
  /* turn on the timer clock in the power management controller */
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);
 
  /* we want wavesel 01 with RC */
  TC_Configure(/* clock */TC1,/* channel */1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK2);
  TC_SetRC(TC1, 1, 238); // sets <> 44.1 Khz interrupt rate
  TC_Start(TC1, 1);
 
  // enable timer interrupts on the timer
  TC1->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;
 
  /* Enable the interrupt in the nested vector interrupt controller */
  /* TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number
  (=(1*3)+1) for timer1 channel1 */
  NVIC_EnableIRQ(TC4_IRQn);
 
  //ADC Configuration
  ADC->ADC_MR |= 0x80;   // DAC in free running mode.
  ADC->ADC_CR=2;         // Starts ADC conversion.
  ADC->ADC_CHER=0x1CC0;  // Enable ADC channels 0,1,8,9 and 10  
 
  //DAC Configuration
  analogWrite(DAC0,0);  // Enables DAC0
  analogWrite(DAC1,0);  // Enables DAC0
 
  //Pin Configuration
  pinMode(LED, OUTPUT);  
  pinMode(FOOTSWITCH, INPUT);     
  pinMode(TOGGLE, INPUT);    
}
 
void loop()
{
//BEEP PART: Enable Interruption which makes the sinewave.   
if (digitalRead(TOGGLE)) digitalWrite(LED, HIGH); 
NVIC_EnableIRQ(TC4_IRQn);  
delay(50); //the beep has a constant time of 50ms.
 
//SILENT PART: Disabling Interruption which makes the sinewave.
digitalWrite(LED, LOW); 
NVIC_DisableIRQ(TC4_IRQn);
 
sample=0;
//adjusing metronome from (1000/1000)x60=60bpm to (1000/150)x60=400bpm 
time_on =map(POT0,0,4095,1000,150); 
delay(time_on);
}
 
void TC4_Handler()
{
  // We need to get the status to clear it and allow the interrupt to fire again
  TC_GetStatus(TC1, 1);
 
  //Read ADCs.
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0);// wait for ADC 0, 1, 8, 9, 10 conversion complete.
  in_ADC0=ADC->ADC_CDR[7];           // read data from ADC0
  in_ADC1=ADC->ADC_CDR[6];           // read data from ADC1  
  POT0=ADC->ADC_CDR[10];                // read data from ADC8        
  POT1=ADC->ADC_CDR[11];                // read data from ADC9   
  POT2=ADC->ADC_CDR[12];                // read data from ADC10  
 
  //Adjust frequency from 1Hz to 2.5KHz 
  accumulator =map(POT1,0,4095,1,2500);
  sample=sample+accumulator;
  if(sample>=no_samples)sample=0;
 
  //calculate the samples
  out_DAC0 = nSineTable[sample];
  //Output_DAC1 = 4095-nSineTable[sample];
 
  //calculate the samples
  out_DAC0 = (nSineTable[sample]);
  //Output_DAC1 = (4095-nSineTable[sample]);
 
  //to add volume feature
  out_DAC0 =map(out_DAC0,0,4095,POT2,1);
  //Output_DAC1 =map(Output_DAC1,0,4095,pot2,1);
 
  //Write the DACs
  dacc_set_channel_selection(DACC_INTERFACE, 0);          //select DAC channel 0
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC0);//write on DAC
  dacc_set_channel_selection(DACC_INTERFACE, 1);          //select DAC channel 1
  dacc_write_conversion_data(DACC_INTERFACE, 0);//write on DAC 
}


