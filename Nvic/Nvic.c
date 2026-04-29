    //
    // Created by Khalaf on 29/04/2026.
    //
    #include "Nvic_private.h"
    #include "Nvic.h"


    void Nvic_EnableInterrupt(uint8 IRQn) {
        if (IRQn <= 239) {
            /* Write 1 to the specific bit in the correct ISER register */
            NVIC->ISER[IRQn / 32] |= (1UL << (IRQn % 32));
        }
    }

    void Nvic_DisableInterrupt(uint8 IRQn) {
        if (IRQn <= 239) {
            /* Write 1 to the specific bit in the correct ICER register */
            NVIC->ICER[IRQn / 32] |= (1UL << (IRQn % 32));
        }
    }