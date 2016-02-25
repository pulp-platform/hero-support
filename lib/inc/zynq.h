#ifndef ZYNQ_H___
#define ZYNQ_H___

/*
 * Board selection
 */
#define ZEDBOARD 1
#define ZC706 2
#define MINI_ITX 3

//#define BOARD ZC706
#ifndef BOARD
#define BOARD MINI_ITX
#endif // BOARD

/*
 * Board specific settings
 */
#if BOARD == ZEDBOARD

#define ARM_CLK_FREQ_MHZ 667
//#define ARM_CLK_FREQ_MHZ 300
#define DRAM_SIZE_MB 512

#elif BOARD == ZC706 || BOARD == MINI_ITX

#define ARM_CLK_FREQ_MHZ 667
#define DRAM_SIZE_MB 1024
 
#endif // BOARD

/*
 * Independent parameters
 */
#define ZYNQ_PMM_N_DEV_NUMBERS 1
#define N_ARM_CORES 2
#define ZYNQ_PMM_PROC_N_CHARS_MAX 1000

// System-Level Control Register - for FPGA reset
#define SLCR_BASE_ADDR              0xF8000000
#define SLCR_SIZE_B                 0x1000
#define SLCR_FPGA_RST_CTRL_OFFSET_B 0x240
#define SLCR_FPGA_OUT_RST           1
#define SLCR_FPGA0_THR_CNT_OFFSET_B 0x178
#define SLCR_FPGA0_THR_STA_OFFSET_B 0x17C

// Performance Monitor Unit
// ARM Architecture Reference Manual ARMv7, Appendix D3.1
#define ARM_PMU_PMXEVCNTR1_EV 0x3 // event: L1 data refill/miss
#define ARM_PMU_PMXEVCNTR0_EV 0x4 // event: L1 data access
//#define ARM_PMU_PMXEVCNTR1_EV 0x5 // event: data micro TLB refill/miss
// ARM Architecture Reference Manual ARMv7, Appendix D3.1 -- not implemented in Cortex-A9
//#define ARM_PMU_PMXEVCNTR1_EV 0x42 // event: L1 data read refill
//#define ARM_PMU_PMXEVCNTR0_EV 0x40 // event: L1 data read access
//#define ARM_PMU_PMXEVCNTR1_EV 0x43 // event: L1 data write refill
//#define ARM_PMU_PMXEVCNTR0_EV 0x41 // event: L1 data write access
#define ARM_PMU_CLK_DIV 64 // clock divider for clock cycle counter

// MPCore - SCU, Interrupt Controller, Counters and Timers
#define MPCORE_BASE_ADDR         0xF8F00000
#define MPCORE_SIZE_B            0x2000
#define MPCORE_ICDICFR3_OFFSET_B 0x1C0C
#define MPCORE_ICDICFR4_OFFSET_B 0x1C10

// Level 2 Cache Controller L2C-310
#define L2C_310_BASE_ADDR 0xF8F02000
#define L2C_310_SIZE_B    0x1000

#define L2C_310_REG2_EV_COUNTER_CTRL_OFFSET_B 0x200 // event counter control register
#define L2C_310_REG2_EV_COUNTERS_DISABLE      0x0
#define L2C_310_REG2_EV_COUNTERS_ENABLE       0x1
#define L2C_310_REG2_EV_COUNTERS_RESET        0x7

#define L2C_310_REG2_EV_COUNTER1_CFG_OFFSET_B 0x204 // event counter 1 configuration register
#define L2C_310_REG2_EV_COUNTER0_CFG_OFFSET_B 0x208 // event counter 0 configuration register
// CoreLink L2C-310 Technical Reference Manual, Sections 2.5.8 & 3.3.7
#define L2C_310_REG2_EV_COUNTER1_EV_RH        0x2   // event: L2 data read hit
#define L2C_310_REG2_EV_COUNTER0_EV_RR        0x3   // event: L2 data read request
#define L2C_310_REG2_EV_COUNTER1_EV_WH        0x4   // event: L2 data write hit
#define L2C_310_REG2_EV_COUNTER0_EV_WR        0x5   // event: L2 data write request

#define L2C_310_REG2_EV_COUNTER1_OFFSET_B     0x20C // event counter 1 value register offset
#define L2C_310_REG2_EV_COUNTER0_OFFSET_B     0x210 // event counter 0 value register offset

//#define L2C_310_REG1_TAG_RAM_CONTROL_OFFSET_B  0x0108 // tag RAM latency control register 
//#define L2C_310_REG1_DATA_RAM_CONTROL_OFFSET_B 0x010c // data RAM latency control register
//#define L2C_310_TAG_RAM_LAT                    0x1    // tag RAM latency  
//#define L2C_310_DATA_RAM_WRITE_LAT             0x1    // data RAM write latency
//#define L2C_310_DATA_RAM_READ_LAT              0x2    // data RAM read latency

/*
 * Dependent parameters
 */
// Level 2 Cache Controller L2C-310
//#define L2C_310_TAG_RAM_LAT_VALUE  ((L2C_310_TAG_RAM_LAT << 8) | (L2C_310_TAG_RAM_LAT << 4) | L2C_310_TAG_RAM_LAT)
//#define L2C_310_DATA_RAM_LAT_VALUE ((L2C_310_DATA_RAM_WRITE_LAT << 8) | (L2C_310_DATA_RAM_READ_LAT << 4) | L2C_310_DATA_RAM_WRITE_LAT)

#endif // ZYNQ_H___
