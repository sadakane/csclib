/*
  This is an implementation of the following paper:

  Nuttapong Attrapadung, Hiraku Morita, Kazuma Ohara, Jacob C. N. Schuldt, Tadanori Teruya, and Kazunari Tozawa.
  Secure Parallel Computation on Privately Partitioned Data and Applications. In Proceedings of the 2022 ACM SIGSAC 
  Conference on Computer and Communications Security (CCS '22). Association for Computing Machinery, New York, NY, USA, 151–164. 
  https://doi.org/10.1145/3548606.3560695
*/

//////////////////////////////////////////////////
// GenCycle and related functions
//////////////////////////////////////////////////
#ifndef PROPAGATE_H
 #define PROPAGATE_H

#include "stablesort.h"



/************************************
def GenCycle(g):
  N = len(g)
  sigma = StableSort(g)
  t0 = vmul(Perm_ID(N), vneg(g))
  t1 = vmul(Perm_ID(N), g)
  u = AppInvPerm(t1, sigma)
  v = lshift(u, 0)
  y = AppPerm(v, sigma)
  pi = vadd(y, t0)
  w = rshift(u, 0)
  z = AppPerm(w, sigma)
  pi_inv = vadd(z, t0)
  pi_inv[0] = u[N-1]
  return (pi, pi_inv)
*************************************/
_pair GenCycle(_ g)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int n = len(g);
  _ gtmp = share_shrink(g, 2);
  _ sigma = StableSort2(gtmp);
  _ perm = Perm_ID2(len(sigma), order(sigma));
  _ gneg = vneg(g);
  _ gnegtmp = _shrink(gneg, 2);
  _ gnegtmp2 = B2A(gneg, order(perm));
  _ gtmp2 = B2A(gtmp, order(perm));
  _ t0 = vmul(perm, gnegtmp2);
  _ t1 = vmul(perm, gtmp2);
  _free(gnegtmp); _free(gnegtmp2);
  _free(gtmp); _free(gtmp2);
  _ u = AppInvPerm(t1, sigma);
  _ v = lshift(u, 0);
  _ y = AppPerm(v, sigma);
  _ pi = vadd(y, t0);
  _ w = rshift(u, 0);
  _ z = AppPerm(w, sigma);
  _ pi_inv = vadd(z, t0);
  _setshare(pi_inv, 0, u, n-1);

  _pair ans = {pi, pi_inv};

  _free(z);
  _free(w);
  _free(y);
  _free(v);
  _free(t0);
  _free(t1);
  _free(u);
  _free(gneg);
  _free(sigma);
  _free(perm);

  return ans;
}

_pair GenCycle2(_ g_)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int n = len(g_);
  _ sigma = StableSort2(g_);
  share_t k = blog(n-1)+1;
  share_t q = 1<<k;
  _ g = B2A(g_, q);
  _ perm = Perm_ID(g);
  _ gneg = vneg(g);
  _ t0 = vmul(perm, gneg);
  _ t1 = vmul(perm, g);
  _ u = AppInvPerm(t1, sigma);
  _ v = lshift(u, 0);
  _ y = AppPerm(v, sigma);
  _ pi = vadd(y, t0);
  _ w = rshift(u, 0);
  _ z = AppPerm(w, sigma);
  _ pi_inv = vadd(z, t0);
  _setshare(pi_inv, 0, u, n-1);

  _pair ans = {pi, pi_inv};

  _free(g);
  _free(z);
  _free(w);
  _free(y);
  _free(v);
  _free(t0);
  _free(t1);
  _free(u);
  _free(gneg);
  _free(sigma);
  _free(perm);

  return ans;
}

/********************************
def Propagate(g, v):
  (pi, pi_inv) = GenCycle(g)
  x = AppInvPerm(v, pi)
  y = x
  y[0] = 0
  z = PrefixSum(vsub(v, y))
  return z
********************************/
_ Propagate(_ g, _ v)
{
  if (_party >  2) return NULL;
  _pair tmp = GenCycle(g);
  _ pi = tmp.x;
  _free(tmp.y);

  _ x = AppInvPerm(v, pi);
  _setpublic(x, 0, 0);
  _ v2 = vsub(v, x);
  _ z = PrefixSum(v2);
  _free(pi);
  _free(x);
  _free(v2);
  return z;
}

_ Propagate2(_ g, _ v)
{
  if (_party > 2) {
    NEWT(_, ans);
    *ans = *v;
    ans->A = NULL;
    return ans;
  }
  _pair tmp = GenCycle2(g);
  _ pi = tmp.x;
  _free(tmp.y);

  _ x = AppInvPerm(v, pi);
  _setpublic(x, 0, 0);
  _ v2 = vsub(v, x);
  _ z = PrefixSum(v2);
  _free(pi);
  _free(x);
  _free(v2);
  return z;
}

