#ifndef _DSHARE_H
 #define _DSHARE_H

/*
  This is an implementation of the double share operations described in the following paper:

  Nuttapong Attrapadung, Goichiro Hanaoaka, Takahiro Matsuda, Hiraku Morita, Kazuma Ohara, Jacob C. N. Schuldt, Tadanori Teruya, 
  and Kazunari Tozawa. Oblivious Linear Group Actions and Applications. In Proceedings of the 2021 ACM SIGSAC Conference on Computer and
  Communications Security (CCS '21). Association for Computing Machinery, New York, NY, USA, 630–650. 
  https://doi.org/10.1145/3460120.3484584 
*/


#include <stdarg.h>
#include "share.h"

#ifndef max
 #define max(x, y) ((x > y)?x:y)
#endif

extern long total_perm;

typedef struct {
  int n;
  int bs;
  precomp_table PRG;
  precomp_table pp_1, b_1, pp_2, b_2;
  MMAP *map;
}* DS_tables;

typedef struct ds_tbl_list {
  DS_tables tbl;
  int n;
  int bs;
  int inverse;
  long count;
  struct ds_tbl_list *next;
}* ds_tbl_list;

ds_tbl_list PRE_DS_tbl[MAX_CHANNELS];
long PRE_DS_count[MAX_CHANNELS];

typedef packed_array perm;

typedef struct dshare {
// public
  int n;
  share_t q;       // modulus of array elements
  int bs;

// P0: pi is an n-element permutation
  perm pi;
  perm p1, p2p; // pi = p1 · p2p
  perm p2, p1p; // pi = p2 · p1p

// P1, P2
  perm g, gp; // P1:(g, gp) = (p1, p1p)   P2:(g, gp) = (p2, p2p)


/////////////////
// correlated_random
/////////////////

// P0: a1, b1, a2, b2, c are each n*bs-dimensional vectors
  perm a1, b1;  // b1 = a2 · p1p + c
  perm a2, b2;  // b2 = a1 · p2p - c

// P1, P2
  perm a, b;  // P1:(a, b) = (a1, b1)   P2:(a, b) = (a2, b2)

}* dshare;

typedef struct dshare_cont {
  //DS_tables tbl;
  int bs;
  share_array x;
  share_array sigma;
  dshare ds;

  // variables used in dshare_shuffle
  share_array v;

}* dshare_cont;

typedef struct AppPerm_cont {
  int n, n2;
  int bs;
  share_array x;
  share_array sigma;
  dshare_cont dc1;
  dshare_cont dc2;
  _ rho;
  _ z;
}* AppPerm_cont;

/////////////////////////////////////////////////
// Permutation (plaintext): 0, 1, ..., n-1
/////////////////////////////////////////////////

static perm perm_id(int n)
{
  if (_party >  2) return NULL;
  perm pi;
  int i;
  int k = blog(n-1)+1;
  pi = pa_new(n, k);

#if 0
  for (i=0; i<n; i++) pa_set(pi, i, i);
#else
  pa_iter itr = pa_iter_new(pi);
  for (i=0; i<n; i++) pa_iter_set(itr, i);
  pa_iter_flush(itr);
#endif

  return pi;
}

static void perm_print(int n, perm pi)
{
  if (_party >  2) return;
  int i;
  printf("(");
  for (i=0; i<n; i++) {
    printf("%d", (int)pa_get(pi, i));
    if (i < n-1) printf(" ");
  }
  printf(")\n");
}

static void perm_free(perm pi)
{
  if (_party >  2);
  pa_free(pi);
}

static perm perm_inverse(perm pi)
{
  if (_party >  2) return NULL;
  perm pi_inv;
  share_t *pi_inv_tmp;
  NEWA(pi_inv_tmp, share_t, pi->n);

  pa_iter itr = pa_iter_new(pi);

  int n = pi->n;
  for (int i=0; i<n; i++) {
    share_t x = pa_iter_get(itr);
    if ((x < 0) || (x >= n)) {
      printf("pi[%d] = %d", i, (int)x);
      exit(1);
    }
    pi_inv_tmp[x] = i;
  }
  pi_inv = pa_pack(n, blog(n-1)+1, pi_inv_tmp);
  pa_iter_free(itr);
  free(pi_inv_tmp);
  return pi_inv;
}

//////////////////////////////////////////////////////////////////////
// Random permutation (plaintext)
//////////////////////////////////////////////////////////////////////
static perm perm_random(MT mt, int n)
{
  if (_party >  2) return NULL;
  int i, j, m;
  share_t *pi_tmp, *pi2_tmp;
  NEWA(pi_tmp, share_t, n);
  NEWA(pi2_tmp, share_t, n);
  for (int i=0; i<n; i++) pi2_tmp[i] = i;
  for (m=0; m<n; m++) {
    i = RANDOM(mt, n-m);
    j = pi2_tmp[i];
    pi_tmp[j] = m;
    pi2_tmp[i] = pi2_tmp[n-1-m];
  }
  perm pi = pa_pack(n, blog(n-1)+1, pi_tmp);
  free(pi_tmp); free(pi2_tmp);
  return pi;
}

static _ share_random_perm(int n)
{
  if (_party >  2) return NULL;
  int k = blog(n-1)+1;
  _ ans = share_const(n, 0, 1<<k);
  if (_party <= 0) {
    perm p = perm_random(mt0, n); // TODO: channel?
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i, pa_get(p, i));
    }
    perm_free(p);
  }
  return ans;
}
#define _random_perm share_random_perm


/////////////////////////////////////////////////////
// Compute p · q for permutation q
/////////////////////////////////////////////////////
/**
 * @brief Applies a block-wise permutation to a packed permutation array.
 *
 * Constructs a new permutation by reordering blocks of size @p bs from @p p
 * according to the indices stored in @p q. For each position `i` in @p q, the
 * block starting at `q[i] * bs` in @p p is copied into block `i` of the result.
 *
 * The returned permutation has `q->n * bs` elements and preserves the word
 * width of @p p.
 *
 * @param bs Size of each block to copy.
 * @param p Source permutation containing the blocks to be selected.
 * @param q Permutation of block indices indicating the order of blocks in the result.
 * @return A newly allocated permutation with blocks rearranged as specified by @p q,
 *         or `NULL` when the current party configuration is unsupported.
 *
 * @note This function is only supported when `_party <= 2`.
 * @note The caller is responsible for releasing the returned permutation.
 */
static perm block_perm_apply(int bs, perm p, perm q) {
  if (_party >  2) return NULL;
  perm pq;
  int k = p->w;
  int n = q->n;
  pq = pa_new(n*bs, k);
  pa_iter itr_pq = pa_iter_new(pq);
  pa_iter itr_q = pa_iter_new(q);
  share_t *p_raw = pa_unpack(p);
  for (int i = 0; i < n; ++i) {
    share_t vq = pa_iter_get(itr_q);
    for (int j = 0; j < bs; ++j) {
      pa_iter_set(itr_pq, p_raw[vq*bs + j]);
    }
  }
  pa_iter_flush(itr_pq);  pa_iter_free(itr_q);
  free(p_raw);
  return pq;
}
#define perm_apply(p, q) block_perm_apply(1, p, q)

/**
 * @brief Applies a block-wise permutation to a shared array.
 *
 * Creates a duplicate of the input shared array and permutes it in units of
 * fixed-size blocks. For each block index `i`, the value `pi[i]` selects the
 * source block copied into destination block `i`. Each selected block contains
 * `bs` consecutive elements.
 *
 * This routine is only supported when the number of parties is at most 2.
 * If `_party > 2`, the function returns `NULL`.
 *
 * @param bs Block size in elements.
 * @param x  Input shared array whose contents are permuted block by block.
 * @param pi Permutation array of block indices. Its length is expected to be
 *           equal to `x->n / bs`.
 *
 * @return A newly allocated shared array containing the permuted blocks, or
 *         `NULL` when the current party configuration is unsupported.
 *
 * @note The function expects `x->n / bs == pi->n`. If this does not hold, a
 *       diagnostic message is printed.
 * @note The input array length should be divisible by `bs`.
 */
static share_array block_share_perm(int bs, _ x, perm pi) {
  if (_party >  2) return NULL;
  if (x->n/bs != pi->n) {
    printf("block_share_perm: x->n %d pi->n %ld\n", x->n, pi->n);
  }
  _ ans = _dup(x);
  int n = len(x)/bs;
  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_pi = pa_iter_new(pi);
  share_t *x_raw = pa_unpack(x->A);
  for (int i = 0; i < n; ++i) {
    share_t v = pa_iter_get(itr_pi);
    for (int j = 0; j < bs; ++j) {
      pa_iter_set(itr_ans, x_raw[v *bs + j]);
    }
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_pi);
  free(x_raw);
  return ans;
}
#define share_perm(a, pi) block_share_perm(1, a, pi) 



/////////////////////////////////////////////////////////////////////////////
// Compute correlated randomness for dshare online
/////////////////////////////////////////////////////////////////////////////
static void block_dshare_correlated_random_channel(dshare ds, int channel) {
  if (_party >  2) return;
    int bs = ds->bs;
    int n = ds->n;
    share_t q = ds->q;
    int k = blog(q - 1) + 1;
    if (_party <= 0) {
        ds->a1 = pa_new(n*bs, k);
        ds->a2 = pa_new(n*bs, k);
        perm c = pa_new(n*bs, k);

        pa_iter itr_a1 = pa_iter_new(ds->a1);
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            pa_iter_set(itr_a1, RANDOM(mt_[TO_PARTY1][channel], q));
          }
        }
        pa_iter_flush(itr_a1);
        pa_iter itr_a2 = pa_iter_new(ds->a2);
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            pa_iter_set(itr_a2, RANDOM(mt_[TO_PARTY2][channel], q));
          }
        }
        pa_iter_flush(itr_a2);
        pa_iter itr_c = pa_iter_new(c);
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            pa_iter_set(itr_c, RANDOM(mt_[0][channel], q));
          }
        }
        pa_iter_flush(itr_c);
        ds->b1 = block_perm_apply(bs, ds->a2, ds->p1p);
        ds->b2 = block_perm_apply(bs, ds->a1, ds->p2p);

        pa_iter itr_b1r = pa_iter_new(ds->b1);
        pa_iter itr_b2r = pa_iter_new(ds->b2);
        pa_iter itr_b1w = pa_iter_new(ds->b1);
        pa_iter itr_b2w = pa_iter_new(ds->b2);
        itr_c = pa_iter_new(c);
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            share_t cp = pa_iter_get(itr_c);
            pa_iter_set(itr_b1w, MOD(pa_iter_get(itr_b1r) + cp));
            pa_iter_set(itr_b2w, MOD(pa_iter_get(itr_b2r) - cp));
          }
        }
        pa_iter_flush(itr_b1w); pa_iter_free(itr_b1r);
        pa_iter_flush(itr_b2w); pa_iter_free(itr_b2r);
        pa_iter_free(itr_c);

        perm_free(c);

        if (_party == 0) {
            mpc_send_pa_channel(TO_PARTY1, ds->b1, channel);    //send_3 += pa_size(ds->b1);
            mpc_send_pa_channel(TO_PARTY2, ds->b2, channel);
        }
    }
    else {  // party 1, 2
        ds->a = pa_new(n*bs, k);
        ds->b = pa_new(n*bs, k);
        
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            int p = i*bs+j;
            pa_set(ds->a, p, RANDOM(mt_[FROM_SERVER][channel], q));
          }
        }
        mpc_recv_channel(FROM_SERVER, (char *)ds->b->B, pa_size(ds->b), channel);
    }
}
#define dshare_correlated_random_channel(ds, channel) block_dshare_correlated_random_channel(ds, channel)
#define dshare_correlated_random(ds) dshare_correlated_random_channel(ds, 0)

