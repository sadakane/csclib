//////////////////////////////////////////////////////////
/**
 * @file slice.h
 * @brief Utilities for operations on `share_array` (such as concatenation).
 *
 * This module provides local `share_array` manipulation routines.
 * No communication is performed by these operations.
 */
// Operations such as concatenation for share_array
// No communication is performed
//////////////////////////////////////////////////////////
#ifndef SLICE_H
 #define SLICE_H
#include "share_core.h"
 ////////////////////////////////////////////
// a[i] := b[j]
////////////////////////////////////////////
static void share_setshare(share_array a, int i, share_array b, int j)
{
  //if (_party >  2) return;
  if (a == NULL || b == NULL) {
    printf("share_setshare: a = %p b = %p\n", a, b);
    return;
  }
  if (_party > max_partyid(a)) return;

  if (i < 0 || i >= a->n) {
    printf("share_setshare a: n %d i %d\n", a->n, i);
    exit(1);
  }
  if (j < 0 || j >= b->n) {
    printf("share_setshare b: n %d j %d\n", b->n, j);
    exit(1);
  }
  if (a->q != b->q) {
    printf("share_setshare a->q %d b->q %d\n", (int)a->q, (int)b->q);
    exit(1);
  }
  if (a->A != NULL && b->A != NULL) pa_set(a->A,i, pa_get(b->A,j));
}
#define _setshare share_setshare

////////////////////////////////////////////
// a[is:ie) := b[js:je)
////////////////////////////////////////////
static void share_setshares(share_array a, int is, int ie, share_array b, int js)
{
//  if (_party >  2) return;
  if (a == NULL || b == NULL) {
    printf("share_setshares: a = %p b = %p\n", a, b);
    return;
  }
  if (_party > max_partyid(a)) return;

  if (is < 0 || is >= a->n) {
    printf("share_setshares a: n %d is %d\n", a->n, is);
    exit(1);
  }
  if (ie > a->n) {
    printf("share_setshares a: n %d ie %d\n", a->n, ie);
    exit(1);
  }
  if (js < 0 || js >= b->n) {
    printf("share_setshares b: n %d js %d\n", b->n, js);
    exit(1);
  }
  if (js + (ie-is) > b->n) {
    printf("share_setshares b: n %d is %d ie %d js %d\n", b->n, is, ie, js);
    exit(1);
  }
  if (a->q != b->q) {
    printf("share_setshares a->q %d b->q %d\n", (int)a->q, (int)b->q);
    exit(1);
  }
  pa_iter itr_b = pa_iter_pos_new(b->A, js);
  if (a->A != NULL && b->A != NULL) {
    for (int i = 0; i < ie-is; i++) {
      pa_set(a->A,is + i, pa_iter_get(itr_b));
    }
  }
  pa_iter_free(itr_b);
}
#define _setshares share_setshares

#if 0
static void share_addshare(share_array a, int i, share_array b, int j)
{
  share_t q = a->q;
  share_t atmp = share_getraw(a, i);
  share_t btmp = share_getraw(b, j);
  share_t c = MOD(atmp + btmp);
  share_setraw(a, i, c);
}

static void share_subshare(share_array a, int i, share_array b, int j)
{
  share_t q = a->q;
  share_t atmp = share_getraw(a, i);
  share_t btmp = share_getraw(b, j);
  share_t c = MOD(atmp - btmp);
  share_setraw(a, i, c);
}

static void share_mulshare(share_array a, int i, share_t x)
{
  share_t q = a->q;
  share_t atmp = share_getraw(a, i);
  share_t c = MOD(atmp * x);
  share_setraw(a, i, c);
}
#endif

static share_array share_concat(share_array a, share_array b)
{
  if (a == NULL || b == NULL) {
    printf("share_concat: a = %p b = %p\n", a, b);
    return NULL;
  }
  if (a->q != b->q) {
    printf("share_concat a->q %d b->q %d\n", (int)a->q, (int)b->q);
  }
  NEWT(share_array, ans);
  ans->n = a->n + b->n;
  ans->q = a->q;
  ans->type = a->type;
  ans->irr_poly = a->irr_poly;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(ans->n, a->A->w, a->A->type);
  for (int i=0; i<a->n; i++) pa_set(ans->A,i,pa_get(a->A,i)); // needs optimization
  for (int i=0; i<b->n; i++) pa_set(ans->A,a->n + i, pa_get(b->A,i));
  return ans;
}
#define _concat share_concat

