#ifndef _FUNC_H
 #define _FUNC_H

/*
  Various function and lookup-table computations
  - overflow
  - one-hot  
  - B2A
*/

precomp_tables PRE_B2A_tbl[MAX_CHANNELS];
precomp_tables PRE_OF_tbl[OF_MAX][MAX_CHANNELS];
precomp_tables PRE_OHA_tbl[ONEHOT_MAX][MAX_CHANNELS];
precomp_tables PRE_OHX_tbl[ONEHOT_MAX][MAX_CHANNELS];
precomp_tables PRE_OHS_tbl[ONEHOT_MAX][MAX_CHANNELS];
precomp_tables PRE_OHS3_tbl[ONEHOT_MAX][MAX_CHANNELS];
precomp_tables PRE_OHR_tbl[ONEHOT_MAX][MAX_CHANNELS];
 
long PRE_B2A_count[MAX_CHANNELS];
long PRE_OF_count[OF_MAX][MAX_CHANNELS];
long PRE_OHA_count[ONEHOT_MAX][MAX_CHANNELS];
long PRE_OHX_count[ONEHOT_MAX][MAX_CHANNELS];
long PRE_OHS_count[ONEHOT_MAX][MAX_CHANNELS];
long PRE_OHR_count[ONEHOT_MAX][MAX_CHANNELS];


void precomp_free_func(void) {
  for (int i=0; i<_opt.channels; i++) {
    if (PRE_B2A_tbl[i] != NULL) precomp_free_tables(PRE_B2A_tbl[i]);
    for (int j=1; j<=OF_MAX; j++) {
      if (PRE_OF_tbl[j-1][i] != NULL) precomp_free_tables(PRE_OF_tbl[j-1][i]);
    }
    for (int j=1; j<=ONEHOT_MAX; j++) {
      if (PRE_OHA_tbl[j-1][i] != NULL) precomp_free_tables(PRE_OHA_tbl[j-1][i]);
      if (PRE_OHX_tbl[j-1][i] != NULL) precomp_free_tables(PRE_OHX_tbl[j-1][i]);
    }
  }  
}


typedef struct precomp_tbl_list {
  precomp_tables tbl;
  int d;
  share_t irr_poly;
  long count;
  struct precomp_tbl_list *next;
}* precomp_tbl_list;

precomp_tbl_list PRE_RE_tbl[MAX_CHANNELS];

precomp_tbl_list precomp_tbl_list_insert(precomp_tables tbl, int d, share_t irr_poly, precomp_tbl_list head)
{
  NEWT(precomp_tbl_list, list);
  list->tbl = tbl;
  list->d = d;
  list->irr_poly = irr_poly;
  list->count = 0;
  list->next = head;
  return list;
}

precomp_tables precomp_tbl_list_search(precomp_tbl_list list, int d, share_t irr_poly)
{
  precomp_tables ans = NULL;
  while (list != NULL) {
    if (list->d == d && list->irr_poly == irr_poly) {
      ans = list->tbl;
      break;
    }
    list = list->next;
  }
  return ans;
}

void precomp_tbl_list_free(precomp_tbl_list list)
{
  precomp_tbl_list next;
  while (list != NULL) {
    next = list->next;
    precomp_free_tables(list->tbl);
    free(list);
    list = next;
  }
}

///////////////////////////////////////////////////////////////////////////////////////
// 1-bit function computation
///////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// Compute f([x]_1, [x]_2)
// Is it difficult to simulate this computation on a single machine?
////////////////////////////////////////////////////////
_ func1bit(_ x, share_t q, share_t *func_table)
{
  int n = x->n;
  int b = 1;
  int k = 1 << b; // table size

  _ R = NULL, t = NULL;

// Precomputation
  if (_party <= 0) {
    for (int i=0; i<k; i++) {
      for (int j=0; j<k; j++) {
      }
    }
    // Compute tables for P1 and P2

    _ F = _const(n*k*k, 0, q);
    // Random values for masking x1 and x2
    _ R = _const(n, 0, k);
    _ S = _const(n, 0, k);
    unsigned long *init1 = MT_init[1];
    MT m1 = MT_init_by_array(init1, 5);
    unsigned long *init2 = MT_init[2];
    MT m2 = MT_init_by_array(init2, 5);
    for (int p=0; p<n; p++) {
      share_t r = RANDOM(m1, k);
      share_t s = RANDOM(m2, k);
      pa_set(R->A, p, r);
      pa_set(S->A, p, s);
      for (int i=0; i<k; i++) {
        for (int j=0; j<k; j++) {
          pa_set(F->A, p*k*k + i*k + j, func_table[(i^r)*k + (j^s)]);
        }
      }
    }
    MT_free(m1);
    MT_free(m2);

    _ t0 = _const(n*k*k, 0, q);
    _ t1 = _const(n*k*k, 0, q);
    _ t2 = _const(n*k*k, 0, q);
    unsigned long init0[5]={0x123, 0x234, 0x345, 0x456, 1};
    MT m0 = MT_init_by_array(init0, 5);
    for (int p=0; p<n; p++) {
      for (int i=0; i<k; i++) {
        for (int j=0; j<k; j++) {
          share_t rr = RANDOM(m0, q);
          pa_set(t1->A, p*k*k + i*k + j, rr);
          pa_set(t2->A, p*k*k + i*k + j, MOD(pa_get(F->A, p*k*k + i*k + j)-rr));
        }
      }
    }
    MT_free(m0);

    if (_party == 0) {
      mpc_send_share(TO_PARTY1, R);
      mpc_send_share(TO_PARTY2, S);
      mpc_send_share(TO_PARTY1, t1);
      mpc_send_share(TO_PARTY2, t2);
    }
  } else {
    R = _const(n, 0, k);
    mpc_recv_share(FROM_SERVER, R);
    t = _const(n*k*k, 0, q);
    mpc_recv_share(FROM_SERVER, t);

  }

// Main computation
  _ ans = _const(n, 0, q);
  if (_party <= 0) {
  } else {
    _ y = _const(n, 0, k);
    for (int p=0; p<n; p++) {
      pa_set(y->A, p, pa_get(R->A, p) ^ pa_get(x->A, p));
    }
    _ z = _const(n, 0, k);
    mpc_exchange_share(y, z);
    if (_party == 1) {
      for (int p=0; p<n; p++) {
      //  share_t r = pa_get(R->A, p);
        share_t xr = pa_get(y->A, p);
        share_t ys = pa_get(z->A, p);
        pa_set(ans->A, p, pa_get(t->A, p*k*k + xr*k + ys));
      }
    }
    if (_party == 2) {
      for (int p=0; p<n; p++) {
        share_t xr = pa_get(z->A, p);
        share_t ys = pa_get(y->A, p);
        pa_set(ans->A, p, pa_get(t->A, p*k*k + xr*k + ys));
      }      
    }
  }
  return ans;
}


