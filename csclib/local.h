#ifndef LOCAL_H
 #define LOCAL_H

/////////////////////////////////////////////
// Functions that can be computed locally
/////////////////////////////////////////////


/**
 * @brief Computes the inclusive prefix sum of a secret/shared vector.
 *
 * This function returns a new vector `ans` where:
 * - `ans[i] = v[0] + v[1] + ... + v[i]` for `0 <= i < len(v)`.
 *
 * The implementation initializes an accumulator to public zero, then
 * iteratively adds each element of `v` and stores the running total.
 *
 * @param v Input vector of shares.
 * @return A newly allocated vector containing inclusive prefix sums.
 *         Returns `NULL` when `_party > 2`.
 *
 * @note The caller is responsible for freeing the returned vector.
 */
_ PrefixSum(_ v)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _ sum = _slice(v, 0, 1);
  _setpublic(sum, 0, 0);
  for (int i=0; i<n; i++) {
    _addshare(sum, 0, v, i);
    _setshare(ans, i, sum, 0);
  }
  _free(sum);
  return ans;
}

_ PrefixSumBlock(_ v, int block_size)
{
  if (_party >  2) return NULL;
  if (len(v) % block_size != 0) {
      printf("PrefixSumBlock: len(v) %d block_size %d\n", len(v), block_size);
      return NULL;
  }
  int n = len(v) / block_size;

  _ ans = _const(len(v), 0, order(v));
  _ sum = _const(block_size, 0, order(v));
  //_ tmp = _const(block_size, 0, order(v));
  for (int i=0; i<n; i++) {
    //_setshares(tmp, 0, block_size, v, i*block_size);
    //vadd_(sum, tmp);
    //_setshares(ans, i*block_size, (i+1)*block_size, sum, 0);
    _ tmp = _slice(v, i*block_size, (i+1)*block_size);
    vadd_(sum, tmp);
    _free(tmp);
    _setshares(ans, i*block_size, (i+1)*block_size, sum, 0);
  }
  _free(sum);
  //_free(tmp);
  return ans;
}

/**
 * @brief Computes the suffix sum of a shared vector.
 *
 * For each index `i`, the returned vector contains the sum of elements
 * `v[i] + v[i+1] + ... + v[n-1]`, preserving the share representation.
 *
 * @param v Input shared vector.
 * @return A newly allocated shared vector of the same length containing
 *         suffix sums, or `NULL` when `_party > 2`.
 *
 * @note This function currently supports only parties `0..2`.
 * @note The caller is responsible for freeing the returned vector.
 */
_ SuffixSum(_ v)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _ sum = _slice(v, 0, 1);
  _setpublic(sum, 0, 0);
  for (int i=n-1; i>=0; i--) {
    _addshare(sum, 0, v, i);
    _setshare(ans, i, sum, 0);
  }
  _free(sum);
  return ans;
}

/**
 * @brief Computes block-wise suffix cumulative sums over a secret-shared vector.
 *
 * This function partitions @p v into contiguous blocks of size @p block_size and
 * computes a suffix sum across blocks (from the last block toward the first).
 * For each block index i, the output block i is:
 *
 *   ans[i] = v[i] + v[i+1] + ... + v[n-1]
 *
 * where addition is element-wise within each block and performed using shared
 * arithmetic primitives.
 *
 * @param v           Input vector of secret-shared values. Its length must be a
 *                    multiple of @p block_size.
 * @param block_size  Number of elements per block.
 *
 * @return A newly allocated vector with the same length/order as @p v containing
 *         block-wise suffix sums; returns NULL if:
 *         - the current party id is greater than 2, or
 *         - len(v) is not divisible by @p block_size.
 *
 * @note On divisibility failure, an error message is printed before returning NULL.
 * @note Caller is responsible for freeing the returned vector.
 */
_ SuffixSumBlock(_ v, int block_size)
{
  if (_party >  2) return NULL;
  if (len(v) % block_size != 0) {
      printf("SuffixSumBlock: len(v) %d block_size %d\n", len(v), block_size);
      return NULL;
  }
  int n = len(v) / block_size;

  _ ans = _const(len(v), 0, order(v));
  _ sum = _const(block_size, 0, order(v));
  _ tmp = _const(block_size, 0, order(v));
  for (int i=n-1; i>=0; i--) {
    _setshares(tmp, 0, block_size, v, i*block_size);
    vadd_(sum, tmp);
    _setshares(ans, i*block_size, (i+1)*block_size, sum, 0);
  }
  _free(sum);
  _free(tmp);
  return ans;
}

