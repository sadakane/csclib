#ifndef DECOMPOSITION_H
 #define DECOMPOSITION_H

// Dependency: vmul

////////////////////////////////////////////////////////
// Convert additive share to least significant bit share and other shares
// Field order q must be a power of 2
// The order of other shares is q/2
// The order of the least significant bit share is qb
////////////////////////////////////////////////////////
_pair share_A2QB_channel(_ a, share_t q, share_t qb, int channel)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int k = blog(q-1)+1;
  if ((1 << k) != q) {
    printf("share_A2QB: %d is not a power of two\n", (int)q);
  }

  int n = len(a);
  _ b = _const(n, 0, qb);
  for (int i=0; i<n; i++) {
    pa_set(b->A, i, (q+pa_get(a->A,i)) % 2); // Least significant bit of additive share
  }
  _ b1 = _const(n, 0, qb);
  if (_party == 2) {
    for (int i=0; i<n; i++) {
      pa_set(b1->A, i, (q+pa_get(a->A, i)) % 2); // Least significant bit of additive share
    }
  }
  _ b2 = _const(n, 0, qb);
  if (_party != 2) {
    for (int i=0; i<n; i++) {
      pa_set(b2->A, i, (q+pa_get(a->A, i)) % 2); // Least significant bit of additive share
    }
  }
  _ bp = vmul_channel(b1, b2, channel);
  _free(b1); _free(b2); 
  for (int i=0; i<n; i++) {
    pa_set(b->A, i, (10*qb + pa_get(b->A, i) - 2*pa_get(bp->A, i)) % qb); // If both shares are 1, should be 0 but becomes 2
  }
  _free(bp);


  q = q/2;
  _ x;
  if (q > 1) {

    _ c1 = _const(n, 0, q);
    if (_party == 2) {
      for (int i=0; i<n; i++) {
        pa_set(c1->A, i, (q*4+pa_get(a->A, i)) % 2); // Least significant bit of additive share !!!
      }
    }
    _ c2 = _const(n, 0, q);
    if (_party != 2) {
      for (int i=0; i<n; i++) {
        pa_set(c2->A, i, (q*4+pa_get(a->A, i)) % 2); // Least significant bit of additive share
      }
    }
    _ c = vmul_channel(c1, c2, channel);
    _free(c1); _free(c2); 


    x = _const(n, 0, q);

    for (int i=0; i<n; i++) {
      pa_set(x->A, i, ((q*4+pa_get(a->A, i)) / 2) % q); // !!!
    }
    vadd_(x, c);

    //_free(c1); _free(c2); 
    _free(c);
  } else {
    x = _const(n, 0, 2);
  }

  _pair ans = {x, b};
  //_free(b1); _free(b2); _free(bp);


  return ans;
}
#define _A2QB share_A2QB

_pair share_A2QB_xor(_ a)
{
  share_t q = order(a);
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int k = blog(q-1)+1;
  if ((1 << k) != q) {
    printf("share_A2QB_xor: %d is not a power of two\n", (int)q);
  }

  int n = len(a);
  _ b = _const(n, 0, 2);
  for (int i=0; i<n; i++) {
    pa_set(b->A, i, (q+pa_get(a->A,i)) % 2); // Least significant bit of XOR share
  }


  q = q/2;
  _ x;
  if (q > 1) {
    x = _const(n, 0, q);
    for (int i=0; i<n; i++) {
      pa_set(x->A, i, ((q*4+pa_get(a->A, i)) / 2) % q); // !!!
    }
  } else {
    x = _const(n, 0, 2);
  }

  _pair ans = {x, b};

  return ans;
}

#define share_A2QB(a, q, qb) share_A2QB_channel(a, q, qb, 0)

////////////////////////////////////////////////////////
// Convert additive share to bit-wise share (order qb)
// Field order q must be a power of 2
/////////////////////////////////////////////////////////////////
_bits share_A2B(_ a, share_t qb)
{
  if (_party >  2) return NULL;
  NEWT(_bits, ans);

  share_t q = order(a);
  int k = blog(q-1)+1;
  if ((1 << k) != q) {
    printf("share_A2B: %d is not a power of two\n", (int)q);
  }
  ans->d = k;

  NEWA(ans->a, share_array, k);
  _ x = _dup(a);
  for (int i = 0; i<k; i++) {
    _pair tmp = share_A2QB(x, order(x), qb);
    ans->a[i] = tmp.y;
    _move_(x, tmp.x);
  }
  _free(x);

  return ans;
}

#endif
