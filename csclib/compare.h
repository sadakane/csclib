#ifndef _COMPARE_H
 #define _COMPARE_H

#define COMP_pPE 0
#define COMP_PIE 1
#define COMP_LT 2
#define COMP_LE 3
#define COMP_EQ 4
#define COMP_GE 5
#define COMP_GT 6
#define COMP_3 7


//////////////////////////////////////////////////////////////
// When x[i] == y[i], set b[i] := 1
// x, y are mod 2^k, b is mod 2
//////////////////////////////////////////////////////////////
_b Equality_bit(_ x, _ y)
{
  int q = 2;
  int n = len(x);
  _b ans = _const(n, 0, q);
  if (_party <= 1) {
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i, MOD(pa_get(x->A, i) + pa_get(y->A, i) + 1));
    }
  }
  if (_party == 2) {
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i, MOD(pa_get(x->A, i) + pa_get(y->A, i)));
    }
  }
  return ans;
}


_ Equality2(_ a, _ b)
{
  if (a == NULL || b == NULL) {
    printf("Equality2: a = %p b = %p\n", a, b);
    return NULL;
  }
  if (a->q != 2 || b->q != 2) {
    printf("Equality2: a->q = %d b->q = %d\n", a->q, b->q);
  }
  _ ans = XOR(a, b);
  vneg_(ans);
  return ans;
}


_b Equality_bits(_bits x, _bits y)
{
  if (_party >  2) return NULL;
  if (x->d != y->d) {
    printf("Equality_bits: x->d = %d y->d = %d\n", x->d, y->d);
  }
  int d = x->d;
  _b b = Equality_bit(x->a[0], y->a[0]);
  vneg_(b);
  for (int i=1; i<d; i++) {
    _b c = Equality_bit(x->a[i], y->a[i]);
    vneg_(c);
    _move_(b, OR(b, c));
    _free(c);
  }
  vneg_(b);
  return b;
}

_b Equality_old(_ a, _ b)
{
  if (a == NULL || b == NULL) {
    printf("Equality: a = %p b = %p\n", a, b);
    return NULL;
  }
  int n = len(a);
  _ ans;
  if (_party > max_partyid(a)) {
    NEWA(ans, struct share_array, 1);
    *ans = *a;
    ans->q = 2;
    ans->type = SHARE_T_22ADD; // Requires review - BINARY?
    ans->A = NULL;
    return ans;
  }
  if (_party >= 0) {
    share_t q = order(a);
    int k = blog(q-1)+1; // Number of digits
    _ c = _const(n*k, 0, 2);
    _ v;
    if (_party == 1) {
      v = vsub(a, b);
    } else {
      v = vsub(b, a);
    }

    int d = (_party & 1) ^ 1;
    for (int i=0; i<n; i++) {
      share_t x = pa_get(v->A, i);
      for (int j=0; j<k; j++) {
        pa_set(c->A, j*n+i, (x & 1) ^ d);
        x >>= 1;
      }
    }
    ans = AND_rec(c, n);
    _free(c); _free(v);
    if (_party == 0) _free(ans);
  }
  if (_party <= 0) {
    ans = _const(n, 0, 2);
    for (int i=0; i<n; i++) {
      pa_set(ans->A, i, pa_get(a->A, i) == pa_get(b->A, i));
    }
  }
  return ans;
}


_b LessThan_bit_channel(_b x, _b y, int channel)
{
  if (_party >  2) return NULL;
  if (order(x) != 2 || order(y) != 2) {
//    printf("LessThan: x->q = %d y->q = %d\n", (int)x->q, (int)y->q);
  }
//  int q = order(x);
  int q = 2;
  int n = len(x);

  _b cx = _dup(x);
  if (_party <= 1) {
    for (int i=0; i<n; i++) {
      pa_set(cx->A, i, MOD(pa_get(x->A, i) + 1));
    }
  }
  if (_party == 2) {
    for (int i=0; i<n; i++) {
      pa_set(cx->A, i, MOD(pa_get(x->A, i)));
    }
  }

  _b cy = _dup(x);
  if (_party <= 1) {
    for (int i=0; i<n; i++) {
      pa_set(cy->A, i, MOD(pa_get(y->A, i)));
    }
  }
  if (_party == 2) {
    for (int i=0; i<n; i++) {
      pa_set(cy->A, i, MOD(pa_get(y->A, i)));
    }
  }
  _b ans = vmul_channel(cx, cy, channel);

  _free(cx);
  _free(cy);

  return ans;
}
#define LessThan_bit(x, y) LessThan_bit_channel(x, y, 0)


_b LessThan_bits_channel(_bits x, _bits y, int channel)
{
  if (_party >  2) return NULL;
  if (x->d != y->d) {
    printf("LessThan_bits: x->d = %d y->d = %d\n", x->d, y->d);
  }
  int d = x->d;
  _b lt = LessThan_bit_channel(x->a[d-1], y->a[d-1], channel);
  _b eq = Equality_bit(x->a[d-1], y->a[d-1]);
  for (int i=d-2; i>=0; i--) {
    _b c = LessThan_bit_channel(x->a[i], y->a[i], channel);
    _b e = Equality_bit(x->a[i], y->a[i]);
    _b l = AND_channel(eq, c, channel);
    _move_(lt, OR_channel(lt, l, channel));
    _move_(eq, AND_channel(eq, e, channel));
    _free(l);
    _free(c);
    _free(e);
  }
  _free(eq);
  return lt;
}
#define LessThan_bits(x, y) LessThan_bits_channel(x, y, 0)


_b LessThan_channel(_ x, _ y, int channel)
{
  if (_party >  2) return NULL;
  _bits bx, by;
  bx = _A2B_channel(x, 2, channel);
  by = _A2B_channel(y, 2, channel);
  _b c = LessThan_bits_channel(bx, by, channel);
  _free_bits(bx);
  _free_bits(by);
  return c;
}
#define LessThan(x, y) LessThan_channel(x, y, 0)

// x <= y
_ LessEqual_channel(_ x, _ y, int channel)
{
  if (_party >  2) return NULL;
  _ ans = Comparison2_channel(x, y, 0); // x <= y
  return ans;
}

// x < y
_ LessThan2_channel(_ x, _ y, int channel)
{
  if (_party >  2) return NULL;
  _ ans_tmp = Comparison2_channel(y, x, 0); // y <= x
  _ ans = vneg(ans_tmp); // y > x
  _free(ans_tmp);
  return ans;
}
#define LessThan2(x, y) LessThan2_channel(x, y, 0)

// x >= y
_ GreaterEqual_channel(_ x, _ y, int channel)
{
  if (_party >  2) return NULL;
  _ ans = Comparison2_channel(y, x, 0); // y <= x
  return ans;
}
#define GreaterEqual(x, y) GreaterEqual_channel(x, y, 0)

// x > y
_ GreaterThan2_channel(_ x, _ y, int channel)
{
  if (_party >  2) return NULL;
  _ ans_tmp = Comparison2_channel(x, y, 0); // x <= y
  _ ans = vneg(ans_tmp); // x > y
  _free(ans_tmp);
  return ans;
}
#define GreaterThan2(x, y) GreaterThan2_channel(x, y, 0)

#endif