_ PropagateBlock(_ g, _ v, int block_size)
{
  if (_party > 2) {
    NEWT(_, ans);
    *ans = *v;
    ans->A = NULL;
    return ans;
  }
  _pair tmp = GenCycle2(g);
  _ pi = tmp.x;
  _free(tmp.y);

  _ x = block_AppInvPerm(block_size, v, pi);
  _setpublics(x, 0, block_size, 0);
  _ v2 = vsub(v, x);
  _ z = PrefixSumBlock(v2, block_size);
  _free(pi);
  _free(x);
  _free(v2);
  return z;
}

_ Propagate3(_ g, _ X)
{
  if (_party > 2) {
    NEWT(_, ans);
    *ans = *X;
    ans->A = NULL;
    return ans;
  }

  int n = len(g);
  _ pi = StableSort2(g); // 1-bit sort しかしていないので TFHE 向きではあるが，TFHE では GenCycle の方が良いかも
  _ x2 = AppInvPerm(X, pi);
  _ g2_ = AppInvPerm(g, pi);
  _ g2 = _extend(g2_, order(X));
  _ x3 = rshift(x2, 0);
  _ diff = vsub(x2, x3);
  _ diff2 = vmul(diff, g2);
  _ y0 = AppPerm(diff2, pi);
  share_setshare(y0, 0, X, 0); // test
  _ z = PrefixSum(y0);
  _free(x2); _free(x3); _free(g2); _free(g2_); _free(diff); _free(diff2);
  _free(pi); _free(y0);
 
  return z;
}

_bits Propagate_bits(_ g, _bits v)
{
  if (_party >  2) return NULL;
  NEWT(_bits, ans);
  NEWA(ans->a, share_array, v->d);
  for (int i = 0; i<v->d; i++) {
    ans->a[i] = Propagate(g, v->a[i]);
  }
  return ans;
}

/*******************************
def GroupSum(g,v):
    (pi, pi_inv) = GenCycle(g)
    s = SuffixSum(v)
    t = [0] * len(v)
    t[1:] = s[1:]
    y = AppInvPerm(t, pi_inv)
    z = vsub(s, y)
    return z
*******************************/
_ GroupSum(_ g, _ v)
{
  if (_party >  2) return NULL;
  _pair tmp = GenCycle(g);
  _free(tmp.x);
  _ pi_inv = tmp.y;
  _ s = SuffixSum(v);
  _ t = _dup(s);
  _setpublic(t, 0, 0);
  _ y = AppInvPerm(t, pi_inv);
  vsub_(s, y);

  _free(pi_inv);
  _free(t);
  _free(y);

  return s;
}

/************************************************
def GroupSumBlock(g_, v, block_size):
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.extend(1<<k2)
    else:
        g = g_.dup()
    (tmp_x, pi_inv) = GenCycle(g)
    #s = SuffixSum(v)
    n = len(g)
    s = v.dup()
    sum = Share([0]*block_size, v.order())
    i = n
    while i > 0:
        sum += v[(i-1)*block_size:i*block_size]
        s[(i-1)*block_size:i*block_size] = sum
        i -= 1

    t = s.dup()
    t[0:block_size] = 0
    y = t.AppInvPermBlock(block_size, pi_inv)
    s = s - y
    return s
************************************************/

_ GroupSumBlock(_ g, _ v, int block_size)
{
  if (_party >  2) return NULL;
  _pair tmp = GenCycle(g);
  _free(tmp.x);
  _ pi_inv = tmp.y;
  _ s = SuffixSumBlock(v, block_size);
  _ t = _dup(s);
  _setpublics(t, 0, block_size, 0);
  _ y = block_AppInvPerm(block_size, t, pi_inv);
  vsub_(s, y);

  _free(pi_inv);
  _free(t);
  _free(y);

  return s;
}


/************************************************
def Grouping(V):
  n = len(V)
  G = [0]*n
  G[0] = 1
  for i in range(1,n):
    if V[i] != V[i-1]:
      G[i] = 1
  return G
************************************************/
_ Grouping(_ V)
{
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *V;
    ans->q = 2;
    ans->A = NULL;
    return ans;
  }
  _ Vp = rshift(V, 1);
  _subshare(Vp, 0, V, 0);
  _ ans = Equality(V, Vp);
  //_ ans = EqualityConst2_channel(V, Vp, 2, 0);
  vneg_(ans);
  _free(Vp);
  return ans;
}

_ Grouping_name(_ L, share_t q)
{
  if (_party >  2) return NULL;
  int n = len(L);
  _ V = _const(n, 0, q);
  for (int i=0; i<n; i++) {
    _setpublic(V, i, i);
  }
//  _ ans = Propagate(L, V);
  _ ans = Propagate2(L, V);
  _free(V);
  return ans;
}

_ Grouping_bit(_ V)
{
  if (_party >  2) return NULL;
  _ Vp = rshift(V, 1);
  _subshare(Vp, 0, V, 0);
  _ ans = Equality_bit(V, Vp);
  vneg_(ans);
  _free(Vp);
  return ans;
}

_ Grouping_bits(_bits V)
{
  if (_party >  2) return NULL;
  int d = V->d;
  _ b = Grouping_bit(V->a[0]);
  for (int i=1; i<d; i++) {
    _ g = Grouping_bit(V->a[i]);
    _move_(b, OR(b, g));
    _free(g);
  }
  return b;
}

#endif
