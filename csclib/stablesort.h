#ifndef SORT_H
 #define SORT_H

#include "dshare.h"

////////////////////////////////////////////////////
// 1-bit sort
////////////////////////////////////////////////////

_ StableSort(_ g)
{
  if (_party >  2) return NULL;
  int n = len(g);
  _ r0 = rank0(g);
  _ r1 = rank1(g);
  _ s0 = rshift(r0, 0);
  _ s1 = rshift(r1, 0);
  for (int i=0; i<n; i++) {
    _addshare(s1, i, r0, n-1);
  }
  _ sigma = IfThenElse(g, s1, s0);
  _free(r0); _free(r1);
  _free(s0); _free(s1);
  return sigma;
}

_ StableSort2_channel(_ g_, int channel)
{
  if (_party >  2) return NULL;
  int n = len(g_);
  int k = blog(n-1)+1;
  _ g;
  if (order(g_) == 2) {
    g = B2A_channel(g_, 1<<k, channel);
    //printf("g: total send %ld\n", get_total_send());
  } else {
    g = _dup(g_);
  //  g = B2A_channel(g_, 1<<k, channel);  // test 2023-08-05
  }
  _ r0 = rank0(g);
  _ r1 = rank1(g);
  _ s0 = rshift(r0, 0);
  _ s1 = rshift(r1, 0);
  for (int i=0; i<n; i++) {
    _addshare(s1, i, r0, n-1);
  }
  _ sigma = IfThenElse_channel(g, s1, s0, channel);
  _free(r0); _free(r1);
  _free(s0); _free(s1);
  _free(g);
  return sigma;
}

_ StableSort_channel2(_ g_, int channel)
{
  if (_party >  2) return NULL;
  int n = len(g_);
  int k = blog(n-1)+1;
  _ g;
  if (order(g_) == 2) {
    g = B2A_channel(g_, 1<<k, channel);
  } else {
    g = _dup(g_);
  }
  _ r0 = rank0(g);
  _ r1 = rank1(g);
  _ s0 = rshift(r0, 0);
  _ s1 = rshift(r1, 0);
  for (int i=0; i<n; i++) {
    _addshare(s1, i, r0, n-1);
  }
  _ sigma = IfThenElse_channel(g, s1, s0, channel);
  _free(r0); _free(r1);
  _free(s0); _free(s1);
  _free(g);
  return sigma;
}

_ StableSort_channel3(int d, _ b, int channel)
{
  if (_party >  2) return NULL;
  int n = len(b);
  int k = blog(n-1)+1;
  share_t q = 1<<k;
  int w = 1<<d;
  if (w > order(b)) w = order(b);

  _ ohb = onehotvec_channel(b, q, 0, channel);
  _ sum = _dup(ohb);
  for (int j=0; j<w; j++) {
    if (j != 0) _addshare(sum, j, sum, (j-1)+(n-1)*w);
    for (int i=1; i<n; i++) {
      _addshare(sum, j+i*w, sum, j+(i-1)*w);
    }
  }
  _ m = vmul(ohb, sum);
  _ pi = share_const(n, q-1, q);
  for (int i=0; i<n; i++) {
    for (int j=0; j<w; j++) {
      _addshare(pi, i, m, i*w+j);
    }
  }
  _free(m);
  _free(sum);
  _free(ohb);

  return pi;
}
#define StableSort2(g) StableSort2_channel(g, 0)


// If p = 0, assume the subsequent AppInvPerm will not communicate in parallel.
// If p = 1, set the modulus assuming the subsequent AppInmPerm is processed in parallel.
share_array* _ParallelStableSort2_channel(int m, share_array *g_, int p, int channel) {
  if (_party > 2) return NULL;
  int n = len(g_[0]);
  int k;
  if (p == 0) {
    k = blog(n-1) + 1;
  } 
  else {
    k = blog(n*m-1) + 1;
  }
  share_array serialized_g;
  share_array serialized_s0 = share_const(m * n, 0, 1<<k);
  share_array serialized_s1 = share_const(m * n, 0, 1<<k);
  
  if (order(g_[0]) == 2) {
    share_array serialized_g_B = share_const(m * n, 0, 2);
    for (int i = 0; i < m; ++i) {
      share_setshares(serialized_g_B, i * n, (i + 1) * n, g_[i], 0);
    }
    serialized_g = B2A_channel(serialized_g_B, 1<<k, channel);  // B2A
    share_free(serialized_g_B);
  } else {
    serialized_g = share_const(m * n, 0, order(g_[0]));
    pa_iter itr = pa_iter_new(serialized_g->A);
    for (int i = 0; i < m; ++i) {
      share_setshares(serialized_g, i * n, (i + 1) * n, g_[i], 0);
      pa_iter itr_g = pa_iter_new(g_[i]->A);
      for (int j=0; j<n; j++) {
        pa_iter_set(itr, pa_iter_get(itr_g));
      }
      pa_iter_free(itr_g);
    }
    pa_iter_flush(itr);
  }

  pa_iter itr_ss0 =  pa_iter_new(serialized_s0->A);
  pa_iter itr_ss1 =  pa_iter_new(serialized_s1->A);
  share_t qs = order(serialized_s0);
  for (int i = 0; i < m; ++i) {
    share_array g = share_slice(serialized_g, i * n,  (i + 1) * n);
    _ r0 = rank0(g);
    _ r1 = rank1(g);
    _ s0 = rshift(r0, 0);
    _ s1 = rshift(r1, 0);
    share_t r0_n1 = share_getraw(r0, n-1);
    pa_iter itr_s0 =  pa_iter_new(s0->A);
    pa_iter itr_s1 =  pa_iter_new(s1->A);
    for (int j=0; j<n; j++) {
      pa_iter_set(itr_ss0, pa_iter_get(itr_s0));
      pa_iter_set(itr_ss1, (pa_iter_get(itr_s1) + r0_n1) % qs);
    }
    pa_iter_free(itr_s0);
    pa_iter_free(itr_s1);

    share_free(g);
    share_free(r0);
    share_free(r1);
    share_free(s0);
    share_free(s1);
  }
  pa_iter_flush(itr_ss0);
  pa_iter_flush(itr_ss1);

  share_array serialized_sigma = IfThenElse_channel(serialized_g, serialized_s1, serialized_s0, channel); // B2A, BT
  share_array *sigma;
  NEWA(sigma, share_array, m);
  for (int i = 0; i < m; ++i) {
    sigma[i] = share_slice(serialized_sigma, i * n, (i + 1) * n);
  }

  share_free(serialized_g);
  share_free(serialized_s0);
  share_free(serialized_s1);
  share_free(serialized_sigma);

  return sigma;
}

share_array* ParallelStableSort2_channel(int m, share_array *g_, int channel) {
  return _ParallelStableSort2_channel(m, g_, 0, channel);
}



#endif
