////////////////////////////////////////////
// Functions that manipulate struct contents or perform communication go here
////////////////////////////////////////////

/*
  Basic operations for share_array

  - creation
  - deallocation
  - concatenation and slicing
  - send/receive
  - addition/subtraction and scalar multiplication
  - multiplication
  - dshare is handled separately
*/


#ifndef _SHARE_CORE_H
 #define _SHARE_CORE_H

typedef int share_t;


///////////////////////////////////////////////////
// Parameters that control behavior
///////////////////////////////////////////////////
struct {
  int parties; // number of parties (including client)
  int channels; // number of channels
  int warn_precomp;
  int comm_no_delay;
  int send_queue;
  int oram_check_overflow;
} _opt = {0, 0, 0, 0, 0, 0};

#include "mman.h"
#include "mpc.h"    // communication
#include "bits.h"   // memory management
#include "random.h" // random number generation



typedef struct share_array {
  int type; // share type
  int n; // number of elements
  share_t q; // mod
  share_t irr_poly; // irreducible polynomial
  packed_array A; // raw values (party==0) or shares (party==1,2)
  int own; // do not free when set to 1
}* share_array;
#define _ share_array

#define SHARE_T_RAW 0
#define SHARE_T_22ADD 1
#define SHARE_T_BINARY 2
#define SHARE_T_33ADD 3
#define SHARE_T_SHAMIR 4
#define SHARE_T_RSS 5
#define SHARE_T_ADDITIVE SHARE_T_22ADD

#ifndef MOD
 #define MOD(x) (((x)%(q)+(q)*1) % (q))
#endif

#define LMUL(x, y, q) (((x)*(y))%(q)) // valid only when q is a power of two

#include "precompute.h"
#include "beaver.h"

int max_partyid(_ a)
{
  if (a == NULL) {
    printf("max_partyid: a = NULL\n");
    return -1;
  }
  int n = -1;
  switch(a->type) {
  case SHARE_T_22ADD:  n = 2; break;
  case SHARE_T_33ADD:  n = 3; break;
  case SHARE_T_BINARY: n = 2; break;
  case SHARE_T_RSS:    n = 3; break;
  case SHARE_T_SHAMIR: n = 3; break;
  case SHARE_T_RAW:    n = -1; break; // needs review
  }
  return n;
}


static int len(share_array a)
{
  if (a->type == SHARE_T_RSS) return a->n/2;
  return a->n;
}

static share_t order(share_array a)
{
  return a->q;
}

static packed_array share_raw(share_array a)
{
  return a->A;
}



#ifndef RANDOM0
 #define RANDOM0(n) (MT_genrand_int32(mt0) % (n))
#endif


typedef share_array _x;  // xor share (GF(2^d))
typedef share_array _b;  // binary additive (xor) share
typedef share_array _s;  // Shamir's share
typedef share_array _sx;  // Shamir's share (GF(2^d))
typedef share_array _s3; // 3-party additive share
typedef share_array _s3x;  // 3-party additive share (GF(2^d))
typedef share_array _r; // replicated secret share

typedef struct {
  share_array x, y;
} share_pair;
#define _pair share_pair

static _pair _P(_ x, _ y)
{
  _pair ans;
  ans.x = x;
  ans.y = y;
  return ans;
}

static void mpc_send_pa_channel(int party_to, packed_array A, int channel)
{
  if (_party < 0) return;
  void *buf;
  int size;
  packed_array tmp = NULL;

  if (A->type != PA_PACK) {
    tmp = pa_convert(A, PA_PACK);
    buf = tmp->B;
    size = pa_size(tmp);
  } else {
    buf = A->B;
    size = pa_size(A);
  }

  mpc_send_channel(party_to, buf, size, channel);  //send_6 += size;
  if (tmp) pa_free(tmp);
}
#define mpc_send_pa(party_to, A) mpc_send_pa_channel(party_to, A, 0)


static void mpc_send_share_channel(int party_to, _ A, int channel)
{
  if (_party < 0) return;
  mpc_send_pa_channel(party_to, A->A, channel);  //send_6 += size;
}
#define mpc_send_share(party_to, A) mpc_send_share_channel(party_to, A, 0)

static void mpc_recv_pa_channel(int party_from, packed_array A, int channel)
{
  if (_party < 0) return;
  void *buf;
  int size;
  packed_array tmp = NULL;

  if (A->type != PA_PACK) {
    tmp = pa_new_type(A->n, A->w, PA_PACK);
    buf = tmp->B;
    size = pa_size(tmp);
  } else {
    buf = A->B;
    size = pa_size(A);
  }

  mpc_recv_channel(party_from, buf, size, channel);
  if (tmp) {
    packed_array tmp2 = pa_convert(tmp, A->type);
    free(A->B);
    A->B = tmp2->B;
    free(tmp2);
    pa_free(tmp);
  }
}
#define mpc_recv_pa(party_to, A) mpc_recv_pa_channel(party_to, A, 0)

static void mpc_recv_share_channel(int party_from, _ A, int channel)
{
  if (_party < 0) return;
  mpc_recv_pa_channel(party_from, A->A, channel);
}
#define mpc_recv_share(party_to, A) mpc_recv_share_channel(party_to, A, 0)


static void mpc_exchange_pa_channel(packed_array p_send, packed_array p_recv, int channel)
{
  void *buf_send, *buf_recv;
  packed_array send_tmp = NULL, recv_tmp = NULL;
  int size;
  if (p_send->type != PA_PACK) {
    send_tmp = pa_convert(p_send, PA_PACK);
    buf_send = send_tmp->B;
    size = pa_size(send_tmp);
  } else {
    buf_send = p_send->B;
    size = pa_size(p_send);
  }
  if (p_recv->type != PA_PACK) {
    recv_tmp = pa_new_type(p_recv->n, p_recv->w, PA_PACK);
    buf_recv = recv_tmp->B;
  } else {
    buf_recv = p_recv->B;
  }

  mpc_exchange_channel(buf_send, buf_recv, size, channel);

  if (p_recv->type != PA_PACK) {
    packed_array tmp2 = pa_convert(recv_tmp, p_recv->type);
    free(p_recv->B);
    p_recv->B = tmp2->B;
    free(tmp2);
    pa_free(recv_tmp);
  }

}


static void mpc_exchange_share_channel(_ share_send, _ share_recv, int channel)
{
  if (_party >  2) return;
  if (_party <= 0) return;

  mpc_exchange_pa_channel(share_send->A, share_recv->A, channel);
}
#define mpc_exchange_share(send, recv) mpc_exchange_share_channel(send, recv, 0)

static void share_print(share_array a)
{
//  if (_party >  2) return;
  if (a == NULL) return;
  printf("n = %d q = %d w = %d irr = %d party %d: ", a->n, (int)a->q, a->A->w, a->irr_poly, _party);
  if (a->A != NULL) {
    for (int i=0; i<a->n; i++) printf("%d ", (int)pa_get(a->A, i));
  }
  printf("\n");
}
#define _print share_print

