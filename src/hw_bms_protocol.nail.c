#include "hw_bms_protocol.nail.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef int32_t pos;
typedef struct{
        pos *trace;
        pos capacity,iter,grow;
} n_trace; 
#define parser_fail(i) __builtin_expect((i)<0,0)
static uint64_t read_unsigned_bits_littleendian(NailStream *stream, unsigned count) {
    uint64_t retval = 0;
    unsigned int out_idx=0;
    size_t pos = stream->pos;
    char bit_offset = stream->bit_offset;
    const uint8_t *data = stream->data;
    while(count>0) {
        if(bit_offset == 0 && (count &7) ==0) {
            retval|= data[pos] << out_idx;
            out_idx+=8;
            pos ++;
            count-=8;
        }
        else {
            //This can use a lot of performance love
//TODO: test this
            retval |= ((data[pos] >> (bit_offset)) & 1) << out_idx;
            out_idx++;
            count--;
            bit_offset++;
            if(bit_offset >7) {
                bit_offset -= 8;
                pos++;
            }
        }
    }
    stream->pos = pos;
    stream->bit_offset = bit_offset;
    return retval;

}

static uint64_t read_unsigned_bits(NailStream *stream, unsigned count){ 
        uint64_t retval = 0;
        unsigned int out_idx=count;
        size_t pos = stream->pos;
        char bit_offset = stream->bit_offset;
        const uint8_t *data = stream->data;
        //TODO: Implement little endian too
        //Count LSB to MSB
        while(count>0) {
                if(bit_offset == 0 && (count &7) ==0) {
                        out_idx-=8;
                        retval|= data[pos] << out_idx;
                        pos ++;
                        count-=8;
                }
                else{
                        //This can use a lot of performance love
//TODO: implement other endianesses
                        out_idx--;
                        retval |= ((data[pos] >> (7-bit_offset)) & 1) << out_idx;
                        count--;
                        bit_offset++;
                        if(bit_offset > 7){
                                bit_offset -= 8;
                                pos++;
                        }
                }
        }
        stream->pos = pos;
        stream->bit_offset = bit_offset;
    return retval;
}
static int stream_check(const NailStream *stream, unsigned count){
        if(stream->size - (count>>3) - ((stream->bit_offset + count & 7)>>3) < stream->pos)
                return -1;
        return 0;
}
static void stream_advance(NailStream *stream, unsigned count){
        
        stream->pos += count >> 3;
        stream->bit_offset += count &7;
        if(stream->bit_offset > 7){
                stream->pos++;
                stream->bit_offset-=8;
        }
}
static NailStreamPos   stream_getpos(NailStream *stream){
  return (stream->pos << 3) + stream->bit_offset; //TODO: Overflow potential!
}
static void stream_backup(NailStream *stream, unsigned count){
        stream->pos -= count >> 3;
        stream->bit_offset -= count & 7;
        if(stream->bit_offset < 0){
                stream->pos--;
                stream->bit_offset += 8;
         }
}
//#define BITSLICE(x, off, len) read_unsigned_bits(x,off,len)
/* trace is a minimalistic representation of the AST. Many parsers add a count, choice parsers add
 * two pos parameters (which choice was taken and where in the trace it begins)
 * const parsers emit a new input position  
*/
typedef struct{
        pos position;
        pos parser;
        pos result;        
} n_hash;

typedef struct{
//        p_hash *memo;
        unsigned lg_size; // How large is the hashtable - make it a power of two 
} n_hashtable;

