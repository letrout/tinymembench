/*
 * Copyright © 2011 Siarhei Siamashka <siarhei.siamashka@gmail.com>
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

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "util.h"
#include "asm-opt.h"
#include "version.h"

#define SIZE             (32 * 1024 * 1024)
#define BLOCKSIZE        2048
#define COUNT            16
#define MAXREPEATS       10

static double bandwidth_bench_helper(int64_t *dstbuf, int64_t *srcbuf,
                                     int64_t *tmpbuf,
                                     int size, int blocksize,
                                     const char *indent_prefix,
                                     int use_tmpbuf,
                                     void (*f)(int64_t *, int64_t *, int),
                                     const char *description)
{
    int i, j, loopcount, n;
    double t1, t2;
    double speed, maxspeed;
    double s, s0, s1, s2;

    /* do up to MAXREPEATS measurements */
    s0 = s1 = s2 = 0;
    maxspeed   = 0;
    for (n = 0; n < MAXREPEATS; n++)
    {
        f(dstbuf, srcbuf, size);
        loopcount = 0;
        t1 = gettime();
        do
        {
            loopcount++;
            if (use_tmpbuf)
            {
                for (i = 0; i < COUNT; i++)
                {
                    for (j = 0; j < size; j += blocksize)
                        {
                        f(tmpbuf, srcbuf + j / sizeof(int64_t), blocksize);
                        f(dstbuf + j / sizeof(int64_t), tmpbuf, blocksize);
                    }
                }
            }
            else
            {
                for (i = 0; i < COUNT; i++)
                {
                    f(dstbuf, srcbuf, size);
                }
            }
            t2 = gettime();
        } while (t2 - t1 < 0.5);
        speed = (double)size * COUNT * loopcount / (t2 - t1) / 1000000.;

        s0 += 1;
        s1 += speed;
        s2 += speed * speed;

        if (speed > maxspeed)
            maxspeed = speed;

        if (s0 > 2)
        {
            s = sqrt((s0 * s2 - s1 * s1) / (s0 * (s0 - 1)));
            if (s < maxspeed / 1000.)
                break;
        }
    }

    if (s / maxspeed * 100. >= 0.1)
    {
        printf("%s%-52s : %8.1f MB/s (%.1f%%)\n", indent_prefix, description,
                                               maxspeed, s / maxspeed * 100.);
    }
    else
    {
        printf("%s%-52s : %8.1f MB/s\n", indent_prefix, description, maxspeed);
    }
    return maxspeed;
}

void memcpy_wrapper(int64_t *dst, int64_t *src, int size)
{
    memcpy(dst, src, size);
}

void memset_wrapper(int64_t *dst, int64_t *src, int size)
{
    memset(dst, src[0], size);
}

void bandwidth_bench(int64_t *dstbuf, int64_t *srcbuf, int64_t *tmpbuf,
                     int size, int blocksize, const char *indent_prefix)
{
    bench_info *bi = get_asm_benchmarks();

    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 0,
                           aligned_block_copy_backwards,
                           "C copy backwards");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 0,
                           aligned_block_copy,
                           "C copy");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 0,
                           aligned_block_copy_pf32,
                           "C copy prefetched (32 bytes step)");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 0,
                           aligned_block_copy_pf64,
                           "C copy prefetched (64 bytes step)");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 1,
                           aligned_block_copy,
                           "C 2-pass copy");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 1,
                           aligned_block_copy_pf32,
                           "C 2-pass copy prefetched (32 bytes step)");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 1,
                           aligned_block_copy_pf64,
                           "C 2-pass copy prefetched (64 bytes step)");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 0,
                           aligned_block_fill,
                           "C fill");

    printf("%s---\n", indent_prefix);
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 0,
                           memcpy_wrapper,
                           "standard memcpy");
    bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                           indent_prefix, 0,
                           memset_wrapper,
                           "standard memset");

    if (bi->f)
        printf("%s---\n", indent_prefix);

    while (bi->f)
    {
        bandwidth_bench_helper(dstbuf, srcbuf, tmpbuf, size, blocksize,
                               indent_prefix, bi->use_tmpbuf,
                               bi->f,
                               bi->description);
        bi++;
    }

}

