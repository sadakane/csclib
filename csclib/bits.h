#ifndef _BITS_H
 #define _BITS_H

/*
  Memory management
  - bit vectors
  - compressed array storage

  - communication is not handled here
*/


#include <string.h>

typedef long i64;
typedef unsigned long u64;
typedef unsigned char uchar;
typedef share_t pa_t;

#ifdef FAST_BIT
 #define logQ 6
 typedef u64 bitvec_t;
#else
 #define logQ 6
 typedef u64 bitvec_t;
#endif

#define _Q (1L<<logQ) // size of a small block (one word)

#define ID_PACKEDARRAY 0x18
#define ID_SHARE 0x1C
#define ID_SCSA 0x1D

#ifndef NEWA
 #define NEWA(p,t,n) {p = (t*)malloc((n)*sizeof(*p));if ((p)==NULL) {printf("not enough memory\n"); exit(1);};}
#endif

#ifndef NEWT
 #define NEWT(t, p) \
  t p; \
  p = (t)malloc(sizeof(*p)); \
  if ((p)==NULL) {printf("not enough memory\n"); exit(1);}
#endif


/////////////////////////////////////////////////////
// Compute the bit length of an integer
// to store an integer in [0,x-1], we need blog(x-1)+1 bits
/////////////////////////////////////////////////////
static int blog(u64 x)
{
int l;
  l = -1;
  while (x>0) {
    x>>=1;
    l++;
  }
  return l;
}



static int setbit(bitvec_t *B, i64 i,int x)
{
  i64 j,l;

  j = i / _Q;
//  l = i & (_Q-1);
  l = _Q - (i & (_Q-1)) -1; // !!!
  if (x==0) B[j] &= (~(1L<<(l)));
  else if (x==1) B[j] |= (1L<<(l));
  else {
    printf("error setbit x=%d\n",x);
    exit(1);
  }
  return x;
}

static int getbit(bitvec_t *B, i64 i)
{
  i64 j,l;

  j = i >> logQ;
//  l = i & (_Q-1);
  l = _Q - (i & (_Q-1)) -1; // !!!
  return (B[j] >> (l)) & 1;
}

static u64 getbits(bitvec_t *B, i64 i, int d)
{
  u64 x,z;

  if (d == 0) return 0;
  B += (i >>logQ);
  i &= (_Q-1);
  if (i+d <= _Q) {
    x = B[0];
    x <<= i;
    x >>= (_Q-d);  // does not work when Q==64 and d==0
  } else {
    x = B[0] << i;
    x >>= _Q-d;
    z = B[1] >> (_Q-(i+d-_Q));
    x += z;
  }
  return x;
}

static void setbits(bitvec_t *B, i64 i, int d, u64 x)
{
  u64 y,m;
  int d2;

  B += (i>>logQ);
  i &= (_Q-1);

  while (i+d > _Q) {
    d2 = _Q-i; // store the upper d2 bits of x
    y = x >> (d-d2);
    m = (1<<d2)-1;
    *B = (*B & (~m)) | y;
    B++;  i=0;
    d -= d2;
    x &= (1L<<d)-1; // clear upper bits of x
  }
  m = (1L<<d)-1;
  y = x << (_Q-i-d);
  m <<= (_Q-i-d);
  *B = (*B & (~m)) | y;

}

static void writeuint(int k, i64 x, FILE *f)
{
  int i;
  for (i=k-1; i>=0; i--) {
    fputc(x & 0xff,f); // little endian
    x >>= 8;
  }
}

static u64 getuint(uchar *s, i64 i, i64 w)
{
  u64 x;
  i64 j;
  s += i*w;
  x = 0;
  for (j=0; j<w; j++) {
    x += ((u64)(*s++)) << (j*8);
  }
  return x;
}

static void putuint(uchar *s, i64 i, i64 x, i64 w)
{
  i64 j;
  s += i*w;
  for (j=0; j<w; j++) {
    *s++ = x & 0xff;
    x >>= 8;
  }
}

typedef struct {
  i64 n;
  int w;
  bitvec_t *B;

  int type; // bit compression method

}* packed_array;

#define PA_PACK 0
//#define PA_BIT 1
#define PA_SIGN 2
//#define PA_ 3
#define PA_RAW 4

/******************************************************************
 * Planned bit packing scheme
 * 1 bit -> 1 bit (addition computed with XOR)
 * k bit -> stored in k+1 bits (1 bit reserved for comparison)
 * A W-bit word stores floor(W/(k+1)) values
******************************************************************/


