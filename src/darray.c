
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>


#define ATTR_COUNTED_BY(count) __attribute__((counted_by(count)))


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define NOINLINE noinline
#elif defined(__GNUC__) || defined(__clang__)
#  define NOINLINE noinline
#elif defined(_MSC_VER)
#  define NOINLINE __declspec(noinline)
#else
#  define NOINLINE
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define NODISCARD nodiscard
#elif defined(__GNUC__) || defined(__clang__)
#  define NODISCARD warn_unused_result
#elif defined(_MSC_VER)
#  include <sal.h>
#  define NODISCARD _Check_return_
#else
#  define NODISCARD
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    /* C23 standard attribute */
    #define MAYBE_UNUSED maybe_unused
#elif defined(__GNUC__) || defined(__clang__)
    /* GCC and Clang legacy attribute */
    #define MAYBE_UNUSED unused
#else
    /* Fallback for other compilers */
    #define MAYBE_UNUSED
#endif

/* Note: no C23 standard attribute */
#if defined(__GNUC__) || defined(__clang__)
    #define ALWAYS_INLINE always_inline
#elif defined(_MSC_VER)
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define ATTRIBS(...) [[__VA_ARGS__]]
#elif defined(__GNUC__) || defined(__clang__)
    #define ATTRIBS(...) __attribute__((__VA_ARGS__))
#elif defined(_MSC_VER)
    #define ATTRIB_APPLY_1(a)          a
    #define ATTRIB_APPLY_2(a, b)       a b
    #define ATTRIB_APPLY_3(a, b, c)    a b c
    #define ATTRIB_APPLY_4(a, b, c, d) a b c d

    #define ATTRIB_COUNT(_1, _2, _3, _4N, ...) N
    #define ATTRIBS(...) ATTRIB_COUNT(__VA_ARGS__, \
        ATTRIB_APPLY_4, ATTRIB_APPLY_3, ATTRIB_APPLY_2, ATTRIB_APPLY_1)(__VA_ARGS__)
#else
    #define ATTRIBS(...)
#endif


typedef unsigned int darray_size_t;
#define darray_max_alloc(item_size) (UINT_MAX / (item_size))

enum {
    DARRAY_SIZE_WIDTH = sizeof(darray_size_t) * CHAR_BIT,
    DARRAY_SIZE_MAX = UINT_MAX
};

#define darray(type) struct {         \
    /** Current size */               \
    darray_size_t size;               \
    /** Count of allocated items */   \
    darray_size_t alloc;              \
    /** Array of items */             \
    type *item ATTR_COUNTED_BY(size); \
}

#define darray_new() { 0, 0, 0 }


/*** Access ***/

#define darray_item(arr, i)     ((arr).item[i])
#define darray_items(arr)       ((arr).item)
#define darray_size(arr)        ((arr).size)
#define darray_empty(arr)       ((arr).size == 0)

/*** Size management ***/

ATTRIBS(NODISCARD, noinline) bool
darray_heap_grow(void ** restrict data, size_t item_size,
                 darray_size_t * restrict capacity,
                 darray_size_t need);

ATTRIBS(NODISCARD, noinline) bool
darray_heap_grow(void ** restrict data, size_t item_size,
                 darray_size_t * restrict capacity,
                 darray_size_t need)
{
    const darray_size_t need_max = darray_max_alloc(item_size);
    darray_size_t new = *capacity;

    if (need > new) {
        if (new == 0) {
            new = 4;
        } else if (need > need_max / 2) {
            return false;
        }

        while (new < need) {
            new *= 2;
        }

        void * tmp = realloc(*data, new * item_size);
        if (!tmp) {
            return false;
        }

        *data = tmp;
        *capacity = new;
    }

    return true;
}


ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_resize_(void ** restrict data, size_t item_size,
               darray_size_t * restrict capacity, darray_size_t * restrict size,
               darray_size_t need)
{
    if (!darray_heap_grow(data, item_size, capacity, need))
        return false;
    *size = need;
    return true;
}

#define darray_resize(arr, new_size) \
    darray_resize_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                   &(arr).size, new_size)

/*** Insertion (single item) ***/

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_append_(void ** restrict data, size_t item_size,
               darray_size_t * restrict capacity, darray_size_t * restrict size,
               const void * item)
{
    if (!darray_resize_(data, item_size, capacity, size, *size + 1))
        return false;

    memcpy((char *)*data + (*size - 1) * item_size, item, item_size);

    return true;
}

#define darray_append(arr, i) \
    darray_append_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                   &(arr).size, &(i))

typedef struct S { int a; const char * b; } S;
typedef darray(S) SS;

typedef darray(int) darray_int;

int main() {
    SS arr = darray_new();
    struct S s1 = { .a = 1, .b = "coucou" };
    struct S s2 = { .a = 2, .b = "coucou" };
    struct S s3 = { .a = 3, .b = "coucou" };
    darray_append(arr, s1);
    if (!darray_append(arr, s2) || !darray_append(arr, s3)) {
        printf("Failed!");
        exit(1);
    }

    const struct S *s = &darray_item(arr, 2);
    printf("!!! %d %s\n", s->a, s->b);


    darray_int arr2 = darray_new();
    darray_append(arr2, (int){1});
    printf("!!! %d\n", darray_item(arr2, 0));

}
