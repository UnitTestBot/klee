// It requires bitwuzla because the script currently runs with bitwuzla solver backend
// REQUIRES: bitwuzla
// REQUIRES: target-x86_64
// RUN: %kleef --property-file=%S/coverage-branches.prp --max-memory=7000000000 --max-cputime-soft=900 --32 %s 2>&1 | FileCheck %s

// CHECK: KLEE: done: generated tests = 5
// This file is part of the SV-Benchmarks collection of verification tasks:
// https://gitlab.com/sosy-lab/benchmarking/sv-benchmarks
//
// SPDX-FileCopyrightText: 2023 Edoardo Manino
//
// SPDX-License-Identifier: MIT

float __VERIFIER_nondet_float();
extern void __assert_fail (const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern void __assert_perror_fail (int __errnum, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern void __assert (const char *__assertion, const char *__file, int __line)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));

void reach_error() {
 ((void) sizeof ((0) ? 1 : 0), __extension__ ({ if (0) ; else __assert_fail ("0", "main.c", 4, __extension__ __PRETTY_FUNCTION__); }));
}
typedef unsigned int size_t;
typedef long int wchar_t;

typedef struct
  {
    int quot;
    int rem;
  } div_t;
typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;
__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ , __leaf__)) ;
extern double atof (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;
extern int atoi (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;
extern long int atol (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;
__extension__ extern long long int atoll (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;
extern double strtod (const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern float strtof (const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern long double strtold (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern long int strtol (const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern unsigned long int strtoul (const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
__extension__
extern long long int strtoq (const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
__extension__
extern long long int strtoll (const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern char *l64a (long int __n) __attribute__ ((__nothrow__ , __leaf__)) ;
extern long int a64l (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;
typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;
__extension__ typedef long long int __quad_t;
__extension__ typedef unsigned long long int __u_quad_t;
__extension__ typedef long long int __intmax_t;
__extension__ typedef unsigned long long int __uintmax_t;
__extension__ typedef __uint64_t __dev_t;
__extension__ typedef unsigned int __uid_t;
__extension__ typedef unsigned int __gid_t;
__extension__ typedef unsigned long int __ino_t;
__extension__ typedef __uint64_t __ino64_t;
__extension__ typedef unsigned int __mode_t;
__extension__ typedef unsigned int __nlink_t;
__extension__ typedef long int __off_t;
__extension__ typedef __int64_t __off64_t;
__extension__ typedef int __pid_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
__extension__ typedef long int __clock_t;
__extension__ typedef unsigned long int __rlim_t;
__extension__ typedef __uint64_t __rlim64_t;
__extension__ typedef unsigned int __id_t;
__extension__ typedef long int __time_t;
__extension__ typedef unsigned int __useconds_t;
__extension__ typedef long int __suseconds_t;
__extension__ typedef __int64_t __suseconds64_t;
__extension__ typedef int __daddr_t;
__extension__ typedef int __key_t;
__extension__ typedef int __clockid_t;
__extension__ typedef void * __timer_t;
__extension__ typedef long int __blksize_t;
__extension__ typedef long int __blkcnt_t;
__extension__ typedef __int64_t __blkcnt64_t;
__extension__ typedef unsigned long int __fsblkcnt_t;
__extension__ typedef __uint64_t __fsblkcnt64_t;
__extension__ typedef unsigned long int __fsfilcnt_t;
__extension__ typedef __uint64_t __fsfilcnt64_t;
__extension__ typedef int __fsword_t;
__extension__ typedef int __ssize_t;
__extension__ typedef long int __syscall_slong_t;
__extension__ typedef unsigned long int __syscall_ulong_t;
typedef __off64_t __loff_t;
typedef char *__caddr_t;
__extension__ typedef int __intptr_t;
__extension__ typedef unsigned int __socklen_t;
typedef int __sig_atomic_t;
__extension__ typedef __int64_t __time64_t;
typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;
typedef __loff_t loff_t;
typedef __ino_t ino_t;
typedef __dev_t dev_t;
typedef __gid_t gid_t;
typedef __mode_t mode_t;
typedef __nlink_t nlink_t;
typedef __uid_t uid_t;
typedef __off_t off_t;
typedef __pid_t pid_t;
typedef __id_t id_t;
typedef __ssize_t ssize_t;
typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;
typedef __key_t key_t;
typedef __clock_t clock_t;
typedef __clockid_t clockid_t;
typedef __time_t time_t;
typedef __timer_t timer_t;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;
typedef int register_t __attribute__ ((__mode__ (__word__)));
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{
  return __builtin_bswap16 (__bsx);
}
static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{
  return __builtin_bswap32 (__bsx);
}
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{
  return __builtin_bswap64 (__bsx);
}
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}
static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}
static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
typedef __sigset_t sigset_t;
struct timeval
{
  __time_t tv_sec;
  __suseconds_t tv_usec;
};
struct timespec
{
  __time_t tv_sec;
  __syscall_slong_t tv_nsec;
};
typedef __suseconds_t suseconds_t;
typedef long int __fd_mask;
typedef struct
  {
    __fd_mask __fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];
  } fd_set;
typedef __fd_mask fd_mask;

extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);

typedef __blksize_t blksize_t;
typedef __blkcnt_t blkcnt_t;
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;
typedef union
{
  __extension__ unsigned long long int __value64;
  struct
  {
    unsigned int __low;
    unsigned int __high;
  } __value32;
} __atomic_wide_counter;
typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;
typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
struct __pthread_mutex_s
{
  int __lock;
  unsigned int __count;
  int __owner;
  int __kind;
  unsigned int __nusers;
  __extension__ union
  {
    struct
    {
      short __espins;
      short __eelision;
    } __elision_data;
    __pthread_slist_t __list;
  };
};
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;
  unsigned char __flags;
  unsigned char __shared;
  signed char __rwelision;
  unsigned char __pad2;
  int __cur_writer;
};
struct __pthread_cond_s
{
  __atomic_wide_counter __wseq;
  __atomic_wide_counter __g1_start;
  unsigned int __g_refs[2] ;
  unsigned int __g_size[2];
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
};
typedef unsigned int __tss_t;
typedef unsigned long int __thrd_t;
typedef struct
{
  int __data ;
} __once_flag;
typedef unsigned long int pthread_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;
typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
union pthread_attr_t
{
  char __size[36];
  long int __align;
};
typedef union pthread_attr_t pthread_attr_t;
typedef union
{
  struct __pthread_mutex_s __data;
  char __size[24];
  long int __align;
} pthread_mutex_t;
typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;
typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[32];
  long int __align;
} pthread_rwlock_t;
typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;
typedef volatile int pthread_spinlock_t;
typedef union
{
  char __size[20];
  long int __align;
} pthread_barrier_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;

extern long int random (void) __attribute__ ((__nothrow__ , __leaf__));
extern void srandom (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));
extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
extern char *setstate (char *__statebuf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };
extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));
extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int rand (void) __attribute__ ((__nothrow__ , __leaf__));
extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));
extern int rand_r (unsigned int *__seed) __attribute__ ((__nothrow__ , __leaf__));
extern double drand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern double erand48 (unsigned short int __xsubi[3]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern long int lrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int nrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern long int mrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int jrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern void srand48 (long int __seedval) __attribute__ ((__nothrow__ , __leaf__));
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    __extension__ unsigned long long int __a;
  };
extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern void *malloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;
extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_size__ (1, 2))) ;
extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__)) __attribute__ ((__alloc_size__ (2)));
extern void free (void *__ptr) __attribute__ ((__nothrow__ , __leaf__));
extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__))
     __attribute__ ((__alloc_size__ (2, 3)))
     ;
extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     ;

extern void *alloca (size_t __size) __attribute__ ((__nothrow__ , __leaf__));

extern void *valloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;
extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
extern void *aligned_alloc (size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_align__ (1)))
     __attribute__ ((__alloc_size__ (2))) ;
extern void abort (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int at_quick_exit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern void exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern void quick_exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern void _Exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern char *getenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
extern int putenv (char *__string) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int setenv (const char *__name, const char *__value, int __replace)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
extern int unsetenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int clearenv (void) __attribute__ ((__nothrow__ , __leaf__));
extern char *mktemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int mkstemp (char *__template) __attribute__ ((__nonnull__ (1))) ;
extern int mkstemps (char *__template, int __suffixlen) __attribute__ ((__nonnull__ (1))) ;
extern char *mkdtemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
extern int system (const char *__command) ;
extern char *realpath (const char *__restrict __name,
         char *__restrict __resolved) __attribute__ ((__nothrow__ , __leaf__)) ;
typedef int (*__compar_fn_t) (const void *, const void *);
extern void *bsearch (const void *__key, const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;
extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));
extern int abs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *gcvt (double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;
extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;
extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int mblen (const char *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));
extern int mbtowc (wchar_t *__restrict __pwc,
     const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));
extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ , __leaf__));
extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__read_only__, 2)));
extern size_t wcstombs (char *__restrict __s,
   const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1, 3)))
  __attribute__ ((__access__ (__read_only__, 2)));
extern int rpmatch (const char *__response) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
extern int getsubopt (char **__restrict __optionp,
        char *const *__restrict __tokens,
        char **__restrict __valuep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2, 3))) ;
extern int getloadavg (double __loadavg[], int __nelem)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