static int pa_W(i64 w) // number of bits to represent one element
{
  if (w <= 0 || w >= _Q) {
    printf("pa_W: w = %ld\n", w);
    return -1;
  }
  if (w == 1) return 1;
  return w+1;
}


static i64 pa_size(packed_array p)
{
  i64 n = p->n;
  i64 w = p->w;
  i64 num_words;

  switch(p->type) {
    case PA_PACK:
      num_words = (n / _Q)*w + ((n % _Q)*w + _Q-1) / _Q;
      break;
    case PA_SIGN:
      {
        i64 m;
        m = _Q / pa_W(p->w); // number of elements storable in one word
        num_words = (n + m-1) / m; // number of words required for storage
      }
      break;
    case PA_RAW:
      num_words = n; // one word per element
      break;
    default:
      printf("pa_size: type=%d\n", p->type);
      num_words = 0; // dummy
      break;
  }

  return num_words*sizeof(bitvec_t);
}


static packed_array pa_new_type(i64 n, int w, int type)
{
  i64 num_words;

  NEWT(packed_array, p);
  p->n = n;  p->w = w;  p->type = type;
  num_words = pa_size(p);
  if (num_words == 0) num_words = 1; // allocate one word even when w == 0
  NEWA(p->B, bitvec_t, num_words);
  memset(p->B, 0, num_words*sizeof(bitvec_t));

  return p;
}
#define pa_new(n, w) pa_new_type(n, w, PA_PACK)


static packed_array pa_dup(packed_array a)
{
  i64 num_words;

#ifdef FAST_BIT
  w = _Q;
#endif

  NEWT(packed_array, p);
  *p = *a;
  num_words = pa_size(p);
  NEWA(p->B, bitvec_t, num_words);
  memcpy(p->B, a->B, num_words*sizeof(bitvec_t));

  return p;
}

static void pa_resize(packed_array a, i64 new_size)
{
  i64 num_words;
  i64 num_words_new;
  i64 n = a->n;

  num_words = pa_size(a);
  a->n = new_size;
  num_words_new = pa_size(a);
  if (num_words_new > num_words) {
    a->B = (bitvec_t*)realloc(a->B, num_words_new*sizeof(bitvec_t));
    if (a->B == NULL) {
      printf("pa_resize: not enough memory\n");
      exit(1);
    }
    memset(a->B + num_words, 0, (num_words_new - num_words)*sizeof(bitvec_t));
  }
}

static void pa_free(packed_array p)
{
  if (p == NULL) return;
  free(p->B);
  free(p);
}

static void pa_free_map(packed_array p)
{
  free(p);
}

static pa_t pa_get(packed_array p, i64 i)
{
  int w;
  bitvec_t *B;

  switch (p->type) {
    case PA_RAW: 
      return (pa_t)p->B[i];
    case PA_PACK:
      B = p->B;
      if (p->w == 1) return (pa_t)getbit(B,i);
      return (pa_t)getbits(B,i*p->w,p->w);
    case PA_SIGN:
      {
        i64 w = p->w;
        i64 W = pa_W(w);
        i64 m = _Q / W;
        i64 iq = i / m;
        i64 ir = i % m;
        B = p->B + iq;
        u64 z = getbits(B,ir * W + (W-w), w);
        return (pa_t)z;        
      }
    default:
      printf("pa_get: type=%d\n", p->type);
      break;
  }
  printf("pa_get: type=%d\n", p->type);
  return -1;
}

static void pa_set(packed_array p, i64 i, pa_t x)
{
  int w;
  bitvec_t *B;

  switch (p->type) {
    case PA_RAW: 
      p->B[i] = x & ((1<<p->w)-1);
      break;
    case PA_PACK:
      B = p->B;
      if (p->w == 1) {
        setbit(B,i,x);
        break;
      }
      setbits(B,i*p->w,p->w,x);
      break;
    case PA_SIGN:
      {
        i64 w = p->w;
        i64 W = pa_W(w);
        i64 m = _Q / W;
        i64 iq = i / m;
        i64 ir = i % m;
        B = p->B + iq;
        setbits(B,ir * W, W, x); // input should be w bits; assume higher bits are 0 and write W bits
      }
      break;
    default:
      printf("pa_set: type=%d\n", p->type);
      break;
  }

}



#define _W (8*sizeof(bitvec_t))


typedef struct pa_iter {
  bitvec_t *p;
  bitvec_t x;
  int k, w;
  int type;

// tmp
  packed_array a;
}* pa_iter;

#define NEWITER(var, for) pa_iter var = pa_iter_new(for->A);

