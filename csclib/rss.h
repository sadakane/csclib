#ifndef _RSS_H
 #define _RSS_H

#include <stdlib.h>
#include "share.h"

/****************************************************************************
 * Replicated Secret Sharing (RSS)
 * Array length is doubled
 ****************************************************************************/

int len_rss(_ a)
{
  if (a->type != SHARE_T_RSS) {
    printf("len_rss: type = %d\n", a->type);
  }
  return a->n / 2;
}


share_array share_rss_GF_new(int n, share_t q, share_t *A, share_t irr_poly)
{
  int i;
  NEWT(share_array, ans);
  int k;

  ans->type = SHARE_T_RSS;
  ans->irr_poly = irr_poly;
  ans->n = n*2; // Duplicate/replicate

  ans->q = q;
  ans->own = 0;
  k = blog(q-1)+1;

  ans->A = pa_new(n*2, k);

  if (_party < 0) {
    for (i=0; i<n; i++) {
      pa_set(ans->A, i, A[i]);
      pa_set(ans->A, i+n, 0);
    }
    return ans;
  }

  if (_party == 0) {
    packed_array A1, A2, A3;
    A1 = pa_new(n*2, k);
    A2 = pa_new(n*2, k);
    A3 = pa_new(n*2, k);
    for (i=0; i<n; i++) {
      share_t r1, r2, r3;
      pa_set(ans->A, i, A[i]);
      pa_set(ans->A, i+n, 0);
      r1 = RANDOM0(q);
      r2 = RANDOM0(q);
      if (irr_poly) {
        r3 = r1 ^ r2;
        pa_set(A1, i, A[i] ^ r1);
        pa_set(A2, i, 0    ^ r2);
        pa_set(A3, i, 0    ^ r3);
        pa_set(A1, i+n, 0  ^ r2);
        pa_set(A2, i+n, 0  ^ r3);
        pa_set(A3, i+n, A[i] ^ r1);
      } else {
        r3 = MOD(q*2 - r1 - r2);
        pa_set(A1, i, MOD(A[i] + r1));
        pa_set(A2, i, MOD(0    + r2));
        pa_set(A3, i, MOD(0    + r3));
        pa_set(A1, i+n, MOD(0  + r2));
        pa_set(A2, i+n, MOD(0  + r3));
        pa_set(A3, i+n, MOD(A[i] + r1));
      }
    }
    if (_party == 0) {
      mpc_send_pa(TO_PARTY1, A1);
      mpc_send_pa(TO_PARTY2, A2);
      mpc_send_pa(TO_PARTY3, A3);
    }

    pa_free(A1);
    pa_free(A2);
    pa_free(A3);
  } else {
    mpc_recv_pa(FROM_SERVER, ans->A);
  }

  return ans;
}
#define share_rss_new(n, q, A) share_rss_GF_new(n, q, A, 0)
#define rss_new share_rss_new

_ share_const_rss_GF(int n, share_t v, share_t q, share_t irr_poly)
{
  _ ans = share_const_type(n, 0, q, SHARE_T_RSS);
  ans->type = SHARE_T_RSS;
  ans->irr_poly = irr_poly;

  if (_party <= 1) {
    pa_iter itr = pa_iter_new(ans->A);
    for (int i=0; i<n; i++) {
      pa_iter_set(itr, v);
    }
    pa_iter_flush(itr);
  }
  if (_party == 3) {
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i+n, v);
    }
  }
  return ans;
}
#define share_const_rss(n, v, q) share_const_rss_GF(n, v, q, 0)
#define _const_rss share_const_rss