static void share_fprint(FILE *f, share_array a)
{
  if (a->A == NULL) return;
  fprintf(f, "n = %d q = %d w = %d party %d: ", a->n, (int)a->q, a->A->w, _party);
  for (int i=0; i<a->n; i++) fprintf(f, "%d ", (int)pa_get(a->A, i));
  fprintf(f, "\n");
}

static char *share_print_str(share_array a)
{
  if (a == NULL) {
    char *p = malloc(1);
    *p = 0;
    return p;
  }
  char *buf;
  NEWA(buf, char, 100 + a->n * 30);
  int s, total;
  total = 0;

  s = sprintf(buf+total, "n = %d q = %d w = %d irr = %d party %d: ", a->n, (int)a->q, a->A->w, a->irr_poly, _party);
  total += s;

  if (a->A != NULL) {
    for (int i=0; i<a->n; i++) {
      s = sprintf(buf+total, "%d ", (int)pa_get(a->A, i));
      total += s;
    }
  }
  buf[total] = 0;
  buf = realloc(buf, total+1);
  return buf;
}
#define _print share_print


static share_array share_new_channel_type(int n, share_t q, share_t *A, int channel, int pa_type)
{
  int i;
  NEWT(share_array, ans);
  int k;

  ans->type = SHARE_T_ADDITIVE;
  ans->n = n;
  ans->q = q;
  ans->own = 0;
  ans->irr_poly = 0;
  k = blog(q-1)+1;
  ans->A = NULL;
  if (_party > 2) return ans;

  ans->A = pa_new_type(n, k, pa_type);
  if (_party <= 0) {
    packed_array A1, A2;
    A1 = pa_new_type(n, k, pa_type);
    A2 = pa_new_type(n, k, pa_type);
    pa_iter itr_ans = pa_iter_new(ans->A);
    pa_iter itr_A1 = pa_iter_new(A1);
    pa_iter itr_A2 = pa_iter_new(A2);
    for (i=0; i<n; i++) {
      share_t r;
      //pa_set(ans->A, i, A[i]);
      pa_iter_set(itr_ans, A[i]);
      //r = RANDOM0(q);
      r = RANDOM(mt_[0][channel], q);
      //pa_set(A1, i, r);
      //pa_set(A2, i, MOD(A[i] - r));
      pa_iter_set(itr_A1, r);
      pa_iter_set(itr_A2, MOD(A[i] - r));
    }
    pa_iter_flush(itr_ans);
    pa_iter_flush(itr_A1);
    pa_iter_flush(itr_A2);
    mpc_send_pa_channel(TO_PARTY1, A1, channel);
    mpc_send_pa_channel(TO_PARTY2, A2, channel);

    pa_free(A1);
    pa_free(A2);
  } else {
    mpc_recv_share_channel(FROM_SERVER, ans, channel);
  }

  return ans;
}
#define share_new_channel(n, q, A, channel) share_new_channel_type(n, q, A, channel, PA_PACK)
#define share_new(n, q, A) share_new_channel(n, q, A, 0)
#define share_new_type(n, q, A, pa_type) share_new_channel_type(n, q, A, 0, pa_type)


static share_array share_xor_new_channel(int n, share_t q, share_t *A, int channel)
{
  int i;
  NEWT(share_array, ans);
  int k;

  ans->type = SHARE_T_ADDITIVE;
  ans->irr_poly = q;
  ans->n = n;
  ans->q = q;
  ans->own = 0;
  k = blog(q-1)+1;
  ans->A = NULL;
  if (_party > 2) return ans;

  ans->A = pa_new(n, k);
  if (_party <= 0) {
    packed_array A1, A2;
    A1 = pa_new(n, k);
    A2 = pa_new(n, k);
    for (i=0; i<n; i++) {
      share_t r;
      pa_set(ans->A, i, A[i]);
      r = RANDOM0(q);
      pa_set(A1, i, r);
      pa_set(A2, i, A[i] ^ r);
    }
    mpc_send_pa_channel(TO_PARTY1, A1, channel);  //send_7 += pa_size(A1);
    mpc_send_pa_channel(TO_PARTY2, A2, channel);

    pa_free(A1);
    pa_free(A2);
  } else {
    mpc_recv_share_channel(FROM_SERVER, ans, channel);
  }

  return ans;
}
#define share_xor_new(n, q, A) share_xor_new_channel(n, q, A, 0)


#if 1
static share_array share_new_queue_channel(int n, share_t q, share_t *A, int channel)
{
  if (_party >  2) return NULL;
  int i;
  NEWT(share_array, ans);
  int k;

  ans->type = SHARE_T_ADDITIVE;
  ans->n = n;
  ans->q = q;
  ans->irr_poly = 0; // !!
  ans->own = 0;
  k = blog(q-1)+1;

  ans->A = NULL;
  if (_party > 2) return ans;


  ans->A = pa_new(n, k);
  if (_party <= 0) {
    packed_array A1, A2;
    A1 = pa_new(n, k);
    A2 = pa_new(n, k);
    for (i=0; i<n; i++) {
      share_t r;
      pa_set(ans->A, i, A[i]);
      r = RANDOM0(q);
      pa_set(A1, i, r);
      pa_set(A2, i, MOD(A[i] - r));
    }
    mpc_send_pa_channel(TO_PARTY1, A1, channel);
    mpc_send_pa_channel(TO_PARTY2, A2, channel);

    pa_free(A1);
    pa_free(A2);
  } else {
    mpc_recv_pa_channel(FROM_SERVER, ans->A, channel);
  }

  return ans;
}
#endif

// 定数の配列からシェアを作る．通信無し．
static share_array share_new_const(int n, share_t q, share_t *A)
{
  if (_party >  2) return NULL;
  int i;
  NEWT(share_array, ans);
  int k;

  ans->type = SHARE_T_ADDITIVE;
  ans->n = n;
  ans->q = q;
  ans->irr_poly = 0; // !!
  ans->own = 0;
  k = blog(q-1)+1;

  ans->A = pa_new(n, k);
  if (_party <= 1) {
    for (i=0; i<n; i++) {
      pa_set(ans->A, i, A[i]);
    }
  }
  return ans;
}

static void share_free(share_array a)
{
  if (a == NULL) return;
  if (a->A != NULL) pa_free(a->A);
  free(a);
}
#define _free share_free

static void share_save(share_array a, char *filename)
{
  if (_party >  2) return; // needs review
  char buf[100];
  int p = _party;
  if (p < 0) p = 0;
  sprintf(buf, "%s%d.txt", filename, p);
  FILE *f = fopen(buf, "w");
  if (f == NULL) {
    perror("share_save: ");
  }
  int n = len(a);
  fprintf(f, "%d %d\n", n, (int)order(a));
  for (int i=0; i<n; i++) {
    fprintf(f, "%d\n", (int)pa_get(a->A, i));
  }
  fclose(f);
}
#define _save share_save