typedef long double float_t;
typedef long double double_t;
extern int __fpclassify (double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
extern int __signbit (double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
extern int __isinf (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __finite (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __isnan (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __iseqsig (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
extern int __issignaling (double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
 extern double acos (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __acos (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double asin (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __asin (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double atan (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atan (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double atan2 (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atan2 (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double cos (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __cos (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double sin (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __sin (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double tan (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __tan (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double cosh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __cosh (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double sinh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __sinh (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double tanh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __tanh (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double acosh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __acosh (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double asinh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __asinh (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double atanh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atanh (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double exp (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __exp (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double frexp (double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__)); extern double __frexp (double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__));
extern double ldexp (double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__)); extern double __ldexp (double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__));
 extern double log (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double log10 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log10 (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double modf (double __x, double *__iptr) __attribute__ ((__nothrow__ , __leaf__)); extern double __modf (double __x, double *__iptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
 extern double expm1 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __expm1 (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double log1p (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log1p (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double logb (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __logb (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double exp2 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __exp2 (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double log2 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log2 (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double pow (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __pow (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
extern double sqrt (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __sqrt (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double hypot (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __hypot (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
 extern double cbrt (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __cbrt (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double ceil (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __ceil (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double fabs (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __fabs (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double floor (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __floor (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double fmod (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __fmod (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
extern int isinf (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int finite (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern double drem (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __drem (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
extern double significand (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __significand (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double copysign (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __copysign (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double nan (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern double __nan (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
extern int isnan (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern double j0 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __j0 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double j1 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __j1 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double jn (int, double) __attribute__ ((__nothrow__ , __leaf__)); extern double __jn (int, double) __attribute__ ((__nothrow__ , __leaf__));
extern double y0 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __y0 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double y1 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __y1 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double yn (int, double) __attribute__ ((__nothrow__ , __leaf__)); extern double __yn (int, double) __attribute__ ((__nothrow__ , __leaf__));
 extern double erf (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __erf (double) __attribute__ ((__nothrow__ , __leaf__));
 extern double erfc (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __erfc (double) __attribute__ ((__nothrow__ , __leaf__));
extern double lgamma (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __lgamma (double) __attribute__ ((__nothrow__ , __leaf__));
extern double tgamma (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __tgamma (double) __attribute__ ((__nothrow__ , __leaf__));
extern double gamma (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __gamma (double) __attribute__ ((__nothrow__ , __leaf__));
extern double lgamma_r (double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__)); extern double __lgamma_r (double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__));
extern double rint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __rint (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double nextafter (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __nextafter (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
extern double nexttoward (double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __nexttoward (double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern double remainder (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __remainder (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
extern double scalbn (double __x, int __n) __attribute__ ((__nothrow__ , __leaf__)); extern double __scalbn (double __x, int __n) __attribute__ ((__nothrow__ , __leaf__));
extern int ilogb (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern int __ilogb (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double scalbln (double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__)); extern double __scalbln (double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__));
extern double nearbyint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __nearbyint (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double round (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __round (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double trunc (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __trunc (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double remquo (double __x, double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__)); extern double __remquo (double __x, double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__));
extern long int lrint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lrint (double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llrint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llrint (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long int lround (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lround (double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llround (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llround (double __x) __attribute__ ((__nothrow__ , __leaf__));
extern double fdim (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __fdim (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
extern double fmax (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __fmax (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double fmin (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern double __fmin (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern double fma (double __x, double __y, double __z) __attribute__ ((__nothrow__ , __leaf__)); extern double __fma (double __x, double __y, double __z) __attribute__ ((__nothrow__ , __leaf__));
extern double scalb (double __x, double __n) __attribute__ ((__nothrow__ , __leaf__)); extern double __scalb (double __x, double __n) __attribute__ ((__nothrow__ , __leaf__));
extern int __fpclassifyf (float __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
extern int __signbitf (float __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
extern int __isinff (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __finitef (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __isnanf (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __iseqsigf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
extern int __issignalingf (float __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
 extern float acosf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __acosf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float asinf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __asinf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float atanf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atanf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float atan2f (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atan2f (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float cosf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __cosf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float sinf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __sinf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float tanf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __tanf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float coshf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __coshf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float sinhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __sinhf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float tanhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __tanhf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float acoshf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __acoshf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float asinhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __asinhf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float atanhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atanhf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float expf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __expf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float frexpf (float __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__)); extern float __frexpf (float __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__));
extern float ldexpf (float __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__)); extern float __ldexpf (float __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__));
 extern float logf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __logf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float log10f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log10f (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float modff (float __x, float *__iptr) __attribute__ ((__nothrow__ , __leaf__)); extern float __modff (float __x, float *__iptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
 extern float expm1f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __expm1f (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float log1pf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log1pf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float logbf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __logbf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float exp2f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __exp2f (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float log2f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log2f (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float powf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __powf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
extern float sqrtf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __sqrtf (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float hypotf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __hypotf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
 extern float cbrtf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __cbrtf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float ceilf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __ceilf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float fabsf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __fabsf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float floorf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __floorf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float fmodf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __fmodf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
extern int isinff (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int finitef (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern float dremf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __dremf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
extern float significandf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __significandf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float copysignf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __copysignf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float nanf (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern float __nanf (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
extern int isnanf (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern float j0f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __j0f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float j1f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __j1f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float jnf (int, float) __attribute__ ((__nothrow__ , __leaf__)); extern float __jnf (int, float) __attribute__ ((__nothrow__ , __leaf__));
extern float y0f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __y0f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float y1f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __y1f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float ynf (int, float) __attribute__ ((__nothrow__ , __leaf__)); extern float __ynf (int, float) __attribute__ ((__nothrow__ , __leaf__));
 extern float erff (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __erff (float) __attribute__ ((__nothrow__ , __leaf__));
 extern float erfcf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __erfcf (float) __attribute__ ((__nothrow__ , __leaf__));
extern float lgammaf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __lgammaf (float) __attribute__ ((__nothrow__ , __leaf__));
extern float tgammaf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __tgammaf (float) __attribute__ ((__nothrow__ , __leaf__));
extern float gammaf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __gammaf (float) __attribute__ ((__nothrow__ , __leaf__));
extern float lgammaf_r (float, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__)); extern float __lgammaf_r (float, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__));
extern float rintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __rintf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float nextafterf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __nextafterf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
extern float nexttowardf (float __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __nexttowardf (float __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern float remainderf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __remainderf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
extern float scalbnf (float __x, int __n) __attribute__ ((__nothrow__ , __leaf__)); extern float __scalbnf (float __x, int __n) __attribute__ ((__nothrow__ , __leaf__));
extern int ilogbf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern int __ilogbf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float scalblnf (float __x, long int __n) __attribute__ ((__nothrow__ , __leaf__)); extern float __scalblnf (float __x, long int __n) __attribute__ ((__nothrow__ , __leaf__));
extern float nearbyintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __nearbyintf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float roundf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __roundf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float truncf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __truncf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float remquof (float __x, float __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__)); extern float __remquof (float __x, float __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__));
extern long int lrintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lrintf (float __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llrintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llrintf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern long int lroundf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lroundf (float __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llroundf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llroundf (float __x) __attribute__ ((__nothrow__ , __leaf__));
extern float fdimf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __fdimf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
extern float fmaxf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __fmaxf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float fminf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern float __fminf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern float fmaf (float __x, float __y, float __z) __attribute__ ((__nothrow__ , __leaf__)); extern float __fmaf (float __x, float __y, float __z) __attribute__ ((__nothrow__ , __leaf__));
extern float scalbf (float __x, float __n) __attribute__ ((__nothrow__ , __leaf__)); extern float __scalbf (float __x, float __n) __attribute__ ((__nothrow__ , __leaf__));
extern int __fpclassifyl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
extern int __signbitl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
extern int __isinfl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __finitel (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __isnanl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int __iseqsigl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern int __issignalingl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
 extern long double acosl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __acosl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double asinl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __asinl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double atanl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atanl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double atan2l (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atan2l (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double cosl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __cosl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double sinl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __sinl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double tanl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __tanl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double coshl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __coshl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double sinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __sinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double tanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __tanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double acoshl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __acoshl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double asinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __asinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double atanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double expl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __expl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double frexpl (long double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__)); extern long double __frexpl (long double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__));
extern long double ldexpl (long double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__)); extern long double __ldexpl (long double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__));
 extern long double logl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __logl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double log10l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log10l (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double modfl (long double __x, long double *__iptr) __attribute__ ((__nothrow__ , __leaf__)); extern long double __modfl (long double __x, long double *__iptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
 extern long double expm1l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __expm1l (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double log1pl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log1pl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double logbl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __logbl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double exp2l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __exp2l (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double log2l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log2l (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double powl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __powl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern long double sqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __sqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double hypotl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __hypotl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
 extern long double cbrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __cbrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double ceill (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __ceill (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double fabsl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __fabsl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double floorl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __floorl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double fmodl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fmodl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern int isinfl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern int finitel (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern long double dreml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __dreml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern long double significandl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __significandl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double copysignl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __copysignl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double nanl (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nanl (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
extern int isnanl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));
extern long double j0l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __j0l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double j1l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __j1l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double jnl (int, long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __jnl (int, long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double y0l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __y0l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double y1l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __y1l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double ynl (int, long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __ynl (int, long double) __attribute__ ((__nothrow__ , __leaf__));
 extern long double erfl (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __erfl (long double) __attribute__ ((__nothrow__ , __leaf__));
 extern long double erfcl (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __erfcl (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double lgammal (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __lgammal (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double tgammal (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __tgammal (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double gammal (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __gammal (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double lgammal_r (long double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__)); extern long double __lgammal_r (long double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__));
extern long double rintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __rintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double nextafterl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nextafterl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern long double nexttowardl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nexttowardl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern long double remainderl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __remainderl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern long double scalbnl (long double __x, int __n) __attribute__ ((__nothrow__ , __leaf__)); extern long double __scalbnl (long double __x, int __n) __attribute__ ((__nothrow__ , __leaf__));
extern int ilogbl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern int __ilogbl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double scalblnl (long double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__)); extern long double __scalblnl (long double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__));
extern long double nearbyintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nearbyintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double roundl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __roundl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double truncl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __truncl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double remquol (long double __x, long double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__)); extern long double __remquol (long double __x, long double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__));
extern long int lrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long int lroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
extern long double fdiml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fdiml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
extern long double fmaxl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __fmaxl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double fminl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)); extern long double __fminl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern long double fmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__));
extern long double scalbl (long double __x, long double __n) __attribute__ ((__nothrow__ , __leaf__)); extern long double __scalbl (long double __x, long double __n) __attribute__ ((__nothrow__ , __leaf__));
 
 
 
 
 
 
 
extern int signgam;
enum
  {
    FP_NAN =
      0,
    FP_INFINITE =
      1,
    FP_ZERO =
      2,
    FP_SUBNORMAL =
      3,
    FP_NORMAL =
      4
  };

void entry(const float tensor_input[1][4], float tensor_output[1][2]);
int main()
{
 float tensor_input[1][4];
 float tensor_output[1][2];
 tensor_input[0][0] = __VERIFIER_nondet_float();
 tensor_input[0][1] = __VERIFIER_nondet_float();
 tensor_input[0][2] = __VERIFIER_nondet_float();
 tensor_input[0][3] = __VERIFIER_nondet_float();
 if(!(tensor_input[0][0] >= 0.05381735414854336f && tensor_input[0][0] <= 0.14946724585145665f)) abort();
 if(!(tensor_input[0][1] >= 0.9329833541485433f && tensor_input[0][1] <= 1.0286332458514567f)) abort();
 if(!(tensor_input[0][2] >= -0.20433929585145663f && tensor_input[0][2] <= -0.10868940414854336f)) abort();
 if(!(tensor_input[0][3] >= -1.6417829458514566f && tensor_input[0][3] <= -1.5461330541485434f)) abort();
 entry(tensor_input, tensor_output);
 if(!(!((tensor_output[0][0] <= tensor_output[0][1])))) reach_error();
 return 0;
}
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;
typedef __int_least8_t int_least8_t;
typedef __int_least16_t int_least16_t;
typedef __int_least32_t int_least32_t;
typedef __int_least64_t int_least64_t;
typedef __uint_least8_t uint_least8_t;
typedef __uint_least16_t uint_least16_t;
typedef __uint_least32_t uint_least32_t;
typedef __uint_least64_t uint_least64_t;
typedef signed char int_fast8_t;
typedef int int_fast16_t;
typedef int int_fast32_t;
__extension__
typedef long long int int_fast64_t;
typedef unsigned char uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned int uint_fast32_t;
__extension__
typedef unsigned long long int uint_fast64_t;
typedef int intptr_t;
typedef unsigned int uintptr_t;
typedef __intmax_t intmax_t;
typedef __uintmax_t uintmax_t;

extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
       size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern void *memmove (void *__dest, const void *__src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern void *memccpy (void *__restrict __dest, const void *__restrict __src,
        int __c, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__write_only__, 1, 4)));
extern void *memset (void *__s, int __c, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int memcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern int __memcmpeq (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern void *memchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
extern char *strcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strcat (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strncat (char *__restrict __dest, const char *__restrict __src,
        size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int strcmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern int strncmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern int strcoll (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern size_t strxfrm (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 1, 3)));
struct __locale_struct
{
  struct __locale_data *__locales[13];
  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;
  const char *__names[13];
};
typedef struct __locale_struct *__locale_t;
typedef __locale_t locale_t;
extern int strcoll_l (const char *__s1, const char *__s2, locale_t __l)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));
extern size_t strxfrm_l (char *__dest, const char *__src, size_t __n,
    locale_t __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)))
     __attribute__ ((__access__ (__write_only__, 1, 3)));
extern char *strdup (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
extern char *strndup (const char *__string, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
extern char *strchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
extern char *strrchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
extern size_t strcspn (const char *__s, const char *__reject)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern size_t strspn (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strpbrk (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strstr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strtok (char *__restrict __s, const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
extern char *__strtok_r (char *__restrict __s,
    const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
         char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
extern size_t strlen (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
extern size_t strnlen (const char *__string, size_t __maxlen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
extern char *strerror (int __errnum) __attribute__ ((__nothrow__ , __leaf__));
extern int strerror_r (int __errnum, char *__buf, size_t __buflen) __asm__ ("" "__xpg_strerror_r") __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)))
    __attribute__ ((__access__ (__write_only__, 2, 3)));
extern char *strerror_l (int __errnum, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));

extern int bcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern void bcopy (const void *__src, void *__dest, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern void bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern char *index (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
extern char *rindex (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
extern int ffs (int __i) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern int ffsl (long int __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__ extern int ffsll (long long int __ll)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern int strcasecmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern int strncasecmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
extern int strcasecmp_l (const char *__s1, const char *__s2, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));
extern int strncasecmp_l (const char *__s1, const char *__s2,
     size_t __n, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 4)));

extern void explicit_bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));
extern char *strsep (char **__restrict __stringp,
       const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strsignal (int __sig) __attribute__ ((__nothrow__ , __leaf__));
extern char *__stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *__stpncpy (char *__restrict __dest,
   const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

static const float tensor_q_net_0_weight[64][4] =
{
  {0.15721032023429870605f, 0.11475459486246109009f, -0.63993811607360839844f, -0.069510847330093383789f},
  {0.17542746663093566895f, 0.037346746772527694702f, 0.11804900318384170532f, -0.42608326673507690430f},
  {0.17451041936874389648f, -0.038390919566154479980f, -0.12378698587417602539f, 0.16069194674491882324f},
  {-0.27305021882057189941f, -0.085580311715602874756f, 0.75955444574356079102f, 0.51875507831573486328f},
  {-0.36573705077171325684f, -0.24695795774459838867f, 0.54157340526580810547f, -0.10584691166877746582f},
  {-0.065357878804206848145f, -0.29461735486984252930f, -0.86868923902511596680f, -0.42447930574417114258f},
  {-0.16696630418300628662f, -0.44955989718437194824f, -0.46123439073562622070f, -0.020981112495064735413f},
  {-0.66190141439437866211f, -0.23531131446361541748f, 0.62862229347229003906f, -0.27901083230972290039f},
  {0.17868228256702423096f, 0.35911080241203308105f, 0.14576785266399383545f, -0.39424306154251098633f},
  {-0.65974438190460205078f, 0.15513201057910919189f, 0.15558756887912750244f, -0.16923116147518157959f},
  {-0.060384672135114669800f, 0.48458999395370483398f, -0.49716308712959289551f, -0.31139120459556579590f},
  {-0.19236545264720916748f, 0.017238076776266098022f, 0.31643331050872802734f, 0.11882077902555465698f},
  {0.077491581439971923828f, -0.45291128754615783691f, -0.24384044110774993896f, -0.17995877563953399658f},
  {0.49704143404960632324f, 0.082509785890579223633f, -0.20520922541618347168f, -0.29227843880653381348f},
  {-0.52873462438583374023f, 0.022500123828649520874f, -0.080530084669589996338f, 0.47344842553138732910f},
  {-0.35248750448226928711f, -0.16583487391471862793f, -0.013518642634153366089f, 0.023007424548268318176f},
  {-0.42581608891487121582f, 0.052996642887592315674f, 0.39690676331520080566f, -0.26001733541488647461f},
  {0.33361932635307312012f, 0.29740148782730102539f, 0.56780290603637695312f, 0.19177463650703430176f},
  {-0.11368326097726821899f, -0.27305486798286437988f, 0.26973405480384826660f, 0.23418289422988891602f},
  {0.075950942933559417725f, 0.18417342007160186768f, 0.61009067296981811523f, 0.26281380653381347656f},
  {-0.39552086591720581055f, -0.042891699820756912231f, -0.21572166681289672852f, -0.058940868824720382690f},
  {-0.57776516675949096680f, 0.47246569395065307617f, -0.25832870602607727051f, 0.36092242598533630371f},
  {0.10012030601501464844f, -0.18621048331260681152f, -0.22833108901977539062f, 0.031087309122085571289f},
  {0.38490217924118041992f, -0.49956321716308593750f, -0.47275030612945556641f, 0.077935919165611267090f},
  {0.30921569466590881348f, -0.42641988396644592285f, -0.40230366587638854980f, -0.40324175357818603516f},
  {0.63655608892440795898f, 0.21062850952148437500f, -0.65637254714965820312f, 0.46128243207931518555f},
  {-0.38448631763458251953f, 0.10538971424102783203f, 0.49003174901008605957f, 0.052914798259735107422f},
  {-0.42741405963897705078f, 0.37639018893241882324f, 0.59647685289382934570f, -0.45300826430320739746f},
  {0.27839130163192749023f, -0.15608096122741699219f, -0.47896218299865722656f, -0.43156555294990539551f},
  {-0.53123921155929565430f, 0.49752473831176757812f, 0.032899670302867889404f, 0.10219132900238037109f},
  {-0.13477079570293426514f, -0.38954326510429382324f, -0.43758767843246459961f, -0.27198565006256103516f},
  {0.19732491672039031982f, -0.35278046131134033203f, -0.10187369585037231445f, -0.27081882953643798828f},
  {-0.042072765529155731201f, 0.44140005111694335938f, -0.13818964362144470215f, -0.18287837505340576172f},
  {-0.34508317708969116211f, 0.47647181153297424316f, 0.24145375192165374756f, 0.40118521451950073242f},
  {-0.30380111932754516602f, -0.29507231712341308594f, -0.025434054434299468994f, 0.32625019550323486328f},
  {-0.27462953329086303711f, 0.00089584459783509373665f, -0.17577506601810455322f, 0.11180936545133590698f},
  {0.63477182388305664062f, -0.13386686146259307861f, -0.64766478538513183594f, -0.44009038805961608887f},
  {-0.12738856673240661621f, 0.48643681406974792480f, -0.17503669857978820801f, 0.39376839995384216309f},
  {0.68278217315673828125f, 0.018378410488367080688f, -0.16910094022750854492f, -0.50995582342147827148f},
  {-0.49679356813430786133f, 0.026661410927772521973f, 0.13810670375823974609f, 0.53860598802566528320f},
  {-0.0053787906654179096222f, -0.14719724655151367188f, 0.36476159095764160156f, -0.063192754983901977539f},
  {-0.10614252090454101562f, 0.48463654518127441406f, 0.34106862545013427734f, -0.45423215627670288086f},
  {0.31066700816154479980f, -0.0033910141792148351669f, 0.30815017223358154297f, 0.30849862098693847656f},
  {0.17261597514152526855f, 0.48857432603836059570f, 0.30169779062271118164f, -0.060204260051250457764f},
  {0.31123095750808715820f, 0.33437609672546386719f, 0.12725287675857543945f, -0.38050138950347900391f},
  {0.55887973308563232422f, 0.45446309447288513184f, -0.54596185684204101562f, -0.21886797249317169189f},
  {0.31797805428504943848f, 0.40895953774452209473f, 0.59627884626388549805f, 0.23894858360290527344f},
  {0.30402877926826477051f, -0.44058898091316223145f, 0.14562137424945831299f, -0.17414566874504089355f},
  {-0.27986198663711547852f, 0.15126775205135345459f, -0.022729687392711639404f, 0.25112929940223693848f},
  {-0.10832308977842330933f, 0.23695550858974456787f, 0.73702901601791381836f, -0.36840069293975830078f},
  {-0.27382770180702209473f, 0.26596125960350036621f, 0.20365367829799652100f, 0.31078708171844482422f},
  {0.28197550773620605469f, 0.46804508566856384277f, 0.56256306171417236328f, 0.17713961005210876465f},
  {0.29725003242492675781f, -0.15884678065776824951f, -0.060288373380899429321f, 0.065528862178325653076f},
  {0.60779553651809692383f, -0.12328117340803146362f, -0.69134706258773803711f, -0.40284743905067443848f},
  {0.48629778623580932617f, -0.31832030415534973145f, -0.62152892351150512695f, 0.34267014265060424805f},
  {-0.67640513181686401367f, 0.28546535968780517578f, 0.55224311351776123047f, 0.45257028937339782715f},
  {-0.48299044370651245117f, -0.48568195104598999023f, 0.52203935384750366211f, -0.19417253136634826660f},
  {0.13328588008880615234f, -0.32945892214775085449f, 0.41532999277114868164f, 0.18933498859405517578f},
  {-0.44472277164459228516f, -0.35175630450248718262f, 0.25645589828491210938f, 0.41243952512741088867f},
  {0.51961445808410644531f, -0.53158932924270629883f, 0.18742810189723968506f, 0.33608660101890563965f},
  {-0.28276044130325317383f, 0.30912891030311584473f, 0.35654854774475097656f, -0.36720177531242370605f},
  {-0.19587863981723785400f, -0.25889432430267333984f, -0.13906119763851165771f, 0.37115162611007690430f},
  {-0.30637884140014648438f, 0.053323369473218917847f, 0.41713884472846984863f, -0.057852618396282196045f},
  {0.11183704435825347900f, -0.091638505458831787109f, -0.62372940778732299805f, -0.40521082282066345215f}
};
static const float tensor_q_net_0_bias[64] =
{-0.038480948656797409058f, 0.12814424932003021240f, 0.49260348081588745117f, -0.30465084314346313477f, 0.35254269838333129883f, -0.26110908389091491699f, 0.20527216792106628418f, 0.10104730725288391113f, 0.57948935031890869141f, 0.17996396124362945557f, -0.21129722893238067627f, 0.43121445178985595703f, 0.40312266349792480469f, -0.48459199070930480957f, -0.45620569586753845215f, 0.44695967435836791992f, 0.44353476166725158691f, 0.17125770449638366699f, -0.25991657376289367676f, -0.053610563278198242188f, 0.51717704534530639648f, 0.50246846675872802734f, 0.38229703903198242188f, 0.33439946174621582031f, -0.17707166075706481934f, 0.21073415875434875488f, -0.26519128680229187012f, -0.016289036720991134644f, 0.034695561975240707397f, 0.087283737957477569580f, 0.24151174724102020264f, 0.53101652860641479492f, -0.32516640424728393555f, 0.39164531230926513672f, 0.46727159619331359863f, 0.34065410494804382324f, -0.18368704617023468018f, 0.24994476139545440674f, -0.22628302872180938721f, -0.023869842290878295898f, 0.51308518648147583008f, 0.36233329772949218750f, -0.32683113217353820801f, -0.030779188498854637146f, 0.32206705212593078613f, -0.53335618972778320312f, 0.38285782933235168457f, 0.35463356971740722656f, -0.17398750782012939453f, 0.45185545086860656738f, -0.11234466731548309326f, 0.57393497228622436523f, 0.43061342835426330566f, -0.29271811246871948242f, 0.40734162926673889160f, -0.26300087571144104004f, -0.17399179935455322266f, -0.44128191471099853516f, -0.40234717726707458496f, 0.33810994029045104980f, 0.32970127463340759277f, 0.27349570393562316895f, 0.20511500537395477295f, 0.081185646355152130127f};
static const float tensor_q_net_2_weight[64][64] =
{
  {-0.081372834742069244385f, 0.024248756468296051025f, 0.085219748318195343018f, -0.020931074395775794983f, 0.092145785689353942871f, 0.051900878548622131348f, -0.091962411999702453613f, 0.012122990563511848450f, -0.096147723495960235596f, 0.11863323301076889038f, -0.034693796187639236450f, -0.066232405602931976318f, -0.15095156431198120117f, -0.17523497343063354492f, 0.11800386011600494385f, 0.016400158405303955078f, 0.077730171382427215576f, 0.085327595472335815430f, 0.098276004195213317871f, 0.22289833426475524902f, 0.018943466246128082275f, 0.088348664343357086182f, 0.0055694859474897384644f, 0.020647743716835975647f, 0.083886988461017608643f, -0.12078075855970382690f, -0.097537018358707427979f, 0.055285315960645675659f, -0.015381305478513240814f, 0.12187845259904861450f, -0.19759547710418701172f, 0.024525033310055732727f, 0.028528936207294464111f, -0.070564292371273040771f, 0.051963280886411666870f, 0.090760760009288787842f, -0.12127671390771865845f, -0.059935815632343292236f, -0.077119290828704833984f, -0.078511618077754974365f, 0.018362935632467269897f, -0.081992305815219879150f, 0.18881429731845855713f, -0.041745025664567947388f, -0.10426150262355804443f, 0.012190807610750198364f, 0.036430440843105316162f, -0.019101515412330627441f, 0.083770230412483215332f, 0.17562228441238403320f, 0.33865499496459960938f, 0.12329152226448059082f, -0.061573341488838195801f, -0.018120002001523971558f, 0.035032179206609725952f, 0.20414918661117553711f, 0.13209012150764465332f, 0.10642226785421371460f, 0.099781796336174011230f, 0.085235834121704101562f, 0.0046696858480572700500f, -0.073618113994598388672f, 0.18081563711166381836f, -0.053694937378168106079f},
  {0.10821110755205154419f, -0.048061583191156387329f, 0.032478809356689453125f, -0.030768042430281639099f, -0.0065180980600416660309f, 0.043709207326173782349f, 0.037830356508493423462f, -0.0077088261023163795471f, 0.0017136676469817757607f, -0.083898633718490600586f, 0.13094928860664367676f, -0.17965765297412872314f, 0.041967261582612991333f, 0.14386267960071563721f, 0.065416276454925537109f, -0.088840737938880920410f, 0.095667086541652679443f, -0.096677698194980621338f, 0.079267822206020355225f, 0.068360865116119384766f, 0.082300886511802673340f, 0.081863507628440856934f, -0.028800588101148605347f, 0.024890067055821418762f, 0.14898034930229187012f, 0.15771330893039703369f, 0.10655769705772399902f, 0.073847264051437377930f, -0.0036930146161466836929f, 0.063212133944034576416f, 0.091580465435981750488f, 0.044743329286575317383f, 0.034241653978824615479f, -0.050271943211555480957f, -0.070932701230049133301f, -0.13829301297664642334f, 0.16232050955295562744f, -0.19468173384666442871f, 0.12204391509294509888f, -0.031161298975348472595f, 0.024081243202090263367f, 0.0072913626208901405334f, -0.056040525436401367188f, -0.099714681506156921387f, -0.11220680177211761475f, 0.082641407847404479980f, -0.11130551993846893311f, -0.011506899259984493256f, 0.032543670386075973511f, 0.0089939618483185768127f, -0.084105938673019409180f, -0.13073593378067016602f, -0.10091417282819747925f, 0.030786162242293357849f, -0.062660053372383117676f, -0.029145771637558937073f, 0.073242232203483581543f, -0.037560336291790008545f, -0.029807414859533309937f, -0.088740952312946319580f, -0.041346203535795211792f, -0.035435378551483154297f, 0.0010576612548902630806f, -0.10207390785217285156f},
  {0.060114387422800064087f, 0.026201015338301658630f, -0.042837239801883697510f, -0.0017738216556608676910f, -0.10928431153297424316f, 0.34681487083435058594f, 0.040679506957530975342f, -0.26714485883712768555f, 0.098133757710456848145f, -0.0090836500748991966248f, 0.073229119181632995605f, -0.14316192269325256348f, 0.19630108773708343506f, 0.22116057574748992920f, -0.092910461127758026123f, 0.038292177021503448486f, -0.12545080482959747314f, -0.28222945332527160645f, -0.040536701679229736328f, -0.10688839107751846313f, 0.040625654160976409912f, -0.22298622131347656250f, 0.16925211250782012939f, 0.16242957115173339844f, 0.37003055214881896973f, -0.024799123406410217285f, 0.10835189372301101685f, -0.023653896525502204895f, 0.026152992621064186096f, -0.019895913079380989075f, 0.023769596591591835022f, 0.092309713363647460938f, 0.072950817644596099854f, -0.34874197840690612793f, -8.0885212810244411230e-05f, -0.087701708078384399414f, 0.052431315183639526367f, -0.30419534444808959961f, 0.13826856017112731934f, -0.062896564602851867676f, -0.080861687660217285156f, 0.082519896328449249268f, -0.038843430578708648682f, 0.022669566795229911804f, -0.11942196637392044067f, 0.15758146345615386963f, -0.26246312260627746582f, 0.068553797900676727295f, 0.081354565918445587158f, -0.016722289845347404480f, -0.035032834857702255249f, -0.18777638673782348633f, -0.047067921608686447144f, 0.11763118952512741089f, 0.12495507299900054932f, -0.091490112245082855225f, 0.050474539399147033691f, 0.10817455500364303589f, 0.051937445998191833496f, -0.13949279487133026123f, 0.055874593555927276611f, -0.049590572714805603027f, -0.15441720187664031982f, 0.095730677247047424316f},
  {-0.18319934606552124023f, -0.13761918246746063232f, 0.19880264997482299805f, -0.10480381548404693604f, 0.064829140901565551758f, -0.29045048356056213379f, -0.026417812332510948181f, 0.18397842347621917725f, 0.14347881078720092773f, 0.078156083822250366211f, 0.010422244668006896973f, 0.0018768266309052705765f, 0.11721905320882797241f, -0.13651807606220245361f, -0.031793046742677688599f, -0.013285271823406219482f, 0.12402024120092391968f, 0.016099566593766212463f, -0.17482370138168334961f, -0.23365940153598785400f, 0.12494295835494995117f, 0.17229954898357391357f, -0.0098244296386837959290f, 0.15416134893894195557f, -0.28595909476280212402f, 0.023933168500661849976f, -0.036810155957937240601f, 0.032774556428194046021f, -0.040341477841138839722f, 0.022355435416102409363f, -0.034406114369630813599f, -0.014160990715026855469f, -0.13247823715209960938f, 0.025737171992659568787f, -0.020141854882240295410f, 0.11847010999917984009f, -0.15103706717491149902f, -0.011499987915158271790f, -0.14496710896492004395f, -0.13207083940505981445f, 0.22133706510066986084f, -0.066295705735683441162f, -0.058302320539951324463f, -0.084845088422298431396f, 0.13582605123519897461f, -0.20707590878009796143f, 0.17727078497409820557f, 0.19461569190025329590f, -0.11740882694721221924f, 0.14883731305599212646f, -0.30684557557106018066f, 0.073393702507019042969f, 0.018240382894873619080f, -0.27630645036697387695f, 0.036786425858736038208f, -0.27276548743247985840f, -0.066069878637790679932f, -0.24438452720642089844f, -0.0024198524188250303268f, 0.061526201665401458740f, 0.11702647060155868530f, -0.027048202231526374817f, 0.037767019122838973999f, -0.085933037102222442627f},
  {0.023682469502091407776f, -0.070373192429542541504f, 0.036316074430942535400f, -0.087790578603744506836f, 0.11479047685861587524f, -0.37911167740821838379f, 0.12076190114021301270f, 0.027719711884856224060f, 0.12025012075901031494f, -0.010655391030013561249f, 0.067637987434864044189f, 0.16970926523208618164f, 0.18408335745334625244f, -0.18135629594326019287f, 0.021458059549331665039f, 0.051319874823093414307f, 0.070043504238128662109f, 0.078719943761825561523f, -0.070588052272796630859f, -0.26449915766716003418f, 0.10526254773139953613f, 0.17228205502033233643f, 0.023948784917593002319f, 0.034037120640277862549f, -0.40068048238754272461f, -0.022911319509148597717f, -0.082478977739810943604f, 0.0047204862348735332489f, -0.027093974873423576355f, 0.15889863669872283936f, 0.17036998271942138672f, 0.13293303549289703369f, 0.0020875139161944389343f, 0.049992699176073074341f, -0.096863955259323120117f, 0.0086652617901563644409f, -0.091037638485431671143f, 0.052670948207378387451f, -0.057605866342782974243f, 0.0044752149842679500580f, 0.13064286112785339355f, 0.066609665751457214355f, 0.014855401590466499329f, -0.049195919185876846313f, 0.019958034157752990723f, -0.053715661168098449707f, 0.073729850351810455322f, 0.089251898229122161865f, -0.17840954661369323730f, 0.13504026830196380615f, -0.20920532941818237305f, -0.015347192995250225067f, 0.12087622284889221191f, -0.30946937203407287598f, 0.093162745237350463867f, -0.097247488796710968018f, -0.088523313403129577637f, -0.11071351170539855957f, -0.18373945355415344238f, 0.031124487519264221191f, -0.053840912878513336182f, -0.098496489226818084717f, 0.19780935347080230713f, -0.057359553873538970947f},
  {-0.018553428351879119873f, -0.10297658294439315796f, -0.033379420638084411621f, 0.12469784915447235107f, 0.064765892922878265381f, 0.20092424750328063965f, 0.066281095147132873535f, 0.035680595785379409790f, 0.11422763019800186157f, 0.095193170011043548584f, 0.0035137874074280261993f, 0.12595458328723907471f, -0.12350752949714660645f, -0.037290517240762710571f, -0.017513576894998550415f, -0.077781245112419128418f, 0.051392834633588790894f, -0.048998974263668060303f, 0.12627494335174560547f, 0.10132956504821777344f, 0.10577498376369476318f, 0.13393428921699523926f, -0.087175659835338592529f, -0.14808911085128784180f, 0.32401975989341735840f, -0.027749720960855484009f, -0.077538028359413146973f, 0.067368447780609130859f, 0.069118611514568328857f, -0.020841563120484352112f, 0.071426235139369964600f, 0.095178470015525817871f, -0.16046711802482604980f, -0.054282724857330322266f, -0.056536193937063217163f, 0.060540854930877685547f, 0.015773816034197807312f, 0.13353519141674041748f, -0.098008789122104644775f, -0.085071943700313568115f, 0.0028560601640492677689f, 0.0046862689778208732605f, 0.12714357674121856689f, -0.10510702431201934814f, -0.051351770758628845215f, -0.12554636597633361816f, 0.028460431843996047974f, -0.11963807791471481323f, 0.29191681742668151855f, -0.079102165997028350830f, 0.29462441802024841309f, -0.0013104262761771678925f, -0.035966839641332626343f, 0.081995218992233276367f, -0.11706796288490295410f, 0.12740299105644226074f, 0.073274545371532440186f, -0.059483453631401062012f, 0.13847382366657257080f, 0.063728794455528259277f, -0.068215250968933105469f, 0.022460084408521652222f, 0.11032537370920181274f, 0.033736720681190490723f},
  {-0.079679884016513824463f, 0.045072868466377258301f, 0.017688754945993423462f, -0.20319499075412750244f, 0.023115316405892372131f, -0.35628134012222290039f, 0.13385246694087982178f, -0.022101160138845443726f, 0.096959121525287628174f, 0.15558555722236633301f, -0.028817659243941307068f, 0.11320365220308303833f, 0.0097041381523013114929f, -0.10742770135402679443f, -0.17645084857940673828f, 0.086269788444042205811f, 0.067877195775508880615f, -0.096506044268608093262f, -0.12678748369216918945f, -0.16206711530685424805f, -0.043902173638343811035f, 0.12891252338886260986f, 0.13043032586574554443f, 0.16124054789543151855f, -0.34406626224517822266f, 0.13110765814781188965f, 0.056222222745418548584f, 0.010239752009510993958f, -0.029929429292678833008f, 0.053171459585428237915f, -0.030142406001687049866f, -0.050727136433124542236f, -0.10512978583574295044f, -0.032559830695390701294f, 0.038052793592214584351f, -0.018467223271727561951f, -0.16944809257984161377f, -0.045218762010335922241f, 0.020001972094178199768f, -0.0068981735967099666595f, 0.065902680158615112305f, 0.092915989458560943604f, -0.22301505506038665771f, -0.0040548308752477169037f, 0.0029135101940482854843f, -0.015889531001448631287f, 0.16318188607692718506f, 0.040896330028772354126f, -0.096041388809680938721f, 0.095498248934745788574f, -0.17575596272945404053f, 0.13273102045059204102f, 0.10242429375648498535f, -0.17085701227188110352f, 0.014395162463188171387f, -0.20660670101642608643f, 0.0014481131220236420631f, 0.019544340670108795166f, -0.13190610706806182861f, 0.029805649071931838989f, -0.060642719268798828125f, 0.0016933585284277796745f, 0.11970835924148559570f, -0.13573457300662994385f},
  {0.13086941838264465332f, -0.069862470030784606934f, -0.18321175873279571533f, 0.089863821864128112793f, -0.38681539893150329590f, 0.31538245081901550293f, 0.18395459651947021484f, -0.0021607074886560440063f, 0.12319327890872955322f, 0.12272789329290390015f, 0.16874377429485321045f, -0.39101785421371459961f, 0.083301298320293426514f, 0.21667306125164031982f, 0.012967050075531005859f, -0.27988076210021972656f, 0.075329676270484924316f, -0.28242775797843933105f, 0.078880071640014648438f, -0.11420036107301712036f, -0.079711057245731353760f, -0.17700295150279998779f, -0.025920866057276725769f, 0.075230337679386138916f, 0.37184017896652221680f, -0.040857568383216857910f, -0.014384359121322631836f, -0.0099192848429083824158f, -0.047874856740236282349f, 0.065930902957916259766f, 0.16842411458492279053f, -0.063844904303550720215f, 0.15739764273166656494f, -0.38411143422126770020f, 0.083590105175971984863f, -0.23443523049354553223f, 0.21286186575889587402f, -0.25053241848945617676f, -0.018520452082157135010f, -0.11975815147161483765f, -0.19991216063499450684f, -0.0092437453567981719971f, 0.025101065635681152344f, -0.0017814738675951957703f, -0.057318389415740966797f, 0.13385468721389770508f, -0.12496071308851242065f, 0.019093252718448638916f, 0.094330012798309326172f, 0.092959493398666381836f, 0.020928138867020606995f, -0.14585123956203460693f, -0.080768942832946777344f, 0.070693708956241607666f, 0.085966661572456359863f, 0.039192721247673034668f, 0.084288634359836578369f, -0.10653026401996612549f, -0.018284326419234275818f, 0.016121380031108856201f, 0.0092486981302499771118f, -0.047898549586534500122f, -0.038233306258916854858f, 0.14471389353275299072f},
  {-0.14071078598499298096f, -0.10822665691375732422f, 0.090363457798957824707f, 0.12687845528125762939f, 0.064638338983058929443f, -0.087393388152122497559f, 0.053808778524398803711f, 0.13869468867778778076f, 0.097428634762763977051f, 0.089060641825199127197f, -0.0042062932625412940979f, 0.11099881678819656372f, -0.038175202906131744385f, -0.13347560167312622070f, 0.099577412009239196777f, 0.050919245928525924683f, -0.029259994626045227051f, 0.13987946510314941406f, 0.12015531212091445923f, 0.16180798411369323730f, -0.068160705268383026123f, 0.033775396645069122314f, -0.053634423762559890747f, 0.042195439338684082031f, -0.11696203798055648804f, 0.0095882825553417205811f, 0.097523108124732971191f, -0.068007558584213256836f, -0.16197982430458068848f, 0.018137907609343528748f, -0.017709335312247276306f, -0.10344560444355010986f, 0.093395590782165527344f, -0.016676383092999458313f, -0.00036885839654132723808f, -0.043047502636909484863f, -0.074509181082248687744f, -0.012342350557446479797f, -0.096569679677486419678f, -0.00042980193393304944038f, 0.080498330295085906982f, 0.033636651933193206787f, -0.074240490794181823730f, 0.011736322194337844849f, -0.048933401703834533691f, -0.053947877138853073120f, -0.024770181626081466675f, -0.051515426486730575562f, -0.035623405128717422485f, 0.13980294764041900635f, 0.028882989659905433655f, 0.035609200596809387207f, -0.090788364410400390625f, -0.10390188544988632202f, -0.12097077071666717529f, 0.024886330589652061462f, -0.035536922514438629150f, 0.092518657445907592773f, -0.11450131237506866455f, -0.086734287440776824951f, 0.042369276285171508789f, 0.047122340649366378784f, 0.12082967162132263184f, 0.047590866684913635254f},
  {-0.13957901298999786377f, 0.013683257624506950378f, 0.010659835301339626312f, -0.13384710252285003662f, 0.16263939440250396729f, -0.24118062853813171387f, -0.0026909119915217161179f, 0.049555204808712005615f, 0.13041238486766815186f, 0.15589040517807006836f, -0.13873682916164398193f, 0.16390974819660186768f, 0.081357240676879882812f, -0.27971827983856201172f, -0.23545731604099273682f, 0.027398535981774330139f, 0.18187136948108673096f, 0.13358226418495178223f, -0.12255290150642395020f, -0.055723004043102264404f, 0.011212849989533424377f, 0.030909201130270957947f, -0.039522424340248107910f, 0.0014044443378224968910f, -0.34401476383209228516f, 0.13640472292900085449f, -0.038271017372608184814f, 0.068334460258483886719f, 0.063437134027481079102f, -0.019768143072724342346f, 0.057085014879703521729f, -0.026767183095216751099f, -0.079345107078552246094f, 0.024820787832140922546f, 0.16286745667457580566f, 0.18657445907592773438f, -0.19381996989250183105f, 0.17169128358364105225f, 0.061958644539117813110f, -0.12346681952476501465f, 0.034380070865154266357f, 0.16628074645996093750f, -0.030447911471128463745f, 0.15766106545925140381f, 0.13781476020812988281f, -0.27271899580955505371f, 0.15598991513252258301f, 0.18793936073780059814f, -0.11172480136156082153f, 0.10581704229116439819f, -0.096782214939594268799f, -0.015512188896536827087f, 0.072586268186569213867f, -0.12931556999683380127f, 0.034058097749948501587f, -0.28174114227294921875f, -0.032809689640998840332f, -0.21556940674781799316f, 0.022078948095440864563f, -0.050296880304813385010f, 0.068694278597831726074f, 0.11092536896467208862f, 0.068333171308040618896f, 0.044681243598461151123f},
  {-0.14409026503562927246f, 0.037841446697711944580f, -0.030244836583733558655f, -0.054379247128963470459f, -0.039894230663776397705f, -0.026108026504516601562f, -0.012981429696083068848f, 0.063787408173084259033f, -0.066010177135467529297f, 0.11774413287639617920f, -0.11042925715446472168f, 0.059380587190389633179f, -0.031789299100637435913f, -0.084570556879043579102f, 0.13122293353080749512f, 0.019576029852032661438f, 0.099792398512363433838f, -0.0018439098494127392769f, -0.093479692935943603516f, -0.0061355810612440109253f, 0.088208816945552825928f, 0.067433036863803863525f, 0.092992626130580902100f, 0.014950299635529518127f, -0.0068179732188582420349f, -0.018090516328811645508f, 0.037485107779502868652f, -0.040646992623805999756f, -0.072997316718101501465f, 0.085574164986610412598f, 0.037741236388683319092f, -0.065972216427326202393f, 0.16559748351573944092f, -0.0077321524731814861298f, 0.13087630271911621094f, -0.066302239894866943359f, -0.17416182160377502441f, 0.019038617610931396484f, -0.20230281352996826172f, 0.031847279518842697144f, 0.010439305566251277924f, 0.027500880882143974304f, 0.10624416917562484741f, 0.12350246310234069824f, -0.12361042946577072144f, -0.00049471348756924271584f, 0.075093545019626617432f, 0.064462020993232727051f, 0.11573318392038345337f, 0.050906080752611160278f, -0.00082188413944095373154f, 0.13585498929023742676f, 0.095884598791599273682f, -0.21024610102176666260f, -0.070879518985748291016f, 0.10639406740665435791f, 0.012077334336936473846f, 0.10841520130634307861f, 0.12602686882019042969f, -0.12773011624813079834f, -0.042229060083627700806f, -0.079938761889934539795f, -0.052113551646471023560f, 0.036298271268606185913f},
  {-0.013812547549605369568f, 0.063270643353462219238f, -0.12580533325672149658f, 0.19470100104808807373f, 0.0067244884558022022247f, -0.020589496940374374390f, -0.13444344699382781982f, 0.17930027842521667480f, -0.072264119982719421387f, 0.11544155329465866089f, -0.014432940632104873657f, 0.084628418087959289551f, -0.018133159726858139038f, -0.085591524839401245117f, 0.16879390180110931396f, -0.0038759121671319007874f, -0.051453813910484313965f, 0.029339194297790527344f, 0.16145420074462890625f, 0.22250272333621978760f, 0.091741345822811126709f, 0.12816560268402099609f, 0.090127952396869659424f, 0.00057938479585573077202f, -0.059114962816238403320f, 0.10321333259344100952f, -0.099545672535896301270f, 0.00066346925450488924980f, -0.043087843805551528931f, 0.18734647333621978760f, -0.18221475183963775635f, -0.0067890072241425514221f, -0.042697437107563018799f, 0.13937491178512573242f, 0.048531465232372283936f, -0.042741209268569946289f, -0.10841123759746551514f, 0.030013995245099067688f, -0.070608332753181457520f, 0.092889927327632904053f, -0.094490617513656616211f, 0.083577230572700500488f, 0.25475522875785827637f, -0.043434973806142807007f, -0.071925334632396697998f, -0.019453432410955429077f, -0.039443179965019226074f, 0.031781718134880065918f, 0.10410448908805847168f, 0.10887277126312255859f, 0.28346696496009826660f, 0.015294498763978481293f, -0.066967099905014038086f, -0.24958078563213348389f, -0.042483814060688018799f, 0.29087150096893310547f, 0.19365000724792480469f, 0.078000962734222412109f, 0.16772781312465667725f, 0.10130422562360763550f, 0.059198148548603057861f, -0.067375801503658294678f, 0.14418970048427581787f, -0.094005987048149108887f},
  {-0.017232162877917289734f, 0.11298821121454238892f, 0.17738169431686401367f, 0.029830312356352806091f, 0.024326939135789871216f, -0.21720698475837707520f, 0.036130167543888092041f, -0.031972732394933700562f, 0.16906775534152984619f, 0.081664316356182098389f, -0.023672977462410926819f, 0.17339731752872467041f, 0.047173134982585906982f, -0.13462975621223449707f, -0.20195955038070678711f, 0.043194722384214401245f, 0.13688357174396514893f, 0.14831686019897460938f, -0.012366684153676033020f, -0.24195742607116699219f, -0.030700761824846267700f, 0.14631572365760803223f, 0.037366792559623718262f, 0.10452696681022644043f, -0.30419823527336120605f, 0.14981332421302795410f, -0.058457408100366592407f, -0.026094892993569374084f, -0.12655831873416900635f, 0.21608933806419372559f, 0.16083259880542755127f, 0.12878233194351196289f, -0.092364810407161712646f, 0.056300755590200424194f, -0.031812198460102081299f, -0.025960680097341537476f, -0.14451575279235839844f, -0.011298796162009239197f, -0.10334481298923492432f, -0.043491732329130172729f, 0.070219412446022033691f, 0.027208006009459495544f, -0.18230098485946655273f, -0.076672933995723724365f, 0.056751333177089691162f, -0.12758283317089080811f, 0.21262772381305694580f, 0.011553641408681869507f, -0.077790290117263793945f, 0.033537946641445159912f, -0.23417213559150695801f, 0.059310410171747207642f, 0.12340234965085983276f, -0.14704468846321105957f, 0.20550498366355895996f, -0.14842671155929565430f, 0.0096320193260908126831f, -0.15098769962787628174f, 0.0038196598179638385773f, 0.13024546205997467041f, 0.15317279100418090820f, 0.012895080260932445526f, 0.10235947370529174805f, -0.0097839329391717910767f},
  {-0.10321185737848281860f, 0.058375488966703414917f, 0.14159287512302398682f, -0.086593143641948699951f, 0.060571745038032531738f, -0.32418894767761230469f, 0.18900187313556671143f, 0.064690023660659790039f, 0.18255490064620971680f, 0.18128210306167602539f, -0.14202781021595001221f, 0.074107035994529724121f, -0.021074606105685234070f, -0.19891811907291412354f, -0.068797320127487182617f, 0.11020364612340927124f, 0.15837574005126953125f, 0.068283788859844207764f, 0.034140355885028839111f, -0.12472005188465118408f, 0.072079055011272430420f, 0.10946970432996749878f, -0.052666291594505310059f, 0.099483124911785125732f, -0.38863840699195861816f, -0.045486059039831161499f, 0.070625349879264831543f, -0.012007180601358413696f, -0.060584295541048049927f, 0.20232172310352325439f, 0.088994808495044708252f, 0.12887161970138549805f, 0.0028323847800493240356f, 0.13389778137207031250f, 0.086490370333194732666f, -0.047995783388614654541f, -0.20413497090339660645f, 0.17254516482353210449f, -0.17750003933906555176f, 0.0027144700288772583008f, 0.11774683743715286255f, -0.029552433639764785767f, -0.14515896141529083252f, 0.14534850418567657471f, 0.071252673864364624023f, -0.17610491812229156494f, 0.17659665644168853760f, 0.17354796826839447021f, -0.17655771970748901367f, 0.20612810552120208740f, -0.32037216424942016602f, 0.19902586936950683594f, -0.0090600335970520973206f, -0.11932021379470825195f, 0.12275634706020355225f, -0.33509793877601623535f, -0.11266432702541351318f, -0.032770253717899322510f, -0.0026806236710399389267f, -0.074749886989593505859f, 0.14894616603851318359f, 0.030137829482555389404f, 0.21353088319301605225f, -0.072311058640480041504f},
  {-0.062448371201753616333f, -0.016985032707452774048f, -0.018110495060682296753f, -0.15159390866756439209f, -0.14429032802581787109f, 0.0038188823964446783066f, 0.14267474412918090820f, 0.0071675134822726249695f, 0.015868291258811950684f, -0.010147893801331520081f, 0.084876783192157745361f, 0.0036218755412846803665f, 0.084173910319805145264f, 0.10887847840785980225f, -0.21160054206848144531f, -0.052037678658962249756f, 0.10361513495445251465f, -0.22352901101112365723f, -0.13325478136539459229f, -0.084779806435108184814f, 0.083216860890388488770f, 0.097787961363792419434f, 0.051650974899530410767f, -0.023827072232961654663f, 0.18724791705608367920f, 0.11698152869939804077f, -0.084098264575004577637f, 0.083037920296192169189f, 0.017737913876771926880f, 0.039964418858289718628f, 0.13251693546772003174f, 0.10532746464014053345f, -0.00088506878819316625595f, -0.17974148690700531006f, -0.049049243330955505371f, 0.0021242075599730014801f, 0.11736975610256195068f, -0.053481884300708770752f, 0.14833357930183410645f, 0.10041803121566772461f, -0.031722050160169601440f, -0.045308265835046768188f, -0.13242977857589721680f, -0.0071218935772776603699f, -0.060308132320642471313f, 0.11556465178728103638f, -0.048707369714975357056f, 0.14005950093269348145f, -0.12367113679647445679f, 0.0054751299321651458740f, -0.077131606638431549072f, -0.093024380505084991455f, -0.071550466120243072510f, 0.032717552036046981812f, 0.10000413656234741211f, -0.23376870155334472656f, -0.074998781085014343262f, -0.023298189043998718262f, 7.6771277235820889473e-05f, 0.071964226663112640381f, -0.099216774106025695801f, -0.067684486508369445801f, -0.029956754297018051147f, 5.9210775361862033606e-05f},
  {0.061385318636894226074f, -0.011052256450057029724f, 0.13899719715118408203f, -0.0052396948449313640594f, 0.11946717649698257446f, -0.27404949069023132324f, 0.064579389989376068115f, 0.031764905899763107300f, 0.086803294718265533447f, 0.0038817957974970340729f, 0.0056718471460044384003f, 0.029985837638378143311f, -0.025919985026121139526f, -0.19516286253929138184f, -0.20989139378070831299f, 0.16600483655929565430f, 0.14593534171581268311f, 0.046575255692005157471f, 0.048445019870996475220f, -0.15642195940017700195f, 0.16567991673946380615f, 0.062076587229967117310f, 0.17815957963466644287f, 0.11527352035045623779f, -0.44929936528205871582f, 0.037603754550218582153f, 0.0053906682878732681274f, 0.0035983952693641185760f, 0.062496319413185119629f, 0.049864873290061950684f, 0.13435873389244079590f, 0.024130148813128471375f, -0.060515385121107101440f, -0.0083698313683271408081f, 0.096433117985725402832f, 0.16223874688148498535f, -0.23145073652267456055f, 0.15439078211784362793f, 0.0067798714153468608856f, -0.031439013779163360596f, 0.19144526124000549316f, -0.020940292626619338989f, -0.035010229796171188354f, 0.064709812402725219727f, 0.010912268422544002533f, -0.14864705502986907959f, 0.17257733643054962158f, 0.043626725673675537109f, -0.10888382792472839355f, 0.13035170733928680420f, -0.25559312105178833008f, -0.0059516481123864650726f, 0.18038684129714965820f, -0.21702191233634948730f, 0.15764260292053222656f, -0.33517605066299438477f, -0.17442269623279571533f, -0.13344456255435943604f, 0.037868171930313110352f, 0.15838246047496795654f, -0.056895066052675247192f, 0.0057360115461051464081f, 0.044716011732816696167f, -0.0010543927783146500587f},
  {0.055472832173109054565f, 0.12087787687778472900f, -0.077834717929363250732f, -0.035648688673973083496f, -0.091820500791072845459f, 0.011572278104722499847f, -0.11298482120037078857f, -0.074787415564060211182f, -0.12359557300806045532f, 0.037479598075151443481f, -0.061373937875032424927f, 0.096634104847908020020f, 0.069053016602993011475f, -0.11319817602634429932f, 0.0010088086128234863281f, 0.10600689798593521118f, -0.0024067417252808809280f, 0.063451558351516723633f, -0.079108044505119323730f, 0.079726882278919219971f, 0.069154568016529083252f, -0.039681855589151382446f, -0.10158590972423553467f, -0.10327281802892684937f, -0.0016865914221853017807f, -0.078823693096637725830f, -0.0044261515140533447266f, 0.024576583877205848694f, 0.043850876390933990479f, -0.019298551604151725769f, -0.031900074332952499390f, 0.11755136400461196899f, -0.12799696624279022217f, -0.0040925205685198307037f, -0.083634279668331146240f, -0.11013705283403396606f, 0.11234975606203079224f, 0.11108835786581039429f, -0.10860317200422286987f, -0.084857188165187835693f, -0.10272883623838424683f, -0.094557441771030426025f, -0.041604507714509963989f, -0.0017507144948467612267f, -0.063931331038475036621f, -0.0076322960667312145233f, 0.019487814977765083313f, -0.11201039701700210571f, -0.11030369997024536133f, 0.030151858925819396973f, 0.11415249109268188477f, 0.11314220726490020752f, 0.086174592375755310059f, 0.030449727550148963928f, -0.11442040652036666870f, 0.027783002704381942749f, -0.065930977463722229004f, 0.00049895048141479492188f, 0.058994740247726440430f, -0.064030051231384277344f, 0.054363153874874114990f, -0.12816838920116424561f, 0.044012524187564849854f, 0.054371666163206100464f},
  {0.023005826398730278015f, -0.00075408548582345247269f, 0.16615702211856842041f, -0.17014433443546295166f, 0.11345510184764862061f, -0.27216815948486328125f, 0.14693850278854370117f, 0.18953169882297515869f, 0.16498346626758575439f, 0.044801797717809677124f, 0.10271756350994110107f, 0.030132444575428962708f, 0.086432456970214843750f, -0.26145824790000915527f, -0.21464923024177551270f, 0.017760867252945899963f, 0.19016946852207183838f, 0.055381454527378082275f, 0.041167985647916793823f, -0.10018207132816314697f, 0.036347925662994384766f, 0.046117659658193588257f, 0.072952441871166229248f, 0.095686554908752441406f, -0.25722968578338623047f, 0.090387582778930664062f, 0.090716078877449035645f, 0.15570226311683654785f, -0.11118333786725997925f, -0.028940912336111068726f, -0.073332801461219787598f, 0.11316750943660736084f, -0.096166647970676422119f, 0.13708493113517761230f, -0.015416032634675502777f, 0.041018981486558914185f, -0.10817564278841018677f, 0.11517053097486495972f, 0.064828373491764068604f, 0.0090564889833331108093f, 0.16815443336963653564f, 0.10892909765243530273f, -0.17254909873008728027f, 0.032535910606384277344f, 0.10221398621797561646f, -0.26475089788436889648f, 0.091016866266727447510f, 0.0030499978456646203995f, -0.16327464580535888672f, -0.020729498937726020813f, -0.31790637969970703125f, 0.10360075533390045166f, 0.061866365373134613037f, -0.18193382024765014648f, 0.060347143560647964478f, -0.25395616888999938965f, 0.052946481853723526001f, -0.10250723361968994141f, -0.071770280599594116211f, -0.0040538874454796314240f, 0.16975404322147369385f, 0.066047973930835723877f, 0.031082248315215110779f, -0.12712161242961883545f},
  {-0.076898306608200073242f, 0.076940469443798065186f, -0.049001801759004592896f, 0.20154422521591186523f, -0.096813872456550598145f, -0.17492920160293579102f, 0.011172950267791748047f, 0.16304594278335571289f, 0.020410085096955299377f, -0.051401026546955108643f, 0.0067420690320432186127f, -0.094492368400096893311f, -0.055868916213512420654f, -0.16640329360961914062f, 0.21406103670597076416f, 0.027552092447876930237f, 0.12342401593923568726f, 0.047231175005435943604f, 0.047369979321956634521f, 0.23472967743873596191f, 0.018005482852458953857f, 0.092724181711673736572f, -0.043614588677883148193f, -0.11690487712621688843f, -0.19395582377910614014f, -0.11202487349510192871f, -0.025079250335693359375f, 0.0087636495009064674377f, 0.010256711393594741821f, 0.044036105275154113770f, -0.061554990708827972412f, -0.19491919875144958496f, -0.0050057489424943923950f, 0.13041710853576660156f, 0.11056920140981674194f, -0.066549256443977355957f, -0.0096300067380070686340f, 0.15030971169471740723f, 0.039946295320987701416f, 0.16124448180198669434f, -0.042546037584543228149f, -0.11138723790645599365f, -0.0029712193645536899567f, -0.10957625508308410645f, -0.078009024262428283691f, -0.097518429160118103027f, 0.17903262376785278320f, -0.059390213340520858765f, 0.20212137699127197266f, 0.12374445050954818726f, 0.29621654748916625977f, 0.084756322205066680908f, -0.054744143038988113403f, -0.22074836492538452148f, -0.016692981123924255371f, 0.27614086866378784180f, -0.042497031390666961670f, 0.075382031500339508057f, 0.13653618097305297852f, -0.0086629744619131088257f, 0.13108777999877929688f, 0.027629205957055091858f, -0.0039756735786795616150f, -0.11202780902385711670f},
  {0.12567900121212005615f, 0.037996683269739151001f, -0.083541557192802429199f, 0.024228911846876144409f, -0.32711827754974365234f, 0.26305210590362548828f, 0.22534324228763580322f, -0.020326493307948112488f, 0.090299472212791442871f, 0.076920382678508758545f, -0.0027543313335627317429f, -0.085868664085865020752f, 0.094234272837638854980f, 0.24361561238765716553f, 0.054977502673864364624f, -0.069098077714443206787f, -0.13888315856456756592f, -0.34601613879203796387f, -0.061579920351505279541f, 0.046405538916587829590f, 0.10368818044662475586f, -0.20774754881858825684f, 0.099973119795322418213f, 0.12479018419981002808f, 0.35338231921195983887f, 0.080673940479755401611f, -0.086155578494071960449f, 0.10074214637279510498f, 0.037166252732276916504f, -0.091889306902885437012f, 0.18750460445880889893f, 0.12477630376815795898f, -0.0034751479979604482651f, -0.43405893445014953613f, -0.0082677369937300682068f, 0.010377092286944389343f, 0.017336657270789146423f, -0.19943124055862426758f, 0.030375083908438682556f, -0.13406716287136077881f, -0.15501552820205688477f, 0.11069359630346298218f, -0.090762987732887268066f, -0.0037794059608131647110f, -0.10715659707784652710f, 0.010192527435719966888f, -0.14979483187198638916f, 0.15499767661094665527f, -0.027948487550020217896f, -0.12210092693567276001f, -0.065398149192333221436f, -0.053179077804088592529f, 0.096712298691272735596f, 0.24507497251033782959f, 0.048477847129106521606f, -0.13892483711242675781f, 0.0014748064568266272545f, -0.017033927142620086670f, 0.0030700082425028085709f, -0.16992820799350738525f, 0.087147898972034454346f, -0.060147501528263092041f, -0.063763305544853210449f, 0.097826771438121795654f},
  {-0.12252490222454071045f, -0.024586791172623634338f, 0.20953688025474548340f, -0.16518931090831756592f, 0.17771960794925689697f, -0.20078131556510925293f, 0.12259833514690399170f, 0.093871034681797027588f, 0.076846964657306671143f, 0.0057878759689629077911f, 0.022603902965784072876f, -0.020920185372233390808f, 0.13250087201595306396f, -0.10991032421588897705f, -0.022988744080066680908f, 0.14743728935718536377f, 0.19857537746429443359f, 0.0077946027740836143494f, -0.12598522007465362549f, -0.099793151021003723145f, 0.10733328759670257568f, -0.013591721653938293457f, -0.013988469727337360382f, 0.067190743982791900635f, -0.32405263185501098633f, 0.0075636114925146102905f, 0.051590669900178909302f, -0.060930375009775161743f, -0.12198886275291442871f, 0.0029887263663113117218f, -0.025083512067794799805f, 0.11893940716981887817f, -0.16219976544380187988f, 0.026605764403939247131f, -0.066145770251750946045f, 0.18902683258056640625f, -0.038385737687349319458f, 0.15750660002231597900f, -0.0097922561690211296082f, 0.085993640124797821045f, 0.20025406777858734131f, 0.13395240902900695801f, -0.056574545800685882568f, -0.018845368176698684692f, 0.12007930874824523926f, -0.10226920247077941895f, 0.13237084448337554932f, -0.014174319803714752197f, -0.11509434878826141357f, 0.19609288871288299561f, -0.099900595843791961670f, 0.20371849834918975830f, -0.032206661999225616455f, -0.23356826603412628174f, 0.066646225750446319580f, -0.13414992392063140869f, -0.076733000576496124268f, -0.011450014077126979828f, -0.054178126156330108643f, -0.034645002335309982300f, -0.038861270993947982788f, 0.11738679558038711548f, -0.021227004006505012512f, 0.0061461133882403373718f},
  {-0.011311417445540428162f, 0.12520831823348999023f, -0.00064590544207021594048f, 0.030036242678761482239f, 0.085241913795471191406f, -0.21405506134033203125f, 0.10973832756280899048f, -0.025928601622581481934f, 0.14839504659175872803f, -0.044631347060203552246f, -0.078685440123081207275f, 0.13610760867595672607f, 0.15818078815937042236f, -0.14327026903629302979f, -0.14324842393398284912f, 0.066030181944370269775f, 0.19784572720527648926f, -0.0084492079913616180420f, -0.12184291332960128784f, -0.23434777557849884033f, -0.021587986499071121216f, 0.12022536247968673706f, 0.080275557935237884521f, 0.095956206321716308594f, -0.22416779398918151855f, -0.0024336255155503749847f, -0.010216377675533294678f, 0.072049401700496673584f, -0.14500121772289276123f, 0.014449190348386764526f, 0.12704183161258697510f, 0.17084038257598876953f, 0.029998525977134704590f, 0.21714921295642852783f, 0.051770925521850585938f, 0.19100339710712432861f, -0.15303067862987518311f, 0.061040304601192474365f, 0.048732072114944458008f, -0.12573498487472534180f, 0.14621536433696746826f, 0.15658400952816009521f, -0.21947203576564788818f, 0.0049821957945823669434f, -0.030179154127836227417f, -0.19042940437793731689f, 0.19601567089557647705f, 0.044924642890691757202f, -0.086356826126575469971f, 0.014956475235521793365f, -0.26990482211112976074f, 0.14545054733753204346f, 0.085883460938930511475f, -0.27778717875480651855f, -0.0077099385671317577362f, -0.23918268084526062012f, 0.0013292214134708046913f, -0.19128027558326721191f, -0.010245488956570625305f, 0.024113541468977928162f, 0.051977537572383880615f, 0.018005525693297386169f, 0.15540048480033874512f, 0.026306249201297760010f},
  {0.10575424879789352417f, 0.13909524679183959961f, -0.095763735473155975342f, 0.092114187777042388916f, -0.078339785337448120117f, 0.21252445876598358154f, -0.047819677740335464478f, -0.19687688350677490234f, 0.013016888871788978577f, 0.037278592586517333984f, 0.076660022139549255371f, -0.14121791720390319824f, -0.059006188064813613892f, 0.29775148630142211914f, 0.066201493144035339355f, -0.034836251288652420044f, -0.11124220490455627441f, -0.16760990023612976074f, -0.028583152219653129578f, 0.023013561964035034180f, 0.080771140754222869873f, -0.17917688190937042236f, 0.014324772171676158905f, 0.092119328677654266357f, 0.40781837701797485352f, 0.091657154262065887451f, -0.035897988826036453247f, -0.052092045545578002930f, -0.0072487844154238700867f, -0.047691944986581802368f, 0.17506456375122070312f, -0.10594917833805084229f, -0.016835857182741165161f, -0.21450245380401611328f, 0.034933473914861679077f, 0.092890307307243347168f, 0.079365737736225128174f, -0.020454907789826393127f, -0.0090839993208646774292f, 0.13082648813724517822f, -0.0099895512685179710388f, 0.084404222667217254639f, 0.11483825743198394775f, 0.028463454917073249817f, -0.025535937398672103882f, 0.081624723970890045166f, -0.13871493935585021973f, -0.10471004992723464966f, 0.051763795316219329834f, 0.045793663710355758667f, 0.30124837160110473633f, 0.0076544308103621006012f, 0.093514859676361083984f, 0.26167258620262145996f, -0.076381288468837738037f, 0.29963240027427673340f, -0.016202801838517189026f, 0.049748409539461135864f, 0.11228424310684204102f, 0.10000071674585342407f, 0.10928824543952941895f, 0.034466236829757690430f, -0.14702123403549194336f, -0.028802610933780670166f},
  {-0.00051622081082314252853f, -0.027645822614431381226f, 0.14949658513069152832f, -0.0091388467699289321899f, 0.12793348729610443115f, -0.41008526086807250977f, 0.11664129793643951416f, 0.18669086694717407227f, 0.0032048637513071298599f, 0.18225508928298950195f, -0.062310930341482162476f, 0.11248558759689331055f, 0.17162998020648956299f, -0.14553004503250122070f, -0.040860347449779510498f, -0.011734434403479099274f, 0.18169820308685302734f, 0.049754422158002853394f, -0.043908569961786270142f, -0.11289177089929580688f, 0.13534666597843170166f, 0.16414809226989746094f, 0.14895118772983551025f, 0.11234217882156372070f, -0.40716153383255004883f, 0.081361129879951477051f, 0.12725292146205902100f, 0.0025405907072126865387f, 0.026189973577857017517f, 0.063114188611507415771f, 0.073609024286270141602f, 0.087968073785305023193f, 0.019635984674096107483f, 0.020788745954632759094f, 0.15337717533111572266f, 0.076990105211734771729f, -0.11071814596652984619f, 0.11633880436420440674f, -0.10105327516794204712f, -0.091757223010063171387f, 0.21157658100128173828f, 0.093423418700695037842f, -0.069196470081806182861f, 0.12469226866960525513f, -0.037942029535770416260f, -0.22214436531066894531f, 0.18030934035778045654f, 0.023604834452271461487f, -0.087884336709976196289f, 0.13330623507499694824f, -0.12960234284400939941f, 0.076713606715202331543f, 0.085167512297630310059f, -0.24582763016223907471f, 0.15116053819656372070f, -0.33978813886642456055f, -0.039498079568147659302f, -0.045740775763988494873f, 0.025286508724093437195f, -0.0020266762003302574158f, 0.11522626876831054688f, 0.12684537470340728760f, 0.078526549041271209717f, -0.028247758746147155762f},
  {-0.0067339837551116943359f, -0.052527517080307006836f, 0.045521363615989685059f, -0.019838869571685791016f, -0.069373860955238342285f, -0.10932455956935882568f, -0.016464382410049438477f, -0.11004914343357086182f, -0.10808859765529632568f, -0.034882754087448120117f, -0.0012493282556533813477f, -0.011616945266723632812f, -0.042460516095161437988f, 0.013306692242622375488f, -0.089508965611457824707f, 0.053151413798332214355f, -0.12371192872524261475f, 0.0050208568572998046875f, 0.079045012593269348145f, -0.027163535356521606445f, -0.036375641822814941406f, 0.10767175257205963135f, -0.041264757513999938965f, -0.028664171695709228516f, 0.036610350012779235840f, -0.11367045342922210693f, 0.039883509278297424316f, 0.11242869496345520020f, 0.11893174052238464355f, -0.018215283751487731934f, -0.043586283922195434570f, -0.089874297380447387695f, 0.074721753597259521484f, 0.028841108083724975586f, 0.090841352939605712891f, 0.021296501159667968750f, -0.048600703477859497070f, 0.056954547762870788574f, 0.028754457831382751465f, -0.0054751336574554443359f, 0.0086336582899093627930f, -0.12269541621208190918f, 0.0071277469396591186523f, 0.012324959039688110352f, -0.041413843631744384766f, 0.0018855780363082885742f, -0.049399420619010925293f, 0.044399276375770568848f, 0.037516310811042785645f, -0.10261285305023193359f, -0.013675019145011901855f, -0.078025564551353454590f, -0.063889101147651672363f, 0.083030134439468383789f, -0.077641561627388000488f, -0.0050868391990661621094f, -0.088821500539779663086f, -0.035684883594512939453f, -0.089142963290214538574f, -0.10307110846042633057f, -0.10809141397476196289f, -0.039011403918266296387f, -0.11094015836715698242f, -0.080089136958122253418f},
  {-0.036656141281127929688f, -0.054070845246315002441f, -0.11682035773992538452f, 0.081308506429195404053f, -0.035435087978839874268f, -0.11771088838577270508f, 0.0023584847804158926010f, -0.085746943950653076172f, -0.12839134037494659424f, 0.024223221465945243835f, 0.11962541937828063965f, -0.13200098276138305664f, -0.054353386163711547852f, -0.054848939180374145508f, -0.039510235190391540527f, 0.10279404371976852417f, 0.079379953444004058838f, 0.0098952949047088623047f, 0.033470697700977325439f, -0.045729473233222961426f, 0.043709993362426757812f, -0.093486875295639038086f, 0.097385533154010772705f, -0.020823249593377113342f, 0.024965235963463783264f, 0.059430148452520370483f, -0.047427751123905181885f, 0.054757475852966308594f, -0.00078862562077119946480f, 0.0049462658353149890900f, -0.12442589551210403442f, 0.039383545517921447754f, 0.036668017506599426270f, 0.061629906296730041504f, 0.0042184167541563510895f, 0.062933765351772308350f, -0.037328653037548065186f, -0.044565774500370025635f, -0.053588330745697021484f, -0.022304477170109748840f, 0.027847306802868843079f, -0.10062226653099060059f, -0.12032295018434524536f, -0.030793201178312301636f, -0.064144104719161987305f, 0.095905587077140808105f, -0.12479100376367568970f, -0.058464154601097106934f, -0.13405632972717285156f, -0.087861098349094390869f, 0.11731479316949844360f, -0.086122289299964904785f, 0.039312906563282012939f, 0.080134347081184387207f, 0.036700993776321411133f, -0.011510166339576244354f, -0.033109050244092941284f, 0.017648465931415557861f, -0.12447400391101837158f, -0.077666655182838439941f, -0.0070904395543038845062f, 0.036022979766130447388f, -0.042316533625125885010f, -0.049489300698041915894f},
  {0.064255669713020324707f, 0.071484461426734924316f, -0.032708048820495605469f, -0.038552813231945037842f, 0.016926158219575881958f, -0.12521241605281829834f, 0.11131570488214492798f, -0.063515283167362213135f, -0.0011728102108463644981f, -0.017796948552131652832f, -0.087774045765399932861f, 0.044795434921979904175f, 0.13423669338226318359f, -0.083613306283950805664f, -0.057563953101634979248f, -0.046352960169315338135f, 0.016729017719626426697f, 0.020376034080982208252f, -0.053225908428430557251f, -0.0068806740455329418182f, -0.020427083596587181091f, 0.042699273675680160522f, 0.029217278584837913513f, 0.017268173396587371826f, -0.11135691404342651367f, 0.13248431682586669922f, -0.088299140334129333496f, -0.0065799066796898841858f, -0.064071118831634521484f, 0.073721639811992645264f, 0.075981564819812774658f, 0.077487319707870483398f, -0.089834831655025482178f, -0.096289165318012237549f, -0.097217008471488952637f, 0.048818655312061309814f, 0.050174061208963394165f, 0.023581994697451591492f, -0.10001230984926223755f, -0.029295178130269050598f, 0.11888458579778671265f, 0.079362906515598297119f, 0.010081809014081954956f, -0.068856939673423767090f, 0.10989472270011901855f, -0.036256633698940277100f, -0.092365950345993041992f, 0.0066406992264091968536f, -0.10401613265275955200f, -0.076065890491008758545f, -0.11179693043231964111f, -0.086871147155761718750f, -0.079266607761383056641f, 0.061094630509614944458f, 0.12764501571655273438f, -0.19481649994850158691f, 0.041683450341224670410f, 0.089730441570281982422f, 0.047567497938871383667f, -0.067269884049892425537f, -0.094797804951667785645f, 0.11089792102575302124f, -0.13011184334754943848f, -0.071369722485542297363f},
  {-0.17968718707561492920f, 0.061446636915206909180f, 0.068845383822917938232f, -0.041480071842670440674f, 0.17150209844112396240f, -0.31559467315673828125f, 0.19055141508579254150f, 0.21064639091491699219f, 0.083681926131248474121f, 0.11328340321779251099f, -0.10506720840930938721f, 0.15464241802692413330f, 0.20098237693309783936f, -0.14784573018550872803f, -0.11921263486146926880f, 0.15204115211963653564f, -0.0097484374418854713440f, -0.064290650188922882080f, 0.00020984878938179463148f, -0.10977753251791000366f, 0.13179074227809906006f, 0.049163736402988433838f, 0.15408521890640258789f, 0.10068087279796600342f, -0.23173376917839050293f, 0.11550584435462951660f, 0.093407444655895233154f, -0.0023213659878820180893f, -0.082343973219394683838f, 0.18959982693195343018f, 0.14002533257007598877f, 0.022425273433327674866f, -0.039065599441528320312f, 0.041154049336910247803f, -0.050020787864923477173f, -0.036915440112352371216f, -0.14158606529235839844f, 0.075548715889453887939f, -0.16608206927776336670f, 0.0080091580748558044434f, 0.039336789399385452271f, 0.10291659832000732422f, -0.097960256040096282959f, 0.084640070796012878418f, 0.076201334595680236816f, -0.091940194368362426758f, 0.10268753767013549805f, 0.12858475744724273682f, -0.083428204059600830078f, 0.017225911840796470642f, -0.30259981751441955566f, 0.17054000496864318848f, 0.19579115509986877441f, -0.14170621335506439209f, -0.017520163208246231079f, -0.18745616078376770020f, -0.14917965233325958252f, -0.21259783208370208740f, -0.16594521701335906982f, -0.060172796249389648438f, 0.14610007405281066895f, 0.13086669147014617920f, -0.016811190173029899597f, 0.043687645345926284790f},
  {0.034263454377651214600f, -0.0097990101203322410583f, 0.15624070167541503906f, -0.060661923140287399292f, 0.16515819728374481201f, -0.27054417133331298828f, 0.0061527867801487445831f, 0.036357365548610687256f, 0.0052667008712887763977f, -0.0062946043908596038818f, -0.039258636534214019775f, 0.12442566454410552979f, 0.0017543371068313717842f, -0.32720908522605895996f, -0.18517301976680755615f, 0.043927937746047973633f, 0.13360868394374847412f, -0.025513585656881332397f, 0.011900741606950759888f, -0.17409968376159667969f, 0.0043410733342170715332f, 0.21737717092037200928f, 0.039841920137405395508f, 0.17717893421649932861f, -0.23270316421985626221f, 0.078779019415378570557f, -0.084406450390815734863f, 0.14177596569061279297f, -0.085507042706012725830f, 0.091292984783649444580f, 0.14760205149650573730f, 0.17266334593296051025f, -0.12439035624265670776f, 0.055222898721694946289f, -0.035632789134979248047f, 0.16126552224159240723f, -0.0025924041401594877243f, 0.19948028028011322021f, -0.056862387806177139282f, 0.0087034050375223159790f, -0.0087691210210323333740f, 0.15096728503704071045f, -0.078680753707885742188f, -0.076603166759014129639f, -0.019500838592648506165f, -0.10530152171850204468f, 0.20699305832386016846f, 0.17751298844814300537f, -0.10327531397342681885f, 0.13463100790977478027f, -0.16755825281143188477f, 0.063466936349868774414f, 0.012430853210389614105f, -0.28346818685531616211f, 0.21347452700138092041f, -0.38294604420661926270f, 0.029051214456558227539f, -0.11426161974668502808f, -0.021744400262832641602f, -0.018619423732161521912f, 0.069484002888202667236f, -0.0075741652399301528931f, 0.16067405045032501221f, -0.064902201294898986816f},
  {-0.0036506205797195434570f, 0.075350672006607055664f, -0.050580769777297973633f, 0.030922429636120796204f, -0.026195872575044631958f, 0.12489865720272064209f, -0.062831990420818328857f, 0.029998973011970520020f, -0.0048390091396868228912f, 0.021156296133995056152f, -0.0025344491004943847656f, -0.068082168698310852051f, 0.046024162322282791138f, 0.056079789996147155762f, -0.10524119436740875244f, 0.055153965950012207031f, -0.013248901814222335815f, 0.077297128736972808838f, -0.0025944283697754144669f, -0.031362563371658325195f, -0.027949538081884384155f, 0.11348240822553634644f, -0.071246288716793060303f, 0.012089690193533897400f, 0.065538674592971801758f, 0.042608041316270828247f, -0.0042520314455032348633f, -0.039218202233314514160f, -0.061976164579391479492f, 0.087105415761470794678f, -0.073420740664005279541f, -0.034250892698764801025f, -0.10420088469982147217f, 0.051156654953956604004f, 0.0086270915344357490540f, -0.0079592289403080940247f, -0.015499860048294067383f, 0.048107903450727462769f, -0.0071565210819244384766f, -0.025689523667097091675f, 0.11399354040622711182f, -0.027450010180473327637f, -0.0014491683105006814003f, -0.11786353588104248047f, 0.030795574188232421875f, 0.093494594097137451172f, -0.11848989129066467285f, -0.012970414943993091583f, -0.028711972758173942566f, 0.11028181016445159912f, 0.054337233304977416992f, -0.048183389008045196533f, 0.097299009561538696289f, -0.12033258378505706787f, -0.040205497294664382935f, -0.11112599074840545654f, -0.085756652057170867920f, -0.033737771213054656982f, -0.026452865451574325562f, 0.037145763635635375977f, -0.10338802635669708252f, 0.024621626362204551697f, -0.030935809016227722168f, 0.043622195720672607422f},
  {0.017969215288758277893f, 0.13583122193813323975f, 0.017321534454822540283f, -0.12543222308158874512f, -0.13102930784225463867f, 0.35036337375640869141f, 0.18905967473983764648f, -0.14529533684253692627f, -0.0090586617588996887207f, -0.10062608122825622559f, 0.086215503513813018799f, -0.086912684142589569092f, 0.061659447848796844482f, 0.15716794133186340332f, 0.022635240107774734497f, 0.041470434516668319702f, -0.074102126061916351318f, -0.28071752190589904785f, -0.044662456959486007690f, -0.048381730914115905762f, 0.029486471787095069885f, -0.11886105686426162720f, 0.042693201452493667603f, 0.049231618642807006836f, 0.25640773773193359375f, 0.0023152858484536409378f, -0.11913504451513290405f, 0.083590909838676452637f, -0.038230773061513900757f, -0.10725639760494232178f, 0.24312756955623626709f, 0.097669333219528198242f, 0.046741291880607604980f, -0.27404671907424926758f, -0.074478372931480407715f, 0.10452987998723983765f, 0.13905791938304901123f, -0.036957588046789169312f, 0.068828627467155456543f, 0.058454733341932296753f, -0.078340373933315277100f, -0.062559366226196289062f, -0.035235825926065444946f, 0.054110668599605560303f, -0.058741074055433273315f, 0.15585961937904357910f, -0.052177555859088897705f, 0.0015835186932235956192f, -0.036560609936714172363f, 0.025996476411819458008f, 0.032355878502130508423f, -0.047512065619230270386f, 0.041027333587408065796f, 0.18273572623729705811f, 0.16032803058624267578f, -0.15658719837665557861f, -0.012639861553907394409f, 0.024174630641937255859f, -0.010688565671443939209f, -0.10920823365449905396f, 0.12512236833572387695f, -0.012626963667571544647f, -0.14932204782962799072f, 0.18127702176570892334f},
  {-0.20256590843200683594f, -0.017316751182079315186f, 0.020281879231333732605f, -0.23389500379562377930f, 0.23396842181682586670f, -0.28668263554573059082f, 0.14138470590114593506f, 0.066729664802551269531f, 0.023132212460041046143f, 0.011622747406363487244f, -0.014902977272868156433f, 0.10265815258026123047f, 0.21667179465293884277f, -0.23638381063938140869f, -0.20976422727108001709f, 0.094310082495212554932f, 0.22514331340789794922f, -0.027596788480877876282f, -0.066107802093029022217f, -0.15307506918907165527f, -0.0013128629652783274651f, 0.083659447729587554932f, 0.056737896054983139038f, 0.21767479181289672852f, -0.25388455390930175781f, 0.12165144085884094238f, -0.052307061851024627686f, -0.030795156955718994141f, -0.12999898195266723633f, 0.026137225329875946045f, 0.11213058978319168091f, 0.068072594702243804932f, -0.025796348229050636292f, 0.10756444185972213745f, 0.18522946536540985107f, 0.090286567807197570801f, -0.17347665131092071533f, 0.12517450749874114990f, 0.0040600188076496124268f, -0.099677056074142456055f, 0.043991621583700180054f, -0.031399048864841461182f, -0.018255053088068962097f, 0.087956771254539489746f, -0.013437319546937942505f, -0.081439264118671417236f, 0.073277659714221954346f, 0.11182903498411178589f, -0.26817598938941955566f, 0.20180758833885192871f, -0.22597241401672363281f, 0.25645360350608825684f, 0.18459317088127136230f, -0.22599105536937713623f, 0.024420669302344322205f, -0.26141706109046936035f, -0.21642196178436279297f, -0.063722439110279083252f, -0.18691286444664001465f, 0.034256611019372940063f, 0.026748146861791610718f, 0.084230095148086547852f, 0.060367975383996963501f, -0.10457528382539749146f},
  {-0.0095867738127708435059f, 0.028000287711620330811f, 0.072461627423763275146f, 0.017136069014668464661f, 0.16170333325862884521f, -0.27235132455825805664f, 0.17243026196956634521f, 0.20151126384735107422f, 0.064601860940456390381f, 0.15854890644550323486f, 0.073034130036830902100f, 0.043404642492532730103f, 0.15373556315898895264f, -0.12674885988235473633f, -0.088963806629180908203f, -0.018757000565528869629f, 0.15481053292751312256f, 0.023639481514692306519f, -0.020954767242074012756f, -0.021542508155107498169f, 0.026434568688273429871f, 0.22341577708721160889f, 0.061920199543237686157f, 0.10303399711847305298f, -0.17372317612171173096f, 0.077955387532711029053f, 0.026344014331698417664f, 0.13716870546340942383f, 0.057446438819169998169f, 0.013444180600345134735f, 0.068846933543682098389f, 0.10751321911811828613f, -0.10029737651348114014f, 0.073792628943920135498f, 0.022994555532932281494f, 0.16801075637340545654f, -0.13148593902587890625f, 0.22129212319850921631f, 0.026425307616591453552f, 0.048896398395299911499f, 0.040724512189626693726f, 0.11631072312593460083f, -0.11856207251548767090f, -0.053364597260951995850f, 0.043047249317169189453f, -0.072177939116954803467f, 0.22124646604061126709f, -0.037253301590681076050f, -2.0405568648129701614e-05f, 0.15975041687488555908f, -0.17537108063697814941f, 0.17225544154644012451f, 0.099849775433540344238f, -0.10658672451972961426f, 0.10217214375734329224f, -0.27007588744163513184f, -7.8326258517336100340e-05f, -0.20251293480396270752f, -0.097061954438686370850f, 0.018863534554839134216f, 0.14395587146282196045f, -0.022529810667037963867f, 0.20319215953350067139f, 0.012261854484677314758f},
  {0.025409737601876258850f, -0.069355256855487823486f, 0.048992492258548736572f, -0.039104886353015899658f, 0.076438829302787780762f, -0.20463243126869201660f, 0.045331545174121856689f, 0.13690687716007232666f, 0.15574407577514648438f, -0.018329979851841926575f, -0.0064814505167305469513f, 0.11615212261676788330f, 0.048823252320289611816f, -0.10082074999809265137f, -0.079897522926330566406f, 0.11983715742826461792f, 0.077083840966224670410f, 0.0083806877955794334412f, -0.099349319934844970703f, -0.24440678954124450684f, 0.13539330661296844482f, 0.079419858753681182861f, 0.038736168295145034790f, 0.19359797239303588867f, -0.22782020270824432373f, 0.032793100923299789429f, 0.10061310231685638428f, 0.046829879283905029297f, -0.053203295916318893433f, 0.16067692637443542480f, -0.025773447006940841675f, 0.10317148268222808838f, -0.11273214221000671387f, 0.19566376507282257080f, 0.10818716883659362793f, -0.024233302101492881775f, -0.22600258886814117432f, 0.079995229840278625488f, -0.16660800576210021973f, -0.030457738786935806274f, 0.10019899159669876099f, 0.10764707624912261963f, -0.11382690072059631348f, -0.074600055813789367676f, -0.022220358252525329590f, -0.062330812215805053711f, 0.16715750098228454590f, 0.038788583129644393921f, -0.086991019546985626221f, 0.12802942097187042236f, -0.20978987216949462891f, 0.21545094251632690430f, 0.090883217751979827881f, -0.25659334659576416016f, 0.046264473348855972290f, -0.35949125885963439941f, 0.00024013523943722248077f, -0.21443453431129455566f, -0.12658511102199554443f, 0.18602317571640014648f, 0.081548079848289489746f, 0.0051122750155627727509f, 0.22023448348045349121f, -0.095561854541301727295f},
  {0.12357617914676666260f, 0.047029633074998855591f, 0.093184649944305419922f, 0.096150241792201995850f, 0.013965618796646595001f, 0.083388961851596832275f, 0.11472500860691070557f, -0.016914559528231620789f, -0.10262049734592437744f, -0.051704835146665573120f, -0.055060602724552154541f, -0.12788383662700653076f, 0.17987021803855895996f, 0.14326563477516174316f, 0.068549312651157379150f, 0.044637985527515411377f, 0.10493127256631851196f, -0.13431645929813385010f, 0.12153444439172744751f, -0.042443551123142242432f, -0.030621157959103584290f, -0.088251389563083648682f, -0.076096631586551666260f, 0.036610826849937438965f, 0.27209198474884033203f, -0.021469850093126296997f, -0.038524847477674484253f, 0.00014363833179231733084f, 0.15607275068759918213f, 0.0076512447558343410492f, -0.033580817282199859619f, -0.054055497050285339355f, 0.029407173395156860352f, -0.036288402974605560303f, -0.038804598152637481689f, -0.11386998742818832397f, 0.20094072818756103516f, 0.036098882555961608887f, 0.012299175374209880829f, -0.078473344445228576660f, -0.031106276437640190125f, -0.075655207037925720215f, -0.052313104271888732910f, 0.030172305181622505188f, 0.0041425582021474838257f, 0.11105509847402572632f, -0.072042316198348999023f, 0.065706096589565277100f, 0.10668263584375381470f, 0.051424413919448852539f, 0.13708658516407012939f, -0.074092730879783630371f, -0.099784739315509796143f, 0.21048893034458160400f, 0.12072443217039108276f, -0.028090514242649078369f, 0.11768595874309539795f, 0.089934311807155609131f, 0.10432575643062591553f, -0.089695610105991363525f, -0.013208787888288497925f, -0.014787012711167335510f, -0.031789537519216537476f, 0.079478159546852111816f},
  {-0.079810634255409240723f, -0.048157468438148498535f, -0.051639586687088012695f, -0.016566142439842224121f, -0.00031052529811859130859f, -0.081109851598739624023f, -0.033419921994209289551f, -0.053676992654800415039f, -0.10123002529144287109f, -0.037079662084579467773f, 0.031026229262351989746f, -0.088928744196891784668f, -0.12439845502376556396f, 0.10087183117866516113f, -0.092345148324966430664f, -0.11535909771919250488f, -0.11977455019950866699f, -0.087234929203987121582f, -0.077280193567276000977f, 0.043819457292556762695f, -0.040094986557960510254f, 0.031218111515045166016f, -0.083915352821350097656f, -0.10848751664161682129f, 0.068677976727485656738f, -0.0093957781791687011719f, -0.013341769576072692871f, -0.11225818097591400146f, -0.012243211269378662109f, 0.054213047027587890625f, 0.024234786629676818848f, 0.022216901183128356934f, 0.0049533843994140625000f, -0.078512340784072875977f, -0.033349782228469848633f, 0.0038236826658248901367f, -0.0083552449941635131836f, 0.057361200451850891113f, 0.087954610586166381836f, -0.058748483657836914062f, -0.11439834535121917725f, 0.10189542174339294434f, -0.046413853764533996582f, -0.029361024498939514160f, -0.12308694422245025635f, 0.0015065521001815795898f, -0.052272096276283264160f, -0.059474796056747436523f, -0.0064015090465545654297f, 0.086306840181350708008f, 0.11232404410839080811f, -0.046894580125808715820f, 0.094892293214797973633f, -0.040273666381835937500f, -0.031101241707801818848f, -0.011436134576797485352f, 0.072925835847854614258f, 0.096054032444953918457f, 0.021422848105430603027f, -0.076417237520217895508f, -0.098989263176918029785f, 0.077609881758689880371f, -0.057956039905548095703f, 0.048576444387435913086f},
  {-0.019190063700079917908f, -0.032159797847270965576f, 0.082331359386444091797f, -0.028866052627563476562f, 0.092442706227302551270f, -0.34944853186607360840f, -0.016906732693314552307f, -0.0069137592799961566925f, 0.041175790131092071533f, 0.034689985215663909912f, -0.023362699896097183228f, 0.17309178411960601807f, 0.084095917642116546631f, -0.24234795570373535156f, -0.18154101073741912842f, -0.0033648419193923473358f, 0.17777523398399353027f, 0.082150556147098541260f, 0.061733271926641464233f, -0.27201902866363525391f, 0.16082550585269927979f, 0.050374340265989303589f, 0.14803284406661987305f, 0.15298508107662200928f, -0.25531291961669921875f, 0.13295392692089080811f, -0.0053849131800234317780f, 0.083268783986568450928f, -0.025538658723235130310f, 0.18610496819019317627f, -0.019606217741966247559f, 0.11418908089399337769f, 0.048226911574602127075f, 0.14889347553253173828f, 0.039632678031921386719f, 0.064644344151020050049f, -0.14909771084785461426f, -0.053747404366731643677f, -0.012333677150309085846f, 0.057475276291370391846f, 0.028539892286062240601f, 0.14315029978752136230f, -0.17110377550125122070f, 0.15593738853931427002f, 0.013734164647758007050f, -0.058230064809322357178f, 0.17400494217872619629f, 0.11239372938871383667f, -0.038294736295938491821f, 0.19102001190185546875f, -0.24323315918445587158f, 0.0037638889625668525696f, -0.035870354622602462769f, -0.13385322690010070801f, 0.021062972024083137512f, -0.27560874819755554199f, -0.11421770602464675903f, -0.12145024538040161133f, 0.066607497632503509521f, -0.040881115943193435669f, 0.074875071644783020020f, 0.098251916468143463135f, 0.0096310134977102279663f, 0.025485970079898834229f},
  {-0.00011354684829711914062f, 0.11948834359645843506f, -0.031868416815996170044f, -0.095001503825187683105f, -0.11681912839412689209f, -0.12107332050800323486f, -0.020676717162132263184f, -0.045037582516670227051f, 0.046786669641733169556f, -0.081562474370002746582f, -0.039258360862731933594f, -0.075513914227485656738f, -0.014264847151935100555f, 0.078673601150512695312f, -0.023753374814987182617f, -0.10479558259248733521f, -0.11241745948791503906f, 0.028562664985656738281f, 0.012805908918380737305f, 0.12218900769948959351f, -0.11524576693773269653f, 0.010165545158088207245f, -0.10108065605163574219f, -0.028412481769919395447f, -0.11786977946758270264f, -0.046712126582860946655f, 0.069571197032928466797f, -0.056804016232490539551f, 0.12020485103130340576f, 0.10828360170125961304f, -0.087999731302261352539f, 0.11728186160326004028f, -0.061991900205612182617f, 0.080242194235324859619f, -0.075711041688919067383f, 0.10458113998174667358f, -0.077323466539382934570f, -0.082598902285099029541f, -0.097565785050392150879f, -0.017402600497007369995f, -0.070020057260990142822f, -0.062989279627799987793f, -0.0029348144307732582092f, -0.053166061639785766602f, -0.10054180026054382324f, 0.055233806371688842773f, 0.068951167166233062744f, 0.059939775615930557251f, -0.045032575726509094238f, -0.11310955137014389038f, -0.056645218282938003540f, -0.066966421902179718018f, 0.068667121231555938721f, 0.0086987912654876708984f, -0.11258158087730407715f, 0.12471264600753784180f, -0.025720357894897460938f, 0.11450016498565673828f, -0.079726845026016235352f, 0.12274113297462463379f, 0.078131973743438720703f, -0.053681753575801849365f, 0.031618490815162658691f, -0.12277965247631072998f},
  {-0.040600456297397613525f, 0.0098342271521687507629f, 0.059599932283163070679f, 0.044584024697542190552f, -0.089470587670803070068f, 0.066810972988605499268f, -0.010070917196571826935f, -0.042604368180036544800f, 0.042819667607545852661f, -0.10666128247976303101f, 0.041422929614782333374f, 0.072207167744636535645f, -0.11614679545164108276f, -0.098379589617252349854f, 0.095439590513706207275f, -0.081703007221221923828f, 0.047458119690418243408f, 0.016774510964751243591f, 0.099038489162921905518f, 0.097837083041667938232f, -0.13153196871280670166f, 0.090224303305149078369f, 0.0057517462410032749176f, 0.052249532192945480347f, -0.085585102438926696777f, -0.099286437034606933594f, 0.0091624706983566284180f, -0.10553033649921417236f, -0.066177949309349060059f, -0.11611381173133850098f, 0.033448375761508941650f, 0.082528419792652130127f, 0.073953777551651000977f, 0.028552897274494171143f, -0.14037330448627471924f, -0.035751756280660629272f, -0.081540375947952270508f, 0.044397249817848205566f, -0.064265742897987365723f, 0.060371588915586471558f, 0.072550043463706970215f, -0.086230233311653137207f, -0.11240189522504806519f, 0.095528006553649902344f, -0.091444477438926696777f, -0.013019427657127380371f, -0.054565005004405975342f, 0.014884109608829021454f, 0.044846814125776290894f, 0.056135907769203186035f, 0.082465387880802154541f, -0.10372274369001388550f, 0.070056773722171783447f, -0.11229866743087768555f, 0.025124613195657730103f, -0.072362087666988372803f, -0.095350213348865509033f, 0.088187418878078460693f, -0.012540084309875965118f, -0.10368099063634872437f, 0.016397735103964805603f, -0.073505148291587829590f, -0.034333348274230957031f, 0.017243305221199989319f},
  {0.094191133975982666016f, -0.093141719698905944824f, 0.088078059256076812744f, 0.077687956392765045166f, -0.052284184843301773071f, -0.026572259142994880676f, 0.049651376903057098389f, -0.028464924544095993042f, 0.10020203888416290283f, -0.071113966405391693115f, 0.028539789840579032898f, -0.077687062323093414307f, 0.031281307339668273926f, 0.071996293962001800537f, -0.054579257965087890625f, -0.093261502683162689209f, 0.032805580645799636841f, -0.014125353656709194183f, -0.012441116385161876678f, 0.040674172341823577881f, -0.019850127398967742920f, 0.077664427459239959717f, 0.0090230377390980720520f, 0.10066212713718414307f, 0.094549857079982757568f, -0.071379289031028747559f, 0.077780187129974365234f, -0.066506505012512207031f, -0.028473457321524620056f, 0.055268824100494384766f, -0.085291370749473571777f, -0.031855981796979904175f, 0.065131939947605133057f, 0.053792972117662429810f, 0.065725341439247131348f, -0.0024493793025612831116f, -0.022669240832328796387f, -0.11390277743339538574f, -0.041278202086687088013f, -0.11633550375699996948f, 0.010828716680407524109f, -0.029486630111932754517f, 0.00013198373198974877596f, -0.13246472179889678955f, 0.088658802211284637451f, 0.10136069357395172119f, -0.016830861568450927734f, -0.099928639829158782959f, 0.054130315780639648438f, -0.045355942100286483765f, -0.061871133744716644287f, -0.015727913007140159607f, -0.14027896523475646973f, -0.091824330389499664307f, 0.073504179716110229492f, 0.078039824962615966797f, -0.024795807898044586182f, -0.038716826587915420532f, -0.12261117249727249146f, -0.0072494228370487689972f, -0.027222391217947006226f, -0.0087843155488371849060f, -0.0089934244751930236816f, -0.063666261732578277588f},
  {-0.12463716417551040649f, 0.10551434755325317383f, 0.11240638047456741333f, -0.084278814494609832764f, 0.068821415305137634277f, -0.41702356934547424316f, 0.12545336782932281494f, 0.088910557329654693604f, 0.057534392923116683960f, 0.011414952576160430908f, 0.10188335180282592773f, 0.14772529900074005127f, 0.14659011363983154297f, -0.28362384438514709473f, -0.13838171958923339844f, 0.0019110211869701743126f, 0.099330782890319824219f, -0.048679091036319732666f, -0.034048534929752349854f, -0.16069580614566802979f, 0.066286005079746246338f, 0.12104108929634094238f, 0.18757884204387664795f, 0.0031074862927198410034f, -0.28116014599800109863f, 0.10444714128971099854f, 0.080557905137538909912f, 0.14502291381359100342f, 0.069858938455581665039f, 0.096145436167716979980f, 0.12885022163391113281f, 0.18817263841629028320f, -0.13060075044631958008f, 0.083586342632770538330f, -0.019274363294243812561f, 0.14731961488723754883f, -0.18975119292736053467f, 0.16842915117740631104f, -0.045799184590578079224f, -0.072627320885658264160f, 0.023894160985946655273f, 0.088455572724342346191f, -0.14930431544780731201f, 0.15070454776287078857f, -0.0063012903556227684021f, -0.074335612356662750244f, 0.11206796020269393921f, 0.049675274640321731567f, -0.17523302137851715088f, 0.11716181784868240356f, -0.29238799214363098145f, 0.10060818493366241455f, 0.14512698352336883545f, -0.28919550776481628418f, 0.027366571128368377686f, -0.26213398575782775879f, -0.069856755435466766357f, -0.16221195459365844727f, -0.048436623066663742065f, -0.0050323964096605777740f, 0.080796867609024047852f, 0.040753010660409927368f, 0.013997427187860012054f, 0.015117488801479339600f},
  {-0.052411790937185287476f, 0.14295510947704315186f, -0.010618566535413265228f, -0.076802097260951995850f, 0.13315279781818389893f, -0.045093260705471038818f, 0.097329854965209960938f, -0.020665660500526428223f, -0.061222035437822341919f, -0.032104358077049255371f, 0.081428982317447662354f, 0.077548421919345855713f, -0.036885295063257217407f, 0.073039025068283081055f, -0.15027320384979248047f, 0.023965094238519668579f, -0.058514531701803207397f, 0.11752840131521224976f, -0.058979488909244537354f, -0.16179111599922180176f, -0.075982831418514251709f, 0.030052771791815757751f, -0.056429829448461532593f, -0.050287000834941864014f, -0.16622687876224517822f, 0.093729637563228607178f, -0.018217459321022033691f, -0.043662577867507934570f, -0.041955407708883285522f, 0.062636055052280426025f, -0.071845620870590209961f, 0.13709205389022827148f, -0.061614997684955596924f, -0.038403611630201339722f, 0.067547708749771118164f, -0.087553903460502624512f, -0.12264499068260192871f, 0.13863566517829895020f, 0.053518638014793395996f, -0.069451570510864257812f, 0.056469928473234176636f, -0.054546441882848739624f, -0.0059950263239443302155f, 0.064781270921230316162f, 0.11131579428911209106f, -0.081742234528064727783f, 0.12073719501495361328f, 0.11483067274093627930f, -0.10804823040962219238f, -0.030617313459515571594f, -0.11241175979375839233f, 0.052276641130447387695f, -0.033224530518054962158f, -0.11538071185350418091f, 0.035880655050277709961f, -0.11290506273508071899f, -0.074567243456840515137f, 0.032636430114507675171f, 0.067960754036903381348f, -0.056864511221647262573f, 0.020839659497141838074f, 0.12007378786802291870f, -0.039467655122280120850f, -0.050897162407636642456f},
  {0.052722137421369552612f, 0.070868313312530517578f, -0.019355939701199531555f, 0.00059609673917293548584f, 0.072918698191642761230f, -0.20877975225448608398f, 0.097443826496601104736f, -0.0065188263542950153351f, 0.084676049649715423584f, -0.029257403686642646790f, 0.025868063792586326599f, 0.076007634401321411133f, -0.045984700322151184082f, -0.10925541818141937256f, -0.10599450021982192993f, 0.20558877289295196533f, 0.12037561088800430298f, 0.067437559366226196289f, -0.12548306584358215332f, -0.14231266081333160400f, 0.046373847872018814087f, -0.038148611783981323242f, 0.012238952331244945526f, 0.056470308452844619751f, -0.40715429186820983887f, 0.16854310035705566406f, 0.027328496798872947693f, 0.14823707938194274902f, 0.072024412453174591064f, 0.064878478646278381348f, -0.071603581309318542480f, 0.12924145162105560303f, -0.14161051809787750244f, 0.048422250896692276001f, 0.028641596436500549316f, 0.054010022431612014771f, -0.20897376537322998047f, 0.18272963166236877441f, -0.079770982265472412109f, 0.056316312402486801147f, -0.011632983572781085968f, 0.17347823083400726318f, -0.024156203493475914001f, -0.061542779207229614258f, 0.13660061359405517578f, -0.24436664581298828125f, 0.22342041134834289551f, -0.020658604800701141357f, -0.20055283606052398682f, 0.090481981635093688965f, -0.10072965919971466064f, 0.19014582037925720215f, 0.19289736449718475342f, -0.31518760323524475098f, 0.036180142313241958618f, -0.22256807982921600342f, -0.12975808978080749512f, -0.14364276826381683350f, -0.026119986549019813538f, 0.12785851955413818359f, -0.057533871382474899292f, -0.060058433562517166138f, 0.096860542893409729004f, 0.047562003135681152344f},
  {-0.0017378813354298472404f, -0.12271101772785186768f, 0.10952423512935638428f, -0.066824257373809814453f, 0.099492251873016357422f, -0.090894579887390136719f, 0.076712876558303833008f, -0.045079872012138366699f, -0.088269330561161041260f, 0.0076479115523397922516f, -0.051775552332401275635f, 0.059543315321207046509f, 0.012434182688593864441f, -0.084485583007335662842f, -0.014104828238487243652f, 0.032347131520509719849f, 0.043389450758695602417f, 0.17890879511833190918f, -0.071694619953632354736f, 0.12449982762336730957f, -0.0018407606985419988632f, 0.069569021463394165039f, -0.084010489284992218018f, -0.0025933980941772460938f, -0.089492522180080413818f, 0.0033383935224264860153f, -0.14420558512210845947f, 0.10547961294651031494f, 0.0095465974882245063782f, -0.074609860777854919434f, -0.0017569367773830890656f, -0.011244563385844230652f, 0.032787561416625976562f, 0.16217423975467681885f, 0.054414156824350357056f, -0.082282423973083496094f, -0.055270723998546600342f, 0.027995241805911064148f, 0.036211136728525161743f, 0.032657578587532043457f, 0.082648403942584991455f, -0.0058057690039277076721f, -0.015506004914641380310f, -0.090752907097339630127f, -0.14229802787303924561f, -0.11650917679071426392f, 0.015200217254459857941f, -0.089326031506061553955f, -0.024741349741816520691f, 0.053252387791872024536f, 0.14093132317066192627f, 0.016861891373991966248f, -0.090304829180240631104f, 0.061401415616273880005f, -0.063640177249908447266f, -0.0018809029133990406990f, 0.036630310118198394775f, 0.021786460652947425842f, 0.068775616586208343506f, -0.092183388769626617432f, 0.097236655652523040771f, -0.035777572542428970337f, 0.16218493878841400146f, 0.048819806426763534546f},
  {-0.17757546901702880859f, 0.060285653918981552124f, -0.017092168331146240234f, -0.0063272025436162948608f, 0.16194181144237518311f, -0.31038969755172729492f, 0.023659517988562583923f, -0.0051626269705593585968f, 0.15754686295986175537f, 0.12774482369422912598f, -0.089380331337451934814f, 0.13503056764602661133f, -0.020466078072786331177f, -0.27210566401481628418f, -0.24860976636409759521f, 0.20194301009178161621f, 0.095721848309040069580f, -0.016884505748748779297f, -0.042205508798360824585f, -0.17259675264358520508f, 0.15258885920047760010f, 0.025983801111578941345f, 0.054279610514640808105f, 0.049377296119928359985f, -0.30359616875648498535f, 0.030138522386550903320f, 0.10983991622924804688f, -0.063902132213115692139f, 0.056099154055118560791f, 0.19055604934692382812f, 0.049550149589776992798f, 0.11190810799598693848f, 0.010717513039708137512f, 0.19136714935302734375f, 0.070089228451251983643f, -0.021831044927239418030f, -0.062432236969470977783f, -0.020844675600528717041f, -0.051706373691558837891f, 0.052218507975339889526f, 0.074968382716178894043f, -0.052018713206052780151f, -0.020996121689677238464f, 0.045084856450557708740f, 0.13531073927879333496f, -0.16518504917621612549f, 0.18562160432338714600f, -0.0048203356564044952393f, -0.062026642262935638428f, 0.12100687623023986816f, -0.29743745923042297363f, 0.037984255701303482056f, 0.030878271907567977905f, -0.24690748751163482666f, 0.11595158278942108154f, -0.29272550344467163086f, -0.11051647365093231201f, -0.15298017859458923340f, -0.043713491410017013550f, 0.12032058089971542358f, -0.055574335157871246338f, -0.066239729523658752441f, 0.15014505386352539062f, 0.041087284684181213379f},
  {-0.21715149283409118652f, -0.076977796852588653564f, 0.012412884272634983063f, 0.069682136178016662598f, 0.045842353254556655884f, -0.077355921268463134766f, -0.087446637451648712158f, 0.17706513404846191406f, 0.0060355244204401969910f, 0.061416774988174438477f, 0.044359821826219558716f, -0.084351375699043273926f, -0.10899945348501205444f, -0.16558785736560821533f, 0.14831063151359558105f, 0.082116208970546722412f, 0.092278845608234405518f, 0.091961152851581573486f, -0.023557739332318305969f, 0.10302422940731048584f, 0.023507677018642425537f, -0.042285744100809097290f, -0.021348146721720695496f, 0.069694027304649353027f, -0.072881869971752166748f, -0.0091716693714261054993f, -0.13628679513931274414f, -0.0017899608938023447990f, 0.044310193508863449097f, 0.18799029290676116943f, -0.18534575402736663818f, 0.028284830972552299500f, -0.023199036717414855957f, 0.10957615822553634644f, 0.083488196134567260742f, 0.079386122524738311768f, -0.22382400929927825928f, 0.025879492983222007751f, -0.19265611469745635986f, 0.098299466073513031006f, -0.058710213750600814819f, 0.052383761852979660034f, 0.18741713464260101318f, -0.083196789026260375977f, -0.025228211656212806702f, -0.095424674451351165771f, 0.046810399740934371948f, -0.15616384148597717285f, 0.25736361742019653320f, 0.097601890563964843750f, 0.12922450900077819824f, 0.0085782576352357864380f, -0.029336627572774887085f, -0.10424403101205825806f, -0.098541274666786193848f, 0.29313609004020690918f, 0.12004357576370239258f, 0.15369592607021331787f, -0.0021120177116245031357f, 0.077449135482311248779f, 0.040769737213850021362f, 0.10674872994422912598f, 0.032533682882785797119f, -0.041262760758399963379f},
  {-0.11622413992881774902f, -0.065573126077651977539f, -0.035128898918628692627f, 0.056650985032320022583f, -0.077849544584751129150f, -0.078530490398406982422f, 0.089778184890747070312f, -0.015894835814833641052f, -0.076805710792541503906f, 0.089038498699665069580f, -0.048687994480133056641f, -0.12301249802112579346f, -0.084896542131900787354f, -0.10032975673675537109f, 0.075188599526882171631f, -0.018166506662964820862f, 0.033610507845878601074f, -0.12328810989856719971f, 0.0010421089828014373779f, 0.095250040292739868164f, 0.11271815747022628784f, -0.078175082802772521973f, 0.046732891350984573364f, -0.068310365080833435059f, 0.11396850645542144775f, -0.0052052140235900878906f, 0.057164117693901062012f, 0.028454437851905822754f, -0.045636788010597229004f, -0.12680180370807647705f, 0.033109437674283981323f, -0.079286411404609680176f, -0.047965213656425476074f, 0.092286609113216400146f, 0.012359045445919036865f, 0.018260009586811065674f, -0.097831338644027709961f, -0.078308388590812683105f, 0.041329398751258850098f, 0.061237297952175140381f, 0.047542978078126907349f, 0.11928236484527587891f, -0.038025513291358947754f, -0.094877988100051879883f, -0.10987810790538787842f, 0.10334980487823486328f, -0.012692019343376159668f, -0.061469778418540954590f, 0.080218151211738586426f, 0.082371339201927185059f, -0.011296944692730903625f, 0.082898572087287902832f, 0.055765729397535324097f, 0.098665118217468261719f, -0.024342749267816543579f, 0.099227860569953918457f, 0.054228715598583221436f, 0.019831914454698562622f, -0.10930103808641433716f, -0.064022243022918701172f, -0.059331998229026794434f, -0.044760268181562423706f, 0.084800153970718383789f, -0.058029338717460632324f},
  {-0.010740897618234157562f, -0.017898213118314743042f, 0.14279389381408691406f, -0.11636760830879211426f, -0.0071096015162765979767f, -0.28138726949691772461f, 0.16441392898559570312f, 0.13999758660793304443f, -0.028207901865243911743f, 0.065768308937549591064f, -0.12705738842487335205f, 0.033856105059385299683f, 0.049041051417589187622f, -0.14145314693450927734f, -0.19285233318805694580f, 0.031578440219163894653f, 0.15674284100532531738f, -0.042482249438762664795f, -0.051063306629657745361f, -0.18680419027805328369f, 0.14632754027843475342f, -0.019156700000166893005f, 0.076941058039665222168f, 0.14307783544063568115f, -0.34467566013336181641f, 0.11746288836002349854f, 0.016776015982031822205f, 0.044295877218246459961f, -0.047954335808753967285f, 0.19150799512863159180f, 0.10001511126756668091f, 0.097062498331069946289f, -0.025094525888562202454f, 0.025795351713895797729f, 0.058827601373195648193f, -0.0067334547638893127441f, 0.010284390300512313843f, -0.026817480102181434631f, -0.094737112522125244141f, -0.10237003117799758911f, 0.14966145157814025879f, -0.068245619535446166992f, 0.029524493962526321411f, 0.081404246389865875244f, 0.070624187588691711426f, -0.055942572653293609619f, 0.11541414260864257812f, 0.18511417508125305176f, -0.21693105995655059814f, 0.11649245023727416992f, -0.094714380800724029541f, 0.066395804286003112793f, 0.0033410054165869951248f, -0.24463365972042083740f, 0.055509988218545913696f, -0.16734398901462554932f, -0.15417633950710296631f, -0.16135361790657043457f, -0.12207701802253723145f, 0.12368812412023544312f, 0.11776646971702575684f, -0.0017203256720677018166f, 0.19334468245506286621f, -0.050426453351974487305f},
  {-0.068230502307415008545f, -0.091502830386161804199f, -0.13395772874355316162f, 0.058226503431797027588f, 0.11002951115369796753f, 0.090075746178627014160f, -0.027827408164739608765f, 0.0060894568450748920441f, 0.014642318710684776306f, 0.021296499297022819519f, -0.0064306426793336868286f, -0.039071761071681976318f, 0.046567320823669433594f, -0.078630372881889343262f, 0.17735327780246734619f, 0.073547258973121643066f, -0.011689779348671436310f, -0.029288168996572494507f, -0.067342303693294525146f, 0.032349679619073867798f, 0.047425944358110427856f, 0.061989065259695053101f, -0.0071717519313097000122f, -0.12049783021211624146f, 0.087086141109466552734f, -0.094776399433612823486f, -0.074518255889415740967f, -0.023207403719425201416f, -0.16032803058624267578f, 0.0035204302985221147537f, -0.054471772164106369019f, -0.013292607851326465607f, 0.15540291368961334229f, 0.059166286140680313110f, -0.0086453165858983993530f, 0.039713110774755477905f, -0.12430564314126968384f, -0.00041521043749526143074f, 0.046917878091335296631f, 0.015552039258182048798f, -0.056159708648920059204f, 0.088689818978309631348f, 0.041542630642652511597f, -0.0042529739439487457275f, -0.026782259345054626465f, -0.10577525943517684937f, 0.13230244815349578857f, -0.12572610378265380859f, 0.028031041845679283142f, 0.095589503645896911621f, 0.17604650557041168213f, -0.043951917439699172974f, 0.073784768581390380859f, -0.14769007265567779541f, 0.073100537061691284180f, 0.12320701777935028076f, 0.0084050996229052543640f, 0.065615966916084289551f, 0.10248780250549316406f, 0.0070707490667700767517f, -0.098192229866981506348f, 0.067140638828277587891f, -0.0079354392364621162415f, -0.056820519268512725830f},
  {0.074310168623924255371f, 0.11848349124193191528f, -0.076699629426002502441f, 0.0069775874726474285126f, 0.045162126421928405762f, -0.27714729309082031250f, -0.0062900334596633911133f, -0.025635935366153717041f, 0.036309700459241867065f, -0.078283660113811492920f, 0.087641760706901550293f, -0.075600109994411468506f, 0.037223752588033676147f, -0.25068518519401550293f, 0.053531248122453689575f, 0.031153360381722450256f, 0.15005709230899810791f, 0.076140359044075012207f, 0.064653106033802032471f, -0.026186933740973472595f, 0.062539182603359222412f, -0.051872480660676956177f, -0.019107600674033164978f, 0.060265373438596725464f, -0.24179777503013610840f, 0.034587223082780838013f, 0.025810206308960914612f, -0.061403688043355941772f, -0.078655242919921875000f, 0.12328464537858963013f, 0.066030159592628479004f, 0.13889093697071075439f, -0.13452871143817901611f, 0.088260948657989501953f, 0.047237500548362731934f, 0.019128566607832908630f, -0.089599609375000000000f, 0.084822654724121093750f, -0.051668774336576461792f, -0.091691426932811737061f, 0.12925498187541961670f, 0.13987848162651062012f, -0.0095110340043902397156f, 0.046856019645929336548f, 0.077378295361995697021f, -0.12270084768533706665f, 0.071910277009010314941f, 0.15372399985790252686f, -0.21443852782249450684f, 0.019636133685708045959f, -0.20394386351108551025f, 0.073730602860450744629f, 0.059363082051277160645f, -0.096880301833152770996f, 0.014576156623661518097f, -0.20071707665920257568f, 0.046248078346252441406f, -0.12162913382053375244f, -0.056008134037256240845f, -0.080241233110427856445f, 0.15338523685932159424f, 0.00031194119947031140327f, 0.0013358007417991757393f, -0.021550890058279037476f},
  {-0.13339771330356597900f, 0.076075762510299682617f, 0.073675572872161865234f, 0.14843975007534027100f, -0.082774758338928222656f, 0.063200116157531738281f, 0.054993260651826858521f, 0.19875666499137878418f, -0.039120834320783615112f, 0.18549910187721252441f, -0.12459271401166915894f, -0.024566428735852241516f, -0.068291857838630676270f, -0.027556018903851509094f, 0.19409152865409851074f, -0.10073330998420715332f, 0.070228159427642822266f, 0.19524899125099182129f, 0.073012672364711761475f, 0.11557821929454803467f, 0.030983889475464820862f, 0.10752300918102264404f, -0.12272502481937408447f, -0.098971389234066009521f, -0.11694537848234176636f, 0.030609529465436935425f, 0.074156150221824645996f, -0.039487075060606002808f, 0.064957544207572937012f, 0.11887749284505844116f, -0.16972096264362335205f, 0.019691077992320060730f, 0.0019048925023525953293f, 0.13002406060695648193f, 0.12084472179412841797f, 0.012270380742847919464f, -0.13690374791622161865f, 0.10404501110315322876f, 0.027531998232007026672f, 0.082710087299346923828f, -0.023744951933622360229f, -0.083849176764488220215f, 0.0037833238020539283752f, 0.0086269322782754898071f, -0.046934116631746292114f, -0.029070651158690452576f, -0.036221835762262344360f, -0.012093639932572841644f, 0.26907706260681152344f, 0.036615859717130661011f, 0.28876149654388427734f, -0.010354810394346714020f, -0.031723909080028533936f, -0.061102587729692459106f, -0.11789201945066452026f, 0.29506489634513854980f, 0.16889929771423339844f, 0.15590664744377136230f, 0.14996394515037536621f, -0.00090572074986994266510f, 0.056182373315095901489f, 0.058100190013647079468f, 0.23238007724285125732f, -0.10212177783250808716f},
  {-0.11201731115579605103f, -0.058274060487747192383f, 0.030558161437511444092f, -0.14042994379997253418f, 0.18368856608867645264f, -0.38466688990592956543f, 0.042054895311594009399f, 0.12069498747587203979f, 0.14838150143623352051f, -0.021525245159864425659f, -0.020097084343433380127f, 0.13167034089565277100f, 0.047668561339378356934f, -0.26961699128150939941f, -0.20555263757705688477f, 0.13572040200233459473f, 0.077320910990238189697f, 0.073176734149456024170f, -0.17438197135925292969f, -0.26327079534530639648f, -0.035491000860929489136f, 0.039826475083827972412f, -0.0018228366971015930176f, 0.058049105107784271240f, -0.33134096860885620117f, 0.016969529911875724792f, 0.017463037744164466858f, -0.042631708085536956787f, -0.12212419509887695312f, 0.068247638642787933350f, 0.18894192576408386230f, 0.026066925376653671265f, -0.091138742864131927490f, 0.12277060747146606445f, 0.0075268154032528400421f, 0.014280087314546108246f, -0.042357347905635833740f, -0.041526217013597488403f, 0.036869183182716369629f, 0.067845314741134643555f, 0.088344618678092956543f, 0.15021868050098419189f, -0.22577187418937683105f, 0.12399632483720779419f, 0.068169273436069488525f, -0.17099972069263458252f, 0.17246064543724060059f, 0.17751787602901458740f, -0.17830067873001098633f, 0.20253182947635650635f, -0.12948843836784362793f, 0.18813592195510864258f, 0.043317634612321853638f, -0.13764499127864837646f, 0.17495051026344299316f, -0.27678048610687255859f, 0.0012835601810365915298f, -0.10401595383882522583f, -0.14360417425632476807f, 0.12681750953197479248f, -0.060067579150199890137f, -0.026677325367927551270f, -0.027717219665646553040f, -0.12578067183494567871f},
  {-0.015192824415862560272f, 0.15697775781154632568f, 0.026492791250348091125f, 0.19284877181053161621f, 0.084408171474933624268f, -0.071418568491935729980f, -0.15856738388538360596f, 0.12581747770309448242f, 0.082898341119289398193f, -0.040076501667499542236f, -0.016814697533845901489f, 0.12765456736087799072f, -0.17178648710250854492f, -0.24119621515274047852f, 0.23063780367374420166f, -0.12281082570552825928f, 0.12187115103006362915f, 0.078146807849407196045f, 0.15180662274360656738f, 0.069643996655941009521f, -0.11632586270570755005f, -0.028572345152497291565f, 0.054196167737245559692f, -0.097487777471542358398f, -0.21121306717395782471f, 0.041726842522621154785f, -0.10866022855043411255f, 0.16061434149742126465f, -0.011551565490663051605f, 0.064517416059970855713f, -0.25952175259590148926f, -0.065630592405796051025f, 0.12114038318395614624f, 0.11357925832271575928f, -0.092429444193840026855f, 0.10397642105817794800f, -0.022756256163120269775f, 0.029579680413007736206f, -0.083843737840652465820f, 0.099663652479648590088f, -0.13395464420318603516f, -0.057349059730768203735f, 0.010453464463353157043f, 0.031697735190391540527f, -0.040547102689743041992f, -0.14734838902950286865f, 0.14675441384315490723f, -0.16600878536701202393f, 0.20736086368560791016f, 0.11110793799161911011f, 0.23585435748100280762f, -0.070512264966964721680f, 0.0079652378335595130920f, -0.012417331337928771973f, 0.051147911697626113892f, 0.26999807357788085938f, 0.12480009347200393677f, 0.24586749076843261719f, 0.010290425270795822144f, -0.057450406253337860107f, -0.095690600574016571045f, 0.059782020747661590576f, 0.10695803910493850708f, -0.091440305113792419434f},
  {-0.037007078528404235840f, 0.0027498155832290649414f, -0.070947468280792236328f, -0.068998217582702636719f, 0.029949873685836791992f, 0.057436063885688781738f, -0.10663205385208129883f, 0.058857351541519165039f, -0.026234939694404602051f, -0.047977551817893981934f, 0.045691013336181640625f, 0.046236023306846618652f, -0.11362661421298980713f, -0.0056976974010467529297f, 0.017407462000846862793f, 0.056493908166885375977f, -0.0016987770795822143555f, 0.084117218852043151855f, 0.019271850585937500000f, 0.028539091348648071289f, 0.087340816855430603027f, -0.093459621071815490723f, -0.036334201693534851074f, -0.023630291223526000977f, 0.0099854916334152221680f, 0.0041171163320541381836f, -0.012375444173812866211f, 0.052284181118011474609f, 0.089273288846015930176f, -0.026931881904602050781f, 0.023861780762672424316f, -0.081570655107498168945f, -0.068753480911254882812f, 0.031676873564720153809f, 0.12340749800205230713f, -0.0045223236083984375000f, -0.065498307347297668457f, -0.10622510313987731934f, 0.090137407183647155762f, 0.065973147749900817871f, 0.12097956240177154541f, -0.12454332411289215088f, 0.045718550682067871094f, -0.0019483119249343872070f, 0.12137277424335479736f, -0.0064844638109207153320f, 0.012372851371765136719f, -0.10053320229053497314f, -0.0036917626857757568359f, -0.017690196633338928223f, -0.020276159048080444336f, -0.076576963067054748535f, -0.12060403823852539062f, 0.075481593608856201172f, -0.020692616701126098633f, -0.11194464564323425293f, 0.0022533833980560302734f, 0.060412734746932983398f, -0.11775554716587066650f, -0.078132554888725280762f, -0.12413358688354492188f, -0.11865961551666259766f, 0.095626398921012878418f, -0.11442224681377410889f},
  {-0.11237558722496032715f, -0.11414568126201629639f, -0.12165053188800811768f, -0.028377383947372436523f, -0.099132612347602844238f, -0.069378554821014404297f, -0.10683393478393554688f, 0.091878622770309448242f, 0.00036929547786712646484f, -0.12271195650100708008f, -0.12253575026988983154f, -0.010743752121925354004f, 0.052766472101211547852f, 0.053240135312080383301f, -0.098972514271736145020f, -0.093901678919792175293f, 0.11197522282600402832f, -0.032151624560356140137f, 0.076205164194107055664f, 0.11015230417251586914f, 0.034710034728050231934f, -0.078954026103019714355f, -0.052621185779571533203f, 0.063825488090515136719f, -0.12396240234375000000f, -0.073535770177841186523f, -0.026160120964050292969f, 0.086170300841331481934f, -0.10660324990749359131f, 0.11381615698337554932f, -0.030812993645668029785f, -0.070088446140289306641f, 0.025245681405067443848f, -0.081418633460998535156f, 0.013513684272766113281f, -0.097444638609886169434f, 0.083400949835777282715f, 0.038187175989151000977f, 0.062948331236839294434f, -0.093874454498291015625f, 0.051869213581085205078f, 0.093431264162063598633f, -0.086140275001525878906f, 0.020686820149421691895f, 0.0041395723819732666016f, -0.047468990087509155273f, 0.031387835741043090820f, -0.073487430810928344727f, 0.067102968692779541016f, -0.096170619130134582520f, -0.096660554409027099609f, 0.042938679456710815430f, -0.029043868184089660645f, 0.085355371236801147461f, -0.068949446082115173340f, 0.068558171391487121582f, -0.11854165792465209961f, 0.0081668943166732788086f, -0.064654067158699035645f, -0.045686990022659301758f, -0.070589944720268249512f, -0.043410390615463256836f, -0.11629579961299896240f, -0.065086811780929565430f},
  {-0.10401766747236251831f, -0.073898531496524810791f, -0.056081078946590423584f, 0.057416409254074096680f, 0.12569148838520050049f, -0.10136678814888000488f, 0.071649827063083648682f, 0.077881932258605957031f, -0.082081809639930725098f, 0.018659371882677078247f, 0.052927885204553604126f, -0.071743316948413848877f, -0.057000569999217987061f, -0.066999919712543487549f, -0.029426386579871177673f, 0.073713332414627075195f, -0.034799627959728240967f, 0.12877146899700164795f, 0.10473801940679550171f, 0.15344284474849700928f, 0.0032604387961328029633f, 0.025048250332474708557f, -0.13384680449962615967f, -0.11437495797872543335f, -0.0018078163266181945801f, 0.066000513732433319092f, -0.11738342791795730591f, 0.085860975086688995361f, -0.0047678188420832157135f, 0.046373918652534484863f, -0.14001590013504028320f, -0.13926285505294799805f, -0.0079196393489837646484f, 0.12924794852733612061f, 0.10996799170970916748f, -0.12666395306587219238f, -0.11864267289638519287f, -0.063011467456817626953f, -0.051038879901170730591f, -0.065350316464900970459f, 0.095350906252861022949f, 0.080308876931667327881f, 0.028387028723955154419f, 0.099826186895370483398f, -0.10888528823852539062f, -0.017579881474375724792f, 0.020074885338544845581f, 0.027499748393893241882f, 0.015082211233675479889f, -0.057934615761041641235f, 0.21484772861003875732f, 0.051061548292636871338f, 0.10444218665361404419f, -0.0033293813467025756836f, -0.12213729321956634521f, 0.052898272871971130371f, 0.0036947226617485284805f, -0.041606102138757705688f, -0.058021884411573410034f, 0.048952974379062652588f, 0.054699666798114776611f, -0.10908053815364837646f, 0.14947684109210968018f, -0.00089677737560123205185f},
  {-0.14498217403888702393f, 0.074144110083580017090f, 0.078971870243549346924f, -0.073725007474422454834f, -0.028067400678992271423f, -0.24103644490242004395f, 0.046676486730575561523f, -0.029008457437157630920f, 0.10859466344118118286f, 0.025920039042830467224f, 0.014405543915927410126f, 0.19478766620159149170f, 0.19766783714294433594f, -0.10828222334384918213f, -0.13021951913833618164f, 0.094381265342235565186f, -0.038747418671846389771f, -0.0051675075665116310120f, 0.011424759402871131897f, -0.11018894612789154053f, -0.023242330178618431091f, 0.10866776108741760254f, -0.00021791509061586111784f, -0.030362756922841072083f, -0.26184695959091186523f, 0.13534571230411529541f, -0.067870453000068664551f, 0.022738048806786537170f, -0.058205649256706237793f, 0.17846435308456420898f, -0.025482064113020896912f, 0.16467101871967315674f, -0.16514003276824951172f, 0.010808383114635944366f, 0.14805015921592712402f, 0.18350391089916229248f, -0.025720849633216857910f, 0.11067273467779159546f, -0.033462624996900558472f, -0.053308080881834030151f, 0.013112464919686317444f, 0.18483899533748626709f, -0.058489691466093063354f, 0.021299026906490325928f, -0.010689054615795612335f, -0.24202078580856323242f, 0.15166375041007995605f, 0.0026409565471112728119f, -0.23286831378936767578f, -0.0098250210285186767578f, -0.11983308941125869751f, 0.082176685333251953125f, 0.13790932297706604004f, -0.20222097635269165039f, 0.11399821937084197998f, -0.18310561776161193848f, -0.064248025417327880859f, -0.18784993886947631836f, -0.036432795226573944092f, -0.019443366676568984985f, 0.052980896085500717163f, -0.040857587009668350220f, 0.21439157426357269287f, -0.097518518567085266113f},
  {0.0024426332674920558929f, 0.084821254014968872070f, 0.060501657426357269287f, -0.038594800978899002075f, 0.13812157511711120605f, -0.41090080142021179199f, -0.0057085249572992324829f, 0.040470756590366363525f, 0.035851493477821350098f, -0.030705016106367111206f, -0.055043969303369522095f, 0.19453386962413787842f, 0.012778892181813716888f, -0.10367522388696670532f, -0.18196661770343780518f, 0.038665007799863815308f, 0.19231082499027252197f, 0.22673642635345458984f, -0.041912451386451721191f, -0.24260659515857696533f, 0.047120235860347747803f, 0.11048289388418197632f, 0.21140550076961517334f, 0.14490863680839538574f, -0.38927420973777770996f, 0.068246938288211822510f, 0.051638498902320861816f, -0.059119269251823425293f, 0.087509907782077789307f, 0.011194187216460704803f, -0.014952283352613449097f, 0.20003984868526458740f, 0.011296593584120273590f, 0.17558139562606811523f, 0.0061151552945375442505f, 0.055986005812883377075f, -0.24038849771022796631f, 0.22126586735248565674f, 0.013703156262636184692f, -0.024479933083057403564f, 0.10846860706806182861f, -0.041922677308320999146f, -0.25623345375061035156f, 0.099038124084472656250f, 0.15334101021289825439f, -0.097162485122680664062f, 0.13961316645145416260f, 0.19834910333156585693f, -0.11860865354537963867f, 0.17037463188171386719f, -0.25739055871963500977f, 0.066272377967834472656f, 0.028757931664586067200f, -0.17029595375061035156f, 0.047676775604486465454f, -0.38338649272918701172f, -0.082652606070041656494f, -0.14203292131423950195f, -0.056091140955686569214f, 0.11141894012689590454f, 0.11247614026069641113f, -0.043192766606807708740f, 0.010978429578244686127f, 0.0025721217971295118332f},
  {0.061709236353635787964f, 0.091290757060050964355f, -0.043488293886184692383f, 0.14728294312953948975f, 0.13334149122238159180f, 0.12371796369552612305f, 0.077573217451572418213f, 0.094469763338565826416f, 0.065163403749465942383f, 0.0086889760568737983704f, -0.011553262360394001007f, -0.078238025307655334473f, -0.14540275931358337402f, 0.17678648233413696289f, 0.13308155536651611328f, 0.10475579649209976196f, 0.038655832409858703613f, -0.059890843927860260010f, -0.064624108374118804932f, 0.17209465801715850830f, -0.027027435600757598877f, -0.084929779171943664551f, -0.044942151755094528198f, -0.11267159879207611084f, 0.10469326376914978027f, -0.068262450397014617920f, 0.041543852537870407104f, -0.12699472904205322266f, -0.013713195919990539551f, -0.062895208597183227539f, -0.10848747193813323975f, 0.045246995985507965088f, -0.077824644744396209717f, -9.7402386018075048923e-05f, 0.061868757009506225586f, -0.046295650303363800049f, 0.019166607409715652466f, 0.14501027762889862061f, 0.027002498507499694824f, 0.10128147900104522705f, 0.11065807938575744629f, -0.061957307159900665283f, -0.041515748947858810425f, 0.041650854051113128662f, -0.062196157872676849365f, 0.10857308655977249146f, 0.033991251140832901001f, 0.036762416362762451172f, 0.10137639194726943970f, 0.080362342298030853271f, 0.047062642872333526611f, 0.10510341823101043701f, 0.082273177802562713623f, -0.055331211537122726440f, 0.14473353326320648193f, 0.17920988798141479492f, 0.036588575690984725952f, 0.087098255753517150879f, -0.013687088154256343842f, -0.033080011606216430664f, 0.014654614962637424469f, -0.021200526505708694458f, 0.016803773120045661926f, -0.017571447417140007019f},
  {0.14582124352455139160f, 0.13416878879070281982f, 0.099255472421646118164f, -0.12110057473182678223f, 0.031362704932689666748f, 0.15129782259464263916f, 0.025356987491250038147f, -0.032103136181831359863f, 0.10466384887695312500f, -0.10418442636728286743f, 0.12897458672523498535f, -0.12977863848209381104f, 0.023670705035328865051f, 0.083430632948875427246f, 0.0096728438511490821838f, 0.063621364533901214600f, 0.069672942161560058594f, -0.017196804285049438477f, 0.044398035854101181030f, -0.034639004617929458618f, -0.053514815866947174072f, -0.042131364345550537109f, 0.27296090126037597656f, 0.068745106458663940430f, 0.25497210025787353516f, 0.096582248806953430176f, 0.10185045748949050903f, 0.068662345409393310547f, 0.15494477748870849609f, -0.12070393562316894531f, 0.11818043142557144165f, -0.017584625631570816040f, 0.020580520853400230408f, -0.21192526817321777344f, -0.043760430067777633667f, 0.064074084162712097168f, 0.16390004754066467285f, -0.050971701741218566895f, -0.031147390604019165039f, -0.045458346605300903320f, -0.078536190092563629150f, 0.058627933263778686523f, -0.12906880676746368408f, 0.022497404366731643677f, 0.090892344713211059570f, 0.069961234927177429199f, -0.14100621640682220459f, 0.049730777740478515625f, 0.0061249807476997375488f, 0.10424516350030899048f, 0.097878143191337585449f, -0.065982826054096221924f, -0.020405638962984085083f, 0.084195427596569061279f, 0.16668394207954406738f, -0.13966158032417297363f, 0.13591039180755615234f, -0.049738012254238128662f, -0.11434141546487808228f, -0.10549148172140121460f, -0.066083110868930816650f, 0.018704498186707496643f, -0.066420651972293853760f, 0.012285497039556503296f},
  {-0.040117502212524414062f, -0.10007674247026443481f, 0.097312852740287780762f, -0.062359668314456939697f, 0.24094434082508087158f, -0.20297551155090332031f, 0.019556924700736999512f, 0.19050478935241699219f, -0.060057498514652252197f, 0.15906202793121337891f, -0.10080702602863311768f, 0.11793009936809539795f, 0.14496777951717376709f, -0.065728113055229187012f, -0.14808622002601623535f, 0.20679523050785064697f, -8.3975435700267553329e-05f, -0.0089267333969473838806f, 0.026259277015924453735f, -0.10216629505157470703f, 0.18354888260364532471f, 0.092439793050289154053f, 0.14270281791687011719f, 0.075184166431427001953f, -0.37752416729927062988f, 0.15785120427608489990f, -0.061065986752510070801f, 0.12493377923965454102f, -0.056496337056159973145f, 0.085782796144485473633f, 0.13952545821666717529f, -0.042048625648021697998f, -0.067061550915241241455f, 0.13339225947856903076f, 0.13784511387348175049f, -0.026276867836713790894f, -0.15166933834552764893f, 0.099880792200565338135f, -0.18924054503440856934f, -0.051832664757966995239f, 0.011433481238782405853f, 0.10989702492952346802f, -0.12999410927295684814f, -0.092193521559238433838f, -0.069762744009494781494f, -0.13568031787872314453f, 0.041560299694538116455f, 0.14502644538879394531f, -0.11845356225967407227f, 0.052750278264284133911f, -0.084671929478645324707f, 0.22227264940738677979f, 0.10948400944471359253f, -0.067074947059154510498f, 0.080780200660228729248f, -0.30780676007270812988f, 0.026945114135742187500f, -0.099766016006469726562f, -0.027298206463456153870f, 0.050556998699903488159f, 0.034653849899768829346f, 0.17426100373268127441f, 0.19931569695472717285f, 0.0092919683083891868591f},
  {-0.15278755128383636475f, -0.087104529142379760742f, 0.14522665739059448242f, -0.18495775759220123291f, 0.020615890622138977051f, -0.26572462916374206543f, 0.20124918222427368164f, -0.020954692736268043518f, 0.14723081886768341064f, 0.16932784020900726318f, -0.020040834322571754456f, 0.15089026093482971191f, 0.16118556261062622070f, -0.12477976828813552856f, -0.22230687737464904785f, 0.016244117170572280884f, 0.029141971841454505920f, 0.12768381834030151367f, 0.0057904613204300403595f, -0.13198582828044891357f, 0.15609464049339294434f, 0.15516602993011474609f, 0.10403261333703994751f, 0.22339242696762084961f, -0.25879731774330139160f, 0.13234755396842956543f, -0.012847899459302425385f, 0.13535000383853912354f, -0.14497584104537963867f, 0.14736555516719818115f, -0.022914826869964599609f, 0.18473429977893829346f, -0.15124826133251190186f, 0.093544207513332366943f, 0.018747987225651741028f, 0.11141520738601684570f, -0.070702955126762390137f, -0.0084126545116305351257f, -0.13058942556381225586f, 0.024603942409157752991f, 0.022259024903178215027f, 0.13036559522151947021f, -0.071212828159332275391f, 0.12016880512237548828f, 0.15068890154361724854f, -0.11638120561838150024f, 0.073782920837402343750f, 0.079236678779125213623f, -0.052878405898809432983f, 0.081459872424602508545f, -0.19080758094787597656f, 0.11013249307870864868f, 0.073123000562191009521f, -0.13455866277217864990f, -0.021426772698760032654f, -0.23350316286087036133f, -0.052656911313533782959f, -0.20892307162284851074f, 0.043684963136911392212f, 0.088576249778270721436f, 0.12643119692802429199f, -0.039343047887086868286f, 0.064397759735584259033f, 0.10009685903787612915f},
  {-0.086049318313598632812f, -0.034295100718736648560f, -0.043371643871068954468f, 0.054444704204797744751f, 0.0037982757203280925751f, -0.18018735945224761963f, -0.11209211498498916626f, 0.22656969726085662842f, 0.094506911933422088623f, 0.11690008640289306641f, -0.033310234546661376953f, 0.13691514730453491211f, -0.089837506413459777832f, 0.060573127120733261108f, 0.040039129555225372314f, 0.010551179759204387665f, -0.039162866771221160889f, 0.22109337151050567627f, 0.11775935441255569458f, 0.19602893292903900146f, -0.042401969432830810547f, -0.067044876515865325928f, 0.015156491659581661224f, -0.076047435402870178223f, -0.26246169209480285645f, -0.11014785617589950562f, 0.0080521516501903533936f, -0.046520914882421493530f, -0.14845900237560272217f, 0.096137985587120056152f, -0.20659901201725006104f, 0.038496278226375579834f, 0.10600338131189346313f, 0.016817072406411170959f, 0.10393842309713363647f, -0.039003357291221618652f, -0.32307946681976318359f, 0.18394696712493896484f, -0.20559011399745941162f, 0.13313636183738708496f, 0.14283961057662963867f, 0.039540417492389678955f, 0.058626294136047363281f, -0.061148397624492645264f, -0.025370173156261444092f, -0.14014957845211029053f, 0.19848206639289855957f, 0.070020005106925964355f, 0.12303746491670608521f, 0.034182179719209671021f, 0.22301192581653594971f, 0.11979442834854125977f, 0.0088324919342994689941f, -0.25305840373039245605f, -0.077745653688907623291f, 0.10047631710767745972f, -0.10837655514478683472f, 0.12738724052906036377f, -0.098601698875427246094f, -0.054327215999364852905f, 0.075804978609085083008f, 0.030651267617940902710f, 0.16510020196437835693f, -0.21415407955646514893f},
  {-0.086748644709587097168f, 0.099568262696266174316f, 0.11406669020652770996f, -0.056097932159900665283f, 0.064535170793533325195f, 0.12026183307170867920f, 0.028994726017117500305f, -0.10887882858514785767f, -0.12489275634288787842f, -0.039537280797958374023f, 0.00028298795223236083984f, 0.026954013854265213013f, 0.0032525854185223579407f, 0.028910785913467407227f, -0.095637612044811248779f, 0.051155261695384979248f, -0.10120197385549545288f, -0.090080060064792633057f, -0.10588328540325164795f, -0.091511569917201995850f, -0.11603450775146484375f, -0.10310941189527511597f, -0.048253767192363739014f, 0.056797511875629425049f, -0.12241497635841369629f, 0.061981912702322006226f, -0.034301519393920898438f, -0.0080565661191940307617f, 0.054199159145355224609f, 0.040219128131866455078f, 0.016477262601256370544f, -0.10295023769140243530f, -0.12140145897865295410f, -0.039552748203277587891f, 0.010696174576878547668f, 0.044259596616029739380f, 0.0044669359922409057617f, 0.027315070852637290955f, 0.020718574523925781250f, 0.074719622731208801270f, -0.11690987646579742432f, 0.063505902886390686035f, -0.086451590061187744141f, -0.079580739140510559082f, -0.045407682657241821289f, -0.10300225019454956055f, 0.027933942154049873352f, -0.064018264412879943848f, -0.066603071987628936768f, 0.011599063873291015625f, 0.033965971320867538452f, -0.070140823721885681152f, -0.012645905837416648865f, -0.053042799234390258789f, 0.10393191128969192505f, -0.019162125885486602783f, 0.058196205645799636841f, -0.0047873235307633876801f, -0.070056617259979248047f, 0.016911083832383155823f, -0.014761805534362792969f, -0.082879766821861267090f, -0.10781235992908477783f, -0.034528210759162902832f}
};
static const float tensor_q_net_2_bias[64] =
{0.0096521666273474693298f, 0.033651806414127349854f, 0.039734158664941787720f, 0.20449157059192657471f, -0.048061478883028030396f, -0.056718215346336364746f, 0.026126472279429435730f, -0.065203808248043060303f, -0.034804169088602066040f, 0.10555473715066909790f, 0.028207514435052871704f, -0.0071139787323772907257f, 0.078003294765949249268f, 0.12952941656112670898f, 0.0068084192462265491486f, 0.13565428555011749268f, -0.068175248801708221436f, 0.11213050037622451782f, 0.058198858052492141724f, -0.12900085747241973877f, 0.031319487839937210083f, 0.068159498274326324463f, -0.027732206508517265320f, 0.083435602486133575439f, 0.0016890019178390502930f, 0.089291818439960479736f, 0.080637045204639434814f, 0.035527806729078292847f, 0.15749275684356689453f, -0.12806032598018646240f, 0.014444061554968357086f, 0.0088132573291659355164f, 0.093024194240570068359f, 0.10810915380716323853f, 0.065124087035655975342f, -0.030817240476608276367f, 0.13396817445755004883f, 0.10516948997974395752f, 0.063125923275947570801f, -0.0096406666561961174011f, 0.18038420379161834717f, 0.10798822343349456787f, 0.17046803236007690430f, -0.0025395611301064491272f, 0.14440056681632995605f, 0.059908881783485412598f, -0.090780213475227355957f, 0.078167028725147247314f, 0.10325656086206436157f, 0.057576376944780349731f, 0.061316471546888351440f, 0.057112295180559158325f, -0.017522133886814117432f, 0.041483655571937561035f, -0.079162329435348510742f, 0.033095199614763259888f, 0.13147293031215667725f, 0.023220958188176155090f, -0.10670194774866104126f, 0.0076403943821787834167f, 0.14721906185150146484f, 0.057191677391529083252f, 0.042262200266122817993f, -0.069172650575637817383f};
static const float tensor_q_net_4_weight[2][64] =
{
  {-0.10003891587257385254f, -0.22389468550682067871f, -0.24801196157932281494f, 0.11498097330331802368f, 0.28772687911987304688f, -0.16866405308246612549f, 0.20909047126770019531f, -0.21250520646572113037f, -0.22376564145088195801f, 0.21262970566749572754f, -0.10199058055877685547f, -0.22679378092288970947f, 0.11511342227458953857f, 0.15330210328102111816f, -0.10844686627388000488f, 0.17825371026992797852f, 0.059347081929445266724f, 0.21142321825027465820f, -0.16515047848224639893f, -0.17857280373573303223f, 0.22156931459903717041f, 0.18464004993438720703f, -0.26450186967849731445f, 0.11040878295898437500f, -0.091056838631629943848f, 0.0013872138224542140961f, 0.18776123225688934326f, 0.18121501803398132324f, 0.11458509415388107300f, -0.11783507466316223145f, -0.12730509042739868164f, 0.18567420542240142822f, 0.032896466553211212158f, 0.10013416409492492676f, -0.081150569021701812744f, -0.11053027212619781494f, 0.22552049160003662109f, -0.0011648660292848944664f, -0.038057755678892135620f, -0.056109391152858734131f, 0.19609737396240234375f, 0.091320544481277465820f, 0.20271755754947662354f, -0.16160650551319122314f, 0.19359958171844482422f, -0.11616066098213195801f, 0.021920168772339820862f, 0.24542203545570373535f, -0.14282654225826263428f, 0.13642586767673492432f, -0.20393803715705871582f, 0.20961524546146392822f, -0.11975769698619842529f, 0.11171920597553253174f, 0.11423823237419128418f, -0.13448771834373474121f, 0.23380319774150848389f, 0.13872042298316955566f, -0.16640475392341613770f, 0.0080497665330767631531f, 0.058250959962606430054f, 0.10097389668226242065f, -0.23964695632457733154f, 0.031074119731783866882f},
  {-0.064644664525985717773f, 0.025081984698772430420f, -0.14800614118576049805f, 0.17206047475337982178f, 0.10196705907583236694f, -0.24276515841484069824f, 0.12451238930225372314f, -0.25660970807075500488f, -0.11515801399946212769f, 0.13696485757827758789f, 0.0023003430105745792389f, -0.19940260052680969238f, 0.23395220935344696045f, 0.12657716870307922363f, -0.21764779090881347656f, 0.18812216818332672119f, -0.12095117568969726562f, 0.22301824390888214111f, -0.11812508851289749146f, -0.20545639097690582275f, 0.17318363487720489502f, 0.22760221362113952637f, -0.21225596964359283447f, 0.093557655811309814453f, -0.062611520290374755859f, -0.030258854851126670837f, 0.055224511772394180298f, 0.098708644509315490723f, 0.16946645081043243408f, 0.047866143286228179932f, -0.21249262988567352295f, 0.23448053002357482910f, 0.14521183073520660400f, 0.19439262151718139648f, -0.28245556354522705078f, -0.032327592372894287109f, 0.13505572080612182617f, -0.095423951745033264160f, -0.093676775693893432617f, -0.10718034952878952026f, 0.18533910810947418213f, 0.17598302662372589111f, 0.17581206560134887695f, -0.046368718147277832031f, 0.15297882258892059326f, -0.051575455814599990845f, 0.048099324107170104980f, 0.067076772451400756836f, -0.036686591804027557373f, 0.033101703971624374390f, -0.17118781805038452148f, 0.10254925489425659180f, -0.25841554999351501465f, -0.059365600347518920898f, 0.083757340908050537109f, -0.16113641858100891113f, 0.11898251622915267944f, 0.21790604293346405029f, 0.041434187442064285278f, -0.19526278972625732422f, 0.19229492545127868652f, 0.19248346984386444092f, -0.027954874560236930847f, -0.056124195456504821777f}
};
static const float tensor_q_net_4_bias[2] =
{0.020197313278913497925f, 0.00082921260036528110504f};
union tensor_union_0 {
float tensor_7[1][4];
float tensor_9[1][64];
float tensor_11[1][64];
};
static union tensor_union_0 tu0;
union tensor_union_1 {
float tensor_8[1][64];
float tensor_10[1][64];
};
static union tensor_union_1 tu1;
static inline void node_Flatten_0( const float tensor_input[1][4], float tensor_7[1][4] )
{
 float *input = (float*)tensor_input;
 float *output = (float*)tensor_7;
 for( uint32_t i=0; i<4; i++ )
  output[i] = input[i];
}
static inline void node_Gemm_1( const float tensor_7[1][4], const float tensor_q_net_0_weight[64][4], const float tensor_q_net_0_bias[64], float tensor_8[1][64] )
{
 const int M = 1;
 const int K = 4;
 const int N = 64;
 float (*A)[4] = (float(*)[4])tensor_7;
 float (*Y)[64] = (float(*)[64])tensor_8;
 float alpha = 1.0000000000000000000;
 float beta = 1.0000000000000000000;
 float (*C)[64] = (float(*)[64])tensor_q_net_0_bias;
 for( uint32_t r=0; r<M; r++ )
  for( uint32_t c=0; c<N; c++ ) {
   float ABrc = 0;
   for( uint32_t i=0; i<K; i++ ) {
    float B = tensor_q_net_0_weight[c][i];
    ABrc += A[r][i] * B;
   }
   float tmp = ABrc * alpha;
   tmp += C[0][c] * beta;
   Y[r][c] = tmp;
 }
}
static inline void node_Relu_2( const float tensor_8[1][64], float tensor_9[1][64] )
{
 float *X = (float*)tensor_8;
 float *Y = (float*)tensor_9;
 for( uint32_t i=0; i<64; i++ )
  Y[i] = X[i] > 0 ? X[i] : 0;
}
static inline void node_Gemm_3( const float tensor_9[1][64], const float tensor_q_net_2_weight[64][64], const float tensor_q_net_2_bias[64], float tensor_10[1][64] )
{
 const int M = 1;
 const int K = 64;
 const int N = 64;
 float (*A)[64] = (float(*)[64])tensor_9;
 float (*Y)[64] = (float(*)[64])tensor_10;
 float alpha = 1.0000000000000000000;
 float beta = 1.0000000000000000000;
 float (*C)[64] = (float(*)[64])tensor_q_net_2_bias;
 for( uint32_t r=0; r<M; r++ )
  for( uint32_t c=0; c<N; c++ ) {
   float ABrc = 0;
   for( uint32_t i=0; i<K; i++ ) {
    float B = tensor_q_net_2_weight[c][i];
    ABrc += A[r][i] * B;
   }
   float tmp = ABrc * alpha;
   tmp += C[0][c] * beta;
   Y[r][c] = tmp;
 }
}
static inline void node_Relu_4( const float tensor_10[1][64], float tensor_11[1][64] )
{
 float *X = (float*)tensor_10;
 float *Y = (float*)tensor_11;
 for( uint32_t i=0; i<64; i++ )
  Y[i] = X[i] > 0 ? X[i] : 0;
}
static inline void node_Gemm_5( const float tensor_11[1][64], const float tensor_q_net_4_weight[2][64], const float tensor_q_net_4_bias[2], float tensor_output[1][2] )
{
 const int M = 1;
 const int K = 64;
 const int N = 2;
 float (*A)[64] = (float(*)[64])tensor_11;
 float (*Y)[2] = (float(*)[2])tensor_output;
 float alpha = 1.0000000000000000000;
 float beta = 1.0000000000000000000;
 float (*C)[2] = (float(*)[2])tensor_q_net_4_bias;
 for( uint32_t r=0; r<M; r++ )
  for( uint32_t c=0; c<N; c++ ) {
   float ABrc = 0;
   for( uint32_t i=0; i<K; i++ ) {
    float B = tensor_q_net_4_weight[c][i];
    ABrc += A[r][i] * B;
   }
   float tmp = ABrc * alpha;
   tmp += C[0][c] * beta;
   Y[r][c] = tmp;
 }
}
void entry(const float tensor_input[1][4], float tensor_output[1][2]) {
 node_Flatten_0( tensor_input, tu0.tensor_7);
 node_Gemm_1( tu0.tensor_7, tensor_q_net_0_weight, tensor_q_net_0_bias, tu1.tensor_8);
 node_Relu_2( tu1.tensor_8, tu0.tensor_9);
 node_Gemm_3( tu0.tensor_9, tensor_q_net_2_weight, tensor_q_net_2_bias, tu1.tensor_10);
 node_Relu_4( tu1.tensor_10, tu0.tensor_11);
 node_Gemm_5( tu0.tensor_11, tensor_q_net_4_weight, tensor_q_net_4_bias, tensor_output);
}
