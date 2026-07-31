#ifndef SHARE_BITS_H
 #define SHARE_BITS_H

/**
 * @file share_bits.h
 * @brief Provides fundamental operations for bit-decomposed secret shares.
 *
 * This header defines core utilities used to manipulate and process shares
 * represented in bit-decomposed form.
 */
/*
  Basic operations on bit-decomposed secret shares
*/

typedef struct bits {
  int d; // Number of bits
  _* a;
}* _bits;

typedef struct {
  _bits x;
  _ y;
} share_pair_bits;
#define _pair_bits share_pair_bits



// Decompose constant x into bits and create a share array
/**
 * @brief Constructs a bit-decomposed shared representation of a constant value.
 *
 * Creates a `_bits` object of length `d`, where each element `a[j]` is a share
 * array (size `n`) initialized to the `j`-th bit of `x` (LSB-first), modulo/share
 * context `q`.
 *
 * Behavior notes:
 * - Bits are extracted as `(x >> j) & 1` for `j = 0..d-1`.
 * - Each bit-plane is initialized via `share_const(n, 0, q)` and then filled.
 * - Population of public values is performed only when `_party <= 1`.
 *
 * @param d Number of bits to export from `x`.
 * @param n Number of share entries per bit-plane.
 * @param q Share/modulus context passed to `share_const`.
 * @param x Constant input value to decompose into bits.
 * @return Pointer to a newly allocated `_bits` structure containing `d` shared bit-planes.
 *
 * @note The caller is responsible for releasing the returned structure according to
 *       the project’s memory management conventions.
 */
_bits share_bits_const(int d, int n, share_t q, share_t x) {
    share_t *x_bits;
    NEWA(x_bits, share_t, d);
    for (int i = 0; i < d; ++i) {
        x_bits[i] = (x>>i) & 1;
    }

    NEWT(_bits, b);
    b->d = d;
    NEWA(b->a, share_array, d);
    for (int j = 0; j < d; ++j) {
        b->a[j] = share_const(n, 0, q);
        //pa_iter_new(b->a[j]->A);
    }

    if (_party <= 1) {
      for (int j = 0; j < d; ++j) {
        pa_iter itr = pa_iter_new(b->a[j]->A);
        for (int i = 0; i < n; ++i) {
          //pa_iter_set(b->a[j]->A, x_bits[j]);
          pa_iter_set(itr, x_bits[j]);
        }
        //pa_iter_flush(b->a[j]->A);
        pa_iter_flush(itr);
      }
    }
    free(x_bits);

    return b;
}

// Decompose plaintext array A into bits and create a share
/**
 * @brief Creates a bit-decomposed shared representation from an input share array.
 *
 * This function allocates and initializes a `_bits` object containing `d` bit-slices
 * of the input values `A[0..n-1]`. For each bit position `j` in `[0, d)`, it builds
 * an array where each element is `(A[i] >> j) & 1`, then calls `share_new(n, q, ...)`
 * to create the corresponding share array entry.
 *
 * Behavior depends on the global `_party` value:
 * - `_party <= 0`:
 *   - Allocates a temporary buffer `A_bits` of length `n`.
 *   - Properly fills `A_bits` for each bit position and initializes `ans->a[j]`.
 * - `1 <= _party && _party <= 2`:
 *   - Prints a warning message.
 *   - Calls `share_new(n, q, A_bits)` without initializing `A_bits`
 *     (undefined behavior / likely bug).
 *
 * @param d Number of bit positions (bit-slices) to generate.
 * @param n Number of elements in the input share array.
 * @param q Modulus/field parameter passed to `share_new`.
 * @param A Input share values to be bit-decomposed.
 * @return Newly allocated `_bits` object with `d` share arrays.
 *
 * @note The returned object and nested allocations must be released by the corresponding
 * deallocation routine.
 * @warning For `_party` in `{1,2}`, `A_bits` is used uninitialized in the current code.
 */
