/*
 * Button sensor for ez430-RF2500 platform.
 *
 * Copied from button_sensor.c from Sky target.
 */

#include "lib/sensors.h"
#include "dev/hwconf.h"
#include "dev/button-sensor.h"

const struct sensors_sensor button_sensor;

static struct timer debouncetimer;

HWCONF_PIN(BUTTON, 1, P1BUTTON);
HWCONF_IRQ(BUTTON, 1, P1BUTTON);

/*---------------------------------------------------------------------------*/
static void
init(void)
{
  timer_set(&debouncetimer, 0);
  BUTTON_IRQ_EDGE_SELECTU();

  BUTTON_SELECT();
  BUTTON_MAKE_INPUT();
}
/*---------------------------------------------------------------------------*/
static int
irq(void)
{
  if(BUTTON_CHECK_IRQ()) {
    if(timer_expired(&debouncetimer)) {
      timer_set(&debouncetimer, CLOCK_SECOND / 4);
      sensors_changed(&button_sensor);
      return 1;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static void
activate(void)
{
  sensors_add_irq(&button_sensor, BUTTON_IRQ_PORT());
  BUTTON_ENABLE_IRQ();
}
/*---------------------------------------------------------------------------*/
static void
deactivate(void)
{
  BUTTON_DISABLE_IRQ();
  sensors_remove_irq(&button_sensor, BUTTON_IRQ_PORT());
}
/*---------------------------------------------------------------------------*/
static int
active(void)
{
  return BUTTON_IRQ_ENABLED();
}
/*---------------------------------------------------------------------------*/
static unsigned int
value(int type)
{
  return BUTTON_READ() || !timer_expired(&debouncetimer);
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, void *c)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void *
status(int type)
{
  return NULL;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR,
	       init, irq, activate, deactivate, active,
	       value, configure, status);
