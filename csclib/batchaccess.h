#ifndef BATCHACCESS_H
 #define BATCHACCESS_H

#include "propagate.h"

/***********************************************************
# v is the array to be accessed. Length U.
# idx is the array of indices of elements to access in v, represented as unary notation.
# The maximum value of idx is less than len(v)
def BatchAccessUnary(v, idx):
  U = len(v)
  N = len(idx)
  sigma = StableSort(idx)
  n = sum(idx)
  X = v + ([0] * (N-U))
  Y = AppPerm(X, sigma)
  Z = Propagate(vneg(idx), Y)
  W = AppInvPerm(Z, sigma)
  return W[U:]
***********************************************************/
_ BatchAccessUnary(_ v, _ idx)
{
  if (_party >  2) return NULL;
  int U = len(v);
  int N = len(idx);
  _ idx_tmp = _shrink(idx, 2);
  _ sigma = StableSort2(idx_tmp);
  //printf("sigma "); _print(sigma);
  _ zeros = _const(N-U, 0, order(v));
  _ X = _concat(v, zeros);
  _ Y = AppPerm(X, sigma);
  _ nidx = vneg(idx);
  _move_(nidx, _shrink(nidx,2));
  _ Z = Propagate2(nidx, Y);
  _ W = AppInvPerm(Z, sigma);
  _ ans = _slice(W, U, N);

  _free(sigma);
  _free(X);
  _free(Y);
  _free(nidx);
  _free(Z);
  _free(W);
  _free(zeros);
  _free(idx_tmp);

  return ans;
}

_ BatchAccessUnaryBlock(_ v, _ idx, int block_size)
{
  if (_party >  2) return NULL;
  if (len(v) % block_size != 0) {
      printf("BatchAccessUnaryBlock: len(v) %d block_size %d\n", len(v), block_size);
      return NULL;
  }
  int U = len(v) / block_size;
  int N = len(idx);
  _ idx_tmp = _shrink(idx, 2);
  _ sigma = StableSort2(idx_tmp);
  //printf("sigma "); _print(sigma);
  _ zeros = _const((N-U) * block_size, 0, order(v));
  _ X = _concat(v, zeros);
  _ Y = block_AppPerm(block_size, X, sigma);
  _ nidx = vneg(idx);
  _move_(nidx, _shrink(nidx,2));
  _ Z = PropagateBlock(nidx, Y, block_size);
  _ W = block_AppInvPerm(block_size, Z, sigma);
  _ ans = _slice(W, U * block_size, N * block_size);

  _free(sigma);
  _free(X);
  _free(Y);
  _free(nidx);
  _free(Z);
  _free(W);
  _free(zeros);
  _free(idx_tmp);

  return ans;
}


/************************************************
def Unary(x, U):
  N = len(x)
  i = 0
  X = [0] * (N+U)
  print('N', N, 'U', U)
  while i < U:
    X[i] = (i, 0, i) # (x, b, pos)
    i += 1
  i = 0
  while i < N:
    X[i+U] = (x[i], 1, i+U)
    i += 1
  print('X1', X)
  X.sort() # log N rounds
  print('X2', X)
  B = [b for (x, b, i) in X]
##  sigma = [i for (x, b, i) in X]
##  print(B)
  return B
************************************************/
_ Unary(_ x, int U)
{
  if (_party >  2) return NULL;
  int N = len(x);
  share_t q = order(x);
  _ X = _const(N+U, 0, q);
  int d = blog(N+U+1)+1;
  _ Y = _const(N+U, 0, 1<<d);
  for (int i=0; i<U; i++) {
    _setpublic(X, i, i);
    _setpublic(Y, i, 0);
  }
  for (int i=0; i<N; i++) {
    _setshare(X, i+U, x, i);
    _setpublic(Y, i+U, 1);
  }
  _pair tmp = share_radix_sort(X);
  _ sigma = _move(tmp.y);
  _ ans = AppPerm(Y, sigma);
  _free(sigma);
  _free(X);
  _free(Y);
  _free(tmp.x);

  return ans;

}

_pair Unary2(_ x, int U)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int N = len(x);
  share_t q = order(x);
  _ X = _const(N+U, 0, q);
  int d = blog(N+U+1)+1;
  _ Y = _const(N+U, 0, 1<<d);
  for (int i=0; i<U; i++) {
    _setpublic(X, i, i);
    _setpublic(Y, i, 0);
  }
  for (int i=0; i<N; i++) {
    _setshare(X, i+U, x, i);
    _setpublic(Y, i+U, 1);
  }
  _pair tmp = share_radix_sort(X);
  _pair ans;
  ans.x = AppPerm(Y, tmp.y);

  _ sigma = StableSort(ans.x);
  _ qq = AppInvPerm(tmp.y, sigma);
  _ rho = _slice(qq, U, U+N);
  for (int i=0; i<N; i++) {
    _addpublic(rho, i, -U);
  }
  ans.y = rho;

  _free(X);
  _free(Y);
  _free(tmp.x);
  _free(tmp.y);
  _free(sigma);
  _free(qq);

  return ans;

}

