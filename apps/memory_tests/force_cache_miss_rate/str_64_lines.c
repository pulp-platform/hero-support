asm volatile ("ldr r0, [%0]" : : "r" (&address) : "r4","r5","r6","r7","r8","r9","r10","r11");
/*
 * 64 lines - increment address by 32 byte (one cache line) after the access
 */
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
asm volatile ("str %0, [r0], +#32" : : "r" (value) );
// \_64
asm volatile ("str r0, [%0]" :  : "r" (&address) );