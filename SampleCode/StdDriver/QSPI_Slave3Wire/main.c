/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief    Configure QSPI0 as Slave 3-wire mode and demonstrate how to
 *           communicate with an off-chip SPI Master device with FIFO mode.
 *           This sample code needs to work with SPI_MasterFIFOMode sample code.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

#define TEST_COUNT 16

static uint32_t s_au32SourceData[TEST_COUNT];
static uint32_t s_au32DestinationData[TEST_COUNT];

void SYS_Init(void);
void QSPI_Init(void);

void SYS_Init(void)
{
    /* Set PF multi-function pins for XT1_OUT(PF.2) and XT1_IN(PF.3) */
    SET_XT1_OUT_PF2();
    SET_XT1_IN_PF3();

    /* Set PF multi-function pins for X32_OUT(PF.4) and X32_IN(PF.5) */
    SET_X32_OUT_PF4();
    SET_X32_IN_PF5();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable HIRC, HXT and LXT clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_LXTEN_Msk);

    /* Wait for HIRC, HXT and LXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_HXTSTB_Msk | CLK_STATUS_LXTSTB_Msk);

    /* Set PCLK0 and PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Set core clock to 200MHz */
    CLK_SetCoreClock(FREQ_200MHZ);

    /* Enable all GPIO clock */
    CLK->AHBCLK0 |= CLK_AHBCLK0_GPACKEN_Msk | CLK_AHBCLK0_GPBCKEN_Msk | CLK_AHBCLK0_GPCCKEN_Msk | CLK_AHBCLK0_GPDCKEN_Msk |
                    CLK_AHBCLK0_GPECKEN_Msk | CLK_AHBCLK0_GPFCKEN_Msk | CLK_AHBCLK0_GPGCKEN_Msk | CLK_AHBCLK0_GPHCKEN_Msk;
    CLK->AHBCLK1 |= CLK_AHBCLK1_GPICKEN_Msk | CLK_AHBCLK1_GPJCKEN_Msk;

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART0 module clock source as HIRC and UART0 module clock divider as 1 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Enable QSPI0 module clock */
    CLK_EnableModuleClock(QSPI0_MODULE);

    /* Select QSPI0 module clock source as PCLK0 */
    CLK_SetModuleClock(QSPI0_MODULE, CLK_CLKSEL2_QSPI0SEL_PCLK0, MODULE_NoMsk);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set multi-function pins for UART0 RXD and TXD */
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();

    /* Setup QSPI0 multi-function pins */
    SET_QSPI0_MOSI0_PA0();
    SET_QSPI0_MISO0_PA1();
    SET_QSPI0_CLK_PA2();

    /* Enable QSPI0 clock pin (PA2) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
}

void QSPI_Init(void)
{
    /* Enable slave 3-wire mode before enabling QSPI controller */
    QSPI_ENABLE_3WIRE_MODE(QSPI0);

    /* Configure as a slave, clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    /* Configure QSPI0 as a low level active device. */
    QSPI0->CTL = QSPI_SLAVE | QSPI_MODE_0 | QSPI_CTL_SPIEN_Msk;
}

int main(void)
{
    volatile uint32_t u32TxDataCount, u32RxDataCount;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O. */
    SYS_Init();

    /* Configure UART0: 115200, 8-bit word, no parity bit, 1 stop bit. */
    UART_Open(UART0, 115200);

    /* Init QSPI */
    QSPI_Init();

    printf("\n\n");
    printf("+-----------------------------------------------------------------------+\n");
    printf("|                  QSPI0 Slave 3-Wire Mode Sample Code                  |\n");
    printf("+-----------------------------------------------------------------------+\n");
    printf("\n");
    printf("Configure QSPI0 as a slave.\n");
    printf("Bit length of a transaction: 32\n");
    printf("The I/O connection for QSPI0:\n");
    printf("    QSPI0_CLK(PA2)\n    QSPI0_MISO0(PA1)\n    QSPI0_MOSI0(PA0)\n\n");
    printf("QSPI controller will enable FIFO mode and transfer %d data to an off-chip master device.\n", TEST_COUNT);
    printf("In the meanwhile the QSPI controller will receive %d data from the off-chip master device.\n", TEST_COUNT);
    printf("After the transfer is done, the %d received data will be printed out.\n", TEST_COUNT);

    for(u32TxDataCount = 0; u32TxDataCount < TEST_COUNT; u32TxDataCount++)
    {
        /* Write the initial value to source buffer */
        s_au32SourceData[u32TxDataCount] = 0x00AA0000 + u32TxDataCount;
        /* Clear destination buffer */
        s_au32DestinationData[u32TxDataCount] = 0;
    }

    u32TxDataCount = 0;
    u32RxDataCount = 0;
    printf("Press any key if the master device configuration is ready.\n");
    getchar();
    printf("\n");

    /* Set TX FIFO threshold and enable FIFO mode. */
    QSPI_SetFIFO(QSPI0, 4, 4);

    /* Access TX and RX FIFO */
    while(u32RxDataCount < TEST_COUNT)
    {
        /* Check TX FULL flag and TX data count */
        if((QSPI_GET_TX_FIFO_FULL_FLAG(QSPI0) == 0) && (u32TxDataCount < TEST_COUNT))
            QSPI_WRITE_TX(QSPI0, s_au32SourceData[u32TxDataCount++]); /* Write to TX FIFO */
        /* Check RX EMPTY flag */
        if(QSPI_GET_RX_FIFO_EMPTY_FLAG(QSPI0) == 0)
            s_au32DestinationData[u32RxDataCount++] = QSPI_READ_RX(QSPI0); /* Read RX FIFO */
    }

    /* Print the received data */
    printf("Received data:\n");
    for(u32RxDataCount = 0; u32RxDataCount < TEST_COUNT; u32RxDataCount++)
    {
        printf("%d:\t0x%X\n", u32RxDataCount, s_au32DestinationData[u32RxDataCount]);
    }
    printf("The data transfer was done.\n");

    printf("\n\nExit QSPI driver sample code.\n");

    /* Reset QSPI0 */
    QSPI_Close(QSPI0);

    while(1);
}
