#include <stdint.h>

void uart_tx_init(void);
void uart_tx_byte(uint8_t data);
void uart_tx_hex2( uint8_t c);
void uart_tx_hex4( uint16_t i);
void uart_tx_crlf();