///////////////////////////////////////////////////////////
// Build precomputation tables
///////////////////////////////////////////////////////////
void func1bit3_precomp(int n, share_t q, share_t *func_table, char *fname)
{
  int b = 1;
  int k = 1 << b; // table size

  if (_party > 0) goto sync;

  char *fname0 = precomp_fname(fname, 0);
  char *fname1 = precomp_fname(fname, 1);
  char *fname2 = precomp_fname(fname, 2);

  for (int i=0; i<k; i++) {
    for (int j=0; j<k; j++) {
    }
  }
  // Compute tables for P1 and P2

  _ F = _const(n*k*k, 0, q);
  _ R = _const(n, 0, k);
  _ S = _const(n, 0, k);
  unsigned long *init1 = MT_init[1];
  MT m1 = MT_init_by_array(init1, 5);
  unsigned long *init2 = MT_init[2];
  MT m2 = MT_init_by_array(init2, 5);
  for (int p=0; p<n; p++) {
    share_t r = RANDOM(m1, k);
    share_t s = RANDOM(m2, k);
    pa_set(R->A, p, r);
    pa_set(S->A, p, s);
    for (int i=0; i<k; i++) {
      for (int j=0; j<k; j++) {
        pa_set(F->A, p*k*k + i*k + j, func_table[(i^r)*k + (j^s)]);
      }
    }
  }
  MT_free(m1);
  MT_free(m2);

  FILE *f0, *f1, *f2;
  f0 = fopen(fname0, "wb");
  f1 = fopen(fname1, "wb");
  f2 = fopen(fname2, "wb");
  precomp_write_seed(f1, n, q, init1);
  precomp_write_seed(f2, n, q, init2);
  precomp_write_seed(f0, n, q, init1);



  _ t1 = _const(n*k*k, 0, q);
  _ t2 = _const(n*k*k, 0, q);
  unsigned long *init0 = MT_init[0];
  MT m0 = MT_init_by_array(init0, 5);
  for (int p=0; p<n; p++) {
    for (int i=0; i<k; i++) {
      for (int j=0; j<k; j++) {
        share_t rr = RANDOM(m0, q);
        pa_set(t1->A, p*k*k + i*k + j, rr);
        pa_set(t2->A, p*k*k + i*k + j, MOD(pa_get(F->A, p*k*k + i*k + j)-rr));
      }
    }
  }
  MT_free(m0);
  precomp_write_seed(f1, n*k*k, q, init0);
  precomp_write_share(f2, t2);

// P0 stores the table as-is
  _ t0 = _const(k, 0, q);
  for (int i=0; i<k; i++) {
    pa_set(t0->A, i, func_table[i*k+0]);
  }
  precomp_write_share(f0, t0);

  fclose(f0);
  fclose(f1);
  fclose(f2);

  _free(F);  _free(R);  _free(S);
  _free(t0);  _free(t1);  _free(t2);
  free(fname0);  free(fname1);  free(fname2);

sync:;

}




///////////////////////////////////////////////////////////
// Load precomputation tables
///////////////////////////////////////////////////////////
precomp_tables func1bit3_read(char *fname)
{
  if (_party >  2) return NULL;
  int party = _party;
  if (_party < 0) {
    party = 0;
  }
  char *fname2 = precomp_fname(fname, party);

  NEWT(precomp_tables, T);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  T->TR = precomp_read(&p);
  T->Tt = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}

