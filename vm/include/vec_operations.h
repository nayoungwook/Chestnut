#ifndef VEC_OPERATIONS_H
#define VEC_OPERATIONS_H

union VMNumericValue;

void v_add(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_sub(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_mul(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_div(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_mod(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_equal(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_notequal(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_greater(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_less(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_equalgreater(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_equalless(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value);
void v_or(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	  union VMNumericValue *result_value); // must be error.
void v_and(const union VMNumericValue *lhs_value,
		  const union VMNumericValue *rhs_value,
	   union VMNumericValue *result_value); // must be error.

#endif