static void __attribute__((noinline)) random_read_test(char *zerobuffer,
                                                       int count, int nbits)
{
    uint32_t seed = 0;
    uintptr_t addrmask = (1 << nbits) - 1;
    uint32_t v;
    static volatile uint32_t dummy;

#ifdef __arm__
    uint32_t tmp;
    __asm__ volatile (
        "subs %[count], %[count],       #16\n"
        "blt  1f\n"
    "0:\n"
        "subs %[count], %[count],       #16\n"
    ".rept 16\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "and  %[v],     %[xFF],         %[seed],        lsr #16\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "and  %[tmp],   %[xFF00],       %[seed],        lsr #8\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "orr  %[v],     %[v],           %[tmp]\n"
        "and  %[tmp],   %[x7FFF0000],   %[seed]\n"
        "orr  %[v],     %[v],           %[tmp]\n"
        "and  %[v],     %[v],           %[addrmask]\n"
        "ldrb %[v],     [%[zerobuffer], %[v]]\n"
        "orr  %[seed],  %[seed],        %[v]\n"
    ".endr\n"
        "bge  0b\n"
    "1:\n"
        "add  %[count], %[count],       #16\n"
        : [count] "+&r" (count),
          [seed] "+&r" (seed), [v] "=&r" (v),
          [tmp] "=&r" (tmp)
        : [c1103515245] "r" (1103515245), [c12345] "r" (12345),
          [xFF00] "r" (0xFF00), [xFF] "r" (0xFF),
          [x7FFF0000] "r" (0x7FFF0000),
          [zerobuffer] "r" (zerobuffer),
          [addrmask] "r" (addrmask)
        : "cc");
#endif

    #define RANDOM_MEM_ACCESS()                 \
        seed = seed * 1103515245 + 12345;       \
        v = (seed >> 16) & 0xFF;                \
        seed = seed * 1103515245 + 12345;       \
        v |= (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v |= seed & 0x7FFF0000;                 \
        seed |= zerobuffer[v & addrmask];

    while (count >= 16) {
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        count -= 16;
    }
    while (count-- > 0) {
        RANDOM_MEM_ACCESS();
    }
    dummy = seed;
    #undef RANDOM_MEM_ACCESS
}

static void __attribute__((noinline)) random_dual_read_test(char *zerobuffer,
                                                            int count, int nbits)
{
    uint32_t seed = 0;
    uintptr_t addrmask = (1 << nbits) - 1;
    uint32_t v1, v2;
    static volatile uint32_t dummy;

#ifdef __arm__
    uint32_t tmp;
    __asm__ volatile (
        "subs %[count], %[count],       #16\n"
        "blt  1f\n"
    "0:\n"
        "subs %[count], %[count],       #16\n"
    ".rept 16\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "and  %[v1],    %[xFF00],       %[seed],        lsr #8\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "and  %[v2],    %[xFF00],       %[seed],        lsr #8\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "and  %[tmp],   %[x7FFF0000],   %[seed]\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "orr  %[v1],    %[v1],          %[tmp]\n"
        "and  %[tmp],   %[x7FFF0000],   %[seed]\n"
        "mla  %[seed],  %[seed],        %[c1103515245], %[c12345]\n"
        "orr  %[v2],    %[v2],          %[tmp]\n"
        "and  %[tmp],   %[xFF],         %[seed],        lsr #16\n"
        "orr  %[v2],    %[v2],          %[seed],        lsr #24\n"
        "orr  %[v1],    %[v1],          %[tmp]\n"
        "and  %[v2],    %[v2],          %[addrmask]\n"
        "eor  %[v1],    %[v1],          %[v2]\n"
        "and  %[v1],    %[v1],          %[addrmask]\n"
        "ldrb %[v2],    [%[zerobuffer], %[v2]]\n"
        "ldrb %[v1],    [%[zerobuffer], %[v1]]\n"
        "orr  %[seed],  %[seed],        %[v2]\n"
        "add  %[seed],  %[seed],        %[v1]\n"
    ".endr\n"
        "bge  0b\n"
    "1:\n"
        "add  %[count], %[count],       #16\n"
        : [count] "+&r" (count),
          [seed] "+&r" (seed), [v1] "=&r" (v1), [v2] "=&r" (v2),
          [tmp] "=&r" (tmp)
        : [c1103515245] "r" (1103515245), [c12345] "r" (12345),
          [xFF00] "r" (0xFF00), [xFF] "r" (0xFF),
          [x7FFF0000] "r" (0x7FFF0000),
          [zerobuffer] "r" (zerobuffer),
          [addrmask] "r" (addrmask)
        : "cc");
#endif

    #define RANDOM_MEM_ACCESS()                 \
        seed = seed * 1103515245 + 12345;       \
        v1 = (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v2 = (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v1 |= seed & 0x7FFF0000;                \
        seed = seed * 1103515245 + 12345;       \
        v2 |= seed & 0x7FFF0000;                \
        seed = seed * 1103515245 + 12345;       \
        v1 |= (seed >> 16) & 0xFF;              \
        v2 |= (seed >> 24);                     \
        v2 &= addrmask;                         \
        v1 ^= v2;                               \
        seed |= zerobuffer[v2];                 \
        seed += zerobuffer[v1 & addrmask];

    while (count >= 16) {
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        count -= 16;
    }
    while (count-- > 0) {
        RANDOM_MEM_ACCESS();
    }
    dummy = seed;
    #undef RANDOM_MEM_ACCESS
}

void latency_bench(int size, int count)
{
    double t, t2, t_before, t_after, t_noaccess, t_noaccess2;
    double xs, xs0, xs1, xs2;
    double ys, ys0, ys1, ys2;
    double min_t, min_t2;
    int nbits, n;
    char *buffer, *buffer_alloc;
    buffer_alloc = (char *)malloc(size + 4095);
    if (!buffer_alloc)
        return;
    buffer = (char *)(((uintptr_t)buffer_alloc + 4095) & ~(uintptr_t)4095);
    memset(buffer, 0, size);

    for (n = 1; n <= MAXREPEATS; n++)
    {
        t_before = gettime();
        random_read_test(buffer, count, 1);
        t_after = gettime();
        if (n == 1 || t_after - t_before < t_noaccess)
            t_noaccess = t_after - t_before;

        t_before = gettime();
        random_dual_read_test(buffer, count, 1);
        t_after = gettime();
        if (n == 1 || t_after - t_before < t_noaccess2)
            t_noaccess2 = t_after - t_before;
    }

    printf("block size : read access time (single random read / dual random read)\n");

    for (nbits = 1; (1 << nbits) <= size; nbits++)
    {
        xs1 = xs2 = ys = ys1 = ys2 = 0;
        for (n = 1; n <= MAXREPEATS; n++)
        {
            t_before = gettime();
            random_read_test(buffer, count, nbits);
            t_after = gettime();
            t = t_after - t_before - t_noaccess;
            if (t < 0) t = 0;

            xs1 += t;
            xs2 += t * t;

            if (n == 1 || t < min_t)
                min_t = t;

            t_before = gettime();
            random_dual_read_test(buffer, count, nbits);
            t_after = gettime();
            t2 = t_after - t_before - t_noaccess2;
            if (t2 < 0) t2 = 0;

            ys1 += t2;
            ys2 += t2 * t2;

            if (n == 1 || t2 < min_t2)
                min_t2 = t2;

            if (n > 2)
            {
                xs = sqrt((xs2 * n - xs1 * xs1) / (n * (n - 1)));
                ys = sqrt((ys2 * n - ys1 * ys1) / (n * (n - 1)));
                if (xs < min_t / 1000. && ys < min_t2 / 1000.)
                    break;
            }
        }
        printf("%10d : %6.1f ns  /  %6.1f ns \n", (1 << nbits),
            min_t * 1000000000. / count,  min_t2 * 1000000000. / count);
    }
    free(buffer_alloc);
}

int main(void)
{
    int64_t *srcbuf, *dstbuf, *tmpbuf;
    void *poolbuf;

    printf("ssvb-membench v" VERSION " (simple benchmark for memory throughput and latency)\n");

    poolbuf = alloc_four_nonaliased_buffers((void **)&srcbuf, SIZE,
                                            (void **)&dstbuf, SIZE,
                                            (void **)&tmpbuf, BLOCKSIZE,
                                            NULL, 0);
    printf("\n");
    printf("==========================================================================\n");
    printf("== Memory bandwidth tests                                               ==\n");
    printf("==                                                                      ==\n");
    printf("== Note 1: 1MB = 1000000 bytes                                          ==\n");
    printf("== Note 2: Results for 'copy' tests show how many bytes can be          ==\n");
    printf("==         copied per second (adding together read and writen           ==\n");
    printf("==         bytes would have provided twice higher numbers)              ==\n");
    printf("== Note 3: 2-pass copy means that we are using a small temporary buffer ==\n");
    printf("==         to first fetch data into it, and only then write it to the   ==\n");
    printf("==         destination (source -> L1 cache, L1 cache -> destination)    ==\n");
    printf("== Note 4: If sample standard deviation exceeds 0.1%%, it is shown in    ==\n");
    printf("==         brackets                                                     ==\n");
    printf("==========================================================================\n\n");
    bandwidth_bench(dstbuf, srcbuf, tmpbuf, SIZE, BLOCKSIZE, " ");
    free(poolbuf);

    printf("\n");
    printf("==========================================================================\n");
    printf("== Memory latency test                                                  ==\n");
    printf("==                                                                      ==\n");
    printf("== Average time is measured for random memory accesses in the buffers   ==\n");
    printf("== of different sizes. The larger is the buffer, the more significant   ==\n");
    printf("== are relative contributions of TLB, L1/L2 cache misses and SDRAM      ==\n");
    printf("== accesses. For extremely large buffer sizes we are expecting to see   ==\n");
    printf("== page table walk with total 3 requests to SDRAM for almost every      ==\n");
    printf("== memory access (though 64MiB is not large enough to experience this   ==\n");
    printf("== effect to its fullest).                                              ==\n");
    printf("==                                                                      ==\n");
    printf("== Note 1: All the numbers are representing extra time, which needs to  ==\n");
    printf("==         be added to L1 cache latency. The cycle timings for L1 cache ==\n");
    printf("==         latency can be usually found in the processor documentation. ==\n");
    printf("== Note 1: Dual random read means that we are simultaneously performing ==\n");
    printf("==         two independent memory accesses at a time. In the case if    ==\n");
    printf("==         the memory subsystem can't handle multiple outstanding       ==\n");
    printf("==         requests, dual random read has the same timings as two       ==\n");
    printf("==         single reads performed one after another.                    ==\n");
    printf("==========================================================================\n\n");

    latency_bench(SIZE * 2, 10000000);

    return 0;
}
