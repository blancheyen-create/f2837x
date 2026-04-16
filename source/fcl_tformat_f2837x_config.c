//#############################################################################
// File Name        :   fcl_tformat_f2837x_config.c
//#############################################################################
//
// Contains all the initialization, data declarations and setup for tformat
// encoder interface. This file serves are a template for using PM_tformat
// Library to interface and incorporates all the encoder and library specific
// initializations and other important aspects of usage.
//
//#############################################################################
// $Copyright:
// Copyright (C) 2017-2025 Texas Instruments Incorporated - http://www.ti.com/
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################


//
// includes
//
#include "fcl_tformat_f2837x_config.h"

#ifdef _FLASH
#ifndef __cplusplus
#pragma CODE_SECTION(spiRxFIFOISR,".TI.ramfunc");
#pragma CODE_SECTION(readTformatEncPosition,".TI.ramfunc");
#pragma CODE_SECTION(tformat_checkCRC,".TI.ramfunc");
#endif
#endif

uint16_t tformatCRCtable[PM_TFORMAT_CRCTABLE_SIZE];  //table - CRC calculations

//
// PM tformat data structure
// RX data buffer.  Filled in the SPI RX ISR
//
PM_tformat_DataStruct tformatData;
volatile uint16_t tformatRxData[PM_TFORMAT_FIELDS_MAX];

//
// Function to initialize tformat operation
//
void tformat_init(void)
{

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM4);

    //
    //Configure EPWM4 to drive default values on GPIO7
    //
    tformat_configEPWM4();

    //
    // Generate table for tformat Polynomial defied as POLY
    //
    PM_tformat_generateCRCTable(PM_TFORMAT_NBITS_POLY,
                                PM_TFORMAT_POLY,
                                tformatCRCtable);

    //
    //GPIO configuration for tformat operation
    //
    tformat_setupGPIO();

    //
    //XBAR configuration for tformat operation
    //
    tformat_configXBAR();

    PM_tformat_setupPeriph(DEVICE_LSPCLK_FREQ);
    PM_tformat_setFreq(TFORMAT_FREQ_DIVIDER);
    tformat_configCLBConnect();

    //
    // Must put SPI into reset before configuring it
    //
    SPI_disableModule(ENCODER_SPI_BASE);
    SPI_disableInterrupt(ENCODER_SPI_BASE, SPI_INT_RXFF);

    //
    // FIFO and interrupt configuration
    //
    SPI_clearInterruptStatus(ENCODER_SPI_BASE, SPI_INT_RXFF);
    SPI_enableInterrupt(ENCODER_SPI_BASE, SPI_INT_RXFF);

    //
    // Configuration complete. Enable the module.
    //
    SPI_enableModule(ENCODER_SPI_BASE);

    //
    // Power up tformat 5v supply through GPIO139
    //
    GPIO_writePin(ENC_PWREN_PIN, 1);
    DEVICE_DELAY_US(TFORMAT_POWER_ON_DELAY_US);

    //
    // Optional
    // Change to #if 1 to test all of the T-format commands
    //
#if 0
    EINT;
    tformat_exCommands();
    DINT;
#endif
}

void tformat_setupGPIO(void)
{

    //
    // CLB_SPI_CLOCK.  Must be routed outside of
    // the device to the SPI CLK input pin.
    //
    GPIO_setMasterCore(ENC_CLK_PWM_PIN, GPIO_CORE_CPU1);
    GPIO_setPinConfig(ENC_CLK_PWM_CFG);

    //
    // Setup SPISIMO. This is the response from the
    // encoder.
    //
    GPIO_setMasterCore(ENC_SPI_SIMO_PIN, GPIO_CORE_CPU1);
    GPIO_setPinConfig(ENC_SPI_SIMO_CFG);
    GPIO_setQualificationMode(ENC_SPI_SIMO_PIN, GPIO_QUAL_ASYNC);

    //
    // Setup SPISOMI.  This is the request to the encoder.
    //
    GPIO_setMasterCore(ENC_SPI_SOMI_PIN, GPIO_CORE_CPU1);
    GPIO_setPinConfig(ENC_SPI_SOMI_CFG);
    GPIO_setQualificationMode(ENC_SPI_SOMI_PIN, GPIO_QUAL_ASYNC);

    //
    // Setup SPICLK. Must be connected to the CLB_SPI_CLK
    // output.
    //
    GPIO_setMasterCore(ENC_SPI_CLK_PIN, GPIO_CORE_CPU1);
    GPIO_setPinConfig(ENC_SPI_CLK_CFG);
    GPIO_setQualificationMode(ENC_SPI_CLK_PIN, GPIO_QUAL_ASYNC);

    //
    // Setup SPISTE.  This pin is tied to ground.
    //
    GPIO_setMasterCore(ENC_SPI_STE_PIN, GPIO_CORE_CPU1);
    GPIO_setPinConfig(ENC_SPI_STE_CFG);
    GPIO_setQualificationMode(ENC_SPI_STE_PIN, GPIO_QUAL_ASYNC);

    //
    // Setup tformat TxEN.  This controls the TX/RX of the
    // RS485 drivers.
    //
    GPIO_setMasterCore(ENC_TXEN_PIN, GPIO_CORE_CPU1);
    GPIO_setPinConfig(ENC_TXEN_CFG);  // out x bar

    //
    // Setup PwrEN.  This enables power to the encoder.
    //
    GPIO_setMasterCore(ENC_PWREN_PIN, GPIO_CORE_CPU1);
    GPIO_setDirectionMode(ENC_PWREN_PIN, GPIO_DIR_MODE_OUT);
}

