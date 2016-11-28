//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - ADC
// Application Overview - The application is a reference to usage of ADC DriverLib 
//                        functions on CC3200. /Users Developerscan refer to this
//                        simple application and re-use the functions in 
//                        their applications.
//
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_ADC
// or
// docs\examples\CC32xx_ADC.pdf
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup adc_example
//! @{
//
//*****************************************************************************

// Standard includes
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Driverlib includes
#include "utils.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_adc.h"
#include "hw_ints.h"
#include "hw_gprcm.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"
#include "pinmux.h"
#include "pin.h"
#include "adc.h"

#include "adc_userinput.h"
#include "uart_if.h"

#define USER_INPUT 
#define UART_PRINT         Report
#define FOREVER            1
#define APP_NAME           "ADC Reference"
#define NO_OF_SAMPLES 		128

unsigned long pulAdcSamples[4096];

//*****************************************************************************
//                      GLOBAL VARIABLES
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
static void BoardInit(void);
static void DisplayBanner(char * AppName);

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t     Emotion Detection System \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}
// CC3200 %s Application (In the Middle)

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//! main - calls Crypt function after populating either from pre- defined vector 
//! or from User
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void 
main()
 {
    unsigned long  uiAdcInputPin;
    unsigned int  heartuiChannel;
    unsigned int  skinuiChannel;
    unsigned int  uiIndex=0;
    unsigned long ulSample;

    //
    // Initialize Board configurations
    //
    BoardInit();

    //
    // Configuring UART for Receiving input and displaying output
    // 1. PinMux setting
    // 2. Initialize UART
    // 3. Displaying Banner
    //
    PinMuxConfig();
    InitTerm();
    DisplayBanner(APP_NAME);
    MAP_UtilsDelay(800000);

    while(FOREVER)
    {
        //
        // Initialize Array index for multiple execution
        //
        uiIndex=0;
      
        //
        // Configure Heart Pin
        //
        uiAdcInputPin = PIN_60;

#ifdef CC3200_ES_1_2_1
        //
        // Enable ADC clocks.###IMPORTANT###Need to be removed for PG 1.32
        //
        HWREG(GPRCM_BASE + GPRCM_O_ADC_CLK_CONFIG) = 0x00000043;
        HWREG(ADC_BASE + ADC_O_ADC_CTRL) = 0x00000004;
        HWREG(ADC_BASE + ADC_O_ADC_SPARE0) = 0x00000100;
        HWREG(ADC_BASE + ADC_O_ADC_SPARE1) = 0x0355AA00;
#endif
        //
        // Pinmux for the selected ADC input pin
        //
        MAP_PinTypeADC(uiAdcInputPin,PIN_MODE_255);

        //
        // Convert pin number to channel number
        //
        heartuiChannel = ADC_CH_3;

        //
        // Configure ADC timer which is used to timestamp the ADC data samples
        //
        MAP_ADCTimerConfig(ADC_BASE,2^17);

        //
        // Enable ADC timer which is used to timestamp the ADC data samples
        //
        MAP_ADCTimerEnable(ADC_BASE);

        //
        // Enable ADC module
        //
        MAP_ADCEnable(ADC_BASE);

        //
        // Enable ADC Heart channel
        //
        MAP_ADCChannelEnable(ADC_BASE, heartuiChannel);

        while(uiIndex < NO_OF_SAMPLES + 4)
        {
            if(MAP_ADCFIFOLvlGet(ADC_BASE, heartuiChannel))
            {
                ulSample = MAP_ADCFIFORead(ADC_BASE, heartuiChannel);
                pulAdcSamples[uiIndex++] = ulSample;
            }
        }
        MAP_ADCChannelDisable(ADC_BASE, heartuiChannel);

        //
        // Print out ADC Heart samples
        //
        uiIndex = 0;
        float heartNumber;

        while(uiIndex < 1)
        {
        	// Obtain Heart Rate Voltage
            UART_PRINT("\n\rHeart Rate Voltage is %f\n\r",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);
            heartNumber = (float)(pulAdcSamples[4+uiIndex] >> 2 & 0x0FFF)*1.4/4096;

            uiIndex++;
        }
        MAP_UtilsDelay(8000000);

        //
        // Configure Skin Conductance Pin
        //
        uiAdcInputPin = PIN_59;

        //
        // Pinmux for the selected ADC input pin
        //
        MAP_PinTypeADC(uiAdcInputPin,PIN_MODE_255);

        //
        // Convert pin number to channel number
        //
        skinuiChannel = ADC_CH_2;

        //
        // Enable ADC Skin channel
        //
        MAP_ADCChannelEnable(ADC_BASE, skinuiChannel);

        while(uiIndex < NO_OF_SAMPLES + 4)
        {

        	if(MAP_ADCFIFOLvlGet(ADC_BASE, skinuiChannel))
            {
        		ulSample = MAP_ADCFIFORead(ADC_BASE, skinuiChannel);
                pulAdcSamples[uiIndex++] = ulSample;
            }
        }

        //
        // Disable Skin Conductance Channel
        //
        MAP_ADCChannelDisable(ADC_BASE, skinuiChannel);

        //
        // Print Out ADC Skin Conductance Samples
        //
        uiIndex = 0;
        float skinNumber;

        while(uiIndex < 1)
        {
        	UART_PRINT("Skin Conductance Voltage is %f\n\r",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);
        	skinNumber = (float)(pulAdcSamples[4+uiIndex] >> 2 & 0x0FFF)*1.4/4096;
        	uiIndex++;
        }

        MAP_UtilsDelay(8000000);

        //
        // Values for Estimating Emotion
        //
        if((heartNumber >= .6 && heartNumber <= .65) && (skinNumber >= .63 && skinNumber <= .65))
        {
        	UART_PRINT("========================\n\rCURRENT EMOTION: NERVOUS.\n\r========================");
        }
        else if((heartNumber >= .65 && heartNumber <= 1.4) && (skinNumber >= .57 && skinNumber <= .58))
        {
        	UART_PRINT("========================\n\rCURRENT EMOTION: NEUTRAL\n\r========================");
        }
        else if((heartNumber >= .9 && heartNumber <= 1.5) && (skinNumber >= .581 && skinNumber <= .66))
        {
            UART_PRINT("========================\n\rCURRENT EMOTION: HAPPY\n\r========================");
        }
        else if((heartNumber >= 0 && heartNumber <= .65) && (skinNumber >= 0 && skinNumber <= .61))
        {
            UART_PRINT("========================\n\rCURRENT EMOTION: SAD\n\r========================");
        }
        else
        {
        	UART_PRINT("========================\n\rUNABLE TO DETECT EMOTION. CURRENTLY ANALYZING DATA.\n\r========================");
        }

        UART_PRINT("\n\r");

    }

}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
