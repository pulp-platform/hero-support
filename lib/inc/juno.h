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