_bits share_bits_new(int d, int n, share_t q, share_t *A) {
    share_t *A_bits;

    NEWT(_bits, ans);
    ans->d = d;
    NEWA(ans->a, share_array, n);
    if (_party <= 0) {
        NEWA(A_bits, share_t, n);

        for (int j = 0; j < d; ++j) {
            for (int i = 0; i < n; ++i) {
                A_bits[i] = (A[i]>>j) & 1;
            }
            ans->a[j] = share_new(n, q, A_bits);
        }

        free(A_bits);
    } else if (1 <= _party && _party <= 2) {
        //printf("share_bits_new: A_bits is not initialized for party %d\n", _party);
        A_bits = NULL; // A_bits is not used
        for (int j = 0; j < d; ++j) {
            ans->a[j] = share_new(n, q, A_bits);
        }
    }

    return ans;
}


/**
 * @brief Releases all memory owned by a `_bits` object.
 *
 * Frees each element in `a->a` (for indices `0 .. a->d-1`) using `_free`,
 * then frees the container array `a->a`, and finally frees the `_bits`
 * structure itself.
 *
 * @param a Pointer to the `_bits` instance to deallocate.
 *
 * @note Assumes `a` is valid and that `a->a` contains at least `a->d` entries.
 * @warning Calling this function with invalid or already-freed pointers results
 * in undefined behavior.
 */
void _free_bits(_bits a)
{
//  if (_party >  2) return;
  for (int i=0; i<a->d; i++){
    _free(a->a[i]);
  }
  free(a->a);
  free(a);
}

////////////////////////////////////////////
// a[i] := b[j]
////////////////////////////////////////////
static void share_setshare_bits(_bits a, int i, _bits b, int j)
{
  if (_party >  2) return;
  if (i < 0 || i >= a->a[0]->n) {
    printf("share_setshare_bits a: n %d i %d\n", a->a[0]->n, i);
    exit(1);
  }
  if (j < 0 || j >= b->a[0]->n) {
    printf("share_setshare_bits b: n %d j %d\n", b->a[0]->n, j);
    exit(1);
  }
  if (a->a[0]->q != b->a[0]->q) {
    printf("share_setshare_bits a->q %d b->q %d\n", (int)a->a[0]->q, (int)b->a[0]->q);
    exit(1);
  }
  if (a->d != b->d) {
    printf("share_setshare_bits a->d %d b->d %d\n", (int)a->d, (int)b->d);
    exit(1);
  }
  for (int k=0; k<a->d; k++) {
    pa_set(a->a[k]->A,i, pa_get(b->a[k]->A,j));
  }
}
#define _setshare_bits share_setshare_bits

void share_setshares_bits(_bits x, int si, int ei, _bits y, int j) {
    if (x->d != y->d) {
        printf("share_setshres_bits x->d: %d    y->d: %d\n", x->d, y->d);
        exit(1);
    }
    if (order(x->a[0]) != order(y->a[0])) {
        printf("share_setshraes_bits: order(x->a[0]): %d    order(y->a[0]): %d\n", order(x->a[0]), order(y->a[0]));
        exit(1);
    }
    if (si > ei || si < 0 || ei > len(x->a[0])) {
        printf("share_setshares_bits si: %d ei: %d\n", si, ei);
        exit(1);
    }
    if (j + ei - si > len(y->a[0])) {
        printf("share_setshre_bits  si: %d  ei: %d  j: %d   len(y->a[0]): %d\n", si, ei, j, len(y->a[0]));
        exit(1);
    }

    for (int i = 0; i < ei - si; ++i) {
        share_setshare_bits(x, si + i, y, j + i);
    }
}

/**
 * @brief Reconstructs a bitwise shared value over a specified communication channel.
 *
 * This function reconstructs secret-shared bit data for 2-party execution contexts
 * (`_party` in {0,1,2}) by combining local shares with data exchanged through the
 * given channel.
 *
 * Behavior:
 * - Returns `NULL` if the current party index is greater than 2.
 * - Returns `NULL` if `x` is `NULL`.
 * - Allocates a zero-initialized result with the same bit-width (`d`), vector length (`n`),
 *   and share order (`q`) as `x`.
 * - If `_party == 0`, it copies shares directly from `x` into the result.
 * - Otherwise, for each bit plane, it receives peer share data via
 *   `mpc_exchange_channel(...)` and adds local shares with `vadd_(...)` to reconstruct.
 *
 * @param x        Input bit-share structure to reconstruct.
 * @param channel  Communication channel identifier used for MPC exchange.
 * @return Reconstructed bit-share object on success; `NULL` on invalid party index or `NULL` input.
 */