/***********************************************************************
 * Do we need to specify irr_poly?
***********************************************************************/
_ share_rss_reconstruct(_ x, share_t irr_poly)
{
  if (x->type != SHARE_T_RSS) {
    printf("share_rss_reconstruct: type = %d\n", x->type);
  }

  int n = x->n / 2;

  if (_party <= 0) {
    _ ans = _slice_raw(x, 0, n); 
    ans->type = SHARE_T_RAW;
    return ans;
  }

  _ ans, tmp1, tmp2;
  share_t q = order(x);
  if (_party == 1) {
    tmp1 = _slice_raw(x, 0, n); // x1
    mpc_send_share(TO_PARTY2, tmp1);
  }
  if (_party == 2) {
    tmp1 = _slice_raw(x, 0, n); // x2
    mpc_send_share(TO_PARTY3, tmp1);
  }
  if (_party == 3) {
    tmp1 = _slice_raw(x, 0, n); // x3
    mpc_send_share(TO_PARTY1, tmp1);
  }
  if (_party == 1) {
    tmp2 = share_const_type(n, 0, q, SHARE_T_33ADD);
    mpc_recv_share(FROM_PARTY3, tmp2);
  }
  if (_party == 2) {
    tmp2 = share_const_type(n, 0, q, SHARE_T_33ADD);
    mpc_recv_share(FROM_PARTY1, tmp2);
  }
  if (_party == 3) {
    tmp2 = share_const_type(n, 0, q, SHARE_T_33ADD);
    mpc_recv_share(FROM_PARTY2, tmp2);
  }

  ans = share_const_type(n, 0, q, SHARE_T_33ADD);
  for (int i=0; i<n; i++) {
    share_t z, x1, x2, x3;
    x1 = pa_get(x->A, i);
    x2 = pa_get(x->A, i+n);
    x3 = pa_get(tmp2->A, i);
    if (irr_poly) {
      z = x1 ^ x2 ^ x3;
    } else {
      z = MOD(x1 + x2 + x3);
    }
    pa_set(ans->A, i, z);
  }

  ans->type = SHARE_T_RAW;
  ans->irr_poly = irr_poly;
  return ans;
}
#define rss_reconstruct(x, xor) share_rss_reconstruct(x, xor)


_ vadd_rss(_ a, _ b)
{
  if (a->type != SHARE_T_RSS || b->type != SHARE_T_RSS) {
    printf("vadd_rss: type = %d, %d\n", a->type, b->type);
  }
  if (_party > 3) return NULL; // Need to check
  return vadd(a, b);
}
#define _vadd_rss vadd_rss

_ vsub_rss(_ a, _ b)
{
  if (a->type != SHARE_T_RSS || b->type != SHARE_T_RSS) {
    printf("vsub_rss: type = %d, %d\n", a->type, b->type);
  }
  if (_party > 3) return NULL; // Need to check
  return vsub(a, b);
}
#define _vsub_rss vsub_rss


_ vadd_rss_GF(_ a, _ b)
{
  if (_party > max_partyid(a)) return NULL;
  int n = a->n;
  share_t q = order(a);
  if (b->n != n || order(b) != q) {
    printf("vadd_rss_GF: len %d %d order %d %d\n", n, b->n, q, order(b));
    exit(1);
  }
  _ ans = vadd_GF(a, b);
  ans->type = a->type;
  return ans;
}


_ smul_rss(share_t s, _ a) // s is a public value
{
  if (a->type != SHARE_T_RSS) {
    printf("smul_rss: type = %d\n", a->type);
  }
  if (_party >  3) return NULL; // Need to check
  return smul(s, a);
}
#define _smul_rss smul_rss

_ smul_rss_GF(share_t s, _ a, share_t irr_poly)
{
  if (a->type != SHARE_T_RSS) {
    printf("smul_rss_GF: type = %d\n", a->type);
  }
  if (_party >  3) return NULL;
  int n = len(a);
  share_t q = order(a);
  _ ans = _const(2*n, 0, q);
  NEWITER(itr_ans, ans);
  NEWITER(itr_a, a);
  for (int i=0; i<2*n; i++) {
    share_t c;
    c = GF_mul(s, pa_iter_get(itr_a), irr_poly);
    pa_iter_set(itr_ans, c);
  }
  pa_iter_flush(itr_ans);
  pa_iter_free(itr_a);
  ans->type = SHARE_T_RSS;
  ans->irr_poly = irr_poly;
  return ans;
}

_ slice_rss(_ a, int start, int end)
{
  if (a->type != SHARE_T_RSS) {
    printf("slice_rss: type = %d\n", a->type);
  }
  if (_party >  3) return NULL; // Need to check
  int n = len_rss(a);
  int len = end - start;

  _ ans = _const_rss(len, 0, a->q);
  for (int i=0; i<len; i++) {
    pa_set(ans->A, i, pa_get(a->A, start+i));
    pa_set(ans->A, i+len, pa_get(a->A, start+i+n));
  }
  ans->irr_poly = a->irr_poly;
  return ans;
}