static int  n_trace_init(n_trace *out,pos size,pos grow){
        if(size <= 1){
                return -1;
        }
        out->trace = (pos *)malloc(size * sizeof(pos));
        if(!out){
                return -1;
        }
        out->capacity = size -1 ;
        out->iter = 0;
        if(grow < 16){ // Grow needs to be at least 2, but small grow makes no sense
                grow = 16; 
        }
        out->grow = grow;
        return 0;
}
static void n_trace_release(n_trace *out){
        free(out->trace);
        out->trace =NULL;
        out->capacity = 0;
        out->iter = 0;
        out->grow = 0;
}
static pos n_trace_getpos(n_trace *tr){
        return tr->iter;
}
static void n_tr_setpos(n_trace *tr,pos offset){
        assert(offset<tr->capacity);
        tr->iter = offset;
}
static int n_trace_grow(n_trace *out, int space){
        if(out->capacity - space>= out->iter){
                return 0;
        }

        pos * new_ptr= (pos *)realloc(out->trace, (out->capacity + out->grow) * sizeof(pos));
        if(!new_ptr){
                return -1;
        }
        out->trace = new_ptr;
        out->capacity += out->grow;
        return 0;
}
static pos n_tr_memo_optional(n_trace *trace){
        if(n_trace_grow(trace,1))
                return -1;
        trace->trace[trace->iter] = 0xFFFFFFFD;
        return trace->iter++;
}
static void n_tr_optional_succeed(n_trace * trace, pos where){
        trace->trace[where] = -1;
}
static void n_tr_optional_fail(n_trace * trace, pos where){
        trace->trace[where] = trace->iter;
}
static pos n_tr_memo_many(n_trace *trace){
        if(parser_fail(n_trace_grow(trace,2)))
                return -1;
        trace->trace[trace->iter] = 0xFFFFFFFE;
        trace->trace[trace->iter+1] = 0xEEEEEEEF;
        trace->iter +=2;
        return trace->iter-2;

}
static void n_tr_write_many(n_trace *trace, pos where, pos count){
        trace->trace[where] = count;
        trace->trace[where+1] = trace->iter;
#ifdef NAIL_DEBUG
        fprintf(stderr,"%d = many %d %d\n", where, count,trace->iter);
#endif
}

static pos n_tr_begin_choice(n_trace *trace){
        if(parser_fail(n_trace_grow(trace,2)))
                return -1;
      
        //Debugging values
        trace->trace[trace->iter] = 0xFFFFFFFF;
        trace->trace[trace->iter+1] = 0xEEEEEEEE;
        trace->iter+= 2;
        return trace->iter - 2;
}
static int n_tr_stream(n_trace *trace, const NailStream *stream){
        assert(sizeof(stream) % sizeof(pos) == 0);
        if(parser_fail(n_trace_grow(trace,sizeof(*stream)/sizeof(pos))))
                return -1;
        *(NailStream *)(trace->trace + trace->iter) = *stream;
#ifdef NAIL_DEBUG
        fprintf(stderr,"%d = stream\n",trace->iter,stream);
#endif
        trace->iter += sizeof(*stream)/sizeof(pos);  
        return 0;
}
static pos n_tr_memo_choice(n_trace *trace){
        return trace->iter;
}
static void n_tr_pick_choice(n_trace *trace, pos where, pos which_choice, pos  choice_begin){
        trace->trace[where] = which_choice;
        trace->trace[where + 1] = choice_begin;
#ifdef NAIL_DEBUG
        fprintf(stderr,"%d = pick %d %d\n",where, which_choice,choice_begin);
#endif
}
static int n_tr_const(n_trace *trace,NailStream *stream){
        if(parser_fail(n_trace_grow(trace,1)))
                return -1;
        NailStreamPos newoff = stream_getpos(stream);
#ifdef NAIL_DEBUG
        fprintf(stderr,"%d = const %d \n",trace->iter, newoff);        
#endif
        trace->trace[trace->iter++] = newoff;
        return 0;
}
#define n_tr_offset n_tr_const
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>
#define parser_fail(i) __builtin_expect((i)<0,0)

static int stream_reposition(NailStream *stream, NailStreamPos p)
{
        stream->pos = p >> 3;
        stream->bit_offset = p & 7;
        return 0;
}
static int NailOutStream_reposition(NailOutStream *stream, NailOutStreamPos p)
{
        stream->pos = p >> 3;
        stream->bit_offset = p & 7;
        return 0;
}
static NailOutStreamPos   NailOutStream_getpos(NailOutStream *stream){
  return (stream->pos << 3) + stream->bit_offset; //TODO: Overflow potential!
}


