// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield

/* reverb.ino creates two copies of the guitar signal to be delayed independently producing
  a reverb-like sound, the toggle switch selects between delaying or echoing the signals*/
  
int in_ADC0, in_ADC1;  //variables for 2 ADCs values (ADC0, ADC1)
int POT0, POT1, POT2, out_DAC0, out_DAC1; //variables for 3 pots (ADC8, ADC9, ADC10)
int LED = 3;
int FOOTSWITCH = 7; 
int TOGGLE = 2; 
 
#define MAX_DELAY_A 20000
#define MAX_DELAY_B 20000
uint16_t DelayBuffer_A[MAX_DELAY_A];
uint16_t DelayBuffer_B[MAX_DELAY_B];
unsigned int DelayCounter_A = 0;
unsigned int DelayCounter_B = 0;
unsigned int Delay_Depth_A, Delay_Depth_B;
 
void setup()
{
  //turn on the timer clock in the power management controller
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);
 
  //we want wavesel 01 with RC 
  TC_Configure(TC1,1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK2);
  TC_SetRC(TC1, 1, 238); // sets <> 44.1 Khz interrupt rate
  TC_Start(TC1, 1);
 
  // enable timer interrupts on the timer
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
  analogWrite(DAC1,0);  // Enables DAC0
 
  //Pin Configuration
  pinMode(LED, OUTPUT);  
  pinMode(FOOTSWITCH, INPUT);     
  pinMode(TOGGLE, INPUT);     
}
 
void loop()
{
  //Read the ADCs
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0);// wait for ADC 0, 1, 8, 9, 10 conversion complete.
  in_ADC0=ADC->ADC_CDR[7];           // read data from ADC0
  in_ADC1=ADC->ADC_CDR[6];           // read data from ADC1  
  POT0=ADC->ADC_CDR[10];                // read data from ADC8        
  POT1=ADC->ADC_CDR[11];                // read data from ADC9   
  POT2=ADC->ADC_CDR[12];                // read data from ADC10     
}
 
void TC4_Handler() //Interrupt at 44.1KHz rate (every 22.6us)
{
  //Clear status allowing the interrupt to be fired again.
  TC_GetStatus(TC1, 1);
 
    //Check the TOGGLE SWITCH and select between super-reverb and reverb.
    if (digitalRead(TOGGLE))
  { 
     //Store current readings in ECHO mode
     DelayBuffer_A[DelayCounter_A]=(in_ADC0 + (DelayBuffer_A[DelayCounter_A]))>>1;
     DelayBuffer_B[DelayCounter_B]=(in_ADC1 + (DelayBuffer_B[DelayCounter_B]))>>1; 
     digitalWrite(LED, HIGH); 
  }
 
    else 
  {
     //Store current readings in DELAY mode  
     DelayBuffer_A[DelayCounter_A]  = in_ADC0 ;
     DelayBuffer_B[DelayCounter_B]  = in_ADC1 ;  
     digitalWrite(LED, LOW);
  }
 
  //Adjust Delay Depth based in POT0 and POT1 position.
  Delay_Depth_A =map(POT0>>3,0,512,1,MAX_DELAY_A);
  Delay_Depth_B =map(POT1>>3,0,512,1,MAX_DELAY_B);
 
  //Increse/reset delay counter.   
  DelayCounter_A++;
  DelayCounter_B++;
  if(DelayCounter_A >= Delay_Depth_A) DelayCounter_A = 0; 
  if(DelayCounter_B >= Delay_Depth_B) DelayCounter_B = 0; 
 
  //Calculate the output as the sum of DelayBuffer_A + DelayBuffer_B 
  out_DAC0 = (DelayBuffer_A[DelayCounter_A]);
  out_DAC1 = (DelayBuffer_B[DelayCounter_B]);
 
  //Add volume feature based in pot2 position.
  out_DAC0=map(out_DAC0,0,4095,1,POT2);
  out_DAC1=map(out_DAC1,0,4095,1,POT2);
 
  //Write the DACs
  dacc_set_channel_selection(DACC_INTERFACE, 0);       //select DAC channel 0
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC0);//write on DAC
  dacc_set_channel_selection(DACC_INTERFACE, 1);       //select DAC channel 1
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC1);//write on DAC
}
