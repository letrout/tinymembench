/*
 * Copyright © 2020 Joel Luth <joelluth@gmail.com>
 * Based on code by Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#if defined(__amd64__)

.intel_syntax noprefix
.text

#define PREFETCH_DISTANCE 256
#define REGSZ 32

.macro asm_function_helper function_name
    .global \function_name
.func \function_name
\function_name:
#ifdef _WIN64
    .set DST,  rcx
    .set SRC,  rdx
    .set SIZE, r8
#else
    .set DST,  rdi
    .set SRC,  rsi
    .set SIZE, rdx
#endif
.endm

.macro asm_function function_name
asm_function_helper \function_name
.endm

.macro push3 a, b, c
    push \a
    push \b
    push \c
.endm

.macro pop3 a, b, c
    pop \c
    pop \b
    pop \a
.endm

/*****************************************************************************/

asm_function aligned_block_copy_avx2
0:
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
    vmovdqa      ymm1,       [SRC + (1 * REGSZ)]
    vmovdqa      ymm2,       [SRC + (2 * REGSZ)]
    vmovdqa      ymm3,       [SRC + (3 * REGSZ)]
    vmovdqa      [DST + (0 * REGSZ)], ymm0
    vmovdqa      [DST + (1 * REGSZ)], ymm1
    vmovdqa      [DST + (2 * REGSZ)], ymm2
    vmovdqa      [DST + (3 * REGSZ)], ymm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE, (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_nt_avx2
0:
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
    vmovdqa      ymm1,       [SRC + (1 * REGSZ)]
    vmovdqa      ymm2,       [SRC + (2 * REGSZ)]
    vmovdqa      ymm3,       [SRC + (3 * REGSZ)]
    vmovntdq     [DST + (0 * REGSZ)], ymm0
    vmovntdq     [DST + (1 * REGSZ)], ymm1
    vmovntdq     [DST + (2 * REGSZ)], ymm2
    vmovntdq     [DST + (3 * REGSZ)], ymm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE, (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_pf32_avx2
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    prefetchnta [SRC + PREFETCH_DISTANCE + 32]
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
    vmovdqa      ymm1,       [SRC + (1 * REGSZ)]
    vmovdqa      ymm2,       [SRC + (2 * REGSZ)]
    vmovdqa      ymm3,       [SRC + (3 * REGSZ)]
    vmovdqa      [DST + (0 * REGSZ)], ymm0
    vmovdqa      [DST + (1 * REGSZ)], ymm1
    vmovdqa      [DST + (2 * REGSZ)], ymm2
    vmovdqa      [DST + (3 * REGSZ)], ymm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_nt_pf32_avx2
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    prefetchnta [SRC + PREFETCH_DISTANCE + 32]
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
    vmovdqa      ymm1,       [SRC + (1 * REGSZ)]
    vmovdqa      ymm2,       [SRC + (2 * REGSZ)]
    vmovdqa      ymm3,       [SRC + (3 * REGSZ)]
    vmovntdq     [DST + (0 * REGSZ)], ymm0
    vmovntdq     [DST + (1 * REGSZ)], ymm1
    vmovntdq     [DST + (2 * REGSZ)], ymm2
    vmovntdq     [DST + (3 * REGSZ)], ymm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_pf64_avx2
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
    vmovdqa      ymm1,       [SRC + (1 * REGSZ)]
    vmovdqa      ymm2,       [SRC + (2 * REGSZ)]
    vmovdqa      ymm3,       [SRC + (3 * REGSZ)]
    vmovdqa      [DST + (0 * REGSZ)], ymm0
    vmovdqa      [DST + (1 * REGSZ)], ymm1
    vmovdqa      [DST + (2 * REGSZ)], ymm2
    vmovdqa      [DST + (3 * REGSZ)], ymm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_nt_pf64_avx2
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
    vmovdqa      ymm1,       [SRC + (1 * REGSZ)]
    vmovdqa      ymm2,       [SRC + (2 * REGSZ)]
    vmovdqa      ymm3,       [SRC + (3 * REGSZ)]
    vmovntdq     [DST + (0 * REGSZ)], ymm0
    vmovntdq     [DST + (1 * REGSZ)], ymm1
    vmovntdq     [DST + (2 * REGSZ)], ymm2
    vmovntdq     [DST + (3 * REGSZ)], ymm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_fill_avx2
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
0:
    vmovdqa      [DST + (0 * REGSZ)], ymm0
    vmovdqa      [DST + (1 * REGSZ)], ymm0
    vmovdqa      [DST + (2 * REGSZ)], ymm0
    vmovdqa      [DST + (3 * REGSZ)], ymm0
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_fill_nt_avx2
    vmovdqa      ymm0,       [SRC + (0 * REGSZ)]
0:
    vmovntdq     [DST + (0 * REGSZ)], ymm0
    vmovntdq     [DST + (1 * REGSZ)], ymm0
    vmovntdq     [DST + (2 * REGSZ)], ymm0
    vmovntdq     [DST + (3 * REGSZ)], ymm0
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

/*****************************************************************************/

#endif
