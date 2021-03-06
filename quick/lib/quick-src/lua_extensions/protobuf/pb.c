/*
 * =====================================================================================
 *
 *      Filename:  pb.c
 *
 *      Description: protoc-gen-lua
 *      Google's Protocol Buffers project, ported to lua.
 *      https://code.google.com/p/protoc-gen-lua/
 *
 *      Copyright (c) 2010 , 林卓毅 (Zhuoyi Lin) netsnail@gmail.com
 *      All rights reserved.
 *
 *      Use, modification and distribution are subject to the "New BSD License"
 *      as listed at <url: http://www.opensource.org/licenses/bsd-license.php >.
 *
 *      Created:  2010年08月02日 18时04分21秒
 *
 *      Company:  NetEase
 *
 * =====================================================================================
 */
#include <stdint.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <sys/types.h> /* This will likely define BYTE_ORDER */

#ifndef BYTE_ORDER
#if (BSD >= 199103)
# include <machine/endian.h>
#else
#if defined(linux) || defined(__linux__)
# include <endian.h>
#else
#define LITTLE_ENDIAN   1234    /* least-significant byte first (vax, pc) */
#define BIG_ENDIAN  4321    /* most-significant byte first (IBM, net) */
#define PDP_ENDIAN  3412    /* LSB first in word, MSW first in long (pdp)*/

#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || \
defined(vax) || defined(ns32000) || defined(sun386) || \
defined(MIPSEL) || defined(_MIPSEL) || defined(BIT_ZERO_ON_RIGHT) || \
defined(__alpha__) || defined(__alpha)
#define BYTE_ORDER    LITTLE_ENDIAN
#endif

#if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || \
defined(MIPSEB) || defined(_MIPSEB) || defined(_IBMR2) || defined(DGUX) ||\
defined(apollo) || defined(__convex__) || defined(_CRAY) || \
defined(__hppa) || defined(__hp9000) || \
defined(__hp9000s300) || defined(__hp9000s700) || \
defined (BIT_ZERO_ON_LEFT) || defined(m68k) || defined(__sparc)
#define BYTE_ORDER  BIG_ENDIAN
#endif
#endif /* linux */
#endif /* BSD */
#endif /* BYTE_ORDER */

#if !defined(BYTE_ORDER) || (BYTE_ORDER == LITTLE_ENDIAN)
#define IS_LITTLE_ENDIAN
#endif

#define IOSTRING_META "protobuf.IOString"

#define checkiostring(L) \
    (IOString*) luaL_checkudata(L, 1, IOSTRING_META)

#define IOSTRING_BUF_LEN 65535

typedef struct{
    size_t size;
    char buf[IOSTRING_BUF_LEN];
} IOString;

union IntOrDouble
{
    double d;
    unsigned char buffer[8];
};

union IntOrFloat
{
    float f;
    unsigned char buffer[4];
};

static void pack_varint(luaL_Buffer *b, uint64_t value)
{
    if (value >= 0x80)
    {
        luaL_addchar(b, value | 0x80);
        value >>= 7;
        if (value >= 0x80)
        {
            luaL_addchar(b, value | 0x80);
            value >>= 7;
            if (value >= 0x80)
            {
                luaL_addchar(b, value | 0x80);
                value >>= 7;
                if (value >= 0x80)
                {
                    luaL_addchar(b, value | 0x80);
                    value >>= 7;
                    if (value >= 0x80)
                    {
                        luaL_addchar(b, value | 0x80);
                        value >>= 7;
                        if (value >= 0x80)
                        {
                            luaL_addchar(b, value | 0x80);
                            value >>= 7;
                            if (value >= 0x80)
                            {
                                luaL_addchar(b, value | 0x80);
                                value >>= 7;
                                if (value >= 0x80)
                                {
                                    luaL_addchar(b, value | 0x80);
                                    value >>= 7;
                                    if (value >= 0x80)
                                    {
                                        luaL_addchar(b, value | 0x80);
                                        value >>= 7;
                                    } 
                                } 
                            } 
                        } 
                    } 
                } 
            } 
        } 
    }
    luaL_addchar(b, value);
} 

static int varint_encoder(lua_State *L)
{
    lua_Number l_value = luaL_checknumber(L, 2);
    uint64_t value = (uint64_t)l_value;

    luaL_Buffer b;
    luaL_buffinit(L, &b);
    
    pack_varint(&b, value);

    lua_settop(L, 1);
    luaL_pushresult(&b);
    lua_call(L, 1, 0);
    return 0;
}

static int signed_varint_encoder(lua_State *L)
{
    lua_Number l_value = luaL_checknumber(L, 2);
    int64_t value = (int64_t)l_value;
    
    luaL_Buffer b;
    luaL_buffinit(L, &b);

    if (value < 0)
    {
        pack_varint(&b, *(uint64_t*)&value);
    }else{
        pack_varint(&b, value);
    }
    
    lua_settop(L, 1);
    luaL_pushresult(&b);
    lua_call(L, 1, 0);
    return 0;
}

