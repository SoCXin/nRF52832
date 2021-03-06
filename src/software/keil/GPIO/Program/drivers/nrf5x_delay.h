/**
 *      __            ____
 *     / /__ _  __   / __/                      __  
 *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                    /_/   github.com/KitSprout    
 * 
 *  @file    nrf5x_delay.h
 *  @author  KitSprout
 *  @date    01-Dec-2017
 *  @brief   
 * 
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef __NRF5x_DELAY_H
#define __NRF5x_DELAY_H

#ifdef __cplusplus
 extern "C" {
#endif
/* Includes --------------------------------------------------------------------------------*/
#include "nrf.h"

/* Exported types --------------------------------------------------------------------------*/
/* Exported constants ----------------------------------------------------------------------*/
/* Exported macro --------------------------------------------------------------------------*/
/* Exported functions ----------------------------------------------------------------------*/
void delay_nop( __IO uint32_t __n );
void delay_us( __IO uint32_t __us );
void delay_ms( __IO uint32_t __ms );

#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