_bits share_reconstruct_bits_channel(_bits x, int channel) {
    if (_party > 2) {
        return NULL;
    }
    if (x == NULL) {
        return NULL;
    }

    int n = len(x->a[0]);
    int d = x->d;
    share_t q = order(x->a[0]);
    _bits ans = share_bits_const(d, n, q, 0);
    if (_party == 0) {
        share_setshares_bits(ans, 0, n, x, 0);
    }
    else {
        for (int k = 0; k < d; ++k) {   // TODO: Parallelize this
            mpc_exchange_channel(x->a[k]->A->B, ans->a[k]->A->B, pa_size(x->a[k]->A), channel);
            vadd_(ans->a[k], x->a[k]);
        }
    }

    return ans;
}



/////////////////////////////////////////////////
// For bit-decomposed v, perform v[i] := x
/////////////////////////////////////////////////
void _setpublic_bits(_bits v, int i, share_t x)
{
  if (_party >  2) return;
  for (int d=0; d<v->d; d++) {
    _setpublic(v->a[d], i, (x>>d) & 1);
  }
}

int len_bits(_bits b)
{
  return len(b->a[0]);
}

int order_bits(_bits b)
{
  return order(b->a[0]);
}

int depth_bits(_bits b)
{
  return b->d;
}

static _bits share_const_bits(int n, share_t v, share_t q, int d)
{
  if (_party >  2) return NULL;
  NEWT(_bits, ans);
  ans->d = d;
  NEWA(ans->a, _, d); 
  for (int i=0; i<d; i++) {
    ans->a[i] = _const(n, 0, q);
  }

  for (int i=0; i<n; i++) {
    _setpublic_bits(ans, i, v);
  }
  return ans;
}
#define _const_bits share_const_bits

static _bits share_const_bits_3party(int n, share_t v, share_t q, int d)
{
  if (_party >  3) return NULL;
  NEWT(_bits, ans);
  ans->d = d;
  NEWA(ans->a, _, d); 
  for (int i=0; i<d; i++) {
    ans->a[i] = share_const_type(n, 0, q, SHARE_T_SHAMIR);
  }

  for (int i=0; i<n; i++) {
    _setpublic_bits(ans, i, v);
  }
  return ans;
}
#define _const_bits_3party share_const_bits_3party


static _bits share_slice_bits(_bits a, int start, int end)
{
  if (_party >  2) return NULL;
  if (start < 0) start = a->a[0]->n + start;
  if (end <= 0) end = a->a[0]->n + end;
  if (start < 0 || start > a->a[0]->n) {
    printf("share_slice_bits n %d start %d\n", a->a[0]->n, start);
  }
  if (end < 0 || end > a->a[0]->n) {
    printf("share_slice_bits n %d end %d\n", a->a[0]->n, end);
  }
  _bits ans = _const_bits(end-start, 0, a->a[0]->q, a->d);
  for (int i=0; i<ans->a[0]->n; i++) _setshare_bits(ans, i, a, start+i);
  return ans;
}
#define _slice_bits share_slice_bits

static share_t debug_get_bits(_bits a, int i)
{
  _bits tmp = share_slice_bits(a, i, i+1);
  //_bits tmp2 = share_reconstruct_bits_channel(tmp, 0);
  share_t ans = 0;
  for (int d = tmp->d-1; d >= 0; d--) {
    ans <<= 1;
    ans += debug_get(tmp->a[d], 0);
  }
  _free_bits(tmp);
  return ans;
}

_ B2A_channel(_ a, share_t q, int channel); // in func.h
#ifndef B2A
 #define B2A(b, q) B2A_channel(b, q, 0)
#endif





//////////////////////////////////////////////////
// Convert per-bit shares into an additive share.
// Assumes each digit-share has the same modulus as the original value,
// so carry handling can be performed directly as-is.
//////////////////////////////////////////////////
// bit-wise share to additive share conversion
// Each digit share modulus is assumed to equal the original modulus
// (i.e., carries can be handled as-is)
///////////////////////////////////////////////////

