/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "protocol.nail.h"

/**********************
 *      DEFINES
 *********************/
#define START_IDX           0
#define CMD_IDX             1
#define STATE_IDX           2
#define SIZE_IDX            3
#define DATA_IDX            4

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *       CONSTS
 **********************/

/**********************
 *     GLOBALS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
*      EXTERNS
**********************/

/**********************
 *    PROTOTYPES
 **********************/

static uint16_t calc_checksum( uint8_t * data );
static uint16_t read_checksum( uint8_t * data );

int checksum_generate(NailArena *tmp_arena, NailOutStream *str_current, uint16_t *checksum);
int checksum_parse( NailArena *tmp, NailStream *current, uint16_t *checksum );


int checksum_generate(NailArena *tmp_arena, NailOutStream *str_current, uint16_t *checksum)
{
    NailStream    * stream_in;

    // Rename I/O Streams
    stream_in = str_current;

    // Calculate Checksum from stream_in
    *checksum = calc_checksum( stream_in->data );

    return 0;
}

int checksum_parse( NailArena *tmp, NailStream *current, uint16_t *checksum )
{
    NailStream    * stream_in;
    bool            success;

    // Rename I/O Streams
    stream_in = current;

    // Calculate Checksum Using Current Data
    uint16_t cl_checksum = calc_checksum( stream_in->data );

    // Get Checksum From Current Data
    uint16_t rx_checksum = read_checksum(stream_in->data);

    // Compare Checksums
    success = ( cl_checksum == rx_checksum );

    if( success ) {
        *checksum = cl_checksum;
    }

    return success ? 0 : -1;
}


static uint16_t calc_checksum( uint8_t * data )
{
    uint16_t checksum = 0xFFFF;
    uint8_t length = data[SIZE_IDX];

    // Subtract Command/Status
    checksum -= data[2];

    // Subtract Length
    checksum -= length;

    // Subtract Data
    for( int i = 0; i < length; i++ ) {
        checksum -= data[DATA_IDX + i];
    }

    checksum += 1;

    return checksum;
}



static uint16_t read_checksum( uint8_t * data )
{
    uint16_t checksum;
    uint8_t length = data[SIZE_IDX];

    checksum = data[DATA_IDX + length] << 8 | data[DATA_IDX + length + 1];

    return checksum;
}
