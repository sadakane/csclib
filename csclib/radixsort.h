#ifndef RADIXSORT_H
 #define RADIXSORT_H

#include "stablesort.h"


//////////////////////////////////////////////////////////
// Radix Sort
//////////////////////////////////////////////////////////


_pair share_radix_sort_channel(_ a_, int channel)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  _ a = _dup(a_);
  int w = blog(a->n-1)+1;
  _ pi = Perm_ID2(a->n, 1<<w);
  share_t q = a->q;
  for (int k=1; k<q; k*=2) {
    _pair tmp = share_A2QB_channel(a, q/k, 2, channel);
      _ sigma = StableSort2_channel(tmp.y, channel);
    if (tmp.x->q > 1) {
      _move_(a, AppInvPerm_channel(tmp.x, sigma, channel));
    }
    _free(tmp.x);
    _free(tmp.y);
    _move_(pi, AppInvPerm_channel(pi, sigma, channel));
    _free(sigma);
  }
  _free(a);
  _ x = AppPerm_channel(a_, pi, channel);
  _pair ans = {x, pi};
  return ans;
}

/////////////////////////////////////////////////////////////////////
// Improved version (2023-07-21)
/////////////////////////////////////////////////////////////////////
_pair share_radix_sort_channel2(_ a_, int channel)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  _ a = _dup(a_);
  int w = blog(a->n-1)+1;
  _ pi = Perm_ID2(a->n, 1<<w);
  share_t q = a->q;
  for (int k=1; k<q; k*=2) {
    _pair tmp = share_A2QB_channel2(a, q/k, 1<<w, channel);
    _ sigma = StableSort_channel2(tmp.y, channel);
    if (tmp.x->q > 1) {
      _move_(a, AppInvPerm_channel(tmp.x, sigma, channel));
    }
    _free(tmp.x);
    _free(tmp.y);
    _move_(pi, AppInvPerm_channel(pi, sigma, channel));
    _free(sigma);
  }
  _free(a);
  _ x = AppPerm_channel(a_, pi, channel);
  _pair ans = {x, pi};
  return ans;
}

_pair share_radix_sort_channel4(_ a_, share_t q, int channel)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  _ a = _dup(a_);
  int w = blog(a->n-1)+1;
  _ pi = Perm_ID2(a->n, 1<<w);
  q = min(q, a->q);
  for (int k=1; k<q; k*=2) {
    _pair tmp = share_A2QB_channel2(a, q/k, 1<<w, channel);
    _ sigma = StableSort_channel2(tmp.y, channel);
    if (tmp.x->q > 1) {
      _move_(a, AppInvPerm_channel(tmp.x, sigma, channel));
    }
    _free(tmp.x);
    _free(tmp.y);
    _move_(pi, AppInvPerm_channel(pi, sigma, channel));
    _free(sigma);
  }
  _free(a);
  _ x = AppPerm_channel(a_, pi, channel);
  _pair ans = {x, pi};
  return ans;
}


_pair share_radix_sort_xor_channel(_ a_, int channel)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  _ a = _dup(a_);
  int w = blog(a->n-1)+1;
  _ pi = Perm_ID2(a->n, 1<<w);
  share_t q = a->q;
  for (int k=1; k<q; k*=2) {
    _pair tmp = share_A2QB_xor(a);
    printf("tmp.x "); _print(tmp.x);
    printf("tmp.y "); _print(tmp.y);
    _ sigma = StableSort_channel2(tmp.y, channel);
    printf("sigma "); _print(sigma);
    if (tmp.x->q > 1) {
      _move_(a, AppPerm_inverse_xor_channel(tmp.x, sigma, channel));
    }
    _free(tmp.y);
    _move_(pi, AppInvPerm_channel(pi, sigma, channel));
    _free(sigma);
  }
  _free(a);
  _ x = AppPerm_fwd_xor_channel(a_, pi, channel);
  _pair ans = {x, pi};
  return ans;
}
#define share_radix_sort_xor(a) share_radix_sort_xor_channel(a, 0)


#define share_radix_sort(a) share_radix_sort_channel2(a, 0)
#define _radix_sort share_radix_sort




_pair share_radix_sort_cont4(_ pi, _ a, share_t q)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  _ key = AppPerm(a, pi);
  _pair tmp = share_radix_sort_channel4(key, q, 0);
  _free(key);
  AppPerm_(pi, tmp.y);
  _move_(tmp.y, pi);
  return tmp;
}

_ share_radix_sort_bits(_bits a)
{
  if (_party >  2) return NULL;
  int d = a->d;
  _ pi = StableSort2(a->a[0]);
  _ ap;
  _ sigma;
  for (int k=1; k<d; k++) {
    ap = AppInvPerm(a->a[k], pi);
    sigma = StableSort2(ap);
    _free(ap);
    _move_(pi, AppPerm(sigma, pi));
    _free(sigma);
  }
  return pi;
}
#define _radix_sort_bits share_radix_sort_bits

_ share_radix_sort_bits_channel(_bits a, int channel)
{
  if (_party >  2) return NULL;
  int d = a->d;
  _ pi = StableSort2_channel(a->a[0], channel);
  _ ap;
  _ sigma;
  for (int k=1; k<d; k++) {
    ap = AppInvPerm_channel(a->a[k], pi, channel);
    sigma = StableSort2_channel(ap, channel);
    _free(ap);
    _move_(pi, AppPerm_channel(sigma, pi, channel));
    _free(sigma);
  }
  return pi;
}
#define _radix_sort_bits2(a) share_radix_sort_bits_channel(a, 0)


#endif
