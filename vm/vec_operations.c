#include <stdio.h>
#include <stdlib.h>

#include <emmintrin.h>
#include <smmintrin.h>

#include <vm.h>

static inline __m128
normalize_mask_ps(__m128 mask)
{
    return _mm_and_ps(mask, _mm_set1_ps(1.0f));
}

void
v_add(const union VMNumericValue *lhs_value,
      const union VMNumericValue *rhs_value,
      union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);
    __m128 result = _mm_add_ps(lhs, rhs);

    _mm_storeu_ps(result_value->v, result);
}

void
v_sub(const union VMNumericValue *lhs_value,
      const union VMNumericValue *rhs_value,
      union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);
    __m128 result = _mm_sub_ps(lhs, rhs);

    _mm_storeu_ps(result_value->v, result);
}

void
v_mul(const union VMNumericValue *lhs_value,
      const union VMNumericValue *rhs_value,
      union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);
    __m128 result = _mm_mul_ps(lhs, rhs);

    _mm_storeu_ps(result_value->v, result);
}

void
v_div(const union VMNumericValue *lhs_value,
      const union VMNumericValue *rhs_value,
      union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);
    __m128 result = _mm_div_ps(lhs, rhs);

    _mm_storeu_ps(result_value->v, result);
}

void
v_mod(const union VMNumericValue *lhs_value,
      const union VMNumericValue *rhs_value,
      union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);

    __m128 quotient = _mm_div_ps(lhs, rhs);

    quotient = _mm_round_ps(
        quotient,
        _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC
    );

    __m128 result = _mm_sub_ps(
        lhs,
        _mm_mul_ps(quotient, rhs)
    );

    _mm_storeu_ps(result_value->v, result);
}

void
v_equal(const union VMNumericValue *lhs_value,
        const union VMNumericValue *rhs_value,
        union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);

    __m128 mask = _mm_cmpeq_ps(lhs, rhs);
    __m128 result = normalize_mask_ps(mask);

    _mm_storeu_ps(result_value->v, result);
}

void
v_notequal(const union VMNumericValue *lhs_value,
           const union VMNumericValue *rhs_value,
           union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);

    __m128 mask = _mm_cmpneq_ps(lhs, rhs);
    __m128 result = normalize_mask_ps(mask);

    _mm_storeu_ps(result_value->v, result);
}

void
v_greater(const union VMNumericValue *lhs_value,
          const union VMNumericValue *rhs_value,
          union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);

    __m128 mask = _mm_cmpgt_ps(lhs, rhs);
    __m128 result = normalize_mask_ps(mask);

    _mm_storeu_ps(result_value->v, result);
}

void
v_less(const union VMNumericValue *lhs_value,
       const union VMNumericValue *rhs_value,
       union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);

    __m128 mask = _mm_cmplt_ps(lhs, rhs);
    __m128 result = normalize_mask_ps(mask);

    _mm_storeu_ps(result_value->v, result);
}

void
v_equalgreater(const union VMNumericValue *lhs_value,
               const union VMNumericValue *rhs_value,
               union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);

    __m128 mask = _mm_cmpge_ps(lhs, rhs);
    __m128 result = normalize_mask_ps(mask);

    _mm_storeu_ps(result_value->v, result);
}

void
v_equalless(const union VMNumericValue *lhs_value,
            const union VMNumericValue *rhs_value,
            union VMNumericValue *result_value)
{
    __m128 lhs = _mm_loadu_ps(lhs_value->v);
    __m128 rhs = _mm_loadu_ps(rhs_value->v);

    __m128 mask = _mm_cmple_ps(lhs, rhs);
    __m128 result = normalize_mask_ps(mask);

    _mm_storeu_ps(result_value->v, result);
}

void
v_or(const union VMNumericValue *lhs_value,
     const union VMNumericValue *rhs_value,
     union VMNumericValue *result_value)
{
    (void)lhs_value;
    (void)rhs_value;
    (void)result_value;

    fprintf(stderr, "Error: Logical OR is not supported for vector types.\n");
    exit(EXIT_FAILURE);
}

void
v_and(const union VMNumericValue *lhs_value,
      const union VMNumericValue *rhs_value,
      union VMNumericValue *result_value)
{
    (void)lhs_value;
    (void)rhs_value;
    (void)result_value;

    fprintf(stderr, "Error: Logical AND is not supported for vector types.\n");
    exit(EXIT_FAILURE);
}
