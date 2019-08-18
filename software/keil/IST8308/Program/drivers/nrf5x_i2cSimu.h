/**
 *       __            ____
 *      / /__ _  __   / __/                      __  
 *     / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *    / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *   /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                     /_/   github.com/KitSprout    
 *  
 *  @file    nrf5x_i2cSimu.h
 *  @author  KitSprout
 *  @date    26-Nov-2017
 *  @brief   
 * 
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef __NRF5X_I2CSIMU_H
#define __NRF5X_I2CSIMU_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------*/
#include "nrf.h"

/* Exported types --------------------------------------------------------------------------*/
typedef struct {
  uint32_t PinSDA;
  uint32_t PinSCL;
  uint32_t Freguency;
} SimuI2C_InitTypeDef;

/* Exported constants ----------------------------------------------------------------------*/
/* Exported functions ----------------------------------------------------------------------*/  
void    SimuI2C_Config( SimuI2C_InitTypeDef *SimuI2C_Init );
void    SimuI2C_Start( SimuI2C_InitTypeDef *SimuI2C_Init );
void    SimuI2C_Stop( SimuI2C_InitTypeDef *SimuI2C_Init );
void    SimuI2C_SendACK( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t sendACK );
uint8_t SimuI2C_RecvACK( SimuI2C_InitTypeDef *SimuI2C_Init );
void    SimuI2C_SendByte( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t sendByte );
uint8_t SimuI2C_RecvByte( SimuI2C_InitTypeDef *SimuI2C_Init );
void    SimuI2C_WriteData( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t slaveAddr, uint8_t *writeData, uint8_t lens );
void    SimuI2C_ReadData( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t slaveAddr, uint8_t *readData, uint8_t lens );
void    SimuI2C_WriteReg( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t slaveAddr, uint8_t writeAddr, uint8_t writeData );
void    SimuI2C_ReadReg( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t slaveAddr, uint8_t readAddr, uint8_t *readData );
void    SimuI2C_WriteRegs( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t slaveAddr, uint8_t writeAddr, uint8_t *writeData, uint8_t lens );
void    SimuI2C_ReadRegs( SimuI2C_InitTypeDef *SimuI2C_Init, uint8_t slaveAddr, uint8_t readAddr, uint8_t *readData, uint8_t lens );

#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