static void share_concat_(share_array a, share_array b)
{
  if (a == NULL || b == NULL) {
    printf("share_concat_: a = %p b = %p\n", a, b);
    return;
  }
  if (a->q != b->q) {
    printf("share_concat_: a->q %d b->q %d\n", (int)a->q, (int)b->q);
  }
  i64 old_n = a->n;
  i64 new_n = a->n + b->n;
  pa_resize(a->A, new_n);
  a->n = new_n;
  share_setshares(a, old_n, new_n, b, 0);
}
#define _concat_ share_concat_

static share_array share_insert_head(share_array a, share_t x)
{
  if (a == NULL) {
    printf("share_insert_head: a = %p\n", a);
    return NULL;
  }
  NEWT(share_array, ans);
  ans->n = a->n + 1;
  ans->q = a->q;
  ans->type = a->type;
  ans->irr_poly = a->irr_poly;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(ans->n, a->A->w, a->A->type);
  if (_party < 2) {
    pa_set(ans->A, 0, x);
  } else {
    pa_set(ans->A, 0, 0);
  }
  for (int i=1; i<ans->n; i++) pa_set(ans->A, i, pa_get(a->A, i-1));
  return ans;
}
#define _insert_head share_insert_head

// The function below is slow and should not be used
static void share_insert_head_(share_array a, share_t x)
{
  if (a == NULL) {
    printf("share_insert_head_: a = %p\n", a);
    return;
  }
  share_array tmp = share_insert_head(a, x);
  pa_free(a->A);  *a = *tmp;  free(tmp);
}
#define _insert_head_ share_insert_head_

static share_array share_insert_tail(share_array a, share_t x)
{
  if (a == NULL) {
    printf("share_insert_tail: a = %p\n", a);
    return NULL;
  }
  NEWT(share_array, ans);
  *ans = *a;
  ans->n = a->n + 1;
  ans->q = a->q;
  ans->irr_poly = a->irr_poly;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(ans->n, a->A->w, a->A->type);
  if (_party < 2) {
    pa_set(ans->A, ans->n-1, x);
  } else {
    pa_set(ans->A, ans->n-1, 0);
  }
  for (int i=0; i<ans->n-1; i++) pa_set(ans->A, i, pa_get(a->A, i));
  return ans;
}
#define _insert_tail share_insert_tail

static void share_insert_tail_(share_array a, share_t x)
{
  if (a == NULL) {
    printf("share_insert_tail_: a = %p\n", a);
    return;
  }
  i64 n = a->n;
  pa_resize(a->A, n+1);
  if (_party < 2) {
    pa_set(a->A, n, x);
  } else {
    pa_set(a->A, n, 0);
  }
  a->n = n+1;
}
#define _insert_tail_ share_insert_tail_

/////////////////////////////////////////
// Extract the range [start, end-1]
// Note that end is exclusive (Python-style)
/////////////////////////////////////////
static share_array share_slice_raw(share_array a, int start, int end)
{
  if (a == NULL) {
    printf("share_slice_raw: a = %p\n", a);
    return NULL;
  }
  if (start < 0) start = a->n + start;
  //if (end <= 0) end = a->n + end;
  if (end < 0) end = a->n + end;
  if (start < 0 || start > a->n) {
    printf("share_slice n %d start %d\n", a->n, start);
  }
  if (end < 0 || end > a->n) {
    printf("share_slice n %d end %d\n", a->n, end);
  }
  NEWT(share_array, ans);
  *ans = *a;
  ans->n = end - start;
  ans->q = a->q;
  ans->type = a->type;
  ans->irr_poly = a->irr_poly;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(ans->n, a->A->w, a->A->type);
  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr = pa_iter_pos_new(a->A, start);
  for (int i=0; i<ans->n; i++) pa_iter_set(itr_ans, pa_iter_get(itr));
  pa_iter_flush(itr_ans);
  pa_iter_free(itr);

  return ans;
}
#define _slice_raw share_slice_raw
#define share_slice share_slice_raw
#define _slice share_slice_raw

