#ifndef _PRECOMPUTE_H
 #define _PRECOMPUTE_H


/**********************************************************************
 * Precompute and store on disk.
 * Store either shares or random seeds.
 * 
**********************************************************************/



// in beaver.h
void bt_tbl_init(void);
void precomp_free_bt(void); // in beaver.h
extern long BT_count[MAX_CHANNELS];

// in func.h
void of_tbl_init(void);
void b2a_tbl_init(void);
void precomp_free_func(void); // in func.h
extern struct ds_tbl_list *PRE_DS_tbl[MAX_CHANNELS];
extern long PRE_B2A_count[MAX_CHANNELS];
extern long PRE_OF_count[][MAX_CHANNELS];
extern long PRE_OHA_count[][MAX_CHANNELS];

// in dshare.h
void precomp_free_ds(void); // in dshare.h
extern long PRE_DS_count[MAX_CHANNELS];




#define ID_PRECOMP 0x1B


#define PRECOMP_SEED 1
#define PRECOMP_SHARE 2

typedef struct {
  int type; // seed or share
  union {
    struct {
      MT r;
      int n;
      //share_t q;
      unsigned long seed[5];
    } seed;
    struct {
      packed_array a;
    } share;
  } u;
  share_t q; // unused
  int current;    // position of the next element to use
}* precomp_table;

typedef struct precomp_tables {
  precomp_table TR, Tt;
  MMAP *map;
}* precomp_tables;
void precomp_free_tables(struct precomp_tables *T);



char *precomp_fname(char *fname, int party)
{
  if (party < 0) party = 0;
  char *fname2;
  NEWA(fname2, char, strlen(fname)+3);
  sprintf(fname2, "%s.%1d", fname, party);

  return fname2;
}


void precomp_write_seed(FILE *f, int n, share_t q, unsigned long seed[5])
{
  writeuint(1,ID_PRECOMP,f);
  writeuint(1,PRECOMP_SEED,f);
  writeuint(sizeof(n),n,f);
  writeuint(sizeof(q),q,f);
  for (int i=0; i<5; i++) {
    writeuint(sizeof(seed[0]), seed[i], f);
  }
}

precomp_table precomp_table_new_seed(int n, share_t q, unsigned long seed[5])
{
  NEWT(precomp_table, T); 
  T->type = PRECOMP_SEED;
  T->current = 0;

  T->u.seed.n = n;
  T->q = q;
  for (int i=0; i<5; i++) {
    T->u.seed.seed[i] = seed[i];
  }
  T->u.seed.r = MT_init_by_array(T->u.seed.seed, 5);

  return T;
}

void precomp_write_pa(FILE *f, packed_array a, share_t q)
{
  writeuint(1,ID_PRECOMP,f);
  writeuint(1,PRECOMP_SHARE,f);
  writeuint(sizeof(q),q,f);
  pa_write(a, f);
}

void precomp_write_share(FILE *f, share_array a)
{
  precomp_write_pa(f, a->A, a->q);
}



precomp_table precomp_table_new_share(packed_array a)
{
  NEWT(precomp_table, T); 
  T->type = PRECOMP_SHARE;
  T->current = 0;

  T->u.share.a = a;

  return T;
}

void precomp_write(FILE *f, precomp_table T)
{
  if (T->type == PRECOMP_SEED) {
    precomp_write_seed(f, T->u.seed.n, T->q, T->u.seed.seed);
  } else if (T->type == PRECOMP_SHARE) {
    precomp_write_pa(f, T->u.share.a, T->q);
  }
}

precomp_table precomp_read(uchar **addr)
{
  uchar *p = *addr;
  NEWT(precomp_table, T);

  int id = getuint(p,0,1);  p += 1;
  if (id != ID_PRECOMP) {
    printf("precomp_read: id = %d is not supported.\n", id);
    exit(1);
  }
  int type = getuint(p,0,1);  p += 1;
  T->type = type;
  if (type == PRECOMP_SEED) {
    T->u.seed.n = getuint(p,0,sizeof(T->u.seed.n));  p += sizeof(T->u.seed.n);
    T->q = getuint(p,0,sizeof(T->q));  p += sizeof(T->q);
    for (int i=0; i<5; i++) {
      T->u.seed.seed[i] = getuint(p,0,sizeof(T->u.seed.seed[0]));  p += sizeof(T->u.seed.seed[0]);
    }
    T->u.seed.r = MT_init_by_array(T->u.seed.seed, 5);
  } else if (type == PRECOMP_SHARE) {
    T->q = getuint(p,0,sizeof(T->q));  p += sizeof(T->q);
    T->u.share.a = pa_read(&p);
  } else {
    printf("precomp_read: type = %d is not supported.\n", type);
    exit(1);
  }

  T->current = 0;
  *addr = p;
  return T;
}

