#include "Emma-API.h"

#define LED0 5
#define LED1 6
#define LED2 7

enum {
  PWM0,
  PWM1,
  PWM2,
  PWM3
};

RESSOURCE_LIST_LOAD(ressources_list);

// Declaration of the ressource
RESSOURCE_INIT(led0);
RESSOURCE_INIT(PwmLed0);

void setLed0(int val){
  if(val) HARDWARE_DIGIT_SET_UP(PORTB, LED0);
  else    HARDWARE_DIGIT_SET_DOWN(PORTB,LED0);
}

int getLed0(){
  return (HARDWARE_DIGIT_GET(PORTB, LED0) != 0);
}

void setPwmLed0(int val){
  HARDWARE_PWM_SET(PWM1,val);
}

int getPwmLed0(){
  HARDWARE_PWM_GET(PWM1);
}

RESSOURCES_INIT(){
  HARDWARE_PWM_INIT();
}

// Connection between the driver API and the REST API
RESSOURCES_LOADER(){

	RESSOURCE_UNIT(led0,boolean);
	RESSOURCE_GET(led0,&getLed0);
	RESSOURCE_SET(led0,&setLed0);

	RESSOURCE_UNIT(PwmLed0,%);
	RESSOURCE_SET(PwmLed0,&setPwmLed0);
	RESSOURCE_GET(PwmLed0,&getPwmLed0);
}
