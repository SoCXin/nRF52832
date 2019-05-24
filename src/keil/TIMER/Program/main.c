/* Includes --------------------------------------------------------------------------------*/
#include "drivers\nrf5x_system.h"
#include "drivers\nrf5x_timer.h"
#include "nrf5x_bsp.h"

/** @addtogroup NRF5x_Program
 *  @{
 */

/* Private typedef -------------------------------------------------------------------------*/
/* Private define --------------------------------------------------------------------------*/
/* Private macro ---------------------------------------------------------------------------*/
/* Private variables -----------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------*/
void IRQEvent_TimerTick( void );

/* Private functions -----------------------------------------------------------------------*/

int main( void )
{
  BSP_CLOCK_Config();
  BSP_GPIO_Config();
  BSP_TIMER_Config(IRQEvent_TimerTick, 500);   // 2 KHz

  while (1) {
    LED_B_Toggle();
    delay_ms(100);
  }
}

void IRQEvent_TimerTick( void )
{
  LED_G_Toggle();
}

/*************************************** END OF FILE ****************************************/
