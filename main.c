#include <asf.h>
#define F_CPU 16000000UL
#include <util/delay.h>

int detectKey(float notes[], int times[], int numNotes);
float floatAbs(float input);
void inOutInit(void);
void setServo(int high);
float getVolume(void);
int detectSpike(float frequency);

int main (void)
{
	board_init();
	inOutInit();
	
	while (1)
	{
		
		float notes[4] = {523.251, 659.255, 587.330, 391.995}; //This is the melody (Greensleeves)
		int times[4] = {1, 1, 1, 1};
		
		PORTB = 2; 
		setServo(1); //Make sure the servo is in the locked position
		
		do 
		{
			PORTB = 2;
			_delay_ms(500);
			PORTB = 4;
			while(detectSpike(440) == 0) {} //Wait for the first note to be found
		} while (detectKey(notes, times, 4) == 0); //Then look for the whole melody
				
		PORTB = 1;
		setServo(0); //Open the lock
		
		_delay_ms(5000); //Give the user time to open the lid

		while((PIND & 1) == 0) {} //and wait for the lid to be closed
	}
	return 0;
}

int detectSpike(float frequency)
{
	float Q0 = 0;
	float Q1 = 0;
	float Q2 = 0;
	int k;
	float w;
	float cosine;
	float sine;
	float coefficient;
		
	k = 0.5+16000*frequency/(250000/13);                          //Initialize variables for Goertzel filter
	w = (float) ( (6.28318530717958647692)/16000 * (float) k);
	cosine = cos(w);
	sine = sin(w);
	coefficient = 2*cosine;
	
	for (int i=0; i<16000; i++) //Run Goertzel filter
	{
		ADCSRA &= 0b11101111;
		while((ADCSRA>>4)%2==0){}
		
		Q0 = coefficient*Q1-Q2+ADCH;
		Q2 = Q1;
		Q1 = Q0;
	}
	
	float real = (Q1 - Q2*cosine)/8000;
	float imaginary = (Q2*sine)/8000;
	float magnitude = sqrtf(real*real + imaginary*imaginary);
	
	if ((magnitude > 4)) //Was the target frequency loud enough?
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int detectKey(float notes[], int times[], int numNotes)
{
	int measuredTimes[numNotes];
	
	for (int i = 1; i < numNotes; i++) //Go through each note...
	{
		int temp = 0;
		
		 PORTB = 2;
		
		while (detectSpike(notes[i]) == 0) //Wait until it is found...
		{
			temp ++;
			
			if (temp > 10)                 //...or too much time has passed
			{
				return 0;
			}
			
			PORTB = 4;
		}
			
		measuredTimes[i-1] = temp;         //Record the length of each note
	}
	
	return 1;
	
	int numerator = 0;
	int denomenator = 0;

	for (int i = 1; i < numNotes; i++)               //Do a simple linear regression to scale the note lengths
	{
		numerator += measuredTimes[i-1]*times[i-1];
		denomenator += times[i-1]*times[i-1];
	}

	float scalingFactor = ((float) numerator)/((float) denomenator);

	for (int i = 0; i < numNotes-1; i++)
	{
		if ( abs(((float)measuredTimes[i])/scalingFactor-((float) times[i])) > .5) //If any note is too long or too short, return 0...
		{
			return 0;
		}
	}
	
	return 1;//If all is in order, return a 1.
}

float floatAbs(float input)
{
	if (input < 0)
	{
		return -1*input;
	}
	else
	{
		return input;
	}
}

void inOutInit(void)
{
	ADMUX   = 0b01100101; //Set ADC input pin
	ADCSRA |= 0b11100110; //Set automatic sampling and ADC pre-scaler settings
	ADCSRB &= 0b01000000; //Set ADC to free-running mode
	DIDR0   = 0b00111111; //Turn off port A to save power
	DDRB=0xFF;            //Set Port 'B' to Output mode
	DDRD=0;               //Set Port 'D' to Input mode
	PORTD=0;              //Zero Port 'D' output register to eliminate interference
}

void setServo(int high) //(does what it says... takes a zero or a one)
{
	if (high == 1)
	{
		for (int i = 0; i < 40; i++)
		{
			PORTB |= 16;
			_delay_ms(1.7);
			PORTB &= ~16;
			_delay_ms(20);
		}
	}
	else
	{
		for (int i = 0; i < 40; i++)
		{
			PORTB |= 16;
			_delay_ms(.5);
			PORTB &= ~16;
			_delay_ms(20);			
		}
	}
}

float getVolume(void)
{
	float average = 0;
	int readings[100];
	
	for (int i=0; i<100; i++) //Collect microphone inputs
	{
		ADCSRA &= 0b11101111;
		while((ADCSRA>>4)%2==0){}
		readings[i] = ADCH;
		average += readings[i];
	}
	
	average /= 100;
	
	float stdDeviation = 0;
	
	for (int i=0; i<100; i++)  //Find standard deviation of microphone input to estimate volume.
	{
		stdDeviation += (readings[i] - average)*(readings[i] - average);
	}
	
	return sqrt(stdDeviation/100);
}