/**
 * Computes first-order differences of a secret/shared vector with an initial public value.
 *
 * For each index `i`, the output is:
 * `ans[i] = v[i] - prev`, where `prev` is initialized to `z` and then updated to `v[i]`
 * after each iteration. This yields:
 * - `ans[0] = v[0] - z`
 * - `ans[i] = v[i] - v[i-1]` for `i > 0`
 *
 * The operation is supported only when `_party <= 2`; otherwise the function returns `NULL`.
 *
 * @param v Input vector of shared values.
 * @param z Initial public baseline used for the first difference.
 * @return A newly allocated vector containing consecutive differences, or `NULL` if unsupported.
 */
_ Diff(_ v, share_t z)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _ prev = _slice(v, 0, 1);
  _setpublic(prev, 0, z);
  for (int i=0; i<n; i++) {
    _setshare(ans, i, v, i);
    _subshare(ans, i, prev, 0);
    _setshare(prev, 0, v, i);
  }
  _free(prev);
  return ans;
}

/**
 * @brief Computes the prefix sum (rank) of a shared vector for a 2-party computation.
 *
 * This function calculates the cumulative sum of elements in the input shared vector,
 * storing the running total at each position in the output vector.
 * Only supports up to 2 parties; returns NULL if more than 2 parties are involved.
 *
 * @param v The input shared vector to compute the rank/prefix sum of.
 *
 * @return A new shared vector containing the prefix sums of the input vector,
 *         or NULL if the number of parties exceeds 2.
 *
 * @note The caller is responsible for freeing the returned vector.
 * @note The computation is performed using secret sharing arithmetic.
 */
_ rank1(_ v)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _ sum = _slice(v, 0, 1);
  _setpublic(sum, 0, 0);
  for (int i=0; i<n; i++) {
    _addshare(sum, 0, v, i);
    _setshare(ans, i, sum, 0);
  }
  _free(sum);
  return ans;
}

/**
 * @brief Computes the prefix rank of zeros for a secret/shared binary vector.
 *
 * For each position `i`, this function stores in the result the number of `0` values
 * in `v[0..i]` (inclusive). Internally, it maintains a running counter as:
 * `sum = sum + 1 - v[i]`, which increments on `0` and leaves the count unchanged on `1`.
 *
 * @param v Input shared vector (typically binary shares).
 * @return A shared vector of the same length containing cumulative zero-counts,
 *         or `NULL` when `_party > 2` (unsupported party configuration).
 *
 * @note This routine is intended for 2-party settings (`_party <= 2`).
 */
_ rank0(_ v)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _dup(v);
  _ sum = _slice(v, 0, 1);
  _setpublic(sum, 0, 0);
  for (int i=0; i<n; i++) {
    _addpublic(sum, 0, 1);
    _subshare(sum, 0, v, i);
    _setshare(ans, i, sum, 0);
  }
  _free(sum);
  return ans;
}

/**
 * @brief Computes the sum of elements in a shared secret vector.
 *
 * Iterates over all elements of the input vector and accumulates
 * their values into a single shared secret element using additive
 * secret sharing.
 *
 * @param v A shared secret vector whose elements are to be summed.
 * @return A shared secret vector containing the sum of all elements
 *         in @p v, or NULL if the party index is greater than 2.
 *
 * @note This function only supports up to 2 parties. If `_party > 2`,
 *       the function returns NULL.
 */
_ sum(_ v)
{
  if (_party >  2) return NULL;
  int n = len(v);
  _ ans = _slice(v, 0, 1);
  for (int i=1; i<n; i++) {
    _addshare(ans, 0, v, i);
  }
  return ans;
}


/**
 * @brief Adds the first share of `b` to every element of share vector `a`.
 *
 * This function iterates over all elements in `a` and applies `_addshare(a, i, b, 0)`
 * for each index `i`. If `_party > 2`, the function returns immediately without
 * performing any operation.
 *
 * @param a Destination share vector to be updated element-wise.
 * @param b Source share vector; only index `0` is used as the addend.
 */
void addall(_ a, _ b)
{
  if (_party >  2) return;
  int n = len(a);
  for (int i=0; i<n; i++) {
    _addshare(a, i, b, 0);
  }
}

void suball(_ a, _ b)
{
  if (_party >  2) return;
  int n = len(a);
  for (int i=0; i<n; i++) {
    _subshare(a, i, b, 0);
  }
}