static int pack_fixed32(lua_State *L, uint8_t* value){
#ifdef IS_LITTLE_ENDIAN
    lua_pushlstring(L, (char*)value, 4);
#else
    uint32_t v = htole32(*(uint32_t*)value);
    lua_pushlstring(L, (char*)&v, 4);
#endif
    return 0;
}

static int pack_fixed64(lua_State *L, uint8_t* value){
#ifdef IS_LITTLE_ENDIAN
    lua_pushlstring(L, (char*)value, 8);
#else
    uint64_t v = htole64(*(uint64_t*)value);
    lua_pushlstring(L, (char*)&v, 8);
#endif
    return 0;
}

static int struct_pack(lua_State *L)
{
    uint8_t format = luaL_checkinteger(L, 2);
    lua_Number value = luaL_checknumber(L, 3);
    lua_settop(L, 1);

    switch(format){
        case 'i':
            {
                int32_t v = (int32_t)value;
                pack_fixed32(L, (uint8_t*)&v);
                break;
            }
        case 'q':
            {
                int64_t v = (int64_t)value;
                pack_fixed64(L, (uint8_t*)&v);
                break;
            }
        case 'f':
            {
                float v = (float)value;
                pack_fixed32(L, (uint8_t*)&v);
                break;
            }
        case 'd':
            {
                double v = (double)value;
                pack_fixed64(L, (uint8_t*)&v);
                break;
            }
        case 'I':
            {
                uint32_t v = (uint32_t)value;
                pack_fixed32(L, (uint8_t*)&v);
                break;
            }
        case 'Q':
            {
                uint64_t v = (uint64_t) value;
                pack_fixed64(L, (uint8_t*)&v);
                break;
            }
        default:
            luaL_error(L, "Unknown, format");
    }
    lua_call(L, 1, 0);
    return 0;
}

static size_t size_varint(const char* buffer, size_t len)
{
    size_t pos = 0;
    while(buffer[pos] & 0x80){
        ++pos;
        if(pos > len){
            return -1;
        }
    }
    return pos+1;
}

static uint64_t unpack_varint(const char* buffer, size_t len)
{
    uint64_t value = buffer[0] & 0x7f;
    size_t shift = 7;
    size_t pos=0;
    for(pos = 1; pos < len; ++pos)
    {
        value |= ((uint64_t)(buffer[pos] & 0x7f)) << shift;
        shift += 7;
    }
    return value;
}

static int varint_decoder(lua_State *L)
{
    size_t len;
    const char* buffer = luaL_checklstring(L, 1, &len);
    size_t pos = luaL_checkinteger(L, 2);
    
    buffer += pos;
    len = size_varint(buffer, len);
    if(len == -1){
        luaL_error(L, "error data %s, len:%d", buffer, len);
    }else{
        lua_pushnumber(L, (lua_Number)unpack_varint(buffer, len));
        lua_pushinteger(L, len + pos);
    }
    return 2;
}

static int signed_varint_decoder(lua_State *L)
{
    size_t len;
    const char* buffer = luaL_checklstring(L, 1, &len);
    size_t pos = luaL_checkinteger(L, 2);
    buffer += pos;
    len = size_varint(buffer, len);
    
    if(len == -1){
        luaL_error(L, "error data %s, len:%d", buffer, len);
    }else{
        lua_pushnumber(L, (lua_Number)(int64_t)unpack_varint(buffer, len));
        lua_pushinteger(L, len + pos);
    }
    return 2;
}

static int zig_zag_encode32(lua_State *L)
{
    int32_t n = luaL_checkinteger(L, 1);
    uint32_t value = (n << 1) ^ (n >> 31);
    lua_pushinteger(L, value);
    return 1;
}

static int zig_zag_decode32(lua_State *L)
{
    uint32_t n = (uint32_t)luaL_checkinteger(L, 1);
    int32_t value = (n >> 1) ^ - (int32_t)(n & 1);
    lua_pushinteger(L, value);
    return 1;
}

static int zig_zag_encode64(lua_State *L)
{
    int64_t n = (int64_t)luaL_checknumber(L, 1);
    uint64_t value = (n << 1) ^ (n >> 63);
    lua_pushinteger(L, value);
    return 1;
}

static int zig_zag_decode64(lua_State *L)
{
    uint64_t n = (uint64_t)luaL_checknumber(L, 1);
    int64_t value = (n >> 1) ^ - (int64_t)(n & 1);
    lua_pushinteger(L, value);
    return 1;
}

static int read_tag(lua_State *L)
{
    size_t len;
    const char* buffer = luaL_checklstring(L, 1, &len);
    size_t pos = luaL_checkinteger(L, 2);
    
    buffer += pos;
    len = size_varint(buffer, len);
    if(len == -1){
        luaL_error(L, "error data %s, len:%d", buffer, len);
    }else{
        lua_pushlstring(L, buffer, len);
        lua_pushinteger(L, len + pos);
    }
    return 2;
}

static const uint8_t* unpack_fixed32(const uint8_t* buffer, uint8_t* cache)
{
#ifdef IS_LITTLE_ENDIAN
    return buffer;
#else
    *(uint32_t*)cache = le32toh(*(uint32_t*)buffer);
    return cache;
#endif
}