static share_array share_load(char *filename)
{
  if (_party >  2) return NULL; // needs review
  char buf[100];
  int p = _party;
  if (p < 0) p = 0;
  sprintf(buf, "%s%d.txt", filename, p);
  FILE *f = fopen(buf, "r");
  if (f == NULL) {
    perror("share_load: ");
  }
  int n;
  share_t q, v;
  int qtmp;
  fscanf(f, " %d %d", &n, &qtmp);
  q = qtmp;
  int k = blog(q-1)+1;
  NEWT(share_array, a);
  a->A = pa_new(n, k);
  a->n = n;
  a->q = q;
  for (int i=0; i<n; i++) {
    int vtmp;
    fscanf(f, " %d", &vtmp);
    v = vtmp;
    pa_set(a->A, i, v);
  }
  fclose(f);
  return a;
}
#define _load share_load


static void share_save_binary(share_array a, FILE *f)
{
  writeuint(1,ID_SHARE,f);
  writeuint(sizeof(a->type), a->type, f);
  writeuint(sizeof(a->n), a->n, f);
  writeuint(sizeof(a->q), a->q, f);
  writeuint(sizeof(a->irr_poly), a->irr_poly, f);

  pa_write(a->A, f);

}
#define _save_binary share_save_binary

static void share_save_binary_to_file(share_array a, char *filename)
{
  if (_party >  max_partyid(a)) return;

  int p = _party;
  if (p < 0) p = 0;
  char *fname = precomp_fname(filename, p);

  FILE *f = fopen(fname, "w");
  if (f == NULL) {
    perror("share_save_binary_to_file: ");
  }
  share_save_binary(a, f);

  fclose(f);
  free(fname);
}

static _ share_load_binary(uchar **p_)
{
  uchar *p = *p_;

  int type = getuint(p,0,1);  p += 1;
  if (type != ID_SHARE) {
    printf("share_load_binary: ID = %d\n", type);
    exit(1);
  }

  NEWT(_, ans);
  ans->type = getuint(p,0,sizeof(ans->type));  p += sizeof(ans->type);
  ans->n = getuint(p,0,sizeof(ans->n));  p += sizeof(ans->n);
  ans->q = getuint(p,0,sizeof(ans->q));  p += sizeof(ans->q);
  ans->irr_poly = getuint(p,0,sizeof(ans->irr_poly));  p += sizeof(ans->irr_poly);
  ans->own = 0;
  ans->A = pa_read(&p);

  *p_ = p;
  return ans;
}
#define _load_binary share_load_binary

static _ share_load_binary_from_file(char *filename)
{
  if (_party >  2) return NULL; // needs review

  int party = _party;
  if (party < 0) party = 0;
  char *fname = precomp_fname(filename, party);

  MMAP *map = NULL;
  map = mymmap(fname);
  uchar *p = (uchar *)map->addr;

  _ ans = share_load_binary(&p);

  free(fname);

  return ans;
}

static void share_check(share_array a)
{
  if (_party >  2) return; // needs review
  if (a->type != SHARE_T_22ADD) {
    printf("share_check: type = %d\n", a->type);
  }
  int i, n;
  share_t q;

  n = a->n;
  q = a->q;
  int k = blog(q-1)+1;
  if (_party <= 0) {
    packed_array A1, A2;
    A1 = pa_new(n, k);
    A2 = pa_new(n, k);
    printf("check party %d: ", _party);
    mpc_recv_pa(FROM_PARTY1, A1);
    mpc_recv_pa(FROM_PARTY2, A2);
    if (_party == 0) {
      for (i=0; i<n; i++) {
        share_t x;
        x = MOD(q + pa_get(A1, i) + pa_get(A2, i));
        if (x != pa_get(a->A, i)) {
          printf("i = %d A = %d %d A1 = %d A2 = %d\n", i, (int)pa_get(a->A, i), (int)x, (int)pa_get(A1,i), (int)pa_get(A2,i));
          exit(1);
        }
      }
      printf("check done\n");
    }
    pa_free(A1);
    pa_free(A2);
  } else {
    printf("check party %d: \n", _party);
    mpc_send_share(TO_SERVER, a);
  }
}
#define _check share_check

static share_array share_reconstruct_channel(share_array a, int channel)
{
  if (_party >  2) return NULL;

  if (a->type != SHARE_T_22ADD) {
    printf("share_reconstruct: type = %d\n", a->type);
  }

  int i, n;
  share_t q;

  int mode = 0;

  NEWT(share_array, ans);
  *ans = *a;
  n = a->n;
  q = a->q;
  int k = blog(q-1)+1;
  ans->type = a->type;
  ans->A = pa_new_type(n, k, a->A->type);

  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_a = pa_iter_new(a->A);
  //pa_iter_new(a->A);
  if (_party <= 0) {
    for (i=0; i<n; i++) {
      pa_iter_set(itr_ans, pa_iter_get(itr_a));
    }
  } else {
    packed_array x;
    x = pa_new_type(n, k, a->A->type);
    if (mode == 0) {
      mpc_exchange_channel(a->A->B, x->B, pa_size(a->A), channel);
      pa_iter itr_x = pa_iter_new(x);
      for (i=0; i<n; i++) {
        pa_iter_set(itr_ans, MOD(pa_iter_get(itr_a)+ pa_iter_get(itr_x)));
      }
      pa_iter_free(itr_x);
    }
    pa_free(x);
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_a);
  return ans;
}
#define _reconstruct_channel share_reconstruct_channel
#define share_reconstruct(a) share_reconstruct_channel(a, 0)
#define _reconstruct share_reconstruct

static void share_reconstruct_channel_phase0(share_array a, int channel)
{
  if (_party >  2) return;

  if (a->type != SHARE_T_22ADD) {
    printf("share_reconstruct: type = %d\n", a->type);
  }

  int i, n;
  share_t q;

  if (_party <= 0) {
  } else {
    mpc_send_channel(TO_PAIR, a->A->B, pa_size(a->A), channel);
  }
}

static share_array share_reconstruct_channel_phase1(share_array a, int channel)
{
  if (_party >  2) return NULL;

  NEWT(share_array, ans);
  *ans = *a;
  int n = a->n;
  share_t q = a->q;
  int k = blog(q-1)+1;
  ans->type = a->type;
  ans->A = pa_new_type(n, k, a->A->type);

  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_a = pa_iter_new(a->A);
  if (_party <= 0) {
    for (int i=0; i<n; i++) {
      pa_iter_set(itr_ans, pa_iter_get(itr_a));
    }
  } else {
    packed_array x;
    x = pa_new_type(n, k, a->A->type);
    mpc_recv_channel(FROM_PAIR, x->B, pa_size(x), channel);
    pa_iter itr_x = pa_iter_new(x);
    for (int i=0; i<n; i++) {
      pa_iter_set(itr_ans, MOD(pa_iter_get(itr_a)+ pa_iter_get(itr_x)));
    }
    pa_iter_free(itr_x);
    pa_free(x);
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_a);
  return ans;
}

