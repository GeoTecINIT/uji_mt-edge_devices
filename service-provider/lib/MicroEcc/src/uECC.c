/* Copyright 2014, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#include "uECC.h"
#include "uECC_vli.h"

#ifndef uECC_RNG_MAX_TRIES
    #define uECC_RNG_MAX_TRIES 64
#endif

#if uECC_ENABLE_VLI_API
    #define uECC_VLI_API
#else
    #define uECC_VLI_API static
#endif

#if (uECC_PLATFORM == uECC_avr) || \
    (uECC_PLATFORM == uECC_arm) || \
    (uECC_PLATFORM == uECC_arm_thumb) || \
    (uECC_PLATFORM == uECC_arm_thumb2)
    #define CONCATX(a, ...) a ## __VA_ARGS__
    #define CONCAT(a, ...) CONCATX(a, __VA_ARGS__)

    #define STRX(a) #a
    #define STR(a) STRX(a)

    #define EVAL(...)  EVAL1(EVAL1(EVAL1(EVAL1(__VA_ARGS__))))
    #define EVAL1(...) EVAL2(EVAL2(EVAL2(EVAL2(__VA_ARGS__))))
    #define EVAL2(...) EVAL3(EVAL3(EVAL3(EVAL3(__VA_ARGS__))))
    #define EVAL3(...) EVAL4(EVAL4(EVAL4(EVAL4(__VA_ARGS__))))
    #define EVAL4(...) __VA_ARGS__

    #define DEC_1  0
    #define DEC_2  1
    #define DEC_3  2
    #define DEC_4  3
    #define DEC_5  4
    #define DEC_6  5
    #define DEC_7  6
    #define DEC_8  7
    #define DEC_9  8
    #define DEC_10 9
    #define DEC_11 10
    #define DEC_12 11
    #define DEC_13 12
    #define DEC_14 13
    #define DEC_15 14
    #define DEC_16 15
    #define DEC_17 16
    #define DEC_18 17
    #define DEC_19 18
    #define DEC_20 19
    #define DEC_21 20
    #define DEC_22 21
    #define DEC_23 22
    #define DEC_24 23
    #define DEC_25 24
    #define DEC_26 25
    #define DEC_27 26
    #define DEC_28 27
    #define DEC_29 28
    #define DEC_30 29
    #define DEC_31 30
    #define DEC_32 31

    #define DEC(N) CONCAT(DEC_, N)

    #define SECOND_ARG(_, val, ...) val
    #define SOME_CHECK_0 ~, 0
    #define GET_SECOND_ARG(...) SECOND_ARG(__VA_ARGS__, SOME,)
    #define SOME_OR_0(N) GET_SECOND_ARG(CONCAT(SOME_CHECK_, N))

    #define EMPTY(...)
    #define DEFER(...) __VA_ARGS__ EMPTY()

    #define REPEAT_NAME_0() REPEAT_0
    #define REPEAT_NAME_SOME() REPEAT_SOME
    #define REPEAT_0(...)
    #define REPEAT_SOME(N, stuff) DEFER(CONCAT(REPEAT_NAME_, SOME_OR_0(DEC(N))))()(DEC(N), stuff) stuff
    #define REPEAT(N, stuff) EVAL(REPEAT_SOME(N, stuff))

    #define REPEATM_NAME_0() REPEATM_0
    #define REPEATM_NAME_SOME() REPEATM_SOME
    #define REPEATM_0(...)
    #define REPEATM_SOME(N, macro) macro(N) \
        DEFER(CONCAT(REPEATM_NAME_, SOME_OR_0(DEC(N))))()(DEC(N), macro)
    #define REPEATM(N, macro) EVAL(REPEATM_SOME(N, macro))
#endif

// #include "lib/MicroEcc/src/platform-specific.inc"
// Begin manual include platform-specific.inc

/* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#ifndef _UECC_PLATFORM_SPECIFIC_H_
#define _UECC_PLATFORM_SPECIFIC_H_

#include "types.h"

#if (defined(_WIN32) || defined(_WIN64))
/* Windows */

// use pragma syntax to prevent tweaking the linker script for getting CryptXYZ function
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>

static int default_RNG(uint8_t *dest, unsigned size) {
    HCRYPTPROV prov;
    if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return 0;
    }

    CryptGenRandom(prov, size, (BYTE *)dest);
    CryptReleaseContext(prov, 0);
    return 1;
}
#define default_RNG_defined 1

#elif defined(unix) || defined(__linux__) || defined(__unix__) || defined(__unix) || \
    (defined(__APPLE__) && defined(__MACH__)) || defined(uECC_POSIX)

/* Some POSIX-like system with /dev/urandom or /dev/random. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef O_CLOEXEC
    #define O_CLOEXEC 0
#endif

static int default_RNG(uint8_t *dest, unsigned size) {
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        fd = open("/dev/random", O_RDONLY | O_CLOEXEC);
        if (fd == -1) {
            return 0;
        }
    }
    
    char *ptr = (char *)dest;
    size_t left = size;
    while (left > 0) {
        ssize_t bytes_read = read(fd, ptr, left);
        if (bytes_read <= 0) { // read failed
            close(fd);
            return 0;
        }
        left -= bytes_read;
        ptr += bytes_read;
    }
    
    close(fd);
    return 1;
}
#define default_RNG_defined 1

#elif defined(RIOT_VERSION)

#include <random.h>

static int default_RNG(uint8_t *dest, unsigned size) {
    random_bytes(dest, size);
    return 1;
}
#define default_RNG_defined 1

#elif defined(NRF52_SERIES)

#include "app_error.h"
#include "nrf_crypto_rng.h"

static int default_RNG(uint8_t *dest, unsigned size) 
{
    // make sure to call nrf_crypto_init and nrf_crypto_rng_init first
    ret_code_t ret_code = nrf_crypto_rng_vector_generate(dest, size);
    return (ret_code == NRF_SUCCESS) ? 1 : 0;
}
#define default_RNG_defined 1

#endif /* platform */

#endif /* _UECC_PLATFORM_SPECIFIC_H_ */

// End manual include platform-specific.inc

#if (uECC_WORD_SIZE == 1)
    #if uECC_SUPPORTS_secp160r1
        #define uECC_MAX_WORDS 21 /* Due to the size of curve_n. */
    #endif
    #if uECC_SUPPORTS_secp192r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 24
    #endif
    #if uECC_SUPPORTS_secp224r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 28
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 32
    #endif
#elif (uECC_WORD_SIZE == 4)
    #if uECC_SUPPORTS_secp160r1
        #define uECC_MAX_WORDS 6 /* Due to the size of curve_n. */
    #endif
    #if uECC_SUPPORTS_secp192r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 6
    #endif
    #if uECC_SUPPORTS_secp224r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 7
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 8
    #endif
#elif (uECC_WORD_SIZE == 8)
    #if uECC_SUPPORTS_secp160r1
        #define uECC_MAX_WORDS 3
    #endif
    #if uECC_SUPPORTS_secp192r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 3
    #endif
    #if uECC_SUPPORTS_secp224r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 4
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 4
    #endif
#endif /* uECC_WORD_SIZE */

#define BITS_TO_WORDS(num_bits) ((num_bits + ((uECC_WORD_SIZE * 8) - 1)) / (uECC_WORD_SIZE * 8))
#define BITS_TO_BYTES(num_bits) ((num_bits + 7) / 8)

struct uECC_Curve_t {
    wordcount_t num_words;
    wordcount_t num_bytes;
    bitcount_t num_n_bits;
    uECC_word_t p[uECC_MAX_WORDS];
    uECC_word_t n[uECC_MAX_WORDS];
    uECC_word_t G[uECC_MAX_WORDS * 2];
    uECC_word_t b[uECC_MAX_WORDS];
    void (*double_jacobian)(uECC_word_t * X1,
                            uECC_word_t * Y1,
                            uECC_word_t * Z1,
                            uECC_Curve curve);
#if uECC_SUPPORT_COMPRESSED_POINT
    void (*mod_sqrt)(uECC_word_t *a, uECC_Curve curve);
#endif
    void (*x_side)(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve);
#if (uECC_OPTIMIZATION_LEVEL > 0)
    void (*mmod_fast)(uECC_word_t *result, uECC_word_t *product);
#endif
};

#if uECC_VLI_NATIVE_LITTLE_ENDIAN
static void bcopy(uint8_t *dst,
                  const uint8_t *src,
                  unsigned num_bytes) {
    while (0 != num_bytes) {
        num_bytes--;
        dst[num_bytes] = src[num_bytes];
    }
}
#endif

static cmpresult_t uECC_vli_cmp_unsafe(const uECC_word_t *left,
                                       const uECC_word_t *right,
                                       wordcount_t num_words);