///////////////////////////////////////////////////////////
// Compute using precomputation tables
///////////////////////////////////////////////////////////
_ func1bit3_channel(_ x, share_t q, precomp_tables T, int channel)
{
  int n = x->n;
  int b = 1;
  int k = 1 << b; // table size

  _ ans = _const(n, 0, q);

  if (_party <= 0) {
    pa_iter itr_ans = pa_iter_new(ans->A);
    pa_iter itr_x = pa_iter_new(x->A);
    for (int p=0; p<n; p++) {
      share_t xx = pa_iter_get(itr_x) % k;
      pa_iter_set(itr_ans, pa_get(T->Tt->u.share.a, xx)%q);
    }
    pa_iter_flush(itr_ans); pa_iter_free(itr_x);
    return ans;
  }


  _ t = _const(n*k*k, 0, q);
  pa_iter itr_t = pa_iter_new(t->A);
  for (int p=0; p<n*k*k; p++) {
    pa_iter_set(itr_t, precomp_get(T->Tt)%q);
  }
  pa_iter_flush(itr_t);
  _ y = _const(n, 0, k);
  pa_iter itr_y = pa_iter_new(y->A);
  pa_iter itr_x = pa_iter_new(x->A);
  for (int p=0; p<n; p++) {
    pa_iter_set(itr_y, (precomp_get(T->TR) ^ pa_iter_get(itr_x))%k);
  }
  pa_iter_flush(itr_y); pa_iter_free(itr_x);
  _ z = _const(n, 0, k);
  mpc_exchange_share_channel(y, z, channel);
  if (_party == 1) {
    pa_iter itr_y = pa_iter_new(y->A);
    pa_iter itr_z = pa_iter_new(z->A);
    pa_iter itr_ans = pa_iter_new(ans->A);
    for (int p=0; p<n; p++) {
      share_t xr = pa_iter_get(itr_y);
      share_t ys = pa_iter_get(itr_z);
      pa_iter_set(itr_ans, pa_get(t->A, p*k*k + xr*k + ys)%q);
    }
    pa_iter_flush(itr_ans); pa_iter_free(itr_y); pa_iter_free(itr_z);
  }
  if (_party == 2) {
    pa_iter itr_y = pa_iter_new(y->A);
    pa_iter itr_z = pa_iter_new(z->A);
    pa_iter itr_ans = pa_iter_new(ans->A);
    for (int p=0; p<n; p++) {
      share_t xr = pa_iter_get(itr_z);
      share_t ys = pa_iter_get(itr_y);
      pa_iter_set(itr_ans, pa_get(t->A, p*k*k + xr*k + ys)%q);
    }
    pa_iter_flush(itr_ans); pa_iter_free(itr_y); pa_iter_free(itr_z);
  }
  _free(y);  _free(z);  _free(t);
  return ans;
}
#define func1bit3(x, q, T) func1bit3_channel(x, q, T, 0)
#define func1bit(x, q, T) func1bit3_channel(x, q, T, 0)
#define func1bit_channel(x, q, T, c) func1bit3_channel(x, q, T, c)

///////////////////////////////////////////////////////////////////////////////////////
// k-bit function computation
///////////////////////////////////////////////////////////////////////////////////////

void funckbit_precomp(int b, int n, share_t q, share_t *func_table, char *fname)
{
  if (_party > 0) goto sync;
  int k = 1 << b; // table size


  char *fname0 = precomp_fname(fname, 0);
  char *fname1 = precomp_fname(fname, 1);
  char *fname2 = precomp_fname(fname, 2);

  for (int i=0; i<k; i++) {
    for (int j=0; j<k; j++) {
    }
  }
  // Compute tables for P1 and P2

  _ F = _const(n*k*k, 0, q);
  // Random values for masking x1 and x2
  _ R = _const(n, 0, k);
  _ S = _const(n, 0, k);
  unsigned long *init1 = MT_init[1];
  MT m1 = MT_init_by_array(init1, 5);
  unsigned long *init2 = MT_init[2];
  MT m2 = MT_init_by_array(init2, 5);
  for (int p=0; p<n; p++) {
    share_t r = RANDOM(m1, k);
    share_t s = RANDOM(m2, k);
    pa_set(R->A, p, r);
    pa_set(S->A, p, s);
    for (int i=0; i<k; i++) {
      for (int j=0; j<k; j++) {
        pa_set(F->A, p*k*k + i*k + j, func_table[(i^r)*k + (j^s)]);
      }
    }
  }
  MT_free(m1);
  MT_free(m2);

  FILE *f0, *f1, *f2;
  f0 = fopen(fname0, "wb");
  f1 = fopen(fname1, "wb");
  f2 = fopen(fname2, "wb");
  precomp_write_seed(f1, n, q, init1);
  precomp_write_seed(f2, n, q, init2);
  precomp_write_seed(f0, n, q, init1);

  _ t1 = _const(n*k*k, 0, q);
  _ t2 = _const(n*k*k, 0, q);
  unsigned long *init0 = MT_init[0];
  MT m0 = MT_init_by_array(init0, 5);
  for (int p=0; p<n; p++) {
    for (int i=0; i<k; i++) {
      for (int j=0; j<k; j++) {
        share_t rr = RANDOM(m0, q);
        pa_set(t1->A, p*k*k + i*k + j, rr);
        pa_set(t2->A, p*k*k + i*k + j, MOD(pa_get(F->A, p*k*k + i*k + j)-rr));
      //  pa_set(t2->A, p*k*k + i*k + j, MOD(pa_get(F->A, p*k*k + i*k + j) ^ rr)); // use XOR instead of additive shares?
      }
    }
  }
  MT_free(m0);
  precomp_write_seed(f1, n*k*k, q, init0);
  precomp_write_share(f2, t2);

// P0 stores the table as-is
  _ t0 = _const(k, 0, q);
  for (int i=0; i<k; i++) {
    pa_set(t0->A, i, func_table[i*k+0]);
  }
  precomp_write_share(f0, t0);

  fclose(f0);
  fclose(f1);
  fclose(f2);

  _free(F);  _free(R);  _free(S);
  _free(t0);  _free(t1);  _free(t2);
  free(fname0);  free(fname1);  free(fname2);

sync:;

}

precomp_tables funckbit_read(char *fname)
{
  if (_party > 2) return NULL;
  char *fname2 = precomp_fname(fname, _party);

  NEWT(precomp_tables, T);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  T->TR = precomp_read(&p);
  T->Tt = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}