static share_array share_reconstruct_xor_channel(share_array a, int channel)
{
  if (_party >  2) return NULL;

  int i, n;
  share_t q;

  NEWT(share_array, ans);
  *ans = *a;
  n = a->n;
  q = a->q;
  int k = blog(q-1)+1;
  ans->A = pa_new(n, k);

  if (_party <= 0) {
    for (i=0; i<n; i++) {
      pa_set(ans->A, i, pa_get(a->A, i));
    }
  } else {
    packed_array x;
    x = pa_new(n, k);
    mpc_exchange_channel(a->A->B, x->B, pa_size(a->A), channel);
    for (i=0; i<n; i++) {
      pa_set(ans->A, i, pa_get(a->A,i) ^ pa_get(x,i));
    }
    pa_free(x);
  }
  return ans;
}
#define share_reconstruct_xor(a) share_reconstruct_xor_channel(a, 0)
#define _reconstruct_xor share_reconstruct_xor

static void _print_debug(_ a)
{
  _ tmp = _reconstruct(a);
  printf("debug "); _print(tmp);
  _free(tmp);
}
static void _print_debug_xor(_ a)
{
  _ tmp = _reconstruct_xor(a);
  printf("debug "); _print(tmp);
  _free(tmp);
}
static void _print_debug_channel(_ a, int channel)
{
  _ tmp = _reconstruct_channel(a, channel);
  printf("debug "); _print(tmp);
  _free(tmp);
}


static share_array share_dup(share_array a)
{
  if (a == NULL) {
    printf("share_dup: a == NULL\n");
    return NULL;
  }
  NEWT(share_array, D);
  *D = *a;
  D->A = NULL;
  if (a->A != NULL) {
    D->A = pa_new_type(D->n, a->A->w, a->A->type); // valgrind errors occur if pa_new does not initialize memory
    memcpy(D->A->B, a->A->B, pa_size(a->A));
  }
  return D;
}
#define _dup share_dup



//////////////////////////////////
// Synchronize P0, P1, and P2
//////////////////////////////////
void _sync(void)
{
  if (_party >  2) return;
  share_t A[1] = {0};
  _ tmp = share_new(1, 2, A);
  _check(tmp);
  _free(tmp);
}

void _sync_channel(int channel)
{
  if (_party >  2) return;
  char tmp[1];
  tmp[0] = '$';
  if (_party == 0) {
    mpc_send_channel(TO_PARTY1, tmp, 1, channel);
    mpc_send_channel(TO_PARTY2, tmp, 1, channel);
    mpc_recv_channel(FROM_PARTY1, tmp, 1, channel);
    mpc_recv_channel(FROM_PARTY2, tmp, 1, channel);
  }
  tmp[0] = '?';
  if (_party == 1 || _party == 2) {
    mpc_recv_channel(FROM_SERVER, tmp, 1, channel);
    //printf("sync: recv %c %d\n", tmp[0], tmp[0]);
    if (tmp[0] != '$') {
      printf("!sync: recv %c %d\n", tmp[0], tmp[0]);
      exit(1);
    }
    mpc_send_channel(TO_SERVER, tmp, 1, channel);
  }
}

void _sync3_channel(int channel)
{
  if (_party >  3) return;
  char tmp[1];
  tmp[0] = '$';
  if (_party == 0) {
    mpc_send_channel(TO_PARTY1, tmp, 1, channel);
    mpc_send_channel(TO_PARTY2, tmp, 1, channel);
    mpc_send_channel(TO_PARTY3, tmp, 1, channel);
    mpc_recv_channel(FROM_PARTY1, tmp, 1, channel);
    mpc_recv_channel(FROM_PARTY2, tmp, 1, channel);
    mpc_recv_channel(FROM_PARTY3, tmp, 1, channel);
  }
  tmp[0] = '?';
  if (_party == 1 || _party == 2 || _party == 3) {
    mpc_recv_channel(FROM_SERVER, tmp, 1, channel);
    if (tmp[0] != '$') {
      printf("sync3: recv %c %d\n", tmp[0], tmp[0]);
      exit(1);
    }
    mpc_send_channel(TO_SERVER, tmp, 1, channel);
  }
}
#define _sync3() _sync3_channel(0)


//////////////////////////////////
// a := b (free old a and b memory)
//////////////////////////////////
static void share_move_(share_array a, share_array b)
{
  if (a == NULL || b == NULL) {
    printf("move_ a = %p b = %p\n", a, b);
    return;
  }
  if (a->A != NULL) pa_free(a->A);
  *a = *b;
  free(b);
}
#define _move_ share_move_

static share_array share_move(share_array b)
{
  if (b == NULL) {
    printf("move b = %p\n", b);
  }
  return b;
}
#define _move share_move

///////////////////////////////////////
// Get one component of a share
///////////////////////////////////////
static share_t share_getraw(share_array a, int i)
{
  if (_party > max_partyid(a)) return 0;
  if (a == NULL) {
    printf("share_getraw: a = NULL\n");
    return 0;
  }
  if (a->A == NULL) {
    printf("share_getraw: a->A = NULL\n");
    return 0;
  }
  if (i < 0 || i >= a->n) {
    printf("share_getraw: n %d i %d\n", a->n, i);
  }
  return pa_get(a->A,i);
}

static void share_setraw(share_array a, int i, share_t x)
{
  if (_party > max_partyid(a)) return;
  if (a == NULL) {
    printf("share_setraw: a = NULL\n");
    return;
  }
  if (a->A == NULL) {
    printf("share_setraw: a->A = NULL\n");
    return;
  }
  if (i < 0 || i >= a->n) {
    printf("share_setraw: n %d i %d\n", a->n, i);
  }
  if (x < 0) {
    printf("share_setraw: x %d q %d\n", x, a->q);
  }
  if (a->A == NULL) return;
  share_t q = a->q;
  pa_set(a->A, i, MOD(x));
}

static share_array share_const_type2(int n, share_t v, share_t q, int type, int pa_type)
{
  NEWT(share_array, ans);
  int n2 = n;
  if (type == SHARE_T_RSS) n2 = n*2;
  ans->n = n2;
  ans->q = q;
  ans->irr_poly = 0;
  ans->type = type;
  int k = blog(q-1)+1;
  ans->A = NULL;
  v = MOD(v); // test
  if (_party > max_partyid(ans)) return ans;
  ans->A = pa_new_type(n2, k, pa_type); // TODO: this allocates memory on party 3 even for 22ADD in 3-party mode
  if (_party >= 2) {
    memset(ans->A->B, 0, pa_size(ans->A));
  } else {
    if (v == 0) {
      memset(ans->A->B, 0, pa_size(ans->A));
    } else {
      pa_iter itr_ans = pa_iter_new(ans->A);
      for (int i=0; i<n; i++) {
        pa_iter_set(itr_ans, v);
      }
      pa_iter_flush(itr_ans);
    }
  }
  return ans;
}
#define share_const_type(n, v, q, type) share_const_type2(n, v, q, type, PA_PACK)
#define share_const(n, v, q) share_const_type(n, v, q, SHARE_T_22ADD)
#define _const share_const
#define _const_shamir(n, v, q) share_const_type(n, v, q, SHARE_T_SHAMIR)

