/*
 * Copyright © 2015 Kazunobu Kuriyama <kazunobu.kuriyama@nifty.com>
 * Copyright © 2015 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <limits.h>

enum { BENCH_MAX_N = ((UINT_MAX >> 2) + 1u) };

struct bench_time {
    long int seconds;
    long long int picoseconds;
};

struct bench {
    struct bench_time start;
    struct bench_time stop;
};

struct estimate {
    long long int elapsed; /* picoseconds */
    long long int stdev;   /* picoseconds */
};

void
bench_start(struct bench *bench);
void
bench_stop(struct bench *bench);

#ifndef _WIN32
void
bench_start2(struct bench *bench);
void
bench_stop2(struct bench *bench);
#else
/* TODO: implement clock_getres for Windows */
#define bench_start2 bench_start
#define bench_stop2  bench_stop
#endif

void
bench_elapsed(const struct bench *bench, struct bench_time *result);

#define bench_pico_to_micro(t) ((t) / 1000000)
#define bench_pico_to_nano(t)  ((t) / 1000)

#define bench_time_elapsed_microseconds(elapsed) \
    ((elapsed)->picoseconds / 1000000 + 1000000LL * (elapsed)->seconds)
#define bench_time_elapsed_nanoseconds(elapsed) \
    ((elapsed)->picoseconds / 1000 + 1000000000LL * (elapsed)->seconds)
#define bench_time_elapsed_picoseconds(elapsed) \
    ((elapsed)->picoseconds + 1000000000000LL * (elapsed)->seconds)

/* The caller is responsibile to free() the returned string. */
char *
bench_elapsed_str(const struct bench *bench);

/* Bench method adapted from: https://hackage.haskell.org/package/tasty-bench */
#define BENCH(target_stdev, n, time, est, pre, post, ...) do {              \
    struct bench _bench;                                                    \
    struct bench_time _t1;                                                  \
    struct bench_time _t2;                                                  \
    n = 1;                                                                  \
    pre;                                                                  \
    bench_start2(&_bench);                                                  \
    do { __VA_ARGS__ } while (0);                                           \
    bench_stop2(&_bench);                                                   \
    bench_elapsed(&_bench, &_t1);                                           \
    do {                                                                    \
        pre;                                                                \
        bench_start2(&_bench);                                              \
        for (unsigned int k = 0; k < 2 * n; k++) {                          \
            __VA_ARGS__                                                     \
        }                                                                   \
        bench_stop2(&_bench);                                               \
        post;                                                               \
        bench_elapsed(&_bench, &_t2);                                       \
        predictPerturbed(&_t1, &_t2, &est);                                 \
        if (est.stdev <                                                     \
            (long long)(MAX(0, target_stdev * (long double)est.elapsed)) || \
            n >= BENCH_MAX_N)                                               \
        {                                                                   \
            break;                                                          \
        }                                                                   \
        n *= 2;                                                             \
        _t1 = _t2;                                                          \
    } while (1);                                                            \
    scale_estimate(est, n);                                                 \
    time = _t2;                                                             \
    n *= 2;                                                                 \
} while (0)

void
predictPerturbed(const struct bench_time *t1, const struct bench_time *t2,
                 struct estimate *est);

#define scale_estimate(est, n) do { \
    (est).elapsed /= (n);           \
    (est).stdev /= (n);             \
} while (0)