/**
 * @brief Converts a bit-vector representation into an arithmetic value.
 *
 * This function reconstructs an arithmetic share/value from the bitwise
 * representation stored in @p b by evaluating the bits from most significant
 * to least significant using repeated doubling and addition.
 *
 * The conversion is only supported when the global party count is at most 2.
 * If `_party > 2`, the function returns `NULL`.
 *
 * If the modulus of the source bits is 2, a warning is printed because that
 * modulus is not suitable for this conversion path.
 *
 * @param b Bitwise representation to convert. Each element `b->a[k]` is treated
 *          as the k-th bit, and `b->d` is the number of bits.
 * @return Converted arithmetic value on success, or `NULL` when the current
 *         party configuration is unsupported.
 */
_ _B2A_bits(_bits b)
{
  if (_party >  2) return NULL;
  // Original modulus cannot be 2
  if (b->a[0]->q == 2) {
    printf("B2A_bits: warning q = %d\n", b->a[0]->q);
  }
  _ ans = _const(b->a[0]->n, 0, b->a[0]->q);
  for (int k=b->d-1; k>=0; k--) {
    smul_(2, ans);
    vadd_(ans, b->a[k]);
  }
  return ans;
}

/**
 * @brief Converts a bit-decomposed value into a packed field element representation over GF(2^d).
 *
 * This function reconstructs each element from its bit shares in @p x by combining the bits
 * as coefficients of a polynomial basis representation in GF(2^d). Each bit at position `j`
 * contributes `bit * x^j`, implemented via finite-field multiplication with the basis value
 * `(1 << j)` modulo the supplied irreducible polynomial.
 *
 * The output share has the same length and share type as the input bit shares, and its
 * irreducible polynomial is set to @p irr_poly.
 *
 * @param x Bit-decomposed input, where `x->a[j]` holds the share for bit position `j`.
 * @param irr_poly Irreducible polynomial used for multiplication in GF(2^d).
 * @return A newly allocated share containing the reconstructed GF(2^d) elements.
 */
static _ B2A_GF(_bits x, share_t irr_poly)
{
  share_t q = 1 << x->d;
  int n = len(x->a[0]);
  _ ans = share_const_type(n, 0, q, x->a[0]->type);
  ans->type = x->a[0]->type;
  ans->irr_poly = irr_poly;
  for (int i=0; i<ans->n; i++) {
    share_t z;
    z = 0;
    for (int j=x->d-1; j>=0; j--) {
      share_t t = share_getraw(x->a[j], i);
      z ^= GF_mul(t, (1<<j), irr_poly);
    }
    share_setraw(ans, i, z);
  }
  return ans;
}

void _print_bits(_bits a)
{
  for (int i=0; i<a->d; i++) {
    printf("i = %d ", i); _print(a->a[i]);
  }
}

char *share_print_bits_str(_bits a)
{
  char **bufs;
  NEWA(bufs, char*, a->d);
  int total = 0;
  for (int i=0; i<a->d; i++) {
    bufs[i] = share_print_str(a->a[i]);
    total += strlen(bufs[i]);
  }
  char *buf;
  NEWA(buf, char, total + 20*a->d);
  total = 0;
  int s;
  for (int i=0; i<a->d; i++) {
    s = sprintf(buf+total, "i = %d %s\n", i, bufs[i]);
    total += s;
  }
  buf[total] = 0;
  buf = realloc(buf, total+1);
  for (int i=0; i<a->d; i++) {
    free(bufs[i]);
  }
  free(bufs);
  return buf;
}


_bits _dup_bits(_bits a)
{
  if (_party >  2) return NULL;
  NEWT(_bits, ans);
  ans->d = a->d;
  NEWA(ans->a, _, a->d); 
  for (int i=0; i<a->d; i++) {
    ans->a[i] = _dup(a->a[i]);
  }
  return ans;
}

/**
 * @brief Concatenates two `_bits` sequences into a newly allocated `_bits` object.
 *
 * Creates a new `_bits` value whose elements consist of all entries from
 * @p low followed by all entries from @p high. Each element is duplicated
 * into the result rather than referenced directly.
 *
 * @param low  The `_bits` sequence placed at the beginning of the result.
 * @param high The `_bits` sequence appended after @p low.
 * @return A newly allocated `_bits` containing the concatenated contents of
 *         @p low and @p high, or `NULL` if `_party > 2`.
 *
 * @note The caller is responsible for releasing the returned `_bits` object
 *       and its associated storage according to the project's memory
 *       management conventions.
 */