_ funckbit_channel(int b, _ x, share_t q, precomp_tables T, int channel)
{
  int n = x->n;
  int k = 1 << b; // table size

  _ ans = _const(n, 0, q);

  if (_party <= 0) {
    for (int p=0; p<n; p++) {
      share_t xx = pa_get(x->A, p);
      pa_set(ans->A, p, pa_get(T->Tt->u.share.a, xx*k + 0)%q);
    }
    return ans;
  }


  _ t = _const(n*k*k, 0, q);
  for (int p=0; p<n*k*k; p++) {
    pa_set(t->A, p, precomp_get(T->Tt)%q);
  }
  _ y = _const(n, 0, k);
  for (int p=0; p<n; p++) {
    pa_set(y->A, p, (precomp_get(T->TR) ^ pa_get(x->A, p))%k); // x is an additive share; is XOR still valid?
  }
  _ z = _const(n, 0, k);
  mpc_exchange_share_channel(y, z, channel);
  if (_party == 1) {
    for (int p=0; p<n; p++) {
      share_t xr = pa_get(y->A, p);
      share_t ys = pa_get(z->A, p);
      pa_set(ans->A, p, pa_get(t->A, p*k*k + xr*k + ys)%q);
    }
  }
  if (_party == 2) {
    for (int p=0; p<n; p++) {
      share_t xr = pa_get(z->A, p);
      share_t ys = pa_get(y->A, p);
      pa_set(ans->A, p, pa_get(t->A, p*k*k + xr*k + ys)%q);
    }      
  }
  _free(y);  _free(z);  _free(t);
  return ans;
}
#define funckbit(b, x, q, T) funckbit_channel(b, x, q, T, 0)


_ funckbit_online_channel(int b, _ x, share_t q, share_t *func_table, int channel)
{
  int n = len(x);
  funckbit_precomp(b, n, q, func_table, "funcktmp.dat");
  precomp_tables tbl = funckbit_read("funcktmp.dat");
  return funckbit_channel(b, x, q, tbl, channel);
}
#define funckbit_online(b, x, q, func_table) funckbit_online_channel(b, x, q, func_table, 0)

///////////////////////////////////////////////////////////////////////////////////////
// Definitions of concrete functions
///////////////////////////////////////////////////////////////////////////////////////

void of_tbl_init(void)
{
  for (int i=0; i<_opt.channels; i++) {
    for (int j=1; j<=OF_MAX; j++) {
      PRE_OF_tbl[j-1][i] = NULL;
      PRE_OF_count[j-1][i] = 0;
    }
  }
}

void of_tbl_read(int d, int channel, char *fname)
{
  PRE_OF_tbl[d-1][channel] = func1bit3_read(fname);
}

void b2a_tbl_init(void)
{
  for (int i=0; i<_opt.channels; i++) {
    PRE_B2A_tbl[i] = NULL;
    PRE_B2A_count[i] = 0;
  }
}

void b2a_tbl_read(int channel, char *fname)
{
  PRE_B2A_tbl[channel] = func1bit3_read(fname);
}

void onehot_tbl_init(void)
{
  for (int i=0; i<_opt.channels; i++) {
    for (int j=1; j<=ONEHOT_MAX; j++) {
      PRE_OHA_tbl[j-1][i] = NULL;
      PRE_OHX_tbl[j-1][i] = NULL;
      PRE_OHS_tbl[j-1][i] = NULL;
      PRE_OHR_tbl[j-1][i] = NULL;
      PRE_OHA_count[j-1][i] = 0;
      PRE_OHX_count[j-1][i] = 0;
      PRE_OHS_count[j-1][i] = 0;
      PRE_OHR_count[j-1][i] = 0;
    }
  }
}

precomp_tables onehotvec_read(char *fname)
{
  int party = _party;
  if (party < 0) party = 0;
  char *fname2 = precomp_fname(fname, party);

  NEWT(precomp_tables, T);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  T->TR = precomp_read(&p);
  T->Tt = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}

precomp_tables onehotvec_shamir3_read(char *fname)
{
  int party = _party;
  if (party < 0) party = 0;
  char *fname2 = precomp_fname(fname, party);

  NEWT(precomp_tables, T);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  T->TR = precomp_read(&p);
  T->Tt = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}

precomp_tables onehotvec_rss_read(char *fname)
{
  int party = _party;
  if (party < 0) party = 0;
  char *fname2 = precomp_fname(fname, party);

  NEWT(precomp_tables, T);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  T->TR = precomp_read(&p);
  T->Tt = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}


void onehot_tbl_read(int d, int xor, int channel, char *fname)
{
  if (d < 1 || d > ONEHOT_MAX) {
    printf("onehot_tbl_read: d = %d MAX = %d\n", d, ONEHOT_MAX);
    exit(1);
  }
  if (xor) {
    PRE_OHX_tbl[d-1][channel] = onehotvec_read(fname);
  } else {
    PRE_OHA_tbl[d-1][channel] = onehotvec_read(fname);
  }
}

void onehot_shamir_tbl_read(int d, int channel, char *fname)
{
  if (d < 1 || d > ONEHOT_MAX) {
    printf("onehot_shamir_tbl_read: d = %d MAX = %d\n", d, ONEHOT_MAX);
    exit(1);
  }
  PRE_OHS_tbl[d-1][channel] = onehotvec_read(fname);
}

void onehot_rss_tbl_read(int d, share_t irr_poly, int channel, char *fname)
{
  if (d < 1 || d > ONEHOT_MAX) {
    printf("onehot_rss_tbl_read: d = %d MAX = %d\n", d, ONEHOT_MAX);
    exit(1);
  }
  PRE_OHR_tbl[d-1][channel] = onehotvec_rss_read(fname);
}

void onehot_shamir3_tbl_read(int d, share_t irr_poly, int channel, char *fname)
{
  if (d < 1 || d > ONEHOT_MAX) {
    printf("onehot_shamir3_tbl_read: d = %d MAX = %d\n", d, ONEHOT_MAX);
    exit(1);
  }
  PRE_OHS3_tbl[d-1][channel] = onehotvec_shamir3_read(fname);
}


