/* Includes --------------------------------------------------------------------------------*/
#include "drivers\nrf5x_system.h"
#include "nrf5x_bsp.h"

/** @addtogroup NRF5x_Program
 *  @{
 */

/* Private typedef -------------------------------------------------------------------------*/
/* Private define --------------------------------------------------------------------------*/
/* Private macro ---------------------------------------------------------------------------*/
/* Private variables -----------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------*/
/* Private functions -----------------------------------------------------------------------*/

int main( void )
{
  BSP_CLOCK_Config();
  BSP_GPIO_Config();

  while (1) {
    LED_R_Toggle();
    delay_ms(80);
    LED_G_Toggle();
    delay_ms(80);
    LED_B_Toggle();
    delay_ms(80);
    while (KEY_Read()) {
      LED_R_Set();
      LED_G_Set();
      LED_B_Set();
      delay_ms(100);
      LED_R_Reset();
      LED_G_Reset();
      LED_B_Reset();
      delay_ms(100);
    }
  }
}

/*************************************** END OF FILE ****************************************/
