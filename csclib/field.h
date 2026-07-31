#ifndef _FIELD_H
 #define _FIELD_H

#define GFADD(x, y, irr_poly) ((irr_poly)? (x)^(y) : MOD((x)+(y)))
#define GFSUB(x, y, irr_poly) ((irr_poly)? (x)^(y) : MOD((x)-(y)))

/////////////////////////////////////////////////////////////////////////////
// Multiplication in GF(2^degree) (irreducible polynomial irr_poly)
// For X^4 + X + 1, irr_poly = 0x13
// For X^8 + X^4 + X^3 + X + 1, irr_poly = 0x11b
/////////////////////////////////////////////////////////////////////////////
static share_t GF_mul(share_t a, share_t b, share_t irr_poly)
{
  share_t c = 0, msb;

  int degree = blog(irr_poly);
  share_t msb_mask = 1 << (degree-1);

  for (int i=0; i<degree; i++) {
    if (b & 1)
      c ^= a;

    msb = a & msb_mask;
    a <<= 1;
    if (msb)
      a ^= irr_poly;
    b >>= 1;
  }

  return c;
}



typedef struct GF_tbl_list {
  BT_tables tbl;
  share_t irr_poly;
  long count;
  struct GF_tbl_list *next;
}* GF_tbl_list;

GF_tbl_list GF_tbl_list_insert(BT_tables tbl, share_t irr_poly, GF_tbl_list head)
{
  NEWT(GF_tbl_list, list);
  list->tbl = tbl;
  list->irr_poly = irr_poly;
  list->count = 0;
  list->next = head;
  return list;
}

BT_tables GF_tbl_list_search(GF_tbl_list list, share_t irr_poly)
{
  BT_tables ans = NULL;
  while (list != NULL) {
    if (list->irr_poly == irr_poly) {
      ans = list->tbl;
      break;
    }
    list = list->next;
  }
  return ans;
}

void GF_tbl_list_free(GF_tbl_list list)
{
  GF_tbl_list next;
  while (list != NULL) {
    next = list->next;
    BeaverTriple_free_tables(list->tbl);
    free(list);
    list = next;
  }
}

GF_tbl_list PRE_GF_tbl[MAX_CHANNELS];
long PRE_GF_count[MAX_CHANNELS];

void GF_tbl_init(void)
{
  for (int i=0; i<_opt.channels; i++) {
    PRE_GF_tbl[i] = NULL;
    PRE_GF_count[i] = 0;
  }
}

void GF_tbl_read(int channel, share_t irr_poly, char *fname)
{
  BT_tables tbl = BeaverTriple_read(fname);
  PRE_GF_tbl[channel] = GF_tbl_list_insert(tbl, irr_poly, PRE_GF_tbl[channel]);
}


static _ vadd_GF(_ a, _ b)
{
  if (_party > max_partyid(a)) return NULL;
  int n = a->n;
  share_t q = order(a);
  if (b->n != n || order(b) != q) {
    printf("vadd_GF: len %d %d order %d %d\n", n, b->n, q, order(b));
    exit(1);
  }
  _ ans = share_const_type(len(a), 0, q, a->type);
  ans->irr_poly = a->irr_poly;
  for (int i=0; i<n; i++) {
    share_t c;
    c = pa_get(a->A, i) ^ pa_get(b->A, i);
    pa_set(ans->A, i, c);
  }
  return ans;
}

static _ vadd_shamir_GF(_ a, _ b)
{
  if (_party > 3) return NULL;
  int n = len(a);
  share_t q = order(a);
  if (len(b) != n || order(b) != q) {
    printf("vadd_GF: len %d %d order %d %d\n", n, len(b), q, order(b));
    exit(1);
  }
  _ ans = share_const_type(n, 0, q, a->type);
  for (int i=0; i<n; i++) {
    share_t c;
    c = pa_get(a->A, i) ^ pa_get(b->A, i);
    pa_set(ans->A, i, c);
  }
  return ans;
}

static _ smul_GF(share_t s, _ a, share_t irr_poly)
{
  if (_party > max_partyid(a)) return NULL;
  int n = a->n;
  share_t q = order(a);
  _ ans = share_const_type(n, 0, q, a->type);
  ans->irr_poly = a->irr_poly;
  for (int i=0; i<n; i++) {
    share_t c;
    c = GF_mul(s, pa_get(a->A, i), irr_poly);
    pa_set(ans->A, i, c);
  }
  return ans;
}