void onehotvec_precomp(int b, int n, share_t q, char *fname, int xor)
{
  int k = 1 << b; // table size
  int w = 1 << b; // vector length

  if (_party > 0) goto sync;

  char *fname0 = precomp_fname(fname, 0);
  char *fname1 = precomp_fname(fname, 1);
  char *fname2 = precomp_fname(fname, 2);

  _ F = _const(n*w, 0, q);
  // Random values for masking x1 and x2
  _ R = _const(n, 0, k);
  _ S = _const(n, 0, k);
  unsigned long *init1 = MT_init[1];
  MT m1 = MT_init_by_array(init1, 5);
  unsigned long *init2 = MT_init[2];
  MT m2 = MT_init_by_array(init2, 5);
  for (int p=0; p<n; p++) {
    share_t r = RANDOM(m1, k);
    share_t s = RANDOM(m2, k);
    share_t t;
    if (xor) {
      t = r ^ s;
    } else {
      t = (r + s) % k;
    }
    pa_set(R->A, p, r);
    pa_set(S->A, p, s);
    for (int j=0; j<w; j++) {
      if (xor) {
        int z = (t == j);
        pa_set(F->A, p*w + j, z);
      } else {
        int z = (t == j);
        pa_set(F->A, p*w + j, z);
      }
    }
  }
  MT_free(m1);
  MT_free(m2);

  FILE *f0, *f1, *f2;
  f0 = fopen(fname0, "wb");
  f1 = fopen(fname1, "wb");
  f2 = fopen(fname2, "wb");
  precomp_write_seed(f1, n, q, init1);
  precomp_write_seed(f2, n, q, init2);
  precomp_write_seed(f0, n, q, init1);


  _ t1 = _const(n*w, 0, q);
  _ t2 = _const(n*w, 0, q);
  unsigned long *init0 = MT_init[0];
  MT m0 = MT_init_by_array(init0, 5);
  for (int p=0; p<n; p++) {
    for (int j=0; j<w; j++) {
      share_t rr = RANDOM(m0, q);
      pa_set(t1->A, p*w + j, rr);
      pa_set(t2->A, p*w + j, MOD(pa_get(F->A, p*w + j)-rr));
    }
  }
  MT_free(m0);
  precomp_write_seed(f1, n*w, q, init0);
  precomp_write_share(f2, t2);

// P0 stores the table as-is
  _ t0 = _const(w, 0, q);
  for (int j=0; j<w; j++) {
    pa_set(t0->A, j, (j == 0)); // needs confirmation
  }
  precomp_write_share(f0, t0);

  fclose(f0);
  fclose(f1);
  fclose(f2);

  _free(F);  _free(R);  _free(S);
  _free(t0);  _free(t1);  _free(t2);
  free(fname0);  free(fname1);  free(fname2);

sync:;

}

_ onehotvec_table_channel(int b, _ x, share_t q, precomp_tables T, int xor, int channel)
{
  int n = x->n;
  int k = 1 << b; // table size
  int w = 1 << b; // vector length

  _ ans = _const(n*w, 0, q);

  if (_party <= 0) {
    for (int p=0; p<n; p++) {
      share_t xx = pa_get(x->A, p);
      for (int j=0; j<w; j++) {
        pa_set(ans->A, p*w+j, pa_get(T->Tt->u.share.a, xx*w + j)%q);
      }
    }
    return ans;
  }

  _ t = _const(n*w, 0, q);
  for (int p=0; p<n*w; p++) {
    pa_set(t->A, p, precomp_get(T->Tt)%q);
  }

  _ y = _const(n, 0, k);
  for (int p=0; p<n; p++) {
    if (xor) {
      pa_set(y->A, p, (precomp_get(T->TR) ^ pa_get(x->A, p))%k);
    } else {
      pa_set(y->A, p, (k - precomp_get(T->TR) + pa_get(x->A, p))%k);
    }
  }
  _ z = _const(n, 0, k);
  mpc_exchange_share_channel(y, z, channel);
  if (_party == 1 || _party == 2) {
    for (int p=0; p<n; p++) {
      share_t xr = pa_get(y->A, p);
      share_t ys = pa_get(z->A, p);
      share_t tt;
      if (xor) {
        tt = xr ^ ys; // input is XOR shares
      } else {
        tt = (xr + ys) % k; // input is additive shares
      }
      for (int j=0; j<w; j++) {
        share_t z;
        if (xor) {
          z = tt ^ j;
        } else {
          z = (k + j - tt) % k;
        }
        pa_set(ans->A, p*w+j, pa_get(t->A, p*w + z)%q);
      }
    }
  }
  _free(y);  _free(z);  _free(t);
  return ans;
}
#define onehotvec_table(b, x, q, T, xor) onehotvec_table_channel(b, x, q, T, xor, 0)


