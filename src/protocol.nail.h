#include <stdint.h>
 #include <string.h>
#include <assert.h>
enum N_types {_NAIL_NULL};
typedef struct cmd_resp_t cmd_resp_t;
typedef struct cmd_req_t cmd_req_t;
typedef struct protection_status_t protection_status_t;
typedef struct fet_control_status_t fet_control_status_t;
typedef struct basic_status_resp_t basic_status_resp_t;
typedef struct cell_voltage_resp_t cell_voltage_resp_t;
typedef struct motor_controller_cmd_req_t motor_controller_cmd_req_t;
typedef struct motor_controller_error_status_t motor_controller_error_status_t;
typedef struct motor_controller_monitor_1_resp_t motor_controller_monitor_1_resp_t;
struct cmd_resp_t{
uint8_t command;
uint8_t state;
struct {
uint8_t*elem;
 size_t count;
} data;
}
;
struct cmd_req_t{
uint8_t command;
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
struct motor_controller_cmd_req_t{
uint8_t command;
}
;
struct motor_controller_error_status_t{
uint8_t identity;
uint8_t battery_over_voltage;
uint8_t battery_under_voltage;
uint8_t locking;
uint8_t v_positive;
uint8_t controller_over_temp;
uint8_t reset;
uint8_t pedal;
uint8_t hall_sensor;
uint8_t emergency_rev;
uint8_t motor_over_temp;
uint8_t current_meter;
}
;
struct motor_controller_monitor_1_resp_t{
uint8_t tps_pedal;
uint8_t brake_pedal;
uint8_t brake_switch;
uint8_t reverse_switch;
uint8_t hall_a;
uint8_t hall_b;
uint8_t hall_c;
uint8_t battery_voltage;
uint8_t motor_temperature;
uint8_t controller_temperature;
uint8_t setting_direction;
uint8_t actual_direction;
uint8_t brake_switch_2;
uint8_t low_speed;
motor_controller_error_status_t error_status;
uint16_t motor_speed;
uint16_t phase_current;
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
cmd_req_t*parse_cmd_req_t(NailArena *arena, const uint8_t *data, size_t size);
protection_status_t*parse_protection_status_t(NailArena *arena, const uint8_t *data, size_t size);
fet_control_status_t*parse_fet_control_status_t(NailArena *arena, const uint8_t *data, size_t size);
basic_status_resp_t*parse_basic_status_resp_t(NailArena *arena, const uint8_t *data, size_t size);
cell_voltage_resp_t*parse_cell_voltage_resp_t(NailArena *arena, const uint8_t *data, size_t size);
motor_controller_cmd_req_t*parse_motor_controller_cmd_req_t(NailArena *arena, const uint8_t *data, size_t size);
motor_controller_error_status_t*parse_motor_controller_error_status_t(NailArena *arena, const uint8_t *data, size_t size);
motor_controller_monitor_1_resp_t*parse_motor_controller_monitor_1_resp_t(NailArena *arena, const uint8_t *data, size_t size);
extern int checksum_parse(NailArena *tmp,NailStream *current,uint16_t* checksum);
extern int checksum_parse(NailArena *tmp,NailStream *current,uint16_t* checksum);
extern int motor_controller_checksum_parse(NailArena *tmp,NailStream *current,uint8_t* checksum);

int gen_cmd_resp_t(NailArena *tmp_arena,NailOutStream *out,cmd_resp_t * val);
extern  int checksum_generate(NailArena *tmp_arena,NailOutStream *str_current,uint16_t* checksum);int gen_cmd_req_t(NailArena *tmp_arena,NailOutStream *out,cmd_req_t * val);
extern  int checksum_generate(NailArena *tmp_arena,NailOutStream *str_current,uint16_t* checksum);int gen_protection_status_t(NailArena *tmp_arena,NailOutStream *out,protection_status_t * val);
int gen_fet_control_status_t(NailArena *tmp_arena,NailOutStream *out,fet_control_status_t * val);
int gen_basic_status_resp_t(NailArena *tmp_arena,NailOutStream *out,basic_status_resp_t * val);
int gen_cell_voltage_resp_t(NailArena *tmp_arena,NailOutStream *out,cell_voltage_resp_t * val);
int gen_motor_controller_cmd_req_t(NailArena *tmp_arena,NailOutStream *out,motor_controller_cmd_req_t * val);
extern  int motor_controller_checksum_generate(NailArena *tmp_arena,NailOutStream *str_current,uint8_t* checksum);int gen_motor_controller_error_status_t(NailArena *tmp_arena,NailOutStream *out,motor_controller_error_status_t * val);
int gen_motor_controller_monitor_1_resp_t(NailArena *tmp_arena,NailOutStream *out,motor_controller_monitor_1_resp_t * val);


