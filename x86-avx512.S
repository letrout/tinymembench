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
#define REGSZ 64

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

asm_function aligned_block_copy_avx512
0:
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
    vmovdqa64      zmm1,       [SRC + (1 * REGSZ)]
    vmovdqa64      zmm2,       [SRC + (2 * REGSZ)]
    vmovdqa64      zmm3,       [SRC + (3 * REGSZ)]
    vmovdqa64      [DST + (0 * REGSZ)], zmm0
    vmovdqa64      [DST + (1 * REGSZ)], zmm1
    vmovdqa64      [DST + (2 * REGSZ)], zmm2
    vmovdqa64      [DST + (3 * REGSZ)], zmm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE, (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_nt_avx512
0:
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
    vmovdqa64      zmm1,       [SRC + (1 * REGSZ)]
    vmovdqa64      zmm2,       [SRC + (2 * REGSZ)]
    vmovdqa64      zmm3,       [SRC + (3 * REGSZ)]
    vmovntdq     [DST + (0 * REGSZ)], zmm0
    vmovntdq     [DST + (1 * REGSZ)], zmm1
    vmovntdq     [DST + (2 * REGSZ)], zmm2
    vmovntdq     [DST + (3 * REGSZ)], zmm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE, (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_pf32_avx512
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    prefetchnta [SRC + PREFETCH_DISTANCE + 32]
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
    vmovdqa64      zmm1,       [SRC + (1 * REGSZ)]
    vmovdqa64      zmm2,       [SRC + (2 * REGSZ)]
    vmovdqa64      zmm3,       [SRC + (3 * REGSZ)]
    vmovdqa64      [DST + (0 * REGSZ)], zmm0
    vmovdqa64      [DST + (1 * REGSZ)], zmm1
    vmovdqa64      [DST + (2 * REGSZ)], zmm2
    vmovdqa64      [DST + (3 * REGSZ)], zmm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_nt_pf32_avx512
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    prefetchnta [SRC + PREFETCH_DISTANCE + 32]
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
    vmovdqa64      zmm1,       [SRC + (1 * REGSZ)]
    vmovdqa64      zmm2,       [SRC + (2 * REGSZ)]
    vmovdqa64      zmm3,       [SRC + (3 * REGSZ)]
    vmovntdq     [DST + (0 * REGSZ)], zmm0
    vmovntdq     [DST + (1 * REGSZ)], zmm1
    vmovntdq     [DST + (2 * REGSZ)], zmm2
    vmovntdq     [DST + (3 * REGSZ)], zmm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_pf64_avx512
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
    vmovdqa64      zmm1,       [SRC + (1 * REGSZ)]
    vmovdqa64      zmm2,       [SRC + (2 * REGSZ)]
    vmovdqa64      zmm3,       [SRC + (3 * REGSZ)]
    vmovdqa64      [DST + (0 * REGSZ)], zmm0
    vmovdqa64      [DST + (1 * REGSZ)], zmm1
    vmovdqa64      [DST + (2 * REGSZ)], zmm2
    vmovdqa64      [DST + (3 * REGSZ)], zmm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_copy_nt_pf64_avx512
0:
    prefetchnta [SRC + PREFETCH_DISTANCE]
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
    vmovdqa64      zmm1,       [SRC + (1 * REGSZ)]
    vmovdqa64      zmm2,       [SRC + (2 * REGSZ)]
    vmovdqa64      zmm3,       [SRC + (3 * REGSZ)]
    vmovntdq     [DST + (0 * REGSZ)], zmm0
    vmovntdq     [DST + (1 * REGSZ)], zmm1
    vmovntdq     [DST + (2 * REGSZ)], zmm2
    vmovntdq     [DST + (3 * REGSZ)], zmm3
    add         SRC,        (4 * REGSZ)
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_fill_avx512
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
0:
    vmovdqa64      [DST + (0 * REGSZ)], zmm0
    vmovdqa64      [DST + (1 * REGSZ)], zmm0
    vmovdqa64      [DST + (2 * REGSZ)], zmm0
    vmovdqa64      [DST + (3 * REGSZ)], zmm0
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

asm_function aligned_block_fill_nt_avx512
    vmovdqa64      zmm0,       [SRC + (0 * REGSZ)]
0:
    vmovntdq     [DST + (0 * REGSZ)], zmm0
    vmovntdq     [DST + (1 * REGSZ)], zmm0
    vmovntdq     [DST + (2 * REGSZ)], zmm0
    vmovntdq     [DST + (3 * REGSZ)], zmm0
    add         DST,        (4 * REGSZ)
    sub         SIZE,       (4 * REGSZ)
    jg          0b
    ret
.endfunc

/*****************************************************************************/

#endif