_ onehotvec_online_channel(_ x, share_t q, int xor, int channel)
{
  if (_party > 2) return NULL;
  int n = x->n;
  int b = blog(order(x)-1)+1;
  int k = 1 << b; // table size
  int w = 1 << b; // vector length

  _ R = NULL, t = NULL;

// Precomputation
  if (_party <= 0) {
    // Compute tables for P1 and P2

    _ F = _const(n*w, 0, q);
    // Random values for masking x1 and x2
    _ R = _const(n, 0, k);
    _ S = _const(n, 0, k);
    MT m1 = MT_init_by_array(MT_init[1], 5);
    MT m2 = MT_init_by_array(MT_init[2], 5);
    for (int p=0; p<n; p++) {
      share_t r = RANDOM(m1, k);
      share_t s = RANDOM(m2, k);
      share_t t;
      if (xor) {
        t = r ^ s;
      } else {
        t = (r + s) % k;
      }
      pa_set(R->A, p, r);
      pa_set(S->A, p, s);
      for (int j=0; j<w; j++) {
        if (xor) {
          int z = (t == j);
          pa_set(F->A, p*w + j, z);
        } else {
          int z = (t == j);
          pa_set(F->A, p*w + j, z);
        }
      }
    }
    MT_free(m1);
    MT_free(m2);

    _ t0 = _const(n*w, 0, q);
    _ t1 = _const(n*w, 0, q);
    _ t2 = _const(n*w, 0, q);
    unsigned long *init0 = MT_init[0];
    MT m0 = MT_init_by_array(init0, 5); // needs revision
    for (int p=0; p<n; p++) {
      for (int j=0; j<w; j++) {
        share_t rr = RANDOM(m0, q);
        pa_set(t1->A, p*w + j, rr);
        pa_set(t2->A, p*w + j, MOD(pa_get(F->A, p*w + j)-rr)); // result is an additive share
      }
    }
    MT_free(m0);

    if (_party == 0) {
      mpc_send_share_channel(TO_PARTY1, R, channel);
      mpc_send_share_channel(TO_PARTY2, S, channel);
      mpc_send_share_channel(TO_PARTY1, t1, channel);
      mpc_send_share_channel(TO_PARTY2, t2, channel);
    }
    _free(t0);
    _free(t1);
    _free(t2);
    _free(F);
    _free(R);
    _free(S);
  } else {
    R = _const(n, 0, k);
    mpc_recv_share_channel(FROM_SERVER, R, channel);
    t = _const(n*w, 0, q);
    mpc_recv_share_channel(FROM_SERVER, t, channel);

  }

// Main computation
  _ ans = _const(n*w, 0, q);
  if (_party <= 0) {
    for (int p=0; p<n; p++) {
      share_t xx = pa_get(x->A, p);
      for (int j=0; j<w; j++) {
        int z = (xx) == (j);
        pa_set(ans->A, p*w+j, z % q);
      }
    }
  } else {
    _ y = _const(n, 0, k);
    for (int p=0; p<n; p++) {
      if (xor) {
        pa_set(y->A, p, (pa_get(R->A, p) ^ pa_get(x->A, p))%k);
      } else {
        pa_set(y->A, p, (k - pa_get(R->A, p) + pa_get(x->A, p))%k);
      }
    }
    _ z = _const(n, 0, k);
    mpc_exchange_share_channel(y, z, channel);
    if (_party == 1 || _party == 2) {
      for (int p=0; p<n; p++) {
        share_t xr = pa_get(y->A, p);
        share_t ys = pa_get(z->A, p);
        share_t tt;
        if (xor) {
          tt = xr ^ ys; // input is XOR shares
        } else {
          tt = (xr + ys) % k; // input is additive shares
        }
        for (int j=0; j<w; j++) {
          share_t z;
          if (xor) {
            z = tt ^ j;
          } else {
            z = (k + j - tt) % k;
          }
          pa_set(ans->A, p*w+j, pa_get(t->A, p*w + z)%q);
        }
      }
    }
    _free(y);
    _free(z);
    _free(R);
    _free(t);
  }
  return ans;
}
#define onehotvec_online(x, q, xor) onehotvec_online_channel(x, q, xor, 0)

///////////////////////////////////////////////////////////////////////////
// One-hot vector
// Input: 22ADD (arithmetic or XOR), specified by xor
// Output: 22ADD
///////////////////////////////////////////////////////////////////////////
_ onehotvec_channel(_ x, share_t q, int xor, int channel)
{
  int d = blog(order(x)-1)+1;
  if (d < 1 || d > ONEHOT_MAX) {
    printf("onehotvec: d=%d MAX=%d\n", d, ONEHOT_MAX);
    exit(1);
  }
  precomp_tables T = NULL;
  if (xor) {
    T = PRE_OHX_tbl[d-1][channel];
  } else {
    T = PRE_OHA_tbl[d-1][channel];
  }
  if (T != NULL) {
    if (xor) {
      PRE_OHX_count[d-1][channel] += len(x);
    } else {
      PRE_OHA_count[d-1][channel] += len(x);
    }
    return onehotvec_table_channel(d, x, q, T, xor, channel);
  } else {
    return onehotvec_online_channel(x, q, xor, channel);
  }
}
#define onehotvec(x, q, xor) onehotvec_channel(x, q, xor, 0)




void onehotvec_shamir_precomp(int b, int n, share_t q, char *fname)
{
  int k = 1 << b; // table size
  int w = 1 << b; // vector length

  if (_party > 0) goto sync;

  char *fname0 = precomp_fname(fname, 0);
  char *fname1 = precomp_fname(fname, 1);
  char *fname2 = precomp_fname(fname, 2);
  char *fname3 = precomp_fname(fname, 3);

  _ F = _const(n*w, 0, q);
  // Random values for masking x1 and x2
  _ R = _const(n, 0, k);
  _ S = _const(n, 0, k);
  unsigned long *init1 = MT_init[1];
  MT m1 = MT_init_by_array(init1, 5);
  unsigned long *init2 = MT_init[2];
  MT m2 = MT_init_by_array(init2, 5);
  for (int p=0; p<n; p++) {
    share_t r = RANDOM(m1, k);
    share_t s = RANDOM(m2, k);
    share_t t;
    t = (r + s) % k;
    pa_set(R->A, p, r);
    pa_set(S->A, p, s);
    for (int j=0; j<w; j++) {
      int z = (t == j);
      pa_set(F->A, p*w + j, z);
    }
  }
  MT_free(m1);
  MT_free(m2);

  FILE *f0, *f1, *f2, *f3;
  f0 = fopen(fname0, "wb");
  f1 = fopen(fname1, "wb");
  f2 = fopen(fname2, "wb");
  f3 = fopen(fname3, "wb");
  precomp_write_seed(f1, n, q, init1);
  precomp_write_seed(f2, n, q, init2);
  precomp_write_seed(f3, n, q, init1); // unused?
  precomp_write_seed(f0, n, q, init1); // unused?

  _ t1 = _const(n*w, 0, q);
  _ t2 = _const(n*w, 0, q);
  _ t3 = _const(n*w, 0, q);
  unsigned long *init0 = MT_init[0];
  MT m0 = MT_init_by_array(init0, 5);
  for (int p=0; p<n; p++) {
    for (int j=0; j<w; j++) {
      share_t rr = RANDOM(m0, q);
      share_t x = pa_get(F->A, p*w + j);
      pa_set(t1->A, p*w + j, MOD(x + 1 * rr));
      pa_set(t2->A, p*w + j, MOD(x + 2 * rr));
      pa_set(t3->A, p*w + j, MOD(x + 3 * rr));
    }
  }
  MT_free(m0);
  precomp_write_share(f1, t1);
  precomp_write_share(f2, t2);
  precomp_write_share(f3, t3);

// P0 stores the table as-is
  _ t0 = _const(w, 0, q);
  for (int j=0; j<w; j++) {
    pa_set(t0->A, j, (j == 0)); // needs confirmation
  }
  precomp_write_share(f0, t0);

  fclose(f0);
  fclose(f1);
  fclose(f2);
  fclose(f3);

  _free(F);  _free(R);  _free(S);
  _free(t0);  _free(t1);  _free(t2);  _free(t3);
  free(fname0);  free(fname1);  free(fname2);  free(fname3);

sync:;

}