static void share_slice_(share_array a, int start, int end)
{
  if (a == NULL) {
    printf("share_slice_: a = %p\n", a);
    return;
  }
  share_array tmp = share_slice(a, start, end);
  pa_free(a->A);  *a = *tmp;  free(tmp);
}
#define _slice_ share_slice_

static share_array share_slice_step_raw_reverse(share_array a, int start, int end, int step)
{
  //printf("share_slice_step_raw_reverse n %d start %d stop %d step %d\n", a->n, start, end, step);
  if (step == 0) {
    printf("share_slice_step_raw: step = 0\n");
    exit(1);
  }
  if (a == NULL) {
    printf("share_slice_step_raw: a = %p\n", a);
    return NULL;
  }
  //if (start < 0) start = a->n + start;
  //if (end <= 0) end = a->n + end;
  if (start < 0 || start > a->n) {
    printf("share_slice_step_raw_reverse n %d start %d\n", a->n, start);
  }
  if (end > a->n) {
    printf("share_slice_step_raw_reverse n %d end %d\n", a->n, end);
  }
  //printf("share_slice_step_raw n %d start %d stop %d step %d\n", a->n, start, end, step);
  step = -step;
  NEWT(share_array, ans);
  *ans = *a;

  int n_items = (start - end + step - 1) / step;
  ans->n = n_items;


  ans->q = a->q;
  ans->type = a->type;
  ans->irr_poly = a->irr_poly;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(ans->n, a->A->w, a->A->type);

  for (int i=0; i<ans->n; i++) {
    pa_set(ans->A,i,pa_get(a->A,start-i*step));
  }

  return ans;
}


static share_array share_slice_step_raw(share_array a, int start, int end, int step)
{
  //printf("share_slice_step_raw n %d start %d stop %d step %d\n", a->n, start, end, step);
  if (step <= 0) {
    return share_slice_step_raw_reverse(a, start, end, step);
  }
  if (a == NULL) {
    printf("share_slice_step_raw: a = %p\n", a);
    return NULL;
  }
  if (start < 0) start = a->n + start;
  if (end <= 0) end = a->n + end;
  if (start < 0 || start > a->n) {
    printf("share_slice_step_raw n %d start %d\n", a->n, start);
  }
  if (end < 0 || end > a->n) {
    printf("share_slice_step_raw n %d end %d\n", a->n, end);
  }
  NEWT(share_array, ans);
  *ans = *a;

  int n_items = (end - start + step - 1) / step;
  ans->n = n_items;


  ans->q = a->q;
  ans->type = a->type;
  ans->irr_poly = a->irr_poly;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(ans->n, a->A->w, a->A->type);

  for (int i=0; i<ans->n; i++) {
    pa_set(ans->A,i,pa_get(a->A,start+i*step));
  }

  return ans;
}
#define share_slice_step share_slice_step_raw
#define _slice_step share_slice_step_raw

// Returns the input share_array with element order reversed.
_ share_reverse(share_array a) {
    int n = len(a);
    _ ans = _dup(a);
    for (int i = 0; i < n; ++i) {
        pa_set(ans->A, i, pa_get(a->A, n-1-i));
    }
    return ans;
}
#define _reverse share_reverse


void share_reverse_(share_array a) {
    share_array ans = share_reverse(a);
    _move_(a, ans);
}
#define reverse_ share_reverse_



_ rshift(_ v, share_t z)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _setshares(ans, 1, n, v, 0);
  _setpublic(ans, 0, z);
  return ans;
}

void rshift_(_ v, share_t z)
{
  if (_party >  2) return;
  _ tmp = rshift(v, z);
  pa_free(v->A);  *v = *tmp;  free(tmp);
}

_ lshift(_ v, share_t z)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _setshares(ans, 0, n-1, v, 1);
  _setpublic(ans, n-1, z);
  return ans;
}

