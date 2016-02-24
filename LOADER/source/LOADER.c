/**
 *  \file  LOADER.c
 *  \brief  MDR32F9Qx EEPROM LOADER.
 *  \author  Eduard Ivanov (the program is based on RAM-LOADER)
 *  \author  Vitaly Kravtsov (in4lio@gmail.com)
 *  \copyright  See the LICENSE file.
 */

#define VERSION  "0.3"

#include "MDR32Fx.h"
#include "MDR32F9Qx_rst_clk.h"
#include "SEGGER_RTT.h"

#define EEPROM_START    0x8000000
#define EEPROM_SIZE     0x20000

/* interface to JFlash script */
struct {

#define BLOCK_SIZE      0x4000
	/* incoming data buffer */
	uint32_t data[ BLOCK_SIZE / sizeof( uint32_t )];

	/* EEPROM address of writing */
	uint32_t addr;

	/* incoming data length */
	uint32_t len;

#define IDLE            (( uint32_t ) -1 )
#define ERASE           1
#define WRITE_BLOCK     2
	/* LOADER state */
	uint32_t state;

#define ERR_NONE        0
#define ERR_ADDR        1
#define ERR_ADDR_ALIGN  2
#define ERR_ADDR_END    3
	/* last error */
	uint32_t err;

} iface = { 0 };

void usleep( uint32_t val )
{
	val *= ( SystemCoreClock / 500000 );
	val = ( val + 4 ) / 8;
	while ( val-- );
}

void eeprom_erase( void )
{
	SEGGER_RTT_WriteString( 0, "eeprom_erase()\n" );

	__disable_irq();
	MDR_EEPROM->KEY = 0x8AAA5551;
	MDR_EEPROM->CMD |= EEPROM_CMD_CON;
	for ( int i = 0; i < 0x010; i += 4 ) {
		MDR_EEPROM->ADR = EEPROM_START + i;
		MDR_EEPROM->CMD |= EEPROM_CMD_MAS1 | EEPROM_CMD_XE | EEPROM_CMD_ERASE;
		usleep( 5 );
		MDR_EEPROM->CMD |= EEPROM_CMD_NVSTR;
		usleep( 40000 );  /* 40 ms */
		MDR_EEPROM->CMD &= ~EEPROM_CMD_ERASE;
		usleep( 100 );
		MDR_EEPROM->CMD &= ~( EEPROM_CMD_MAS1 | EEPROM_CMD_XE | EEPROM_CMD_NVSTR );
		usleep( 100 );
	}
	MDR_EEPROM->CMD &= EEPROM_CMD_DELAY_Msk;  /* cancel PROGRAM MODE */
	MDR_EEPROM->KEY = 0x00000000;
	usleep( 1 );
	__enable_irq();

	SEGGER_RTT_WriteString( 0, "ok\n" );
}

uint32_t eeprom_write_block( uint32_t addr, uint32_t *data, uint32_t len )
{
	SEGGER_RTT_printf( 0, "eeprom_write_block( 0x%x, 0x%x, 0x%x )\n", addr, data, len );

	if ( addr < EEPROM_START ) {
		SEGGER_RTT_printf( 0, "ERROR: Wrong EERPOM address (0x%x).\n", addr );

		return ERR_ADDR;
	}
	if (( addr & 3 ) != 0 ) {
		SEGGER_RTT_printf( 0, "ERROR: Wrong EERPOM address alignment (0x%x).\n", addr );

		return ERR_ADDR_ALIGN;
	}
	if ( addr + len * ( sizeof( uint32_t )) > EEPROM_START + EEPROM_SIZE ) {
		SEGGER_RTT_printf( 0, "ERROR: Wrong EERPOM address (0x%x).\n", addr + len * ( sizeof( uint32_t )));

		return ERR_ADDR_END;
	}
	__disable_irq();
	MDR_EEPROM->KEY = 0x8AAA5551;
	MDR_EEPROM->CMD |= EEPROM_CMD_CON;
	MDR_EEPROM->ADR = addr;
	while ( len-- ) {
		MDR_EEPROM->DI = *data;
		MDR_EEPROM->CMD |= EEPROM_CMD_XE | EEPROM_CMD_PROG;
		usleep( 5 );
		MDR_EEPROM->CMD |= EEPROM_CMD_NVSTR;
		usleep( 10 );
		MDR_EEPROM->CMD |= EEPROM_CMD_YE;  /* according data-sheet */
		usleep( 40 );
		MDR_EEPROM->CMD &= ~EEPROM_CMD_YE;
		usleep( 1 );
		MDR_EEPROM->CMD &= ~( EEPROM_CMD_XE | EEPROM_CMD_NVSTR );
		usleep( 1 );
		MDR_EEPROM->CMD &= ~EEPROM_CMD_PROG;
		usleep( 5 );
		MDR_EEPROM->ADR += 4;
		data += 1;
	}
	MDR_EEPROM->CMD &= EEPROM_CMD_DELAY_Msk;  /* cancel PROGRAM MODE */
	MDR_EEPROM->KEY = 0x00000000;
	usleep( 1 );
	__enable_irq();

	SEGGER_RTT_WriteString( 0, "ok\n" );

	return ERR_NONE;
}

int main( void )
{
	NVIC->ICER[ 0 ] = 0xFFFFFFFF;  /* disable all interrupts */
	NVIC->ICPR[ 0 ] = 0xFFFFFFFF;  /* reset all interrupts */

	MDR_RST_CLK->PER_CLOCK = RST_CLK_PCLK_PORTB | RST_CLK_PCLK_PORTD | RST_CLK_PCLK_BKP | RST_CLK_PCLK_RST_CLK
	| RST_CLK_PCLK_EEPROM;

	MDR_EEPROM->CMD = ( 3 << EEPROM_CMD_DELAY_Pos );  /* set EEPROM delay */

	SEGGER_RTT_ConfigUpBuffer( 0, 0, 0, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL );
	SEGGER_RTT_WriteString( 0, "\nMCU MDR32F9Qx EEPROM LOADER " VERSION "\n\n" );
	SEGGER_RTT_printf( 0, "data buffer at 0x%x\n", &iface.data );

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