_ overflow1_online_channel(_b a, share_t q, int channel); // in share_core.h

_ overflow_channel(int d, _ b, share_t q, int channel)
{
  if (1 <= d && d <= OF_MAX && PRE_OF_tbl[d-1][channel] != NULL) {
    PRE_OF_count[d-1][channel] += len(b);
    return funckbit_channel(d, b, q, PRE_OF_tbl[d-1][channel], channel);
  }
  if (d == 1) {
    return overflow1_online_channel(b, q, channel);
  }
  printf("overflow_channel: d = %d\n", d);
  exit(1);
}
#define overflow(d, bb, q) overflow_channel(d, bb, q, 0)


precomp_tables shamir3_revert_read(char *fname)
{
  int party = _party;
  if (party < 0) party = 0;
  char *fname2 = precomp_fname(fname, party);

  NEWT(precomp_tables, T);

  MMAP *map = NULL;
  map = mymmap(fname2);
  uchar *p = (uchar *)map->addr;
  T->TR = precomp_read(&p);
  T->Tt = precomp_read(&p);
  T->map = map;

  free(fname2);

  return T;
}

void shamir3_revert_tbl_read(int d, share_t irr_poly, int channel, char *fname)
{
  precomp_tables tbl = shamir3_revert_read(fname);
  PRE_RE_tbl[channel] = precomp_tbl_list_insert(tbl, d, irr_poly, PRE_RE_tbl[channel]);
}

////////////////////////////////////////////////////////////////////////////////
// The table is public
////////////////////////////////////////////////////////////////////////////////
_bits tablelookup(_ x, share_t *tbl, share_t q, int xor)
{
  int n = len(x);
  int k = blog(q-1)+1; // number of output bits
  share_t w = order(x);

  _bits ans = share_const_bits(n, 0, 2, k);

  _ ohv = onehotvec(x, 2, xor); // n one-hot vectors of length w

  for (int i=0; i<n; i++) {
    for (int j=0; j<k; j++) { // compute per output bit
      _ V = ans->a[j];
      for (share_t x=0; x<w; x++) {
        share_t y = tbl[x]; // compute assuming output value y
        if (y & (1<<j)) { // if the j-th output bit is 1
          share_t tmp = pa_get(V->A, i);
          tmp ^= pa_get(ohv->A, i*w+x);
          pa_set(V->A, i, tmp);
        }
      }
    }    
  }
  _free(ohv);
  return ans;
}

_ B2A_channel(_b a, share_t q, int channel)
{
  if (_party > 2) return NULL; // needs review
  int n = len(a);

  if (_party == -1) {
    _ ans = _const(n, 0, q);
    NEWITER(itr_ans, ans);
    NEWITER(itr_a, a);
    for (int i=0; i<n; i++) {
      pa_iter_set(itr_ans, pa_iter_get(itr_a));
    }
    pa_iter_flush(itr_ans);
    pa_iter_free(itr_a);
    return ans;
  }

  _ tmp = NULL;
  if (PRE_B2A_tbl[channel] != NULL) {
    PRE_B2A_count[channel] += len(a);
    return func1bit_channel(a, q, PRE_B2A_tbl[channel], channel);
  }

  if (PRE_OF_tbl[1-1][channel] != NULL) {
    _ ans = _const(n, 0, q);
    NEWITER(itr_ans, ans);
    NEWITER(itr_a, a);
    for (int i=0; i<n; i++) {
      pa_iter_set(itr_ans, pa_iter_get(itr_a));
    }
    pa_iter_flush(itr_ans);
    pa_iter_free(itr_a);
    _ bo = overflow_channel(1, a, q, channel);
    smul_(2, bo);
    vsub_(ans, bo);
    _free(bo);
    return ans;
  }
  return B2A_online_channel(a, q, channel);
}
#ifndef B2A
 #define B2A(a, q) B2A_channel(a, q, 0)
#endif

static void B2A_channel_(_b a, share_t q, int channel) {
  if (_party > 2) return;
  _ ans = B2A_channel(a, q, channel);
  _move_(a, ans);
} 

static void B2A_(_b a, share_t q)
{
  if (_party >  2) return;
  _ ans = B2A(a, q);
  _move_(a, ans);
}

#ifndef max
 #define max(x, y) ((x > y)?x:y)
#endif