static void dshare_correlated_random_xor_channel(dshare ds, int channel) {
  if (_party >  2) return;
    int bs = ds->bs;
    int n = ds->n;
    share_t q = ds->q;
    int k = blog(q - 1) + 1;
    if (_party <= 0) {
        ds->a1 = pa_new(n*bs, k);
        ds->a2 = pa_new(n*bs, k);
        perm c = pa_new(n*bs, k);

        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            pa_set(ds->a1, i*bs+j, RANDOM(mt_[TO_PARTY1][channel], q));
          }
        }
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            pa_set(ds->a2, i*bs+j, RANDOM(mt_[TO_PARTY2][channel], q));
          }
        }
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            pa_set(c, i*bs+j, RANDOM(mt_[0][0], q));
          }
        }
        ds->b1 = block_perm_apply(bs, ds->a2, ds->p1p);
        ds->b2 = block_perm_apply(bs, ds->a1, ds->p2p);

        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            int p = i*bs+j;
            pa_set(ds->b1, p, pa_get(ds->b1, p) ^ pa_get(c, p));
            pa_set(ds->b2, p, pa_get(ds->b2, p) ^ pa_get(c, p));
          }
        }

        perm_free(c);

        if (_party == 0) {
            mpc_send_pa_channel(TO_PARTY1, ds->b1, channel);    //send_3 += pa_size(ds->b1);
            mpc_send_pa_channel(TO_PARTY2, ds->b2, channel);
        }
    }
    else {  // party 1, 2
        ds->a = pa_new(n*bs, k);
        ds->b = pa_new(n*bs, k);
        
        for (int i = 0; i < n; ++i) {
          for (int j = 0; j < bs; ++j) {
            int p = i*bs+j;
            pa_set(ds->a, p, RANDOM(mt_[FROM_SERVER][channel], q));
          }
        }
        mpc_recv_channel(FROM_SERVER, (char *)ds->b->B, pa_size(ds->b), channel);
    }
}


///////////////////////////////////////////////////////////////////////////////////
// Compute dshare online
// ds is not used here
///////////////////////////////////////////////////////////////////////////////////
static dshare block_dshare_new2_channel(int bs, perm pi, share_t q, int channel) {
  if (_party >  2) return NULL;
    NEWT(dshare, ds);
    int n = pi->n;
    ds->n = n;
    ds->q = q;
    ds->bs = bs;
    if (_party <= 0) {
        ds->pi = pa_dup(pi);
        perm p1_inv, p2_inv;
        ds->p1 = perm_random(mt_[TO_PARTY1][channel], n);
        p1_inv = perm_inverse(ds->p1);
        ds->p2p = perm_apply(p1_inv, pi);

        ds->p2 = perm_random(mt_[TO_PARTY2][channel], n);
        p2_inv = perm_inverse(ds->p2);
        ds->p1p = perm_apply(p2_inv, pi);
        perm_free(p1_inv);
        perm_free(p2_inv);

        if (_party == 0) {
            mpc_send_pa_channel(TO_PARTY1, ds->p1p, channel);  //send_4 += pa_size(ds->p1p);
            mpc_send_pa_channel(TO_PARTY2, ds->p2p, channel);
        }
    } else {
        ds->g = perm_random(mt_[FROM_SERVER][channel], n);
        ds->gp = perm_id(n);
        mpc_recv_channel(FROM_SERVER, (char *)ds->gp->B, pa_size(ds->gp), channel);
    }

    return ds;
}

///////////////////////////////////////////////////////////////////////////////////
// Compute dshare offline
// ds is not used here
///////////////////////////////////////////////////////////////////////////////////
static dshare block_dshare_new_offline(int bs, perm pi, share_t q, int channel) 
{
    if (_party >  2) return NULL;
    NEWT(dshare, ds);
    int n = pi->n;
    ds->n = n;
    ds->q = q;
    ds->bs = bs;
    if (_party <= 0) {
        ds->pi = perm_id(n);
        for (int i = 0; i < n; ++i) {
            pa_set(ds->pi, i, pa_get(pi, i));
        }

        perm p1_inv, p2_inv;
        ds->p1 = perm_random(mt_[TO_PARTY1][0], n);
        p1_inv = perm_inverse(ds->p1);
        ds->p2p = perm_apply(p1_inv, pi);
        ds->p2 = perm_random(mt_[TO_PARTY2][0], n);
        p2_inv = perm_inverse(ds->p2);
        ds->p1p = perm_apply(p2_inv, pi);
        perm_free(p1_inv);
        perm_free(p2_inv);

    }

    block_dshare_correlated_random_channel(ds, channel);

    return ds;
}


///////////////////////////////////////////////////////////////////////////////////
// Compute dshare online
///////////////////////////////////////////////////////////////////////////////////
static dshare block_dshare_new_channel(int bs, perm pi, share_t q, int channel)
{
  dshare ds = block_dshare_new2_channel(bs, pi, q, channel);
  block_dshare_correlated_random_channel(ds, channel);
  return ds;
}
#define dshare_new_channel(pi, q, channel) block_dshare_new_channel(1, pi, q, channel)
#define dshare_new(pi, q) dshare_new_channel(pi, q, 0)
#define block_dshare_new(bs, pi, q) block_dshare_new_channel(bs, pi, q, 0)

static dshare dshare_new_xor_channel(perm pi, share_t q, int channel)
{
  dshare ds = block_dshare_new2_channel(1, pi, q, channel);
  dshare_correlated_random_xor_channel(ds, channel);
  return ds;
}


static dshare block_dshare_new_party0(int bs, int n, share_t q)
{
  if (_party >  2) return NULL;

  NEWT(dshare, ds);
  ds->n = n;
  ds->q = q;
  ds->bs = bs;

  ds->pi = perm_id(n);

  perm p1_inv, p2_inv;
  ds->p1 = perm_id(n);
  ds->p2p = perm_id(n);

  ds->p2 = perm_id(n);
  ds->p1p = perm_id(n);


  int k = blog(q-1)+1;

  ds->a1 = pa_new(n*bs, k);
  ds->a2 = pa_new(n*bs, k);
  for (int i=0; i<n*bs; i++) {
    pa_set(ds->a1, i, 0);
  }
  for (int i=0; i<n*bs; i++) {
    pa_set(ds->a2, i, 0);
  }
  ds->b1 = block_perm_apply(bs, ds->a2, ds->p1p);
  ds->b2 = block_perm_apply(bs, ds->a1, ds->p2p);

  return ds;
}
#define dshare_new_party0(n, q) block_dshare_new_party0(1, n, q)

///////////////////////////////////////////
// Generate only the permutation. Generate additional randomness separately.
///////////////////////////////////////////
static dshare dshare_new2_channel(perm pi, share_t q, int channel)
{
  if (_party >  2) return NULL;
  int n = pi->n;
  total_perm++;

  NEWT(dshare, ds);
  ds->n = n;
  ds->q = q;
  ds->bs = 1;
  if (_party <= 0) {
    ds->pi = perm_id(n);
    for (int i=0; i<n; i++) pa_set(ds->pi, i, pa_get(pi, i));

    perm p1_inv, p2_inv;
    ds->p1 = perm_random(mt_[TO_PARTY1][channel], n);
    p1_inv = perm_inverse(ds->p1);
    ds->p2p = perm_apply(p1_inv, pi);

    ds->p2 = perm_random(mt_[TO_PARTY2][channel], n);
    p2_inv = perm_inverse(ds->p2);
    ds->p1p = perm_apply(p2_inv, pi);
    perm_free(p1_inv);
    perm_free(p2_inv);

    mpc_send_pa_channel(TO_PARTY1, ds->p1p, channel);  //send_5 += pa_size(ds->p1p);
    mpc_send_pa_channel(TO_PARTY2, ds->p2p, channel);
  } else {
    ds->g = perm_random(mt_[FROM_SERVER][channel], n);
    ds->gp = perm_id(n);
    mpc_recv_pa_channel(FROM_SERVER, ds->gp, channel);
  }

  return ds;
}

#define dshare_new2(pi, q) dshare_new2_channel(pi, q, 0)

static void dshare_free(dshare ds)
{
  if (_party >  2) return;
  if (_party <= 0) {
    perm_free(ds->pi);
    perm_free(ds->p1);
    perm_free(ds->p1p);
    perm_free(ds->p2);
    perm_free(ds->p2p);
    perm_free(ds->a1);
    perm_free(ds->b1);
    perm_free(ds->a2);
    perm_free(ds->b2);
  } else {
    perm_free(ds->g);
    perm_free(ds->gp);
    perm_free(ds->a);
    perm_free(ds->b);
  }
  free(ds);
}

///////////////////////////////////////////
// Free everything except the additional randomness
///////////////////////////////////////////
static void dshare_free2(dshare ds)
{
  if (_party >  2) return;
  if (_party <= 0) {
    perm_free(ds->pi);
    perm_free(ds->p1);
    perm_free(ds->p1p);
    perm_free(ds->p2);
    perm_free(ds->p2p);
//    perm_free(ds->a1);
//    perm_free(ds->b1);
//    perm_free(ds->a2);
//    perm_free(ds->b2);
  } else {
    perm_free(ds->g);
    perm_free(ds->gp);
//    perm_free(ds->a);
//    perm_free(ds->b);
  }
  free(ds);
}

///////////////////////////////////////////
// Free only the additional randomness
///////////////////////////////////////////
static void dshare_free3(dshare ds)
{
  if (_party >  2) return;
  if (_party <= 0) {
    perm_free(ds->a1);
    perm_free(ds->b1);
    perm_free(ds->a2);
    perm_free(ds->b2);
  } else {
    perm_free(ds->a);
    perm_free(ds->b);
  }
}

/*************************************************************
def dshare_shuffle(X1, X2, p1, p2, p1p, p2p, a1, a2, a1p, a2p):
  n = len(X1)

# P1
  x1 = X1
  v1 = perm_apply(x1, p1)
  i = 0
  while i < n:
    v1[i] += a1[i]
    i += 1

# P2
  x2 = X2
  v2 = perm_apply(x2, p2)
  i = 0
  while i < n:
    v2[i] += a2[i]
    i += 1

# P1
  y1 = perm_apply(v2, p1p)
  i = 0
  while i < n:
    y1[i] -= a1p[i]
    i += 1

# P2
  y2 = perm_apply(v1, p2p)
  i = 0
  while i < n:
    y2[i] -= a2p[i]
    i += 1

  return (y1, y2)
*************************************************************/

#define block_dshare_shuffle_channel block_dshare_shuffle_channel_new
#define block_dshare_shuffle(bs, x, ds) block_dshare_shuffle_channel(bs, x, ds, 0)
#define dshare_shuffle_channel(X, ds, channel) block_dshare_shuffle_channel(1, X, ds, channel)
#define dshare_shuffle(X, ds) dshare_shuffle_channel(X, ds, 0)

////////////////////////////////////////////////////////////////////////////////////////////
// Split into phases
////////////////////////////////////////////////////////////////////////////////////////////

static void block_dshare_shuffle_channel_phase0(dshare_cont dc, int channel) {
  if (_party >  2) return;
  
  int bs = dc->bs;
  share_array x = dc->x;
  dshare ds = dc->ds;

  if (bs != ds->bs) {
    printf("block_dshare_shuffle_channel: bs = %d ds->bs = %d\n", bs, ds->bs);
    exit(1);
  }
  share_t q = order(x);
  share_array v;
  int n = len(x) / bs;
  if (n != ds->n) {
    printf("block_dshare_shuffle_channel: n %d ds->n %d\n", n, ds->n);
    exit(EXIT_FAILURE);
  }

  if (_party <= 0) {
    v = block_share_perm(bs, x, ds->p1);
    pa_iter itr_vr = pa_iter_new(v->A);
    pa_iter itr_vw = pa_iter_new(v->A);
    pa_iter itr_a1 = pa_iter_new(ds->a1);
    for (int i = 0; i < n*bs; ++i) {
      pa_iter_set(itr_vw, MOD(pa_iter_get(itr_vr) + pa_iter_get(itr_a1)));
    }
    pa_iter_flush(itr_vw); pa_iter_free(itr_vr); pa_iter_free(itr_a1);
  } else {
    v = block_share_perm(bs, x, ds->g);
    pa_iter itr_vr = pa_iter_new(v->A);
    pa_iter itr_vw = pa_iter_new(v->A);
    pa_iter itr_a = pa_iter_new(ds->a);
    for (int i = 0; i < n*bs; ++i) {
      pa_iter_set(itr_vw, MOD(pa_iter_get(itr_vr) + pa_iter_get(itr_a)));
    }
    pa_iter_flush(itr_vw); pa_iter_free(itr_vr); pa_iter_free(itr_a);
  }

  dc->v = v;

  share_array y;
  if (_party <= 0) {
  } else {
    mpc_send_channel(TO_PAIR, v->A->B, pa_size(v->A), channel);
  }
}