_s3 vmul_rss(_ a, _ b)
{
  if (a->type != SHARE_T_RSS || b->type != SHARE_T_RSS) {
    printf("vmul_rss: type = %d, %d\n", a->type, b->type);
  }
  if (_party > max_partyid(a)) return NULL; // Need to check

  int n = len_rss(a);
  if (n != len_rss(b)) {
    printf("vmul_rss_GF: len(a) = %d len(b) = %d\n", n, len_rss(b));
    exit(1);
  }
  share_t irr_poly = a->irr_poly;
  if (_party <= 0) {
    _ a2 = _slice_raw(a, 0, n);
    _ b2 = _slice_raw(b, 0, n);
    _ ans = _dup(a2);
    ans->type = SHARE_T_33ADD;
    if (irr_poly) {
      for (int i=0; i<n; i++) {
        pa_set(ans->A, i, GF_mul(pa_get(a->A,i), pa_get(b->A,i), irr_poly));
      }
    } else {
      share_t q = order(a);
      for (int i=0; i<n; i++) {
        pa_set(ans->A, i, LMUL(pa_get(a->A,i), pa_get(b->A,i), q));
      }
    }
    ans->type = SHARE_T_33ADD;
    _free(a2);
    _free(b2);
    return ans;
  }
  _ ans = share_const_type(n, 0, order(a), SHARE_T_33ADD);
  ans->irr_poly = irr_poly;
  share_t q = order(a);
  if (q != order(b)) {
    printf("vmul_rss: order = %d, %d\n", q, order(b));
  }
  for (int i=0; i<n; i++) {
    share_t a1 = pa_get(a->A, i);
    share_t a2 = pa_get(a->A, i+n);
    share_t b1 = pa_get(b->A, i);
    share_t b2 = pa_get(b->A, i+n);
    share_t z;
    if (irr_poly) {
      z = GF_mul(a1, b1, irr_poly);
      z = GFADD(z, GF_mul(a1, b2, irr_poly), irr_poly);
      z = GFADD(z, GF_mul(a2, b1, irr_poly), irr_poly);
    } else {
      z = MOD(a1*b1 + a1*b2 + a2*b1);
    }
    pa_set(ans->A, i, z);
  }
  return ans;
}
#define _vadd_rss vadd_rss

///////////////////////////////////////////////////////////////////////////
// Create RSS shares from 3-party additive shares
///////////////////////////////////////////////////////////////////////////
_ shamir3_to_rss_GF_channel(_ x, share_t irr_poly, int channel)
{
  if (x->type != SHARE_T_33ADD) {
    printf("shamir3_to_rss: type = %d\n", x->type);
  }

  _ ans;
  int n = len(x);
  share_t q = order(x);
  ans = share_const_type(n, 0, q, SHARE_T_RSS);
  ans->type = SHARE_T_RSS;
  ans->irr_poly = irr_poly;
  if (_party <= 0) {
    NEWITER(itr_ans, ans);
    NEWITER(itr_x, x);
    for (int i=0; i<n; i++) {
      pa_iter_set(itr_ans, pa_iter_get(itr_x));
    }
    pa_iter_flush(itr_ans);
    pa_iter_free(itr_x);

    return ans;
  }

  if (_party == 1) {
    NEWITER(itr_ans, ans);
    NEWITER(itr_x, x);
    for (int i=0; i<n; i++) {
      share_t r = RANDOM(mt_[TO_PARTY2][channel], q);
      share_t z;
      if (irr_poly) {
        z = pa_iter_get(itr_x) ^ r;
      } else {
        z = MOD(pa_iter_get(itr_x) + r);
      }
      pa_iter_set(itr_ans, z);
    }
    pa_iter_flush(itr_ans);
    pa_iter_free(itr_x);
    _ tmp = _slice_raw(ans, 0, n);
    mpc_send_share_channel(TO_PARTY3, tmp, channel);
    mpc_recv_share_channel(FROM_PARTY2, tmp, channel);
    NEWITER(itr_tmp, tmp);
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i+n, pa_get(tmp->A, i));
    }
    pa_iter_free(itr_tmp);
    _free(tmp);
  }
  if (_party == 3) {
    NEWITER(itr_ans, ans);
    NEWITER(itr_x, x);
    for (int i=0; i<n; i++) {
      share_t s = RANDOM(mt_[TO_PARTY2][channel], q);
      share_t z;
      if (irr_poly) {
        z = pa_iter_get(itr_x) ^ s;
      } else {
        z = MOD(pa_iter_get(itr_x) + s);
      }
      pa_iter_set(itr_ans, z);
    }
    pa_iter_flush(itr_ans);
    pa_iter_free(itr_x);
    _ tmp = _slice_raw(ans, 0, n);
    mpc_send_share_channel(TO_PARTY2, tmp, channel);
    mpc_recv_share_channel(FROM_PARTY1, tmp, channel);
    NEWITER(itr_tmp, tmp);
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i+n, pa_get(tmp->A, i));
    }
    pa_iter_free(itr_tmp);
    _free(tmp);
  }
  if (_party == 2) {
    NEWITER(itr_ans, ans);
    NEWITER(itr_x, x);
    for (int i=0; i<n; i++) {
      share_t r = RANDOM(mt_[TO_PARTY1][channel], q);
      share_t s = RANDOM(mt_[TO_PARTY3][channel], q);
      share_t z;
      if (irr_poly) {
        z = pa_iter_get(itr_x) ^ r ^ s;
      } else {
        z = MOD(pa_iter_get(itr_x) - r - s);
      }
      pa_set(ans->A, i, z);
    }
    pa_iter_free(itr_x);
    _ tmp = _slice_raw(ans, 0, n);
    mpc_send_share_channel(TO_PARTY1, tmp, channel);
    mpc_recv_share_channel(FROM_PARTY3, tmp, channel);
    NEWITER(itr_tmp, tmp);
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i+n, pa_get(tmp->A, i));
    }
    pa_iter_free(itr_tmp);
    _free(tmp);
  }

  return ans;

}
#define shamir3_to_rss(x) shamir3_to_rss_GF_channel(x, 0, 0)