#if (uECC_PLATFORM == uECC_arm || uECC_PLATFORM == uECC_arm_thumb || \
        uECC_PLATFORM == uECC_arm_thumb2)
    // #include "asm_arm.inc"
    // Begin manual include asm_arm.inc

    /* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

    #ifndef _UECC_ASM_ARM_H_
    #define _UECC_ASM_ARM_H_

    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        #define uECC_MIN_WORDS 8
    #endif
    #if uECC_SUPPORTS_secp224r1
        #undef uECC_MIN_WORDS
        #define uECC_MIN_WORDS 7
    #endif
    #if uECC_SUPPORTS_secp192r1
        #undef uECC_MIN_WORDS
        #define uECC_MIN_WORDS 6
    #endif
    #if uECC_SUPPORTS_secp160r1
        #undef uECC_MIN_WORDS
        #define uECC_MIN_WORDS 5
    #endif

    #if (uECC_PLATFORM == uECC_arm_thumb)
        #define REG_RW "+l"
        #define REG_WRITE "=l"
    #else
        #define REG_RW "+r"
        #define REG_WRITE "=r"
    #endif

    #if (uECC_PLATFORM == uECC_arm_thumb || uECC_PLATFORM == uECC_arm_thumb2)
        #define REG_RW_LO "+l"
        #define REG_WRITE_LO "=l"
    #else
        #define REG_RW_LO "+r"
        #define REG_WRITE_LO "=r"
    #endif

    #if (uECC_PLATFORM == uECC_arm_thumb2)
        #define RESUME_SYNTAX
    #else
        #define RESUME_SYNTAX ".syntax divided \n\t"
    #endif

    #if (uECC_OPTIMIZATION_LEVEL >= 2)

    uECC_VLI_API uECC_word_t uECC_vli_add(uECC_word_t *result,
                                        const uECC_word_t *left,
                                        const uECC_word_t *right,
                                        wordcount_t num_words) {
    #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
    #if (uECC_PLATFORM == uECC_arm_thumb) || (uECC_PLATFORM == uECC_arm_thumb2)
        uint32_t jump = (uECC_MAX_WORDS - num_words) * 4 * 2 + 1;
    #else /* ARM */
        uint32_t jump = (uECC_MAX_WORDS - num_words) * 4 * 4;
    #endif
    #endif
        uint32_t carry;
        uint32_t left_word;
        uint32_t right_word;
        
        __asm__ volatile (
            ".syntax unified \n\t"
            "movs %[carry], #0 \n\t"
        #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
            "adr %[left], 1f \n\t"
            ".align 4 \n\t"
            "adds %[jump], %[left] \n\t"
        #endif
            
            "ldmia %[lptr]!, {%[left]} \n\t"
            "ldmia %[rptr]!, {%[right]} \n\t"
            "adds %[left], %[right] \n\t"
            "stmia %[dptr]!, {%[left]} \n\t"
            
        #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
            "bx %[jump] \n\t"
        #endif
            "1: \n\t"
            REPEAT(DEC(uECC_MAX_WORDS),
                "ldmia %[lptr]!, {%[left]} \n\t"
                "ldmia %[rptr]!, {%[right]} \n\t"
                "adcs %[left], %[right] \n\t"
                "stmia %[dptr]!, {%[left]} \n\t")
            
            "adcs %[carry], %[carry] \n\t"
            RESUME_SYNTAX
            : [dptr] REG_RW_LO (result), [lptr] REG_RW_LO (left), [rptr] REG_RW_LO (right),
        #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
            [jump] REG_RW_LO (jump),
        #endif
            [carry] REG_WRITE_LO (carry), [left] REG_WRITE_LO (left_word),
            [right] REG_WRITE_LO (right_word)
            :
            : "cc", "memory"
        );
        return carry;
    }
    #define asm_add 1

    uECC_VLI_API uECC_word_t uECC_vli_sub(uECC_word_t *result,
                                        const uECC_word_t *left,
                                        const uECC_word_t *right,
                                        wordcount_t num_words) {
    #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
    #if (uECC_PLATFORM == uECC_arm_thumb) || (uECC_PLATFORM == uECC_arm_thumb2)
        uint32_t jump = (uECC_MAX_WORDS - num_words) * 4 * 2 + 1;
    #else /* ARM */
        uint32_t jump = (uECC_MAX_WORDS - num_words) * 4 * 4;
    #endif
    #endif
        uint32_t carry;
        uint32_t left_word;
        uint32_t right_word;
        
        __asm__ volatile (
            ".syntax unified \n\t"
            "movs %[carry], #0 \n\t"
        #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
            "adr %[left], 1f \n\t"
            ".align 4 \n\t"
            "adds %[jump], %[left] \n\t"
        #endif
            
            "ldmia %[lptr]!, {%[left]} \n\t"
            "ldmia %[rptr]!, {%[right]} \n\t"
            "subs %[left], %[right] \n\t"
            "stmia %[dptr]!, {%[left]} \n\t"
            
        #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
            "bx %[jump] \n\t"
        #endif
            "1: \n\t"
            REPEAT(DEC(uECC_MAX_WORDS),
                "ldmia %[lptr]!, {%[left]} \n\t"
                "ldmia %[rptr]!, {%[right]} \n\t"
                "sbcs %[left], %[right] \n\t"
                "stmia %[dptr]!, {%[left]} \n\t")
            
            "adcs %[carry], %[carry] \n\t"
            RESUME_SYNTAX
            : [dptr] REG_RW_LO (result), [lptr] REG_RW_LO (left), [rptr] REG_RW_LO (right),
        #if (uECC_MAX_WORDS != uECC_MIN_WORDS)
            [jump] REG_RW_LO (jump),
        #endif
            [carry] REG_WRITE_LO (carry), [left] REG_WRITE_LO (left_word),
            [right] REG_WRITE_LO (right_word)
            :
            : "cc", "memory"
        );
        return !carry; /* Note that on ARM, carry flag set means "no borrow" when subtracting
                        (for some reason...) */
    }
    #define asm_sub 1

    #endif /* (uECC_OPTIMIZATION_LEVEL >= 2) */

    #if (uECC_OPTIMIZATION_LEVEL >= 3)

    #if (uECC_PLATFORM != uECC_arm_thumb)

    #if uECC_ARM_USE_UMAAL
        #include "asm_arm_mult_square_umaal.inc"
    #else
        #include "asm_arm_mult_square.inc"
    #endif

    #if (uECC_OPTIMIZATION_LEVEL == 3)

    uECC_VLI_API void uECC_vli_mult(uint32_t *result,
                                    const uint32_t *left,
                                    const uint32_t *right,
                                    wordcount_t num_words) {
        register uint32_t *r0 __asm__("r0") = result;
        register const uint32_t *r1 __asm__("r1") = left;
        register const uint32_t *r2 __asm__("r2") = right;
        register uint32_t r3 __asm__("r3") = num_words;
        
        __asm__ volatile (
            ".syntax unified \n\t"
    #if (uECC_MIN_WORDS == 5)
            FAST_MULT_ASM_5
        #if (uECC_MAX_WORDS > 5)
            FAST_MULT_ASM_5_TO_6
        #endif
        #if (uECC_MAX_WORDS > 6)
            FAST_MULT_ASM_6_TO_7
        #endif
        #if (uECC_MAX_WORDS > 7)
            FAST_MULT_ASM_7_TO_8
        #endif
    #elif (uECC_MIN_WORDS == 6)
            FAST_MULT_ASM_6
        #if (uECC_MAX_WORDS > 6)
            FAST_MULT_ASM_6_TO_7
        #endif
        #if (uECC_MAX_WORDS > 7)
            FAST_MULT_ASM_7_TO_8
        #endif
    #elif (uECC_MIN_WORDS == 7)
            FAST_MULT_ASM_7
        #if (uECC_MAX_WORDS > 7)
            FAST_MULT_ASM_7_TO_8
        #endif
    #elif (uECC_MIN_WORDS == 8)
            FAST_MULT_ASM_8
    #endif
            "1: \n\t"
            RESUME_SYNTAX
            : "+r" (r0), "+r" (r1), "+r" (r2)
            : "r" (r3)
            : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
        );
    }
    #define asm_mult 1

    #if uECC_SQUARE_FUNC
    uECC_VLI_API void uECC_vli_square(uECC_word_t *result,
                                    const uECC_word_t *left,
                                    wordcount_t num_words) {
        register uint32_t *r0 __asm__("r0") = result;
        register const uint32_t *r1 __asm__("r1") = left;
        register uint32_t r2 __asm__("r2") = num_words;
        
        __asm__ volatile (
            ".syntax unified \n\t"
    #if (uECC_MIN_WORDS == 5)
            FAST_SQUARE_ASM_5
        #if (uECC_MAX_WORDS > 5)
            FAST_SQUARE_ASM_5_TO_6
        #endif
        #if (uECC_MAX_WORDS > 6)
            FAST_SQUARE_ASM_6_TO_7
        #endif
        #if (uECC_MAX_WORDS > 7)
            FAST_SQUARE_ASM_7_TO_8
        #endif
    #elif (uECC_MIN_WORDS == 6)
            FAST_SQUARE_ASM_6
        #if (uECC_MAX_WORDS > 6)
            FAST_SQUARE_ASM_6_TO_7
        #endif
        #if (uECC_MAX_WORDS > 7)
            FAST_SQUARE_ASM_7_TO_8
        #endif
    #elif (uECC_MIN_WORDS == 7)
            FAST_SQUARE_ASM_7
        #if (uECC_MAX_WORDS > 7)
            FAST_SQUARE_ASM_7_TO_8
        #endif
    #elif (uECC_MIN_WORDS == 8)
            FAST_SQUARE_ASM_8
    #endif

            "1: \n\t"
            RESUME_SYNTAX
            : "+r" (r0), "+r" (r1)
            : "r" (r2)
            : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
        );
    }
    #define asm_square 1
    #endif /* uECC_SQUARE_FUNC */

    #else /* (uECC_OPTIMIZATION_LEVEL > 3) */

    uECC_VLI_API void uECC_vli_mult(uint32_t *result,
                                    const uint32_t *left,
                                    const uint32_t *right,
                                    wordcount_t num_words) {
        register uint32_t *r0 __asm__("r0") = result;
        register const uint32_t *r1 __asm__("r1") = left;
        register const uint32_t *r2 __asm__("r2") = right;
        register uint32_t r3 __asm__("r3") = num_words;
        
    #if uECC_SUPPORTS_secp160r1
        if (num_words == 5) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_MULT_ASM_5
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1), "+r" (r2)
                : "r" (r3)
                : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    #if uECC_SUPPORTS_secp192r1
        if (num_words == 6) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_MULT_ASM_6
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1), "+r" (r2)
                : "r" (r3)
                : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    #if uECC_SUPPORTS_secp224r1
        if (num_words == 7) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_MULT_ASM_7
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1), "+r" (r2)
                : "r" (r3)
                : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        if (num_words == 8) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_MULT_ASM_8
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1), "+r" (r2)
                : "r" (r3)
                : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    }
    #define asm_mult 1

    #if uECC_SQUARE_FUNC
    uECC_VLI_API void uECC_vli_square(uECC_word_t *result,
                                    const uECC_word_t *left,
                                    wordcount_t num_words) {
        register uint32_t *r0 __asm__("r0") = result;
        register const uint32_t *r1 __asm__("r1") = left;
        register uint32_t r2 __asm__("r2") = num_words;
        
    #if uECC_SUPPORTS_secp160r1
        if (num_words == 5) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_SQUARE_ASM_5
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1)
                : "r" (r2)
                : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    #if uECC_SUPPORTS_secp192r1
        if (num_words == 6) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_SQUARE_ASM_6
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1)
                : "r" (r2)
                : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    #if uECC_SUPPORTS_secp224r1
        if (num_words == 7) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_SQUARE_ASM_7
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1)
                : "r" (r2)
                : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        if (num_words == 8) {
            __asm__ volatile (
                ".syntax unified \n\t"
                FAST_SQUARE_ASM_8
                RESUME_SYNTAX
                : "+r" (r0), "+r" (r1)
                : "r" (r2)
                : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
            );
            return;
        }
    #endif
    }
    #define asm_square 1
    #endif /* uECC_SQUARE_FUNC */

    #endif /* (uECC_OPTIMIZATION_LEVEL > 3) */

    #endif /* uECC_PLATFORM != uECC_arm_thumb */

    #endif /* (uECC_OPTIMIZATION_LEVEL >= 3) */

    /* ---- "Small" implementations ---- */

    #if !asm_add
    uECC_VLI_API uECC_word_t uECC_vli_add(uECC_word_t *result,
                                        const uECC_word_t *left,
                                        const uECC_word_t *right,
                                        wordcount_t num_words) {
        uint32_t carry = 0;
        uint32_t left_word;
        uint32_t right_word;
        
        __asm__ volatile (
            ".syntax unified \n\t"
            "1: \n\t"
            "ldmia %[lptr]!, {%[left]} \n\t"  /* Load left word. */
            "ldmia %[rptr]!, {%[right]} \n\t" /* Load right word. */
            "lsrs %[carry], #1 \n\t"          /* Set up carry flag (carry = 0 after this). */
            "adcs %[left], %[left], %[right] \n\t"   /* Add with carry. */
            "adcs %[carry], %[carry], %[carry] \n\t" /* Store carry bit. */
            "stmia %[dptr]!, {%[left]} \n\t"  /* Store result word. */
            "subs %[ctr], #1 \n\t"            /* Decrement counter. */
            "bne 1b \n\t"                     /* Loop until counter == 0. */
            RESUME_SYNTAX
            : [dptr] REG_RW (result), [lptr] REG_RW (left), [rptr] REG_RW (right),
            [ctr] REG_RW (num_words), [carry] REG_RW (carry),
            [left] REG_WRITE (left_word), [right] REG_WRITE (right_word)
            :
            : "cc", "memory"
        );
        return carry;
    }
    #define asm_add 1
    #endif

    #if !asm_sub
    uECC_VLI_API uECC_word_t uECC_vli_sub(uECC_word_t *result,
                                        const uECC_word_t *left,
                                        const uECC_word_t *right,
                                        wordcount_t num_words) {
        uint32_t carry = 1; /* carry = 1 initially (means don't borrow) */
        uint32_t left_word;
        uint32_t right_word;
        
        __asm__ volatile (
            ".syntax unified \n\t"
            "1: \n\t"
            "ldmia %[lptr]!, {%[left]} \n\t"  /* Load left word. */
            "ldmia %[rptr]!, {%[right]} \n\t" /* Load right word. */
            "lsrs %[carry], #1 \n\t"          /* Set up carry flag (carry = 0 after this). */
            "sbcs %[left], %[left], %[right] \n\t"   /* Subtract with borrow. */
            "adcs %[carry], %[carry], %[carry] \n\t" /* Store carry bit. */
            "stmia %[dptr]!, {%[left]} \n\t"  /* Store result word. */
            "subs %[ctr], #1 \n\t"            /* Decrement counter. */
            "bne 1b \n\t"                     /* Loop until counter == 0. */
            RESUME_SYNTAX
            : [dptr] REG_RW (result), [lptr] REG_RW (left), [rptr] REG_RW (right),
            [ctr] REG_RW (num_words), [carry] REG_RW (carry),
            [left] REG_WRITE (left_word), [right] REG_WRITE (right_word)
            :
            : "cc", "memory"
        );
        return !carry;
    }
    #define asm_sub 1
    #endif

    #ifndef asm_mult
    uECC_VLI_API void uECC_vli_mult(uECC_word_t *result,
                                    const uECC_word_t *left,
                                    const uECC_word_t *right,
                                    wordcount_t num_words) {
    #if (uECC_PLATFORM != uECC_arm_thumb)
        uint32_t c0 = 0;
        uint32_t c1 = 0;
        uint32_t c2 = 0;
        uint32_t k = 0;
        uint32_t i;
        uint32_t t0, t1;
        
        __asm__ volatile (
            ".syntax unified \n\t"
            
            "1: \n\t" /* outer loop (k < num_words) */
            "movs %[i], #0 \n\t" /* i = 0 */
            "b 3f \n\t"
            
            "2: \n\t" /* outer loop (k >= num_words) */
            "movs %[i], %[k] \n\t"         /* i = k */
            "subs %[i], %[last_word] \n\t" /* i = k - (num_words - 1) (times 4) */
            
            "3: \n\t" /* inner loop */
            "subs %[t0], %[k], %[i] \n\t" /* t0 = k-i */
            
            "ldr %[t1], [%[right], %[t0]] \n\t" /* t1 = right[k - i] */
            "ldr %[t0], [%[left], %[i]] \n\t"   /* t0 = left[i] */
            
            "umull %[t0], %[t1], %[t0], %[t1] \n\t" /* (t0, t1) = left[i] * right[k - i] */
            
            "adds %[c0], %[c0], %[t0] \n\t" /* add low word to c0 */
            "adcs %[c1], %[c1], %[t1] \n\t" /* add high word to c1, including carry */
            "adcs %[c2], %[c2], #0 \n\t"    /* add carry to c2 */

            "adds %[i], #4 \n\t"          /* i += 4 */
            "cmp %[i], %[last_word] \n\t" /* i > (num_words - 1) (times 4)? */
            "bgt 4f \n\t"                 /*   if so, exit the loop */
            "cmp %[i], %[k] \n\t"         /* i <= k? */
            "ble 3b \n\t"                 /*   if so, continue looping */
            
            "4: \n\t" /* end inner loop */
            
            "str %[c0], [%[result], %[k]] \n\t" /* result[k] = c0 */
            "mov %[c0], %[c1] \n\t"       /* c0 = c1 */
            "mov %[c1], %[c2] \n\t"       /* c1 = c2 */
            "movs %[c2], #0 \n\t"         /* c2 = 0 */
            "adds %[k], #4 \n\t"          /* k += 4 */
            "cmp %[k], %[last_word] \n\t" /* k <= (num_words - 1) (times 4) ? */
            "ble 1b \n\t"                 /*   if so, loop back, start with i = 0 */
            "cmp %[k], %[last_word], lsl #1 \n\t" /* k <= (num_words * 2 - 2) (times 4) ? */
            "ble 2b \n\t"                 /*   if so, loop back, start with i = (k + 1) - num_words */
            /* end outer loop */
            
            "str %[c0], [%[result], %[k]] \n\t" /* result[num_words * 2 - 1] = c0 */
            RESUME_SYNTAX
            : [c0] "+r" (c0), [c1] "+r" (c1), [c2] "+r" (c2),
            [k] "+r" (k), [i] "=&r" (i), [t0] "=&r" (t0), [t1] "=&r" (t1)
            : [result] "r" (result), [left] "r" (left), [right] "r" (right),
            [last_word] "r" ((num_words - 1) * 4)
            : "cc", "memory"
        );
        
    #else /* Thumb-1 */
        uint32_t r4, r5, r6, r7;

        __asm__ volatile (
            ".syntax unified \n\t"
            "subs %[r3], #1 \n\t" /* r3 = num_words - 1 */
            "lsls %[r3], #2 \n\t" /* r3 = (num_words - 1) * 4 */
            "mov r8, %[r3] \n\t"  /* r8 = (num_words - 1) * 4 */
            "lsls %[r3], #1 \n\t" /* r3 = (num_words - 1) * 8 */
            "mov r9, %[r3] \n\t"  /* r9 = (num_words - 1) * 8 */
            "movs %[r3], #0 \n\t" /* c0 = 0 */
            "movs %[r4], #0 \n\t" /* c1 = 0 */
            "movs %[r5], #0 \n\t" /* c2 = 0 */
            "movs %[r6], #0 \n\t" /* k = 0 */
            
            "push {%[r0]} \n\t" /* keep result on the stack */
            
            "1: \n\t" /* outer loop (k < num_words) */
            "movs %[r7], #0 \n\t" /* r7 = i = 0 */
            "b 3f \n\t"
            
            "2: \n\t" /* outer loop (k >= num_words) */
            "movs %[r7], %[r6] \n\t" /* r7 = k */
            "mov %[r0], r8 \n\t"     /* r0 = (num_words - 1) * 4 */
            "subs %[r7], %[r0] \n\t" /* r7 = i = k - (num_words - 1) (times 4) */
            
            "3: \n\t" /* inner loop */
            "mov r10, %[r3] \n\t"
            "mov r11, %[r4] \n\t"
            "mov r12, %[r5] \n\t"
            "mov r14, %[r6] \n\t"
            "subs %[r0], %[r6], %[r7] \n\t"          /* r0 = k - i */
            
            "ldr %[r4], [%[r2], %[r0]] \n\t" /* r4 = right[k - i] */
            "ldr %[r0], [%[r1], %[r7]] \n\t" /* r0 = left[i] */
            
            "lsrs %[r3], %[r0], #16 \n\t" /* r3 = a1 */
            "uxth %[r0], %[r0] \n\t"      /* r0 = a0 */
            
            "lsrs %[r5], %[r4], #16 \n\t" /* r5 = b1 */
            "uxth %[r4], %[r4] \n\t"      /* r4 = b0 */
            
            "movs %[r6], %[r3] \n\t"        /* r6 = a1 */
            "muls %[r6], %[r5], %[r6] \n\t" /* r6 = a1 * b1 */
            "muls %[r3], %[r4], %[r3] \n\t" /* r3 = b0 * a1 */
            "muls %[r5], %[r0], %[r5] \n\t" /* r5 = a0 * b1 */
            "muls %[r0], %[r4], %[r0] \n\t" /* r0 = a0 * b0 */
            
            /* Add middle terms */
            "lsls %[r4], %[r3], #16 \n\t"
            "lsrs %[r3], %[r3], #16 \n\t"
            "adds %[r0], %[r4] \n\t"
            "adcs %[r6], %[r3] \n\t"
            
            "lsls %[r4], %[r5], #16 \n\t"
            "lsrs %[r5], %[r5], #16 \n\t"
            "adds %[r0], %[r4] \n\t"
            "adcs %[r6], %[r5] \n\t"
            
            "mov %[r3], r10\n\t"
            "mov %[r4], r11\n\t"
            "mov %[r5], r12\n\t"
            "adds %[r3], %[r0] \n\t"         /* add low word to c0 */
            "adcs %[r4], %[r6] \n\t"         /* add high word to c1, including carry */
            "movs %[r0], #0 \n\t"            /* r0 = 0 (does not affect carry bit) */
            "adcs %[r5], %[r0] \n\t"         /* add carry to c2 */
            
            "mov %[r6], r14\n\t" /* r6 = k */

            "adds %[r7], #4 \n\t"   /* i += 4 */
            "cmp %[r7], r8 \n\t"    /* i > (num_words - 1) (times 4)? */
            "bgt 4f \n\t"           /*   if so, exit the loop */
            "cmp %[r7], %[r6] \n\t" /* i <= k? */
            "ble 3b \n\t"           /*   if so, continue looping */
            
            "4: \n\t" /* end inner loop */
            
            "ldr %[r0], [sp, #0] \n\t" /* r0 = result */
            
            "str %[r3], [%[r0], %[r6]] \n\t" /* result[k] = c0 */
            "mov %[r3], %[r4] \n\t"          /* c0 = c1 */
            "mov %[r4], %[r5] \n\t"          /* c1 = c2 */
            "movs %[r5], #0 \n\t"            /* c2 = 0 */
            "adds %[r6], #4 \n\t"            /* k += 4 */
            "cmp %[r6], r8 \n\t"             /* k <= (num_words - 1) (times 4) ? */
            "ble 1b \n\t"                    /*   if so, loop back, start with i = 0 */
            "cmp %[r6], r9 \n\t"             /* k <= (num_words * 2 - 2) (times 4) ? */
            "ble 2b \n\t"                    /*   if so, loop back, with i = (k + 1) - num_words */
            /* end outer loop */
            
            "str %[r3], [%[r0], %[r6]] \n\t" /* result[num_words * 2 - 1] = c0 */
            "pop {%[r0]} \n\t"               /* pop result off the stack */
            
            ".syntax divided \n\t"
            : [r3] "+l" (num_words), [r4] "=&l" (r4),
            [r5] "=&l" (r5), [r6] "=&l" (r6), [r7] "=&l" (r7)
            : [r0] "l" (result), [r1] "l" (left), [r2] "l" (right)
            : "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
        );
    #endif
    }
    #define asm_mult 1
    #endif

    #if uECC_SQUARE_FUNC
    #if !asm_square
    uECC_VLI_API void uECC_vli_square(uECC_word_t *result,
                                    const uECC_word_t *left,
                                    wordcount_t num_words) {
    #if (uECC_PLATFORM != uECC_arm_thumb)
        uint32_t c0 = 0;
        uint32_t c1 = 0;
        uint32_t c2 = 0;
        uint32_t k = 0;
        uint32_t i, tt;
        uint32_t t0, t1;
        
        __asm__ volatile (
            ".syntax unified \n\t"
            
            "1: \n\t" /* outer loop (k < num_words) */
            "movs %[i], #0 \n\t" /* i = 0 */
            "b 3f \n\t"
            
            "2: \n\t" /* outer loop (k >= num_words) */
            "movs %[i], %[k] \n\t"         /* i = k */
            "subs %[i], %[last_word] \n\t" /* i = k - (num_words - 1) (times 4) */
            
            "3: \n\t" /* inner loop */
            "subs %[tt], %[k], %[i] \n\t" /* tt = k-i */
            
            "ldr %[t1], [%[left], %[tt]] \n\t" /* t1 = left[k - i] */
            "ldr %[t0], [%[left], %[i]] \n\t"  /* t0 = left[i] */
            
            "umull %[t0], %[t1], %[t0], %[t1] \n\t" /* (t0, t1) = left[i] * right[k - i] */
            
            "cmp %[i], %[tt] \n\t"      /* (i < k - i) ? */
            "bge 4f \n\t"               /*   if i >= k - i, skip */
            "adds %[c0], %[c0], %[t0] \n\t" /* add low word to c0 */
            "adcs %[c1], %[c1], %[t1] \n\t" /* add high word to c1, including carry */
            "adcs %[c2], %[c2], #0 \n\t"    /* add carry to c2 */
            
            "4: \n\t"
            "adds %[c0], %[c0], %[t0] \n\t" /* add low word to c0 */
            "adcs %[c1], %[c1], %[t1] \n\t" /* add high word to c1, including carry */
            "adcs %[c2], %[c2], #0 \n\t"    /* add carry to c2 */
            
            "adds %[i], #4 \n\t"          /* i += 4 */
            "cmp %[i], %[k] \n\t"         /* i >= k? */
            "bge 5f \n\t"                 /*   if so, exit the loop */
            "subs %[tt], %[k], %[i] \n\t" /* tt = k - i */
            "cmp %[i], %[tt] \n\t"        /* i <= k - i? */
            "ble 3b \n\t"                 /*   if so, continue looping */
            
            "5: \n\t" /* end inner loop */
            
            "str %[c0], [%[result], %[k]] \n\t" /* result[k] = c0 */
            "mov %[c0], %[c1] \n\t"       /* c0 = c1 */
            "mov %[c1], %[c2] \n\t"       /* c1 = c2 */
            "movs %[c2], #0 \n\t"         /* c2 = 0 */
            "adds %[k], #4 \n\t"          /* k += 4 */
            "cmp %[k], %[last_word] \n\t" /* k <= (num_words - 1) (times 4) ? */
            "ble 1b \n\t"                 /*   if so, loop back, start with i = 0 */
            "cmp %[k], %[last_word], lsl #1 \n\t" /* k <= (num_words * 2 - 2) (times 4) ? */
            "ble 2b \n\t"                 /*   if so, loop back, start with i = (k + 1) - num_words */
            /* end outer loop */
            
            "str %[c0], [%[result], %[k]] \n\t" /* result[num_words * 2 - 1] = c0 */
            RESUME_SYNTAX
            : [c0] "+r" (c0), [c1] "+r" (c1), [c2] "+r" (c2),
            [k] "+r" (k), [i] "=&r" (i), [tt] "=&r" (tt), [t0] "=&r" (t0), [t1] "=&r" (t1)
            : [result] "r" (result), [left] "r" (left), [last_word] "r" ((num_words - 1) * 4)
            : "cc", "memory"
        );
        
    #else
        uint32_t r3, r4, r5, r6, r7;

        __asm__ volatile (
            ".syntax unified \n\t"
            "subs %[r2], #1 \n\t" /* r2 = num_words - 1 */
            "lsls %[r2], #2 \n\t" /* r2 = (num_words - 1) * 4 */
            "mov r8, %[r2] \n\t"  /* r8 = (num_words - 1) * 4 */
            "lsls %[r2], #1 \n\t" /* r2 = (num_words - 1) * 8 */
            "mov r9, %[r2] \n\t"  /* r9 = (num_words - 1) * 8 */
            "movs %[r2], #0 \n\t" /* c0 = 0 */
            "movs %[r3], #0 \n\t" /* c1 = 0 */
            "movs %[r4], #0 \n\t" /* c2 = 0 */
            "movs %[r5], #0 \n\t" /* k = 0 */
            
            "push {%[r0]} \n\t" /* keep result on the stack */
            
            "1: \n\t" /* outer loop (k < num_words) */
            "movs %[r6], #0 \n\t" /* r6 = i = 0 */
            "b 3f \n\t"
            
            "2: \n\t" /* outer loop (k >= num_words) */
            "movs %[r6], %[r5] \n\t" /* r6 = k */
            "mov %[r0], r8 \n\t"     /* r0 = (num_words - 1) * 4 */
            "subs %[r6], %[r0] \n\t" /* r6 = i = k - (num_words - 1) (times 4) */
            
            "3: \n\t" /* inner loop */
            "mov r10, %[r2] \n\t"
            "mov r11, %[r3] \n\t"
            "mov r12, %[r4] \n\t"
            "mov r14, %[r5] \n\t"
            "subs %[r7], %[r5], %[r6] \n\t"  /* r7 = k - i */
            
            "ldr %[r3], [%[r1], %[r7]] \n\t" /* r3 = left[k - i] */
            "ldr %[r0], [%[r1], %[r6]] \n\t" /* r0 = left[i] */
            
            "lsrs %[r2], %[r0], #16 \n\t" /* r2 = a1 */
            "uxth %[r0], %[r0] \n\t"      /* r0 = a0 */
            
            "lsrs %[r4], %[r3], #16 \n\t" /* r4 = b1 */
            "uxth %[r3], %[r3] \n\t"      /* r3 = b0 */
            
            "movs %[r5], %[r2] \n\t"        /* r5 = a1 */
            "muls %[r5], %[r4], %[r5] \n\t" /* r5 = a1 * b1 */
            "muls %[r2], %[r3], %[r2] \n\t" /* r2 = b0 * a1 */
            "muls %[r4], %[r0], %[r4] \n\t" /* r4 = a0 * b1 */
            "muls %[r0], %[r3], %[r0] \n\t" /* r0 = a0 * b0 */
            
            /* Add middle terms */
            "lsls %[r3], %[r2], #16 \n\t"
            "lsrs %[r2], %[r2], #16 \n\t"
            "adds %[r0], %[r3] \n\t"
            "adcs %[r5], %[r2] \n\t"
            
            "lsls %[r3], %[r4], #16 \n\t"
            "lsrs %[r4], %[r4], #16 \n\t"
            "adds %[r0], %[r3] \n\t"
            "adcs %[r5], %[r4] \n\t"
            
            /* Add to acc, doubling if necessary */
            "mov %[r2], r10\n\t"
            "mov %[r3], r11\n\t"
            "mov %[r4], r12\n\t"
            
            "cmp %[r6], %[r7] \n\t"    /* (i < k - i) ? */
            "bge 4f \n\t"            /*   if i >= k - i, skip */
            "movs %[r7], #0 \n\t"    /* r7 = 0 */
            "adds %[r2], %[r0] \n\t" /* add low word to c0 */
            "adcs %[r3], %[r5] \n\t" /* add high word to c1, including carry */
            "adcs %[r4], %[r7] \n\t" /* add carry to c2 */
            "4: \n\t"
            "movs %[r7], #0 \n\t"    /* r7 = 0 */
            "adds %[r2], %[r0] \n\t" /* add low word to c0 */
            "adcs %[r3], %[r5] \n\t" /* add high word to c1, including carry */
            "adcs %[r4], %[r7] \n\t" /* add carry to c2 */
            
            "mov %[r5], r14\n\t" /* r5 = k */
            
            "adds %[r6], #4 \n\t"           /* i += 4 */
            "cmp %[r6], %[r5] \n\t"         /* i >= k? */
            "bge 5f \n\t"                   /*   if so, exit the loop */
            "subs %[r7], %[r5], %[r6] \n\t" /* r7 = k - i */
            "cmp %[r6], %[r7] \n\t"         /* i <= k - i? */
            "ble 3b \n\t"                   /*   if so, continue looping */
            
            "5: \n\t" /* end inner loop */
            
            "ldr %[r0], [sp, #0] \n\t" /* r0 = result */
            
            "str %[r2], [%[r0], %[r5]] \n\t" /* result[k] = c0 */
            "mov %[r2], %[r3] \n\t"          /* c0 = c1 */
            "mov %[r3], %[r4] \n\t"          /* c1 = c2 */
            "movs %[r4], #0 \n\t"            /* c2 = 0 */
            "adds %[r5], #4 \n\t"            /* k += 4 */
            "cmp %[r5], r8 \n\t"             /* k <= (num_words - 1) (times 4) ? */
            "ble 1b \n\t"                    /*   if so, loop back, start with i = 0 */
            "cmp %[r5], r9 \n\t"             /* k <= (num_words * 2 - 2) (times 4) ? */
            "ble 2b \n\t"                    /*   if so, loop back, with i = (k + 1) - num_words */
            /* end outer loop */
            
            "str %[r2], [%[r0], %[r5]] \n\t" /* result[num_words * 2 - 1] = c0 */
            "pop {%[r0]} \n\t"               /* pop result off the stack */

            ".syntax divided \n\t"
            : [r2] "+l" (num_words), [r3] "=&l" (r3), [r4] "=&l" (r4),
            [r5] "=&l" (r5), [r6] "=&l" (r6), [r7] "=&l" (r7)
            : [r0] "l" (result), [r1] "l" (left)
            : "r8", "r9", "r10", "r11", "r12", "r14", "cc", "memory"
        );
    #endif
    }
    #define asm_square 1
    #endif
    #endif /* uECC_SQUARE_FUNC */

    #endif /* _UECC_ASM_ARM_H_ */

    // End manual include asm_arm.inc