int NailOutStream_init(NailOutStream *out,size_t siz){
        out->data = ( uint8_t *)malloc(siz);
        if(!out->data)
                return -1;
        out->pos = 0;
        out->bit_offset = 0;
        out->size = siz;
        return 0;
}
void NailOutStream_release(NailOutStream *out){
  free((void *)out->data);
        out->data = NULL;
}
const uint8_t * NailOutStream_buffer(NailOutStream *str,size_t *siz){
        if(str->bit_offset)
                return NULL;
        *siz =  str->pos;
        return str->data;
}
//TODO: Perhaps use a separate structure for output streams?
int NailOutStream_grow(NailOutStream *stream, size_t count){
  if(stream->pos + (count>>3) + 1 >= stream->size){
                //TODO: parametrize stream growth
    int alloc_size = stream->pos + (count>>3) + 1;
                if(4096+stream->size>alloc_size) alloc_size = 4096+stream->size;
                stream->data = (uint8_t *)realloc((void *)stream->data,alloc_size);
                stream->size = alloc_size;
                if(!stream->data)
                        return -1;
        }
        return 0;
}
static int NailOutStream_write(NailOutStream *stream,uint64_t data, size_t count){
        if(parser_fail(NailOutStream_grow(stream, count))){
                return -1;
        }
        uint8_t *streamdata = (uint8_t *)stream->data;
        while(count>0){
                if(stream->bit_offset == 0 && (count & 7) == 0){
                        count -= 8;
                        streamdata[stream->pos] = (data >> count );
                        stream->pos++;
                }
                else{
                        count--;
                        if((data>>count)&1)
                                streamdata[stream->pos] |= 1 << (7-stream->bit_offset);
                        else 
                                streamdata[stream->pos] &= ~(1<< (7-stream->bit_offset));
                        stream->bit_offset++;
                        if(stream->bit_offset>7){
                                stream->pos++;
                                stream->bit_offset= 0;
                        }
                }
        }
        return 0;
}

typedef struct NailArenaPool{
        char *iter;char *end;
        struct NailArenaPool *next;
} NailArenaPool;

void *n_malloc(NailArena *arena, size_t size)
{
        void *retval;
        if(arena->current->end - arena->current->iter <= size){
                size_t siz = arena->blocksize;
                if(size>siz)
                        siz = size + sizeof(NailArenaPool);
                NailArenaPool *newpool  = (NailArenaPool *)malloc(siz);
                if(!newpool){
                  longjmp(*arena->error_ret, -1);
                }
                newpool->end = (char *)((char *)newpool + siz);
                newpool->iter = (char*)(newpool+1);
                newpool->next = arena->current;
                arena->current= newpool;
        }
        retval = (void *)arena->current->iter;
        arena->current->iter += size;
        memset(retval,0,size);
        return retval;
}

int NailArena_init(NailArena *arena, size_t blocksize, jmp_buf *err){
        if(blocksize< 2*sizeof(NailArena))
                blocksize = 2*sizeof(NailArena);
        arena->current = (NailArenaPool*)malloc(blocksize);
        if(!arena->current) return 0;
        arena->current->next = NULL;
        arena->current->iter = (char *)(arena->current + 1);
        arena->current->end = (char *) arena->current + blocksize;
        arena->blocksize = blocksize;
        arena->error_ret = err;
        return 1;
}
int NailArena_release(NailArena *arena){
        NailArenaPool *p;
        while((p= arena->current) ){
                arena->current = p->next;
                free(p);
        }
        arena->blocksize = 0;
        return 0;
}

NailArenaPos n_arena_save(NailArena *arena)
{
    NailArenaPos retval = {.pool = arena->current, .iter = arena->current->iter};
    return retval;
}
void n_arena_restore(NailArena *arena, NailArenaPos p) {
    arena->current = p.pool;
    arena->current->iter = p.iter;
    //memory will remain linked
}
//Returns the pointer where the taken choice is supposed to go.








