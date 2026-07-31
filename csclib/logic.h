#ifndef LOGIC_H
 #define LOGIC_H

// Dependency: vmul


/////////////////////////////////
// Logical operations
// Inputs must be only 0 or 1
/////////////////////////////////
//////////////////////////////////////////////////
// Bit inversion (undefined for values other than 0 or 1)
//////////////////////////////////////////////////
static share_array vneg(share_array v)
{
  if (v == NULL) {
    printf("vneg: v = %p\n", v);
    return NULL;
  }
  int n = v->n;
  share_t q = v->q;
  NEWT(share_array, ans);
  *ans = *v;
  ans->A = NULL;
  if (_party > max_partyid(v)) return ans;
  ans->A = pa_new_type(v->n, v->A->w, v->A->type);
  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_v = pa_iter_new(v->A);
  for (int i=0; i<n; i++) {
    if (_party < 2) { // works for 22ADD, 33ADD, and SHAMIR
      pa_iter_set(itr_ans, MOD(1 - pa_iter_get(itr_v)));
    } else {
      pa_iter_set(itr_ans, MOD(0 - pa_iter_get(itr_v)));
    }
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_v);
  return ans;
}
#define _vneg vneg

static void vneg_(share_array v)
{
  if (v == NULL) {
    printf("vneg_: v = %p\n", v);
    return;
  }
  share_array tmp = vneg(v);
  pa_free(v->A);  *v = *tmp;  free(tmp);
}
#define _vneg_ vneg_

_ AND_channel(_ a, _ b, int channel)
{
  if (a == NULL || b == NULL) {
    printf("AND: a = %p b = %p\n", a, b);
    return NULL;
  }
  return vmul_channel(a, b, channel);
}
#define AND(a, b) AND_channel(a, b, 0)

_ OR_channel(_ a, _ b, int channel)
{
  if (a == NULL || b == NULL) {
    printf("OR: a = %p b = %p\n", a, b);
    return NULL;
  }
  _ ap = vneg(a);
  _ bp = vneg(b);
  _ ans = AND_channel(ap, bp, channel);
  vneg_(ans);
  _free(ap);
  _free(bp);
  return ans;
}
#define OR(a, b) OR_channel(a, b, 0)

_ XOR2(_ a, _ b)
{
  if (a == NULL || b == NULL) {
    printf("XOR2: a = %p b = %p\n", a, b);
    return NULL;
  }
  if (a->q != 2 || b->q != 2) {
    printf("XOR2: a->q = %d b->q = %d\n", a->q, b->q);
    return NULL;
  }
  _ ans = vadd(a, b);
  return ans;
}

_ XOR_channel(_ a, _ b, int channel)
{
  if (a == NULL || b == NULL) {
    printf("XOR: a = %p b = %p\n", a, b);
    return NULL;
  }
  if (a->q != b->q) {
    printf("XOR: a->q = %d b->q = %d\n", a->q, b->q);
    return NULL;
  }
  if (a->q == 2) return vadd(a, b);
  _ ans = vadd(a, b);
  _ c = vmul_channel(a, b, channel); // TODO can be optimized
  smul_(2, c);
  vsub_(ans, c);
  _free(c);
  return ans;
}
#define XOR(a, b) XOR_channel(a, b, 0)


_ AND_rec_channel(_ x, int n, int channel)
{
  if (x == NULL) {
    printf("AND_rec: x = %p\n", x);
    return NULL;
  }
  int k = len(x) / n;
  if (k == 1) {
    return _dup(x);
  }
  _ first_half  = _slice(x, 0, (k/2)*n);
  _ second_half = _slice(x, (k/2)*n, (k/2)*2*n);
  _ a1 = AND_rec_channel(first_half, n, channel);
  _ a2 = AND_rec_channel(second_half, n, channel);
  _free(first_half); _free(second_half);
  _ ans = AND_channel(a1, a2, channel);
  _free(a1); _free(a2);
  if (k % 2 == 1) {
    _ rest = _slice(x, (k/2)*2*n, k*n);
    _ atmp = AND_channel(ans, rest, channel);
    _free(ans); _free(rest);
    ans = atmp;
  }
  return ans;
}
#define AND_rec(x, n) AND_rec_channel(x, n, 0)

_ OR_rec_channel(_ x, int n, int channel)
{
  if (x == NULL) {
    printf("OR_rec: x = %p\n", x);
    return NULL;
  }
  int k = len(x) / n;
  if (k == 1) {
    return _dup(x);
  }
  _ first_half  = _slice(x, 0, (k/2)*n);
  _ second_half = _slice(x, (k/2)*n, (k/2)*2*n);
  _ a1 = OR_rec_channel(first_half, n, channel);
  _ a2 = OR_rec_channel(second_half, n, channel);
  _free(first_half); _free(second_half);
  _ ans = OR_channel(a1, a2, channel);
  _free(a1); _free(a2);
  if (k % 2 == 1) {
    _ rest = _slice(x, (k/2)*2*n, k*n);
    _ atmp = OR_channel(ans, rest, channel);
    _free(ans); _free(rest);
    ans = atmp;
  }
  return ans;
}
#define OR_rec(x, n) OR_rec_channel(x, n, 0)


#endif