static share_array block_dshare_shuffle_channel_phase1(dshare_cont dc, int channel) {
  if (_party >  2) return NULL;
  int bs = dc->bs;
  int n = len(dc->x) / bs;
  share_t q = order(dc->x);
  dshare ds = dc->ds;
  _ v = dc->v;
  _ y;

  if (_party <= 0) {
    y = block_share_perm(bs, v, ds->p2p);
    //printf("typeof v %d\n", v->A->type);
    //printf("typeof y %d\n", y->A->type);
    perm tmp = block_perm_apply(bs, ds->a2, ds->p1p);
    pa_iter itr_yr = pa_iter_new(y->A);
    pa_iter itr_yw = pa_iter_new(y->A);
    pa_iter itr_b1 = pa_iter_new(ds->b1);
    pa_iter itr_b2 =pa_iter_new(ds->b2);
    pa_iter itr_tmp = pa_iter_new(tmp);
    for (int i = 0; i < n*bs; ++i) {
      pa_iter_set(itr_yw, MOD(pa_iter_get(itr_yr) + pa_iter_get(itr_tmp) - pa_iter_get(itr_b1) - pa_iter_get(itr_b2)));
    }
    pa_iter_flush(itr_yw); pa_iter_free(itr_yr); pa_iter_free(itr_b2); pa_iter_free(itr_b1); pa_iter_free(itr_tmp);
    perm_free(tmp);

  } else {
    share_array z = share_dup(v);
    mpc_recv_channel(FROM_PAIR, z->A->B, pa_size(z->A), channel);
    y = block_share_perm(bs, z, ds->gp);
    pa_iter itr_yr = pa_iter_new(y->A);
    pa_iter itr_yw = pa_iter_new(y->A);
    pa_iter itr_b = pa_iter_new(ds->b);
    for (int i = 0; i < n*bs; ++i) {
      pa_iter_set(itr_yw, MOD(pa_iter_get(itr_yr) - pa_iter_get(itr_b)));
    }
    pa_iter_flush(itr_yw); pa_iter_free(itr_yr); pa_iter_free(itr_b);
    _free(z);
  }
  _free(v);  dc->v = NULL;
  return y;
}

static share_array block_dshare_shuffle_channel_new(int bs, share_array x, dshare ds, int channel) {
  NEWT(dshare_cont, dc);
  dc->bs = bs;
  dc->x = x;
  dc->ds = ds;
  block_dshare_shuffle_channel_phase0(dc, channel);
  _ ans = block_dshare_shuffle_channel_phase1(dc, channel);
  free(dc);
  return ans;
}
#define block_dshare_shuffle_channel block_dshare_shuffle_channel_new

static share_array dshare_shuffle_xor_channel(share_array x, dshare ds, int channel) {
  if (_party >  2) return NULL;
  share_t q = order(x);
  share_array v;
  int n = len(x);
  if (n != ds->n) {
      printf("dshare_shuffle_xor_channel: n %d ds->n %d\n", n, ds->n);
      exit(EXIT_FAILURE);
  }

  if (_party <= 0) {
    v = share_perm(x, ds->p1);
    for (int i = 0; i < n; ++i) {
      pa_set(v->A, i, pa_get(v->A, i) ^ pa_get(ds->a1, i));
    }
  } else {
    v = share_perm(x, ds->g);
    for (int i = 0; i < n; ++i) {
      pa_set(v->A, i, pa_get(v->A, i) ^ pa_get(ds->a, i));
    }
  }

  share_array y;
  if (_party <= 0) {
    y = share_perm(v, ds->p2p);
    for (int i = 0; i < n; ++i) {
      pa_set(y->A, i, pa_get(y->A, i) ^ pa_get(ds->b2, i));
    }
    perm tmp = perm_apply(ds->a2, ds->p1p);
    for (int i = 0; i < n; ++i) {
      pa_set(y->A, i, pa_get(y->A, i) ^ pa_get(tmp, i) ^ pa_get(ds->b1, i));
    }
    perm_free(tmp);
  } else {
    share_array z = share_dup(v);
    mpc_exchange_channel(v->A->B, z->A->B, pa_size(v->A), channel);
    y = share_perm(z, ds->gp);
    for (int i = 0; i < n; ++i) {
      pa_set(y->A, i, pa_get(y->A, i) ^ pa_get(ds->b, i));
    }
    _free(z);
  }
  _free(v);

  return y;
}



static void check_dshare(dshare ds) {
  if (_party > 2) return;
  if (_party <= 0) {
    packed_array p1_p2p = perm_apply(ds->p1, ds->p2p);
    packed_array p2_p1p = perm_apply(ds->p2, ds->p1p);
  }
  else {
    packed_array g_other = pa_new(ds->g->n, ds->g->w), gp_other = pa_new(ds->g->n, ds->g->w);
    mpc_exchange(ds->g->B, g_other->B, pa_size(g_other));
    mpc_exchange(ds->gp->B, gp_other->B, pa_size(gp_other));
    packed_array g_gp_other = perm_apply(ds->g, gp_other);
    packed_array a_other = pa_new(ds->a->n, ds->a->w);
    mpc_exchange(ds->a->B, a_other->B, pa_size(a_other));
    
  }
}




//////////////////////////////////////////////////
// Precomputation of dshare
// n: length of permutation
// m: number of permutations
//////////////////////////////////////////////////
void DS_tables_precomp(int bs, int m, int n, share_t q, int inverse, char *fname)
{
  FILE *f0, *f1, *f2;

  int kn = blog(n-1)+1;

  char *fname0 = precomp_fname(fname, 0);
  char *fname1 = precomp_fname(fname, 1);
  char *fname2 = precomp_fname(fname, 2);

  f0 = fopen(fname0, "wb");
  f1 = fopen(fname1, "wb");
  f2 = fopen(fname2, "wb");

  unsigned long init[5]={0x123, 0x234, 0x345, 0x456, 0};
  MT m0 = MT_init_by_array(init, 5);

  // random numbers used by party 1
  unsigned long init1[5]={0x123, 0x234, 0x345, 0x456, 0};
  init1[4] = 1; // rand();
  mt_[TO_PARTY1][0] = MT_init_by_array(init1, 5); // WARNING: This function should not be used during normal computation

  // random numbers used by party 2
  unsigned long init2[5]={0x123, 0x234, 0x345, 0x456, 0};
  init2[4] = 2; // rand();
  mt_[TO_PARTY2][0] = MT_init_by_array(init2, 5);

  perm g = perm_random(m0, n);
  dshare ds1, ds2;
  share_t qq = max(1<<kn, q);

  if (inverse == 0) {
    perm g_inv = perm_inverse(g);
    ds1 = block_dshare_new_offline(1, g, qq, 0); // for permutation, block size is irrelevant
    ds2 = block_dshare_new_offline(bs, g_inv, qq, 0);
    perm_free(g_inv);
  } else {
    ds1 = block_dshare_new_offline(1, g, qq, 0);
    ds2 = block_dshare_new_offline(bs, g, qq, 0);
  }
  perm_free(g);

  writeuint(sizeof(m), m, f1);
  writeuint(sizeof(m), m, f2);
  writeuint(sizeof(n), n, f1);
  writeuint(sizeof(n), n, f2);
  writeuint(sizeof(bs), bs, f1);
  writeuint(sizeof(bs), bs, f2);
  precomp_write_seed(f1, n*(1+bs), qq, init1);
  precomp_write_seed(f2, n*(1+bs), qq, init2);
  precomp_write_pa(f1, ds1->p1p, qq);
  precomp_write_pa(f2, ds1->p2p, qq);
  precomp_write_pa(f1, ds1->b1, qq);
  precomp_write_pa(f2, ds1->b2, qq);
  precomp_write_pa(f1, ds2->p1p, qq);
  precomp_write_pa(f2, ds2->p2p, qq);
  precomp_write_pa(f1, ds2->b1, qq);
  precomp_write_pa(f2, ds2->b2, qq);

  MT_free(m0);
  dshare_free(ds1);
  dshare_free(ds2);

  fclose(f0);
  fclose(f1);
  fclose(f2);

  free(fname0);
  free(fname1);
  free(fname2);
}
#define block_dshare_precomp(bs, m, n, q, inverse, fname) DS_tables_precomp(bs, m, n, q, inverse, fname)
#define dshare_precomp(m, n, q, inverse, fname) block_dshare_precomp(1, m, n, q, inverse, fname)


void DS_tables_free(DS_tables T)
{
  if (T == NULL) return;
  if (_party >  2) return;
  if (_party < 0) return;
  precomp_free(T->PRG);
  precomp_free(T->pp_1);
  precomp_free(T->b_1);
  precomp_free(T->pp_2);
  precomp_free(T->b_2);
  if (T->map != NULL) mymunmap(T->map);
  free(T);
}

DS_tables DS_tables_read(char *fname)
{
  NEWT(DS_tables, T);

  if (_party <= 0 || _party > 2) {
    T->PRG = T->pp_1 = T->b_1 = T->pp_2 = T->b_2 = NULL;
    T->map = NULL;
    return T;
  }

  char *fname2 = precomp_fname(fname, _party);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  int m = getuint(p, 0, sizeof(int)); p += sizeof(int);
  T->n = getuint(p, 0, sizeof(int)); p += sizeof(int);
  T->bs = getuint(p, 0, sizeof(int)); p += sizeof(int);
  T->PRG = precomp_read(&p);
  T->pp_1 = precomp_read(&p);
  T->b_1 = precomp_read(&p);
  T->pp_2 = precomp_read(&p);
  T->b_2 = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}

void ds_tbl_list_free(ds_tbl_list list)
{
  ds_tbl_list next;
  while (list != NULL) {
    next = list->next;
    DS_tables_free(list->tbl);
    free(list);
    list = next;
  }
}

void precomp_free_ds(void) {
  for (int i=0; i<_opt.channels; i++) {
    if (PRE_DS_tbl[i] != NULL) ds_tbl_list_free(PRE_DS_tbl[i]);
  }
}

///////////////////////////////////////////////////////////////////
// retrieved from precomputed table
// bs is block size of data. good if <= block size of table tbl
////////////////////////////////////////////////////////////////////////
static void block_dshare_new_precomp(int bs, DS_tables tbl, int n, share_t q_x, share_t q_sigma, dshare *ds1_, dshare *ds2_)
{
  if (_party >  2) return;
  PRE_DS_count[0] += 1; // channel?
  if (_party <= 0) {
    dshare ds1 = dshare_new_party0(n, q_sigma);
    dshare ds2 = block_dshare_new_party0(bs, n, q_x);
    *ds1_ = ds1;
    *ds2_ = ds2;
    return;
  }

  /////////////////// Random permutation
  NEWT(dshare, ds1);
  int k_sigma = blog(q_sigma-1)+1;
  ds1->n = n;
  ds1->q = q_sigma;
  ds1->bs = 1;

  ds1->g = perm_random(tbl->PRG->u.seed.r, n);
  ds1->gp = pa_new(n, k_sigma);
  pa_iter itr;
  itr = pa_iter_new(ds1->gp);
  for (int i=0; i<n; i++) {
    pa_iter_set(itr, precomp_get(tbl->pp_1) % q_sigma);
  }
  pa_iter_flush(itr);
  ds1->a = pa_new(n, k_sigma);
  itr = pa_iter_new(ds1->a);
  for (int i=0; i<n; i++) {
    pa_iter_set(itr, precomp_get(tbl->PRG) % q_sigma);
  }
  pa_iter_flush(itr);
  ds1->b = pa_new(n, k_sigma);
  itr = pa_iter_new(ds1->b);
  for (int i=0; i<n; i++) {
    pa_iter_set(itr, precomp_get(tbl->b_1) % q_sigma);
  }
  pa_iter_flush(itr);

  /////////////////// Random numbers to add to values
  NEWT(dshare, ds2);
  int k_x = blog(q_x-1)+1;
  ds2->n = n;
  ds2->q = q_x;
  ds2->bs = bs;

  ds2->g = perm_random(tbl->PRG->u.seed.r, n);
  ds2->gp = pa_new(n, k_sigma);
  itr = pa_iter_new(ds2->gp);
  for (int i=0; i<n; i++) {
    pa_iter_set(itr, precomp_get(tbl->pp_2) % q_sigma);
  }
  pa_iter_flush(itr);
  share_t *ptmp;
  NEWA(ptmp, share_t, tbl->bs);
  ds2->a = pa_new(n*bs, k_x);
  itr = pa_iter_new(ds2->a);
  for (int i=0; i<n; i++) {
    for (int j=0; j<tbl->bs; j++) {
      ptmp[j] = precomp_get(tbl->PRG) % q_x;
    }
    for (int j=0; j<bs; j++) {
      int j2 = j % tbl->bs; // reuse table when insufficient (not really correct)
      pa_iter_set(itr, ptmp[j2]);
    }
  }
  pa_iter_flush(itr);
  ds2->b = pa_new(n*bs, k_x);
  itr = pa_iter_new(ds2->b);
  for (int i=0; i<n; i++) {
    for (int j=0; j<tbl->bs; j++) {
      ptmp[j] = precomp_get(tbl->b_2) % q_x;
    }
    for (int j=0; j<bs; j++) {
      int j2 = j % tbl->bs; // reuse table when insufficient (not really correct)
      pa_iter_set(itr, ptmp[j2]);
    }
  }
  pa_iter_flush(itr);
  free(ptmp);
  *ds1_ = ds1;  *ds2_ = ds2;
}
#define dshare_new_precomp(tbl, n, q_x, q_sigma, ds1_, ds2_) block_dshare_new_precomp(1, tbl, n, q_x, q_sigma, ds1_, ds2_)