void lshift_(_ v, share_t z)
{
  if (_party >  2) return;
  _ tmp = lshift(v, z);
  pa_free(v->A);  *v = *tmp;  free(tmp);
}

_ rrotate(_ v)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _setshares(ans, 1, n, v, 0);
  _setshare(ans, 0, v, n-1);
  return ans;
}

void rrotate_(_ v)
{
  if (_party >  2) return;
  _ tmp = rrotate(v);
  pa_free(v->A);  *v = *tmp;  free(tmp);
}

_ lrotate(_ v)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _setshares(ans, 0, n-1, v, 1);
  _setshare(ans, n-1, v, 0);
  return ans;
}

void lrotate_(_ v)
{
  if (_party >  2) return;
  _ tmp = lrotate(v);
  pa_free(v->A);  *v = *tmp;  free(tmp);
}

/*
def extend(l, A):
  n = len(A)

  A2 = Share([0] * (n*l), A.order())
  for i in range(0, n):
    for j in range(0, l):
      A2[i*l+j] = A[i]
  return A2
*/
/////////////////////////////////////////////////////////////
// abc => aaabbbccc (copy each element l times)
/////////////////////////////////////////////////////////////
/**
 * @brief Create an expanded share array by repeating each source element and applying an additive shift.
 *
 * Builds a new array of length `len(x) * l` over the same modulus/order as `x`.
 * For each logical source element `x[i]` (read starting from `offset`), it computes:
 *   `(x[i] + diff) mod order(x)`
 * and writes that value `l` consecutive times into the output.
 *
 * If `offset > 0`, iteration wraps once to the beginning of `x` when the read position
 * reaches the end of the source array.
 *
 * @param l       Replication factor for each input element in the output.
 * @param x       Source share array.
 * @param offset  Starting read position in `x`.
 * @param diff    Additive offset applied to each read value before replication.
 *
 * @return A newly allocated share array of size `len(x) * l`, containing the transformed,
 *         repeated values, with modulus/order equal to `order(x)`.
 */
share_array extend_share_array_offset(int l, share_array x, int offset, share_t diff) 
{
  int n = len(x);
  share_t q = order(x);
  share_array ans = share_const(n * l, 0, q);
  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_x = pa_iter_pos_new(x->A, offset);
  for (int i = 0; i < n; ++i) {
    share_t z = pa_iter_get(itr_x);
    z = (z + diff) % q;
    for (int j = 0; j < l; ++j) {
      //share_setshare(ans, l * i + j, x, i);
      pa_iter_set(itr_ans, z);
    }
    if ((offset > 0) && (i + offset + 1 == n)) {
      pa_iter_free(itr_x);
      itr_x = pa_iter_new(x->A);
    }
  }
  pa_iter_flush(itr_ans);
  pa_iter_free(itr_x);
    
  return ans;
}
#define extend_share_array(l, x) extend_share_array_offset(l, x, 0, 0)
#define _stretch(x, l) extend_share_array_offset(l, x, 0, 0)


/*
def extend_cyclic(l, A):
  n = len(A)

  A2 = Share([0] * (n*l), A.order())
  for i in range(0, n):
    for j in range(0, l):
      A2[j*n+i] = A[i]
  return A2
*/      
/////////////////////////////////////////////////////////////
// abc => abcabcabc (repeat l times)
/////////////////////////////////////////////////////////////
/**
 * @brief Extend a cyclic share array by repeating it `l` times, adding a fixed offset `diff`
 *        to each copied element modulo the array order.
 *
 * Creates a new share array of length `len(x) * l` over the same modulus/order as `x`.
 * Each output value is computed as:
 *   `(x[i] + diff) % order(x)`.
 *
 * If `offset > 0`, the source iterator is restarted once per repetition when
 * `i + offset + 1 == len(x)`, effectively introducing a wrap point in the read sequence.
 *
 * @param l       Number of repetitions (blocks) of the source array.
 * @param x       Source share array.
 * @param offset  Optional wrap offset trigger; only active when positive.
 * @param diff    Additive value applied to each source element (mod `order(x)`).
 *
 * @return Newly allocated share array containing the transformed, repeated sequence.
 *
 * @note The caller is responsible for freeing the returned array.
 */
