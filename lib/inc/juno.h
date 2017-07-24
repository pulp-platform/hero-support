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
#ifndef JUNO_H___
#define JUNO_H___

#define ARM_CLK_FREQ_MHZ 1100 // A57 overdrive
#define DRAM_SIZE_MB 8192

#ifndef CONFIG_COMPAT
#define CONFIG_COMPAT
#endif

// io{read,write}64 macros - not defined in the kernel sources
#define ioread64(p)    ({ u64 __v = le64_to_cpu((__force __le64)__raw_readq(p)); __iormb(); __v; })
#define iowrite64(v,p) ({ __iowmb(); __raw_writeq((__force u64)cpu_to_le64(v), p); })

#endif // JUNO_H___