void _print_debug_rss(_s x, int xor_)
{
  _ tmp = share_rss_reconstruct(x, xor_);
  _print(tmp);
  _free(tmp);
}

void share_check_rss(share_array a, share_t irr_poly)
{
  if (a->type != SHARE_T_RSS || irr_poly > 0) {
    printf("share_check_rss: type = %d irr_poly = %x\n", a->type, irr_poly);
  }
  int i, n;
  share_t q;

  n = a->n;
  q = a->q;
  int k = blog(q-1)+1;
  int err=0;
  if (_party <= 0) {
    packed_array A1, A2, A3;
    A1 = pa_new(n, k);
    A2 = pa_new(n, k);
    A3 = pa_new(n, k);
    printf("check party %d: ", _party);
    mpc_recv_pa(FROM_PARTY1, A1);
    mpc_recv_pa(FROM_PARTY2, A2);
    mpc_recv_pa(FROM_PARTY2, A3);
    if (_party == 0) {
      for (i=0; i<n; i++) {
        share_t x, r1, r2;
        r1 = MOD(pa_get(A2, i) - pa_get(A1, i)); 
        r2 = MOD(pa_get(A3, i) - pa_get(A2, i)); 
        x = MOD(pa_get(A1, i) - r1);
        if ((u64)x != pa_get(a->A, i)) {
          printf("i = %d A = %d %d A1 = %d A2 = %d A3 = %d\n", i, (int)pa_get(a->A, i), (int)x, (int)pa_get(A1,i), (int)pa_get(A2,i), (int)pa_get(A3,i));
          err=1;
          exit(1);
        }
      }
      printf("check done\n");
    }
    pa_free(A1);
    pa_free(A2);
    pa_free(A3);
  } else {
    printf("check party %d: \n", _party);
    mpc_send_share(TO_SERVER, a);
  }
}
#define _check_rss share_check_rss

share_array share_rss_random_channel(int n, share_t q, share_t m, int channel)
{
  int i;
  NEWT(share_array, ans);
  int k;

  ans->type = SHARE_T_RSS;
  ans->irr_poly = 0;
  ans->n = n*2; // Duplicate/replicate

  ans->q = q;
  ans->own = 0;
  k = blog(q-1)+1;

  ans->A = pa_new(n*2, k);

  if (_party <= 0) {
    for (i=0; i<n; i++) {
      pa_set(ans->A, i, RANDOM0(m));
      pa_set(ans->A, i+n, 0);
    }
    return ans;
  }

  int next_id = (_party % 3)+1;
  int prev_id = _party - 1;  if (prev_id == 0) prev_id = 3;
  NEWITER(itr_ans, ans);
  for (i=0; i<n; i++) {
    share_t r = RANDOM(mt_[prev_id][channel], m);
    pa_iter_set(itr_ans, r);
  }
  for (i=0; i<n; i++) {
    share_t r = RANDOM(mt_[next_id][channel], m);
    pa_iter_set(itr_ans, r);
  }
  pa_iter_flush(itr_ans);

  return ans;
}
#define share_rss_random(n, q) share_rss_random_channel(n, q, 0)