//////////////////////////////////////////////////////
// When x is an XOR share
//////////////////////////////////////////////////////
_pair Unary_xor(_ x, int U)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int N = len(x);
  share_t q = order(x);
  _ X = _const(N+U, 0, q);
  int d = blog(N+U+1)+1;
  _ Y = _const(N+U, 0, 1<<d);
  for (int i=0; i<U; i++) {
    _setpublic(X, i, i);
    _setpublic(Y, i, 0);
  }
  for (int i=0; i<N; i++) {
    _setshare(X, i+U, x, i);
    _setpublic(Y, i+U, 1);
  }
  _pair tmp = share_radix_sort_xor(X);
  _pair ans;
  ans.x = AppPerm(Y, tmp.y);

  _ sigma = StableSort(ans.x);
  _ qq = AppInvPerm(tmp.y, sigma);
  _ rho = _slice(qq, U, U+N);
  for (int i=0; i<N; i++) {
    _addpublic(rho, i, -U);
  }
  ans.y = rho;

  _free(X);
  _free(Y);
  _free(tmp.x);
  _free(tmp.y);
  _free(sigma);
  _free(qq);

  return ans;

}

_pair Unary_bits(_bits x, int U)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }

  int N = len(x->a[0]);
  share_t q = order(x->a[0]);
  int d = blog(N+U-1)+1;
  //_bits X = _const_bits(N+U, 0, 1<<d, x->d);
  _bits X = _const_bits(N+U, 0, 2, x->d);
  _ Y = _const(N+U, 0, 1<<d);
  for (int i=0; i<U; i++) {
    _setpublic_bits(X, i, i);
    _setpublic(Y, i, 0);
  }
  for (int i=0; i<N; i++) {
    _setshare_bits(X, i+U, x, i);
    _setpublic(Y, i+U, 1);
  }
  _ tmpy = share_radix_sort_bits(X);
  _pair ans;
  _ tmpy_inv = InvPerm(tmpy);
  ans.x = AppPerm(Y, tmpy_inv);

  _ sigma = StableSort(ans.x);
  _ qq = AppInvPerm(tmpy_inv, sigma);
  _ rho = _slice(qq, U, U+N);
  for (int i=0; i<N; i++) {
    _addpublic(rho, i, -U);
  }
  ans.y = rho;

  _free(sigma);
  _free_bits(X);
  _free(Y);
  _free(qq);
  _free(tmpy);
  _free(tmpy_inv);

  return ans;

}



/************************************************
def BatchAccess(v, idx):
  I = Unary(idx, len(v))
  print('idx', idx, 'unary', I)
  return BatchAccessUnary(v, I)
************************************************/
// idx does not need to be monotonically increasing
_ BatchAccess(_ v, _ idx)
{
  if (_party >  2) return NULL;
  _pair tmp = Unary2(idx, len(v));
  _ I = tmp.x;
  _ sigma = tmp.y;
  _ ans = BatchAccessUnary(v, I);
  _ ans2 = AppInvPerm(ans, sigma); // reorder according to the order specified by idx
  _free(I);
  _free(sigma);
  _free(ans);
  return ans2;
}

_ BatchAccessBlock(_ v, _ idx, int block_size)
{
  if (_party >  2) return NULL;
  if (len(v) % block_size != 0) {
      printf("BatchAccessBlock: len(v) %d block_size %d\n", len(v), block_size);
      return NULL;
  }
  int n = len(v) / block_size;
  _pair tmp = Unary2(idx, n);
  _ I = tmp.x;
  _ sigma = tmp.y;
  _ ans = BatchAccessUnaryBlock(v, I, block_size);
  _ ans2 = block_AppInvPerm(block_size, ans, sigma); // reorder according to the order specified by idx
  _free(I);
  _free(sigma);
  _free(ans);
  return ans2;
}

_pair BatchAccessBlockP(_pair Pv, _ idx, int block_size)
{
  _ v = _zipP(Pv);
  _ v2 = BatchAccessBlock(v, idx, 2*block_size);
  _pair ans = _unzipP(v2);
  _free(v); _free(v2);
  return ans;
}

_ BatchAccess_xor(_ v, _ idx)
{
  if (_party >  2) return NULL;
  _pair tmp = Unary_xor(idx, len(v));
  _ I = tmp.x;
  _ sigma = tmp.y;
  _ ans = BatchAccessUnary(v, I);
  _ ans2 = AppInvPerm(ans, sigma); // reorder according to the order specified by idx
  _free(I);
  _free(sigma);
  _free(ans);
  return ans2;
}


_bits BatchAccess_bits(_bits v, _ idx)
{
  if (_party >  2) return NULL;
  _pair tmp = Unary2(idx, len(v->a[0]));
  _ I = tmp.x;
  _ sigma = tmp.y;
  int d = v->d;
  int n = v->a[0]->n;
  share_t q = v->a[0]->q;
  _bits ans = _const_bits(n, 0, q, d);

  // TODO: blocking for better performance

  for (int i=0; i<d; i++) {
    _ tmp2 = BatchAccessUnary(v->a[i], I);
    _move_(ans->a[i], AppInvPerm(tmp2, sigma));
    _free(tmp2);
  }
  _free(sigma);
  _free(I);
  return ans;
}

_bits BatchAccess_bits_bits(_bits v, _bits idx)
{
  if (_party >  2) return NULL;

  _pair tmp = Unary_bits(idx, len(v->a[0]));
  _ I = tmp.x;
  _ sigma = tmp.y;

  int d = v->d;
  int n = v->a[0]->n;
  share_t q = v->a[0]->q;
  _bits ans;
  ans = _const_bits(n, 0, q, d);

  for (int i=0; i<d; i++) {
    _ tmp2 = BatchAccessUnary(v->a[i], I);
    _move_(ans->a[i], AppInvPerm(tmp2, sigma));
    _free(tmp2);
  }
  _free(I);
  _free(sigma);
  return ans;
}



#endif