static share_array Perm_ID2_type(int n, share_t q, int pa_type)
{
  NEWT(share_array, ans);
  ans->n = n;
  ans->q = q;
  ans->type = SHARE_T_22ADD;
  ans->irr_poly = 0;
  ans->A = NULL;
  if (_party > 2) return ans;
  int w = blog(q-1)+1;
  ans->A = pa_new_type(n, w, pa_type);
  pa_iter itr_ans = pa_iter_new(ans->A);
  for (int i=0; i<ans->n; i++) {
    if (_party < 2) {
      //pa_set(ans->A, i, i);
      pa_iter_set(itr_ans, i);
    }
  }
  pa_iter_flush(itr_ans);
  return ans;
}
#define Perm_ID2(n, q) Perm_ID2_type(n, q, PA_PACK)


static share_array Perm_ID(share_array a)
{
  if (a == NULL) {
    printf("Perm_ID: a = %p\n", a);
    return NULL;
  }
  if (a->q < a->n) {
    printf("Perm_ID: n = %d q = %d", a->n, (int)a->q);
  }
  NEWT(share_array, ans);
  *ans = *a;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;

  ans->A = pa_new_type(a->n, a->A->w, a->A->type);
  pa_iter itr_ans = pa_iter_new(ans->A);
  for (int i=0; i<ans->n; i++) {
    if (_party < 2) {
      pa_iter_set(itr_ans, i);
    }
  }
  pa_iter_flush(itr_ans);
  return ans;
}


/////////////////////////////////////////////
// Functions that can be computed locally
// Functions that can be computed locally
/////////////////////////////////////////////




///////////////////////////////////////
// x is public plaintext
///////////////////////////////////////
static void share_setpublic(share_array a, int i, share_t x)
{
  if (a == NULL) {
    printf("share_setpublic: a = NULL\n");
    return;
  }
  if (_party > max_partyid(a)) return;
  if (i < 0) {
    i += a->n;
  }
  if (i < 0 || i >= a->n) {
    printf("share_setpublic n %d i %d\n", a->n, i);
  }
  share_t q = a->q;
  if (_party >= 2) {
    if (a->A != NULL) pa_set(a->A, i, 0);
  } else {
    pa_set(a->A, i, MOD(x));
  }
}
#define _setpublic share_setpublic

static void share_setpublics(share_array a, int is, int ie, share_t x)
{
  if (a == NULL) {
    printf("share_setpublic: a = NULL\n");
    return;
  }
  if (_party > max_partyid(a)) return;
  if (is < 0) {
    is += a->n;
  }
  if (ie < 0) {
    ie += a->n;
  }
  if (is < 0 || ie < 0 || ie > a->n || is >= ie) {
    printf("share_setpublic n %d is %d ie %d\n", a->n, is, ie);
  }
  share_t q = a->q;
  for (int i=is; i<ie; i++) {
    if (_party >= 2) {
      if (a->A != NULL) pa_set(a->A, i, 0);
    } else {
      pa_set(a->A, i, MOD(x));
    }
  }
}
#define _setpublics share_setpublics

static void share_addpublic(share_array a, int i, share_t x)
{
  if (a == NULL) {
    printf("share_addpublic: a = %p\n", a);
    return;
  }
  if (_party > max_partyid(a)) return;
  if (i < 0 || i >= a->n) {
    printf("share_addpublic n %d i %d\n", a->n, i);
  }
  share_t q = a->q;
  if (_party != 2) pa_set(a->A, i, MOD(pa_get(a->A,i) + x));
}
#define _addpublic share_addpublic

static void share_addpublic_all(share_array a, share_t x)
{
  if (a == NULL) {
    printf("share_addpublic: a = %p\n", a);
    return;
  }
  if (_party > max_partyid(a)) return;
  share_t q = a->q;
  for (int i = 0; i < a->n; i++) {
    if (_party != 2) pa_set(a->A, i, MOD(pa_get(a->A,i) + x));
  }
}
#define _addpublic_all share_addpublic_all

static void share_subpublic(share_array a, int i, share_t x)
{
  if (a == NULL) {
    printf("share_subpublic: a = %p\n", a);
    return;
  }
  if (_party > max_partyid(a)) return;
  if (i < 0 || i >= a->n) {
    printf("share_subpublic n %d i %d\n", a->n, i);
  }
  share_t q = a->q;
  if (_party != 2) pa_set(a->A, i, MOD(pa_get(a->A,i) + q - x));
}
#define _subpublic share_subpublic

static void share_subpublic_all(share_array a, share_t x)
{
  if (a == NULL) {
    printf("share_subpublic_all: a = %p\n", a);
    return;
  }
  if (_party > max_partyid(a)) return;
  share_t q = a->q;
  for (int i = 0; i < a->n; i++) {
    if (_party != 2) pa_set(a->A, i, MOD(pa_get(a->A,i) + q - x));
  }
}
#define _subpublic_all share_subpublic_all

static void share_addshare_shamir(share_array a, int i, share_array b, int j)
{
  if (a == NULL || b == NULL) {
    printf("share_addshare_shamir: a = %p b = %p\n", a, b);
    return;
  }
  if (_party > max_partyid(a)) return;
  if (i < 0 || i >= a->n) {
    printf("share_addshare a: n %d i %d\n", a->n, i);
  }
  if (j < 0 || j >= b->n) {
    printf("share_addshare b: n %d j %d\n", b->n, j);
  }
  if (a->q != b->q) {
    printf("share_addshare a->q %d b->q %d\n", (int)a->q, (int)b->q);
  }
  share_t q = a->q;
  pa_set(a->A,i,MOD(pa_get(a->A,i) + pa_get(b->A,j)));
}
#define _addshare_shamir share_addshare_shamir

////////////////////////////////////////////
// a[i] += b[j]
////////////////////////////////////////////
static void share_addshare(share_array a, int i, share_array b, int j)
{
  if (a == NULL || b == NULL) {
    printf("share_addshare: a = %p b = %p\n", a, b);
    return;
  }
  if (_party > max_partyid(a)) return;
  share_addshare_shamir(a, i, b, j);
}
#define _addshare share_addshare


////////////////////////////////////////////
// a[i] -= b[j]
////////////////////////////////////////////
static void share_subshare(share_array a, int i, share_array b, int j)
{
  if (a == NULL || b == NULL) {
    printf("share_subshare: a = %p b = %p\n", a, b);
    return;
  }
  if (_party > max_partyid(a)) return;
  if (i < 0 || i >= a->n) {
    printf("share_subshare a: n %d i %d\n", a->n, i);
  }
  if (j < 0 || j >= b->n) {
    printf("share_subshare b: n %d j %d\n", b->n, j);
  }
  if (a->q != b->q) {
    printf("share_subshare a->q %d b->q %d\n", (int)a->q, (int)b->q);
  }
  share_t q = a->q;
  pa_set(a->A,i,MOD(pa_get(a->A,i) + q - pa_get(b->A,j)));
}
#define _subshare share_subshare


////////////////////////////////////////////
// a[i] *= x
////////////////////////////////////////////
static void share_mulpublic(share_array a, int i, int x)
{
  if (a == NULL) {
    printf("share_mulpublic: a = %p\n", a);
    return;
  }
  if (_party > max_partyid(a)) return;
  if (i < 0 || i >= a->n) {
    printf("share_mulpublic n %d i %d\n", a->n, i);
  }
  share_t q = a->q;
  pa_set(a->A, i, LMUL(pa_get(a->A,i), x, q));
}
#define _mulpublic share_mulpublic