pa_iter pa_iter_new(packed_array p)
{
  NEWT(pa_iter, itr);
  if (p == NULL) {
    itr->p = NULL;
    itr->x = 0;
    itr->k = 0;
    itr->type = -1;
    return itr;
  }
  itr->p = p->B;
  itr->x = 0;
  itr->k = 0;
  itr->type = p->type;

  switch (p->type) {
    case PA_RAW:
      itr->w = 0;
      break;
    case PA_PACK:
      itr->w = p->w;
      break;
    case PA_SIGN:
      itr->w = 0; // temporary
      break;
  }


  itr->a = p;
  return itr;
}

pa_iter pa_iter_pos_new_packed(packed_array p, i64 pos)
{
  NEWT(pa_iter, itr);
  i64 q = (pos*p->w) / _W;
  int r = (int)((pos*p->w) & (int)(_W-1));
  itr->p = p->B + q;
  itr->x = *(itr->p)++;
  itr->x <<= r;
  itr->k = _W-r;
  itr->w = p->w;
  itr->type = p->type;

  itr->a = p;
  return itr;
}

pa_iter pa_iter_pos_new(packed_array p, i64 pos)
{
  if (p->type == PA_PACK) return pa_iter_pos_new_packed(p, pos); 

  NEWT(pa_iter, itr);
  itr->p = p->B;
  itr->k = pos;
  itr->w = p->w;
  itr->type = p->type;

  itr->a = p;
  return itr;
}


pa_t pa_iter_get_packed(pa_iter itr)
{
  pa_t ans = 0;
  int w = itr->a->w; // temporary
  int k = itr->k;
  bitvec_t x = itr->x;
  if (k < w) { // not enough bits
    bitvec_t x2 = *(itr->p)++;
    x += (x2 >> k);
    ans = x >> (_W-w);
    x = x2 << (w-k);
    k += _W;
  } else {
    ans = x >> (_W-w);
    x <<= w;
  }
  k -= w;

  itr->x = x;
  itr->k = k;

  return ans;
}

pa_t pa_iter_get(pa_iter itr)
{
  if (itr->p == NULL) return 0;
  if (itr->type == PA_PACK) return pa_iter_get_packed(itr); 

  i64 k = itr->k;
  pa_t ans = pa_get(itr->a, k);
  itr->k = k+1;
  return ans;
}

void pa_iter_set_packed(pa_iter itr, pa_t z)
{
  int w = itr->w;
  int k = itr->k;
  bitvec_t x = itr->x;

  if (k + w > (int)_W) { // bit overflow
    x += z >> (k+w-_W);
    *(itr->p)++ = x;
    k -= _W;
    x = 0;
  }
  x += ((bitvec_t)z) << (_W-k-w);
  k += w;
  itr->x = x;
  itr->k = k;
}

void pa_iter_set(pa_iter itr, pa_t z)
{
  if (itr->p == NULL) return;
  if (itr->type == PA_PACK) return pa_iter_set_packed(itr, z); 

  i64 k = itr->k;
  pa_set(itr->a, k, z);
  itr->k = k+1;
}

void pa_iter_free(pa_iter itr)
{
  free(itr);
}

void pa_iter_flush(pa_iter itr)
{
  if (itr->type == PA_PACK) {
    if (itr->w > 0) {
      *(itr->p) = itr->x;
    } else {
      *(itr->p) = itr->x;
      //printf("flush: ???\n");
    }
  }
  pa_iter_free(itr);
}


pa_t *pa_unpack_(packed_array p)
{
  i64 n = p->n;
  pa_t *q;
  int w = p->w;
  NEWA(q, pa_t, n);
  bitvec_t *B = p->B;
  bitvec_t x1 = 0;
  int k = 0; // number of bits not read yet
  for (i64 i=0; i<n; i++) {
    if (k < w) { // not enough bits
      bitvec_t x2 = *B++;
      x1 += x2 >> k;
      q[i] = x1 >> (_W-w);
      x1 = x2 << (w-k);
      k += _W;
    } else {
      q[i] = x1 >> (_W-w);
      x1 <<= w;
    }
    k -= w;
  }
  return q;
}

pa_t *pa_unpack(packed_array p)
{
  if (p->type == PA_PACK) return pa_unpack_(p);
  if (p->type != PA_RAW) {
    printf("pa_unpack: type = %d\n", p->type);
    exit(1);
  }
  i64 n = p->n;
  pa_t *q;
  int w = p->w;
  NEWA(q, pa_t, n);

  for (i64 i=0; i<n; i++) {
    q[i] = pa_get(p, i);
  }

  return q;
}

