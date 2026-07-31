#ifndef IF_THEN_H
 #define IF_THEN_H

_ IfThen(_ c, _ x) {
    if (c->q != x->q) {
        printf("IfThen: c->q = %d, x->q = %d\n", c->q, x->q);
        exit(1);
    }
    _ ans = vmul(x, c);

    return ans;
}

// if (f == 1) then a else b
// f * a + (1-f) * b
// = f * (a-b) + b
_ IfThenElse_channel(_ f, _ a, _ b, int channel)
{
  if (_party >  2) return NULL;
  int n = len(f);
  if (n !=len(a) || n != len(b)) {
    printf("IfThenElse f->n = %d a->n = %d b->n = %d\n", n, len(a), len(b));
  }
  _ ans = vsub(a, b);
  vmul_channel_(ans, f, channel);
  vadd_(ans, b);

  return ans;
}
#define IfThenElse(f, a, b) IfThenElse_channel(f, a, b, 0)

#endif