ds_tbl_list ds_tbl_list_insert(DS_tables tbl, int n, int bs, int inverse, ds_tbl_list head)
{
  NEWT(ds_tbl_list, list);
  list->tbl = tbl;
  list->n = n;
  list->bs = bs;
  list->inverse = inverse;
  list->count = 0;
  list->next = head;
  return list;
}

////////////////////////////////////////////////////////////////////////
// return dshare of permutation of length n
////////////////////////////////////////////////////////////////////////
DS_tables ds_tbl_list_search(ds_tbl_list list, int inverse, int n)
{
  DS_tables ans = NULL;
  while (list != NULL) {
    if (list->tbl->n == n && list->inverse == inverse) {
      ans = list->tbl;
      break;
    }
    list = list->next;
  }
  return ans;
}

////////////////////////////////////////////////////////////////////////
// return dshare of permutation of length >= n
////////////////////////////////////////////////////////////////////////
DS_tables ds_tbl_list_search2(ds_tbl_list list, int inverse, int n)
{
  DS_tables ans = NULL;
  while (list != NULL) {
    if ((list->n >= n) && (list->n < n*2) && (list->inverse == inverse)) {
      ans = list->tbl;
      break;
    }
    list = list->next;
  }
  return ans;
}


void ds_tbl_init(void)
{
  for (int i=0; i<_opt.channels; i++) {
    PRE_DS_tbl[i] = NULL;
    PRE_DS_count[i] = 0;
  }
}

void ds_tbl_read(int channel, int n, int bs, int inverse, char *fname)
{
  DS_tables tbl = DS_tables_read(fname);
  tbl->n = n; // test
  PRE_DS_tbl[channel] = ds_tbl_list_insert(tbl, n, bs, inverse, PRE_DS_tbl[channel]);
}

//////////////////////////////////////////////////////////////////////////////////////////
// embed permutation of length n into permutation of length n2 > n and use precomputed data
//////////////////////////////////////////////////////////////////////////////////////////
static share_array block_AppPerm_fwd_offline_channel(DS_tables tbl, int bs, share_array x, share_array sigma, int channel)
{
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *x;
    ans->A = NULL;
    return ans;
  }
  dshare ds1;
  dshare ds2;
  int n2 = tbl->n;
  int n = len(x) / bs;
  int k = blog(n2-1)+1;
  _ sigma2 = _const(n2, 0, 1<<k);
  for (int i=0; i<n; i++) {
    pa_set(sigma2->A, i, pa_get(sigma->A, i) % (1<<k));
  }
  for (int i=n; i<n2; i++) _setpublic(sigma2, i, i);
  _ x2 = _const(n2*bs, 0, order(x));
  _setshares(x2, 0, n*bs, x, 0);
  for (int i=n*bs; i<n2*bs; i++) _setpublic(x2, i, 0); // unnecessary
  block_dshare_new_precomp(bs, tbl, n2, order(x2), order(sigma2), &ds1, &ds2);

  _ w;
  _ rho = dshare_shuffle_channel(sigma2, ds1, channel);
  if (_party <= 0) {
      w = block_share_perm(bs, x2, share_raw(rho));
  } else {
    _ r = share_reconstruct_channel(rho, channel);
    w = block_share_perm(bs, x2, share_raw(r));
    _free(r);
  }

  _ ans0 = block_dshare_shuffle_channel(bs, w, ds2, channel);
  _ ans = _slice(ans0, 0, n*bs);

  dshare_free(ds1);
  dshare_free(ds2);
  _free(rho);
  _free(w);
  _free(sigma2);
  _free(x2);
  _free(ans0);
    
  return ans;
}

//////////////////////////////////////////////////////////////////////////////////////////
// embed permutation of length n into permutation of length n2 > n and use precomputed data
//////////////////////////////////////////////////////////////////////////////////////////
static share_array block_AppPerm_inverse_offline_channel(DS_tables tbl, int bs, share_array x, share_array sigma, int channel)
{
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *x;
    ans->A = NULL;
    return ans;
  }
  dshare ds1;
  dshare ds2;
  int n2 = tbl->n;
  int n = len(x) / bs;
  int k = blog(n2-1)+1;
  _ sigma2 = _const(n2, 0, 1<<k);
  for (int i=0; i<n; i++) {
    pa_set(sigma2->A, i, pa_get(sigma->A, i) % (1<<k));
  }
  for (int i=n; i<n2; i++) _setpublic(sigma2, i, i);
  _ x2 = _const(n2*bs, 0, order(x));
  _setshares(x2, 0, n*bs, x, 0);
  for (int i=n*bs; i<n2*bs; i++) _setpublic(x2, i, 0); // unnecessary
  block_dshare_new_precomp(bs, tbl, n2, order(x2), order(sigma2), &ds1, &ds2);

  _ rho = dshare_shuffle_channel(sigma2, ds1, channel);
  _ z = block_dshare_shuffle_channel(bs, x2, ds2, channel);
  _ r = share_reconstruct_channel(rho, channel);  // send_6 += pa_size(r->A);
  perm rho_inv = perm_inverse(share_raw(r));
  _ ans0 = block_share_perm(bs, z, rho_inv);
  _ ans = _slice(ans0, 0, n*bs);

  _free(r);
  _free(z);
  perm_free(rho_inv);
  _free(rho);
  dshare_free(ds1);
  dshare_free(ds2);
  _free(sigma2);
  _free(x2);
  _free(ans0);
    
  return ans;
}

////////////////////////////////////////////////////////////////////////////////////////////
// use precomputed table if available, otherwise compute online
////////////////////////////////////////////////////////////////////////////////////////////
static share_array block_AppPerm_fwd_channel(int bs, share_array x, share_array sigma, int channel)
{
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *x;
    ans->A = NULL;
    return ans;
  }
    int n = len(x) / bs;
    if (n != len(sigma)) {
        printf("block_AppPerm_fwd_channel: block_len(x) %d len(sigma) %d\n", n, len(sigma));
    }
    dshare ds1;
    dshare ds2;

    DS_tables tbl;

    if ((tbl = ds_tbl_list_search2(PRE_DS_tbl[channel], 0, n)) != NULL) {
      int n2 = tbl->n;
      if (n2 >= n) {
        return block_AppPerm_fwd_offline_channel(tbl, bs, x, sigma, channel);
      } else {
        block_dshare_new_precomp(bs, tbl, n, order(x), order(sigma), &ds1, &ds2);
      }
    } else {
        if (_opt.warn_precomp) printf("without DS_table n = %d\n", n);
        perm g;
        if (_party == 0) {
            g = perm_random(mt_[0][channel], n);
        } else {
            g = perm_id(n);
        }
        perm g_inv = perm_inverse(g);

        ds1 = dshare_new_channel(g, order(sigma), channel);
        ds2 = block_dshare_new_channel(bs, g_inv, order(x), channel);
        perm_free(g_inv);
        perm_free(g);
    }
    // precomputation ends here

    _ rho = dshare_shuffle_channel(sigma, ds1, channel);

    _ w;
    if (_party <= 0) {
        w = block_share_perm(bs, x, share_raw(rho));
    } else {
        _ r = share_reconstruct_channel(rho, channel);
        w = block_share_perm(bs, x, share_raw(r));
        _free(r);
    }

    _ ans = block_dshare_shuffle_channel(bs, w, ds2, channel);

    dshare_free(ds1);
    dshare_free(ds2);
    _free(rho);
    _free(w);
    
    return ans;
}

////////////////////////////////////////////////////////////////////////////////////////////
// randomly permute array elements
////////////////////////////////////////////////////////////////////////////////////////////
static share_array RandomPerm_channel(int n, int channel)
{
  if (_party >  2) {
    NEWT(_, ans);
    ans->A = NULL;
    return ans;
  }
    dshare ds1 = NULL;
    dshare ds2 = NULL;

    DS_tables tbl;

    share_t q = 1 << (blog(n-1)+1);

    if ((tbl = ds_tbl_list_search2(PRE_DS_tbl[channel], 0, n)) != NULL) {
      int n2 = tbl->n;
      if (n2 >= n) {
        printf("block_Shuffle_channel: not implemented\n");
        exit(1);
      } else {
        block_dshare_new_precomp(1, tbl, n, 2, q, &ds1, &ds2);
      }
    } else {
        if (_opt.warn_precomp) printf("without DS_table n = %d\n", n);
        perm g;
        if (_party <= 0) {
            g = perm_random(mt_[0][channel], n);
        } else {
            g = perm_id(n);
        }
        perm g_inv = perm_inverse(g);

        ds1 = dshare_new_channel(g, q, channel);
        ds2 = block_dshare_new_channel(1, g_inv, 2, channel); // actually unnecessary
        perm_free(g_inv);
        perm_free(g);
    }
    // precomputation ends here

    _ id = Perm_ID2(n, q);
    _ ans = dshare_shuffle_channel(id, ds1, channel);
    _free(id);

    dshare_free(ds1);
    dshare_free(ds2);

    return ans;
}
#define RandomPerm(n) RandomPerm_channel(n, 0)


static share_array AppPerm_fwd_xor_channel(share_array x, share_array sigma, int channel)
{
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *x;
    ans->A = NULL;
    return ans;
  }
  int n = len(x);
  if (n != len(sigma)) {
    printf("AppPerm_fwd_xor_channel: len(x) %d len(sigma) %d\n", n, len(sigma));
  }
  dshare ds1;
  dshare ds2;

  perm g;
  if (_party == 0) {
    g = perm_random(mt_[0][channel], n);
  } else {
    g = perm_id(n);
  }
  perm g_inv = perm_inverse(g);

  ds1 = dshare_new_channel(g, order(sigma), channel);
  ds2 = dshare_new_xor_channel(g_inv, order(x), channel);
  perm_free(g_inv);
  perm_free(g);

  // precomputation ends here
  //check_dshare(ds1);
  //check_dshare(ds2);

  _ w;
  _ rho = dshare_shuffle_channel(sigma, ds1, channel);
  if (_party <= 0) {
    w = share_perm(x, share_raw(rho));
  } else {
    _ r = share_reconstruct_channel(rho, channel);
    w = share_perm(x, share_raw(r));
    _free(r);
  }

  _ ans = dshare_shuffle_xor_channel(w, ds2, channel);

  dshare_free(ds1);
  dshare_free(ds2);
  _free(rho);
  _free(w);
    
  return ans;
}
#define block_AppPerm(bs, x, sigma) block_AppPerm_fwd_channel(bs, x, sigma, 0)

static void block_AppPerm_channel_(int bs, _ x, _ sigma, int channel) {
  _ ans = block_AppPerm_fwd_channel(bs, x, sigma, channel);
  _move_(x, ans);
}
#define block_AppPerm_(bs, x, sigma) block_AppPerm_channel_(bs, x, sigma, 0)

