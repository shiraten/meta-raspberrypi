#include <stdio.h>
#include <wiringPi.h>

// définition des entrée/sortie
const int LED = 1;	// doit être la PIN 1

// s'exécute une seule fois
void setup()
{
	printf("PWM\n");
	pinMode(LED, PWM_OUTPUT) ;
}

// s'exécute en boucle
void loop()
{
	int i;
	for(i = 0; i < 512; i++) {
		pwmWrite(LED, i);  // plage 0..1023
		delay(2);	// 2 ms
	}
	for (i = 512; i > 0; i--) {
		pwmWrite(LED, i);
		delay(2);	// 2 ms
	}
}


int main()
{
	wiringPiSetup();
	// doit toujours être déclarée
	setup();
	// initialisation
	for(;;)
		loop();	// programme en boucle
	return 0;
}