////////////////////////////////////////////////////////////////////
// Should work for GF/additive, 22ADD, 33ADD, and RSS
////////////////////////////////////////////////////////////////////
static share_array vadd(share_array a, share_array b)
{
  if (a == NULL || b == NULL) {
    printf("vadd: a = %p b = %p\n", a, b);
    return NULL;
  }
  int n = a->n;
  share_t q = a->q;
  if (a->n != b->n) {
    printf("vadd a->n = %d b->n = %d\n", a->n, b->n);
  }
  if (a->q != b->q) {
    printf("vadd a->q = %d b->q = %d\n", (int)a->q, (int)b->q);
  }
  if (a->irr_poly != b->irr_poly) {
    printf("vadd: a->irrpoly = %x b->irrpoly = %x\n", a->irr_poly, b->irr_poly);
  }
  NEWT(share_array, ans);
  *ans = *a;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(a->n, a->A->w, a->A->type);
  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_a = pa_iter_new(a->A);
  pa_iter itr_b = pa_iter_new(b->A);
  for (int i=0; i<n; i++) {
    share_t za = pa_iter_get(itr_a);
    share_t zb = pa_iter_get(itr_b);
    share_t zc;
    if (ans->irr_poly) {
      zc = za ^ zb;
    } else {
      zc = MOD(za + zb);
    }
    pa_iter_set(itr_ans, zc);
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_a); pa_iter_free(itr_b);
  return ans;
}
#define _vadd vadd

static void vadd_(share_array a, share_array b)
{
  if (a == NULL || b == NULL) {
    printf("vadd_: a = %p b = %p\n", a, b);
    return;
  }
  share_array tmp = vadd(a, b);
  pa_free(a->A);  *a = *tmp;  free(tmp);
}
#define _vadd_ vadd_

static share_array vsub(share_array a, share_array b)
{
  if (a == NULL || b == NULL) {
    printf("vsub: a = %p b = %p\n", a, b);
    return NULL;
  }
  int n = a->n;
  share_t q = a->q;
  if (a->n != b->n) {
    printf("vsub a->n = %d b->n = %d\n", a->n, b->n);
  }
  if (a->q != b->q) {
    printf("vsub a->q = %d b->q = %d\n", (int)a->q, (int)b->q);
  }
  if (a->irr_poly != b->irr_poly) {
    printf("vsub: a->irrpoly = %d b->irrpoly = %d\n", a->irr_poly, b->irr_poly);
  }
  NEWT(share_array, ans);
  *ans = *a;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(a->n, a->A->w, a->A->type);
  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_a = pa_iter_new(a->A);
  pa_iter itr_b = pa_iter_new(b->A);
  for (int i=0; i<n; i++) {
    share_t za = pa_iter_get(itr_a);
    share_t zb = pa_iter_get(itr_b);
    share_t zc;
    if (ans->irr_poly) {
      zc = za ^ zb;
    } else {
      zc = MOD(za + q - zb);
    }
    pa_iter_set(itr_ans, zc);
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_a); pa_iter_free(itr_b);
  return ans;
}
#define _vsub vsub

static void vsub_(share_array a, share_array b)
{
  if (a == NULL || b == NULL) {
    printf("vsub_: a = %p b = %p\n", a, b);
    return;
  }
  share_array tmp = vsub(a, b);
  pa_free(a->A);  *a = *tmp;  free(tmp);
}
#define _vsub_ vsub_


///////////////////////////////////////////////////
// Add random r and -r to a
// Can also be used to share a random sequence (maybe better as a separate function?)
// TODO: switch to shared_random_channel_type
///////////////////////////////////////////////////
static void share_randomize(share_array a)
{
  if (_party >  2) return;
  if (_party <= 0) return;

  if (a->type != SHARE_T_22ADD) {
    printf("share_randomize: type = %d\n", a->type);
  }

  share_t q, x, r;
  q = order(a);
  int n = len(a);
  _ b = _dup(a);
  for (int i=0; i<n; i++) {
    r = RANDOM0(q);
    share_setraw(b, i, r);
  }
  _ c = _dup(a);
  mpc_exchange_pa_channel(b->A, c->A, 0);
  if (_party == 1) {
    vadd_(a, b);
    vsub_(a, c);
  } else {// _party == 2
    vadd_(a, b);
    vsub_(a, c);
  }
  _free(b);
  _free(c);
}
#define _randomize share_randomize

_pair share_separate(_ pa)
{
  if (_party > 2) {
    _pair ans = {NULL, NULL};
    return ans;
  }
  int n = len(pa);
  share_t q = order(pa);

  _ a = _const(n, 0, q);
  _ b = _const(n, 0, q);
  if (_party <= 1) {
    for (int i=0; i<n; i++) {
      share_t x = share_getraw(pa, i);
      share_setraw(a, i, x);
      share_setraw(b, i, 0);
    }
  } else { // _party == 2
    for (int i=0; i<n; i++) {
      share_t x = share_getraw(pa, i);
      share_setraw(a, i, 0);
      share_setraw(b, i, x);
    }
  }
  _pair ans = {a, b};
  return ans;
}


static share_t GF_mul(share_t a, share_t b, share_t irr_poly); // in field.h

/*
  TODO
  - smul also handles GF arithmetic, but field.h already has smul_GF.
*/

static share_array smul(share_t s, share_array a) // s is public
{
  if (a == NULL) {
    printf("smul: a = %p\n", a);
    return NULL;
  }
  int n = a->n;
  share_t q = a->q;
  NEWT(share_array, ans);
  *ans = *a;
  ans->A = NULL;
  if (_party > max_partyid(a)) return ans;
  ans->A = pa_new_type(a->n, a->A->w, a->A->type);
  pa_iter itr_ans = pa_iter_new(ans->A);
  pa_iter itr_a = pa_iter_new(a->A);
  for (int i=0; i<n; i++) {
//#ifdef _FIELD_H
    if (ans->irr_poly) {
      share_t c;
      c = GF_mul(s, pa_iter_get(itr_a), ans->irr_poly);
      pa_iter_set(itr_ans, c);
    } else {
      pa_iter_set(itr_ans, LMUL(s, pa_iter_get(itr_a), q));
    }
//#else
//    pa_iter_set(itr_ans, LMUL(s, pa_iter_get(itr_a), q));
//#endif
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_a);
  return ans;
}
#define _smul smul
#define python_smul(s, a) smul(a, s)

static void smul_(share_t s, share_array a)
{
  if (a == NULL) {
    printf("smul_: a = %p\n", a);
    return;
  }
  share_array tmp = smul(s, a);
  pa_free(a->A);  *a = *tmp;  free(tmp);
}
#define _smul_ smul_
#define python_smul_(s, a) smul_(a, s)