_bits _vconcat_bits(_bits low, _bits high)
{
  if (_party >  2) return NULL;
  NEWT(_bits, ans);
  ans->d = low->d + high->d;
  NEWA(ans->a, _, ans->d); 
  for (int i=0; i<low->d; i++) {
    ans->a[i] = _dup(low->a[i]);
  }
  for (int i=0; i<high->d; i++) {
    ans->a[low->d + i] = _dup(high->a[i]);
  }
  return ans;
}

/**
 * @brief Concatenates two `_bits` structures level-by-level.
 *
 * Creates a new `_bits` object whose depth matches the inputs and whose
 * element arrays are formed by concatenating corresponding entries from
 * `first` and `second` via `_concat`.
 *
 * @param first  The first `_bits` operand.
 * @param second The second `_bits` operand.
 * @return Newly allocated `_bits` containing concatenated data, or `NULL`
 *         when `_party > 2`.
 *
 * @note Both inputs must have the same depth (`d`). If depths differ, the
 *       function prints an error message and terminates the process.
 * @warning The returned object is heap-allocated; the caller is responsible
 *          for releasing it according to the project's memory-management rules.
 */
_bits _concat_bits(_bits first, _bits second)
{
  if (_party >  2) return NULL;
  int d = first->d;
  if (d != second->d) {
    printf("concat_bits: depth %d %d\n", d, second->d);
    exit(1);
  }
  NEWT(_bits, ans);
  ans->d = d;
  NEWA(ans->a, _, d); 
  for (int i=0; i<d; i++) {
    ans->a[i] = _concat(first->a[i], second->a[i]);
  }
  return ans;
}

static void _print_debug_bits(_bits a)
{
  for (int i=0; i<a->d; i++) {
    _ tmp = _reconstruct(a->a[i]);
    printf("debug "); _print(tmp);
    _free(tmp);
  }
}

// This function is copied from aes.c, so be careful about duplicate definitions.
static void share_xor_bits(_bits x, int i, _bits y, int j)
{
  for (int d=0; d<x->d; d++) {
    share_t xtmp, ytmp;
    xtmp = share_getraw(x->a[d], i);
    ytmp = share_getraw(y->a[d], j);
    xtmp ^= ytmp;
    share_setraw(x->a[d], i, xtmp);
  }
}

static void share_xor_bits2(_bits x, int i, _bits y, int j) {
    for (int d = 0; d < min(x->d, y->d); ++d) {
        share_t xtmp, ytmp;
        xtmp = share_getraw(x->a[d], i);
        ytmp = share_getraw(y->a[d], j);
        xtmp ^= ytmp;
        share_setraw(x->a[d], i, xtmp);
    }
}

void bits_free(_bits x) {
    for (int i = 0; i < x->d; ++i) {
        share_free(x->a[i]);
    }
    free(x->a);
    free(x);
}

void _move_bits(_bits x, _bits y) {
    if (x == NULL)  return;
    if (y == NULL)  return;
    for (int i = 0; i < x->d; ++i) {
        share_free(x->a[i]);
    }
    free(x->a);
    *x = *y;
    free(y);
}

_bits vsub_bits(_bits x, _bits y) {
    if (len(x->a[0]) != len(y->a[0])) {
        printf("vsub_bits: len(x->a[0]) %d  len(y->a[0]) %d\n", len(x->a[0]), len(y->a[0]));
        exit(1);
    }
    if (x->d != y->d) {
        printf("vsub_bits: x->d %d  y->d %d\n", x->d, y->d);
        exit(1);
    }
    if (order(x->a[0]) != 2) {
        printf("vsub_bits order(x->a[0]): %d\n", order(x->a[0]));
        exit(1);
    }
    if (order(y->a[0]) != 2) {
        printf("vsub_xor_bits order(y->a[0]): %d\n", order(y->a[0]));
        exit(1);
    }
    
    int n = len(x->a[0]);
    int d = x->d;
    _bits z = share_const_bits(n, 0, 2, d);
    share_setshares_bits(z, 0, n, x, 0);
    for (int i = 0; i < n; ++i) {
        share_xor_bits(z, i, y, i);
    }

    return z;
}
#define vadd_bits vsub_bits
#define xor_bits vsub_bits