#if 1
static share_array block_AppPerm_inverse_channel_old(int bs, share_array x, _ sigma, int channel) {
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *x;
    ans->A = NULL;
    return ans;
  }
    int n = len(x) / bs;
    if (n != len(sigma)) {
        printf("block_AppPerm_inverse: block_len(x) %d len(sigma) %d\n", n, len(sigma));
    }

    //printf("AppPerm_inverse n = %d\n", n);

    dshare ds1;
    dshare ds2;

    DS_tables tbl;

    int ln = blog(n-1) + 1;
    if ((tbl = ds_tbl_list_search2(PRE_DS_tbl[channel], 1, n)) != NULL) {
    //if (tbl = ds_tbl_list_search2(PRE_DS_tbl[0], 1, n)) { // temp
      int n2 = tbl->n;
      //printf("using DSi_table n = %d n2 = %d\n", n, n2);
      if (n2 >= n) {
        return block_AppPerm_inverse_offline_channel(tbl, bs, x, sigma, channel);
      } else {
        block_dshare_new_precomp(bs, tbl, n, order(x), order(sigma), &ds1, &ds2);
      }
    } else {
        if (_opt.warn_precomp) printf("without DSi_table n = %d\n", n);
        perm g;
        if (_party == 0) {
            //g = perm_random(mt0, n);
            g = perm_random(mt_[0][channel], n);
        } else {
            g = perm_id(n);
        }

        ds1 = dshare_new_channel(g, order(sigma), channel);
        ds2 = block_dshare_new_channel(bs, g, order(x), channel);
        
        perm_free(g);
    }
    // precomputation ends here

    _ rho = dshare_shuffle_channel(sigma, ds1, channel);
    share_array z = block_dshare_shuffle_channel(bs, x, ds2, channel);
    _ r = share_reconstruct_channel(rho, channel);  // send_6 += pa_size(r->A);
    perm rho_inv = perm_inverse(share_raw(r));
    share_array ans = block_share_perm(bs, z, rho_inv);
    _free(r);
    _free(z);
    perm_free(rho_inv);
    _free(rho);
    dshare_free(ds1);
    dshare_free(ds2);

    return ans;
}
#endif
#define AppPerm_inverse_channel(x, sigma, channel) block_AppPerm_inverse_channel_new(1, x, sigma, channel)
#define block_AppInvPerm(bs, x, sigma) block_AppPerm_inverse_channel_new(bs, x, sigma, 0)
#define block_AppPerm_inverse_channel block_AppPerm_inverse_channel_new
//#define AppPerm_inverse_channel(x, sigma, channel) block_AppPerm_inverse_channel_old(1, x, sigma, channel)
//#define block_AppInvPerm(bs, x, sigma) block_AppPerm_inverse_channel_old(bs, x, sigma, 0)
//#define block_AppPerm_inverse_channel block_AppPerm_inverse_channel_old

static AppPerm_cont block_AppPerm_inverse_channel_new_phase0(int bs, share_array x, _ sigma, int channel) {
  if (_party >  2) {
    return NULL;
  }
  int n = len(x) / bs;
  if (n != len(sigma)) {
    printf("block_AppPerm_inverse: block_len(x) %d len(sigma) %d\n", n, len(sigma));
  }

  NEWT(AppPerm_cont, ac);

  NEWA(ac->dc1, struct dshare_cont, 1);
  NEWA(ac->dc2, struct dshare_cont, 1);

  dshare ds1 = NULL;
  dshare ds2 = NULL;

  DS_tables tbl;

  int n2 = n;
  _ x2 = x;
  _ sigma2 = sigma;

  int ln = blog(n-1) + 1;
  if ((tbl = ds_tbl_list_search2(PRE_DS_tbl[channel], 1, n)) != NULL) {
    n2 = tbl->n;
    if (n2 > n) { // when not in table, embed in permutation of length n2
      int k = blog(n2-1)+1;
      sigma2 = _const(n2, 0, 1<<k);
      for (int i=0; i<n; i++) {
        pa_set(sigma2->A, i, pa_get(sigma->A, i) % (1<<k));
      }
      for (int i=n; i<n2; i++) _setpublic(sigma2, i, i);
      x2 = _const(n2*bs, 0, order(x));
      _setshares(x2, 0, n*bs, x, 0);
    }
    block_dshare_new_precomp(bs, tbl, n2, order(x2), order(sigma2), &ds1, &ds2);
  } else {
    if (_opt.warn_precomp) printf("without DSi_table n = %d\n", n);
    perm g;
    if (_party == 0) {
      g = perm_random(mt_[0][channel], n);
    } else {
      g = perm_id(n);
    }
    ds1 = dshare_new_channel(g, order(sigma2), channel);
    ds2 = block_dshare_new_channel(bs, g, order(x2), channel);
        
    perm_free(g);
  }
  ac->n = n;
  ac->n2 = n2;
  ac->bs = bs;
  ac->x = x2;
  ac->sigma = sigma2;

  ac->dc1->bs = 1;
  ac->dc1->x = sigma2;
  ac->dc1->ds = ds1;

  ac->dc2->bs = bs;
  ac->dc2->x = x2;
  ac->dc2->ds = ds2;
  // precomputation ends here

  return ac;
}

static void block_AppPerm_inverse_channel_new_phase1(AppPerm_cont ac, int channel) {
  if (_party >  2) {
    return;
  }
  block_dshare_shuffle_channel_phase0(ac->dc1, channel);
  block_dshare_shuffle_channel_phase0(ac->dc2, channel);
}

static void block_AppPerm_inverse_channel_new_phase2(AppPerm_cont ac, int channel) {
  if (_party >  2) {
    return;
  }
  ac->rho = block_dshare_shuffle_channel_phase1(ac->dc1, channel);
  ac->z = block_dshare_shuffle_channel_phase1(ac->dc2, channel);
  share_reconstruct_channel_phase0(ac->rho, channel);
}

static share_array block_AppPerm_inverse_channel_new_phase3(AppPerm_cont ac, int channel) {
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *ac->x;
    ans->A = NULL;
    return ans;
  }
  int bs = ac->bs;
  int n = ac->n;
  int n2 = ac->n2;
  _ sigma2 = ac->sigma;
  _ x2 = ac->x;

  _ r = share_reconstruct_channel_phase1(ac->rho, channel);

  perm rho_inv = perm_inverse(share_raw(r));
  share_array ans = block_share_perm(bs, ac->z, rho_inv);

  if (n2 > n) {
    _ ans2 = _slice(ans, 0, n*bs);
    _move_(ans, ans2);
    _free(x2);
    _free(sigma2);
  }

  _free(r);
  _free(ac->z);
  perm_free(rho_inv);
  _free(ac->rho);
  dshare_free(ac->dc1->ds);
  dshare_free(ac->dc2->ds);
  free(ac->dc1);
  free(ac->dc2);

  return ans;
}

/**
 * @brief Applies the inverse of a block permutation to a shared array using a specified channel.
 *
 * This function computes the inverse permutation of a shared array `x` by executing
 * four sequential phases (phase0 through phase3) on the given block structure and channel.
 *
 * @param bs       The block size used to partition the shared array.
 * @param x        The input shared array to be inverse permuted.
 * @param sigma    The permutation to invert and apply.
 * @param channel  The channel identifier used across all phases of the computation.
 *
 * @return A new share_array containing the result of the inverse block permutation.
 *
 * @note The intermediate AppPerm_cont structure `ac` is allocated during phase0
 *       and freed after phase3. The caller is responsible for managing the returned array.
 */
static share_array block_AppPerm_inverse_channel_new(int bs, share_array x, _ sigma, int channel) {
  AppPerm_cont ac = block_AppPerm_inverse_channel_new_phase0(bs, x, sigma, channel);
  block_AppPerm_inverse_channel_new_phase1(ac, channel);
  block_AppPerm_inverse_channel_new_phase2(ac, channel);
  _ ans = block_AppPerm_inverse_channel_new_phase3(ac, channel);
  free(ac);
  return ans;
}

/**
 * @brief Performs batched inverse approximate permutation with channel support.
 *
 * Processes multiple permutation inversions in batches using a multi-phase
 * approach to optimize communication rounds in secure computation.
 *
 * @param batch_size Number of permutations to process in the batch.
 * @param bs Block size used for partitioning the input array.
 * @param x Array of shared input arrays, one per batch element.
 * @param sigma Array of shared permutation arrays, one per batch element.
 * @param channel Communication channel identifier used for secure computation.
 *
 * @return A share_array containing the concatenated results of all inverse
 *         permutations, with total length of `n * bs * batch_size`,
 *         where `n = len(x[0]) / bs`.
 *
 * @note The function executes in four sequential phases across all batch
 *       elements to minimize interaction rounds:
 *       - Phase 0: Initialization of each AppPerm_cont context.
 *       - Phase 1: First communication round.
 *       - Phase 2: Second communication round.
 *       - Phase 3: Finalization and result extraction.
 *
 * @note Caller is responsible for freeing the returned share_array.
 */
static share_array batched_AppPerm_inverse_channel(int batch_size, int bs, _ *x, _ *sigma, int channel) {
  AppPerm_cont *ac;
  NEWA(ac, AppPerm_cont, batch_size);
  int n = len(x[0]) / bs;
  for (int i=0; i<batch_size; i++) {
    ac[i] = block_AppPerm_inverse_channel_new_phase0(bs, x[i], sigma[i], channel);
  }
  for (int i=0; i<batch_size; i++) {
    block_AppPerm_inverse_channel_new_phase1(ac[i], channel);
  }
  for (int i=0; i<batch_size; i++) {
    block_AppPerm_inverse_channel_new_phase2(ac[i], channel);
  }
  _ ans = _const(n*bs*batch_size, 0, order(x[0]));
  for (int i=0; i<batch_size; i++) {
    _ ans_tmp = block_AppPerm_inverse_channel_new_phase3(ac[i], channel);
    _setshares(ans, i*n*bs, (i+1)*n*bs, ans_tmp, 0);
    _free(ans_tmp);
  }
  for (int i=0; i<batch_size; i++) {
    free(ac[i]);
  }
  free(ac);
  return ans;
}


static AppPerm_cont block_AppPerm_fwd_channel_new_phase0(int bs, share_array x, _ sigma, int channel) {
  if (_party >  2) {
    return NULL;
  }
  int n = len(x) / bs;
  if (n != len(sigma)) {
    printf("block_AppPerm_inverse: block_len(x) %d len(sigma) %d\n", n, len(sigma));
  }

  NEWT(AppPerm_cont, ac);

  NEWA(ac->dc1, struct dshare_cont, 1);
  NEWA(ac->dc2, struct dshare_cont, 1);

  dshare ds1 = NULL;
  dshare ds2 = NULL;

  DS_tables tbl;

  int n2 = n;
  _ x2 = x;
  _ sigma2 = sigma;

  int ln = blog(n-1) + 1;
  if ((tbl = ds_tbl_list_search2(PRE_DS_tbl[channel], 0, n)) != NULL) {
    n2 = tbl->n;
    if (n2 > n) { // when not in table, embed in permutation of length n2
      int k = blog(n2-1)+1;
      sigma2 = _const(n2, 0, 1<<k);
      for (int i=0; i<n; i++) {
        pa_set(sigma2->A, i, pa_get(sigma->A, i) % (1<<k));
      }
      for (int i=n; i<n2; i++) _setpublic(sigma2, i, i);
      x2 = _const(n2*bs, 0, order(x));
      _setshares(x2, 0, n*bs, x, 0);
    }
    block_dshare_new_precomp(bs, tbl, n2, order(x2), order(sigma2), &ds1, &ds2);
  } else {
    if (_opt.warn_precomp) printf("without DSi_table n = %d\n", n);
    perm g;
    if (_party <= 0) {
      g = perm_random(mt_[0][channel], n);
    } else {
      g = perm_id(n);
    }
    perm g_inv = perm_inverse(g);

    ds1 = dshare_new_channel(g, order(sigma2), channel);
    ds2 = block_dshare_new_channel(bs, g_inv, order(x2), channel);
        
    perm_free(g_inv);
    perm_free(g);
  }
  ac->n = n;
  ac->n2 = n2;
  ac->bs = bs;
  ac->x = x2;
  ac->sigma = sigma2;

  ac->dc1->bs = 1;
  ac->dc1->x = sigma2;
  ac->dc1->ds = ds1;

  ac->dc2->bs = bs;
  ac->dc2->x = NULL;
  ac->dc2->ds = ds2;
  // precomputation ends here

  block_dshare_shuffle_channel_phase0(ac->dc1, channel);
  return ac;
}