share_t precomp_order(precomp_table T)
{
  share_t q;
  q = T->q;
  return q;

}

share_t precomp_get(precomp_table T)
{
  share_t x;
  if (T->type == PRECOMP_SEED) {
    x = RANDOM(T->u.seed.r, T->q);
    T->current += 1;
    if (T->current == T->u.seed.n) {
      MT_free(T->u.seed.r);
      T->u.seed.r = MT_init_by_array(T->u.seed.seed, 5);
      T->current = 0;
    }
  } else if (T->type == PRECOMP_SHARE) {
    x = pa_get(T->u.share.a, T->current);
    T->current += 1;
    if (T->current == T->u.share.a->n) {
      T->current = 0;
    }
  } else {
    printf("precomp_get: type = %d is not supported.\n", T->type);
    exit(1);
  }
  return x;
}

void precomp_free(precomp_table T)
{
  if (T == NULL) return;
  if (T->type == PRECOMP_SEED) {
    MT_free(T->u.seed.r);
  } else if (T->type == PRECOMP_SHARE) {
    pa_free_map(T->u.share.a);
  }
  free(T);
}

void precomp_free_tables(precomp_tables T)
{
  if (T == NULL) return;
  precomp_free(T->TR);
  precomp_free(T->Tt);
  if (T->map != NULL) mymunmap(T->map);
  free(T);
}



void precomp_tables_new(void)
{
  bt_tbl_init();
  of_tbl_init();
  b2a_tbl_init();
}

void precomp_tables_free(void) 
{
  precomp_free_func();
  precomp_free_bt();
  precomp_free_ds();
  for (int i=0; i<_opt.channels; i++) {
    // if (PRE_UV_tbl[i] != NULL)  uv_tbl_list_free(PRE_UV_tbl[i]);
  }
  long BT_total=0, B2A_total=0;
  long OHA_total[ONEHOT_MAX], OHX_total[ONEHOT_MAX];
  long OF_total[OF_MAX];
  long DS_total=0, UV_total = 0;
  for (int j=1; j<=ONEHOT_MAX; j++) {
    OHA_total[j-1] = 0;
    OHX_total[j-1] = 0;
  }
  for (int j=1; j<=OF_MAX; j++) {
    OF_total[j-1] = 0;
  }
  for (int i=0; i<_opt.channels; i++) {
    BT_total += BT_count[i];
    B2A_total += PRE_B2A_count[i];
    for (int j=1; j<=ONEHOT_MAX; j++) {
      OHA_total[j-1] += PRE_OHA_count[j-1][i];
      OHX_total[j-1] += PRE_OHA_count[j-1][i];
    }
    for (int j=1; j<=OF_MAX; j++) {
      OF_total[j-1] += PRE_OF_count[j-1][i];
    }
    DS_total += PRE_DS_count[i];
  }
#if 0
  printf("BT   %ld\n", BT_total);
  printf("B2A  %ld\n", B2A_total);
  printf("DS   %ld\n", DS_total);
  printf("UV   %ld\n", UV_total);
  printf("OHA%d %ld\n", 1, OHA_total[1-1]);
  printf("OHA%d %ld\n", 2, OHA_total[2-1]);
  printf("OHA%d %ld\n", 3, OHA_total[3-1]);
  printf("OHX%d %ld\n", 1, OHX_total[1-1]);
  printf("OHX%d %ld\n", 2, OHX_total[2-1]);
  printf("OHX%d %ld\n", 3, OHX_total[3-1]);
  printf("OF%d  %ld\n", 1, OF_total[1-1]);
  printf("OF%d  %ld\n", 2, OF_total[2-1]);
  printf("OF%d  %ld\n", 3, OF_total[3-1]);
#endif
}

#endif