_pair share_A2QB3_channel(precomp_tables T, _ a, share_t q, share_t qb, int channel)
{
  if (_party >  2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int k = blog(q-1)+1;
  if ((1 << k) != q) {
    printf("share_A2QB3: %d is not a power of two\n", (int)q);
  }

  share_t q3 = max(q/2, qb);


  int n = len(a);
  _ b = _const(n, 0, qb);
  _ bb = _const(n, 0, 2);
  for (int i=0; i<n; i++) {
    share_t z = (q+pa_get(a->A,i)) % 2; // least significant bit of additive share
    pa_set(b->A, i, z);
    pa_set(bb->A, i, z);
  }

  _ bo = func1bit3_channel(bb, q3, T, channel);

  for (int i=0; i<n; i++) {
    pa_set(b->A, i, (2*qb + pa_get(b->A, i) - 2*(pa_get(bo->A, i)%qb)) % qb); // if both shares of b are 1, this becomes 2 instead of expected 0
  }


  q = q/2;
  _ x;
  if (q > 1) {
    x = _const(n, 0, q);
    for (int i=0; i<n; i++) {
      share_t z;
      z = (q*4+pa_get(a->A, i)) / 2;
      z += pa_get(bo->A, i);
      pa_set(x->A, i, z % q);
    }
  } else {
    x = _const(n, 0, 2);
  }

  _pair ans = {x, b};

  _free(bb);  _free(bo);

  return ans;
}
#define share_A2QB3(T, a, q, qb) share_A2QB3_channel(T, a, q, qb, 0)

////////////////////////////////////////////////////////
// Convert additive shares into LSB shares and non-LSB shares
// Modulus q is assumed to be a power of two
// The non-LSB share modulus becomes q/2
// The LSB share modulus becomes qb
////////////////////////////////////////////////////////
_pair share_A2QB_channel2(_ a, share_t q, share_t qb, int channel)
{
  if (_party > 2) {
    _pair ans = {NULL, NULL};
    return ans;
  }

  _pair ans;

  if (PRE_OF_tbl[0][channel] != NULL) {
    PRE_OF_count[0][channel] += len(a);
    ans = share_A2QB3_channel(PRE_OF_tbl[0][channel], a, q, qb, channel);
  } else {
    ans = share_A2QB_channel(a, q, qb, channel);    
  }

  return ans;
}

_bits share_A2B_channel(_ a, share_t qb, int channel)
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
    _pair tmp = share_A2QB_channel2(x, order(x), qb, channel);
    ans->a[i] = tmp.y;
    _move_(x, tmp.x);
  }
  _free(x);

  return ans;
}
#define _A2B(a, qb) share_A2B_channel(a, qb, 0)
#define _A2B_channel(a, qb, channel) share_A2B_channel(a, qb, channel)


////////////////////////////////////////////////////////
// Convert additive shares (modulus 1<<k1) to modulus qb = 1<<k2 (k1 < k2)
////////////////////////////////////////////////////////
#if 1
_ share_extend_channel(_ a, share_t qb, int channel)
{
  if (_party >  2) return NULL;

  share_t q = order(a);
  if (qb < q) {
    printf("share_extend: q = %d qb = %d\n", (int)q, (int)qb);
  }
  int k1 = blog(q-1)+1;
  int k2 = blog(qb-1)+1;
  if ((1 << k1) != q) {
    printf("share_extend: %d is not a power of two\n", (int)q);
  }
  if ((1 << k2) != qb) {
    printf("share_extend: %d is not a power of two\n", (int)qb);
  }
  int n = len(a);
  _bits b = _A2B_channel(a, qb, channel);

  _ ans = _const(n, 0, qb);
  ans->type = SHARE_T_22ADD;

  for (int k=b->d-1; k>=0; k--) {
    smul_(2, ans);
    vadd_(ans, b->a[k]);
  }

  _free_bits(b);

  return ans;
}
#else
_ share_extend_channel(_ a, share_t qb, int channel)
{
  if (_party >  2) return NULL;

  share_t q = order(a);
  if (qb < q) {
    printf("share_extend: q = %d qb = %d\n", (int)q, (int)qb);
  }
  if (q == 2) {
    return B2A_channel(a, qb, channel);
  }
  return ChangeModulo_channel(a, qb, channel);
}
#endif
#define _extend_channel share_extend_channel
#define _extend(a, qb) share_extend_channel(a, qb, 0)
#define share_extend(a, qb) share_extend_channel(a, qb, 0)

///////////////////////////////////////////////////////////////////////////////
// Split into lower d bits and the rest
///////////////////////////////////////////////////////////////////////////////
_pair share_A2QD_channel(int d, _ a, share_t q, share_t qb, int channel)
{
  if (_party > 2) {
    NEWT(_, b);
    *b = *a;
    b->A = NULL;
    b->q = qb;
    NEWT(_, x);
    *x = *a;
    x->A = NULL;
    int w = 1 << d;
    q = q/w;
    if (q < 2) q = 2;
    x->q = q;
    _pair ans = {x, b};
    return ans;
  }
  int k = blog(q-1)+1;
  if ((1 << k) != q) {
    printf("share_A2QD: %d is not a power of two\n", (int)q);
  }
  int w = 1 << d;

  share_t q3 = max(q/w, qb);


  int n = len(a);
  _ b = _const(n, 0, qb); // lower digits
  _ bb = _const(n, 0, w); // for carry computation
  for (int i=0; i<n; i++) {
    pa_set(b->A, i, (q+pa_get(a->A,i)) % w); // lower d bits of additive share
    pa_set(bb->A, i, (q+pa_get(a->A,i)) % w); // used for carry computation from lower d bits
  }

  _ bo = overflow_channel(d, bb, q3, channel);

  for (int i=0; i<n; i++) {
    pa_set(b->A, i, (w*qb + pa_get(b->A, i) - w*pa_get(bo->A, i)) & (qb-1)); // correction when extending b modulus from w to qb
  }

  q = q/w; // modulus for upper digits
  _ x;
  if (q > 1) {
    x = _const(n, 0, q);

    for (int i=0; i<n; i++) {
      pa_set(x->A, i, ((q*0+pa_get(a->A, i)) / w) % q); // !!!
    }
    for (int i=0; i<n; i++) {
      pa_set(x->A, i, (4*q + pa_get(x->A, i) + pa_get(bo->A, i)) % q); // add carry from lower digits
    }
  } else {
    x = _const(n, 0, 2);
  }

  _pair ans = {x, b};

  _free(bb);  _free(bo);

  return ans;
}


#endif
