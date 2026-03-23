//
// Created by khalaf on 3/23/26.
//

#ifndef BIT_MATH_H_
#define BIT_MATH_H_

#include "STD_TYPES.h"

/* A2: Single Bit Operations */

/* Set a specific bit to 1 */
#define SET_BIT(REG, BIT)          ((REG) |=  (((uint32)1) << (BIT)))

/* Clear a specific bit to 0 */
#define CLR_BIT(REG, BIT)          ((REG) &= ~(((uint32)1) << (BIT)))

/* Toggle (flip) a specific bit */
#define TOG_BIT(REG, BIT)          ((REG) ^=  (((uint32)1) << (BIT)))

/* Get the value of a specific bit (returns 0U or 1U) */
#define GET_BIT(REG, BIT)          (((REG) >> (BIT)) & ((uint32)1))




/* A2: Basic Multi-Bit Mask Operations */


/* Set multiple bits using a raw mask directly */
#define SET_MASK(REG, MSK)         ((REG) |=  ((uint32)(MSK)))

/* Clear multiple bits using a raw mask directly */
#define CLR_MASK(REG, MSK)         ((REG) &= ~((uint32)(MSK)))




/* A2: Slot-Based Multi-Bit Operations */

/* Set multiple bits within a specific slot index */
#define SET_BITS_MSK(REG, MSK, IDX, SIZE) \
((REG) |= (((uint32)(MSK)) << ((IDX) * (SIZE))))

/* Clear multiple bits within a specific slot index */
#define CLR_BITS_MSK(REG, MSK, IDX, SIZE) \
((REG) &= ~(((uint32)(MSK)) << ((IDX) * (SIZE))))



/* A2: Bit-Field Operations */

/* Write a protected value into a bit-field slot (Clear then Set in one cycle) */
#define WRITE_BIT_FIELD(REG, MSK, IDX, SIZE, DATA) \
((REG) = ((REG) & ~(((uint32)(MSK)) << ((IDX) * (SIZE)))) | ((((uint32)((DATA) & (MSK)))) << ((IDX) * (SIZE))))

/* Read/extract a value from a bit-field slot (Mask first, then shift down) */
#define READ_BIT_FIELD(REG, MSK, IDX, SIZE) \
(((REG) & (((uint32)(MSK)) << ((IDX) * (SIZE)))) >> ((IDX) * (SIZE)))


#endif /* BIT_MATH_H_ */