static void block_AppPerm_fwd_channel_new_phase1(AppPerm_cont ac, int channel) {
  if (_party >  2) {
    return;
  }
  ac->rho = block_dshare_shuffle_channel_phase1(ac->dc1, channel);
  if (_party <= 0) {
  } else {
    share_reconstruct_channel_phase0(ac->rho, channel);
  }
}

static void block_AppPerm_fwd_channel_new_phase2(AppPerm_cont ac, int channel) {
  if (_party >  2) {
    return;
  }

  if (_party <= 0) {
    ac->z = block_share_perm(ac->bs, ac->x, share_raw(ac->rho));
  } else {
    _ r = share_reconstruct_channel_phase1(ac->rho, channel);
    ac->z = block_share_perm(ac->bs, ac->x, share_raw(r));
    _free(r);
  }
  ac->dc2->x = ac->z;
  block_dshare_shuffle_channel_phase0(ac->dc2, channel);
}

static share_array block_AppPerm_fwd_channel_new_phase3(AppPerm_cont ac, int channel) {
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *ac->x;
    ans->A = NULL;
    return ans;
  }
  int bs = ac->bs;
  int n = ac->n;
  int n2 = ac->n2;
  _ sigma2 = ac->sigma;
  _ x2 = ac->x;

  _ ans = block_dshare_shuffle_channel_phase1(ac->dc2, channel);

  if (n2 > n) {
    _ ans2 = _slice(ans, 0, n*bs);
    _move_(ans, ans2);
    _free(x2);
    _free(sigma2);
  }

  _free(ac->z);
  _free(ac->rho);
  dshare_free(ac->dc1->ds);
  dshare_free(ac->dc2->ds);
  free(ac->dc1);
  free(ac->dc2);

  return ans;
}

/**
 * @brief Builds a forward block-wise application permutation using the specified channel.
 *
 * This function executes the permutation construction in multiple phases, using
 * the provided block size, input shared array, alphabet or symbol descriptor,
 * and communication channel. It allocates an intermediate continuation object,
 * runs the phase pipeline, produces the resulting shared array, and releases
 * the intermediate state before returning.
 *
 * @param bs Block size used to partition the input array.
 * @param x Input shared array to be processed.
 * @param sigma Symbol set or alphabet information associated with the input.
 * @param channel Channel identifier used for phase execution.
 * @return The resulting shared array produced by the forward block application permutation.
 */
static share_array block_AppPerm_fwd_channel_new(int bs, share_array x, _ sigma, int channel) {
  //printf("block_AppPerm_fwd_channel_new: bs %d block_len(x) %d len(sigma) %d\n", bs, len(x)/bs, len(sigma));
  AppPerm_cont ac = block_AppPerm_fwd_channel_new_phase0(bs, x, sigma, channel);
  block_AppPerm_fwd_channel_new_phase1(ac, channel);
  block_AppPerm_fwd_channel_new_phase2(ac, channel);
  _ ans = block_AppPerm_fwd_channel_new_phase3(ac, channel);
  free(ac);
  return ans;
}
#define block_AppPerm_fwd(bs, x, sigma) block_AppPerm_fwd_channel_new(bs, x, sigma, 0)
#define AppPerm_fwd_channel(x, sigma, channel) block_AppPerm_fwd_channel_new(1, x, sigma, channel)
#define AppPerm_fwd(x, sigma) AppPerm_fwd_channel(x, sigma, 0)

/**
 * @brief Applies batched block-wise permutation forwarding on a selected channel.
 *
 * This function processes a batch of inputs (`x`) and corresponding permutation
 * descriptors (`sigma`) using a 4-phase block permutation pipeline:
 * phase0 initialization, phase1/phase2 communication or preprocessing steps,
 * and phase3 output reconstruction.
 *
 * For each batch element:
 * - Interprets input length as `n * bs`, where `n = len(x[0]) / bs`.
 * - Runs `block_AppPerm_fwd_channel_new_phase0/1/2/3` for the specified channel.
 * - Writes the resulting shares into a contiguous output buffer at the batch offset.
 *
 * @param batch_size Number of batch elements to process.
 * @param bs Block size used by the block permutation routine.
 * @param x Array of input share arrays, one per batch element.
 * @param sigma Array of permutation descriptors, one per batch element.
 * @param channel Channel index to use for channel-specific forwarding.
 *
 * @return A newly allocated share array containing concatenated results for all
 *         batch elements, with total length `batch_size * n * bs` and order
 *         matching `x[0]`.
 *
 * @note Assumes all batch entries are shape-compatible with `x[0]` and `bs`.
 * @note Intermediate per-batch contexts are allocated internally and freed
 *       before returning.
 */
static share_array batched_AppPerm_fwd_channel(int batch_size, int bs, _ *x, _ *sigma, int channel) {
  AppPerm_cont *ac;
  NEWA(ac, AppPerm_cont, batch_size);
  int n = len(x[0]) / bs;
  for (int i=0; i<batch_size; i++) {
    ac[i] = block_AppPerm_fwd_channel_new_phase0(bs, x[i], sigma[i], channel);
  }
  for (int i=0; i<batch_size; i++) {
    block_AppPerm_fwd_channel_new_phase1(ac[i], channel);
  }
  for (int i=0; i<batch_size; i++) {
    block_AppPerm_fwd_channel_new_phase2(ac[i], channel);
  }
  _ ans = _const(n*bs*batch_size, 0, order(x[0]));
  for (int i=0; i<batch_size; i++) {
    _ ans_tmp = block_AppPerm_fwd_channel_new_phase3(ac[i], channel);
    _setshares(ans, i*n*bs, (i+1)*n*bs, ans_tmp, 0);
    _free(ans_tmp);
  }
  for (int i=0; i<batch_size; i++) {
    free(ac[i]);
  }
  free(ac);
  return ans;
}


static share_array AppPerm_inverse_xor_channel(share_array x, _ sigma, int channel) {
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *x;
    ans->A = NULL;
    return ans;
  }
  int n = len(x);
  if (n != len(sigma)) {
    printf("AppPerm_inverse_xor: len(x) %d len(sigma) %d\n", n, len(sigma));
  }

  dshare ds1;
  dshare ds2;

  perm g;
  if (_party == 0) {
    g = perm_random(mt_[0][channel], n);
  } else {
    g = perm_id(n);
  }

  ds1 = dshare_new_channel(g, order(sigma), channel);
  ds2 = dshare_new_xor_channel(g, order(x), channel);
        
  perm_free(g);
  // precomputation ends here

  _ rho = dshare_shuffle_channel(sigma, ds1, channel);
  share_array z = dshare_shuffle_xor_channel(x, ds2, channel);
  _ r = share_reconstruct_channel(rho, channel);  // send_6 += pa_size(r->A);
  perm rho_inv = perm_inverse(share_raw(r));
  share_array ans = share_perm(z, rho_inv);
  _free(r);
  _free(z);
  perm_free(rho_inv);
  _free(rho);
  dshare_free(ds1);
  dshare_free(ds2);

  return ans;
}



static void block_AppInvPerm_channel_(int bs, share_array x, _ sigma, int channel) {
  share_array ans = block_AppPerm_inverse_channel(bs, x, sigma, channel);
  _move_(x, ans);
}
#define block_AppInvPerm_(bs, x, sigma) block_AppInvPerm_channel_(bs, x, sigma, 0)

///////////////////////////////////////////////////////////////////////////////////
// main computation online (bits version)
///////////////////////////////////////////////////////////////////////////////////
static _bits AppPerm_new_bits_channel(_bits x, _ sigma, int inverse, int channel)
{
  if (_party >  2) return NULL;
  dshare ds;
  int d = x->d;
  int n = len(x->a[0]);
  if (n != len(sigma)) {
    printf("AppPerm: len(x) = %d len(sigma) = %d", len(x->a[0]), len(sigma));
  }
  perm g;
  if (_party == 0) {
    g = perm_random(mt_[0][channel], n);
  } else {
    g = perm_id(n);
  }

  NEWT(_bits, z);
  NEWA(z->a, _, d);
  z->d = d;
  NEWT(_bits, w);
  NEWA(w->a, _, d);
  w->d = d;

  ds = dshare_new_channel(g, order(sigma), channel);
  _ rho = dshare_shuffle_channel(sigma, ds, channel);
  dshare_free(ds);

  if (inverse) {
    dshare ds_x = dshare_new2_channel(g, order(x->a[0]), channel);
    for (int i=0; i<d; i++) {
      dshare_correlated_random_channel(ds_x, channel);
      z->a[i] = dshare_shuffle_channel(x->a[i], ds_x, channel);
      dshare_free3(ds_x);
    }
    dshare_free2(ds_x);
  } else {
    if (_party <= 0) {
      for (int i=0; i<d; i++) {
        w->a[i] = share_perm(x->a[i], share_raw(rho));
      }
    } else {
      _ r = _reconstruct_channel(rho, channel);
      for (int i=0; i<d; i++) {
        w->a[i] = share_perm(x->a[i], share_raw(r));
      }
      _free(r);
    }
  }

  NEWT(_bits, ans);
  NEWA(ans->a, _, d);
  ans->d = d;
  if (inverse == 0) {
    perm g_inv = perm_inverse(g);
    ds = dshare_new2_channel(g_inv, order(w->a[0]), channel);

    for (int i=0; i<d; i++) {
      dshare_correlated_random_channel(ds, channel);
      ans->a[i] = dshare_shuffle_channel(w->a[i], ds, channel);
      dshare_free3(ds);
    }
    dshare_free2(ds);

    perm_free(g_inv);

  } else {
    _ r = _reconstruct_channel(rho, channel);
    perm rho_inv = perm_inverse(share_raw(r));
    for (int i=0; i<d; i++) {
      ans->a[i] = share_perm(z->a[i], rho_inv);
    }
    _free(r);
    perm_free(rho_inv);
  }

  perm_free(g);
  _free(rho);
  if (inverse == 0) _free_bits(w);
  free(z);

  return ans;
}
#define AppPerm_new_bits(x, sigma, inverse) AppPerm_new_bits_channel(x, sigma, inverse, 0)

// TODO: the following function is the same as bits_to_block_share in share_bits.h
_ Bits_to_block(_bits x)
{
  int n = len(x->a[0]);
  int d = x->d;
  _ ans = share_const_type2(n*d, 0, order(x->a[0]), x->a[0]->type, x->a[0]->A->type);
  for (int j=0; j<d; j++) {
    _ b = x->a[j];
    for (int i=0; i<n; i++) {
      _setshare(ans, i*d+j, b, i);
    }
  }
  return ans;
}

_bits block_to_Bits(int bs, _ b)
{
  int n = len(b) / bs;
  NEWT(_bits, ans);
  NEWA(ans->a, _, bs);
  ans->d = bs;
  for (int j=0; j<bs; j++) {
    ans->a[j] = share_const_type2(n, 0, order(b), b->type, b->A->type);
    for (int i=0; i<n; i++) {
      _setshare(ans->a[j], i, b, i*bs+j);
    }
  }
  return ans;
}

typedef struct {
  int m; // number of grouped shares
  int n; // number of elements in each share
  int *d; // depth of original share (Bits)
  share_t *org_q; // original modulus

  _ B; // grouped data
  int bs;
  share_t q; // maximum original modulus
}* _block;

/**
 * @brief Converts multiple _bits structures into a single _block structure.
 *
 * This function takes an array of _bits structures and combines them into
 * a unified _block representation, computing the necessary metadata such as
 * depths, block sizes, and share orders.
 *
 * @param m   The number of _bits structures in the array.
 * @param x   An array of _bits pointers of length m to be combined into a block.
 *
 * @return    A newly allocated _block structure containing:
 *            - m:     Number of input _bits structures.
 *            - n:     Length of the first _bits structure.
 *            - d:     Array of depths for each _bits structure.
 *            - bs:    Total sum of all depths.
 *            - org_q: Array of share orders for each depth layer.
 *            - q:     Maximum share order across all depth layers.
 *            - B:     Combined block data as a packed array.
 *
 * @note The caller is responsible for freeing the returned _block structure.
 */