void vsub_bits_(_bits x, _bits y) {
    _bits tmp = vsub_bits(x, y);
    _move_bits(x, tmp);
}

_bits total_xor_bits(_bits x) {
    // _print_bits(x);
    if (order(x->a[0]) != 2) {
        printf("total_xor_bits order(x->a[0]): %d\n", order(x->a[0]));
        exit(1);
    }
    int n = len(x->a[0]);
    int d = x->d;

    _bits y = share_const_bits(1, 0, 2, d);
    for (int i = 0; i < n; ++i) {
        share_xor_bits(y, 0, x, i);
    }

    return y;
}

// TODO: Parallelization?
_bits random_bits(int n, int d, share_t q) {
    if (_party > 2) {
        return NULL;
    }

    NEWT(_bits, ans);
    ans->d = d;
    NEWA(ans->a, share_array, d);
    share_t *A;
    NEWA(A, share_t, d);
    
    for (int i = 0; i < d; ++i) {
        if (_party <= 0) {
            for (int j = 0; j < n; ++j) {
                A[j] = RANDOM0(q);
            }
        }
        ans->a[i] = share_new(n, q, A);
    }

    free(A);

    return ans;
}

share_array serialize_bits(_bits x) {
    int d = x->d;
    int n = len(x->a[0]);
    share_t q = order(x->a[0]);
    share_array ans = share_const(d * n, 0, q);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < d; ++j) {
            share_setshare(ans, i * d + j, x->a[j], i);
        }
    }
    return ans;
}

_bits IfThenElse_bits_channel(share_array e, _bits x, _bits y, int channel) {
    int n = len(x->a[0]);
    int q = order(x->a[0]);

    share_array serialized_x = serialize_bits(x);
    share_array serialized_y = serialize_bits(y);
    share_array extended_e = extend_share_array(x->d, e);

    share_array serialized_ans = IfThenElse_channel(extended_e, serialized_x, serialized_y, channel);

    _bits ans = share_bits_const(x->d, n, q, 0);
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < x->d; ++k) {
            share_setshare(ans->a[k], i, serialized_ans, x->d * i + k);
        }
    }

    share_free(serialized_x);
    share_free(serialized_y);
    share_free(extended_e);
    share_free(serialized_ans);

    return ans;
}

_ IfThen_b(_ c, _ x) {
    if (c->q != 2) {
        printf("IfThen_b: c->q = %d\n", c->q);
        exit(1);
    }
    _ c_ = B2A(c, x->q);
    _ ans = vmul(x, c_);
    _free(c_);

    return ans;
}


_ IfThenElse2(_ c, _ x, _ y) {
    if (c->q != 2) {
        printf("IfThenElse2: c->q = %d\n", c->q);
        exit(1);
    }
    else if (x->q != y->q) {
        printf("IfThenElse2: x->q = %d y->q = %d\n", x->q, y->q);
        exit(1);
    }
    else if (x->n != y->n) {
        printf("IfThenElse2: x->n = %d y->n = %d\n", x->n, y->n);
        exit(1);
    }
    _ c_ = B2A(c, x->q);
    _ dif = vsub(x, y);
    vmul_(dif, c_);
    _ ans = _dup(y);
    vadd_(ans, dif);
    _free(c_);   _free(dif);

    return ans;
}

_pair IfThenElseP(_ c, _pair x, _pair y)
{
  _ X[2] = {x.x, x.y};
  _ ZX = _zip(2, X);
  _ Y[2] = {y.x, y.y};
  _ ZY = _zip(2, Y);
  _ c2 = _stretch(c, 2);
  _ z = IfThenElse2(c2, ZX, ZY);
  _ *Z = _unzip(z, 2);
  _pair ans = {Z[0], Z[1]};
  free(Z);
  _free(c2); _free(ZX); _free(ZY); _free(z);
  return ans;
}

