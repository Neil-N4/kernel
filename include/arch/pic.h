#ifndef ARCH_PIC_H
#define ARCH_PIC_H

#include <stdint.h>

#define PIC_MASTER_OFFSET 32u
#define PIC_SLAVE_OFFSET  40u

void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

#endif