_ Modulo_channel(_ x, int k, share_t new_q, int channel); // in unitv.h
static share_array smod(share_t s, share_array a) // s is public
{
  int k = blog(s-1)+1;
  if ((1 << k) != s) {
    printf("smod: s = %d is not a power of two\n", (int)s);
    exit(1);
  }
  return Modulo_channel(a, k, order(a), 0);
}
#define python_smod(s, a) smod(a, s)

static void smod_(share_t s, share_array a)
{
  if (a == NULL) {
    printf("smod_: a = %p\n", a);
    return;
  }
  share_array tmp = smod(s, a);
  pa_free(a->A);  *a = *tmp;  free(tmp);
}
#define python_smod_(s, a) smod_(a, s)

//////////////////////////////////////////////////////////////////////////
// Reduce the modulus of additive shares
// (no communication)
//////////////////////////////////////////////////////////////////////////
_ share_shrink(_ a, share_t q)
{
  if (_party >  2) return NULL;

  share_t qa = order(a);
  if (q > qa) {
    printf("share_shrink: qa = %d q = %d\n", (int)qa, (int)q);
  }
  int k1 = blog(qa-1)+1;
  int k2 = blog(q-1)+1;
  if ((1 << k1) != qa) {
    printf("share_shrink: %d is not a power of two\n", (int)qa);
  }
  if ((1 << k2) != q) {
    printf("share_shrink: %d is not a power of two\n", (int)q);
  }
  int n = len(a);

  _ ans = _const(n, 0, q);

  for (int i=0; i<n; i++) {
    pa_set(ans->A, i, MOD(pa_get(a->A, i)));
  }

  return ans;
}
#define _shrink share_shrink


//////////////////////////////////////////////////////////////////////////
// Left shift additive shares (modulus 1<<k0) by k bits and convert to modulus 1<<(k0+k)
// (no communication)
//////////////////////////////////////////////////////////////////////////
_ share_lshift_extend(_ a, int k)
{
  if (_party >  max_partyid(a)) return NULL;

  if (a->type != SHARE_T_22ADD && a->type != SHARE_T_33ADD && a->type != SHARE_T_RSS && a->type != SHARE_T_SHAMIR) {
    printf("share_lshift_extend: type = %d\n", a->type);
    exit(1);
  }
  if (a->irr_poly != 0) {
    printf("share_lshift_extend: irr_poly = %d\n", a->irr_poly);
    exit(1);
  }

  share_t q0 = order(a);
  int k0 = blog(q0-1)+1;
  if ((1 << k0) != q0) {
    printf("share_lshift_extend: %d is not a power of two\n", (int)q0);
  }
  share_t q = q0 << k;
  int n = len(a);

  _ ans = share_const_type(n, 0, q, a->type);
  ans->irr_poly = a->irr_poly;

  NEWITER(itr_a, a);
  NEWITER(itr_ans, ans);
  for (int i=0; i<n; i++) {
    share_t x = pa_iter_get(itr_a);
    pa_iter_set(itr_ans, x << k);
  }
  pa_iter_flush(itr_ans); pa_iter_free(itr_a);

  return ans;
}
#define LeftShift share_lshift_extend

/////////////////////////////////////////////
// Functions that require communication
/////////////////////////////////////////////


static share_array vmul_channel(share_array x, share_array y, int channel)
{
  if (x == NULL || y == NULL) {
    printf("vmul: x = %p y = %p\n", x, y);
    return NULL;
  }
  if (_party > 0 && (x->type != SHARE_T_22ADD || y->type != SHARE_T_22ADD)) {
    printf("vmul: x->type = %d y->type = %d\n", x->type, y->type);
    return NULL;
  }
  if (_party >  2) {
    NEWT(_, ans);
    *ans = *x;
    ans->A = NULL;
    return ans;
  }
  int n = x->n;
  share_t q = x->q;
  int i;
  if (x->n != y->n) {
    printf("vmul x->n = %d y->n = %d\n", x->n, y->n);
  }
  if (x->q != y->q) {
    printf("vmul x->q = %d y->q = %d\n", (int)x->q, (int)y->q);
  }
  NEWT(share_array, ans);
  *ans = *x;
  ans->A = NULL;
  ans->A = pa_new_type(n, x->A->w, x->A->type);

// Beaver Triple computation
  BeaverTriple bt = NULL;

  if (BT_tbl[channel] != NULL) {
    bt = BeaverTriple_new3(n, q, BT_tbl[channel]); // precomputed
  } else {
    if (_opt.warn_precomp) printf("without bt tbl\n");
    if (_party >= 0) bt = BeaverTriple_new_channel(n, q, x->A->w, channel);
  }
  if (_party <= 0) {
    pa_iter itr_ans = pa_iter_new(ans->A);
    pa_iter itr_x = pa_iter_new(x->A);
    pa_iter itr_y = pa_iter_new(y->A);
    for (i=0; i<n; i++) {
      pa_iter_set(itr_ans, LMUL(pa_iter_get(itr_x), pa_iter_get(itr_y), q));
    }
    pa_iter_flush(itr_ans); pa_iter_free(itr_x); pa_iter_free(itr_y);
    if (_party == -1) return ans;
  } else {
    NEWT(share_array, a);
    *a = *x;
    a->A = bt->a;
    NEWT(share_array, b);
    *b = *x;
    b->A = bt->b;

    share_array sigma, rho;
    sigma = vsub(x, a);
    rho = vsub(y, b);
    share_array sigma_c, rho_c;
    sigma_c = share_reconstruct_channel(sigma, channel);
    rho_c = share_reconstruct_channel(rho, channel);

    pa_iter itr_ans = pa_iter_new(ans->A);
    pa_iter itr_sc = pa_iter_new(sigma_c->A);
    pa_iter itr_rc = pa_iter_new(rho_c->A);
    pa_iter itr_a = pa_iter_new(a->A);
    pa_iter itr_b = pa_iter_new(b->A);
    pa_iter itr_c = pa_iter_new(bt->c);

    for (i=0; i<n; i++) {
      share_t tmp;
      share_t sc = pa_iter_get(itr_sc);
      share_t rc = pa_iter_get(itr_rc);
      if (_party == 1) {
        tmp = LMUL(sc, rc, q);
      } else {
        tmp = 0;
      }
      tmp += LMUL(pa_iter_get(itr_a), rc, q);  
      tmp += LMUL(pa_iter_get(itr_b), sc, q);
      tmp = MOD(tmp + pa_iter_get(itr_c));
      pa_iter_set(itr_ans, tmp);
    }
    pa_iter_flush(itr_ans); pa_iter_free(itr_sc); pa_iter_free(itr_rc);
    pa_iter_free(itr_a); pa_iter_free(itr_b); pa_iter_free(itr_c); 

    pa_free(bt->c);
    share_free(a);  share_free(b);
    share_free(sigma); share_free(rho);
    share_free(sigma_c); share_free(rho_c);
  }
  BeaverTriple_free(bt);


  return ans;
}
#define _vmul vmul
#define vmul(x, y) vmul_channel(x, y, 0)

static void vmul_channel_(share_array a, share_array b, int channel)
{
  if (a == NULL || b == NULL) {
    printf("vmul_: a = %p b = %p\n", a, b);
    return;
  }
  share_array tmp = vmul_channel(a, b, channel);
  pa_free(a->A);  *a = *tmp;  free(tmp);
}
#define _vmul_ vmul_
#define vmul_(x, y) vmul_channel_(x, y, 0)




