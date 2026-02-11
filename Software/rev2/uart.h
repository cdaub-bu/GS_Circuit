#include <stdint.h>

void uart_tx_init(void);
void uart_tx_byte(uint8_t data);
void uart_tx_string(const char *s);