share_array extend_cyclic_share_array_offset(int l, share_array x, int offset, share_t diff) 
{
  int n = len(x);
  share_t q = order(x);
  share_array ans = share_const(n * l, 0, q);
  pa_iter itr_ans = pa_iter_new(ans->A);
  for (int j = 0; j < l; ++j) {
    pa_iter itr_x = pa_iter_new(x->A);
    for (int i = 0; i < n; ++i) {
      share_t z = pa_iter_get(itr_x);
      z = (z + diff) % q;
      pa_iter_set(itr_ans, z);
      if ((offset > 0) && (i + offset + 1 == n)) {
        pa_iter_free(itr_x);
        itr_x = pa_iter_new(x->A);
      }
    }
    pa_iter_free(itr_x);
  }
  pa_iter_flush(itr_ans);
    
  return ans;
}
#define extend_cyclic_share_array(l, x) extend_cyclic_share_array_offset(l, x, 0, 0)
#define extend_cyclic(l, x) extend_cyclic_share_array_offset(l, x, 0, 0)
#define ntimes(x, n) extend_cyclic_share_array(n, x)

/////////////////////////////////////////////////////////////
// Concatenate share_arrays
/////////////////////////////////////////////////////////////
/**
 * @brief Serializes an array of share arrays into a single contiguous share array.
 *
 * This function verifies that all input share arrays have the same field/order,
 * then copies their elements sequentially into a newly allocated result array.
 *
 * @param l The number of share arrays in @p x. Must be greater than 0.
 * @param x Pointer to an array of @p l share_array values to be serialized.
 *
 * @return A newly allocated share_array containing the elements of the input
 *         arrays in order.
 *
 * @note All input arrays must have the same order as @p x[0]. If a mismatch is
 *       detected, the function prints an error message and terminates the
 *       process with @c exit(1).
 *
 * @warning This function assumes @p x is non-NULL and that @p l > 0, since it
 *          accesses @p x[0] unconditionally.
 */
share_array serialize_share_arrays(int l, share_array *x) 
{
    int n = len(x[0]);
    int new_n = n;
    share_t q = order(x[0]);
    for (int i = 1; i < l; ++i) {
        if (order(x[i]) != q) {
            printf("serialize_share_arrays: order(x[%d]) %d != q %d\n", i, (int)order(x[i]), (int)q);
            exit(1);
        }
        //printf("serialize_share_arrays: len(x[%d]) %d\n", i, len(x[i]));
        new_n += len(x[i]);
    }
    share_array ans = share_const(new_n, 0, q);
    int pos = 0;
    for (int i = 0; i < l; ++i) {
        for (int j = 0; j < len(x[i]); ++j) {
            //printf("i=%d j=%d set %d\n", i, j, pos + j);
            share_setshare(ans, pos + j, x[i], j);
        }
        pos += len(x[i]);
    }
    return ans;
}
#define _serialize serialize_share_arrays

/////////////////////////////////////////////////////////////
// Split into l parts
/////////////////////////////////////////////////////////////
/**
 * @brief Deserializes a flat share array into an array of equally sized slices.
 *
 * This function partitions the input `share_array a` into `l` contiguous slices.
 * Let `N = len(a)`. Each output slice has length `N / l`, and the function
 * returns an array of `l` slices where:
 *   - slice `i` covers `[i * (N/l), (i + 1) * (N/l))` in `a`.
 *
 * @param a Input flat share array to split.
 * @param l Number of slices to produce.
 * @return Pointer to a newly allocated array of `l` slices (`_ *`).
 *
 * @note The caller is responsible for freeing the returned array container
 *       (and any associated resources, depending on `_slice` semantics).
 *
 * @warning Terminates the process via `exit(1)` if `len(a)` is not divisible
 *          by `l`.
 */
