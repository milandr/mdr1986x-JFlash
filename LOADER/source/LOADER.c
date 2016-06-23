/**
 *  \file  LOADER.c
 *  \brief  MDR32F9Qx EEPROM LOADER.
 *  \author  Eduard Ivanov (the program is based on RAM-LOADER)
 *  \author  Vitaly Kravtsov (in4lio@gmail.com)
 *  \copyright  See the LICENSE file.
 */

#define VERSION  "0.6"

#include "MDR32Fx.h"
#include "MDR32F9Qx_rst_clk.h"
#include "SEGGER_RTT.h"

typedef uint32_t EEPROM_WORD;

#define EEPROM_SIZE  ( 32 * EEPROM_PAGE_SIZE )

/* Interface to JFlash script */
struct {

#define BLOCK_SIZE      0x4000
	/* Incoming data buffer */
	EEPROM_WORD data[ BLOCK_SIZE / EEPROM_WORD_SIZE ];

	/* EEPROM address of writing */
	uint32_t addr;

	/* Incoming data length (in EEPROM_WORD) */
	uint32_t len;

#define START           0
#define ERASE           1
#define WRITE_BLOCK     2
#define IDLE            (( uint32_t ) -1 )
	/* LOADER running command or state */
	uint32_t state;

#define ERR_NONE        0
#define ERR_ADDR        1
#define ERR_ADDR_ALIGN  2
#define ERR_ADDR_END    3
	/* Last error */
	uint32_t err;

} iface;

#define __CRLF__  "\r\n"

void upause( uint32_t val )
{
	val *= ( SystemCoreClock / 500000 );
	val = ( val + 4 ) / 8;
	while ( val-- );
}

void eeprom_erase( void )
{
	SEGGER_RTT_WriteString( 0, "eeprom_erase()"__CRLF__ );

	__disable_irq();
	MDR_EEPROM->KEY  = EEPROM_KEY;
	MDR_EEPROM->CMD &= EEPROM_CMD_DELAY_Msk;

	MDR_EEPROM->CMD |= EEPROM_CMD_CON;
	for ( int i = 0; i < 0x010; i += 4 ) {
		MDR_EEPROM->ADR = i;
		MDR_EEPROM->CMD |= EEPROM_CMD_MAS1 | EEPROM_CMD_XE | EEPROM_CMD_ERASE;
		upause( 5 );
		MDR_EEPROM->CMD |= EEPROM_CMD_NVSTR;
		upause( 40000 );  /* 40 ms */
		MDR_EEPROM->CMD &= ~EEPROM_CMD_ERASE;
		upause( 100 );
		MDR_EEPROM->CMD &= ~( EEPROM_CMD_MAS1 | EEPROM_CMD_XE | EEPROM_CMD_NVSTR );
		upause( 5 );
	}
	MDR_EEPROM->CMD &= EEPROM_CMD_DELAY_Msk;
	MDR_EEPROM->KEY = 0;
	upause( 1 );
	__enable_irq();

	SEGGER_RTT_WriteString( 0, "ok"__CRLF__ );
}

uint32_t eeprom_write_block( uint32_t addr, EEPROM_WORD *data, uint32_t len )
{
	SEGGER_RTT_printf( 0, "eeprom_write_block( 0x%x, 0x%x, 0x%x )"__CRLF__, addr, data, len );

	if ( addr < EEPROM_ADDRESS ) {
		SEGGER_RTT_printf( 0, "ERROR: Wrong EERPOM address (0x%x)."__CRLF__, addr );

		return ERR_ADDR;
	}
	if (( addr & 3 ) != 0 ) {
		SEGGER_RTT_printf( 0, "ERROR: Wrong EERPOM address alignment (0x%x)."__CRLF__, addr );

		return ERR_ADDR_ALIGN;
	}
	if ( addr + len * EEPROM_WORD_SIZE > EEPROM_ADDRESS + EEPROM_SIZE ) {
		SEGGER_RTT_printf( 0, "ERROR: Wrong EERPOM address (0x%x)."__CRLF__, addr + len * EEPROM_WORD_SIZE );

		return ERR_ADDR_END;
	}
	__disable_irq();
	MDR_EEPROM->KEY  = EEPROM_KEY;
	MDR_EEPROM->CMD &= EEPROM_CMD_DELAY_Msk;

	MDR_EEPROM->CMD |= EEPROM_CMD_CON;
	MDR_EEPROM->ADR = addr;
	while ( len-- ) {
		MDR_EEPROM->DI = *data;
		MDR_EEPROM->CMD |= EEPROM_CMD_XE | EEPROM_CMD_PROG;
		upause( 5 );
		MDR_EEPROM->CMD |= EEPROM_CMD_NVSTR;
		upause( 10 );
		MDR_EEPROM->CMD |= EEPROM_CMD_YE;
		upause( 40 );
		MDR_EEPROM->CMD &= ~EEPROM_CMD_YE;
		upause( 1 );
		MDR_EEPROM->CMD &= ~EEPROM_CMD_PROG;
		upause( 5 );
		MDR_EEPROM->CMD &= ~( EEPROM_CMD_XE | EEPROM_CMD_NVSTR );
		upause( 1 );
		MDR_EEPROM->ADR += EEPROM_WORD_SIZE;
		data += 1;
	}
	MDR_EEPROM->CMD &= EEPROM_CMD_DELAY_Msk;
	MDR_EEPROM->KEY = 0;
	upause( 1 );
	__enable_irq();

	SEGGER_RTT_WriteString( 0, "ok"__CRLF__ );

	return ERR_NONE;
}

int main( void )
{
	NVIC->ICER[ 0 ] = 0xFFFFFFFF;  /* Disable all interrupts */
	NVIC->ICPR[ 0 ] = 0xFFFFFFFF;  /* Reset all interrupts */

	MDR_RST_CLK->PER_CLOCK = RST_CLK_PCLK_PORTB | RST_CLK_PCLK_PORTD | RST_CLK_PCLK_BKP | RST_CLK_PCLK_RST_CLK
	| RST_CLK_PCLK_EEPROM;

	MDR_EEPROM->CMD = ( 3 << EEPROM_CMD_DELAY_Pos );  /* Set EEPROM delay */

	SEGGER_RTT_ConfigUpBuffer( 0, 0, 0, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL );
	SEGGER_RTT_WriteString( 0, __CRLF__"MCU MDR32F9Qx EEPROM LOADER " VERSION __CRLF__ __CRLF__ );
	SEGGER_RTT_printf( 0, "data buffer at 0x%x"__CRLF__, &iface.data );

	iface.err = ERR_NONE;
	iface.state = IDLE;

	while ( 1 ) {
		switch ( iface.state ) {

		/* EEPROM erasing */
		case ERASE:
			eeprom_erase();
			iface.err = ERR_NONE;
			iface.state = IDLE;
			break;

		/* EEPROM writing */
		case WRITE_BLOCK:
			iface.err = eeprom_write_block( iface.addr, iface.data, iface.len );
			iface.state = IDLE;
			break;
		}
	}
}
