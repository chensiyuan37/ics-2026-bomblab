#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

static uint32_t p1_step2(uint32_t high, uint32_t low) {
    return (low + 0x8f01) ^ high;
}

static uint32_t p1_step0(uint32_t low, uint32_t shift16) {
    return (low * 5) ^ (shift16 + 0xd5a8);
}

static uint32_t p1_step1(uint32_t res0, uint32_t res2) {
    return (res0 * 3 + res2) ^ 0x5a9a;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1; 

    uint32_t low = (uint32_t)challenge;                
    uint32_t high = (uint32_t)(challenge >> 32);       
    uint32_t shift16 = (uint32_t)(challenge >> 16);    

    uint32_t res2 = p1_step2(high, low);
    uint32_t res0 = p1_step0(low, shift16);
    uint32_t final_res = p1_step1(res0, res2);

    values[0] = final_res;

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6; 

    values[0] = (((uint32_t)(challenge >> 27) * 2) & 0x7e) + 6;

    int shift = 9;
    for (int i = 1; i < 6; i++) {
        uint32_t part = ((challenge >> shift) & 0x3f) ^ i;
        values[i] = part + values[i - 1] * 4;
        shift += 4; 
    }

    return write_answer(out, cap, values, count);
}

static uint32_t p3_dispatch(uint32_t index, uint32_t val) {
    switch (index) {
        case 0: return (val * 4) ^ 0x462a;                 
        case 1: return val * 5 + 0x22f10;                  
        case 2: return (val * 4) ^ 0x95a4;                 
        case 3: return (val ^ 0xd28f) + (val >> 1);        
        case 4: return val * 5 + 0x26f6;                   
        case 5: return (val ^ 0x7df8) + (val >> 2);        
        case 6: return val * 5 + 0x20927;                  
        case 7: return val * 3 + 0x6275;                  
        default: return 0;
    }
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2; 

    uint32_t index = (uint32_t)(challenge & 0x7);
    values[0] = index;

    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t shift3 = (uint32_t)(challenge >> 3);
    uint32_t val = high ^ shift3;

    values[1] = p3_dispatch(index, val);

    return write_answer(out, cap, values, count);
}

static uint32_t p4_walk(uint32_t a, uint32_t lo, uint32_t hi, uint32_t c) {
    uint32_t mid = lo + (hi - lo) / 2;
    
    if (mid == a) {
        return (mid + 0xe0) ^ c;
    }
    
    uint32_t ebx = mid + c;
    if (a < mid) {
        uint32_t res = p4_walk(a, lo, mid - 1, c);
        return ((res * 3) ^ ebx) ^ 0x3;
    } else {
        uint32_t res = p4_walk(a, mid + 1, hi, c);
        return ((res * 3) ^ ebx) ^ 0xb;
    }
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1; 

    uint32_t a = (uint32_t)(challenge & 0x7F);
    uint32_t c = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);

    values[0] = p4_walk(a, 0, 0x7F, c);

    return write_answer(out, cap, values, count);
}

static uint32_t p5_traverse(uint64_t challenge, uint32_t *out_val) {
    static const int64_t  nexts[] = { 2, 11, 5, 8, 3, 9, 1, 6, 0, 10, 7, -1 };
    static const uint32_t v1[]    = { 0x720, 0x12e, 0x541, 0x667, 0x930, 0x35, 0xb4e, 0x235, 0x454, 0xa1c, 0x33b, 0x36e };
    static const uint32_t v2[]    = { 0x899c, 0x6488, 0xb32f, 0xf152, 0x8415, 0x93e8, 0xcc65, 0x7d6a, 0x53df, 0x4697, 0xa04f, 0x8dc0 };
    static const int heads[]      = { 4, 3, 8, 0, 2, 5, 9, 10 };

    uint32_t idx = (uint32_t)(challenge & 0x7);
    int curr = heads[idx];
    
    uint32_t key = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    key ^= 0x785e;

    key = key * 3 + v2[curr];
    *out_val = v1[curr];
    key = key ^ v1[curr];

    uint32_t loops = (uint32_t)((challenge >> 3) & 0x3);
    for (uint32_t i = 0; i < loops; i++) {
        curr = nexts[curr];
        key = key * 3 + v2[curr];
        *out_val = v1[curr];
        key = key ^ v1[curr];
    }

    return key;
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2; 

    uint32_t out_val = 0;
    values[0] = p5_traverse(challenge, &out_val);
    values[1] = out_val;

    return write_answer(out, cap, values, count);
}

static uint32_t p6_op_0(uint32_t x) { return (x ^ 0x363b) + x * 4; }
static uint32_t p6_op_1(uint32_t x) { return ((x << 7) + 0xaff) ^ (x * 5); }
static uint32_t p6_op_2(uint32_t x) { return (x >> 3) | (x << 29); } // ror 3
static uint32_t p6_op_3(uint32_t x) { return (x + 0x16ac) ^ (x >> 1); }
static uint32_t p6_op_4(uint32_t x) { return ((x << 3) | (x >> 29)) ^ 0x47d5; } // rol 3
static uint32_t p6_op_5(uint32_t x) { return ((x >> 1) ^ x) + 0x2dff; }

static uint32_t (*p6_ops[])(uint32_t) = {
    p6_op_3, p6_op_1, p6_op_2, p6_op_4, p6_op_0, p6_op_5, p6_op_4, p6_op_4
};

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1; 

    uint32_t idx1 = (uint32_t)(challenge & 0x7);
    uint32_t arg1 = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t res1 = p6_ops[idx1](arg1);

    uint32_t idx2 = (uint32_t)((challenge >> 3) & 0x7);
    uint32_t arg2 = res1 ^ 0xab37;
    uint32_t res2 = p6_ops[idx2](arg2);

    values[0] = res2 + 0x6fc5;

    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t v0 = (uint32_t)context & 0x3;

    uint32_t m = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t r8 = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t edi = m ^ r8;

    uint32_t mixed = ((edi >> 1) | (edi << 31)) ^ 0xb741;

    uint32_t r9 = (v0 * 0xbea8) ^ (uint32_t)(context >> 16);

    uint32_t v1 = mixed ^ r9;

    uint32_t edx = r8 + v0 + 0x9e9 + (m >> 1);
    uint32_t v2 = v1 ^ edx;

    int written = snprintf(out, cap, "%32sOPEN %010u %010u %010u", "", v0, v1, v2);
    if (written < 0 || (size_t)written >= cap) {
        return 1; 
    }

    return 0; 
}