_block multi_Bits_to_block(int m, _bits *x)
{
  NEWT(_block, B);
  B->m = m;
  int n = len_bits(x[0]);
  B->n = n;
  NEWA(B->d, int, m);
  int block_size = 0;
  int bs = 0;
  for (int i=0; i<m; i++) {
    B->d[i] = depth_bits(x[i]);
    bs += B->d[i];
  }
  B->bs = bs;
  NEWA(B->org_q, share_t, bs);

  int p = 0;
  share_t q = 0;
  for (int i=0; i<m; i++) {
    for (int d=0; d<depth_bits(x[i]); d++) {
      share_t q_tmp = order(x[i]->a[d]);
      if (q_tmp > q) q = q_tmp;
      B->org_q[p++] = q_tmp;
    }
  }
  B->q = q;

  B->B = _const(n * bs, 0, q);
  pa_iter itr_B = pa_iter_new(B->B->A);
  pa_iter *itr;
  NEWA(itr, pa_iter, bs);
  p = 0;
  for (int i=0; i<m; i++) {
    for (int d=0; d<depth_bits(x[i]); d++) {
      itr[p++] = pa_iter_new(x[i]->a[d]->A);
    }
  }
  for (int j=0; j<n; j++) {
    for (int p=0; p<bs; p++) {
      pa_iter_set(itr_B, pa_iter_get(itr[p]));
    }
  }
  pa_iter_flush(itr_B);
  for (int i=0; i<bs; i++) pa_iter_free(itr[i]);
  free(itr);

  return B;
}

/**
 * @brief Converts a packed block representation into an array of per-component bit vectors.
 *
 * This function expands the values stored in @p B into `m` independent `_bits` objects,
 * where `m = B->m`. For each component `i`, it allocates a `_bits` instance with depth
 * `B->d[i]`, initializes each layer with modulus information from `B->org_q`, and then
 * streams all `n = B->n` entries from `B->B->A` into the corresponding output iterators.
 *
 * The input stream is consumed in component/depth order for every column `j`:
 * component `0` depth `0..d[0)-1`, component `1` depth `0..d[1)-1`, ..., up to `m-1`.
 * Each fetched value is reduced modulo the target layer order before being stored.
 *
 * @param B Input block descriptor containing dimensions, depth layout, source packed array,
 *          and per-layer original moduli.
 * @return Newly allocated array of `_bits` of length `B->m`. Each element and its internal
 *         arrays are heap-allocated.
 *
 * @note Ownership of the returned structure is transferred to the caller, who must free it
 *       (including nested allocations) when no longer needed.
 * @note This routine assumes internal consistency of `B` (e.g., total depth sum equals `B->bs`).
 * @warning On allocation failure, the function prints an error message and terminates the process.
 */
_bits* block_to_multi_Bits(_block B)
{
  int m = B->m;
  int n = B->n;
  int bs = B->bs;
  _bits* ans;
  NEWA(ans, _bits, m);

  pa_iter itr_B = pa_iter_new(B->B->A);
  pa_iter *itr;
  NEWA(itr, pa_iter, bs);

  int p;
  p = 0;
  for (int i=0; i<m; i++) {
    int depth = B->d[i];
    ans[i] = (_bits)malloc(sizeof(*ans[i]));
    if (ans[i] == NULL) {
      printf("malloc failed\n");
      exit(1);
    }
    NEWA(ans[i]->a, _, depth);
    ans[i]->d = depth;
    for (int d=0; d<depth; d++) {
      ans[i]->a[d] = _const(n, 0, B->org_q[p]);
      itr[p] = pa_iter_new(ans[i]->a[d]->A);
      p++;
    }
  }

  for (int j=0; j<n; j++) {
    p = 0;
    for (int i=0; i<m; i++) {
      for (int d=0; d<B->d[i]; d++) {
        share_t x = pa_iter_get(itr_B);
        share_t q = order(ans[i]->a[d]);
        pa_iter_set(itr[p++], x % q);
      }
    }
  }
  pa_iter_free(itr_B);
  for (int i=0; i<bs; i++) pa_iter_flush(itr[i]);
  free(itr);

  return ans;
}

void block_free(_block B)
{
  _free(B->B);
  free(B->d);
  free(B->org_q);
  free(B);
}


/**
 * @brief Applies a forward or inverse permutation (optionally batched) to an array of block shares, using a specified channel.
 *
 * This function:
 * 1. Converts each input block share into bit-level representation.
 * 2. Packs all bit shares into a single multi-share block.
 * 3. Applies either forward or inverse AppPerm on that block:
 *    - single permutation path when @p n_sigma == 1
 *    - batched permutation path when @p n_sigma > 1
 * 4. Converts the transformed block back into per-element block shares.
 *
 * @param l
 *   Number of input shares in @p x and @p bs.
 * @param x
 *   Input array of shares to transform.
 * @param bs
 *   Per-share block-size metadata corresponding to @p x.
 * @param n_sigma
 *   Number of permutations in @p sigma. Uses single-path when 1, batched path otherwise.
 * @param sigma
 *   Permutation descriptor(s) used by the AppPerm routines.
 * @param inverse
 *   Direction selector: non-zero for inverse permutation, zero for forward permutation.
 * @param channel
 *   Channel identifier passed to channel-specific AppPerm routines.
 *
 * @return
 *   Newly allocated array of length @p l containing transformed shares.
 *   Caller is responsible for freeing the returned shares/array according to project memory conventions.
 */
_ *multi_AppPerm_channel(int l, _ *x, int *bs, int n_sigma, _ *sigma, int inverse, int channel)
{
  _bits *bits;
  NEWA(bits, _bits, l);
  for (int i=0; i<l; i++) {
    bits[i] = block_share_to_bits(x[i], bs[i]);
  }
  _block B = multi_Bits_to_block(l, bits);
  for (int i=0; i<l; i++) {
    _free_bits(bits[i]);
  }

  _ tmp;
  if (inverse) {
    if (n_sigma == 1) {
      tmp = block_AppPerm_inverse_channel(B->bs, B->B, sigma[0], channel);
    } else {
      //_ * B_array = deserialize_share_array(B->B, n_sigma);
      _* B_array;
      NEWA(B_array, _, n_sigma);
      int pos = 0;
      for (int i=0; i<n_sigma; i++) {
        B_array[i] = _slice(B->B, pos, pos + len(sigma[i]) * B->bs);
        pos += len(sigma[i]) * B->bs;
        //printf("i=%d ", i); _print(B_array[i]);
      }
      tmp = batched_AppPerm_inverse_channel(n_sigma, B->bs, B_array, sigma, channel);
      //_print(tmp);
      //for (int i=0; i<l; i++) _free(B_array[i]);
      for (int i=0; i<n_sigma; i++) _free(B_array[i]);
      free(B_array);
    }
  } else {
    if (n_sigma == 1) {
      tmp = block_AppPerm_fwd_channel(B->bs, B->B, sigma[0], channel);
    } else {
      //_* B_array = deserialize_share_array(B->B, n_sigma);
      _* B_array;
      NEWA(B_array, _, n_sigma);
      int pos = 0;
      for (int i=0; i<n_sigma; i++) {
        B_array[i] = _slice(B->B, pos, pos + len(sigma[i]) * B->bs);
        pos += len(sigma[i]) * B->bs;
      }
      tmp = batched_AppPerm_fwd_channel(n_sigma, B->bs, B_array, sigma, channel);
      for (int i=0; i<n_sigma; i++) _free(B_array[i]);
      free(B_array);
    }
  }
  _free(B->B);
  B->B = tmp;

  _bits *bits2 = block_to_multi_Bits(B);
  

  _ *ans;
  NEWA(ans, _, l);
  for (int i=0; i<l; i++) {
    ans[i] = bits_to_block_share(bits2[i]);
    _free_bits(bits2[i]);
  }
  free(bits);
  free(bits2);
  block_free(B);

  return ans;
}
#define multi_AppPerm(l, x, bs, n_sigma, sigma) multi_AppPerm_channel(l, x, bs, n_sigma, sigma, 0, 0)
#define multi_AppInvPerm(l, x, bs, n_sigma, sigma) multi_AppPerm_channel(l, x, bs, n_sigma, sigma, 1, 0)


/**
 * @brief Applies a secret-shared conditional selection to all values in a block.
 *
 * Creates a deep copy of the input block metadata and arrays (`d`, `org_q`),
 * then conditionally selects each element of the block payload `B->B` using
 * the shared condition `c` via `IfThen_b`.
 *
 * The scalar condition share is expanded to a share array of length `bs`
 * with `extend_share_array`, so the same condition is applied across the
 * entire block payload.
 *
 * @param c Secret-shared boolean/condition used for selection.
 * @param B Pointer to the source block.
 * @return _block Newly allocated block containing copied structure fields and
 *         conditionally selected payload data.
 *
 * @note The returned block is heap-allocated and must be released by the caller.
 */
_block IfThen_b_block(_ c, _block B)
{
  NEWT(_block, ans);
  *ans = *B;
  NEWA(ans->d, int, ans->m);
  NEWA(ans->org_q, share_t, ans->bs);
  for (int i=0; i<ans->m; i++) ans->d[i] = B->d[i];
  for (int i=0; i<ans->bs; i++) ans->org_q[i] = B->org_q[i];
  _ extended_c = extend_share_array(ans->bs, c);
  ans->B = IfThen_b(extended_c, B->B);
  _free(extended_c);
  return ans;
}

/**
 * @brief Conditionally selects between two `_block` values based on a shared condition.
 *
 * This function builds and returns a newly allocated `_block` whose metadata and
 * local arrays are initialized from `B_true`, while its channel field (`B`) is
 * selected element-wise via `IfThenElse_channel(...)` using the condition `c`.
 *
 * The condition share `c` is first converted to the arithmetic domain compatible
 * with `B_true->q` using `B2A`, then expanded to the block size with
 * `extend_share_array`. Temporary converted/expanded shares are freed internally
 * before returning.
 *
 * @param c       Shared boolean/condition value controlling the selection.
 * @param B_true  Block used when `c` evaluates to true (also provides shape/metadata).
 * @param B_false Block used when `c` evaluates to false.
 * @return Newly allocated `_block` containing the conditional selection result.
 *
 * @note The returned `_block` owns newly allocated internal arrays and should be
 *       released by the caller using the project’s corresponding deallocation routine.
 * @warning `B_true` and `B_false` are expected to be structurally compatible
 *          (e.g., same dimensions/channel layout).
 */
_block IfThenElse_b_block(_ c, _block B_true, _block B_false)
{
  NEWT(_block, ans);
  *ans = *B_true;
  NEWA(ans->d, int, ans->m);
  NEWA(ans->org_q, share_t, ans->bs);
  for (int i=0; i<ans->m; i++) ans->d[i] = B_true->d[i];
  for (int i=0; i<ans->bs; i++) ans->org_q[i] = B_true->org_q[i];
  _ c2 = B2A(c, B_true->q); // in share_bits.h
  _ extended_c = extend_share_array(ans->bs, c2);
  ans->B = IfThenElse_channel(extended_c, B_true->B, B_false->B, 0);
  _free(extended_c);
  _free(c2);
  return ans;
}

static _bits block_AppPerm_bits_bd_channel(int bs, _bits x, _ sigma, int inverse, int channel) {
  int d = x->d;
  if (len(x->a[0]) / bs != len(sigma)) {
    printf("block_AppPerm_bits_bd_channel len(x->a[0]) / bs = %d, len(sigma) = %d\n", len(x->a[0]) / bs, len(sigma));
    exit(1);
  }
  _ xb = Bits_to_block(x);
  _ ans_b;
  if (inverse) {
    ans_b = block_AppPerm_inverse_channel(d * bs, xb, sigma, channel);
  }
  else {
    ans_b = block_AppPerm_fwd_channel(d * bs, xb, sigma, channel);
  }
  _bits ans = block_to_Bits(d, ans_b);
  _free(ans_b);
  _free(xb);

  return ans;
}
#define block_AppPerm_bits_channel(bs, a, sigma, channel) block_AppPerm_bits_bd_channel(bs, a, sigma, 0, channel)
#define block_AppInvPerm_bits_channel(bs, a, sigma, channel)  block_AppPerm_bits_bd_channel(bs, a, sigma, 1, channel)
#define block_AppPerm_bits(bs, a, sigma)  block_AppPerm_bits_bd_channel(bs, a, sigma, 0, 0)
#define block_AppInvPerm_bits(bs, a, sigma) block_AppPerm_bits_bd_channel(bs, a, sigma, 1, 0)