#endif

#if (uECC_PLATFORM == uECC_avr)
    #include "asm_avr.inc"
#endif

#ifdef default_RNG_defined
static uECC_RNG_Function g_rng_function = &default_RNG;
#else
static uECC_RNG_Function g_rng_function = 0;
#endif

void uECC_set_rng(uECC_RNG_Function rng_function) {
    g_rng_function = rng_function;
}

uECC_RNG_Function uECC_get_rng(void) {
    return g_rng_function;
}

int uECC_curve_private_key_size(uECC_Curve curve) {
    return BITS_TO_BYTES(curve->num_n_bits);
}

int uECC_curve_public_key_size(uECC_Curve curve) {
    return 2 * curve->num_bytes;
}

#ifndef asm_clear
uECC_VLI_API void uECC_vli_clear(uECC_word_t *vli, wordcount_t num_words) {
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        vli[i] = 0;
    }
}
#endif /* !asm_clear */

/* Constant-time comparison to zero - secure way to compare long integers */
/* Returns 1 if vli == 0, 0 otherwise. */
uECC_VLI_API uECC_word_t uECC_vli_isZero(const uECC_word_t *vli, wordcount_t num_words) {
    uECC_word_t bits = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        bits |= vli[i];
    }
    return (bits == 0);
}

/* Returns nonzero if bit 'bit' of vli is set. */
uECC_VLI_API uECC_word_t uECC_vli_testBit(const uECC_word_t *vli, bitcount_t bit) {
    return (vli[bit >> uECC_WORD_BITS_SHIFT] & ((uECC_word_t)1 << (bit & uECC_WORD_BITS_MASK)));
}

/* Counts the number of words in vli. */
static wordcount_t vli_numDigits(const uECC_word_t *vli, const wordcount_t max_words) {
    wordcount_t i;
    /* Search from the end until we find a non-zero digit.
       We do it in reverse because we expect that most digits will be nonzero. */
    for (i = max_words - 1; i >= 0 && vli[i] == 0; --i) {
    }

    return (i + 1);
}

/* Counts the number of bits required to represent vli. */
uECC_VLI_API bitcount_t uECC_vli_numBits(const uECC_word_t *vli, const wordcount_t max_words) {
    uECC_word_t i;
    uECC_word_t digit;

    wordcount_t num_digits = vli_numDigits(vli, max_words);
    if (num_digits == 0) {
        return 0;
    }

    digit = vli[num_digits - 1];
    for (i = 0; digit; ++i) {
        digit >>= 1;
    }

    return (((bitcount_t)(num_digits - 1) << uECC_WORD_BITS_SHIFT) + i);
}

/* Sets dest = src. */
#ifndef asm_set
uECC_VLI_API void uECC_vli_set(uECC_word_t *dest, const uECC_word_t *src, wordcount_t num_words) {
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        dest[i] = src[i];
    }
}
#endif /* !asm_set */

/* Returns sign of left - right. */
static cmpresult_t uECC_vli_cmp_unsafe(const uECC_word_t *left,
                                       const uECC_word_t *right,
                                       wordcount_t num_words) {
    wordcount_t i;
    for (i = num_words - 1; i >= 0; --i) {
        if (left[i] > right[i]) {
            return 1;
        } else if (left[i] < right[i]) {
            return -1;
        }
    }
    return 0;
}

/* Constant-time comparison function - secure way to compare long integers */
/* Returns one if left == right, zero otherwise. */
uECC_VLI_API uECC_word_t uECC_vli_equal(const uECC_word_t *left,
                                        const uECC_word_t *right,
                                        wordcount_t num_words) {
    uECC_word_t diff = 0;
    wordcount_t i;
    for (i = num_words - 1; i >= 0; --i) {
        diff |= (left[i] ^ right[i]);
    }
    return (diff == 0);
}

uECC_VLI_API uECC_word_t uECC_vli_sub(uECC_word_t *result,
                                      const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words);

/* Returns sign of left - right, in constant time. */
uECC_VLI_API cmpresult_t uECC_vli_cmp(const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words) {
    uECC_word_t tmp[uECC_MAX_WORDS];
    uECC_word_t neg = !!uECC_vli_sub(tmp, left, right, num_words);
    uECC_word_t equal = uECC_vli_isZero(tmp, num_words);
    return (!equal - 2 * neg);
}

/* Computes vli = vli >> 1. */
#ifndef asm_rshift1
uECC_VLI_API void uECC_vli_rshift1(uECC_word_t *vli, wordcount_t num_words) {
    uECC_word_t *end = vli;
    uECC_word_t carry = 0;

    vli += num_words;
    while (vli-- > end) {
        uECC_word_t temp = *vli;
        *vli = (temp >> 1) | carry;
        carry = temp << (uECC_WORD_BITS - 1);
    }
}
#endif /* !asm_rshift1 */

/* Computes result = left + right, returning carry. Can modify in place. */
#if !asm_add
uECC_VLI_API uECC_word_t uECC_vli_add(uECC_word_t *result,
                                      const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words) {
    uECC_word_t carry = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        uECC_word_t sum = left[i] + right[i] + carry;
        if (sum != left[i]) {
            carry = (sum < left[i]);
        }
        result[i] = sum;
    }
    return carry;
}
#endif /* !asm_add */

/* Computes result = left - right, returning borrow. Can modify in place. */
#if !asm_sub
uECC_VLI_API uECC_word_t uECC_vli_sub(uECC_word_t *result,
                                      const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words) {
    uECC_word_t borrow = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        uECC_word_t diff = left[i] - right[i] - borrow;
        if (diff != left[i]) {
            borrow = (diff > left[i]);
        }
        result[i] = diff;
    }
    return borrow;
}
#endif /* !asm_sub */

#if !asm_mult || (uECC_SQUARE_FUNC && !asm_square) || \
    (uECC_SUPPORTS_secp256k1 && (uECC_OPTIMIZATION_LEVEL > 0) && \
        ((uECC_WORD_SIZE == 1) || (uECC_WORD_SIZE == 8)))
static void muladd(uECC_word_t a,
                   uECC_word_t b,
                   uECC_word_t *r0,
                   uECC_word_t *r1,
                   uECC_word_t *r2) {
#if uECC_WORD_SIZE == 8 && !SUPPORTS_INT128
    uint64_t a0 = a & 0xffffffffull;
    uint64_t a1 = a >> 32;
    uint64_t b0 = b & 0xffffffffull;
    uint64_t b1 = b >> 32;

    uint64_t i0 = a0 * b0;
    uint64_t i1 = a0 * b1;
    uint64_t i2 = a1 * b0;
    uint64_t i3 = a1 * b1;

    uint64_t p0, p1;

    i2 += (i0 >> 32);
    i2 += i1;
    if (i2 < i1) { /* overflow */
        i3 += 0x100000000ull;
    }

    p0 = (i0 & 0xffffffffull) | (i2 << 32);
    p1 = i3 + (i2 >> 32);

    *r0 += p0;
    *r1 += (p1 + (*r0 < p0));
    *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#else
    uECC_dword_t p = (uECC_dword_t)a * b;
    uECC_dword_t r01 = ((uECC_dword_t)(*r1) << uECC_WORD_BITS) | *r0;
    r01 += p;
    *r2 += (r01 < p);
    *r1 = r01 >> uECC_WORD_BITS;
    *r0 = (uECC_word_t)r01;
#endif
}
#endif /* muladd needed */

