/* Copyright (C) 2017 ETH Zurich, University of Bologna
 * All rights reserved.
 *
 * This code is under development and not yet released to the public.
 * Until it is released, the code is under the copyright of ETH Zurich and
 * the University of Bologna, and may contain confidential and/or unpublished 
 * work. Any reuse/redistribution is strictly forbidden without written
 * permission from ETH Zurich.
 *
 * Bug fixes and contributions will eventually be released under the
 * SolderPad open hardware license in the context of the PULP platform
 * (http://www.pulp-platform.org), under the copyright of ETH Zurich and the
 * University of Bologna.
 */
#ifndef ZYNQ_H___
#define ZYNQ_H___

/*
 * Board selection
 */
#define ZEDBOARD 1
#define ZC706    2
#define MINI_ITX 3

#ifndef PLATFORM
  #error "Define PLATFORM!"
#endif

// DRAM
#if   PLATFORM == ZEDBOARD
  #define DRAM_SIZE_MB 512
#elif PLATFORM == ZC706 || PLATFORM == MINI_ITX
  #define DRAM_SIZE_MB 1024
#endif

// System-Level Control Register
#define SLCR_BASE_ADDR              0xF8000000
#define SLCR_SIZE_B                 0x1000
#define SLCR_FPGA_RST_CTRL_OFFSET_B 0x240
#define SLCR_FPGA_OUT_RST           1
#define SLCR_FPGA0_THR_CNT_OFFSET_B 0x178
#define SLCR_FPGA0_THR_STA_OFFSET_B 0x17C

// UART0 Controller - PULP -> Host UART
#define HOST_UART_BASE_ADDR      0xE0000000
#define HOST_UART_SIZE_B         0x1000
#define MODEM_CTRL_REG0_OFFSET_B 0x24

#endif // ZYNQ_H___
