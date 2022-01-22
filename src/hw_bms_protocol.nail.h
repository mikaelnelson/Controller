#include <stdint.h>
 #include <string.h>
#include <assert.h>
enum N_types {_NAIL_NULL};
typedef struct cmd_resp_t cmd_resp_t;
typedef struct protection_status_t protection_status_t;
typedef struct fet_control_status_t fet_control_status_t;
typedef struct basic_status_resp_t basic_status_resp_t;
typedef struct cell_voltage_resp_t cell_voltage_resp_t;
struct cmd_resp_t{
uint8_t command;
uint8_t state;
struct {
uint8_t*elem;
 size_t count;
} data;
uint16_t checksum;
}
;
struct protection_status_t{
uint8_t mosfet_software_lock;
uint8_t ic_frontend_error;
uint8_t short_circuit;
uint8_t discharge_overcurrent;
uint8_t charge_overcurrent;
uint8_t discharge_low_temp;
uint8_t discharge_high_temp;
uint8_t charge_low_temp;
uint8_t charge_high_temp;
uint8_t battery_under_voltage;
uint8_t battery_over_voltage;
uint8_t cell_under_voltage;
uint8_t cell_over_voltage;
}
;
struct fet_control_status_t{
uint8_t is_charging;
uint8_t is_discharging;
}
;
struct basic_status_resp_t{
uint16_t total_voltage;
uint16_t current;
uint16_t residual_capacity;
uint16_t nominal_capacity;
uint16_t cycle_life;
uint16_t product_date;
uint16_t balance_status;
uint16_t balance_status_high;
protection_status_t protection_status;
uint8_t version;
uint8_t rsoc;
fet_control_status_t fet_control_status;
uint8_t cell_series_count;
struct {
uint16_t*elem;
 size_t count;
} ntc_temperature;
}
;
struct cell_voltage_resp_t{
struct {
uint16_t*elem;
 size_t count;
} cell_voltage;
}
;


#include <setjmp.h>
struct NailArenaPool;
struct NailArena;
struct NailArenaPos;
typedef struct NailArena NailArena;
typedef struct NailArenaPos NailArenaPos;
NailArenaPos n_arena_save(NailArena *arena);
void n_arena_restore(NailArena *arena, NailArenaPos p);

struct NailArena {
  struct NailArenaPool *current;
  size_t blocksize;
  jmp_buf *error_ret; // XXX: Leaks memory on OOM. Keep a linked list of erroring arenas? 
};
struct NailArenaPos{
        struct NailArenaPool *pool;
        char *iter;
} ;
extern int NailArena_init(NailArena *arena,size_t blocksize, jmp_buf *error_return);
extern int NailArena_release(NailArena *arena);
extern void *n_malloc(NailArena *arena, size_t size);
struct NailStream {
    const uint8_t *data;
    size_t size;
    size_t pos;
    signed char bit_offset;
};

struct NailOutStream {
  uint8_t *data;
  size_t size;
  size_t pos;
  signed char bit_offset;
};
typedef struct NailStream NailStream;
typedef struct NailOutStream NailOutStream;
typedef size_t NailStreamPos;
typedef size_t NailOutStreamPos;
static NailStream * NailStream_alloc(NailArena *arena) {
        return (NailStream *)n_malloc(arena, sizeof(NailStream));
} 
static NailOutStream * NailOutStream_alloc(NailArena *arena) {
        return (NailOutStream *)n_malloc(arena, sizeof(NailOutStream));
} 
extern int NailOutStream_init(NailOutStream *str,size_t siz);
extern void NailOutStream_release(NailOutStream *str);
const uint8_t * NailOutStream_buffer(NailOutStream *str,size_t *siz);
extern int NailOutStream_grow(NailOutStream *stream, size_t count);

#define n_fail(i) __builtin_expect(i,0)
cmd_resp_t*parse_cmd_resp_t(NailArena *arena, const uint8_t *data, size_t size);
protection_status_t*parse_protection_status_t(NailArena *arena, const uint8_t *data, size_t size);
fet_control_status_t*parse_fet_control_status_t(NailArena *arena, const uint8_t *data, size_t size);
basic_status_resp_t*parse_basic_status_resp_t(NailArena *arena, const uint8_t *data, size_t size);
cell_voltage_resp_t*parse_cell_voltage_resp_t(NailArena *arena, const uint8_t *data, size_t size);
extern int checksum_parse(NailArena *tmp,NailStream *str_checksum,NailStream *current);

int gen_cmd_resp_t(NailArena *tmp_arena,NailOutStream *out,cmd_resp_t * val);
extern  int checksum_generate(NailArena *tmp_arena,NailOutStream *str_checksum,NailOutStream *str_current);int gen_protection_status_t(NailArena *tmp_arena,NailOutStream *out,protection_status_t * val);
int gen_fet_control_status_t(NailArena *tmp_arena,NailOutStream *out,fet_control_status_t * val);
int gen_basic_status_resp_t(NailArena *tmp_arena,NailOutStream *out,basic_status_resp_t * val);
int gen_cell_voltage_resp_t(NailArena *tmp_arena,NailOutStream *out,cell_voltage_resp_t * val);