_bits IfThenElse2_bits(share_array e, _bits x, _bits y) {
    int n = len(x->a[0]);
    int q = order(x->a[0]);

    share_array serialized_x = serialize_bits(x);
    share_array serialized_y = serialize_bits(y);
    share_array extended_e = extend_share_array(x->d, e);

    share_array serialized_ans = IfThenElse2(extended_e, serialized_x, serialized_y);

    _bits ans = share_bits_const(x->d, n, q, 0);
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < x->d; ++k) {
            share_setshare(ans->a[k], i, serialized_ans, x->d * i + k);
        }
    }

    share_free(serialized_x);
    share_free(serialized_y);
    share_free(extended_e);
    share_free(serialized_ans);

    return ans;
}

_bits IfThen_b_bits(share_array e, _bits x) {
    int n = len(x->a[0]);
    int q = order(x->a[0]);

    share_array serialized_x = serialize_bits(x);
    share_array extended_e = extend_share_array(x->d, e);

    share_array serialized_ans = IfThen_b(extended_e, serialized_x);

    _bits ans = share_bits_const(x->d, n, q, 0);
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < x->d; ++k) {
            share_setshare(ans->a[k], i, serialized_ans, x->d * i + k);
        }
    }

    share_free(serialized_x);
    share_free(extended_e);
    share_free(serialized_ans);

    return ans;
}


///////////////////////////////////////////////////////////////////////
// Create n/l values using l consecutive elements of x as each digit
////////////////////////////////////////////////////////////////////////
/**
 * @brief Extends a share array into a _bits structure by distributing elements across l layers.
 *
 * Takes a share_array of length n and reshapes it into a _bits structure with l layers,
 * each containing m = n/l elements. Each element from the input array is replicated
 * across all l layers at the corresponding position.
 *
 * @param l   The number of layers to distribute the share array across.
 *            Must evenly divide the length of x.
 * @param x   The input share_array to be extended. Its length must be divisible by l.
 *
 * @return    A _bits structure of order q (same as input) with l layers,
 *            each containing m = n/l elements from the input array.
 *
 * @note      Exits with an error message if the length of x is not divisible by l.
 *
 * @warning   The caller is responsible for freeing the returned _bits structure.
 */
_bits vextend_share_array(int l, share_array x) 
{
  int n = len(x);
  int m = n/l;
  if (m*l != n) {
    printf("vextend_share_array: n = %d l = %d\n", n, l);
    exit(1);
  }
  share_t q = order(x);
  _bits ans = share_const_bits(m, 0, q, l);
  pa_iter *itr_ans;
  NEWA(itr_ans, pa_iter, l);
  for (int i=0; i<l; i++) {
    itr_ans[i] = pa_iter_new(ans->a[i]->A);
  }
  pa_iter itr_x = pa_iter_new(x->A);
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < l; j++) {
      pa_iter_set(itr_ans[j], pa_iter_get(itr_x));
    }
  }
  for (int i=0; i<l; i++) {
    pa_iter_flush(itr_ans[i]);
  }
  free(itr_ans);
  pa_iter_free(itr_x);
    
  return ans;
}

_bits extended_total_xor_bits(int l, _bits x) {
    int n = len(x->a[0]);
    int d = x->d;
    int q = order(x->a[0]);
    if (n % l != 0) {
        printf("extended_total_xor_bits n = %d  l = %d  n %% l = %d\n", n, l, n % l);
        exit(1);
    }
    _bits ans = share_bits_const(d, l, q, 0);
    for (int i = 0; i < n / l; ++i) {
        for (int j = 0; j < l; ++j) {
            share_xor_bits(ans, j, x, i * l + j);
        }
    }

    return ans;
}


////////////////////////////////////////////
// Convert shares to bit-decomposed form with depth 1
// Original variable cannot be used after this
/////////////////////////////////////////////
/**
 * @brief Converts shares into bit-decomposed form at depth 1.
 *
 * Transforms the given shares into their bit-decomposed representation
 * at a depth of 1. Once this conversion is performed, the original
 * variables can no longer be used.
 *
 * @note The original variables are invalidated after this operation.
 *       Do not attempt to access them after calling this function.
 */
_bits share_to_bits(_ a)
{
  NEWT(_bits, ans);
  NEWA(ans->a, _, 1);
  ans->d = 1;
  ans->a[0] = a;

  return ans;
}