_pair RndOhv_rss(int n, int d, int channel)
{
  int m = 1<<d; // Length of one hot vector

  _ r[16];
  _ tmp;

  share_t q = 2;
  r[0] = share_const_rss(n, 1, m); // true
  r[1] = share_rss_random_channel(n, m, q, channel); // r0
  r[2] = share_rss_random_channel(n, m, q, channel); // r1
  r[4] = share_rss_random_channel(n, m, q, channel); // r2
  r[8] = share_rss_random_channel(n, m, q, channel); // r3
  r[0]->irr_poly = r[1]->irr_poly = r[2]->irr_poly = r[4]->irr_poly = r[8]->irr_poly = q;
  tmp = vmul_rss(r[2], r[1]);      // r10
  r[2+1] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[4], r[1]);      // r20
  r[4+1] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[4], r[2]);      // r21
  r[4+2] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[8], r[1]);      // r30
  r[8+1] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[8], r[2]);      // r31
  r[8+2] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[8], r[4]);      // r32
  r[8+4] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[4], r[2+1]);  // r210
  r[4+2+1] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[8], r[2+1]);  // r310
  r[8+2+1] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[8], r[4+1]);  // r320
  r[8+4+1] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[8], r[4+2]);  // r321
  r[8+4+2] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);
  tmp = vmul_rss(r[8+4], r[2+1]); // r3210
  r[8+4+2+1] = shamir3_to_rss_GF_channel(tmp, q, channel); _free(tmp);

  _ ohv = share_const_rss(n*m, 0, m);

    for (int j=0; j<m; j++) {
      _ tmp = share_const_type(n, 0, m, SHARE_T_RSS);
      tmp->irr_poly = q;
      for (int k=0; k<m; k++) { // For each term in the expanded polynomial
        int idx = 0;
        for (int i=0; i<d; i++) {
          if (k & (1<<i)) { // Select r_i
            if (idx >= 0) idx += 1<<i;
          } else {
            if (j & (1<<i)) {
              idx = -1;
            }
          }
        }
        if (idx >= 0) {
          vadd_(tmp, r[idx]);
        }
      }
      for (int p=0; p<n; p++) {
        share_setraw(ohv, p*m+j, share_getraw(tmp, p));
        share_setraw(ohv, p*m+j+m*n, share_getraw(tmp, p+n));
      }
      _free(tmp);
    }

  _ s[4];
  s[0] = share_rss_random_channel(n, m, q, channel);
  s[1] = share_rss_random_channel(n, m, q, channel);
  s[2] = share_rss_random_channel(n, m, q, channel);
  s[3] = share_rss_random_channel(n, m, q, channel);
  s[0]->irr_poly = s[1]->irr_poly = s[2]->irr_poly = s[3]->irr_poly = q;

  _ rr[4];
  for (int k=0; k<d; k++) {
    rr[k] = _slice_raw(r[1<<k], 0, n);
    if (_party > 0) {
      _ s1 = _slice_raw(s[k], 0, n);
      _ s2 = _slice_raw(s[k], n, n*2);
      vadd_(rr[k], s1);
      vadd_(rr[k], s2);
      _free(s1); _free(s2);
    }
  }
  _ E = share_const_type(n, 0, m, SHARE_T_33ADD);
  for (int p=0; p<n; p++) {
    share_t x = 0;
    for (int k=0; k<d; k++) {
      x ^= (1<<k) * (share_getraw(rr[k], p));
    }
    share_setraw(E, p, x);
  }

  for (int i=0; i<m; i++) {
    _free(r[i]);
  }
  for (int i=0; i<d; i++) {
    _free(rr[i]);
    _free(s[i]);
  }

  _pair ans = {E, ohv};
  return ans;
}


_pair RndOhv_rss(int n, int d, int channel);
_ Ohv_shamir(_ v, _pair rndohv);

_bits tablelookup_3party(_s3 x, share_t *tbl, share_t q, share_t irr_poly, int type)
{
  int n = len(x);
  int k = blog(q-1)+1; // Number of output bits
  share_t w = order(x);

  int n2 = n;
  if (type == SHARE_T_RSS) n2 = n*2;

  _bits ans = share_const_bits_3party(n2, 0, q, k);
  for (int d=0; d<ans->d; d++) ans->a[d]->type = type;

  _pair rndohv = RndOhv_rss(n, 4, 0);
  _s ohv = Ohv_shamir(x, rndohv);

  for (int i=0; i<n2; i++) {
    for (int j=0; j<k; j++) { // Compute for each output bit
      _ V = ans->a[j];
      for (share_t x=0; x<w; x++) {
        share_t y = tbl[x]; // Assuming output is y
        if (y & (1<<j)) { // If the j-th output bit is 1
          share_t tmp = pa_get(V->A, i);
          tmp ^= pa_get(ohv->A, i*w+x);
          pa_set(V->A, i, tmp);
        }
      }
    }    
  }
  _free(ohv);
  _free(rndohv.x); _free(rndohv.y);
  return ans;
}


#endif
