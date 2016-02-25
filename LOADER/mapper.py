from __future__ import division

r"""
mapper.py -- Configure JFlash according to LOADER layout
"""

import sys, os
import fileinput
import re
from shutil import copyfile

DIR_SCRIPT  = '../'
DIR_ARMCC   = './ARMCC-MDR32F9Qx/'
DIR_GCC     = './GCC-MDR32F9Qx/'

FILE_MAP    = 'LOADER.map'
FILE_BINARY = 'LOADER.bin'
FILE_SCRIPT = 'JFlash.py'

VARIABLES = (
    'LD_COMPILER',
    'LD_START',
    'LD_STACK',
    'LD_IFACE',
    'LD_IFACE_SZ',
    'LD_RTT'
)

CC_LIST = ( 'ARMCC', 'GCC' )

if ( len( sys.argv ) < 2 ) or ( sys.argv[ 1 ].upper() not in CC_LIST ):
    sys.exit( 'Usage: %s (%s|%s)' % ( os.path.basename(__file__), CC_LIST[ 0 ], CC_LIST[ 1 ]))

CC = CC_LIST.index( sys.argv[ 1 ].upper())

if CC == 0:
    DIR_BINARY = DIR_ARMCC
    RE_ADDR    = r"\s+%s\s+(0x[0-9a-fA-F]+)"
    RE_SIZE    = r"\s+%s\s+0x[0-9a-fA-F]+\s+Data\s+([0-9]+)"
else:
    DIR_BINARY = DIR_GCC
    RE_ADDR    = r"\s+(0x[0-9a-fA-F]+)\s+%s\s*"
    RE_SIZE    = r"%s\s+(0x[0-9a-fA-F]+)"

RE_LIST = (
    '',
    RE_ADDR % ( 'Reset_Handler' ),
    RE_ADDR % ( '__initial_sp' if CC == 0 else '__StackTop' ),
    RE_ADDR % ( 'iface' ),
    RE_SIZE % ( 'iface' ),
    RE_ADDR % ( '_SEGGER_RTT' )
)

RE_VAR = r'^(%s\s*=\s*)(0x[0-9a-fA-F]+|[0-9]+)(.*)'

os.chdir( os.path.dirname( os.path.realpath( __file__ )))
fn_binary = DIR_BINARY + FILE_BINARY
fn_map = DIR_BINARY + FILE_MAP
fn_script = DIR_SCRIPT + FILE_SCRIPT

sys.stderr.write( '\nRead "%s"\n' % fn_map )

with open( fn_map ) as f:
    MAP = f.readlines()

values = [ None ] * len( VARIABLES )
values[ 0 ] = str( CC )
re_list = [ re.compile( x ) for x in RE_LIST ]
for ln in MAP:
    for i, var in enumerate( VARIABLES ):
        if values[ i ] is None:
            m = re_list[ i ].match( ln )
            if m:
                values[ i ] = m.group( 1 )
                sys.stderr.write( ln )

error = False
for i, ex in enumerate( RE_LIST ):
    if not values[ i ]:
        sys.stderr.write( 'ERROR: "%s" not found! (%s)\n' % ( ex, fn_map ))
        error = True

if error:
    sys.exit( 1 )

sys.stderr.write( '\nConfigure "%s"\n' % fn_script )

ok = [ 0 ] * len( VARIABLES )
for ln in fileinput.FileInput( fn_script, inplace=1, backup='.bak' ):
    for i, var in enumerate( VARIABLES ):
        if not ok[ i ]:
            ln, ok[ i ] = re.subn( RE_VAR % var, r'\g<1>%s\g<3>' % values[ i ], ln, 1 )
            if ok[ i ]:
                sys.stderr.write( '    ' + ln )
                break
    sys.stdout.write( ln )

for i, var in enumerate( VARIABLES ):
    if not ok[ i ]:
        sys.stderr.write( 'WARNING: "%s" not found! (%s)\n' % ( var, fn_script ))

sys.stderr.write( '\nCopy "%s"\n' % fn_binary )

copyfile( fn_binary, FILE_BINARY )

sys.stderr.write( '\nDone\n' )
