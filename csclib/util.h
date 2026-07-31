#ifndef _C_UTIL_H
#define _C_UTIL_H

#include <stdio.h>
#include <stdlib.h>

#include "share.h"

// #define min(a, b) ((a) < (b) ? (a) : (b))

share_array flatten(_bits b) {
  /*
  b->a:
    0: b->a[0][0] b->a[0][1] ...
    1: b->a[1][0] b->a[1][1] ...
  ...
    i: b->a[i][0] b->a[i][1] ...
  ...
  d-1: b->a[d-1][0] b->a[d-1][1] ...

  ret: b->a[d-1][0] b->a[d-2][0] ... b->a[d-i-1][0] ... b->a[0][0] b->a[d-1][1]
  ...
  */
  size_t n = len(b->a[0]) * b->d;
  share_array ret = share_const(n, 0, 2);
  for (size_t i = 0; i < b->d; i++) {
    for (size_t j = 0; j < len(b->a[i]); j++) {
      share_setshare(ret, i + j * b->d, b->a[b->d - i - 1], j);
    }
  }
  return ret;
}

_bits sliding_window(share_array bs, size_t window) {
  _bits w_ext = share_bits_const(window, len(bs), 2, 0);

  /*
  w: w[0] w[1] w[2] ...

  w_ext->a:
  [s-1]:  w[0]    w[1]    w[2]    ...   w[len(bs)-1]
  [s-2]:  w[1]    w[2]    w[3]    ...   0
  [s-3]:  w[2]    w[3]    w[4]    ...   0
  ...
    [0]:  w[s-1]  w[s]    w[s+1]  ...   0
  */

  for (size_t i = 0; i < len(bs); i++) {
    for (size_t j = 0; j < min(window, i + 1); j++) {
      share_setshare(w_ext->a[window - j - 1], i - j, bs, i);
    }
  }

  return w_ext;
}

// _pair Unary_sliding_bits(share_array bs, int U, size_t window)
// {
//   if (_party >  2) {
//     _pair ans = {NULL, NULL};
//     return ans;
//   }

//   // int N = len(x->a[0]);
//   int N = len(bs) - window + 1;
//   share_t q = order(bs);
//   int d = blog(N+U-1)+1;
//   _bits X = _const_bits(N+U, 0, q, window);
//   _ Y = _const(N+U, 0, 1<<d);
//   for (int i=0; i<U; i++) {
//     _setpublic_bits(X, i, i);
//     _setpublic(Y, i, 0);
//   }
//   for (int i=0; i<N; i++) {
//     _setshare_bits(X, i+U, x, i);
//     _setpublic(Y, i+U, 1);
//   }
// //  printf("X\n"); _print_bits(X);
//   _ tmpy = share_radix_sort_bits(X);
//   _pair ans;
// //  printf("tmpy "); _print(tmpy);
// //  _ tmpy_inv = AppInvPerm(Perm_ID2(N+U, order(tmpy)), tmpy);
//   _ tmpy_inv = InvPerm(tmpy);
// //  printf("tmpy_inv "); _print(tmpy_inv);
//   ans.x = AppPerm(Y, tmpy_inv);
// //  printf("ans.x "); _print(ans.x);

//   _ sigma = StableSort(ans.x);
// //  printf("sigma "); _print(sigma);
//   _ qq = AppInvPerm(tmpy_inv, sigma);
// //  printf("qq "); _print(qq);
//   _ rho = _slice(qq, U, U+N);
//   for (int i=0; i<N; i++) {
//     _addpublic(rho, i, -U);
//   }
// //  printf("rho "); _print(rho);
//   ans.y = rho;

//   _free(sigma);
//   _free_bits(X);
//   _free(Y);
//   _free(qq);
//   _free(tmpy);
//   _free(tmpy_inv);

//   return ans;

// }

_ BatchAccess_bits_idx(_ v, _bits idx) {
  if (_party > 2) return NULL;

  _pair tmp = Unary_bits(idx, len(v));
  _ I = tmp.x;
  _ sigma = tmp.y;
  //  printf("b idx "); _print_bits(idx);
  //  printf("b unary "); _print(I);
  //  printf("b sigma "); _print(sigma);

  // almost identical to BatchAccess_bits_bits until here

  // Below is the same as BatchAccess

  _ ans = BatchAccessUnary(v, I);
  //  printf("BatchAccess: ans "); _print(ans);
  //  _pair tmp = share_radix_sort(I);
  //  _ ans2 = AppInvPerm(ans, tmp.y);
  _ ans2 = AppInvPerm(ans, sigma);  // arrange in the order specified by idx
  //  printf("BatchAccess: ans2"); _print(ans2);
  _free(I);
  _free(sigma);
  _free(ans);
  return ans2;
}

_ BatchAccess_sliding_bits_idx(_ v, share_array idx, size_t window) {
  if (_party > 2) return NULL;

  _bits idx_ext = sliding_window(idx, window);

  _ ans = BatchAccess_bits_idx(v, idx_ext);

  _free_bits(idx_ext);

  return ans;
}

share_array share_extend_or_shrink(share_array a, share_t q) {
  if (order(a) == q)
    return share_dup(a);
  else if (order(a) < q) {
    return share_extend(a, q);
  } else {
    return share_shrink(a, q);
  }
}

// From share_radix_sort_channel2 in share.h
share_array share_radix_sort_channel2_inv(_ a_, int channel) {
  if (_party > 2) {
    return NULL;
  }
  _ a = _dup(a_);
  //  _ pi = Perm_ID(a);
  int w = blog(a->n - 1) + 1;
  _ pi = Perm_ID2(a->n, 1 << w);
  share_t q = a->q;
  //  share_t qb = order(a);
  //  share_t qb = 1<<w;
  for (int k = 1; k < q; k *= 2) {
    // printf("k %d q %d\n", k, q);
    // printf("a "); _print(a);
    //    _pair tmp = share_A2QB(a, q/k, qb);
    //    printf("old bits "); _print(tmp2.y);
    //    printf("old q    "); _print(tmp2.x);
    //    _check(a);
    //  _pair tmp = share_A2QB_channel2(a, q/k, 2, channel);
    _pair tmp = share_A2QB_channel2(a, q / k, 1 << w, channel);
    // printf("new bits "); _print(tmp.y);
    // printf("new q    "); _print(tmp.x);
    //_ sigma = StableSort(tmp.y);
    //  printf("bits "); _print(tmp.y);
    //_ sigma = StableSort(tmp.y);
    _ sigma = StableSort_channel2(tmp.y, channel);
    // printf("sigma "); _print(sigma);
    if (tmp.x->q > 1) {
      _move_(a, AppInvPerm_channel(tmp.x, sigma, channel));
    }
    _free(tmp.x);
    _free(tmp.y);
    _move_(pi, AppInvPerm_channel(pi, sigma, channel));
    _free(sigma);
  }
  _free(a);
  _ tmp = Perm_ID2(len(pi), order(pi));
  _ ans = AppInvPerm_channel(tmp, pi, channel); 
  _free(tmp);
  _free(pi);
  return ans;
}

#define share_radix_sort_inv(a) share_radix_sort_channel2_inv(a, 0)

_ PrefixSum0(_ v) {
  if (_party > 2) return NULL;
  int n = len(v);
  _ ans = share_const(n, 0, order(v));
  for (int i = 1; i < n; i++) {
    _setshare(ans, i, v, i - 1);
    _addshare(ans, i, ans, i - 1);
  }
  return ans;
}

#endif  // _C_UTIL_H