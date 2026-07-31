/*
  An implementation of MAESTRO: Multi-Party AES Using Lookup Tables
  Hiraku Morita, Erik Pohle, Kunihiko Sadakane, Peter Scholl, Kazunari Tozawa, Daniel Tschudi:
  MAESTRO: Multi-Party AES Using Lookup Tables. USENIX Security Symposium 2025: 1965-1984
  https://www.usenix.org/conference/usenixsecurity25/presentation/morita
*/

/********************************************************************
 * AES
 * https://qiita.com/tobira-code/items/152befa86bd515f67241
 * 
********************************************************************/

#include <stdio.h>

#define USE_RSS
//#define USE_SHAMIR


#include "share.h"
#include "shamir.h"
#include "rss.h"

#define NK_MAX (8)
#define NR_MAX (14)

enum {
  AES128 = 0,
  AES192 = 1,
  AES256 = 2,
};

uint8_t aes_type = AES128;
const uint8_t Nb = 4;

struct key_round {
  uint8_t Nk;
  uint8_t Nr;
} const key_round_table[] = {
  { 4, 10 },  // AES128(0)
  { 6, 12 },  // AES192(1)
  { 8, 14 },  // AES256(2)
};

const uint8_t sbox[] = {
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

/* x^(i-1) mod x^8 + x^4 + x^3 + x + 1 */
const uint32_t rcon[] = {
  0x00000000, /* invalid */
  0x00000001, /* x^0 */
  0x00000002, /* x^1 */
  0x00000004, /* x^2 */
  0x00000008, /* x^3 */
  0x00000010, /* x^4 */
  0x00000020, /* x^5 */
  0x00000040, /* x^6 */
  0x00000080, /* x^7 */
  0x0000001B, /* x^4 + x^3 + x^1 + x^0 */
  0x00000036, /* x^5 + x^4 + x^2 + x^1 */
};

static void print_Nwords(const uint8_t* word, int N)
{
  int i;

  for (i=0; i<N; i++) {
    uint8_t* p = (uint8_t*)(word+4*i);
    printf("%02x %02x %02x %02x ", p[0], p[1], p[2], p[3]);
  }
}

static uint32_t rot_word(uint32_t word)
{
  /* a3 a2 a1 a0 -> a0 a3 a2 a1 */
  return word << 24 | word >> 8;
}

static uint32_t sub_word(uint32_t word)
{
  uint32_t val = word;
  uint8_t* p = (uint8_t*)&val;
  p[0] = sbox[p[0]]; p[1] = sbox[p[1]];
  p[2] = sbox[p[2]]; p[3] = sbox[p[3]];
  return val;
}

void key_expansion(const uint8_t* key /*Nk*/, uint8_t* w /*Nb*(Nr+1)*/)
{
  int i;
  uint8_t Nr = key_round_table[aes_type].Nr;
  uint8_t Nk = key_round_table[aes_type].Nk;

  memcpy(w, key, Nk*4);
  uint32_t *w2 = (uint32_t *)w; // the result depends on endianness
  for (i=Nk; i<Nb*(Nr+1); i++) {
    uint32_t temp = w2[i-1]; // the result depends on endianness
    //printf("i %d temp %08x\n", i, temp);
    if (i%Nk == 0) {
      //printf("rot %08x\n", sub_word(rot_word(temp)));
      temp = sub_word(rot_word(temp)) ^ rcon[i/Nk];
      //printf("temp2 %08x\n", temp);
    } else if (6<Nk && i%Nk == 4) {
      temp = sub_word(temp);
    }
    w2[i] = w2[i-Nk] ^ temp;
  }
}



///////////////////////////////////////////////////////////////////
// Computation based on additive shares
///////////////////////////////////////////////////////////////////

void print_hex(_s3x x)
{
  int n = len(x);
  _ tmp;
  if (_num_parties > 3) {
    tmp = shamir3_reconstruct_xor(x, 0x11b);
  } else {
    tmp = _reconstruct_xor(x);
  }
  for (int i=0; i<n; i++) {
    printf("%02x ", share_getraw(tmp, i));
  }
  _free(tmp);
}


// x[i] ^= y[j]
static void share_xor_(_ x, int i, _ y, int j)
{
  share_t xtmp, ytmp;
//  xtmp = pa_get(x->A, i);
  xtmp = share_getraw(x, i);
//  ytmp = pa_get(y->A, j);
  ytmp = share_getraw(y, j);
  xtmp ^= ytmp;
//  pa_set(x->A, i, xtmp);
  share_setraw(x, i, xtmp);
}

static void share_add_round_key(_s3x state, _s3x w, int pos, int k)
{
  int i;
  for (i=0; i<4*Nb*k; i++) {
    share_xor_(state, i, w, pos+(i % (4*Nb)));
  }
}


static _pair WOL(_x state, int inverse)
{
  int n = len(state);

  _x ah = share_const_type(n, 0, 16, state->type);
  _x al = share_const_type(n, 0, 16, state->type);
  ah->type = al->type = state->type;
  ah->irr_poly = al->irr_poly = 0x13;

  for (int i=0; i<n; i++) {
    share_t a[8], b[8], c[8];
    share_t x;
    x = share_getraw(state, i);
    for (int j=0; j<8; j++) {
      a[j] = (x>>j) & 1;
    }
    if (inverse) {// Inverse S-box https://en.wikipedia.org/wiki/Rijndael_S-box
      c[0] = a[2] ^ a[5] ^ a[7];
      c[1] = a[0] ^ a[3] ^ a[6];
      c[2] = a[1] ^ a[4] ^ a[7];
      c[3] = a[0] ^ a[2] ^ a[5];
      c[4] = a[1] ^ a[3] ^ a[6];
      c[5] = a[2] ^ a[4] ^ a[7];
      c[6] = a[0] ^ a[3] ^ a[5];
      c[7] = a[1] ^ a[4] ^ a[6];
      if (_party <= 1) {
        c[0] ^= 1;
        c[2] ^= 1;
      }
    } else {
      c[0] = a[0];
      c[1] = a[1];
      c[2] = a[2];
      c[3] = a[3];
      c[4] = a[4];
      c[5] = a[5];
      c[6] = a[6];
      c[7] = a[7];
    }


    b[4] = c[4] ^ c[5] ^ c[6]; // h0
    b[5] = c[1] ^ c[4] ^ c[6] ^ c[7]; // h1
    b[6] = c[2] ^ c[3] ^ c[5] ^ c[7]; // h2
    b[7] = c[5] ^ c[7]; // h3
    b[0] = c[0] ^ c[4] ^ c[5] ^ c[6]; // l0
    b[1] = c[1] ^ c[2]; // l1
    b[2] = c[1] ^ c[7]; // l2
    b[3] = c[2] ^ c[4]; // l3
    share_t bh = (b[7]<<3)^(b[6]<<2)^(b[5]<<1)^(b[4]<<0);
    share_t bl = (b[3]<<3)^(b[2]<<2)^(b[1]<<1)^(b[0]<<0);
    for (int j=0; j<8; j++) {
      share_setraw(ah, i, bh);
      share_setraw(al, i, bl);
    }
  }
  _pair tmp = {ah, al};
  return tmp;
}

static _ WOL_inverse(_x ah, _x al, int inverse)
{
  int n = len(ah);

  _x ans = share_const_type(n, 0, 256, ah->type);
  ans->irr_poly = 0x11b;

  for (int i=0; i<n; i++) {
    share_t a[8], b[8], c[8];
    share_t zh = share_getraw(ah, i);
    share_t zl = share_getraw(al, i);
    b[0] = (zl>>0) & 1;
    b[1] = (zl>>1) & 1;
    b[2] = (zl>>2) & 1;
    b[3] = (zl>>3) & 1;
    b[4] = (zh>>0) & 1;
    b[5] = (zh>>1) & 1;
    b[6] = (zh>>2) & 1;
    b[7] = (zh>>3) & 1;
    a[0] = b[0] ^ b[4];
    a[1] = b[4] ^ b[5] ^ b[7];
    a[2] = b[1] ^ b[4] ^ b[5] ^ b[7];
    a[3] = b[4] ^ b[5] ^ b[6] ^ b[1];
    a[4] = b[1] ^ b[3] ^ b[4] ^ b[5] ^ b[7];
    a[5] = b[2] ^ b[4] ^ b[5];
    a[6] = b[1] ^ b[2] ^ b[3] ^ b[4] ^ b[7];
    a[7] = b[2] ^ b[4] ^ b[5] ^ b[7];

    if (inverse == 0) { // Forward S-box https://en.wikipedia.org/wiki/Rijndael_S-box
      c[0] = a[0] ^ a[4] ^ a[5] ^ a[6] ^ a[7];
      c[1] = a[0] ^ a[1] ^ a[5] ^ a[6] ^ a[7];
      c[2] = a[0] ^ a[1] ^ a[2] ^ a[6] ^ a[7];
      c[3] = a[0] ^ a[1] ^ a[2] ^ a[3] ^ a[7];
      c[4] = a[0] ^ a[1] ^ a[2] ^ a[3] ^ a[4];
      c[5] = a[1] ^ a[2] ^ a[3] ^ a[4] ^ a[5];
      c[6] = a[2] ^ a[3] ^ a[4] ^ a[5] ^ a[6];
      c[7] = a[3] ^ a[4] ^ a[5] ^ a[6] ^ a[7];
      if (_party <= 1) {
        c[0] ^= 1;
        c[1] ^= 1;
        c[5] ^= 1;
        c[6] ^= 1;
      }
    } else {
      c[0] = a[0];
      c[1] = a[1];
      c[2] = a[2];
      c[3] = a[3];
      c[4] = a[4];
      c[5] = a[5];
      c[6] = a[6];
      c[7] = a[7];
    }
    share_t x = 0;
    for (int j=0; j<8; j++) {
      x <<= 1;
      x += c[7-j];
    }
    share_setraw(ans, i, x);
  }
  return ans;
}

share_t GF24_inv_table[16] = {0, 1, 9, 14, 13, 11, 7, 6, 15, 2, 12, 5, 10, 4, 3, 8};


static _x GF28_inverse_2party(_x x, int inverse)
{
  _pair tmp = WOL(x, inverse);
  _x ah = tmp.x;
  _x al = tmp.y;
  //printf("ah "); _print(ah); _print_debug_xor(ah);

  _x ah2 = vmul_GF(ah, ah, 0x13); // X^4 + X + 1
  //printf("ah2 "); _print(ah2); _print_debug_xor(ah2);
  _x ahl = vmul_GF(ah, al, 0x13);
  _x al2 = vmul_GF(al, al, 0x13);
  _x ahpl = vadd(ah, al);

  _x vtmp1 = smul(0xe, ah2);
  _x vtmp2 = vadd(vtmp1, ahl);
  _x v = vadd(vtmp2, al2);
  _free(vtmp1); _free(vtmp2);
  //printf("v "); _print(v); _print_debug_xor(v);
  _bits v_inv_tmp = tablelookup(v, GF24_inv_table, 16, 0x13);
  _x v_inv = B2A_GF(v_inv_tmp, 0x13);
  //printf("v_inv "); _print(v_inv); _print_debug_xor(v_inv);
  _free_bits(v_inv_tmp);
  _x aph = vmul_GF(ah, v_inv, 0x13);
  _x apl = vmul_GF(ahpl, v_inv, 0x13);
  _x ans = WOL_inverse(aph, apl, inverse);
  _free(aph); _free(apl); _free(ahpl);
  _free(ah2); _free(al2); _free(ahl);
  _free(ah); _free(al);
  _free(v); _free(v_inv);
  return ans;
}

#ifdef USE_SHAMIR
static _ GF28_inverse_3party(_s3x x, int inverse)
{
  _pair tmp = WOL(x, inverse);
  _s3x ah_tmp = tmp.x;
  _s3x al_tmp = tmp.y;

  _sx ah = shamir3_revert_xor(ah_tmp, 0x13);
  _sx al = shamir3_revert_xor(al_tmp, 0x13);
  _free(ah_tmp);  _free(al_tmp);

  _s3x ah2 = vmul_shamir_GF(ah, ah, 0x13); // X^4 + X + 1
  _s3x ahl = vmul_shamir_GF(ah, al, 0x13);
  _s3x al2 = vmul_shamir_GF(al, al, 0x13);
  _sx ahpl = vadd_shamir_GF(ah, al);

  _s3 vtmp1 = smul_shamir_GF(0xe, ah2, 0x13);
  _s3 vtmp2 = vadd_shamir_GF(vtmp1, ahl);
  _s3 v = vadd_shamir_GF(vtmp2, al2);
  _free(vtmp1); _free(vtmp2);
  _bits v_inv_tmp = tablelookup_3party(v, GF24_inv_table, 16, 0x13, SHARE_T_SHAMIR);
  _sx v_inv = B2A_GF(v_inv_tmp, 0x13);
  _free_bits(v_inv_tmp);
  _s3x aph_tmp = vmul_shamir_GF(ah, v_inv, 0x13);
  _s3x apl_tmp = vmul_shamir_GF(ahpl, v_inv, 0x13);
  _ ans = WOL_inverse(aph_tmp, apl_tmp, inverse);

  _free(v); _free(v_inv);
  _free(aph_tmp); _free(apl_tmp);
  _free(ah2);  _free(ahl);  _free(al2);  _free(ahpl);
  _free(ah);  _free(al);
  return ans;
}
#endif

#ifdef USE_RSS
static _ GF28_inverse_3party(_s3 x, int inverse)
{
  _pair tmp = WOL(x, inverse);
  _s3 ah_tmp = tmp.x;
  _s3 al_tmp = tmp.y;

  _r ah = shamir3_to_rss_GF_channel(ah_tmp, 0x13, 0);
  _r al = shamir3_to_rss_GF_channel(al_tmp, 0x13, 0);
  //printf("ah "); _print_debug_rss(ah, 1);
  //printf("al "); _print_debug_rss(al, 1);
  _free(ah_tmp);  _free(al_tmp);

  _s3 ah2 = vmul_rss(ah, ah); // X^4 + X + 1
  _s3 ahl = vmul_rss(ah, al);
  _s3 al2 = vmul_rss(al, al);
  _r ahpl = vadd_rss(ah, al);

  _s3 vtmp1 = smul(0xe, ah2); // ah2 is a 33ADD share
  _s3 vtmp2 = vadd(vtmp1, ahl);
  _s3 v = vadd(vtmp2, al2);
  //printf("v "); _print(v);
  _free(vtmp1); _free(vtmp2);
  _bits v_inv_tmp = tablelookup_3party(v, GF24_inv_table, 16, 0x13, SHARE_T_RSS);
  _sx v_inv = B2A_GF(v_inv_tmp, 0x13);
  //printf("v_inv "); _print_debug_rss(v_inv, 1);
  _free_bits(v_inv_tmp);
  _s3x aph_tmp = vmul_rss(ah, v_inv);
  _s3x apl_tmp = vmul_rss(ahpl, v_inv);
  _ ans = WOL_inverse(aph_tmp, apl_tmp, inverse);
  ans->irr_poly = 0x11b;

  _free(v); _free(v_inv);
  _free(aph_tmp); _free(apl_tmp);
  _free(ah2);  _free(ahl);  _free(al2);  _free(ahpl);
  _free(ah);  _free(al);
  return ans;
}
#endif

static void share_sub_bytes(_s3x state, int k)
{
  int i;
  _ ans;
  if (_num_parties > 3) {
    ans = GF28_inverse_3party(state, 0);
  } else {
    ans = GF28_inverse_2party(state, 0);
  }
  for (i=0; i<4*Nb*k; i++) {
    _setshare(state, i, ans, i);
  }
  state->irr_poly = 0x11b;
  _free(ans);
}

static void share_inv_sub_bytes(_x state, int k)
{
  int i;
  _ ans;
  if (_num_parties > 3) {
    ans = GF28_inverse_3party(state, 1);
  } else {
    ans = GF28_inverse_2party(state, 1);
  }
  for (i=0; i<4*Nb*k; i++) {
    _setshare(state, i, ans, i);
  }
  state->irr_poly = 0x11b;
  _free(ans);
}

static void share_shift_rows(_s3x state, int k)
{
  /*
     00 04 08 12 => 00 04 08 12
     01 05 09 13 => 05 09 13 01
     02 06 10 14 => 10 14 02 06
     03 07 11 15 => 15 03 07 11
   */
  _s3x tmp = share_const_type(3, 0, 256, state->type);
  for (int i=0; i<k*16; i+=16) {
    _setshare(tmp, 0, state, i+1);
    _setshare(state, i+1, state, i+5);
    _setshare(state, i+5, state, i+9);
    _setshare(state, i+9, state, i+13);
    _setshare(state, i+13, tmp, 0);
    _setshare(tmp, 0, state, i+2);
    _setshare(tmp, 1, state, i+6);
    _setshare(state, i+2, state, i+10);
    _setshare(state, i+6, state, i+14);
    _setshare(state, i+10, tmp, 0);
    _setshare(state, i+14, tmp, 1);
    _setshare(tmp, 0, state, i+3);
    _setshare(tmp, 1, state, i+7);
    _setshare(tmp, 2, state, i+11);
    _setshare(state, i+3, state, i+15);
    _setshare(state, i+7, tmp, 0);
    _setshare(state, i+11, tmp, 1);
    _setshare(state, i+15, tmp, 2);
  }
  _free(tmp);

}

static void share_inv_shift_rows(_s3x state, int k)
{
  /*
     00 04 08 12 => 00 04 08 12
     01 05 09 13 => 13 01 05 09
     02 06 10 14 => 10 14 02 06
     03 07 11 15 => 07 11 15 03
   */
  _x tmp = share_const_type(3, 0, 256, state->type);
  for (int i=0; i<k*16; i+=16) {
    _setshare(tmp, 0, state, i+13);
    _setshare(state, i+13, state, i+9);
    _setshare(state, i+9, state, i+5);
    _setshare(state, i+5, state, i+1);
    _setshare(state, i+1, tmp, 0);
    _setshare(tmp, 0, state, i+14);
    _setshare(tmp, 1, state, i+10);
    _setshare(state, i+14, state, i+6);
    _setshare(state, i+10, state, i+2);
    _setshare(state, i+6, tmp, 0);
    _setshare(state, i+2, tmp, 1);
    _setshare(tmp, 0, state, i+15);
    _setshare(tmp, 1, state, i+11);
    _setshare(tmp, 2, state, i+7);
    _setshare(state, i+15, state, i+3);
    _setshare(state, i+11, tmp, 0);
    _setshare(state, i+7, tmp, 1);
    _setshare(state, i+3, tmp, 2);
  }
  _free(tmp);
}

static void share_mix_columns(_s3x state, int k)
{
  int i;
  _s3x tmp = share_const_type(4, 0, 256, state->type);

  _s3x g02, g03;
  g02 = smul(2, state);
  g03 = smul(3, state);
  printf("g02: "); print_hex(g02); printf("\n");
  printf("g03: "); print_hex(g03); printf("\n");

  for (i=0; i<4*Nb*k; i+=4) {
    _setpublic(tmp, 0, 0);
    share_xor_(tmp, 0, g02,   i+0);
    share_xor_(tmp, 0, g03,   i+1);
    share_xor_(tmp, 0, state, i+2);
    share_xor_(tmp, 0, state, i+3);
    _setpublic(tmp, 1, 0);
    share_xor_(tmp, 1, state, i+0);
    share_xor_(tmp, 1, g02,   i+1);
    share_xor_(tmp, 1, g03,   i+2);
    share_xor_(tmp, 1, state, i+3);
    _setpublic(tmp, 2, 0);
    share_xor_(tmp, 2, state, i+0);
    share_xor_(tmp, 2, state, i+1);
    share_xor_(tmp, 2, g02,   i+2);
    share_xor_(tmp, 2, g03,   i+3);
    _setpublic(tmp, 3, 0);
    share_xor_(tmp, 3, g03,   i+0);
    share_xor_(tmp, 3, state, i+1);
    share_xor_(tmp, 3, state, i+2);
    share_xor_(tmp, 3, g02,   i+3);
    _setshares(state, i, i+4, tmp, 0);
    printf("Round 0: "); print_hex(state); printf("\n");
  }
  _free(g02);
  _free(g03);
  _free(tmp);

}

static void share_inv_mix_columns(_s3x state, int k)
{
  int i;
  _s3x tmp = share_const_type(4, 0, 256, state->type);

  _s3x g09, g0b, g0d, g0e;
  g09 = smul(0x09, state);
  g0b = smul(0x0b, state);
  g0d = smul(0x0d, state);
  g0e = smul(0x0e, state);

  for (i=0; i<4*Nb*k; i+=4) {
    _setpublic(tmp, 0, 0);
    share_xor_(tmp, 0, g0e, i+0);
    share_xor_(tmp, 0, g0b, i+1);
    share_xor_(tmp, 0, g0d, i+2);
    share_xor_(tmp, 0, g09, i+3);
    _setpublic(tmp, 1, 0);
    share_xor_(tmp, 1, g09, i+0);
    share_xor_(tmp, 1, g0e, i+1);
    share_xor_(tmp, 1, g0b, i+2);
    share_xor_(tmp, 1, g0d, i+3);
    _setpublic(tmp, 2, 0);
    share_xor_(tmp, 2, g0d, i+0);
    share_xor_(tmp, 2, g09, i+1);
    share_xor_(tmp, 2, g0e, i+2);
    share_xor_(tmp, 2, g0b, i+3);
    _setpublic(tmp, 3, 0);
    share_xor_(tmp, 3, g0b, i+0);
    share_xor_(tmp, 3, g0d, i+1);
    share_xor_(tmp, 3, g09, i+2);
    share_xor_(tmp, 3, g0e, i+3);
    _setshares(state, i, i+4, tmp, 0);
  }
  _free(g09);
  _free(g0b);
  _free(g0d);
  _free(g0e);
  _free(tmp);
}

void share_cipher(_s3x in, _s3x out, _s3x w, int k)
{
  int i;
  uint8_t Nr = key_round_table[aes_type].Nr;
  _s3x state = out;

  for (int i=0; i<4*Nb*k; i++) _setshare(state, i, in, i);

  share_add_round_key(state, w, 0, k);
  for (i=1; i<Nr; i++) {
    share_sub_bytes(state, k);
    share_shift_rows(state, k);
    share_mix_columns(state, k);
    share_add_round_key(state, w, 4*Nb*i, k);
  }
  share_sub_bytes(state, k);
  share_shift_rows(state, k);
  share_add_round_key(state, w, 4*Nb*Nr, k);

}

void share_inv_cipher(_s3x in, _s3x out, _s3x w, int k)
{
  int i;
  uint8_t Nr = key_round_table[aes_type].Nr;
  _s3x state = out;

  for (int i=0; i<4*Nb*k; i++) _setshare(state, i, in, i);
  share_add_round_key(state, w, 4*Nb*Nr, k);
  for (i=Nr-1; 1<=i; i--) {
    share_inv_shift_rows(state, k);
    share_inv_sub_bytes(state, k);
    share_add_round_key(state, w, 4*Nb*i, k);
    share_inv_mix_columns(state, k);
  }
  share_inv_shift_rows(state, k);
  share_inv_sub_bytes(state, k);
  share_add_round_key(state, w, 0, k);
}

static void share_cipher_and_inv_cipher(const uint8_t *key, _s3x in, _s3x out, int k)
{
  uint8_t w[4*Nb*(NR_MAX+1)];
  uint8_t Nk = key_round_table[aes_type].Nk;
  uint8_t Nr = key_round_table[aes_type].Nr;

  printf("Cipher Key = "); print_Nwords(key, Nk); printf("\n");

  key_expansion(key, w);
  printf("Round Keys = "); print_Nwords(w, Nb*(Nr+1)); printf("\n");

  share_t *wtmp;
  NEWA(wtmp, share_t, 4*Nb*(Nr+1));
  for (int i=0; i<4*Nb*(Nr+1); i++) wtmp[i] = (share_t)w[i];
  _ s_w;
  if (_num_parties > 3) {
    s_w = shamir3_xor_new(4*Nb*(Nr+1), 256, wtmp);
  } else {
    s_w = share_xor_new(4*Nb*(Nr+1), 256, wtmp);
  }
  free(wtmp);

  printf("Input      = "); print_hex(in); printf("\n");
  share_cipher(in, out, s_w, k);
  printf("Output     = "); print_hex(out); printf("\n");
  share_inv_cipher(out, in, s_w, k);
  printf("Input(Inv) = "); print_hex(in); printf("\n");
  printf("\n");

  _free(s_w);
}

int main(int argc, char* argv[])
{

  uint8_t key[NK_MAX*4], in[Nb*4];
  int i;

  for (i=0; i<NK_MAX*4; i++) {
    key[i] = i;
  }
  for (i=0; i<Nb*4; i++) {
    in[i] = i & 255;
  }

  printf("AES-128\n");
  aes_type = AES128;

  int k = 1;
  int n = k * 16;
  _party = -1;
  if (argc > 1) _party = atoi(argv[1]);

  if (_party == -1) {
    _num_parties = 3;
  }

  mpc_start();

  share_t *in3, *out3;
  NEWA(in3, share_t, n);
  NEWA(out3, share_t, n);
  for (i=0; i<n; i++) {
    in3[i] = i & 255;
    out3[i] = 0;
  }

  _ s_in, s_out;
  if (_num_parties > 3) {
    s_in = shamir3_xor_new(n, 256, in3);
    s_out = shamir3_xor_new(n, 256, out3);
  } else {
    s_in = share_xor_new(n, 256, in3);
    s_out = share_xor_new(n, 256, out3);
  }
  free(in3);
  free(out3);
  share_cipher_and_inv_cipher(key, s_in, s_out, k);

  _free(s_in);
  _free(s_out);

  mpc_end();

  return 0;
}
