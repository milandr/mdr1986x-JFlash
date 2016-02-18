from __future__ import division

r"""
JFlash.py -- MCU MDR32F9Qx internal flash programming via J-Link
http://github.com/in4lio/mdr1986x-JFlash/

Copyright (c) 2016 Vitaly Kravtsov (in4lio@gmail.com)
See the LICENSE file.
"""

APP             = 'JFlash'
VERSION         = '0.1b1'

#  J-Link GDB Server
HOST            = 'localhost'
PORT            = 2331

LOADER          = 'LOADER/LOADER.bin'
DUMP            = 'dump.bin'

#  LOADER layout (according to MAP file)
#  NOTE: Use 'mapper.py' if you need to update the following definitions.
LD_START        = 0x2000015D
LD_STACK        = 0x20006880
LD_IFACE        = 0x20002004
LD_IFACE_SZ     = 16400
LD_RTT          = 0x20006424

LD_FIELD_SZ     = 4
LD_DATA         = LD_IFACE
LD_DATA_SZ      = LD_IFACE_SZ - LD_FIELD_SZ * 4
LD_ADDR         = LD_DATA  + LD_DATA_SZ
LD_LEN          = LD_ADDR  + LD_FIELD_SZ
LD_STATE        = LD_LEN   + LD_FIELD_SZ
LD_ERR          = LD_STATE + LD_FIELD_SZ

#  LOADER state
IDLE            = 0
ERASE           = 1
WRITE_BLOCK     = 2

#  LOADER error
ERR_NONE        = 0
ERR_ADDR        = 1
ERR_ADDR_ALIGN  = 2
ERR_ADDR_END    = 3

#  MCU memory layout
RAM_START       = 0x20000000
EEPROM_START    = 0x08000000

import gdb
import sys
import os
from time import sleep
import filecmp

#  Message prefix
JF = '(%s):' % APP

#  MCU registers
#  R0 .. R15
for x in range( 0, 16 ):
    #  I know, it's a really bad idea, but...
    globals()[ 'R%d' % x ] = 'r%d' % x

PC  = 'pc'
LR  = 'lr'
MSP = 'MSP'
PSP = 'PSP'

#  Execute GDB command
def execute( st ):
    return gdb.execute( st, to_string=True )

#  Execute GDB 'monitor' command
def monitor( st ):
    return execute( 'monitor ' + st )

#  Read DWORD for memory
def mem32( addr ):
    return long( execute( 'x ' + str( addr )).split( ':' )[ 1 ].strip(), 16 )

#  Write DWORD to memory
def set_mem32( addr, val ):
    return execute( 'set {int}%d = %d' % ( addr, val ))

#  Read register
def reg( r ):
    return long( gdb.parse_and_eval( '$' + r ))

#  Write register
def set_reg( r, val ):
    return execute( 'set $%s = %d' % ( r, val ))

#  Upload binary data form file to memory
def load_binary( fn, offset, start=None, end=None ):
    st = 'restore %s binary %d' % ( fn, offset )
    if start is not None:
        st += ' %d' % start
        if end is not None:
            st += ' %d' % end
    return execute( st )

#  Save data form memory to dump file
def dump_binary( fn, offset, l ):
    return execute( 'dump binary memory %s %d %d' % ( fn, offset, offset + l ))

#  Cancel script
def quit( status=0 ):
    print
    print JF, 'Bye!'
    sys.exit( status )

#  Directory of script
SCRIPT_DIR = os.path.dirname( os.path.realpath( __file__ ))


#  THE SCRIPT START

print
print JF, 'MCU MDR32F9Qx %s %s' % ( APP, VERSION )
print

#  Check that binary file is specified
if 'binary' not in globals() or not binary:
    print JF, """Usage: gdb --batch -ex "py binary = '<FILE>'" -x %s.py""" % APP
    quit( 2 )

if not os.path.exists( binary ):
    print JF, 'ERROR: Binary file not found (%s).' % binary
    quit( 2 )

binary_sz = os.path.getsize( binary )

print JF, 'Binary file: %s' % binary
print JF, 'Size: %d' % binary_sz
print JF, 'MCU data buffer at %#X' % LD_DATA
print

execute( 'set pagination off' )

print JF, 'J-Link GDB Server connecting...'
try:
    execute( 'target remote %s:%d' % ( HOST, PORT ))

except Exception as e:
    print JF, 'ERROR: Fail to connect.'
    print e.message
    print
    print JF, 'Please start J-Link GDB Server first.'
    quit( 1 )

print JF, 'Hello!'

print monitor( 'reset 0' )
monitor( 'halt' )

print JF, 'LOADER uploading...'
print load_binary( os.path.join( SCRIPT_DIR, LOADER ), RAM_START )
set_reg( MSP, LD_STACK )
set_reg( PC , LD_START & ~1 )
set_mem32( 0xE000E008, 0x20000000 )

#  ERASE EEPROM

print JF, 'EEPROM erasing...'
monitor( 'go' )

#  Check erasing is started
sleep( 0.1 )
if mem32( LD_STATE ) != ERASE:
    print JF, 'ERROR: LOADER is not running.'
    quit( 1 )

#  Wait for ending
print JF,
while mem32( LD_STATE ) == ERASE:
    print '>',
    sleep( 0.2 )
print
print

monitor( 'halt' )

#  Check very first DWORD
if mem32( EEPROM_START ) != 0xFFFFFFFF:
    print JF, 'ERROR: EEPROM is not empty.'
    quit( 1 )

#  WRITING CYCLE

rest = binary_sz
block = 0
start = 0
while ( rest ):
    if rest > LD_DATA_SZ:
        sz = LD_DATA_SZ
        rest -= LD_DATA_SZ
    else:
        sz = rest
        rest = 0

    block += 1
    print JF, 'BLOCK %d writing...' % block
    print load_binary( binary, LD_DATA - start, start, start + sz )

    set_mem32( LD_ADDR, EEPROM_START + start )
    set_mem32( LD_LEN, ( sz + 3 ) // 4 )  # size in DWORDs
    set_mem32( LD_STATE, WRITE_BLOCK )

    monitor( 'go' )

    sleep( 0.1 )
    #  Wait for ending
    while mem32( LD_STATE ) == WRITE_BLOCK:
        sleep( 0.2 )

    monitor( 'halt' )

    #  Check error
    if mem32( LD_ERR ) != ERR_NONE:
        print JF, 'ERROR: Fail to write data (E%d).' % mem32( LD_ERR )
        quit( 1 )

    start += sz

#  VERIFY

print JF, 'EEPROM verification...'
dump = os.path.join( SCRIPT_DIR, DUMP )
dump_binary( dump, EEPROM_START, binary_sz )

#  Compare binary file with dump
if not filecmp.cmp( binary, dump ):
    print JF, 'ERROR: Binary file does NOT match EEPROM content.'
    quit( 1 )

print
print JF, '######## SUCCESS! ########'
print

monitor( 'go' )
print monitor( 'reset 0' )

print JF, 'Bye!'

#  THE END