static _ smul_shamir_GF(share_t s, _ a, share_t irr_poly)
{
  if (_party >  3) return NULL;
  int n = len(a);
  share_t q = order(a);
  //_ ans = share_const_type(n, 0, q, SHARE_T_SHAMIR);
  _ ans = share_const_type(n, 0, q, a->type);
  ans->irr_poly = a->irr_poly;
  for (int i=0; i<n; i++) {
    share_t c;
    c = GF_mul(s, pa_get(a->A, i), irr_poly);
    pa_set(ans->A, i, c);
  }
  return ans;
}

BeaverTriple BeaverTriple_GF_new_channel(int n, share_t q, int w, share_t irr_poly, int channel)
{
  if (_party >  2) return NULL;
  if (q == 2) {
    total_bt2 += n;
  } else {
    total_btn += n;
  }
  NEWT(BeaverTriple, bt);

  if (_party <= 0) {
    packed_array Aa1, Aa2, Ab1, Ab2, Ac1, Ac2;
    Aa1 = pa_new(n, w);
    Aa2 = pa_new(n, w);
    Ab1 = pa_new(n, w);
    Ab2 = pa_new(n, w);
    Ac1 = pa_new(n, w);
    Ac2 = pa_new(n, w);
    share_t a, b, c;
    share_t a1, a2, b1, b2, c1, c2;
    for (int i=0; i<n; i++) {
      a1 = RANDOM(mt_[TO_PARTY1][channel], q);
      pa_set(Aa1, i, a1);
      b1 = RANDOM(mt_[TO_PARTY1][channel], q);
      pa_set(Ab1, i, b1);
      c1 = RANDOM(mt_[TO_PARTY1][channel], q);
      pa_set(Ac1, i, c1);
    }
    for (int i=0; i<n; i++) {
      a2 = RANDOM(mt_[TO_PARTY2][channel], q);
      pa_set(Aa2, i, a2);
      b2 = RANDOM(mt_[TO_PARTY2][channel], q);
      pa_set(Ab2, i, b2);
    }
    for (int i=0; i<n; i++) {
      a1 = pa_get(Aa1, i);
      a2 = pa_get(Aa2, i);
      b1 = pa_get(Ab1, i);
      b2 = pa_get(Ab2, i);
      a = a1 ^ a2;
      b = b1 ^ b2;
      c = GF_mul(a, b, irr_poly);
      c1 = pa_get(Ac1, i);
      c2 = c ^ c1;
      pa_set(Ac2, i, c2);
    }
    mpc_send_pa_channel(TO_PARTY2, Ac2, channel);
    pa_free(Aa1);  pa_free(Aa2);
    pa_free(Ab1);  pa_free(Ab2);
    pa_free(Ac1);  pa_free(Ac2);
    bt->a = NULL;
    bt->b = NULL;
    bt->c = NULL;
  } else {
    share_t a1, a2, b1, b2, c1;
    bt->a = pa_new(n, w);
    bt->b = pa_new(n, w);
    bt->c = pa_new(n, w);
    int size = pa_size(bt->a);
    if (_party == 1) {
      for (int i=0; i<n; i++) {
        a1 = RANDOM(mts[channel], q);
        pa_set(bt->a, i, a1);
        b1 = RANDOM(mts[channel], q);
        pa_set(bt->b, i, b1);
        c1 = RANDOM(mts[channel], q);
        pa_set(bt->c, i, c1);
      }
    } else { // party 2
      for (int i=0; i<n; i++) {
        a2 = RANDOM(mts[channel], q);
        pa_set(bt->a, i, a2);
        b2 = RANDOM(mts[channel], q);
        pa_set(bt->b, i, b2);
      }
      mpc_recv_pa_channel(FROM_SERVER, bt->c, channel); // c2
    }
  }

  return bt;
}