void tformat_configXBAR(void)
{
    //
    // SIMO to INPUTXBAR1 to CLB AUXSIG0
    //
    XBAR_setInputPin(XBAR_INPUT1, ENC_SPI_SIMO_PIN);
    XBAR_setCLBMuxConfig(XBAR_AUXSIG0, XBAR_CLB_MUX01_INPUTXBAR1);
    XBAR_enableCLBMux(XBAR_AUXSIG0, XBAR_MUX01);

    //
    // CLB TXeN to OUTPUTXBAR1
    //
    XBAR_setOutputMuxConfig(XBAR_OUTPUT1, XBAR_OUT_MUX13_CLB4_OUT4);
    XBAR_enableOutputMux(XBAR_OUTPUT1, XBAR_MUX13);
}

void tformat_configCLBConnect(void)
{
    //
    // ENCODER_CLB input/output configuration
    //
    CLB_setOutputMask(ENCODER_CLB_BASE,
                (0UL << 0UL), true);

    //
    // ENCODER_CLB CLB_IN0 initialization
    //
    CLB_configLocalInputMux(ENCODER_CLB_BASE,
                            CLB_IN0,
                            CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configGlobalInputMux(ENCODER_CLB_BASE,
                            CLB_IN0,
                            CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGPInputMux(ENCODER_CLB_BASE,
                            CLB_IN0,
                            CLB_GP_IN_MUX_GP_REG);
    CLB_selectInputFilter(ENCODER_CLB_BASE,
                            CLB_IN0,
                            CLB_FILTER_RISING_EDGE);

    //
    // ENCODER_CLB CLB_IN1 initialization
    //
    CLB_configLocalInputMux(ENCODER_CLB_BASE,
                            CLB_IN1,
                            CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configGlobalInputMux(ENCODER_CLB_BASE,
                            CLB_IN1,
                            CLB_GLOBAL_IN_MUX_CLB_AUXSIG0);
    CLB_configGPInputMux(ENCODER_CLB_BASE,
                            CLB_IN1,
                            CLB_GP_IN_MUX_EXTERNAL);
    CLB_enableSynchronization(ENCODER_CLB_BASE,
                            CLB_IN1);
    CLB_selectInputFilter(ENCODER_CLB_BASE,
                            CLB_IN1,
                            CLB_FILTER_FALLING_EDGE);

    //
    // ENCODER_CLB CLB_IN7 initialization
    //
    CLB_configLocalInputMux(ENCODER_CLB_BASE,
                            CLB_IN7,
                            CLB_LOCAL_IN_MUX_GLOBAL_IN);
    CLB_configGlobalInputMux(ENCODER_CLB_BASE,
                            CLB_IN7,
                            CLB_GLOBAL_IN_MUX_EPWM1A);
    CLB_configGPInputMux(ENCODER_CLB_BASE,
                            CLB_IN7,
                            CLB_GP_IN_MUX_GP_REG);
    CLB_selectInputFilter(ENCODER_CLB_BASE,
                            CLB_IN7,
                            CLB_FILTER_NONE);
    CLB_setGPREG(ENCODER_CLB_BASE, 0);
    CLB_disableCLB(ENCODER_CLB_BASE);
}

void tformat_error(void)
{
    ESTOP0;     //Test failed!! Stop!
    for (;;);
}


interrupt void spiRxFIFOISR(void)
{
    uint16_t i;

    for(i = 0; i <= tformatData.fifoLevel; i++)
    {
        tformatRxData[i] = SPI_readDataNonBlocking(ENCODER_SPI_BASE) & 0xFF;
    }

    readTformatEncPosition();

    SPI_clearInterruptStatus(PM_TFORMAT_SPI, SPI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
    tformatData.dataReady = 1;
}


void tformat_configEPWM4(void)
{

    //
    // Set the PWMA and B high as default values of tformat clk.
    // Action on TZ1
    //
    EPWM_setTripZoneAction(EPWM4_BASE,
                           EPWM_TZ_ACTION_EVENT_TZA,
                           EPWM_TZ_ACTION_HIGH);


    //
    // Action on TZ1
    //
    EPWM_setTripZoneAction(EPWM4_BASE,
                           EPWM_TZ_ACTION_EVENT_TZB,
                           EPWM_TZ_ACTION_HIGH);

    //
    // Forces a Trip Zone event
    //
    EPWM_forceTripZoneEvent(EPWM4_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

uint16_t tformat_checkCRC (uint16_t expectCRC5, uint16_t receiveCRC5)
{
    if(expectCRC5 == receiveCRC5)
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

//
//This function executes all the tformat commands by calling the tformat
//library functions with different IDs.
//This is an optional function that can be used as a sanity checker
//This function is of no relevance during runtime
//At the end of each command execution CRC is also checked.
//

uint16_t tformat_exCommands(void)
{

    uint16_t retval1;
    uint32_t address, data;

    PM_tformat_setupCommandReadoutOrReset(PM_TFORMAT_ID3,
                                         PM_TFORMAT_RX_CLOCKS_ID3,
                                         PM_TFORMAT_RX_FIELDS_ID3,
                                         PM_TFORMAT_TX_CLOCKS_ID3,
                                         PM_TFORMAT_FIFO_LEVEL_ID3);
    PM_tformat_startOperation();
    while(tformatData.dataReady != 1) ;//{;}
    PM_tformat_receiveDataID3();
    if(tformatData.crcCheck != tformatData.crcField)
    {
        ESTOP0;
    }

    PM_tformat_setupCommandReadoutOrReset(PM_TFORMAT_ID2,
                                         PM_TFORMAT_RX_CLOCKS_ID2,
                                         PM_TFORMAT_RX_FIELDS_ID2,
                                         PM_TFORMAT_TX_CLOCKS_ID2,
                                         PM_TFORMAT_FIFO_LEVEL_ID2);
    PM_tformat_startOperation();
    while(tformatData.dataReady != 1) ;//{;}
    PM_tformat_receiveDataID2();
    if(tformatData.crcCheck != tformatData.crcField)
    {
        ESTOP0;
    }

    DEVICE_DELAY_US(TFORMAT_IDLE_DELAY_US);
    PM_tformat_setupCommandReadoutOrReset(PM_TFORMAT_ID0,
                                         PM_TFORMAT_RX_CLOCKS_ID0,
                                         PM_TFORMAT_RX_FIELDS_ID0,
                                         PM_TFORMAT_TX_CLOCKS_ID0,
                                         PM_TFORMAT_FIFO_LEVEL_ID0);
    PM_tformat_startOperation();
    while(tformatData.dataReady != 1) {}
    PM_tformat_receiveDataID0();
    if(tformatData.crcCheck != tformatData.crcField)
    {
           ESTOP0;
    }

    DEVICE_DELAY_US(TFORMAT_IDLE_DELAY_US);
    PM_tformat_setupCommandReadoutOrReset(PM_TFORMAT_ID1,
                                         PM_TFORMAT_RX_CLOCKS_ID1,
                                         PM_TFORMAT_RX_FIELDS_ID1,
                                         PM_TFORMAT_TX_CLOCKS_ID1,
                                         PM_TFORMAT_FIFO_LEVEL_ID1);
    PM_tformat_startOperation();
    while(tformatData.dataReady != 1) {}
    PM_tformat_receiveDataID1();
    if(tformatData.crcCheck != tformatData.crcField)
    {
           ESTOP0;
    }

    DEVICE_DELAY_US(TFORMAT_IDLE_DELAY_US);
    address = 1;
    PM_tformat_setupCommandReadEEPROM(address);
    PM_tformat_startOperation();
    while(tformatData.dataReady != 1) {}
    retval1 = PM_tformat_receiveDataIDD();
    if(tformatData.crcCheck != tformatData.crcField)
    {
        ESTOP0;
    }

    DEVICE_DELAY_US(TFORMAT_EEPROM_BUSY_US);
    address = 1; data = 35;
    PM_tformat_setupCommandWriteEEPROM(address, data);
    PM_tformat_startOperation();
    while(tformatData.dataReady != 1) {}
    retval1 = PM_tformat_receiveDataID6();
    if(tformatData.crcCheck != tformatData.crcField)
    {
        ESTOP0;
    }

    DEVICE_DELAY_US(TFORMAT_EEPROM_BUSY_US);
    address = 1;
    PM_tformat_setupCommandReadEEPROM(address);
    PM_tformat_startOperation();
    while(tformatData.dataReady != 1) {}
    retval1 = PM_tformat_receiveDataIDD();
    if(tformatData.crcCheck != tformatData.crcField)
    {
       ESTOP0;
    }

    DEVICE_DELAY_US(TFORMAT_IDLE_DELAY_US);
    return(retval1);
    }

//
// End of file
//