_ *deserialize_share_array(share_array a, int l) 
{
  int n = len(a) / l;
  if (n * l != len(a)) {
    printf("deserialize_share_array: len(a) %d l %d\n", len(a), l);
    exit(1);
  }
  _ *ans;
  NEWA(ans, _, l);
  for (int i = 0; i < l; ++i) {
    ans[i] = _slice(a, n * i, n * (i + 1));
  }
  return ans;
}
#define _deserialize deserialize_share_array


static share_t debug_get(share_array a, int i)
{
  _ tmp = share_slice(a, i, i+1);
  _ tmp2 = _reconstruct(tmp);
  share_t ans = share_getraw(tmp2, 0);
  _free(tmp); _free(tmp2);
  return ans;
}

/**
 * @brief Interleaves multiple equal-length share arrays in fixed-size blocks.
 *
 * Builds a new share array by taking `block_size` consecutive elements from each
 * input array `x[i]` for each block index, and appending them in the order:
 * block 0 of x[0], block 0 of x[1], ..., block 0 of x[l-1], then block 1, etc.
 *
 * Preconditions:
 * - `x` must not be `NULL`.
 * - All `l` input arrays must have the same length.
 * - The common length `n` must be divisible by `block_size`.
 *
 * The output length is `l * n`, and its order is the maximum order among all
 * input arrays.
 *
 * @param l Number of input share arrays.
 * @param x Array of `l` share arrays to be block-zipped.
 * @param block_size Number of consecutive elements taken from each input at a time.
 * @return A newly allocated share array containing the block-wise interleaving,
 *         or `NULL` if `x == NULL`.
 *
 * @note On length mismatch or non-divisible block sizing, the function prints an
 *       error message and terminates the process via `exit(1)`.
 */
static share_array _zip_block(int l, share_array *x, int block_size)
{
  if (x == NULL) {
    printf("_zip: x = NULL\n");
    return NULL;
  }
  if (x[0] == NULL) { // Avoiding potential null dereference in len(x[0])
    printf("_zip: x[0] = NULL\n");
    return NULL;
  }
  int n = len(x[0]);
  for (int i = 1; i < l; ++i) {
    if (x[i] == NULL) {
      printf("_zip: x[%d] = NULL\n", i);
      return NULL;
    }
    if (len(x[i]) != n) {
      printf("zip: len(x[%d]) %d != n %d\n", i, len(x[i]), n);
      exit(1);
    }
  }
  int m = n / block_size;
  if (m * block_size != n) {
    printf("zip: n %d block_size %d\n", n, block_size);
    exit(1);
  }
  share_t max_q = 0;
  for (int i = 0; i < l; ++i) {
    if (order(x[i]) > max_q) max_q = order(x[i]);
  }
  share_array ans = share_const(l * n, 0, max_q);
  for (int j = 0; j < m; ++j) {
    for (int i = 0; i < l; ++i) {
      //share_setshare(ans, j * l + i, x[i], j);
      for (int k = 0; k < block_size; ++k) {
        share_setshare(ans, block_size * (j * l + i) + k, x[i], block_size * j + k);
        //share_setraw(ans, block_size * l * j + block_size * i + k, share_getraw(x[i], block_size * j + k));
      }
    }
  }
  return ans;
}
#define _zip(l, x) _zip_block(l, x, 1)

static share_array _zip_block2(int l, share_array *x, int *bs_array)
{
  if (x == NULL) {
    printf("_zip: x = NULL\n");
    return NULL;
  }
  if (x[0] == NULL) { // Avoiding potential null dereference in len(x[0])
    printf("_zip: x[0] = NULL\n");
    return NULL;
  }
#if 0
  int n = len(x[0]);
  for (int i = 1; i < l; ++i) {
    if (x[i] == NULL) {
      printf("_zip: x[%d] = NULL\n", i);
      return NULL;
    }
    if (len(x[i]) != n) {
      printf("zip: len(x[%d]) %d != n %d\n", i, len(x[i]), n);
      exit(1);
    }
  }
  int m = n / block_size;
  if (m * block_size != n) {
    printf("zip: n %d block_size %d\n", n, block_size);
    exit(1);
  }
#endif
  int n = len(x[0]);
  int m = n / bs_array[0];
  int block_size = bs_array[0];
  for (int i = 1; i < l; ++i) {
    if (x[i] == NULL) {
      printf("_zip: x[%d] = NULL\n", i);
      return NULL;
    }
    int m2 = len(x[i]) / bs_array[i];
    if (m2 != m) {
      printf("zip: block_size does not match\n");
      exit(1);
    }
    block_size += bs_array[i];
  }

  share_t max_q = 0;
  for (int i = 0; i < l; ++i) {
    if (order(x[i]) > max_q) max_q = order(x[i]);
  }
  share_array ans = share_const(block_size * m, 0, max_q);
  int pos = 0;
  for (int j = 0; j < m; ++j) {
    for (int i = 0; i < l; ++i) {
      for (int k = 0; k < bs_array[i]; ++k) {
        share_setshare(ans, pos, x[i], bs_array[i] * j + k);
        pos++;
      }
    }
  }
  return ans;
}