/**
 * @brief Computes the element-wise bitwise XOR of two secret-share vectors.
 *
 * Duplicates the first input vector and replaces each element with the XOR
 * of corresponding raw share values from @p x and @p y.
 *
 * @param x First share vector.
 * @param y Second share vector (must be compatible in length with @p x).
 * @return A new share vector containing element-wise XOR results, or NULL
 *         when the current party index is greater than 2.
 */
static _ _xor(_ x, _ y)
{
  if (_party >  2) return NULL;
  int n = len(x);
  _ ans = _dup(x);
  for (int i=0; i<n; i++) {
    share_t xtmp, ytmp;
    xtmp = share_getraw(x, i);
    ytmp = share_getraw(y, i);
    xtmp ^= ytmp;
    share_setraw(ans, i, xtmp);
  }
  return ans;
}

/*
# 連続する l 個の要素の和をとる
def extended_sum(l, x):
  n = len(x)
  ans = Share([0] * (n//l), x.order())
  for i in range(0, n//l):
    for j in range(0, l):
      ans[i] += x[i*l+j]
  return ans
*/
//////////////////////////////////////////////////////////////////
// Compute sums of consecutive groups of l in x <- incorrect
// Add elements of x at stride l
//////////////////////////////////////////////////////////////////
/**
 * @brief Computes block-wise sums of a shared array.
 *
 * Splits the input array `x` (length `n`) into `n / l` consecutive blocks,
 * each of size `l`, and returns an array `ans` of length `l` where:
 *
 *   ans[j] = sum_{i=0}^{n/l - 1} x[i*l + j]
 *
 * In other words, it accumulates elements at the same offset `j` within each block.
 *
 * @param l Block size and length of the returned array.
 * @param x Input shared array whose length must be divisible by `l`.
 * @return share_array Array of length `l` containing the per-offset sums.
 *
 * @note The function aborts the program if `len(x) % l != 0`.
 * @note The returned array uses the same field/order as `x`.
 */
share_array extended_sum(int l, share_array x) {

//    printf("extended_sum l=%d\n", l);
//    exit(1);

    int n = len(x);
    share_t q = order(x);
    if (n % l != 0) {
        printf("extended_sum:    n = %d, l = %d, n %% l = %d\n", n, l, n % l);
        exit(1);
    }

    share_array ans = share_const(l, 0, q);
    for (int i = 0; i < n / l; ++i) {
        for (int j = 0; j < l; ++j) {
            share_addshare(ans, j, x, i * l + j); // Looks odd. Should array length be n/l? <- Actually this is correct since it sums every l-th element.
        }
    }

    return ans;
}

///////////////////////////////////////////////////////
// Compute sums over consecutive groups of size step (local)
///////////////////////////////////////////////////////
/**
 * @brief Reduces a sequence by summing fixed-size contiguous groups.
 *
 * This function partitions `x` into `m = len(x) / step` consecutive blocks of
 * size `step` and accumulates each block into one output element:
 * `ans[i] = sum(x[i*step + j])` for `j = 0..step-1`.
 *
 * @param x     Input sequence/object to reduce.
 * @param step  Block size used for reduction; must exactly divide `len(x)`.
 *
 * @return A newly allocated object of length `len(x)/step`, initialized with
 *         zeros and filled with per-block accumulated shares. The output order
 *         matches `order(x)`.
 *
 * @note If `len(x)` is not divisible by `step`, the function prints an error
 *       message and terminates the process via `exit(1)`.
 * @note The outer loop over block index `i` is straightforward to parallelize.
 */
_ _reduce(_ x, int step)
{
  int n = len(x);
  int m = n/step;
  if (m * step != n) {
    printf("_reduce: n = %d step = %d\n", n, step);
    exit(1);
  }
  _ ans = _const(m, 0, order(x));
  //ans->type = x->type;
  // Parallelization over i is straightforward
  for (int i=0; i<m; i++) {
    for (int j=0; j<step; j++) {
      _addshare(ans, i, x, i*step+j);
    }
  }
  return ans;
}

/*

# n/l 個置きの要素の和をとる
def extended_sum_v(l, x):
  n = len(x)
  m = n//l
  ans = Share([0] * m, x.order())
  for i in range(0, m):
    for j in range(0, l):
      ans[i] += x[j*m+i]
  return ans
*/
share_array extended_sum_v(int l, share_array x) {

    int n = len(x);
    share_t q = order(x);
    if (n % l != 0) {
        printf("extended_sum_v:  n = %d, l = %d, n %% l = %d\n", n, l, n % l);
        exit(1);
    }
    int m = n / l;

    share_array ans = share_const(m, 0, q);
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < l; ++j) {
            share_addshare(ans, i, x, j * m + i);
        }
    }

    return ans;
}

#endif