////////////////////////////////////////////////////////
// Convert bit shares to modulus q
////////////////////////////////////////////////////////
/**
 * @brief Converts a binary secret share to an arithmetic secret share online (without precomputed table).
 *
 * This function performs an online B2A (Binary to Arithmetic) conversion for a secret shared value.
 * It operates in a 3-party computation setting where _party <= 2.
 *
 * @param a       Input secret share in binary field (q must be 2).
 * @param q       The modulus for the output arithmetic share.
 * @param channel The communication channel index to use for sending/receiving data.
 *
 * @return A pointer to the resulting arithmetic secret share, or NULL if _party > 2.
 *
 * @note If _opt.warn_precomp is set, a warning message will be printed indicating
 *       that the function is running without a precomputed B2A table.
 * @note The input share `a` must be in binary field (a->q == 2), otherwise the
 *       function will print an error and exit.
 *
 * @details
 * - Server (_party <= 0): Generates random correlated values c1, c2 and sends
 *   a masked share `f` to Party 1 via the specified channel.
 * - Party 1 (_party == 1): Receives `f` from the server and computes its arithmetic share.
 * - Party 2 (_party == 2): Uses pseudo-random values from the server's seed to
 *   compute its arithmetic share.
 * - Parties 1 and 2 collaboratively reconstruct a masked bit and select the
 *   appropriate arithmetic share component.
 */
_ B2A_online_channel(_b a, share_t q, int channel)
{
  if (_party >  2) return NULL;
  if (_opt.warn_precomp) {
    printf("without B2A_table\n"); fflush(stdout);
  }
  share_t r;
  if (a->q != 2) {
    printf("B2A: q = %d\n", a->q);
    exit(1);
  }
  int n = a->n;
  _ ans = _const(n, 0, q);
  if (_party <= 0) {
    // generate c1
    _ c1 = _const(n, 0, 2);
    for (int i=0; i<n; i++) {
      share_t r;
      r = RANDOM(mt_[TO_PARTY1][channel], 2);
      pa_set(c1->A, i, r);
    }

    // generate c2
    _ c2 = _const(n, 0, 2);
    for (int i=0; i<n; i++) {
    //  r = RANDOM(m0, 2);
      r = RANDOM(mt_[TO_PARTY2][channel], 2);
      pa_set(c2->A, i, r);
    }
    _ c = _const(n, 0, 2);
    for (int i=0; i<n; i++) {
      pa_set(c->A, i, (pa_get(c1->A, i) + pa_get(c2->A, i)) % 2);
    }

    _ f = _const(n, 0, q);
    for (int i=0; i<n; i++) {
      r = RANDOM(mt_[TO_PARTY2][channel], q);
      pa_set(f->A, i, MOD(r + pa_get(c->A, i)));
    }
    mpc_send_share_channel(TO_PARTY1, f, channel);

    _free(c1);
    _free(c2);
    _free(c);
    _free(f);

    for (int i=0; i<n; i++) {
      pa_set(ans->A, i, pa_get(a->A, i));
    }
  } else {
    _ c = _const(n, 0, 2);
    for (int i=0; i<n; i++) {
      r = RANDOM(mt_[FROM_SERVER][channel], 2);
      //if (i == 0) printf("[r %d\n]", r);
      pa_set(c->A, i, r);
    }

    _ f0 = _const(n, 0, q);
    _ f1 = _const(n, 0, q);

    if (_party == 1) {
      mpc_recv_channel(FROM_SERVER, f0->A->B, pa_size(f0->A), channel);
      for (int i=0; i<n; i++) {
        pa_set(f1->A, i, MOD(q+1 - pa_get(f0->A, i)));
      }
    } else { // party 2
      for (int i=0; i<n; i++) {
        r = RANDOM(mt_[FROM_SERVER][channel], q);
        pa_set(f0->A, i, MOD(q-r));
        pa_set(f1->A, i, MOD(r));
      }
    }
    _ b = vsub(a, c);
    _ b_c = _reconstruct_channel(b, channel);
    for (int i=0; i<n; i++) {
      if (pa_get(b_c->A, i) == 0) {
        pa_set(ans->A, i, MOD(pa_get(f0->A, i)));
      } else {
        pa_set(ans->A, i, MOD(pa_get(f1->A, i)));
      }
    }
    _free(f0);
    _free(f1);
    _free(b);
    _free(b_c);
    _free(c);
  }
  return ans;
}

/**
 * @brief Computes the overflow bit for a shared value using online channel communication.
 *
 * This function determines the overflow bit of a shared value by extracting
 * the least significant bit of the additive share and performing a multiplication
 * between two party-specific shares over the specified communication channel.
 *
 * @param a       Input share array of length n.
 * @param q       The modulus used for the additive share computation.
 * @param channel The communication channel index to use for the multiplication.
 *
 * @return A new share representing the overflow bit result,
 *         or NULL if the party index is greater than 2.
 *
 * @note This function only supports up to 2 parties (_party <= 2).
 *       - Party 2 computes b1 as the least significant bit of each element in a.
 *       - Other parties compute b2 as the least significant bit of each element in a.
 *       The final result is computed via vmul_channel(b1, b2, channel).
 *       Both b1 and b2 are freed after the multiplication.
 */
_ overflow1_online_channel(_b a, share_t q, int channel)
{
  if (_party >  2) {
    return NULL;
  }
  int n = len(a);

  _ b1 = _const(n, 0, q);
  if (_party == 2) {
    for (int i=0; i<n; i++) {
      pa_set(b1->A, i, (q+pa_get(a->A, i)) % 2); // least significant bit of additive share
    }
  }
  _ b2 = _const(n, 0, q);
  if (_party != 2) {
    for (int i=0; i<n; i++) {
      pa_set(b2->A, i, (q+pa_get(a->A, i)) % 2); // least significant bit of additive share
    }
  }
  _ ans = vmul_channel(b1, b2, channel);

  _free(b1); _free(b2);
  return ans;
}


///////////////////////////////////////////////////
// Create an array of shared random numbers
///////////////////////////////////////////////////
static _ shared_random_channel_type(int n, share_t q, int party1, int party2, int channel, int pa_type)
{
  _ ans;
  if (_party <= 0) {
    ans = share_const_type2(n, 0, q, SHARE_T_ADDITIVE, pa_type);
    return ans;
  }
  if (_party != party1 && _party != party2) return NULL;

  ans = share_const_type2(n, 0, q, SHARE_T_ADDITIVE, pa_type);
  int pair = party1 + party2 - _party;
  for (int i=0; i<n; i++) {
    pa_set(ans->A, i, RANDOM(mt_[pair][channel], q));
  }
  return ans;
}
#define shared_random_channel(n, q, party1, party2, channel) shared_random_channel_type(n, q, party1, party2, channel, PA_PACK)
#define shared_random(n, q, p1, p2) shared_random_channel(n, q, p1, p2, 0)





#endif