static share_array vmul_GF_channel(share_array x, share_array y, share_t irr_poly, int channel)
{
  if (_party >  2) return NULL;
  int n = x->n;
  share_t q = x->q;
  int i;
  if (x->n != y->n) {
    printf("vmul_GF_channel: x->n = %d y->n = %d\n", x->n, y->n);
  }
  if (x->q != y->q) {
    printf("vmul_GF_channel: x->q = %d y->q = %d\n", (int)x->q, (int)y->q);
  }
  NEWT(share_array, ans);
  *ans = *x;
  ans->A = pa_new(n, x->A->w);

  if (_party <= 0) {
    for (i=0; i<n; i++) {
      pa_set(ans->A, i, GF_mul(pa_get(x->A,i), pa_get(y->A,i), irr_poly));
    }
    if (_party == -1) return ans;
  }

// Beaver Triple computation
  BeaverTriple bt;
  BT_tables tbl;

  if ((tbl = GF_tbl_list_search(PRE_GF_tbl[channel], irr_poly)) != NULL) {
    bt = BeaverTriple_new3(n, q, tbl); // precomputation
  } else {
    bt = BeaverTriple_GF_new_channel(n, q, x->A->w, irr_poly, channel);
  }
  if (_party <= 0) return ans;

  NEWT(share_array, a);
  *a = *x;
  a->A = bt->a;
  NEWT(share_array, b);
  *b = *x;
  b->A = bt->b;

  share_array sigma, rho;
  sigma = vadd_GF(x, a);
  rho = vadd_GF(y, b);
  share_array sigma_c, rho_c;
  sigma_c = share_reconstruct_xor_channel(sigma, channel); //
  rho_c = share_reconstruct_xor_channel(rho, channel); //
  for (i=0; i<n; i++) {
    share_t tmp;
    if (_party == 1) {
      tmp = GF_mul(pa_get(sigma_c->A, i), pa_get(rho_c->A, i), irr_poly);
    } else {
      tmp = 0;
    }
    tmp = tmp ^ GF_mul(pa_get(a->A,i), pa_get(rho_c->A,i), irr_poly);  
    tmp = tmp ^ GF_mul(pa_get(b->A,i), pa_get(sigma_c->A,i), irr_poly);
    tmp = tmp ^ pa_get(bt->c,i);
    pa_set(ans->A, i, tmp);
  }
  pa_free(bt->c);
  share_free(a);  share_free(b);
  share_free(sigma); share_free(rho);
  share_free(sigma_c); share_free(rho_c);

  BeaverTriple_GF_free(bt);

  return ans;
}
#define vmul_GF(x, y, irr_poly) vmul_GF_channel(x, y, irr_poly, 0)

static share_t GF_mul(share_t a, share_t b, share_t irr_poly);
static share_array vmul_GF_channel(share_array x, share_array y, share_t irr_poly, int channel);


void BeaverTriple_GF_precomp(int n, share_t irr_poly, char *fname)
{
  if (_party >  2) return;
  FILE *f1, *f2;

  char *fname1 = precomp_fname(fname, 1);
  char *fname2 = precomp_fname(fname, 2); // should we avoid creating a file for party 0?

  f1 = fopen(fname1, "wb");
  f2 = fopen(fname2, "wb");

  int d = blog(irr_poly);
  share_t q = 1 << d;

  // randomness used by party 1
  unsigned long init1[5]={0x123, 0x234, 0x345, 0x456, 0};
  init1[4] = 1; // rand();
  MT m1 = MT_init_by_array(init1, 5);

  precomp_write_seed(f1, n*3, q, init1);

  // randomness used by party 2
  unsigned long init2[5]={0x123, 0x234, 0x345, 0x456, 0};
  init2[4] = 2; // rand();
  MT m2 = MT_init_by_array(init2, 5);

  int k = blog(q-1)+1;
  packed_array Ac2 = pa_new(n, k);

  share_t a, b, c;
  share_t a1, a2, b1, b2, c1, c2;
  for (int i=0; i<n; i++) {
    a1 = RANDOM(m1, q);
    b1 = RANDOM(m1, q);
    c1 = RANDOM(m1, q);

    a2 = RANDOM(m2, q);
    b2 = RANDOM(m2, q);

    a = a1 ^ a2;
    b = b1 ^ b2;
    c = GF_mul(a, b, irr_poly);
    c2 = c ^ c1;
    pa_set(Ac2, i, c2);
  }

  precomp_write_seed(f2, n*2, q, init2);
  precomp_write_pa(f2, Ac2, q);

  pa_free(Ac2);
  MT_free(m1);
  MT_free(m2);

  fclose(f1);
  fclose(f2);

  free(fname1);
  free(fname2);
}

#endif // _FIELD_H