static pos peg_cmd_frame_t(NailArena *tmp_arena,n_trace *trace,NailStream *str_current);
static pos peg_protection_status_t(NailArena *tmp_arena,n_trace *trace,NailStream *str_current);
static pos peg_fet_control_status_t(NailArena *tmp_arena,n_trace *trace,NailStream *str_current);
static pos peg_basic_status_resp_t(NailArena *tmp_arena,n_trace *trace,NailStream *str_current);
static pos peg_cell_voltage_resp_t(NailArena *tmp_arena,n_trace *trace,NailStream *str_current);
static pos peg_cmd_frame_t(NailArena *tmp_arena,n_trace *trace, NailStream *str_current){
pos i;
if(parser_fail(stream_check(str_current,8))) {goto fail;}
if( read_unsigned_bits(str_current,8)!= 221){stream_backup(str_current,8);goto fail;}if(parser_fail(n_tr_const(trace,str_current))){goto fail;}
if(parser_fail(stream_check(str_current,8))) {goto fail;}
{
 uint64_t val = read_unsigned_bits(str_current,8);
if(val!=165 && val!=90){stream_backup(str_current,8);goto fail;}
}
if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
; 
 pos trace_data_sz = n_trace_getpos(trace);
uint8_t dep_data_sz;
{
NailStream tmpstream = *str_current;
if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
NailStream *stream = &tmpstream;dep_data_sz=read_unsigned_bits(stream,8);
}n_tr_setpos(trace,trace_data_sz);
n_tr_const(trace,str_current);
{/*LENGTH*/pos many = n_tr_memo_many(trace);
pos count= dep_data_sz;
pos i1=0;for( i1=0;i1<count;i1++){if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
}n_tr_write_many(trace,many,count);
}/*/LENGTH*/if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
if(parser_fail(stream_check(str_current,8))) {goto fail;}
if( read_unsigned_bits(str_current,8)!= 119){stream_backup(str_current,8);goto fail;}if(parser_fail(n_tr_const(trace,str_current))){goto fail;}
return 0;
fail:
 return -1;
}
static pos peg_protection_status_t(NailArena *tmp_arena,n_trace *trace, NailStream *str_current){
pos i;
if(parser_fail(stream_check(str_current,3))) {goto fail;}
if( read_unsigned_bits(str_current,3)!= 0){stream_backup(str_current,3);goto fail;}if(parser_fail(n_tr_const(trace,str_current))){goto fail;}
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
return 0;
fail:
 return -1;
}
static pos peg_fet_control_status_t(NailArena *tmp_arena,n_trace *trace, NailStream *str_current){
pos i;
if(parser_fail(stream_check(str_current,6))) {goto fail;}
if( read_unsigned_bits(str_current,6)!= 0){stream_backup(str_current,6);goto fail;}if(parser_fail(n_tr_const(trace,str_current))){goto fail;}
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
if(parser_fail(stream_check(str_current,1))) {goto fail;}
stream_advance(str_current,1);
return 0;
fail:
 return -1;
}
static pos peg_basic_status_resp_t(NailArena *tmp_arena,n_trace *trace, NailStream *str_current){
pos i;
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
if(parser_fail(peg_protection_status_t(tmp_arena,trace,str_current))) {goto fail;}
if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
if(parser_fail(peg_fet_control_status_t(tmp_arena,trace,str_current))) {goto fail;}
if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
; 
 pos trace_ntc_count = n_trace_getpos(trace);
uint8_t dep_ntc_count;
{
NailStream tmpstream = *str_current;
if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
NailStream *stream = &tmpstream;dep_ntc_count=read_unsigned_bits(stream,8);
}n_tr_setpos(trace,trace_ntc_count);
n_tr_const(trace,str_current);
{/*LENGTH*/pos many = n_tr_memo_many(trace);
pos count= dep_ntc_count;
pos i2=0;for( i2=0;i2<count;i2++){if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
}n_tr_write_many(trace,many,count);
}/*/LENGTH*/return 0;
fail:
 return -1;
}
static pos peg_cell_voltage_resp_t(NailArena *tmp_arena,n_trace *trace, NailStream *str_current){
pos i;
; 
 pos trace_cell_count = n_trace_getpos(trace);
uint8_t dep_cell_count;
{
NailStream tmpstream = *str_current;
if(parser_fail(stream_check(str_current,8))) {goto fail;}
stream_advance(str_current,8);
NailStream *stream = &tmpstream;dep_cell_count=read_unsigned_bits(stream,8);
}n_tr_setpos(trace,trace_cell_count);
n_tr_const(trace,str_current);
{/*LENGTH*/pos many = n_tr_memo_many(trace);
pos count= dep_cell_count;
pos i3=0;for( i3=0;i3<count;i3++){if(parser_fail(stream_check(str_current,16))) {goto fail;}
stream_advance(str_current,16);
}n_tr_write_many(trace,many,count);
}/*/LENGTH*/return 0;
fail:
 return -1;
}


