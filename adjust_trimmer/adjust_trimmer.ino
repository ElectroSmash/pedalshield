// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield


/*The program helps to adjust the trimmer 1 (RV1).
when the input level is too high, the led turns on.*/

int in_ADC0, in_ADC1;  //variables for 2 ADCs values (ADC0, ADC1)
int POT0, POT1, POT2, in_DAC0, in_DAC1; //variables for 3 pots (ADC8, ADC9, ADC10)
int LED = 3;
int FOOTSWITCH = 7; 
int TOGGLE = 2; 

void setup()
{
  //ADC Configuration
  ADC->ADC_MR |= 0x80;   // DAC in free running mode.
  ADC->ADC_CR=2;         // Starts ADC conversion.
  ADC->ADC_CHER=0x1CC0;  // Enable ADC channels 0,1,8,9 and 10  
  
  pinMode(LED, OUTPUT);   
}

void loop()
{
  //Read the ADCs
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0);// wait for ADC 0, 1, 8, 9, 10 conversion complete.
  in_ADC0=ADC->ADC_CDR[7];           // read data from ADC0
  in_ADC1=ADC->ADC_CDR[6];           // read data from ADC1  

  if (in_ADC0>=4090) 
   {
    digitalWrite(LED, HIGH); 
    delay(200);
   }
   
  else 
    digitalWrite(LED, LOW); 
}