#if !asm_mult
uECC_VLI_API void uECC_vli_mult(uECC_word_t *result,
                                const uECC_word_t *left,
                                const uECC_word_t *right,
                                wordcount_t num_words) {
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;
    wordcount_t i, k;

    /* Compute each digit of result in sequence, maintaining the carries. */
    for (k = 0; k < num_words; ++k) {
        for (i = 0; i <= k; ++i) {
            muladd(left[i], right[k - i], &r0, &r1, &r2);
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    for (k = num_words; k < num_words * 2 - 1; ++k) {
        for (i = (k + 1) - num_words; i < num_words; ++i) {
            muladd(left[i], right[k - i], &r0, &r1, &r2);
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    result[num_words * 2 - 1] = r0;
}
#endif /* !asm_mult */

#if uECC_SQUARE_FUNC

#if !asm_square
static void mul2add(uECC_word_t a,
                    uECC_word_t b,
                    uECC_word_t *r0,
                    uECC_word_t *r1,
                    uECC_word_t *r2) {
#if uECC_WORD_SIZE == 8 && !SUPPORTS_INT128
    uint64_t a0 = a & 0xffffffffull;
    uint64_t a1 = a >> 32;
    uint64_t b0 = b & 0xffffffffull;
    uint64_t b1 = b >> 32;

    uint64_t i0 = a0 * b0;
    uint64_t i1 = a0 * b1;
    uint64_t i2 = a1 * b0;
    uint64_t i3 = a1 * b1;

    uint64_t p0, p1;

    i2 += (i0 >> 32);
    i2 += i1;
    if (i2 < i1)
    { /* overflow */
        i3 += 0x100000000ull;
    }

    p0 = (i0 & 0xffffffffull) | (i2 << 32);
    p1 = i3 + (i2 >> 32);

    *r2 += (p1 >> 63);
    p1 = (p1 << 1) | (p0 >> 63);
    p0 <<= 1;

    *r0 += p0;
    *r1 += (p1 + (*r0 < p0));
    *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#else
    uECC_dword_t p = (uECC_dword_t)a * b;
    uECC_dword_t r01 = ((uECC_dword_t)(*r1) << uECC_WORD_BITS) | *r0;
    *r2 += (p >> (uECC_WORD_BITS * 2 - 1));
    p *= 2;
    r01 += p;
    *r2 += (r01 < p);
    *r1 = r01 >> uECC_WORD_BITS;
    *r0 = (uECC_word_t)r01;
#endif
}

uECC_VLI_API void uECC_vli_square(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  wordcount_t num_words) {
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;

    wordcount_t i, k;

    for (k = 0; k < num_words * 2 - 1; ++k) {
        uECC_word_t min = (k < num_words ? 0 : (k + 1) - num_words);
        for (i = min; i <= k && i <= k - i; ++i) {
            if (i < k-i) {
                mul2add(left[i], left[k - i], &r0, &r1, &r2);
            } else {
                muladd(left[i], left[k - i], &r0, &r1, &r2);
            }
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }

    result[num_words * 2 - 1] = r0;
}
#endif /* !asm_square */

#else /* uECC_SQUARE_FUNC */

#if uECC_ENABLE_VLI_API
uECC_VLI_API void uECC_vli_square(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  wordcount_t num_words) {
    uECC_vli_mult(result, left, left, num_words);
}
#endif /* uECC_ENABLE_VLI_API */

#endif /* uECC_SQUARE_FUNC */

/* Computes result = (left + right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
uECC_VLI_API void uECC_vli_modAdd(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  const uECC_word_t *right,
                                  const uECC_word_t *mod,
                                  wordcount_t num_words) {
    uECC_word_t carry = uECC_vli_add(result, left, right, num_words);
    if (carry || uECC_vli_cmp_unsafe(mod, result, num_words) != 1) {
        /* result > mod (result = mod + remainder), so subtract mod to get remainder. */
        uECC_vli_sub(result, result, mod, num_words);
    }
}

/* Computes result = (left - right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
uECC_VLI_API void uECC_vli_modSub(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  const uECC_word_t *right,
                                  const uECC_word_t *mod,
                                  wordcount_t num_words) {
    uECC_word_t l_borrow = uECC_vli_sub(result, left, right, num_words);
    if (l_borrow) {
        /* In this case, result == -diff == (max int) - diff. Since -x % d == d - x,
           we can get the correct result from result + mod (with overflow). */
        uECC_vli_add(result, result, mod, num_words);
    }
}

/* Computes result = product % mod, where product is 2N words long. */
/* Currently only designed to work for curve_p or curve_n. */
uECC_VLI_API void uECC_vli_mmod(uECC_word_t *result,
                                uECC_word_t *product,
                                const uECC_word_t *mod,
                                wordcount_t num_words) {
    uECC_word_t mod_multiple[2 * uECC_MAX_WORDS];
    uECC_word_t tmp[2 * uECC_MAX_WORDS];
    uECC_word_t *v[2] = {tmp, product};
    uECC_word_t index;

    /* Shift mod so its highest set bit is at the maximum position. */
    bitcount_t shift = (num_words * 2 * uECC_WORD_BITS) - uECC_vli_numBits(mod, num_words);
    wordcount_t word_shift = shift / uECC_WORD_BITS;
    wordcount_t bit_shift = shift % uECC_WORD_BITS;
    uECC_word_t carry = 0;
    uECC_vli_clear(mod_multiple, word_shift);
    if (bit_shift > 0) {
        for(index = 0; index < (uECC_word_t)num_words; ++index) {
            mod_multiple[word_shift + index] = (mod[index] << bit_shift) | carry;
            carry = mod[index] >> (uECC_WORD_BITS - bit_shift);
        }
    } else {
        uECC_vli_set(mod_multiple + word_shift, mod, num_words);
    }

    for (index = 1; shift >= 0; --shift) {
        uECC_word_t borrow = 0;
        wordcount_t i;
        for (i = 0; i < num_words * 2; ++i) {
            uECC_word_t diff = v[index][i] - mod_multiple[i] - borrow;
            if (diff != v[index][i]) {
                borrow = (diff > v[index][i]);
            }
            v[1 - index][i] = diff;
        }
        index = !(index ^ borrow); /* Swap the index if there was no borrow */
        uECC_vli_rshift1(mod_multiple, num_words);
        mod_multiple[num_words - 1] |= mod_multiple[num_words] << (uECC_WORD_BITS - 1);
        uECC_vli_rshift1(mod_multiple + num_words, num_words);
    }
    uECC_vli_set(result, v[index], num_words);
}

/* Computes result = (left * right) % mod. */
uECC_VLI_API void uECC_vli_modMult(uECC_word_t *result,
                                   const uECC_word_t *left,
                                   const uECC_word_t *right,
                                   const uECC_word_t *mod,
                                   wordcount_t num_words) {
    uECC_word_t product[2 * uECC_MAX_WORDS];
    uECC_vli_mult(product, left, right, num_words);
    uECC_vli_mmod(result, product, mod, num_words);
}

uECC_VLI_API void uECC_vli_modMult_fast(uECC_word_t *result,
                                        const uECC_word_t *left,
                                        const uECC_word_t *right,
                                        uECC_Curve curve) {
    uECC_word_t product[2 * uECC_MAX_WORDS];
    uECC_vli_mult(product, left, right, curve->num_words);
#if (uECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    uECC_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

#if uECC_SQUARE_FUNC

#if uECC_ENABLE_VLI_API
/* Computes result = left^2 % mod. */
uECC_VLI_API void uECC_vli_modSquare(uECC_word_t *result,
                                     const uECC_word_t *left,
                                     const uECC_word_t *mod,
                                     wordcount_t num_words) {
    uECC_word_t product[2 * uECC_MAX_WORDS];
    uECC_vli_square(product, left, num_words);
    uECC_vli_mmod(result, product, mod, num_words);
}
#endif /* uECC_ENABLE_VLI_API */

uECC_VLI_API void uECC_vli_modSquare_fast(uECC_word_t *result,
                                          const uECC_word_t *left,
                                          uECC_Curve curve) {
    uECC_word_t product[2 * uECC_MAX_WORDS];
    uECC_vli_square(product, left, curve->num_words);
#if (uECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    uECC_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

#else /* uECC_SQUARE_FUNC */

#if uECC_ENABLE_VLI_API
uECC_VLI_API void uECC_vli_modSquare(uECC_word_t *result,
                                     const uECC_word_t *left,
                                     const uECC_word_t *mod,
                                     wordcount_t num_words) {
    uECC_vli_modMult(result, left, left, mod, num_words);
}
#endif /* uECC_ENABLE_VLI_API */

uECC_VLI_API void uECC_vli_modSquare_fast(uECC_word_t *result,
                                          const uECC_word_t *left,
                                          uECC_Curve curve) {
    uECC_vli_modMult_fast(result, left, left, curve);
}

#endif /* uECC_SQUARE_FUNC */

#define EVEN(vli) (!(vli[0] & 1))
static void vli_modInv_update(uECC_word_t *uv,
                              const uECC_word_t *mod,
                              wordcount_t num_words) {
    uECC_word_t carry = 0;
    if (!EVEN(uv)) {
        carry = uECC_vli_add(uv, uv, mod, num_words);
    }
    uECC_vli_rshift1(uv, num_words);
    if (carry) {
        uv[num_words - 1] |= HIGH_BIT_SET;
    }
}

/* Computes result = (1 / input) % mod. All VLIs are the same size.
   See "From Euclid's GCD to Montgomery Multiplication to the Great Divide" */
uECC_VLI_API void uECC_vli_modInv(uECC_word_t *result,
                                  const uECC_word_t *input,
                                  const uECC_word_t *mod,
                                  wordcount_t num_words) {
    uECC_word_t a[uECC_MAX_WORDS], b[uECC_MAX_WORDS], u[uECC_MAX_WORDS], v[uECC_MAX_WORDS];
    cmpresult_t cmpResult;

    if (uECC_vli_isZero(input, num_words)) {
        uECC_vli_clear(result, num_words);
        return;
    }

    uECC_vli_set(a, input, num_words);
    uECC_vli_set(b, mod, num_words);
    uECC_vli_clear(u, num_words);
    u[0] = 1;
    uECC_vli_clear(v, num_words);
    while ((cmpResult = uECC_vli_cmp_unsafe(a, b, num_words)) != 0) {
        if (EVEN(a)) {
            uECC_vli_rshift1(a, num_words);
            vli_modInv_update(u, mod, num_words);
        } else if (EVEN(b)) {
            uECC_vli_rshift1(b, num_words);
            vli_modInv_update(v, mod, num_words);
        } else if (cmpResult > 0) {
            uECC_vli_sub(a, a, b, num_words);
            uECC_vli_rshift1(a, num_words);
            if (uECC_vli_cmp_unsafe(u, v, num_words) < 0) {
                uECC_vli_add(u, u, mod, num_words);
            }
            uECC_vli_sub(u, u, v, num_words);
            vli_modInv_update(u, mod, num_words);
        } else {
            uECC_vli_sub(b, b, a, num_words);
            uECC_vli_rshift1(b, num_words);
            if (uECC_vli_cmp_unsafe(v, u, num_words) < 0) {
                uECC_vli_add(v, v, mod, num_words);
            }
            uECC_vli_sub(v, v, u, num_words);
            vli_modInv_update(v, mod, num_words);
        }
    }
    uECC_vli_set(result, u, num_words);
}

/* ------ Point operations ------ */

// #include "curve-specific.inc"
// Begin manual include curve-specific.inc

/* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#ifndef _UECC_CURVE_SPECIFIC_H_
#define _UECC_CURVE_SPECIFIC_H_

#define num_bytes_secp160r1 20
#define num_bytes_secp192r1 24
#define num_bytes_secp224r1 28
#define num_bytes_secp256r1 32
#define num_bytes_secp256k1 32

#if (uECC_WORD_SIZE == 1)

#define num_words_secp160r1 20
#define num_words_secp192r1 24
#define num_words_secp224r1 28
#define num_words_secp256r1 32
#define num_words_secp256k1 32

#define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h) \
    0x##a, 0x##b, 0x##c, 0x##d, 0x##e, 0x##f, 0x##g, 0x##h
#define BYTES_TO_WORDS_4(a, b, c, d) 0x##a, 0x##b, 0x##c, 0x##d

#elif (uECC_WORD_SIZE == 4)

#define num_words_secp160r1 5
#define num_words_secp192r1 6
#define num_words_secp224r1 7
#define num_words_secp256r1 8
#define num_words_secp256k1 8

#define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h) 0x##d##c##b##a, 0x##h##g##f##e
#define BYTES_TO_WORDS_4(a, b, c, d) 0x##d##c##b##a

#elif (uECC_WORD_SIZE == 8)

#define num_words_secp160r1 3
#define num_words_secp192r1 3
#define num_words_secp224r1 4
#define num_words_secp256r1 4
#define num_words_secp256k1 4

#define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h) 0x##h##g##f##e##d##c##b##a##ull
#define BYTES_TO_WORDS_4(a, b, c, d) 0x##d##c##b##a##ull

#endif /* uECC_WORD_SIZE */

#if uECC_SUPPORTS_secp160r1 || uECC_SUPPORTS_secp192r1 || \
    uECC_SUPPORTS_secp224r1 || uECC_SUPPORTS_secp256r1
static void double_jacobian_default(uECC_word_t * X1,
                                    uECC_word_t * Y1,
                                    uECC_word_t * Z1,
                                    uECC_Curve curve) {
    /* t1 = X, t2 = Y, t3 = Z */
    uECC_word_t t4[uECC_MAX_WORDS];
    uECC_word_t t5[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;

    if (uECC_vli_isZero(Z1, num_words)) {
        return;
    }

    uECC_vli_modSquare_fast(t4, Y1, curve);   /* t4 = y1^2 */
    uECC_vli_modMult_fast(t5, X1, t4, curve); /* t5 = x1*y1^2 = A */
    uECC_vli_modSquare_fast(t4, t4, curve);   /* t4 = y1^4 */
    uECC_vli_modMult_fast(Y1, Y1, Z1, curve); /* t2 = y1*z1 = z3 */
    uECC_vli_modSquare_fast(Z1, Z1, curve);   /* t3 = z1^2 */

    uECC_vli_modAdd(X1, X1, Z1, curve->p, num_words); /* t1 = x1 + z1^2 */
    uECC_vli_modAdd(Z1, Z1, Z1, curve->p, num_words); /* t3 = 2*z1^2 */
    uECC_vli_modSub(Z1, X1, Z1, curve->p, num_words); /* t3 = x1 - z1^2 */
    uECC_vli_modMult_fast(X1, X1, Z1, curve);                /* t1 = x1^2 - z1^4 */

    uECC_vli_modAdd(Z1, X1, X1, curve->p, num_words); /* t3 = 2*(x1^2 - z1^4) */
    uECC_vli_modAdd(X1, X1, Z1, curve->p, num_words); /* t1 = 3*(x1^2 - z1^4) */
    if (uECC_vli_testBit(X1, 0)) {
        uECC_word_t l_carry = uECC_vli_add(X1, X1, curve->p, num_words);
        uECC_vli_rshift1(X1, num_words);
        X1[num_words - 1] |= l_carry << (uECC_WORD_BITS - 1);
    } else {
        uECC_vli_rshift1(X1, num_words);
    }
    /* t1 = 3/2*(x1^2 - z1^4) = B */

    uECC_vli_modSquare_fast(Z1, X1, curve);                  /* t3 = B^2 */
    uECC_vli_modSub(Z1, Z1, t5, curve->p, num_words); /* t3 = B^2 - A */
    uECC_vli_modSub(Z1, Z1, t5, curve->p, num_words); /* t3 = B^2 - 2A = x3 */
    uECC_vli_modSub(t5, t5, Z1, curve->p, num_words); /* t5 = A - x3 */
    uECC_vli_modMult_fast(X1, X1, t5, curve);                /* t1 = B * (A - x3) */
    uECC_vli_modSub(t4, X1, t4, curve->p, num_words); /* t4 = B * (A - x3) - y1^4 = y3 */

    uECC_vli_set(X1, Z1, num_words);
    uECC_vli_set(Z1, Y1, num_words);
    uECC_vli_set(Y1, t4, num_words);
}

/* Computes result = x^3 + ax + b. result must not overlap x. */
static void x_side_default(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve) {
    uECC_word_t _3[uECC_MAX_WORDS] = {3}; /* -a = 3 */
    wordcount_t num_words = curve->num_words;

    uECC_vli_modSquare_fast(result, x, curve);                             /* r = x^2 */
    uECC_vli_modSub(result, result, _3, curve->p, num_words);       /* r = x^2 - 3 */
    uECC_vli_modMult_fast(result, result, x, curve);                       /* r = x^3 - 3x */
    uECC_vli_modAdd(result, result, curve->b, curve->p, num_words); /* r = x^3 - 3x + b */
}
#endif /* uECC_SUPPORTS_secp... */

#if uECC_SUPPORT_COMPRESSED_POINT
#if uECC_SUPPORTS_secp160r1 || uECC_SUPPORTS_secp192r1 || \
    uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1
/* Compute a = sqrt(a) (mod curve_p). */
static void mod_sqrt_default(uECC_word_t *a, uECC_Curve curve) {
    bitcount_t i;
    uECC_word_t p1[uECC_MAX_WORDS] = {1};
    uECC_word_t l_result[uECC_MAX_WORDS] = {1};
    wordcount_t num_words = curve->num_words;
    
    /* When curve->p == 3 (mod 4), we can compute
       sqrt(a) = a^((curve->p + 1) / 4) (mod curve->p). */
    uECC_vli_add(p1, curve->p, p1, num_words); /* p1 = curve_p + 1 */
    for (i = uECC_vli_numBits(p1, num_words) - 1; i > 1; --i) {
        uECC_vli_modSquare_fast(l_result, l_result, curve);
        if (uECC_vli_testBit(p1, i)) {
            uECC_vli_modMult_fast(l_result, l_result, a, curve);
        }
    }
    uECC_vli_set(a, l_result, num_words);
}
#endif /* uECC_SUPPORTS_secp... */
#endif /* uECC_SUPPORT_COMPRESSED_POINT */

#if uECC_SUPPORTS_secp160r1

#if (uECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp160r1(uECC_word_t *result, uECC_word_t *product);
#endif

static const struct uECC_Curve_t curve_secp160r1 = {
    num_words_secp160r1,
    num_bytes_secp160r1,
    161, /* num_n_bits */
    { BYTES_TO_WORDS_8(FF, FF, FF, 7F, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_4(FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(57, 22, 75, CA, D3, AE, 27, F9),
        BYTES_TO_WORDS_8(C8, F4, 01, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 01, 00, 00, 00) },
    { BYTES_TO_WORDS_8(82, FC, CB, 13, B9, 8B, C3, 68),
        BYTES_TO_WORDS_8(89, 69, 64, 46, 28, 73, F5, 8E),
        BYTES_TO_WORDS_4(68, B5, 96, 4A),

        BYTES_TO_WORDS_8(32, FB, C5, 7A, 37, 51, 23, 04),
        BYTES_TO_WORDS_8(12, C9, DC, 59, 7D, 94, 68, 31),
        BYTES_TO_WORDS_4(55, 28, A6, 23) },
    { BYTES_TO_WORDS_8(45, FA, 65, C5, AD, D4, D4, 81),
        BYTES_TO_WORDS_8(9F, F8, AC, 65, 8B, 7A, BD, 54),
        BYTES_TO_WORDS_4(FC, BE, 97, 1C) },
    &double_jacobian_default,
#if uECC_SUPPORT_COMPRESSED_POINT
    &mod_sqrt_default,
#endif
    &x_side_default,
#if (uECC_OPTIMIZATION_LEVEL > 0)
    &vli_mmod_fast_secp160r1
#endif
};

uECC_Curve uECC_secp160r1(void) { return &curve_secp160r1; }

#if (uECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp160r1)
/* Computes result = product % curve_p
    see http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf page 354
    
    Note that this only works if log2(omega) < log2(p) / 2 */
static void omega_mult_secp160r1(uECC_word_t *result, const uECC_word_t *right);
#if uECC_WORD_SIZE == 8
static void vli_mmod_fast_secp160r1(uECC_word_t *result, uECC_word_t *product) {
    uECC_word_t tmp[2 * num_words_secp160r1];
    uECC_word_t copy;
    
    uECC_vli_clear(tmp, num_words_secp160r1);
    uECC_vli_clear(tmp + num_words_secp160r1, num_words_secp160r1);

    omega_mult_secp160r1(tmp, product + num_words_secp160r1 - 1); /* (Rq, q) = q * c */
    
    product[num_words_secp160r1 - 1] &= 0xffffffff;
    copy = tmp[num_words_secp160r1 - 1];
    tmp[num_words_secp160r1 - 1] &= 0xffffffff;
    uECC_vli_add(result, product, tmp, num_words_secp160r1); /* (C, r) = r + q */
    uECC_vli_clear(product, num_words_secp160r1);
    tmp[num_words_secp160r1 - 1] = copy;
    omega_mult_secp160r1(product, tmp + num_words_secp160r1 - 1); /* Rq*c */
    uECC_vli_add(result, result, product, num_words_secp160r1); /* (C1, r) = r + Rq*c */

    while (uECC_vli_cmp_unsafe(result, curve_secp160r1.p, num_words_secp160r1) > 0) {
        uECC_vli_sub(result, result, curve_secp160r1.p, num_words_secp160r1);
    }
}

static void omega_mult_secp160r1(uint64_t *result, const uint64_t *right) {
    uint32_t carry;
    unsigned i;
    
    /* Multiply by (2^31 + 1). */
    carry = 0;
    for (i = 0; i < num_words_secp160r1; ++i) {
        uint64_t tmp = (right[i] >> 32) | (right[i + 1] << 32);
        result[i] = (tmp << 31) + tmp + carry;
        carry = (tmp >> 33) + (result[i] < tmp || (carry && result[i] == tmp));
    }
    result[i] = carry;
}
#else
static void vli_mmod_fast_secp160r1(uECC_word_t *result, uECC_word_t *product) {
    uECC_word_t tmp[2 * num_words_secp160r1];
    uECC_word_t carry;
    
    uECC_vli_clear(tmp, num_words_secp160r1);
    uECC_vli_clear(tmp + num_words_secp160r1, num_words_secp160r1);

    omega_mult_secp160r1(tmp, product + num_words_secp160r1); /* (Rq, q) = q * c */
    
    carry = uECC_vli_add(result, product, tmp, num_words_secp160r1); /* (C, r) = r + q */
    uECC_vli_clear(product, num_words_secp160r1);
    omega_mult_secp160r1(product, tmp + num_words_secp160r1); /* Rq*c */
    carry += uECC_vli_add(result, result, product, num_words_secp160r1); /* (C1, r) = r + Rq*c */

    while (carry > 0) {
        --carry;
        uECC_vli_sub(result, result, curve_secp160r1.p, num_words_secp160r1);
    }
    if (uECC_vli_cmp_unsafe(result, curve_secp160r1.p, num_words_secp160r1) > 0) {
        uECC_vli_sub(result, result, curve_secp160r1.p, num_words_secp160r1);
    }
}
#endif

#if uECC_WORD_SIZE == 1
static void omega_mult_secp160r1(uint8_t *result, const uint8_t *right) {
    uint8_t carry;
    uint8_t i;
    
    /* Multiply by (2^31 + 1). */
    uECC_vli_set(result + 4, right, num_words_secp160r1); /* 2^32 */
    uECC_vli_rshift1(result + 4, num_words_secp160r1); /* 2^31 */
    result[3] = right[0] << 7; /* get last bit from shift */
    
    carry = uECC_vli_add(result, result, right, num_words_secp160r1); /* 2^31 + 1 */
    for (i = num_words_secp160r1; carry; ++i) {
        uint16_t sum = (uint16_t)result[i] + carry;
        result[i] = (uint8_t)sum;
        carry = sum >> 8;
    }
}
#elif uECC_WORD_SIZE == 4
static void omega_mult_secp160r1(uint32_t *result, const uint32_t *right) {
    uint32_t carry;
    unsigned i;
    
    /* Multiply by (2^31 + 1). */
    uECC_vli_set(result + 1, right, num_words_secp160r1); /* 2^32 */
    uECC_vli_rshift1(result + 1, num_words_secp160r1); /* 2^31 */
    result[0] = right[0] << 31; /* get last bit from shift */
    
    carry = uECC_vli_add(result, result, right, num_words_secp160r1); /* 2^31 + 1 */
    for (i = num_words_secp160r1; carry; ++i) {
        uint64_t sum = (uint64_t)result[i] + carry;
        result[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
}
#endif /* uECC_WORD_SIZE */
#endif /* (uECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp160r1) */

#endif /* uECC_SUPPORTS_secp160r1 */

#if uECC_SUPPORTS_secp192r1

#if (uECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp192r1(uECC_word_t *result, uECC_word_t *product);
#endif

static const struct uECC_Curve_t curve_secp192r1 = {
    num_words_secp192r1,
    num_bytes_secp192r1,
    192, /* num_n_bits */
    { BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FE, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(31, 28, D2, B4, B1, C9, 6B, 14),
        BYTES_TO_WORDS_8(36, F8, DE, 99, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(12, 10, FF, 82, FD, 0A, FF, F4),
        BYTES_TO_WORDS_8(00, 88, A1, 43, EB, 20, BF, 7C),
        BYTES_TO_WORDS_8(F6, 90, 30, B0, 0E, A8, 8D, 18),

        BYTES_TO_WORDS_8(11, 48, 79, 1E, A1, 77, F9, 73),
        BYTES_TO_WORDS_8(D5, CD, 24, 6B, ED, 11, 10, 63),
        BYTES_TO_WORDS_8(78, DA, C8, FF, 95, 2B, 19, 07) },
    { BYTES_TO_WORDS_8(B1, B9, 46, C1, EC, DE, B8, FE),
        BYTES_TO_WORDS_8(49, 30, 24, 72, AB, E9, A7, 0F),
        BYTES_TO_WORDS_8(E7, 80, 9C, E5, 19, 05, 21, 64) },
    &double_jacobian_default,
#if uECC_SUPPORT_COMPRESSED_POINT
    &mod_sqrt_default,
#endif
    &x_side_default,
#if (uECC_OPTIMIZATION_LEVEL > 0)
    &vli_mmod_fast_secp192r1
#endif
};

uECC_Curve uECC_secp192r1(void) { return &curve_secp192r1; }

#if (uECC_OPTIMIZATION_LEVEL > 0)
/* Computes result = product % curve_p.
   See algorithm 5 and 6 from http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf */
#if uECC_WORD_SIZE == 1
static void vli_mmod_fast_secp192r1(uint8_t *result, uint8_t *product) {
    uint8_t tmp[num_words_secp192r1];
    uint8_t carry;
    
    uECC_vli_set(result, product, num_words_secp192r1);
    
    uECC_vli_set(tmp, &product[24], num_words_secp192r1);
    carry = uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    tmp[0] = tmp[1] = tmp[2] = tmp[3] = tmp[4] = tmp[5] = tmp[6] = tmp[7] = 0;
    tmp[8] = product[24]; tmp[9] = product[25]; tmp[10] = product[26]; tmp[11] = product[27];
    tmp[12] = product[28]; tmp[13] = product[29]; tmp[14] = product[30]; tmp[15] = product[31];
    tmp[16] = product[32]; tmp[17] = product[33]; tmp[18] = product[34]; tmp[19] = product[35];
    tmp[20] = product[36]; tmp[21] = product[37]; tmp[22] = product[38]; tmp[23] = product[39];
    carry += uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    tmp[0] = tmp[8] = product[40];
    tmp[1] = tmp[9] = product[41];
    tmp[2] = tmp[10] = product[42];
    tmp[3] = tmp[11] = product[43];
    tmp[4] = tmp[12] = product[44];
    tmp[5] = tmp[13] = product[45];
    tmp[6] = tmp[14] = product[46];
    tmp[7] = tmp[15] = product[47];
    tmp[16] = tmp[17] = tmp[18] = tmp[19] = tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
    carry += uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    while (carry || uECC_vli_cmp_unsafe(curve_secp192r1.p, result, num_words_secp192r1) != 1) {
        carry -= uECC_vli_sub(result, result, curve_secp192r1.p, num_words_secp192r1);
    }
}
#elif uECC_WORD_SIZE == 4
static void vli_mmod_fast_secp192r1(uint32_t *result, uint32_t *product) {
    uint32_t tmp[num_words_secp192r1];
    int carry;
    
    uECC_vli_set(result, product, num_words_secp192r1);
    
    uECC_vli_set(tmp, &product[6], num_words_secp192r1);
    carry = uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    tmp[0] = tmp[1] = 0;
    tmp[2] = product[6];
    tmp[3] = product[7];
    tmp[4] = product[8];
    tmp[5] = product[9];
    carry += uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    tmp[0] = tmp[2] = product[10];
    tmp[1] = tmp[3] = product[11];
    tmp[4] = tmp[5] = 0;
    carry += uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    while (carry || uECC_vli_cmp_unsafe(curve_secp192r1.p, result, num_words_secp192r1) != 1) {
        carry -= uECC_vli_sub(result, result, curve_secp192r1.p, num_words_secp192r1);
    }
}
#else
static void vli_mmod_fast_secp192r1(uint64_t *result, uint64_t *product) {
    uint64_t tmp[num_words_secp192r1];
    int carry;
    
    uECC_vli_set(result, product, num_words_secp192r1);
    
    uECC_vli_set(tmp, &product[3], num_words_secp192r1);
    carry = (int)uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    tmp[0] = 0;
    tmp[1] = product[3];
    tmp[2] = product[4];
    carry += uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    tmp[0] = tmp[1] = product[5];
    tmp[2] = 0;
    carry += uECC_vli_add(result, result, tmp, num_words_secp192r1);
    
    while (carry || uECC_vli_cmp_unsafe(curve_secp192r1.p, result, num_words_secp192r1) != 1) {
        carry -= uECC_vli_sub(result, result, curve_secp192r1.p, num_words_secp192r1);
    }
}
#endif /* uECC_WORD_SIZE */
#endif /* (uECC_OPTIMIZATION_LEVEL > 0) */

#endif /* uECC_SUPPORTS_secp192r1 */

#if uECC_SUPPORTS_secp224r1

#if uECC_SUPPORT_COMPRESSED_POINT
static void mod_sqrt_secp224r1(uECC_word_t *a, uECC_Curve curve);
#endif
#if (uECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp224r1(uECC_word_t *result, uECC_word_t *product);
#endif

static const struct uECC_Curve_t curve_secp224r1 = {
    num_words_secp224r1,
    num_bytes_secp224r1,
    224, /* num_n_bits */
    { BYTES_TO_WORDS_8(01, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_4(FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(3D, 2A, 5C, 5C, 45, 29, DD, 13),
        BYTES_TO_WORDS_8(3E, F0, B8, E0, A2, 16, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_4(FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(21, 1D, 5C, 11, D6, 80, 32, 34),
        BYTES_TO_WORDS_8(22, 11, C2, 56, D3, C1, 03, 4A),
        BYTES_TO_WORDS_8(B9, 90, 13, 32, 7F, BF, B4, 6B),
        BYTES_TO_WORDS_4(BD, 0C, 0E, B7),

        BYTES_TO_WORDS_8(34, 7E, 00, 85, 99, 81, D5, 44),
        BYTES_TO_WORDS_8(64, 47, 07, 5A, A0, 75, 43, CD),
        BYTES_TO_WORDS_8(E6, DF, 22, 4C, FB, 23, F7, B5),
        BYTES_TO_WORDS_4(88, 63, 37, BD) },
    { BYTES_TO_WORDS_8(B4, FF, 55, 23, 43, 39, 0B, 27),
        BYTES_TO_WORDS_8(BA, D8, BF, D7, B7, B0, 44, 50),
        BYTES_TO_WORDS_8(56, 32, 41, F5, AB, B3, 04, 0C),
        BYTES_TO_WORDS_4(85, 0A, 05, B4) },
    &double_jacobian_default,
#if uECC_SUPPORT_COMPRESSED_POINT
    &mod_sqrt_secp224r1,
#endif
    &x_side_default,
#if (uECC_OPTIMIZATION_LEVEL > 0)
    &vli_mmod_fast_secp224r1
#endif
};

uECC_Curve uECC_secp224r1(void) { return &curve_secp224r1; }


#if uECC_SUPPORT_COMPRESSED_POINT
/* Routine 3.2.4 RS;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void mod_sqrt_secp224r1_rs(uECC_word_t *d1,
                                  uECC_word_t *e1,
                                  uECC_word_t *f1,
                                  const uECC_word_t *d0,
                                  const uECC_word_t *e0,
                                  const uECC_word_t *f0) {
    uECC_word_t t[num_words_secp224r1];

    uECC_vli_modSquare_fast(t, d0, &curve_secp224r1);                    /* t <-- d0 ^ 2 */
    uECC_vli_modMult_fast(e1, d0, e0, &curve_secp224r1);                 /* e1 <-- d0 * e0 */
    uECC_vli_modAdd(d1, t, f0, curve_secp224r1.p, num_words_secp224r1);  /* d1 <-- t  + f0 */
    uECC_vli_modAdd(e1, e1, e1, curve_secp224r1.p, num_words_secp224r1); /* e1 <-- e1 + e1 */
    uECC_vli_modMult_fast(f1, t, f0, &curve_secp224r1);                  /* f1 <-- t  * f0 */
    uECC_vli_modAdd(f1, f1, f1, curve_secp224r1.p, num_words_secp224r1); /* f1 <-- f1 + f1 */
    uECC_vli_modAdd(f1, f1, f1, curve_secp224r1.p, num_words_secp224r1); /* f1 <-- f1 + f1 */
}

/* Routine 3.2.5 RSS;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void mod_sqrt_secp224r1_rss(uECC_word_t *d1,
                                   uECC_word_t *e1,
                                   uECC_word_t *f1,
                                   const uECC_word_t *d0,
                                   const uECC_word_t *e0,
                                   const uECC_word_t *f0,
                                   const bitcount_t j) {
    bitcount_t i;

    uECC_vli_set(d1, d0, num_words_secp224r1); /* d1 <-- d0 */
    uECC_vli_set(e1, e0, num_words_secp224r1); /* e1 <-- e0 */
    uECC_vli_set(f1, f0, num_words_secp224r1); /* f1 <-- f0 */
    for (i = 1; i <= j; i++) {
        mod_sqrt_secp224r1_rs(d1, e1, f1, d1, e1, f1); /* RS (d1,e1,f1,d1,e1,f1) */
    }
}

/* Routine 3.2.6 RM;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void mod_sqrt_secp224r1_rm(uECC_word_t *d2,
                                  uECC_word_t *e2,
                                  uECC_word_t *f2,
                                  const uECC_word_t *c,
                                  const uECC_word_t *d0,
                                  const uECC_word_t *e0,
                                  const uECC_word_t *d1,
                                  const uECC_word_t *e1) {
    uECC_word_t t1[num_words_secp224r1];
    uECC_word_t t2[num_words_secp224r1];

    uECC_vli_modMult_fast(t1, e0, e1, &curve_secp224r1); /* t1 <-- e0 * e1 */
    uECC_vli_modMult_fast(t1, t1, c, &curve_secp224r1);  /* t1 <-- t1 * c */
    /* t1 <-- p  - t1 */
    uECC_vli_modSub(t1, curve_secp224r1.p, t1, curve_secp224r1.p, num_words_secp224r1);
    uECC_vli_modMult_fast(t2, d0, d1, &curve_secp224r1);                 /* t2 <-- d0 * d1 */
    uECC_vli_modAdd(t2, t2, t1, curve_secp224r1.p, num_words_secp224r1); /* t2 <-- t2 + t1 */
    uECC_vli_modMult_fast(t1, d0, e1, &curve_secp224r1);                 /* t1 <-- d0 * e1 */
    uECC_vli_modMult_fast(e2, d1, e0, &curve_secp224r1);                 /* e2 <-- d1 * e0 */
    uECC_vli_modAdd(e2, e2, t1, curve_secp224r1.p, num_words_secp224r1); /* e2 <-- e2 + t1 */
    uECC_vli_modSquare_fast(f2, e2, &curve_secp224r1);                   /* f2 <-- e2^2 */
    uECC_vli_modMult_fast(f2, f2, c, &curve_secp224r1);                  /* f2 <-- f2 * c */
    /* f2 <-- p  - f2 */
    uECC_vli_modSub(f2, curve_secp224r1.p, f2, curve_secp224r1.p, num_words_secp224r1);
    uECC_vli_set(d2, t2, num_words_secp224r1); /* d2 <-- t2 */
}

/* Routine 3.2.7 RP;  from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void mod_sqrt_secp224r1_rp(uECC_word_t *d1,
                                  uECC_word_t *e1,
                                  uECC_word_t *f1,
                                  const uECC_word_t *c,
                                  const uECC_word_t *r) {
    wordcount_t i;
    wordcount_t pow2i = 1;
    uECC_word_t d0[num_words_secp224r1];
    uECC_word_t e0[num_words_secp224r1] = {1}; /* e0 <-- 1 */
    uECC_word_t f0[num_words_secp224r1];

    uECC_vli_set(d0, r, num_words_secp224r1); /* d0 <-- r */
    /* f0 <-- p  - c */
    uECC_vli_modSub(f0, curve_secp224r1.p, c, curve_secp224r1.p, num_words_secp224r1);
    for (i = 0; i <= 6; i++) {
        mod_sqrt_secp224r1_rss(d1, e1, f1, d0, e0, f0, pow2i); /* RSS (d1,e1,f1,d0,e0,f0,2^i) */
        mod_sqrt_secp224r1_rm(d1, e1, f1, c, d1, e1, d0, e0);  /* RM (d1,e1,f1,c,d1,e1,d0,e0) */
        uECC_vli_set(d0, d1, num_words_secp224r1); /* d0 <-- d1 */
        uECC_vli_set(e0, e1, num_words_secp224r1); /* e0 <-- e1 */
        uECC_vli_set(f0, f1, num_words_secp224r1); /* f0 <-- f1 */
        pow2i *= 2;
    }
}

/* Compute a = sqrt(a) (mod curve_p). */
/* Routine 3.2.8 mp_mod_sqrt_224; from http://www.nsa.gov/ia/_files/nist-routines.pdf */
static void mod_sqrt_secp224r1(uECC_word_t *a, uECC_Curve curve) {
    bitcount_t i;
    uECC_word_t e1[num_words_secp224r1];
    uECC_word_t f1[num_words_secp224r1];
    uECC_word_t d0[num_words_secp224r1];
    uECC_word_t e0[num_words_secp224r1];
    uECC_word_t f0[num_words_secp224r1];
    uECC_word_t d1[num_words_secp224r1];

    /* s = a; using constant instead of random value */
    mod_sqrt_secp224r1_rp(d0, e0, f0, a, a);           /* RP (d0, e0, f0, c, s) */
    mod_sqrt_secp224r1_rs(d1, e1, f1, d0, e0, f0);     /* RS (d1, e1, f1, d0, e0, f0) */
    for (i = 1; i <= 95; i++) {
        uECC_vli_set(d0, d1, num_words_secp224r1);          /* d0 <-- d1 */
        uECC_vli_set(e0, e1, num_words_secp224r1);          /* e0 <-- e1 */
        uECC_vli_set(f0, f1, num_words_secp224r1);          /* f0 <-- f1 */
        mod_sqrt_secp224r1_rs(d1, e1, f1, d0, e0, f0); /* RS (d1, e1, f1, d0, e0, f0) */
        if (uECC_vli_isZero(d1, num_words_secp224r1)) {     /* if d1 == 0 */
                break;
        }
    }
    uECC_vli_modInv(f1, e0, curve_secp224r1.p, num_words_secp224r1); /* f1 <-- 1 / e0 */
    uECC_vli_modMult_fast(a, d0, f1, &curve_secp224r1);              /* a  <-- d0 / e0 */
}
#endif /* uECC_SUPPORT_COMPRESSED_POINT */

#if (uECC_OPTIMIZATION_LEVEL > 0)
/* Computes result = product % curve_p
   from http://www.nsa.gov/ia/_files/nist-routines.pdf */
#if uECC_WORD_SIZE == 1
static void vli_mmod_fast_secp224r1(uint8_t *result, uint8_t *product) {
    uint8_t tmp[num_words_secp224r1];
    int8_t carry;

    /* t */
    uECC_vli_set(result, product, num_words_secp224r1);

    /* s1 */
    tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0;
    tmp[4] = tmp[5] = tmp[6] = tmp[7] = 0;
    tmp[8] = tmp[9] = tmp[10] = tmp[11] = 0;
    tmp[12] = product[28]; tmp[13] = product[29]; tmp[14] = product[30]; tmp[15] = product[31];
    tmp[16] = product[32]; tmp[17] = product[33]; tmp[18] = product[34]; tmp[19] = product[35];
    tmp[20] = product[36]; tmp[21] = product[37]; tmp[22] = product[38]; tmp[23] = product[39];
    tmp[24] = product[40]; tmp[25] = product[41]; tmp[26] = product[42]; tmp[27] = product[43];
    carry = uECC_vli_add(result, result, tmp, num_words_secp224r1);

    /* s2 */
    tmp[12] = product[44]; tmp[13] = product[45]; tmp[14] = product[46]; tmp[15] = product[47];
    tmp[16] = product[48]; tmp[17] = product[49]; tmp[18] = product[50]; tmp[19] = product[51];
    tmp[20] = product[52]; tmp[21] = product[53]; tmp[22] = product[54]; tmp[23] = product[55];
    tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
    carry += uECC_vli_add(result, result, tmp, num_words_secp224r1);

    /* d1 */
    tmp[0]  = product[28]; tmp[1]  = product[29]; tmp[2]  = product[30]; tmp[3]  = product[31];
    tmp[4]  = product[32]; tmp[5]  = product[33]; tmp[6]  = product[34]; tmp[7]  = product[35];
    tmp[8]  = product[36]; tmp[9]  = product[37]; tmp[10] = product[38]; tmp[11] = product[39];
    tmp[12] = product[40]; tmp[13] = product[41]; tmp[14] = product[42]; tmp[15] = product[43];
    tmp[16] = product[44]; tmp[17] = product[45]; tmp[18] = product[46]; tmp[19] = product[47];
    tmp[20] = product[48]; tmp[21] = product[49]; tmp[22] = product[50]; tmp[23] = product[51];
    tmp[24] = product[52]; tmp[25] = product[53]; tmp[26] = product[54]; tmp[27] = product[55];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp224r1);

    /* d2 */
    tmp[0]  = product[44]; tmp[1]  = product[45]; tmp[2]  = product[46]; tmp[3]  = product[47];
    tmp[4]  = product[48]; tmp[5]  = product[49]; tmp[6]  = product[50]; tmp[7]  = product[51];
    tmp[8]  = product[52]; tmp[9]  = product[53]; tmp[10] = product[54]; tmp[11] = product[55];
    tmp[12] = tmp[13] = tmp[14] = tmp[15] = 0;
    tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
    tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
    tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp224r1);

    if (carry < 0) {
        do {
            carry += uECC_vli_add(result, result, curve_secp224r1.p, num_words_secp224r1);
        } while (carry < 0);
    } else {
        while (carry || uECC_vli_cmp_unsafe(curve_secp224r1.p, result, num_words_secp224r1) != 1) {
            carry -= uECC_vli_sub(result, result, curve_secp224r1.p, num_words_secp224r1);
        }
    }
}
#elif uECC_WORD_SIZE == 4
static void vli_mmod_fast_secp224r1(uint32_t *result, uint32_t *product)
{
    uint32_t tmp[num_words_secp224r1];
    int carry;

    /* t */
    uECC_vli_set(result, product, num_words_secp224r1);

    /* s1 */
    tmp[0] = tmp[1] = tmp[2] = 0;
    tmp[3] = product[7];
    tmp[4] = product[8];
    tmp[5] = product[9];
    tmp[6] = product[10];
    carry = uECC_vli_add(result, result, tmp, num_words_secp224r1);

    /* s2 */
    tmp[3] = product[11];
    tmp[4] = product[12];
    tmp[5] = product[13];
    tmp[6] = 0;
    carry += uECC_vli_add(result, result, tmp, num_words_secp224r1);

    /* d1 */
    tmp[0] = product[7];
    tmp[1] = product[8];
    tmp[2] = product[9];
    tmp[3] = product[10];
    tmp[4] = product[11];
    tmp[5] = product[12];
    tmp[6] = product[13];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp224r1);

    /* d2 */
    tmp[0] = product[11];
    tmp[1] = product[12];
    tmp[2] = product[13];
    tmp[3] = tmp[4] = tmp[5] = tmp[6] = 0;
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp224r1);

    if (carry < 0) {
        do {
            carry += uECC_vli_add(result, result, curve_secp224r1.p, num_words_secp224r1);
        } while (carry < 0);
    } else {
        while (carry || uECC_vli_cmp_unsafe(curve_secp224r1.p, result, num_words_secp224r1) != 1) {
            carry -= uECC_vli_sub(result, result, curve_secp224r1.p, num_words_secp224r1);
        }
    }
}
#else
static void vli_mmod_fast_secp224r1(uint64_t *result, uint64_t *product)
{
    uint64_t tmp[num_words_secp224r1];
    int carry = 0;

    /* t */
    uECC_vli_set(result, product, num_words_secp224r1);
    result[num_words_secp224r1 - 1] &= 0xffffffff;

    /* s1 */
    tmp[0] = 0;
    tmp[1] = product[3] & 0xffffffff00000000ull;
    tmp[2] = product[4];
    tmp[3] = product[5] & 0xffffffff;
    uECC_vli_add(result, result, tmp, num_words_secp224r1);

    /* s2 */
    tmp[1] = product[5] & 0xffffffff00000000ull;
    tmp[2] = product[6];
    tmp[3] = 0;
    uECC_vli_add(result, result, tmp, num_words_secp224r1);

    /* d1 */
    tmp[0] = (product[3] >> 32) | (product[4] << 32);
    tmp[1] = (product[4] >> 32) | (product[5] << 32);
    tmp[2] = (product[5] >> 32) | (product[6] << 32);
    tmp[3] = product[6] >> 32;
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp224r1);

    /* d2 */
    tmp[0] = (product[5] >> 32) | (product[6] << 32);
    tmp[1] = product[6] >> 32;
    tmp[2] = tmp[3] = 0;
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp224r1);

    if (carry < 0) {
        do {
            carry += uECC_vli_add(result, result, curve_secp224r1.p, num_words_secp224r1);
        } while (carry < 0);
    } else {
        while (uECC_vli_cmp_unsafe(curve_secp224r1.p, result, num_words_secp224r1) != 1) {
            uECC_vli_sub(result, result, curve_secp224r1.p, num_words_secp224r1);
        }
    }
}
#endif /* uECC_WORD_SIZE */
#endif /* (uECC_OPTIMIZATION_LEVEL > 0) */

#endif /* uECC_SUPPORTS_secp224r1 */

#if uECC_SUPPORTS_secp256r1

#if (uECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp256r1(uECC_word_t *result, uECC_word_t *product);
#endif

static const struct uECC_Curve_t curve_secp256r1 = {
    num_words_secp256r1,
    num_bytes_secp256r1,
    256, /* num_n_bits */
    { BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(01, 00, 00, 00, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(51, 25, 63, FC, C2, CA, B9, F3),
        BYTES_TO_WORDS_8(84, 9E, 17, A7, AD, FA, E6, BC),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(00, 00, 00, 00, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(96, C2, 98, D8, 45, 39, A1, F4),
        BYTES_TO_WORDS_8(A0, 33, EB, 2D, 81, 7D, 03, 77),
        BYTES_TO_WORDS_8(F2, 40, A4, 63, E5, E6, BC, F8),
        BYTES_TO_WORDS_8(47, 42, 2C, E1, F2, D1, 17, 6B),

        BYTES_TO_WORDS_8(F5, 51, BF, 37, 68, 40, B6, CB),
        BYTES_TO_WORDS_8(CE, 5E, 31, 6B, 57, 33, CE, 2B),
        BYTES_TO_WORDS_8(16, 9E, 0F, 7C, 4A, EB, E7, 8E),
        BYTES_TO_WORDS_8(9B, 7F, 1A, FE, E2, 42, E3, 4F) },
    { BYTES_TO_WORDS_8(4B, 60, D2, 27, 3E, 3C, CE, 3B),
        BYTES_TO_WORDS_8(F6, B0, 53, CC, B0, 06, 1D, 65),
        BYTES_TO_WORDS_8(BC, 86, 98, 76, 55, BD, EB, B3),
        BYTES_TO_WORDS_8(E7, 93, 3A, AA, D8, 35, C6, 5A) },
    &double_jacobian_default,
#if uECC_SUPPORT_COMPRESSED_POINT
    &mod_sqrt_default,
#endif
    &x_side_default,
#if (uECC_OPTIMIZATION_LEVEL > 0)
    &vli_mmod_fast_secp256r1
#endif
};

uECC_Curve uECC_secp256r1(void) { return &curve_secp256r1; }


#if (uECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp256r1)
/* Computes result = product % curve_p
   from http://www.nsa.gov/ia/_files/nist-routines.pdf */
#if uECC_WORD_SIZE == 1
static void vli_mmod_fast_secp256r1(uint8_t *result, uint8_t *product) {
    uint8_t tmp[num_words_secp256r1];
    int8_t carry;
    
    /* t */
    uECC_vli_set(result, product, num_words_secp256r1);
    
    /* s1 */
    tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0;
    tmp[4] = tmp[5] = tmp[6] = tmp[7] = 0;
    tmp[8] = tmp[9] = tmp[10] = tmp[11] = 0;
    tmp[12] = product[44]; tmp[13] = product[45]; tmp[14] = product[46]; tmp[15] = product[47];
    tmp[16] = product[48]; tmp[17] = product[49]; tmp[18] = product[50]; tmp[19] = product[51];
    tmp[20] = product[52]; tmp[21] = product[53]; tmp[22] = product[54]; tmp[23] = product[55];
    tmp[24] = product[56]; tmp[25] = product[57]; tmp[26] = product[58]; tmp[27] = product[59];
    tmp[28] = product[60]; tmp[29] = product[61]; tmp[30] = product[62]; tmp[31] = product[63];
    carry = uECC_vli_add(tmp, tmp, tmp, num_words_secp256r1);
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s2 */
    tmp[12] = product[48]; tmp[13] = product[49]; tmp[14] = product[50]; tmp[15] = product[51];
    tmp[16] = product[52]; tmp[17] = product[53]; tmp[18] = product[54]; tmp[19] = product[55];
    tmp[20] = product[56]; tmp[21] = product[57]; tmp[22] = product[58]; tmp[23] = product[59];
    tmp[24] = product[60]; tmp[25] = product[61]; tmp[26] = product[62]; tmp[27] = product[63];
    tmp[28] = tmp[29] = tmp[30] = tmp[31] = 0;
    carry += uECC_vli_add(tmp, tmp, tmp, num_words_secp256r1);
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s3 */
    tmp[0] = product[32]; tmp[1] = product[33]; tmp[2] = product[34]; tmp[3] = product[35];
    tmp[4] = product[36]; tmp[5] = product[37]; tmp[6] = product[38]; tmp[7] = product[39];
    tmp[8] = product[40]; tmp[9] = product[41]; tmp[10] = product[42]; tmp[11] = product[43];
    tmp[12] = tmp[13] = tmp[14] = tmp[15] = 0;
    tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
    tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
    tmp[24] = product[56]; tmp[25] = product[57]; tmp[26] = product[58]; tmp[27] = product[59];
    tmp[28] = product[60]; tmp[29] = product[61]; tmp[30] = product[62]; tmp[31] = product[63];
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s4 */
    tmp[0] = product[36]; tmp[1] = product[37]; tmp[2] = product[38]; tmp[3] = product[39];
    tmp[4] = product[40]; tmp[5] = product[41]; tmp[6] = product[42]; tmp[7] = product[43];
    tmp[8] = product[44]; tmp[9] = product[45]; tmp[10] = product[46]; tmp[11] = product[47];
    tmp[12] = product[52]; tmp[13] = product[53]; tmp[14] = product[54]; tmp[15] = product[55];
    tmp[16] = product[56]; tmp[17] = product[57]; tmp[18] = product[58]; tmp[19] = product[59];
    tmp[20] = product[60]; tmp[21] = product[61]; tmp[22] = product[62]; tmp[23] = product[63];
    tmp[24] = product[52]; tmp[25] = product[53]; tmp[26] = product[54]; tmp[27] = product[55];
    tmp[28] = product[32]; tmp[29] = product[33]; tmp[30] = product[34]; tmp[31] = product[35];
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* d1 */
    tmp[0] = product[44]; tmp[1] = product[45]; tmp[2] = product[46]; tmp[3] = product[47];
    tmp[4] = product[48]; tmp[5] = product[49]; tmp[6] = product[50]; tmp[7] = product[51];
    tmp[8] = product[52]; tmp[9] = product[53]; tmp[10] = product[54]; tmp[11] = product[55];
    tmp[12] = tmp[13] = tmp[14] = tmp[15] = 0;
    tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
    tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
    tmp[24] = product[32]; tmp[25] = product[33]; tmp[26] = product[34]; tmp[27] = product[35];
    tmp[28] = product[40]; tmp[29] = product[41]; tmp[30] = product[42]; tmp[31] = product[43];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d2 */
    tmp[0] = product[48]; tmp[1] = product[49]; tmp[2] = product[50]; tmp[3] = product[51];
    tmp[4] = product[52]; tmp[5] = product[53]; tmp[6] = product[54]; tmp[7] = product[55];
    tmp[8] = product[56]; tmp[9] = product[57]; tmp[10] = product[58]; tmp[11] = product[59];
    tmp[12] = product[60]; tmp[13] = product[61]; tmp[14] = product[62]; tmp[15] = product[63];
    tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0;
    tmp[20] = tmp[21] = tmp[22] = tmp[23] = 0;
    tmp[24] = product[36]; tmp[25] = product[37]; tmp[26] = product[38]; tmp[27] = product[39];
    tmp[28] = product[44]; tmp[29] = product[45]; tmp[30] = product[46]; tmp[31] = product[47];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d3 */
    tmp[0] = product[52]; tmp[1] = product[53]; tmp[2] = product[54]; tmp[3] = product[55];
    tmp[4] = product[56]; tmp[5] = product[57]; tmp[6] = product[58]; tmp[7] = product[59];
    tmp[8] = product[60]; tmp[9] = product[61]; tmp[10] = product[62]; tmp[11] = product[63];
    tmp[12] = product[32]; tmp[13] = product[33]; tmp[14] = product[34]; tmp[15] = product[35];
    tmp[16] = product[36]; tmp[17] = product[37]; tmp[18] = product[38]; tmp[19] = product[39];
    tmp[20] = product[40]; tmp[21] = product[41]; tmp[22] = product[42]; tmp[23] = product[43];
    tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
    tmp[28] = product[48]; tmp[29] = product[49]; tmp[30] = product[50]; tmp[31] = product[51];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d4 */
    tmp[0] = product[56]; tmp[1] = product[57]; tmp[2] = product[58]; tmp[3] = product[59];
    tmp[4] = product[60]; tmp[5] = product[61]; tmp[6] = product[62]; tmp[7] = product[63];
    tmp[8] = tmp[9] = tmp[10] = tmp[11] = 0;
    tmp[12] = product[36]; tmp[13] = product[37]; tmp[14] = product[38]; tmp[15] = product[39];
    tmp[16] = product[40]; tmp[17] = product[41]; tmp[18] = product[42]; tmp[19] = product[43];
    tmp[20] = product[44]; tmp[21] = product[45]; tmp[22] = product[46]; tmp[23] = product[47];
    tmp[24] = tmp[25] = tmp[26] = tmp[27] = 0;
    tmp[28] = product[52]; tmp[29] = product[53]; tmp[30] = product[54]; tmp[31] = product[55];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    if (carry < 0) {
        do {
            carry += uECC_vli_add(result, result, curve_secp256r1.p, num_words_secp256r1);
        } while (carry < 0);
    } else {
        while (carry || uECC_vli_cmp_unsafe(curve_secp256r1.p, result, num_words_secp256r1) != 1) {
            carry -= uECC_vli_sub(result, result, curve_secp256r1.p, num_words_secp256r1);
        }
    }
}
#elif uECC_WORD_SIZE == 4
static void vli_mmod_fast_secp256r1(uint32_t *result, uint32_t *product) {
    uint32_t tmp[num_words_secp256r1];
    int carry;
    
    /* t */
    uECC_vli_set(result, product, num_words_secp256r1);
    
    /* s1 */
    tmp[0] = tmp[1] = tmp[2] = 0;
    tmp[3] = product[11];
    tmp[4] = product[12];
    tmp[5] = product[13];
    tmp[6] = product[14];
    tmp[7] = product[15];
    carry = uECC_vli_add(tmp, tmp, tmp, num_words_secp256r1);
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s2 */
    tmp[3] = product[12];
    tmp[4] = product[13];
    tmp[5] = product[14];
    tmp[6] = product[15];
    tmp[7] = 0;
    carry += uECC_vli_add(tmp, tmp, tmp, num_words_secp256r1);
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s3 */
    tmp[0] = product[8];
    tmp[1] = product[9];
    tmp[2] = product[10];
    tmp[3] = tmp[4] = tmp[5] = 0;
    tmp[6] = product[14];
    tmp[7] = product[15];
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s4 */
    tmp[0] = product[9];
    tmp[1] = product[10];
    tmp[2] = product[11];
    tmp[3] = product[13];
    tmp[4] = product[14];
    tmp[5] = product[15];
    tmp[6] = product[13];
    tmp[7] = product[8];
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* d1 */
    tmp[0] = product[11];
    tmp[1] = product[12];
    tmp[2] = product[13];
    tmp[3] = tmp[4] = tmp[5] = 0;
    tmp[6] = product[8];
    tmp[7] = product[10];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d2 */
    tmp[0] = product[12];
    tmp[1] = product[13];
    tmp[2] = product[14];
    tmp[3] = product[15];
    tmp[4] = tmp[5] = 0;
    tmp[6] = product[9];
    tmp[7] = product[11];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d3 */
    tmp[0] = product[13];
    tmp[1] = product[14];
    tmp[2] = product[15];
    tmp[3] = product[8];
    tmp[4] = product[9];
    tmp[5] = product[10];
    tmp[6] = 0;
    tmp[7] = product[12];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d4 */
    tmp[0] = product[14];
    tmp[1] = product[15];
    tmp[2] = 0;
    tmp[3] = product[9];
    tmp[4] = product[10];
    tmp[5] = product[11];
    tmp[6] = 0;
    tmp[7] = product[13];
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    if (carry < 0) {
        do {
            carry += uECC_vli_add(result, result, curve_secp256r1.p, num_words_secp256r1);
        } while (carry < 0);
    } else {
        while (carry || uECC_vli_cmp_unsafe(curve_secp256r1.p, result, num_words_secp256r1) != 1) {
            carry -= uECC_vli_sub(result, result, curve_secp256r1.p, num_words_secp256r1);
        }
    }
}
#else
static void vli_mmod_fast_secp256r1(uint64_t *result, uint64_t *product) {
    uint64_t tmp[num_words_secp256r1];
    int carry;
    
    /* t */
    uECC_vli_set(result, product, num_words_secp256r1);
    
    /* s1 */
    tmp[0] = 0;
    tmp[1] = product[5] & 0xffffffff00000000ull;
    tmp[2] = product[6];
    tmp[3] = product[7];
    carry = (int)uECC_vli_add(tmp, tmp, tmp, num_words_secp256r1);
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s2 */
    tmp[1] = product[6] << 32;
    tmp[2] = (product[6] >> 32) | (product[7] << 32);
    tmp[3] = product[7] >> 32;
    carry += uECC_vli_add(tmp, tmp, tmp, num_words_secp256r1);
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s3 */
    tmp[0] = product[4];
    tmp[1] = product[5] & 0xffffffff;
    tmp[2] = 0;
    tmp[3] = product[7];
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* s4 */
    tmp[0] = (product[4] >> 32) | (product[5] << 32);
    tmp[1] = (product[5] >> 32) | (product[6] & 0xffffffff00000000ull);
    tmp[2] = product[7];
    tmp[3] = (product[6] >> 32) | (product[4] << 32);
    carry += uECC_vli_add(result, result, tmp, num_words_secp256r1);
    
    /* d1 */
    tmp[0] = (product[5] >> 32) | (product[6] << 32);
    tmp[1] = (product[6] >> 32);
    tmp[2] = 0;
    tmp[3] = (product[4] & 0xffffffff) | (product[5] << 32);
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d2 */
    tmp[0] = product[6];
    tmp[1] = product[7];
    tmp[2] = 0;
    tmp[3] = (product[4] >> 32) | (product[5] & 0xffffffff00000000ull);
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d3 */
    tmp[0] = (product[6] >> 32) | (product[7] << 32);
    tmp[1] = (product[7] >> 32) | (product[4] << 32);
    tmp[2] = (product[4] >> 32) | (product[5] << 32);
    tmp[3] = (product[6] << 32);
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    /* d4 */
    tmp[0] = product[7];
    tmp[1] = product[4] & 0xffffffff00000000ull;
    tmp[2] = product[5];
    tmp[3] = product[6] & 0xffffffff00000000ull;
    carry -= uECC_vli_sub(result, result, tmp, num_words_secp256r1);
    
    if (carry < 0) {
        do {
            carry += uECC_vli_add(result, result, curve_secp256r1.p, num_words_secp256r1);
        } while (carry < 0);
    } else {
        while (carry || uECC_vli_cmp_unsafe(curve_secp256r1.p, result, num_words_secp256r1) != 1) {
            carry -= uECC_vli_sub(result, result, curve_secp256r1.p, num_words_secp256r1);
        }
    }
}
#endif /* uECC_WORD_SIZE */
#endif /* (uECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp256r1) */

#endif /* uECC_SUPPORTS_secp256r1 */

#if uECC_SUPPORTS_secp256k1

static void double_jacobian_secp256k1(uECC_word_t * X1,
                                      uECC_word_t * Y1,
                                      uECC_word_t * Z1,
                                      uECC_Curve curve);
static void x_side_secp256k1(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve);
#if (uECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp256k1(uECC_word_t *result, uECC_word_t *product);
#endif

static const struct uECC_Curve_t curve_secp256k1 = {
    num_words_secp256k1,
    num_bytes_secp256k1,
    256, /* num_n_bits */
    { BYTES_TO_WORDS_8(2F, FC, FF, FF, FE, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(41, 41, 36, D0, 8C, 5E, D2, BF),
        BYTES_TO_WORDS_8(3B, A0, 48, AF, E6, DC, AE, BA),
        BYTES_TO_WORDS_8(FE, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(98, 17, F8, 16, 5B, 81, F2, 59),
        BYTES_TO_WORDS_8(D9, 28, CE, 2D, DB, FC, 9B, 02),
        BYTES_TO_WORDS_8(07, 0B, 87, CE, 95, 62, A0, 55),
        BYTES_TO_WORDS_8(AC, BB, DC, F9, 7E, 66, BE, 79),

        BYTES_TO_WORDS_8(B8, D4, 10, FB, 8F, D0, 47, 9C),
        BYTES_TO_WORDS_8(19, 54, 85, A6, 48, B4, 17, FD),
        BYTES_TO_WORDS_8(A8, 08, 11, 0E, FC, FB, A4, 5D),
        BYTES_TO_WORDS_8(65, C4, A3, 26, 77, DA, 3A, 48) },
    { BYTES_TO_WORDS_8(07, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 00, 00, 00, 00) },
    &double_jacobian_secp256k1,
#if uECC_SUPPORT_COMPRESSED_POINT
    &mod_sqrt_default,
#endif
    &x_side_secp256k1,
#if (uECC_OPTIMIZATION_LEVEL > 0)
    &vli_mmod_fast_secp256k1
#endif
};

uECC_Curve uECC_secp256k1(void) { return &curve_secp256k1; }


/* Double in place */
static void double_jacobian_secp256k1(uECC_word_t * X1,
                                      uECC_word_t * Y1,
                                      uECC_word_t * Z1,
                                      uECC_Curve curve) {
    /* t1 = X, t2 = Y, t3 = Z */
    uECC_word_t t4[num_words_secp256k1];
    uECC_word_t t5[num_words_secp256k1];
    
    if (uECC_vli_isZero(Z1, num_words_secp256k1)) {
        return;
    }
    
    uECC_vli_modSquare_fast(t5, Y1, curve);   /* t5 = y1^2 */
    uECC_vli_modMult_fast(t4, X1, t5, curve); /* t4 = x1*y1^2 = A */
    uECC_vli_modSquare_fast(X1, X1, curve);   /* t1 = x1^2 */
    uECC_vli_modSquare_fast(t5, t5, curve);   /* t5 = y1^4 */
    uECC_vli_modMult_fast(Z1, Y1, Z1, curve); /* t3 = y1*z1 = z3 */
    
    uECC_vli_modAdd(Y1, X1, X1, curve->p, num_words_secp256k1); /* t2 = 2*x1^2 */
    uECC_vli_modAdd(Y1, Y1, X1, curve->p, num_words_secp256k1); /* t2 = 3*x1^2 */
    if (uECC_vli_testBit(Y1, 0)) {
        uECC_word_t carry = uECC_vli_add(Y1, Y1, curve->p, num_words_secp256k1);
        uECC_vli_rshift1(Y1, num_words_secp256k1);
        Y1[num_words_secp256k1 - 1] |= carry << (uECC_WORD_BITS - 1);
    } else {
        uECC_vli_rshift1(Y1, num_words_secp256k1);
    }
    /* t2 = 3/2*(x1^2) = B */
    
    uECC_vli_modSquare_fast(X1, Y1, curve);                     /* t1 = B^2 */
    uECC_vli_modSub(X1, X1, t4, curve->p, num_words_secp256k1); /* t1 = B^2 - A */
    uECC_vli_modSub(X1, X1, t4, curve->p, num_words_secp256k1); /* t1 = B^2 - 2A = x3 */
    
    uECC_vli_modSub(t4, t4, X1, curve->p, num_words_secp256k1); /* t4 = A - x3 */
    uECC_vli_modMult_fast(Y1, Y1, t4, curve);                   /* t2 = B * (A - x3) */
    uECC_vli_modSub(Y1, Y1, t5, curve->p, num_words_secp256k1); /* t2 = B * (A - x3) - y1^4 = y3 */
}

/* Computes result = x^3 + b. result must not overlap x. */
static void x_side_secp256k1(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve) {
    uECC_vli_modSquare_fast(result, x, curve);                                /* r = x^2 */
    uECC_vli_modMult_fast(result, result, x, curve);                          /* r = x^3 */
    uECC_vli_modAdd(result, result, curve->b, curve->p, num_words_secp256k1); /* r = x^3 + b */
}

#if (uECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp256k1)
static void omega_mult_secp256k1(uECC_word_t *result, const uECC_word_t *right);
static void vli_mmod_fast_secp256k1(uECC_word_t *result, uECC_word_t *product) {
    uECC_word_t tmp[2 * num_words_secp256k1];
    uECC_word_t carry;
    
    uECC_vli_clear(tmp, num_words_secp256k1);
    uECC_vli_clear(tmp + num_words_secp256k1, num_words_secp256k1);
    
    omega_mult_secp256k1(tmp, product + num_words_secp256k1); /* (Rq, q) = q * c */
    
    carry = uECC_vli_add(result, product, tmp, num_words_secp256k1); /* (C, r) = r + q       */
    uECC_vli_clear(product, num_words_secp256k1);
    omega_mult_secp256k1(product, tmp + num_words_secp256k1); /* Rq*c */
    carry += uECC_vli_add(result, result, product, num_words_secp256k1); /* (C1, r) = r + Rq*c */
    
    while (carry > 0) {
        --carry;
        uECC_vli_sub(result, result, curve_secp256k1.p, num_words_secp256k1);
    }
    if (uECC_vli_cmp_unsafe(result, curve_secp256k1.p, num_words_secp256k1) > 0) {
        uECC_vli_sub(result, result, curve_secp256k1.p, num_words_secp256k1);
    }
}

#if uECC_WORD_SIZE == 1
static void omega_mult_secp256k1(uint8_t * result, const uint8_t * right) {
    /* Multiply by (2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;
    wordcount_t k;
    
    /* Multiply by (2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    muladd(0xD1, right[0], &r0, &r1, &r2);
    result[0] = r0;
    r0 = r1;
    r1 = r2;
    /* r2 is still 0 */
    
    for (k = 1; k < num_words_secp256k1; ++k) {
        muladd(0x03, right[k - 1], &r0, &r1, &r2);
        muladd(0xD1, right[k], &r0, &r1, &r2);
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    muladd(0x03, right[num_words_secp256k1 - 1], &r0, &r1, &r2);
    result[num_words_secp256k1] = r0;
    result[num_words_secp256k1 + 1] = r1;
    /* add the 2^32 multiple */
    result[4 + num_words_secp256k1] =
        uECC_vli_add(result + 4, result + 4, right, num_words_secp256k1); 
}
#elif uECC_WORD_SIZE == 4
static void omega_mult_secp256k1(uint32_t * result, const uint32_t * right) {
    /* Multiply by (2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    uint32_t carry = 0;
    wordcount_t k;
    
    for (k = 0; k < num_words_secp256k1; ++k) {
        uint64_t p = (uint64_t)0x3D1 * right[k] + carry;
        result[k] = (uint32_t) p;
        carry = p >> 32;
    }
    result[num_words_secp256k1] = carry;
    /* add the 2^32 multiple */
    result[1 + num_words_secp256k1] =
        uECC_vli_add(result + 1, result + 1, right, num_words_secp256k1); 
}
#else
static void omega_mult_secp256k1(uint64_t * result, const uint64_t * right) {
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;
    wordcount_t k;
    
    /* Multiply by (2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    for (k = 0; k < num_words_secp256k1; ++k) {
        muladd(0x1000003D1ull, right[k], &r0, &r1, &r2);
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    result[num_words_secp256k1] = r0;
}
#endif /* uECC_WORD_SIZE */
#endif /* (uECC_OPTIMIZATION_LEVEL > 0 &&  && !asm_mmod_fast_secp256k1) */

#endif /* uECC_SUPPORTS_secp256k1 */

#endif /* _UECC_CURVE_SPECIFIC_H_ */

// End manual include curve-specific.inc

/* Returns 1 if 'point' is the point at infinity, 0 otherwise. */
#define EccPoint_isZero(point, curve) uECC_vli_isZero((point), (curve)->num_words * 2)

/* Point multiplication algorithm using Montgomery's ladder with co-Z coordinates.
From http://eprint.iacr.org/2011/338.pdf
*/

/* Modify (x1, y1) => (x1 * z^2, y1 * z^3) */
static void apply_z(uECC_word_t * X1,
                    uECC_word_t * Y1,
                    const uECC_word_t * const Z,
                    uECC_Curve curve) {
    uECC_word_t t1[uECC_MAX_WORDS];

    uECC_vli_modSquare_fast(t1, Z, curve);    /* z^2 */
    uECC_vli_modMult_fast(X1, X1, t1, curve); /* x1 * z^2 */
    uECC_vli_modMult_fast(t1, t1, Z, curve);  /* z^3 */
    uECC_vli_modMult_fast(Y1, Y1, t1, curve); /* y1 * z^3 */
}

/* P = (x1, y1) => 2P, (x2, y2) => P' */
static void XYcZ_initial_double(uECC_word_t * X1,
                                uECC_word_t * Y1,
                                uECC_word_t * X2,
                                uECC_word_t * Y2,
                                const uECC_word_t * const initial_Z,
                                uECC_Curve curve) {
    uECC_word_t z[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;
    if (initial_Z) {
        uECC_vli_set(z, initial_Z, num_words);
    } else {
        uECC_vli_clear(z, num_words);
        z[0] = 1;
    }

    uECC_vli_set(X2, X1, num_words);
    uECC_vli_set(Y2, Y1, num_words);

    apply_z(X1, Y1, z, curve);
    curve->double_jacobian(X1, Y1, z, curve);
    apply_z(X2, Y2, z, curve);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P' = (x1', y1', Z3), P + Q = (x3, y3, Z3)
   or P => P', Q => P + Q
*/
static void XYcZ_add(uECC_word_t * X1,
                     uECC_word_t * Y1,
                     uECC_word_t * X2,
                     uECC_word_t * Y2,
                     uECC_Curve curve) {
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    uECC_word_t t5[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;

    uECC_vli_modSub(t5, X2, X1, curve->p, num_words); /* t5 = x2 - x1 */
    uECC_vli_modSquare_fast(t5, t5, curve);                  /* t5 = (x2 - x1)^2 = A */
    uECC_vli_modMult_fast(X1, X1, t5, curve);                /* t1 = x1*A = B */
    uECC_vli_modMult_fast(X2, X2, t5, curve);                /* t3 = x2*A = C */
    uECC_vli_modSub(Y2, Y2, Y1, curve->p, num_words); /* t4 = y2 - y1 */
    uECC_vli_modSquare_fast(t5, Y2, curve);                  /* t5 = (y2 - y1)^2 = D */

    uECC_vli_modSub(t5, t5, X1, curve->p, num_words); /* t5 = D - B */
    uECC_vli_modSub(t5, t5, X2, curve->p, num_words); /* t5 = D - B - C = x3 */
    uECC_vli_modSub(X2, X2, X1, curve->p, num_words); /* t3 = C - B */
    uECC_vli_modMult_fast(Y1, Y1, X2, curve);                /* t2 = y1*(C - B) */
    uECC_vli_modSub(X2, X1, t5, curve->p, num_words); /* t3 = B - x3 */
    uECC_vli_modMult_fast(Y2, Y2, X2, curve);                /* t4 = (y2 - y1)*(B - x3) */
    uECC_vli_modSub(Y2, Y2, Y1, curve->p, num_words); /* t4 = y3 */

    uECC_vli_set(X2, t5, num_words);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P + Q = (x3, y3, Z3), P - Q = (x3', y3', Z3)
   or P => P - Q, Q => P + Q
*/
static void XYcZ_addC(uECC_word_t * X1,
                      uECC_word_t * Y1,
                      uECC_word_t * X2,
                      uECC_word_t * Y2,
                      uECC_Curve curve) {
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    uECC_word_t t5[uECC_MAX_WORDS];
    uECC_word_t t6[uECC_MAX_WORDS];
    uECC_word_t t7[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;

    uECC_vli_modSub(t5, X2, X1, curve->p, num_words); /* t5 = x2 - x1 */
    uECC_vli_modSquare_fast(t5, t5, curve);                  /* t5 = (x2 - x1)^2 = A */
    uECC_vli_modMult_fast(X1, X1, t5, curve);                /* t1 = x1*A = B */
    uECC_vli_modMult_fast(X2, X2, t5, curve);                /* t3 = x2*A = C */
    uECC_vli_modAdd(t5, Y2, Y1, curve->p, num_words); /* t5 = y2 + y1 */
    uECC_vli_modSub(Y2, Y2, Y1, curve->p, num_words); /* t4 = y2 - y1 */

    uECC_vli_modSub(t6, X2, X1, curve->p, num_words); /* t6 = C - B */
    uECC_vli_modMult_fast(Y1, Y1, t6, curve);                /* t2 = y1 * (C - B) = E */
    uECC_vli_modAdd(t6, X1, X2, curve->p, num_words); /* t6 = B + C */
    uECC_vli_modSquare_fast(X2, Y2, curve);                  /* t3 = (y2 - y1)^2 = D */
    uECC_vli_modSub(X2, X2, t6, curve->p, num_words); /* t3 = D - (B + C) = x3 */

    uECC_vli_modSub(t7, X1, X2, curve->p, num_words); /* t7 = B - x3 */
    uECC_vli_modMult_fast(Y2, Y2, t7, curve);                /* t4 = (y2 - y1)*(B - x3) */
    uECC_vli_modSub(Y2, Y2, Y1, curve->p, num_words); /* t4 = (y2 - y1)*(B - x3) - E = y3 */

    uECC_vli_modSquare_fast(t7, t5, curve);                  /* t7 = (y2 + y1)^2 = F */
    uECC_vli_modSub(t7, t7, t6, curve->p, num_words); /* t7 = F - (B + C) = x3' */
    uECC_vli_modSub(t6, t7, X1, curve->p, num_words); /* t6 = x3' - B */
    uECC_vli_modMult_fast(t6, t6, t5, curve);                /* t6 = (y2+y1)*(x3' - B) */
    uECC_vli_modSub(Y1, t6, Y1, curve->p, num_words); /* t2 = (y2+y1)*(x3' - B) - E = y3' */

    uECC_vli_set(X1, t7, num_words);
}

/* result may overlap point. */
static void EccPoint_mult(uECC_word_t * result,
                          const uECC_word_t * point,
                          const uECC_word_t * scalar,
                          const uECC_word_t * initial_Z,
                          bitcount_t num_bits,
                          uECC_Curve curve) {
    /* R0 and R1 */
    uECC_word_t Rx[2][uECC_MAX_WORDS];
    uECC_word_t Ry[2][uECC_MAX_WORDS];
    uECC_word_t z[uECC_MAX_WORDS];
    bitcount_t i;
    uECC_word_t nb;
    wordcount_t num_words = curve->num_words;

    uECC_vli_set(Rx[1], point, num_words);
    uECC_vli_set(Ry[1], point + num_words, num_words);

    XYcZ_initial_double(Rx[1], Ry[1], Rx[0], Ry[0], initial_Z, curve);

    for (i = num_bits - 2; i > 0; --i) {
        nb = !uECC_vli_testBit(scalar, i);
        XYcZ_addC(Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb], curve);
        XYcZ_add(Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb], curve);
    }

    nb = !uECC_vli_testBit(scalar, 0);
    XYcZ_addC(Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb], curve);

    /* Find final 1/Z value. */
    uECC_vli_modSub(z, Rx[1], Rx[0], curve->p, num_words); /* X1 - X0 */
    uECC_vli_modMult_fast(z, z, Ry[1 - nb], curve);               /* Yb * (X1 - X0) */
    uECC_vli_modMult_fast(z, z, point, curve);                    /* xP * Yb * (X1 - X0) */
    uECC_vli_modInv(z, z, curve->p, num_words);            /* 1 / (xP * Yb * (X1 - X0)) */
    /* yP / (xP * Yb * (X1 - X0)) */
    uECC_vli_modMult_fast(z, z, point + num_words, curve);
    uECC_vli_modMult_fast(z, z, Rx[1 - nb], curve); /* Xb * yP / (xP * Yb * (X1 - X0)) */
    /* End 1/Z calculation */

    XYcZ_add(Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb], curve);
    apply_z(Rx[0], Ry[0], z, curve);

    uECC_vli_set(result, Rx[0], num_words);
    uECC_vli_set(result + num_words, Ry[0], num_words);
}

static uECC_word_t regularize_k(const uECC_word_t * const k,
                                uECC_word_t *k0,
                                uECC_word_t *k1,
                                uECC_Curve curve) {
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    bitcount_t num_n_bits = curve->num_n_bits;
    uECC_word_t carry = uECC_vli_add(k0, k, curve->n, num_n_words) ||
        (num_n_bits < ((bitcount_t)num_n_words * uECC_WORD_SIZE * 8) &&
         uECC_vli_testBit(k0, num_n_bits));
    uECC_vli_add(k1, k0, curve->n, num_n_words);
    return carry;
}

/* Generates a random integer in the range 0 < random < top.
   Both random and top have num_words words. */
uECC_VLI_API int uECC_generate_random_int(uECC_word_t *random,
                                          const uECC_word_t *top,
                                          wordcount_t num_words) {
    uECC_word_t mask = (uECC_word_t)-1;
    uECC_word_t tries;
    bitcount_t num_bits = uECC_vli_numBits(top, num_words);

    if (!g_rng_function) {
        return 0;
    }

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        if (!g_rng_function((uint8_t *)random, num_words * uECC_WORD_SIZE)) {
            return 0;
        }
        random[num_words - 1] &= mask >> ((bitcount_t)(num_words * uECC_WORD_SIZE * 8 - num_bits));
        if (!uECC_vli_isZero(random, num_words) &&
                uECC_vli_cmp(top, random, num_words) == 1) {
            return 1;
        }
    }
    return 0;
}

static uECC_word_t EccPoint_compute_public_key(uECC_word_t *result,
                                               uECC_word_t *private_key,
                                               uECC_Curve curve) {
    uECC_word_t tmp1[uECC_MAX_WORDS];
    uECC_word_t tmp2[uECC_MAX_WORDS];
    uECC_word_t *p2[2] = {tmp1, tmp2};
    uECC_word_t *initial_Z = 0;
    uECC_word_t carry;

    /* Regularize the bitcount for the private key so that attackers cannot use a side channel
       attack to learn the number of leading zeros. */
    carry = regularize_k(private_key, tmp1, tmp2, curve);

    /* If an RNG function was specified, try to get a random initial Z value to improve
       protection against side-channel attacks. */
    if (g_rng_function) {
        if (!uECC_generate_random_int(p2[carry], curve->p, curve->num_words)) {
            return 0;
        }
        initial_Z = p2[carry];
    }
    EccPoint_mult(result, curve->G, p2[!carry], initial_Z, curve->num_n_bits + 1, curve);

    if (EccPoint_isZero(result, curve)) {
        return 0;
    }
    return 1;
}

#if uECC_WORD_SIZE == 1

uECC_VLI_API void uECC_vli_nativeToBytes(uint8_t *bytes,
                                         int num_bytes,
                                         const uint8_t *native) {
    wordcount_t i;
    for (i = 0; i < num_bytes; ++i) {
        bytes[i] = native[(num_bytes - 1) - i];
    }
}

uECC_VLI_API void uECC_vli_bytesToNative(uint8_t *native,
                                         const uint8_t *bytes,
                                         int num_bytes) {
    uECC_vli_nativeToBytes(native, num_bytes, bytes);
}

#else

uECC_VLI_API void uECC_vli_nativeToBytes(uint8_t *bytes,
                                         int num_bytes,
                                         const uECC_word_t *native) {
    int i;
    for (i = 0; i < num_bytes; ++i) {
        unsigned b = num_bytes - 1 - i;
        bytes[i] = native[b / uECC_WORD_SIZE] >> (8 * (b % uECC_WORD_SIZE));
    }
}

uECC_VLI_API void uECC_vli_bytesToNative(uECC_word_t *native,
                                         const uint8_t *bytes,
                                         int num_bytes) {
    int i;
    uECC_vli_clear(native, (num_bytes + (uECC_WORD_SIZE - 1)) / uECC_WORD_SIZE);
    for (i = 0; i < num_bytes; ++i) {
        unsigned b = num_bytes - 1 - i;
        native[b / uECC_WORD_SIZE] |=
            (uECC_word_t)bytes[i] << (8 * (b % uECC_WORD_SIZE));
    }
}

#endif /* uECC_WORD_SIZE */

int uECC_make_key(uint8_t *public_key,
                  uint8_t *private_key,
                  uECC_Curve curve) {
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *_private = (uECC_word_t *)private_key;
    uECC_word_t *_public = (uECC_word_t *)public_key;
#else
    uECC_word_t _private[uECC_MAX_WORDS];
    uECC_word_t _public[uECC_MAX_WORDS * 2];
#endif
    uECC_word_t tries;

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        if (!uECC_generate_random_int(_private, curve->n, BITS_TO_WORDS(curve->num_n_bits))) {
            return 0;
        }

        if (EccPoint_compute_public_key(_public, _private, curve)) {
#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
            uECC_vli_nativeToBytes(private_key, BITS_TO_BYTES(curve->num_n_bits), _private);
            uECC_vli_nativeToBytes(public_key, curve->num_bytes, _public);
            uECC_vli_nativeToBytes(
                public_key + curve->num_bytes, curve->num_bytes, _public + curve->num_words);
#endif
            return 1;
        }
    }
    return 0;
}

int uECC_shared_secret(const uint8_t *public_key,
                       const uint8_t *private_key,
                       uint8_t *secret,
                       uECC_Curve curve) {
    uECC_word_t _public[uECC_MAX_WORDS * 2];
    uECC_word_t _private[uECC_MAX_WORDS];

    uECC_word_t tmp[uECC_MAX_WORDS];
    uECC_word_t *p2[2] = {_private, tmp};
    uECC_word_t *initial_Z = 0;
    uECC_word_t carry;
    wordcount_t num_words = curve->num_words;
    wordcount_t num_bytes = curve->num_bytes;

#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) _private, private_key, num_bytes);
    bcopy((uint8_t *) _public, public_key, num_bytes*2);
#else
    uECC_vli_bytesToNative(_private, private_key, BITS_TO_BYTES(curve->num_n_bits));
    uECC_vli_bytesToNative(_public, public_key, num_bytes);
    uECC_vli_bytesToNative(_public + num_words, public_key + num_bytes, num_bytes);
#endif

    /* Regularize the bitcount for the private key so that attackers cannot use a side channel
       attack to learn the number of leading zeros. */
    carry = regularize_k(_private, _private, tmp, curve);

    /* If an RNG function was specified, try to get a random initial Z value to improve
       protection against side-channel attacks. */
    if (g_rng_function) {
        if (!uECC_generate_random_int(p2[carry], curve->p, num_words)) {
            return 0;
        }
        initial_Z = p2[carry];
    }

    EccPoint_mult(_public, _public, p2[!carry], initial_Z, curve->num_n_bits + 1, curve);
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) secret, (uint8_t *) _public, num_bytes);
#else
    uECC_vli_nativeToBytes(secret, num_bytes, _public);
#endif
    return !EccPoint_isZero(_public, curve);
}

#if uECC_SUPPORT_COMPRESSED_POINT
void uECC_compress(const uint8_t *public_key, uint8_t *compressed, uECC_Curve curve) {
    wordcount_t i;
    for (i = 0; i < curve->num_bytes; ++i) {
        compressed[i+1] = public_key[i];
    }
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    compressed[0] = 2 + (public_key[curve->num_bytes] & 0x01);
#else
    compressed[0] = 2 + (public_key[curve->num_bytes * 2 - 1] & 0x01);
#endif
}

void uECC_decompress(const uint8_t *compressed, uint8_t *public_key, uECC_Curve curve) {
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *point = (uECC_word_t *)public_key;
#else
    uECC_word_t point[uECC_MAX_WORDS * 2];
#endif
    uECC_word_t *y = point + curve->num_words;
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy(public_key, compressed+1, curve->num_bytes);
#else
    uECC_vli_bytesToNative(point, compressed + 1, curve->num_bytes);
#endif
    curve->x_side(y, point, curve);
    curve->mod_sqrt(y, curve);

    if ((y[0] & 0x01) != (compressed[0] & 0x01)) {
        uECC_vli_sub(y, curve->p, y, curve->num_words);
    }

#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    uECC_vli_nativeToBytes(public_key, curve->num_bytes, point);
    uECC_vli_nativeToBytes(public_key + curve->num_bytes, curve->num_bytes, y);
#endif
}
#endif /* uECC_SUPPORT_COMPRESSED_POINT */

uECC_VLI_API int uECC_valid_point(const uECC_word_t *point, uECC_Curve curve) {
    uECC_word_t tmp1[uECC_MAX_WORDS];
    uECC_word_t tmp2[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;

    /* The point at infinity is invalid. */
    if (EccPoint_isZero(point, curve)) {
        return 0;
    }

    /* x and y must be smaller than p. */
    if (uECC_vli_cmp_unsafe(curve->p, point, num_words) != 1 ||
            uECC_vli_cmp_unsafe(curve->p, point + num_words, num_words) != 1) {
        return 0;
    }

    uECC_vli_modSquare_fast(tmp1, point + num_words, curve);
    curve->x_side(tmp2, point, curve); /* tmp2 = x^3 + ax + b */

    /* Make sure that y^2 == x^3 + ax + b */
    return (int)(uECC_vli_equal(tmp1, tmp2, num_words));
}

int uECC_valid_public_key(const uint8_t *public_key, uECC_Curve curve) {
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *_public = (uECC_word_t *)public_key;
#else
    uECC_word_t _public[uECC_MAX_WORDS * 2];
#endif

#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    uECC_vli_bytesToNative(_public, public_key, curve->num_bytes);
    uECC_vli_bytesToNative(
        _public + curve->num_words, public_key + curve->num_bytes, curve->num_bytes);
#endif
    return uECC_valid_point(_public, curve);
}

int uECC_compute_public_key(const uint8_t *private_key, uint8_t *public_key, uECC_Curve curve) {
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *_private = (uECC_word_t *)private_key;
    uECC_word_t *_public = (uECC_word_t *)public_key;
#else
    uECC_word_t _private[uECC_MAX_WORDS];
    uECC_word_t _public[uECC_MAX_WORDS * 2];
#endif

#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    uECC_vli_bytesToNative(_private, private_key, BITS_TO_BYTES(curve->num_n_bits));
#endif

    /* Make sure the private key is in the range [1, n-1]. */
    if (uECC_vli_isZero(_private, BITS_TO_WORDS(curve->num_n_bits))) {
        return 0;
    }

    if (uECC_vli_cmp(curve->n, _private, BITS_TO_WORDS(curve->num_n_bits)) != 1) {
        return 0;
    }

    /* Compute public key. */
    if (!EccPoint_compute_public_key(_public, _private, curve)) {
        return 0;
    }

#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    uECC_vli_nativeToBytes(public_key, curve->num_bytes, _public);
    uECC_vli_nativeToBytes(
        public_key + curve->num_bytes, curve->num_bytes, _public + curve->num_words);
#endif
    return 1;
}


/* -------- ECDSA code -------- */

static void bits2int(uECC_word_t *native,
                     const uint8_t *bits,
                     unsigned bits_size,
                     uECC_Curve curve) {
    unsigned num_n_bytes = BITS_TO_BYTES(curve->num_n_bits);
    unsigned num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    int shift;
    uECC_word_t carry;
    uECC_word_t *ptr;

    if (bits_size > num_n_bytes) {
        bits_size = num_n_bytes;
    }

    uECC_vli_clear(native, num_n_words);
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) native, bits, bits_size);
#else
    uECC_vli_bytesToNative(native, bits, bits_size);
#endif
    if (bits_size * 8 <= (unsigned)curve->num_n_bits) {
        return;
    }
    shift = bits_size * 8 - curve->num_n_bits;
    carry = 0;
    ptr = native + num_n_words;
    while (ptr-- > native) {
        uECC_word_t temp = *ptr;
        *ptr = (temp >> shift) | carry;
        carry = temp << (uECC_WORD_BITS - shift);
    }

    /* Reduce mod curve_n */
    if (uECC_vli_cmp_unsafe(curve->n, native, num_n_words) != 1) {
        uECC_vli_sub(native, native, curve->n, num_n_words);
    }
}

static int uECC_sign_with_k_internal(const uint8_t *private_key,
                            const uint8_t *message_hash,
                            unsigned hash_size,
                            uECC_word_t *k,
                            uint8_t *signature,
                            uECC_Curve curve) {

    uECC_word_t tmp[uECC_MAX_WORDS];
    uECC_word_t s[uECC_MAX_WORDS];
    uECC_word_t *k2[2] = {tmp, s};
    uECC_word_t *initial_Z = 0;
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *p = (uECC_word_t *)signature;
#else
    uECC_word_t p[uECC_MAX_WORDS * 2];
#endif
    uECC_word_t carry;
    wordcount_t num_words = curve->num_words;
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    bitcount_t num_n_bits = curve->num_n_bits;

    /* Make sure 0 < k < curve_n */
    if (uECC_vli_isZero(k, num_words) || uECC_vli_cmp(curve->n, k, num_n_words) != 1) {
        return 0;
    }

    carry = regularize_k(k, tmp, s, curve);
    /* If an RNG function was specified, try to get a random initial Z value to improve
       protection against side-channel attacks. */
    if (g_rng_function) {
        if (!uECC_generate_random_int(k2[carry], curve->p, num_words)) {
            return 0;
        }
        initial_Z = k2[carry];
    }
    EccPoint_mult(p, curve->G, k2[!carry], initial_Z, num_n_bits + 1, curve);
    if (uECC_vli_isZero(p, num_words)) {
        return 0;
    }

    /* If an RNG function was specified, get a random number
       to prevent side channel analysis of k. */
    if (!g_rng_function) {
        uECC_vli_clear(tmp, num_n_words);
        tmp[0] = 1;
    } else if (!uECC_generate_random_int(tmp, curve->n, num_n_words)) {
        return 0;
    }

    /* Prevent side channel analysis of uECC_vli_modInv() to determine
       bits of k / the private key by premultiplying by a random number */
    uECC_vli_modMult(k, k, tmp, curve->n, num_n_words); /* k' = rand * k */
    uECC_vli_modInv(k, k, curve->n, num_n_words);       /* k = 1 / k' */
    uECC_vli_modMult(k, k, tmp, curve->n, num_n_words); /* k = 1 / k */

#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    uECC_vli_nativeToBytes(signature, curve->num_bytes, p); /* store r */
#endif

#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) tmp, private_key, BITS_TO_BYTES(curve->num_n_bits));
#else
    uECC_vli_bytesToNative(tmp, private_key, BITS_TO_BYTES(curve->num_n_bits)); /* tmp = d */
#endif

    s[num_n_words - 1] = 0;
    uECC_vli_set(s, p, num_words);
    uECC_vli_modMult(s, tmp, s, curve->n, num_n_words); /* s = r*d */

    bits2int(tmp, message_hash, hash_size, curve);
    uECC_vli_modAdd(s, tmp, s, curve->n, num_n_words); /* s = e + r*d */
    uECC_vli_modMult(s, s, k, curve->n, num_n_words);  /* s = (e + r*d) / k */
    if (uECC_vli_numBits(s, num_n_words) > (bitcount_t)curve->num_bytes * 8) {
        return 0;
    }
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) signature + curve->num_bytes, (uint8_t *) s, curve->num_bytes);
#else
    uECC_vli_nativeToBytes(signature + curve->num_bytes, curve->num_bytes, s);
#endif
    return 1;
}

/* For testing - sign with an explicitly specified k value */
int uECC_sign_with_k(const uint8_t *private_key,
                            const uint8_t *message_hash,
                            unsigned hash_size,
                            const uint8_t *k,
                            uint8_t *signature,
                            uECC_Curve curve) {
    uECC_word_t k2[uECC_MAX_WORDS];
    bits2int(k2, k, BITS_TO_BYTES(curve->num_n_bits), curve);
    return uECC_sign_with_k_internal(private_key, message_hash, hash_size, k2, signature, curve);
}

int uECC_sign(const uint8_t *private_key,
              const uint8_t *message_hash,
              unsigned hash_size,
              uint8_t *signature,
              uECC_Curve curve) {
    uECC_word_t k[uECC_MAX_WORDS];
    uECC_word_t tries;

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        if (!uECC_generate_random_int(k, curve->n, BITS_TO_WORDS(curve->num_n_bits))) {
            return 0;
        }

        if (uECC_sign_with_k_internal(private_key, message_hash, hash_size, k, signature, curve)) {
            return 1;
        }
    }
    return 0;
}

/* Compute an HMAC using K as a key (as in RFC 6979). Note that K is always
   the same size as the hash result size. */
static void HMAC_init(const uECC_HashContext *hash_context, const uint8_t *K) {
    uint8_t *pad = hash_context->tmp + 2 * hash_context->result_size;
    unsigned i;
    for (i = 0; i < hash_context->result_size; ++i)
        pad[i] = K[i] ^ 0x36;
    for (; i < hash_context->block_size; ++i)
        pad[i] = 0x36;

    hash_context->init_hash(hash_context);
    hash_context->update_hash(hash_context, pad, hash_context->block_size);
}

static void HMAC_update(const uECC_HashContext *hash_context,
                        const uint8_t *message,
                        unsigned message_size) {
    hash_context->update_hash(hash_context, message, message_size);
}

static void HMAC_finish(const uECC_HashContext *hash_context,
                        const uint8_t *K,
                        uint8_t *result) {
    uint8_t *pad = hash_context->tmp + 2 * hash_context->result_size;
    unsigned i;
    for (i = 0; i < hash_context->result_size; ++i)
        pad[i] = K[i] ^ 0x5c;
    for (; i < hash_context->block_size; ++i)
        pad[i] = 0x5c;

    hash_context->finish_hash(hash_context, result);

    hash_context->init_hash(hash_context);
    hash_context->update_hash(hash_context, pad, hash_context->block_size);
    hash_context->update_hash(hash_context, result, hash_context->result_size);
    hash_context->finish_hash(hash_context, result);
}

/* V = HMAC_K(V) */
static void update_V(const uECC_HashContext *hash_context, uint8_t *K, uint8_t *V) {
    HMAC_init(hash_context, K);
    HMAC_update(hash_context, V, hash_context->result_size);
    HMAC_finish(hash_context, K, V);
}

/* Deterministic signing, similar to RFC 6979. Differences are:
    * We just use H(m) directly rather than bits2octets(H(m))
      (it is not reduced modulo curve_n).
    * We generate a value for k (aka T) directly rather than converting endianness.

   Layout of hash_context->tmp: <K> | <V> | (1 byte overlapped 0x00 or 0x01) / <HMAC pad> */
int uECC_sign_deterministic(const uint8_t *private_key,
                            const uint8_t *message_hash,
                            unsigned hash_size,
                            const uECC_HashContext *hash_context,
                            uint8_t *signature,
                            uECC_Curve curve) {
    uint8_t *K = hash_context->tmp;
    uint8_t *V = K + hash_context->result_size;
    wordcount_t num_bytes = curve->num_bytes;
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    bitcount_t num_n_bits = curve->num_n_bits;
    uECC_word_t tries;
    unsigned i;
    for (i = 0; i < hash_context->result_size; ++i) {
        V[i] = 0x01;
        K[i] = 0;
    }

    /* K = HMAC_K(V || 0x00 || int2octets(x) || h(m)) */
    HMAC_init(hash_context, K);
    V[hash_context->result_size] = 0x00;
    HMAC_update(hash_context, V, hash_context->result_size + 1);
    HMAC_update(hash_context, private_key, num_bytes);
    HMAC_update(hash_context, message_hash, hash_size);
    HMAC_finish(hash_context, K, K);

    update_V(hash_context, K, V);

    /* K = HMAC_K(V || 0x01 || int2octets(x) || h(m)) */
    HMAC_init(hash_context, K);
    V[hash_context->result_size] = 0x01;
    HMAC_update(hash_context, V, hash_context->result_size + 1);
    HMAC_update(hash_context, private_key, num_bytes);
    HMAC_update(hash_context, message_hash, hash_size);
    HMAC_finish(hash_context, K, K);

    update_V(hash_context, K, V);

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        uECC_word_t T[uECC_MAX_WORDS];
        uint8_t *T_ptr = (uint8_t *)T;
        wordcount_t T_bytes = 0;
        for (;;) {
            update_V(hash_context, K, V);
            for (i = 0; i < hash_context->result_size; ++i) {
                T_ptr[T_bytes++] = V[i];
                if (T_bytes >= num_n_words * uECC_WORD_SIZE) {
                    goto filled;
                }
            }
        }
    filled:
        if ((bitcount_t)num_n_words * uECC_WORD_SIZE * 8 > num_n_bits) {
            uECC_word_t mask = (uECC_word_t)-1;
            T[num_n_words - 1] &=
                mask >> ((bitcount_t)(num_n_words * uECC_WORD_SIZE * 8 - num_n_bits));
        }

        if (uECC_sign_with_k_internal(private_key, message_hash, hash_size, T, signature, curve)) {
            return 1;
        }

        /* K = HMAC_K(V || 0x00) */
        HMAC_init(hash_context, K);
        V[hash_context->result_size] = 0x00;
        HMAC_update(hash_context, V, hash_context->result_size + 1);
        HMAC_finish(hash_context, K, K);

        update_V(hash_context, K, V);
    }
    return 0;
}

static bitcount_t smax(bitcount_t a, bitcount_t b) {
    return (a > b ? a : b);
}

int uECC_verify(const uint8_t *public_key,
                const uint8_t *message_hash,
                unsigned hash_size,
                const uint8_t *signature,
                uECC_Curve curve) {
    uECC_word_t u1[uECC_MAX_WORDS], u2[uECC_MAX_WORDS];
    uECC_word_t z[uECC_MAX_WORDS];
    uECC_word_t sum[uECC_MAX_WORDS * 2];
    uECC_word_t rx[uECC_MAX_WORDS];
    uECC_word_t ry[uECC_MAX_WORDS];
    uECC_word_t tx[uECC_MAX_WORDS];
    uECC_word_t ty[uECC_MAX_WORDS];
    uECC_word_t tz[uECC_MAX_WORDS];
    const uECC_word_t *points[4];
    const uECC_word_t *point;
    bitcount_t num_bits;
    bitcount_t i;
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *_public = (uECC_word_t *)public_key;
#else
    uECC_word_t _public[uECC_MAX_WORDS * 2];
#endif
    uECC_word_t r[uECC_MAX_WORDS], s[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);

    rx[num_n_words - 1] = 0;
    r[num_n_words - 1] = 0;
    s[num_n_words - 1] = 0;

#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy((uint8_t *) r, signature, curve->num_bytes);
    bcopy((uint8_t *) s, signature + curve->num_bytes, curve->num_bytes);
#else
    uECC_vli_bytesToNative(_public, public_key, curve->num_bytes);
    uECC_vli_bytesToNative(
        _public + num_words, public_key + curve->num_bytes, curve->num_bytes);
    uECC_vli_bytesToNative(r, signature, curve->num_bytes);
    uECC_vli_bytesToNative(s, signature + curve->num_bytes, curve->num_bytes);
#endif

    /* r, s must not be 0. */
    if (uECC_vli_isZero(r, num_words) || uECC_vli_isZero(s, num_words)) {
        return 0;
    }

    /* r, s must be < n. */
    if (uECC_vli_cmp_unsafe(curve->n, r, num_n_words) != 1 ||
            uECC_vli_cmp_unsafe(curve->n, s, num_n_words) != 1) {
        return 0;
    }

    /* Calculate u1 and u2. */
    uECC_vli_modInv(z, s, curve->n, num_n_words); /* z = 1/s */
    u1[num_n_words - 1] = 0;
    bits2int(u1, message_hash, hash_size, curve);
    uECC_vli_modMult(u1, u1, z, curve->n, num_n_words); /* u1 = e/s */
    uECC_vli_modMult(u2, r, z, curve->n, num_n_words); /* u2 = r/s */

    /* Calculate sum = G + Q. */
    uECC_vli_set(sum, _public, num_words);
    uECC_vli_set(sum + num_words, _public + num_words, num_words);
    uECC_vli_set(tx, curve->G, num_words);
    uECC_vli_set(ty, curve->G + num_words, num_words);
    uECC_vli_modSub(z, sum, tx, curve->p, num_words); /* z = x2 - x1 */
    XYcZ_add(tx, ty, sum, sum + num_words, curve);
    uECC_vli_modInv(z, z, curve->p, num_words); /* z = 1/z */
    apply_z(sum, sum + num_words, z, curve);

    /* Use Shamir's trick to calculate u1*G + u2*Q */
    points[0] = 0;
    points[1] = curve->G;
    points[2] = _public;
    points[3] = sum;
    num_bits = smax(uECC_vli_numBits(u1, num_n_words),
                    uECC_vli_numBits(u2, num_n_words));

    point = points[(!!uECC_vli_testBit(u1, num_bits - 1)) |
                   ((!!uECC_vli_testBit(u2, num_bits - 1)) << 1)];
    uECC_vli_set(rx, point, num_words);
    uECC_vli_set(ry, point + num_words, num_words);
    uECC_vli_clear(z, num_words);
    z[0] = 1;

    for (i = num_bits - 2; i >= 0; --i) {
        uECC_word_t index;
        curve->double_jacobian(rx, ry, z, curve);

        index = (!!uECC_vli_testBit(u1, i)) | ((!!uECC_vli_testBit(u2, i)) << 1);
        point = points[index];
        if (point) {
            uECC_vli_set(tx, point, num_words);
            uECC_vli_set(ty, point + num_words, num_words);
            apply_z(tx, ty, z, curve);
            uECC_vli_modSub(tz, rx, tx, curve->p, num_words); /* Z = x2 - x1 */
            XYcZ_add(tx, ty, rx, ry, curve);
            uECC_vli_modMult_fast(z, z, tz, curve);
        }
    }

    uECC_vli_modInv(z, z, curve->p, num_words); /* Z = 1/Z */
    apply_z(rx, ry, z, curve);

    /* v = x1 (mod n) */
    if (uECC_vli_cmp_unsafe(curve->n, rx, num_n_words) != 1) {
        uECC_vli_sub(rx, rx, curve->n, num_n_words);
    }

    /* Accept only if v == r. */
    return (int)(uECC_vli_equal(rx, r, num_words));
}

#if uECC_ENABLE_VLI_API

unsigned uECC_curve_num_words(uECC_Curve curve) {
    return curve->num_words;
}

unsigned uECC_curve_num_bytes(uECC_Curve curve) {
    return curve->num_bytes;
}

unsigned uECC_curve_num_bits(uECC_Curve curve) {
    return curve->num_bytes * 8;
}

unsigned uECC_curve_num_n_words(uECC_Curve curve) {
    return BITS_TO_WORDS(curve->num_n_bits);
}

unsigned uECC_curve_num_n_bytes(uECC_Curve curve) {
    return BITS_TO_BYTES(curve->num_n_bits);
}

unsigned uECC_curve_num_n_bits(uECC_Curve curve) {
    return curve->num_n_bits;
}

const uECC_word_t *uECC_curve_p(uECC_Curve curve) {
    return curve->p;
}

const uECC_word_t *uECC_curve_n(uECC_Curve curve) {
    return curve->n;
}

const uECC_word_t *uECC_curve_G(uECC_Curve curve) {
    return curve->G;
}

const uECC_word_t *uECC_curve_b(uECC_Curve curve) {
    return curve->b;
}

#if uECC_SUPPORT_COMPRESSED_POINT
void uECC_vli_mod_sqrt(uECC_word_t *a, uECC_Curve curve) {
    curve->mod_sqrt(a, curve);
}
#endif

void uECC_vli_mmod_fast(uECC_word_t *result, uECC_word_t *product, uECC_Curve curve) {
#if (uECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    uECC_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

void uECC_point_mult(uECC_word_t *result,
                     const uECC_word_t *point,
                     const uECC_word_t *scalar,
                     uECC_Curve curve) {
    uECC_word_t tmp1[uECC_MAX_WORDS];
    uECC_word_t tmp2[uECC_MAX_WORDS];
    uECC_word_t *p2[2] = {tmp1, tmp2};
    uECC_word_t carry = regularize_k(scalar, tmp1, tmp2, curve);

    EccPoint_mult(result, point, p2[!carry], 0, curve->num_n_bits + 1, curve);
}

#endif /* uECC_ENABLE_VLI_API */