static const uint8_t* unpack_fixed64(const uint8_t* buffer, uint8_t* cache)
{
#ifdef IS_LITTLE_ENDIAN
    return buffer;
#else
    *(uint64_t*)cache = le64toh(*(uint64_t*)buffer);
    return cache;
#endif
}

static int struct_unpack(lua_State *L)
{
	uint8_t out[8];
	uint8_t format = luaL_checkinteger(L, 1);
    size_t len;
    const uint8_t* buffer = (uint8_t*)luaL_checklstring(L, 2, &len);
    size_t pos = luaL_checkinteger(L, 3);

    buffer += pos;
    switch(format){
        case 'i':
            {
                lua_pushinteger(L, *(int32_t*)unpack_fixed32(buffer, out));
                break;
            }
        case 'q':
            {
                lua_pushnumber(L, (lua_Number)*(int64_t*)unpack_fixed64(buffer, out));
                break;
            }
        case 'f':
            {
                // use union to avoid crash on Android (signal 7)
                int i = 0;
                const uint8_t *buf = unpack_fixed32(buffer, out);
                size_t size = len <= 4 ? len : 4;
                union IntOrFloat intOrFloat;
                
                memset(&intOrFloat.buffer, 0, 4);
                for (i = 0; i < size; i++) {
                    intOrFloat.buffer[i] = buf[i];
                }
                
                lua_pushnumber(L, (lua_Number)intOrFloat.f);
                break;
            }
        case 'd':
            {
                // use union to avoid crash on Android (signal 7)
                int i = 0;
                const uint8_t *buf = unpack_fixed64(buffer, out);
                size_t size = len <= 8 ? len : 8;
                union IntOrDouble intOrDouble;
                
                memset(&intOrDouble.buffer, 0, 8);
                for (i = 0; i < size; i++) {
                    intOrDouble.buffer[i] = buf[i];
                }
                
                lua_pushnumber(L, (lua_Number)intOrDouble.d);
                break;
            }
        case 'I':
            {
                lua_pushnumber(L, *(uint32_t*)unpack_fixed32(buffer, out));
                break;
            }
        case 'Q':
            {
                lua_pushnumber(L, (lua_Number)*(uint64_t*)unpack_fixed64(buffer, out));
                break;
            }
        default:
            luaL_error(L, "Unknown, format");
    }
    return 1;
}

static int iostring_new(lua_State* L)
{
    IOString* io = (IOString*)lua_newuserdata(L, sizeof(IOString));
    io->size = 0;

    luaL_getmetatable(L, IOSTRING_META);
    lua_setmetatable(L, -2); 
    return 1;
}

static int iostring_str(lua_State* L)
{
    IOString *io = checkiostring(L);
    lua_pushlstring(L, io->buf, io->size);
    return 1;
}

static int iostring_len(lua_State* L)
{
    IOString *io = checkiostring(L);
    lua_pushinteger(L, io->size);
    return 1;
}

static int iostring_write(lua_State* L)
{
    IOString *io = checkiostring(L);
    size_t size;
    const char* str = luaL_checklstring(L, 2, &size);
    if(io->size + size > IOSTRING_BUF_LEN){
        luaL_error(L, "Out of range");
    }
    memcpy(io->buf + io->size, str, size);
    io->size += size;
    return 0;
}

static int iostring_sub(lua_State* L)
{
    IOString *io = checkiostring(L);
    size_t begin = luaL_checkinteger(L, 2);
    size_t end = luaL_checkinteger(L, 3);

    if(begin > end || end > io->size)
    {
        luaL_error(L, "Out of range");
    }
    lua_pushlstring(L, io->buf + begin - 1, end - begin + 1);
    return 1;
}

static int iostring_clear(lua_State* L)
{
    IOString *io = checkiostring(L);
    io->size = 0; 
    return 0;
}

static const struct luaL_Reg _pb [] = {
    {"varint_encoder", varint_encoder},
    {"signed_varint_encoder", signed_varint_encoder},
    {"read_tag", read_tag},
    {"struct_pack", struct_pack},
    {"struct_unpack", struct_unpack},
    {"varint_decoder", varint_decoder},
    {"signed_varint_decoder", signed_varint_decoder},
    {"zig_zag_decode32", zig_zag_decode32},
    {"zig_zag_encode32", zig_zag_encode32},
    {"zig_zag_decode64", zig_zag_decode64},
    {"zig_zag_encode64", zig_zag_encode64},
    {"new_iostring", iostring_new},
    {NULL, NULL}
};

static const struct luaL_Reg _c_iostring_m [] = {
    {"__tostring", iostring_str},
    {"__len", iostring_len},
    {"write", iostring_write},
    {"sub", iostring_sub},
    {"clear", iostring_clear},
    {NULL, NULL}
};

int luaopen_pb (lua_State *L)
{
    luaL_newmetatable(L, IOSTRING_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_register(L, NULL, _c_iostring_m);
    /* remove metatable from stack */
    lua_pop(L, 1);

    luaL_register(L, "pb", _pb);
    return 1;
} 