static void block_AppPerm_bits_bd_channel_(int bs, _bits x, _ sigma, int inverse, int channel) {
  _bits res = block_AppPerm_bits_bd_channel(bs, x, sigma, inverse, channel);
  _move_bits(x, res);
}
#define block_AppPerm_bits_channel_(bs, a, sigma, channel) block_AppPerm_bits_bd_channel_(bs, a, sigma, 0, channel)
#define block_AppInvPerm_bits_channel_(bs, a, sigma, channel)  block_AppPerm_bits_bd_channel_(bs, a, sigma, 1, channel)
#define block_AppPerm_bits_(bs, a, sigma)  block_AppPerm_bits_bd_channel_(bs, a, sigma, 0, 0)
#define block_AppInvPerm_bits_(bs, a, sigma) block_AppPerm_bits_bd_channel_(bs, a, sigma, 1, 0)


/**
 * @brief Applies a permutation to a bit-vector block on a specific channel.
 *
 * Converts the input bit-vector representation to a block representation,
 * applies either the forward or inverse permutation for the given channel,
 * then converts the result back to bit-vector form.
 *
 * @param x        Input bit-vector object; its dimension (`x->d`) is used.
 * @param sigma    Permutation descriptor/index used by the block permutation routines.
 * @param inverse  If non-zero, applies the inverse permutation; otherwise applies forward permutation.
 * @param channel  Channel identifier that selects which channel-specific permutation to apply.
 *
 * @return A newly allocated `_bits` containing the permuted result.
 *
 * @note Temporary block objects created during conversion are released before returning.
 */
static _bits AppPerm_bits_bd_channel(_bits x, _ sigma, int inverse, int channel)
{
  int d = x->d;
  _ xb = Bits_to_block(x);
  _ ans_b;
  if (inverse) {
    ans_b = block_AppPerm_inverse_channel(d, xb, sigma, channel);
  } else {
    ans_b = block_AppPerm_fwd_channel(d, xb, sigma, channel);
  }
  _bits ans = block_to_Bits(d, ans_b);
  _free(ans_b);
  _free(xb);

  return ans;
}
#define AppPerm_bits_channel(a, sigma, channel) AppPerm_bits_bd_channel(a, sigma, 0, channel)
#define AppInvPerm_bits_channel(a, sigma, channel) AppPerm_bits_bd_channel(a, sigma, 1, channel)
#define AppPerm_bits(a, sigma) AppPerm_bits_channel(a, sigma, 0)
#define AppInvPerm_bits(a, sigma) AppInvPerm_bits_channel(a, sigma, 0)

static _ AppPerm_channel(_ x, _ sigma, int channel)
{
  if (_party >  2) return NULL;
  _ ans = block_AppPerm_fwd_channel(1, x, sigma, channel);
  return ans;
}

static _ AppPerm(_ x, _ sigma)
{
  return AppPerm_fwd_channel(x, sigma, 0);
}

static void AppPerm_channel_(_ x, _ sigma, int channel)
{
  _ ans = AppPerm_fwd_channel(x, sigma, channel);
  _move_(x, ans);
}
#define AppPerm_(x, sigma) AppPerm_channel_(x, sigma, 0)

static _ AppInvPerm_channel(_ x, _ sigma, int channel)
{
  _ ans = block_AppPerm_inverse_channel(1, x, sigma, channel);
  return ans;
}
#define AppInvPerm(x, sigma) AppInvPerm_channel(x, sigma, 0)


static void AppInvPerm_channel_(_ x, _ sigma, int channel)
{
  _ ans = AppPerm_inverse_channel(x, sigma, channel);
  _move_(x, ans);
}
#define AppInvPerm_(x, sigma) AppInvPerm_channel_(x, sigma, 0)

_pair AppInvPermP(_pair x, _ sigma)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  _ X[2] = {x.x, x.y};
  _ ZX = _zip(2, X);
  _ z = block_AppInvPerm(2, ZX, sigma);
  _ *Z = _unzip(z, 2);
  _pair ans = {Z[0], Z[1]};
  free(Z);
  _free(z);
  _free(ZX);
  return ans;
} 

_pair AppPermP(_pair x, _ sigma)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  _ X[2] = {x.x, x.y};
  _ ZX = _zip(2, X);
  _ z = block_AppPerm(2, ZX, sigma);
  _ *Z = _unzip(z, 2);
  _pair ans = {Z[0], Z[1]};
  free(Z);
  _free(z);
  _free(ZX);
  return ans;
} 

_* AppInvPerm_list(_ sigma, ...)
{
  if (_party >  2) {
    _* ans = NULL;
    return ans;
  }
  va_list ap;
  int k = 0; // number of arrays in the list
  _ ptr;
  va_start(ap, sigma);
  while (1) {
    ptr = va_arg(ap, _);
    if (ptr == NULL) break;
    k++;
  }
  va_end(ap);
  _* x;
  NEWA(x, _, k);
  va_start(ap, sigma);
  for (int i=0; i<k; i++) {
    x[i] = va_arg(ap, _);
  }
  va_end(ap);

  AppPerm_cont *ac;
  NEWA(ac, AppPerm_cont, k);

  for (int i=0; i<k; i++) {
    ac[i] = block_AppPerm_inverse_channel_new_phase0(1, x[i], sigma, 0);
  }
  for (int i=0; i<k; i++) {
    block_AppPerm_inverse_channel_new_phase1(ac[i], 0);
  }
  for (int i=0; i<k; i++) {
    block_AppPerm_inverse_channel_new_phase2(ac[i], 0);
  }
  _ *Z;
  NEWA(Z, _, k+1);
  Z[k] = NULL;
  for (int i=0; i<k; i++) {
    Z[i] = block_AppPerm_inverse_channel_new_phase3(ac[i], 0);
    free(ac[i]);
  }
  free(ac);
  free(x);

  return Z;
} 

_* AppPerm_list(_ sigma, ...)
{
  if (_party >  2) {
    _* ans = NULL;
    return ans;
  }
  va_list ap;
  int k = 0; // number of arrays in the list
  _ ptr;
  va_start(ap, sigma);
  while (1) {
    ptr = va_arg(ap, _);
    if (ptr == NULL) break;
    k++;
  }
  va_end(ap);
  _* x;
  NEWA(x, _, k);
  va_start(ap, sigma);
  for (int i=0; i<k; i++) {
    x[i] = va_arg(ap, _);
  }
  va_end(ap);

  AppPerm_cont *ac;
  NEWA(ac, AppPerm_cont, k);

  for (int i=0; i<k; i++) {
    ac[i] = block_AppPerm_fwd_channel_new_phase0(1, x[i], sigma, 0);
  }
  for (int i=0; i<k; i++) {
    block_AppPerm_fwd_channel_new_phase1(ac[i], 0);
  }
  for (int i=0; i<k; i++) {
    block_AppPerm_fwd_channel_new_phase2(ac[i], 0);
  }
  _ *Z;
  NEWA(Z, _, k+1);
  Z[k] = NULL;
  for (int i=0; i<k; i++) {
    Z[i] = block_AppPerm_fwd_channel_new_phase3(ac[i], 0);
    free(ac[i]);
  }
  free(ac);
  free(x);

  return Z;
} 

static _bits AppPerm_bits_online_channel(_bits a, _ sigma, int channel)
{
  if (_party >  2) return NULL;
  return AppPerm_new_bits_channel(a, sigma, 0, channel);
}
#define AppPerm_bits_online(a, sigma) AppPerm_bits_online_channel(a, sigma, 0)

static _bits AppInvPerm_bits_online_channel(_bits a, _ sigma, int channel)
{
  if (_party >  2) return NULL;
  return AppPerm_new_bits_channel(a, sigma, 1, channel);
}
#define AppInvPerm_bits_online(a, sigma) AppInvPerm_bits_online_channel(a, sigma, 0)

#ifndef AppPerm_bits_channel
 #define AppPerm_bits_channel(a, sigma, channel) AppPerm_bits_bd_channel(a, sigma, 0, channel)
 static _bits AppPerm_bits_bd_channel(_bits x, _ sigma, int inverse, int channel);
#endif

#ifndef AppInvPerm_bits_channel
 #define AppInvPerm_bits_channel(a, sigma, channel) AppPerm_bits_bd_channel(a, sigma, 1, channel)
 static _bits AppPerm_bits_bd_channel(_bits x, _ sigma, int inverse, int channel);
#endif

void AppPerm_bits_channel_(_bits x, share_array sigma, int channel) {
    _bits tmp = AppPerm_bits_channel(x, sigma, channel);
    _move_bits(x, tmp);
}

void AppInvPerm_bits_channel_(_bits x, share_array sigma, int channel) {
    _bits tmp = AppInvPerm_bits_channel(x, sigma, channel);
    _move_bits(x, tmp);
}


_ InvPerm(_ sigma)
{
  if (_party >  2) return NULL;
  _ perm = Perm_ID(sigma);
  _ ans = AppInvPerm(perm, sigma);
  _free(perm);
  return ans;
}

void setperm(_ a)
{
  if (_party >  2) return;
  int n = len(a);
  for (int i=0; i<n; i++) {
    _setpublic(a, i, i);
  }

}

/*********************************
def select1(g):
  N = len(g)
  s00 = rshift(rank0(g), 0)
  m = sum(g) #   #ones
  s0 = vadd(s00, [m]*N)
  s1 = rshift(rank1(g), 0)
  sigma = IfThenElse(g, s1, s0)
  t0 = smul(N, vneg(g))
  t1 = vmul(Perm_ID(N), g)
  t = vadd(t0, t1)
  u = AppInvPerm(t, sigma)
  return u
*********************************/
/**
 * @brief Computes a secure selection/permutation index vector from a binary share vector.
 *
 * This function is intended for 2-party execution (`_party <= 2`). It builds
 * rank-based indices for zeros and ones in `g`, chooses between them element-wise,
 * and then applies the inverse permutation to a transformed index vector.
 *
 * High-level behavior:
 * - Computes `rank0(g)` and `rank1(g)` (right-shifted by one position),
 * - Selects per position via `IfThenElse(g, rank1, rank0)`,
 * - Builds an auxiliary vector `t[i] = i - n*g[i]`,
 * - Returns `AppInvPerm(t, sigma)` where `sigma` is the selected rank vector.
 *
 * @param g Input shared vector (expected to be binary/selector-like).
 * @return Shared vector `u` containing the resulting selection/permutation mapping,
 *         or `NULL` when `_party > 2`.
 *
 * @note Intermediate buffers are allocated and freed internally; ownership of the
 *       returned value is transferred to the caller.
 */
_ select1(_ g)
{
  if (_party >  2) return NULL;
  int n = len(g);
  _ s0 = rank0(g);
  rshift_(s0, 0);
  _ m = sum(g);
  for (int i=0; i<n; i++) {
    _addshare(s0, i, m, 0);
  }
  _ s1 = rank1(g);
  rshift_(s1, 0);
  _ sigma = IfThenElse(g, s1, s0);
  _ gneg = vneg(g);
  _ t0 = smul(n, gneg);
  _ t1 = _dup(g);
  for (int i=0; i<n; i++) {
    _mulpublic(t1, i, i);
  }
  _ t = vadd(t0, t1);
  _ u = AppInvPerm(t, sigma);

  _free(s0);
  _free(s1);
  _free(m);
  _free(sigma);
  _free(gneg);
  _free(t0);
  _free(t1);
  _free(t);

  return u;
}

/**
 * @brief Computes the selection result for zero-bits by reusing `select1` on the bitwise negation of `v`.
 *
 * This function returns `NULL` when the current party identifier (`_party`) is greater than 2.
 * Otherwise, it:
 * 1. Creates a negated copy of `v` via `vneg(v)`.
 * 2. Calls `select1` on that negated value.
 * 3. Frees the temporary negated value.
 * 4. Returns the result from `select1`.
 *
 * @param v Input value/vector to process.
 * @return Selection result pointer/object, or `NULL` when `_party > 2`.
 */
_ select0(_ v)
{
  if (_party >  2) return NULL;
  _ vn = vneg(v);
  _ ans = select1(vn);

  _free(vn);

  return ans;
}



#endif