_ _zipP(_pair P)
{
  share_array x[2] = {P.x, P.y};
  return _zip(2, x);
}

/**
 * @brief De-interleaves a blocked, zipped share array into `l` separate share arrays.
 *
 * Interprets `z` as `l` streams interleaved in contiguous blocks of `block_size`.
 * The input length must satisfy:
 * - `len(z) % l == 0`
 * - `(len(z) / l) % block_size == 0`
 *
 * If either condition fails, the function prints an error and terminates.
 *
 * @param z           Source share array containing blocked-interleaved data.
 * @param l           Number of output slices (streams) to reconstruct.
 * @param block_size  Number of consecutive elements per stream in each interleaving block.
 * @return            Newly allocated array of length `l`; each element is a share array
 *                    of length `len(z) / l`, preserving the source order metadata.
 *
 * @note The caller owns the returned top-level array and the contained share arrays.
 */
static _* _unzip_block(share_array z, int l, int block_size)
{
  int n = len(z) / l;
  if (len(z) != n * l) {
    printf("unzip: len(z) %d l %d\n", len(z), l);
    exit(1);
  }
  int m = n / block_size;
  if (m * block_size != n) {
    printf("unzip: n %d block_size %d\n", n, block_size);
    exit(1);
  }

  _ *x;
  NEWA(x, _, l);
  for (int i = 0; i < l; ++i) {
    x[i] = _const(n, 0, order(z));
  }

  for (int j = 0; j < m; ++j) {
    for (int i = 0; i < l; ++i) {
      //share_setshare(x[i], j, z, j * l + i);
      //share_setraw(x[i], j, share_getraw(z, j * l + i));
      for (int k = 0; k < block_size; ++k) {
        share_setshare(x[i], block_size * j + k, z, block_size * (j * l + i) + k);
        //share_setraw(x[i], block_size * j + k, share_getraw(z, block_size * l * j + block_size * i + k));
      }
    }
  }
  return x;
}
//#define _unzip(z, l, x) _unzip_block(z, l, x, 1)
#define _unzip(z, l) _unzip_block(z, l, 1)

static _* _unzip_block2(share_array z, int l, int *bs_array)
{
  int block_size = 0;
  for (int i = 0; i < l; ++i) {
    if (bs_array[i] <= 0) {
      printf("unzip: block_size must be positive\n");
      exit(1);
    }
    block_size += bs_array[i];
  }
  int m = len(z) / block_size;
  if (len(z) != m * block_size) {
    printf("unzip: len(z) %d total block_size %d\n", len(z), block_size);
    exit(1);
  }

  _ *x;
  NEWA(x, _, l);
  for (int i = 0; i < l; ++i) {
    x[i] = _const(m * bs_array[i], 0, order(z));
  }

  int pos = 0;
  for (int j = 0; j < m; ++j) {
    for (int i = 0; i < l; ++i) {
      for (int k = 0; k < bs_array[i]; ++k) {
        share_setshare(x[i], bs_array[i] * j + k, z, pos);
        pos++;
      }
    }
  }
  return x;
}

_pair _unzipP(_ P)
{
  share_array *x = _unzip(P, 2);
  _pair ans = {x[0], x[1]};
  free(x);
  return ans;
}

#endif // SLICE_H
