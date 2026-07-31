#ifndef _BEAVER_H
 #define _BEAVER_H

extern long total_btn, total_bt2;


#define ID_BEAVERTRIPLE1 0x19
#define ID_BEAVERTRIPLE2 0x1A



typedef struct BeaverTriple {
  packed_array a, b, c;
}* BeaverTriple;

typedef struct BT_tables {
  precomp_table T1, T2;
  MMAP *map;
}* BT_tables;

//extern BT_tables BT_tbl[];
BT_tables BT_tbl[MAX_CHANNELS];
long BT_count[MAX_CHANNELS];


//////////////////////////////////////////////////
// Beaver triple computation
// Among a1, a2, b1, b2, c1, c2, only c2 is communicated
// The rest are generated from random seeds
//////////////////////////////////////////////////
BeaverTriple BeaverTriple_new_channel(int n, share_t q, int w, int channel)
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
    pa_iter itr_a1 = pa_iter_new(Aa1);
    pa_iter itr_b1 = pa_iter_new(Ab1);
    pa_iter itr_c1 = pa_iter_new(Ac1);
    for (int i=0; i<n; i++) {
      a1 = RANDOM(mt_[TO_PARTY1][channel], q);
      pa_iter_set(itr_a1, a1);
      b1 = RANDOM(mt_[TO_PARTY1][channel], q);
      pa_iter_set(itr_b1, b1);
      c1 = RANDOM(mt_[TO_PARTY1][channel], q);
      pa_iter_set(itr_c1, c1);
    }
    pa_iter_flush(itr_a1); pa_iter_flush(itr_b1); pa_iter_flush(itr_c1); 

    pa_iter itr_a2 = pa_iter_new(Aa2);
    pa_iter itr_b2 = pa_iter_new(Ab2);
    for (int i=0; i<n; i++) {
      a2 = RANDOM(mt_[TO_PARTY2][channel], q);
      pa_iter_set(itr_a2, a2);
      b2 = RANDOM(mt_[TO_PARTY2][channel], q);
      pa_iter_set(itr_b2, b2);
    }
    pa_iter_flush(itr_a2); pa_iter_flush(itr_b2);

    itr_a1 = pa_iter_new(Aa1);
    itr_a2 = pa_iter_new(Aa2);
    itr_b1 = pa_iter_new(Ab1);
    itr_b2 = pa_iter_new(Ab2);
    itr_c1 = pa_iter_new(Ac1);
    pa_iter itr_c2 = pa_iter_new(Ac2);
    for (int i=0; i<n; i++) {
      a1 = pa_iter_get(itr_a1);
      a2 = pa_iter_get(itr_a2);
      b1 = pa_iter_get(itr_b1);
      b2 = pa_iter_get(itr_b2);
      a = MOD(a1+a2);
      b = MOD(b1+b2);
      c = LMUL(a, b, q);
      c1 = pa_iter_get(itr_c1);
      c2 = MOD(c - c1);
      pa_iter_set(itr_c2, c2);
    }
    pa_iter_flush(itr_c2);
    pa_iter_free(itr_a1); pa_iter_free(itr_b1); pa_iter_free(itr_c1); pa_iter_free(itr_a2); pa_iter_free(itr_b2);

    int size = pa_size(Aa1);
    mpc_send_channel(TO_PARTY2, Ac2->B, pa_size(Ac2), channel);
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
      pa_iter itr_a = pa_iter_new(bt->a);
      pa_iter itr_b = pa_iter_new(bt->b);
      pa_iter itr_c = pa_iter_new(bt->c);
      for (int i=0; i<n; i++) {
        a1 = RANDOM(mt_[FROM_SERVER][channel], q);
        pa_iter_set(itr_a, a1);        //b1 = RANDOM(m0, q);
        b1 = RANDOM(mt_[FROM_SERVER][channel], q);
        pa_iter_set(itr_b, b1);
        c1 = RANDOM(mt_[FROM_SERVER][channel], q);
        pa_iter_set(itr_c, c1);
      }
      pa_iter_flush(itr_a); pa_iter_flush(itr_b); pa_iter_flush(itr_c);
    } else { // party 2
      pa_iter itr_a = pa_iter_new(bt->a);
      pa_iter itr_b = pa_iter_new(bt->b);
      for (int i=0; i<n; i++) {
        a2 = RANDOM(mt_[FROM_SERVER][channel], q);
        pa_iter_set(itr_a, a2);
        b2 = RANDOM(mt_[FROM_SERVER][channel], q);
        pa_iter_set(itr_b, b2);
      }
      pa_iter_flush(itr_a); pa_iter_flush(itr_b);
      mpc_recv_channel(FROM_SERVER, bt->c->B, pa_size(bt->c), channel); // c2
    }
  }

  return bt;
}
#define BeaverTriple_new(n, q, w) BeaverTriple_new_channel(n, q, w, 0)