/**
 * Converts a `_bits` value into its share representation and releases the original bit buffer.
 *
 * This function creates a share-compatible value from the given `_bits` input by calling
 * `_B2A_bits`, then frees the original `_bits` object with `_free_bits`.
 *
 * @param b The `_bits` value to convert. Ownership is consumed by this function.
 * @return The converted share value produced from `b`.
 */
_ bits_to_share(_bits b)
{
  _ ans = _B2A_bits(b);
  _free_bits(b);
  return ans;
}

////////////////////////////////////////////
// Convert block-wise shares to bit-decomposed form with depth block_size
// Original variable remains
/////////////////////////////////////////////
/**
 * @brief Converts a block share to a bits representation.
 *
 * Splits the input share array `a` into `block_size` separate share arrays,
 * where each array contains every `block_size`-th element starting at a
 * different offset.
 *
 * @param a          Input share array to be split into blocks.
 * @param block_size Number of blocks to split the input share into.
 *                   Determines both the number of output arrays and the
 *                   stride used when partitioning elements.
 * @return           A `_bits` structure of depth `block_size`, where each
 *                   element `ans->a[j]` holds the shares corresponding to
 *                   positions `j, j + block_size, j + 2*block_size, ...`
 *                   in the original array `a`.
 *
 * @note The length of `a` must be divisible by `block_size`.
 *       The resulting arrays are allocated with size `len(a) / block_size`.
 */
_bits block_share_to_bits(_ a, int block_size)
{
  NEWT(_bits, ans);
  ans->d = block_size;
  NEWA(ans->a, _, block_size);  
  int m = len(a) / block_size;
  for (int i = 0; i < block_size; i++) {
    ans->a[i] = _const(m, 0, a->q);
  }
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < block_size; j++) {
      _setshare(ans->a[j], i, a, block_size * i + j);
    }
  }

  return ans;
}

/**
 * @brief Converts a bit-sliced share representation into a block-wise share vector.
 *
 * This function reconstructs a flat share array from the bit-share structure `b`.
 * Each output block consists of `b->d` shares, where the j-th share of the i-th
 * block is taken from `b->a[j]` at position `i`.
 *
 * The resulting share array has length `len(b->a[0]) * b->d` and is initialized
 * over the same field/modulus `q` as the input shares.
 *
 * @param b Input bit-share structure containing `d` share arrays of equal length.
 * @return A newly allocated share vector in block layout.
 */
_ bits_to_block_share(_bits b)
{
  int m = len(b->a[0]);
  int block_size = b->d;
  _ ans = _const(m * block_size, 0, b->a[0]->q);
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < block_size; j++) {
      _setshare(ans, block_size * i + j, b->a[j], i);
    }
  }
  return ans;
}

/**
 * @brief Flattens a multi-component bit sequence into a single packed bit sequence.
 *
 * Creates a new bit container whose length is `d * n`, where `d` is the number
 * of components in @p b and `n` is the length of each component. Bits are copied
 * in position-major order: for each index `i` in `[0, n)`, the bit from each
 * component `j` in `[0, d)` is appended to the result.
 *
 * The resulting container preserves the bit order returned by `order_bits(b)`.
 *
 * @param b Input bit container to flatten. Ownership is consumed by this
 *          function.
 * @return A newly allocated flattened bit container.
 *
 * @note This function frees @p b before returning.
 */
_ bits_shrink(_bits b)
{
  int d = b->d;
  int n = len_bits(b);
  _ ans = _const(d * n, 0, order_bits(b));
  pa_iter *itr_b;
  NEWA(itr_b, pa_iter, d);
  for (int i=0; i<d; i++) {
    itr_b[i] = pa_iter_new(b->a[i]->A);
  }
  pa_iter itr_ans = pa_iter_new(ans->A);
  for (int i=0; i<n; i++) {
    for (int j=0; j<d; j++) {
      pa_iter_set(itr_ans, pa_iter_get(itr_b[j]));
    }
  }
  pa_iter_flush(itr_ans);
  for (int i=0; i<d; i++) {
    pa_iter_free(itr_b[i]);
  }
  free(itr_b);
  _free_bits(b);

  return ans;
}

#endif // SHARE_BITS_H
