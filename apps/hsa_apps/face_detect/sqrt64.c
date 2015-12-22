/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: sqrt64.c.rca 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: sqrt64.c.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#include <stdint.h>

#define FAST_SQRT64 1 // pour avoir les memes resultats que RAID

#if FAST_SQRT64
#define EXTU64(x, n) (((unsigned long long)(x)<<(64-(n)))>>(64-(n)))
#define EXTU32(x, n) (((unsigned int)(x)<<(32-(n)))>>(32-(n)))
/*
 * Leading zero function
*/
int lzc(int s)
{
    unsigned i = 32-1, lzcnt = 0;
    unsigned long long t0 = EXTU64(s, 32);
    while (i+1 && !((t0>>i)&1))
    {
		lzcnt++, i--;
    }
    return EXTU32(lzcnt, 32);
}

//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Approximation of the square root of the value
//!
//! \param val64        @ref uint64_t Pointer to input value
//! \return             @ref uint32_t Square root of inout
 //-----------------------------------------------------------------------------
uint32_t sqrt64(const uint64_t * val64) {
    register int32_t log2out;
    const int32_t * val32_low;
    const int32_t * val32_high;

    val32_low = (const int32_t *)val64;
    val32_high = val32_low + 1;

    log2out = lzc(*val32_high);
    if ((log2out = (32 - log2out))) {
        log2out = (log2out + 32) >> 1;
        uint32_t temp = ((((uint32_t)(*val64 >> log2out)) + (1 << log2out)) >> 1);
        return temp;
    } else {
        log2out = lzc(*val32_low);
        log2out = (32 - log2out) >> 1;
        uint32_t temp = (((*val32_low >> log2out) + (1 << log2out)) >> 1);
        return temp;
    }
}

#else //FAST_SQRT64

uint32_t sqrt64(const uint64_t * val) {
    uint32_t out, old, i;
    uint64_t tmp;

    out = 1;
    tmp = *val << 1;
    while (tmp >>= 2) {
        out <<= 1;
    }
    old = 0;
    for (i = 7; out != old && i; i--) {
        old = out;
        out = ((uint32_t)(*val / out) + out + 1) >> 1;
    }

    return(out);
}

#endif //FAST_SQRT64

