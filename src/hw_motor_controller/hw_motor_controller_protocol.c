/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>

#include "protocol.nail.h"

/**********************
 *      DEFINES
 *********************/
#define CMD_IDX             0
#define SIZE_IDX            1

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
static uint8_t calc_checksum( const uint8_t * data, size_t data_sz );
static uint8_t read_checksum( const uint8_t * data, size_t data_sz );

int motor_controller_checksum_generate(NailArena *tmp_arena, NailOutStream *str_current, uint8_t *checksum)
{
    NailOutStream    * stream_in;

    // Rename I/O Streams
    stream_in = str_current;

    // Calculate Checksum from stream_in
    *checksum = calc_checksum( stream_in->data, 1 + stream_in->data[SIZE_IDX] );

    return 0;
}


int motor_controller_checksum_parse( NailArena *tmp, NailStream *current, uint8_t *checksum )
{
//    NailStream    * stream_in;
    bool            success;

//    // Rename I/O Streams
//    stream_in = current;
//
//    // Calculate Checksum Using Current Data
//    uint16_t cl_checksum = calc_checksum( stream_in->data, stream_in->size );
//
//    // Get Checksum From Current Data
//    uint16_t rx_checksum = read_checksum(stream_in->data);
//
//    // Compare Checksums
//    success = ( cl_checksum == rx_checksum );
//
//    if( success ) {
//        *checksum = cl_checksum;
//    }

    success = true;

    return success ? 0 : -1;
}

static uint8_t calc_checksum( const uint8_t * data, size_t data_sz )
{
    uint8_t checksum = 0;

    // Calculate Checksum
    for( int idx = 0; idx < data_sz; idx++ ) {
        checksum += data[idx];
    }

    return checksum;
}

static uint8_t read_checksum( const uint8_t * data, size_t data_sz )
{
//    bool            success;
//    header_t      * header;
//    uint16_t        checksum_ofst;
//
//    // Is Input Valid?
//    success = ( NULL != data );
//
//    // Get Header
//    if( success ) {
//        header = (header_t *)data;
//    }
//
//    // Get Checksum Offset
//    if( success ) {
//        checksum_ofst = sizeof(header_t) + header->payload_sz;
//        success = ( checksum_ofst < data_sz );
//    }
//
//    // Update Checksum
//    if( success ) {
//        ((uint8_t *)data)[checksum_ofst] = calc_checksum( (uint8_t *)data, checksum_ofst );
//    }

return 0;
}