void BeaverTriple_free(BeaverTriple bt)
{
  if (_party >  2) return;
  free(bt);
}
#define BeaverTriple_GF_free BeaverTriple_free

void BeaverTriple_free_tables(BT_tables T)
{
  if (T == NULL) return;
  if (_party >  2) return;
  if (_party <= 0) return;
  precomp_free(T->T1);
  if (_party == 2) precomp_free(T->T2);
  mymunmap(T->map);
  free(T);
}

void precomp_free_bt(void) {
  for (int i=0; i<_opt.channels; i++) {
    if (BT_tbl[i] != NULL) BeaverTriple_free_tables(BT_tbl[i]);  
  }
}



//////////////////////////////////////////////////
// Beaver triple computation (precomputed)
//////////////////////////////////////////////////
void BeaverTriple_precomp(int n, share_t q, char *fname)
{
  if (_party >  2) return;
  FILE *f1, *f2;

  char *fname1 = precomp_fname(fname, 1);
  char *fname2 = precomp_fname(fname, 2);

  f1 = fopen(fname1, "wb");
  f2 = fopen(fname2, "wb");


  // randomness used by party 1
  unsigned long *init1 = MT_init[1];
  init1[4] = 1; // rand();
  MT m1 = MT_init_by_array(init1, 5);

  precomp_write_seed(f1, n*3, q, init1);

  // randomness used by party 2
  unsigned long *init2 = MT_init[2];
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

    a = MOD(a1+a2);
    b = MOD(b1+b2);
    c = LMUL(a, b, q);
    c2 = MOD(c - c1);
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



///////////////////////////////////////////////////////////
// Load precomputed tables
///////////////////////////////////////////////////////////
BT_tables BeaverTriple_read(char *fname)
{
  if (_party >  2) return NULL;
  if (_party <= 0) {
    NEWT(BT_tables, T);
    T->map = NULL;
    T->T1 = NULL;
    T->T2 = NULL;
    return T;
  }
  char *fname2 = precomp_fname(fname, _party);

  NEWT(BT_tables, T);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  T->T1 = precomp_read(&p);
  if (_party == 2) T->T2 = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}

//////////////////////////////////////////////////
// Beaver triple computation (uses precomputation, newer version)
// Prepare BT_tables per channel
//////////////////////////////////////////////////
BeaverTriple BeaverTriple_new3(int n, share_t q, BT_tables T)
{
  if (_party >  2) return NULL;
  NEWT(BeaverTriple, bt);

  if (_party <= 0) {
    bt->a = NULL;
    bt->b = NULL;
    bt->c = NULL;
    return bt;
  }

  share_t a1, a2, b1, b2, c1, c2;
  int k = blog(q-1)+1;
  bt->a = pa_new(n, k);
  bt->b = pa_new(n, k);
  bt->c = pa_new(n, k);
  pa_iter itr_a = pa_iter_new(bt->a);
  pa_iter itr_b = pa_iter_new(bt->b);
  pa_iter itr_c = pa_iter_new(bt->c);
  if (_party == 1) {
    for (int i=0; i<n; i++) {
      a1 = precomp_get(T->T1) % q;
      pa_iter_set(itr_a, a1);
      b1 = precomp_get(T->T1) % q;
      pa_iter_set(itr_b, b1);
      c1 = precomp_get(T->T1) % q;
      pa_iter_set(itr_c, c1);
    }
  } else { // party 2
    for (int i=0; i<n; i++) {
      a2 = precomp_get(T->T1) % q;
      pa_iter_set(itr_a, a2);
      b2 = precomp_get(T->T1) % q;
      pa_iter_set(itr_b, b2);
      c2 = precomp_get(T->T2) % q;
      pa_iter_set(itr_c, c2);
    }
  }
  pa_iter_flush(itr_a);
  pa_iter_flush(itr_b);
  pa_iter_flush(itr_c);
  return bt;
}

void bt_tbl_init(void)
{
  for (int i=0; i<_opt.channels; i++) {
    BT_tbl[i] = NULL;
    BT_count[i] = 0;
  }
}

void bt_tbl_read(int channel, char *fname)
{
  BT_tbl[channel] = BeaverTriple_read(fname);
}


#endif