packed_array pa_pack_(i64 n, int w, pa_t *q)
{
  packed_array p = pa_new(n, w);

  bitvec_t *B = p->B;
  bitvec_t x1 = 0;
  int k = 0; // number of bits already stored
  for (i64 i=0; i<n; i++) {
    if (k + w > (int)_W) { // bit overflow
      x1 += ((bitvec_t)q[i]) >> (k+w-_W);
      *B++ = x1;
      x1 = ((bitvec_t)q[i]) << (2*_W-k-w);
      k += w-_W;
    } else {      
      x1 += ((bitvec_t)q[i]) << (_W-k-w);
      k += w;
    }
  }
  if (k > 0) {
    *B++ = x1;
  }
  return p;
}

packed_array pa_convert(packed_array a, int type)
{
  int n = a->n;
  packed_array ans = pa_new_type(n, a->w, type);
  pa_iter itr_a = pa_iter_new(a);
  pa_iter itr_ans = pa_iter_new(ans);
  for (int i=0; i<n; i++) pa_iter_set(itr_ans, pa_iter_get(itr_a));
  pa_iter_flush(itr_ans);
  pa_iter_free(itr_a);
  return ans;
}

packed_array pa_pack_type(i64 n, int w, pa_t *q, int type)
{
  if (type == PA_PACK) return pa_pack_(n, w, q);
  if (type != PA_RAW) {
    printf("pa_pack: type = %d\n", type);
    exit(1);
  }
  packed_array p = pa_new_type(n, w, PA_RAW);

  bitvec_t *B = p->B;
  bitvec_t x1 = 0;
  int k = 0; // number of bits already stored
  for (i64 i=0; i<n; i++) {
    pa_set(p, i, q[i]);
  }
  return p;
}
#define pa_pack(n, w, q) pa_pack_type(n, w, q, PA_PACK)







#undef _W





static packed_array pa_dup_type(packed_array a, int type)
{
  if (a->type == type) return pa_dup(a);

  packed_array b = pa_new_type(a->n, a->w, type);

  pa_iter itr_a = pa_iter_new(a);
  pa_iter itr_b = pa_iter_new(b);
  i64 n = a->n;
  for (i64 i=0; i<n; i++) {
    pa_iter_set(itr_b, pa_iter_get(itr_a));
  }
  pa_iter_flush(itr_b); pa_iter_free(itr_a);
  return b;
}






i64 pa_write(packed_array pa, FILE *f)
{
  i64 size = 0;
  writeuint(1,ID_PACKEDARRAY,f);
  writeuint(sizeof(pa->n), pa->n, f);
  writeuint(sizeof(pa->w), pa->w, f);
  size += 1 + sizeof(pa->n) + sizeof(pa->w);

  if (pa->type == PA_PACK) {
    i64 num_words = (pa->n / _Q)*pa->w + ((pa->n % _Q)*pa->w + _Q-1) / _Q;
    for (i64 i=0; i<num_words; i++) {
      writeuint(sizeof(pa->B[0]), pa->B[i], f); size += sizeof(pa->B[0]);
    }
  } else {
    packed_array tmp = pa_dup_type(pa, PA_PACK);
    i64 num_words = pa_size(tmp);
    for (i64 i=0; i<num_words; i++) {
      writeuint(sizeof(tmp->B[0]), tmp->B[i], f); size += sizeof(tmp->B[0]);
    }
    pa_free(tmp);
  }

  return size;
}

packed_array pa_read(uchar **map)
{
  i64 x;
  uchar *p;

  p = *map;

  x = getuint(p,0,1);  p += 1;
  if (x != ID_PACKEDARRAY) {
    printf("pa_read: id = %ld is not supported.\n", x);
    exit(1);
  }

  NEWT(packed_array, pa);

  pa->n = getuint(p,0,sizeof(pa->n));  p += sizeof(pa->n);
  pa->w = getuint(p,0,sizeof(pa->w));  p += sizeof(pa->w);
  pa->type = PA_PACK;
  pa->B = (bitvec_t *)p;
  x = (pa->n / _Q)*pa->w + ((pa->n % _Q)*pa->w + _Q-1) / _Q;
  p += x * sizeof(pa->B[0]);

  *map = p;

  return pa;
}

void pa_print(packed_array a)
{
  for (int i=0; i<a->n; i++) {
    printf("%ld ", (long)pa_get(a, i));
  }
  printf("\n");
}

#undef logQ
#undef _Q


#endif