static pos bind_cmd_frame_t(NailArena *arena,cmd_frame_t*out,NailStream *stream, pos **trace,  pos * trace_begin);static pos bind_protection_status_t(NailArena *arena,protection_status_t*out,NailStream *stream, pos **trace,  pos * trace_begin);static pos bind_fet_control_status_t(NailArena *arena,fet_control_status_t*out,NailStream *stream, pos **trace,  pos * trace_begin);static pos bind_basic_status_resp_t(NailArena *arena,basic_status_resp_t*out,NailStream *stream, pos **trace,  pos * trace_begin);static pos bind_cell_voltage_resp_t(NailArena *arena,cell_voltage_resp_t*out,NailStream *stream, pos **trace,  pos * trace_begin);
static int bind_cmd_frame_t(NailArena *arena,cmd_frame_t*out,NailStream *stream, pos **trace ,  pos * trace_begin){
 pos *tr = *trace;stream_reposition(stream,*tr);
tr++;
out->status=read_unsigned_bits(stream,8);
out->command=read_unsigned_bits(stream,8);
stream_reposition(stream,*tr);
tr++;
{ /*ARRAY*/ 
 pos save = 0;out->data.count=*(tr++);
save = *(tr++);
out->data.elem= (typeof(out->data.elem))n_malloc(arena,out->data.count* sizeof(*out->data.elem));
for(pos i1=0;i1<out->data.count;i1++){out->data.elem[i1]=read_unsigned_bits(stream,8);
}
tr = trace_begin + save;
}out->checksum=read_unsigned_bits(stream,8);
stream_reposition(stream,*tr);
tr++;
*trace = tr;return 0;}cmd_frame_t*parse_cmd_frame_t(NailArena *arena, const uint8_t *data, size_t size){
NailStream stream = {.data = data, .pos= 0, .size = size, .bit_offset = 0};
NailArena tmp_arena;NailArena_init(&tmp_arena, 4096, arena->error_ret);n_trace trace;
pos *tr_ptr;
 pos pos;
cmd_frame_t* retval;
n_trace_init(&trace,4096,4096);
if(parser_fail(peg_cmd_frame_t(&tmp_arena,&trace,&stream))) goto fail;if(stream.pos != stream.size) goto fail; retval =  (typeof(retval))n_malloc(arena,sizeof(*retval));
stream.pos = 0;
tr_ptr = trace.trace;if(bind_cmd_frame_t(arena,retval,&stream,&tr_ptr,trace.trace) < 0) goto fail;
out: n_trace_release(&trace);
NailArena_release(&tmp_arena);return retval;fail: retval = NULL; goto out;}
static int bind_protection_status_t(NailArena *arena,protection_status_t*out,NailStream *stream, pos **trace ,  pos * trace_begin){
 pos *tr = *trace;stream_reposition(stream,*tr);
tr++;
out->mosfet_software_lock=read_unsigned_bits(stream,1);
out->ic_frontend_error=read_unsigned_bits(stream,1);
out->short_circuit=read_unsigned_bits(stream,1);
out->discharge_overcurrent=read_unsigned_bits(stream,1);
out->charge_overcurrent=read_unsigned_bits(stream,1);
out->discharge_low_temp=read_unsigned_bits(stream,1);
out->discharge_high_temp=read_unsigned_bits(stream,1);
out->charge_low_temp=read_unsigned_bits(stream,1);
out->charge_high_temp=read_unsigned_bits(stream,1);
out->battery_under_voltage=read_unsigned_bits(stream,1);
out->battery_over_voltage=read_unsigned_bits(stream,1);
out->cell_under_voltage=read_unsigned_bits(stream,1);
out->cell_over_voltage=read_unsigned_bits(stream,1);
*trace = tr;return 0;}protection_status_t*parse_protection_status_t(NailArena *arena, const uint8_t *data, size_t size){
NailStream stream = {.data = data, .pos= 0, .size = size, .bit_offset = 0};
NailArena tmp_arena;NailArena_init(&tmp_arena, 4096, arena->error_ret);n_trace trace;
pos *tr_ptr;
 pos pos;
protection_status_t* retval;
n_trace_init(&trace,4096,4096);
if(parser_fail(peg_protection_status_t(&tmp_arena,&trace,&stream))) goto fail;if(stream.pos != stream.size) goto fail; retval =  (typeof(retval))n_malloc(arena,sizeof(*retval));
stream.pos = 0;
tr_ptr = trace.trace;if(bind_protection_status_t(arena,retval,&stream,&tr_ptr,trace.trace) < 0) goto fail;
out: n_trace_release(&trace);
NailArena_release(&tmp_arena);return retval;fail: retval = NULL; goto out;}
static int bind_fet_control_status_t(NailArena *arena,fet_control_status_t*out,NailStream *stream, pos **trace ,  pos * trace_begin){
 pos *tr = *trace;stream_reposition(stream,*tr);
tr++;
out->is_charging=read_unsigned_bits(stream,1);
out->is_discharging=read_unsigned_bits(stream,1);
*trace = tr;return 0;}fet_control_status_t*parse_fet_control_status_t(NailArena *arena, const uint8_t *data, size_t size){
NailStream stream = {.data = data, .pos= 0, .size = size, .bit_offset = 0};
NailArena tmp_arena;NailArena_init(&tmp_arena, 4096, arena->error_ret);n_trace trace;
pos *tr_ptr;
 pos pos;
fet_control_status_t* retval;
n_trace_init(&trace,4096,4096);
if(parser_fail(peg_fet_control_status_t(&tmp_arena,&trace,&stream))) goto fail;if(stream.pos != stream.size) goto fail; retval =  (typeof(retval))n_malloc(arena,sizeof(*retval));
stream.pos = 0;
tr_ptr = trace.trace;if(bind_fet_control_status_t(arena,retval,&stream,&tr_ptr,trace.trace) < 0) goto fail;
out: n_trace_release(&trace);
NailArena_release(&tmp_arena);return retval;fail: retval = NULL; goto out;}
static int bind_basic_status_resp_t(NailArena *arena,basic_status_resp_t*out,NailStream *stream, pos **trace ,  pos * trace_begin){
 pos *tr = *trace;out->total_voltage=read_unsigned_bits(stream,16);
out->current=read_unsigned_bits(stream,16);
out->residual_capacity=read_unsigned_bits(stream,16);
out->nominal_capacity=read_unsigned_bits(stream,16);
out->cycle_life=read_unsigned_bits(stream,16);
out->product_date=read_unsigned_bits(stream,16);
out->balance_status=read_unsigned_bits(stream,16);
out->balance_status_high=read_unsigned_bits(stream,16);
if(parser_fail(bind_protection_status_t(arena,&out->protection_status, stream,&tr,trace_begin))) { return -1;}out->version=read_unsigned_bits(stream,8);
out->rsoc=read_unsigned_bits(stream,8);
if(parser_fail(bind_fet_control_status_t(arena,&out->fet_control_status, stream,&tr,trace_begin))) { return -1;}out->cell_series_count=read_unsigned_bits(stream,8);
stream_reposition(stream,*tr);
tr++;
{ /*ARRAY*/ 
 pos save = 0;out->ntc_temperature.count=*(tr++);
save = *(tr++);
out->ntc_temperature.elem= (typeof(out->ntc_temperature.elem))n_malloc(arena,out->ntc_temperature.count* sizeof(*out->ntc_temperature.elem));
for(pos i2=0;i2<out->ntc_temperature.count;i2++){out->ntc_temperature.elem[i2]=read_unsigned_bits(stream,16);
}
tr = trace_begin + save;
}*trace = tr;return 0;}basic_status_resp_t*parse_basic_status_resp_t(NailArena *arena, const uint8_t *data, size_t size){
NailStream stream = {.data = data, .pos= 0, .size = size, .bit_offset = 0};
NailArena tmp_arena;NailArena_init(&tmp_arena, 4096, arena->error_ret);n_trace trace;
pos *tr_ptr;
 pos pos;
basic_status_resp_t* retval;
n_trace_init(&trace,4096,4096);
if(parser_fail(peg_basic_status_resp_t(&tmp_arena,&trace,&stream))) goto fail;if(stream.pos != stream.size) goto fail; retval =  (typeof(retval))n_malloc(arena,sizeof(*retval));
stream.pos = 0;
tr_ptr = trace.trace;if(bind_basic_status_resp_t(arena,retval,&stream,&tr_ptr,trace.trace) < 0) goto fail;
out: n_trace_release(&trace);
NailArena_release(&tmp_arena);return retval;fail: retval = NULL; goto out;}
static int bind_cell_voltage_resp_t(NailArena *arena,cell_voltage_resp_t*out,NailStream *stream, pos **trace ,  pos * trace_begin){
 pos *tr = *trace;stream_reposition(stream,*tr);
tr++;
{ /*ARRAY*/ 
 pos save = 0;out->cell_voltage.count=*(tr++);
save = *(tr++);
out->cell_voltage.elem= (typeof(out->cell_voltage.elem))n_malloc(arena,out->cell_voltage.count* sizeof(*out->cell_voltage.elem));
for(pos i3=0;i3<out->cell_voltage.count;i3++){out->cell_voltage.elem[i3]=read_unsigned_bits(stream,16);
}
tr = trace_begin + save;
}*trace = tr;return 0;}cell_voltage_resp_t*parse_cell_voltage_resp_t(NailArena *arena, const uint8_t *data, size_t size){
NailStream stream = {.data = data, .pos= 0, .size = size, .bit_offset = 0};
NailArena tmp_arena;NailArena_init(&tmp_arena, 4096, arena->error_ret);n_trace trace;
pos *tr_ptr;
 pos pos;
cell_voltage_resp_t* retval;
n_trace_init(&trace,4096,4096);
if(parser_fail(peg_cell_voltage_resp_t(&tmp_arena,&trace,&stream))) goto fail;if(stream.pos != stream.size) goto fail; retval =  (typeof(retval))n_malloc(arena,sizeof(*retval));
stream.pos = 0;
tr_ptr = trace.trace;if(bind_cell_voltage_resp_t(arena,retval,&stream,&tr_ptr,trace.trace) < 0) goto fail;
out: n_trace_release(&trace);
NailArena_release(&tmp_arena);return retval;fail: retval = NULL; goto out;}
int gen_cmd_frame_t(NailArena *tmp_arena,NailOutStream *out,cmd_frame_t * val);
int gen_protection_status_t(NailArena *tmp_arena,NailOutStream *out,protection_status_t * val);
int gen_fet_control_status_t(NailArena *tmp_arena,NailOutStream *out,fet_control_status_t * val);
int gen_basic_status_resp_t(NailArena *tmp_arena,NailOutStream *out,basic_status_resp_t * val);
int gen_cell_voltage_resp_t(NailArena *tmp_arena,NailOutStream *out,cell_voltage_resp_t * val);
int gen_cmd_frame_t(NailArena *tmp_arena,NailOutStream *str_current,cmd_frame_t * val){if(parser_fail(NailOutStream_write(str_current,221,8))) return -1;if(val->status!=165 && val->status!=90){return -1;}if(parser_fail(NailOutStream_write(str_current,val->status,8))) return -1;if(parser_fail(NailOutStream_write(str_current,val->command,8))) return -1;uint8_t dep_data_sz;NailOutStreamPos rewind_data_sz=NailOutStream_getpos(str_current);NailOutStream_write(str_current,0,8);for(int i0=0;i0<val->data.count;i0++){if(parser_fail(NailOutStream_write(str_current,val->data.elem[i0],8))) return -1;}dep_data_sz=val->data.count;if(parser_fail(NailOutStream_write(str_current,val->checksum,8))) return -1;if(parser_fail(NailOutStream_write(str_current,119,8))) return -1;{/*Context-rewind*/
 NailOutStreamPos  end_of_struct= NailOutStream_getpos(str_current);
NailOutStream_reposition(str_current, rewind_data_sz);NailOutStream_write(str_current,dep_data_sz,8);NailOutStream_reposition(str_current, end_of_struct);}return 0;}int gen_protection_status_t(NailArena *tmp_arena,NailOutStream *str_current,protection_status_t * val){if(parser_fail(NailOutStream_write(str_current,0,3))) return -1;if(parser_fail(NailOutStream_write(str_current,val->mosfet_software_lock,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->ic_frontend_error,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->short_circuit,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->discharge_overcurrent,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->charge_overcurrent,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->discharge_low_temp,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->discharge_high_temp,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->charge_low_temp,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->charge_high_temp,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->battery_under_voltage,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->battery_over_voltage,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->cell_under_voltage,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->cell_over_voltage,1))) return -1;{/*Context-rewind*/
 NailOutStreamPos  end_of_struct= NailOutStream_getpos(str_current);
NailOutStream_reposition(str_current, end_of_struct);}return 0;}int gen_fet_control_status_t(NailArena *tmp_arena,NailOutStream *str_current,fet_control_status_t * val){if(parser_fail(NailOutStream_write(str_current,0,6))) return -1;if(parser_fail(NailOutStream_write(str_current,val->is_charging,1))) return -1;if(parser_fail(NailOutStream_write(str_current,val->is_discharging,1))) return -1;{/*Context-rewind*/
 NailOutStreamPos  end_of_struct= NailOutStream_getpos(str_current);
NailOutStream_reposition(str_current, end_of_struct);}return 0;}int gen_basic_status_resp_t(NailArena *tmp_arena,NailOutStream *str_current,basic_status_resp_t * val){if(parser_fail(NailOutStream_write(str_current,val->total_voltage,16))) return -1;if(parser_fail(NailOutStream_write(str_current,val->current,16))) return -1;if(parser_fail(NailOutStream_write(str_current,val->residual_capacity,16))) return -1;if(parser_fail(NailOutStream_write(str_current,val->nominal_capacity,16))) return -1;if(parser_fail(NailOutStream_write(str_current,val->cycle_life,16))) return -1;if(parser_fail(NailOutStream_write(str_current,val->product_date,16))) return -1;if(parser_fail(NailOutStream_write(str_current,val->balance_status,16))) return -1;if(parser_fail(NailOutStream_write(str_current,val->balance_status_high,16))) return -1;if(parser_fail(gen_protection_status_t(tmp_arena,str_current,&val->protection_status))){return -1;}if(parser_fail(NailOutStream_write(str_current,val->version,8))) return -1;if(parser_fail(NailOutStream_write(str_current,val->rsoc,8))) return -1;if(parser_fail(gen_fet_control_status_t(tmp_arena,str_current,&val->fet_control_status))){return -1;}if(parser_fail(NailOutStream_write(str_current,val->cell_series_count,8))) return -1;uint8_t dep_ntc_count;NailOutStreamPos rewind_ntc_count=NailOutStream_getpos(str_current);NailOutStream_write(str_current,0,8);for(int i1=0;i1<val->ntc_temperature.count;i1++){if(parser_fail(NailOutStream_write(str_current,val->ntc_temperature.elem[i1],16))) return -1;}dep_ntc_count=val->ntc_temperature.count;{/*Context-rewind*/
 NailOutStreamPos  end_of_struct= NailOutStream_getpos(str_current);
NailOutStream_reposition(str_current, rewind_ntc_count);NailOutStream_write(str_current,dep_ntc_count,8);NailOutStream_reposition(str_current, end_of_struct);}return 0;}int gen_cell_voltage_resp_t(NailArena *tmp_arena,NailOutStream *str_current,cell_voltage_resp_t * val){uint8_t dep_cell_count;NailOutStreamPos rewind_cell_count=NailOutStream_getpos(str_current);NailOutStream_write(str_current,0,8);for(int i2=0;i2<val->cell_voltage.count;i2++){if(parser_fail(NailOutStream_write(str_current,val->cell_voltage.elem[i2],16))) return -1;}dep_cell_count=val->cell_voltage.count;{/*Context-rewind*/
 NailOutStreamPos  end_of_struct= NailOutStream_getpos(str_current);
NailOutStream_reposition(str_current, rewind_cell_count);NailOutStream_write(str_current,dep_cell_count,8);NailOutStream_reposition(str_current, end_of_struct);}return 0;}


