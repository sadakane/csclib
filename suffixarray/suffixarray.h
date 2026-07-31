/*
  This is an implementation of the following paper:

  Isayama, K., Jimbo, K., Okamoto, N., Sadakane, K., Tozawa, K. 
  Oblivious Suffix Sorting: A Multi-Party Computation Scheme for Secure and Efficient Suffix Sorting. 
  In: Fischlin, M., Moonsamy, V. (eds) Applied Cryptography and Network Security. ACNS 2025. 
  Lecture Notes in Computer Science, vol 15825. Springer, Cham. 
  https://doi.org/10.1007/978-3-031-95761-1_10
*/


#ifndef _SUFFIXARRAY_H
 #define _SUFFIXARRAY_H

#include "share.h"
#include "shamir.h"
//#include "rmq.h"

#ifndef max
 #define max(x, y) ((x > y)?x:y)
#endif

/************************************************
def SuffixSort(T):
  n = len(T)
  I = Perm_ID(n+1)
  V = [0] * (n+1)
  for i in range(n):
    V[i] = ord(T[i])
  V[n] = 0
  X = [None] * (n+1)
  for i in range(n+1):
    X[i] = (V[i], i)
  X.sort()
  print('X', X)
  W = [v for (v, i) in X]
  I = [i for (v, i) in X]
  print('I', I)
  L = Grouping(W)
  print('L', L)
  V = Perm_ID(n+1)
  V2 = Propagate(L, V)
  print('V', V2)
  V = AppInvPerm(V2, I)
#  for i in range(n+1):
#    print(V[I[i]])
  h = 1
  while h < n:
    V1 = AppPerm(V, I)
    I2 = [(i+h) % (n+1) for i in I]
    V2 = AppPerm(V, I2)
    for i in range(n+1):
      X[i] = (V1[i], V2[i], I[i])
    X.sort()
    print('X', X)
    W = [v*(n+1)+v2 for (v, v2, i) in X]
    I = [i for (v, v2, i) in X]
    print('I', I)
    L = Grouping(W)
    print('L', L)
    V = Perm_ID(n+1)
    V2 = Propagate(L, V)
    print('V', V)
    V = AppInvPerm(V2, I)
    h = h*2
  return I
************************************************/
_ SuffixSort(_ T)
{
  int n = len(T);
  int d = blog(n+1-1)+1;
  _ allone = _const(n, 1, order(T));
  _ V = vadd(T, allone);
  _free(allone);
  _insert_tail_(V, 0);
  _bits Vb = _A2B(V, 1<<d); // Bit-decompose T
  _free(V);
  printf("Vb\n"); _print_bits(Vb);
  _ J = _radix_sort_bits(Vb); // permutation representing 1-character sort
  printf("J: ");share_print(J);
  _bits W = AppInvPerm_bits(Vb, J); // result of 1-character sort (pos -> lex)
  _free_bits(Vb);
  // printf("I a "); _print(I);
  printf("W\n"); _print_bits(W);
  _ L = Grouping_bits(W); // obtain group boundaries
  _free_bits(W);
  printf("L "); _print(L);
  _bits Va = _const_bits(n+1, 0, 1<<d, d);
  //_bits Va = _const_bits(n+1, 0, 2, d);
  for (int i=0; i<n+1; i++) {
    _setpublic_bits(Va, i, i);
  }
  _ L2 = B2A(L, 1<<d);
  for (int i=0; i<d; i++) {
    //_move_(Va->a[i], Propagate(L, Va->a[i]));
    _move_(Va->a[i], Propagate(L2, Va->a[i]));
  }
  // printf("I b "); _print(I);

  int h = 1;
  //_ s = sum(L);
  _ s = sum(L2);
  _free(L);
  _free(L2);
  printf("h = %d #groups %d\n", h, (int)pa_get(s->A,0));
  _free(s);
  while (h < n) {
    _bits V1 = AppPerm_bits(Va, J);
    printf("V1\n"); _print_bits(V1);
    _ J2 = _const(n+1, 0, 1<<d);
    for (int i=0; i<n+1; i++) {
      _setshare(J2, i, J, (i+h) % (n+1));
    }
    printf("J2 "); _print(J2);
    _bits V2 = AppPerm_bits(Va, J2);
    printf("V2\n"); _print_bits(V2);
    _free(J2);
    _bits Vb = _const_bits(n+1, 0, 1<<d, d*2);
    for (int k=0; k<d; k++) {
      _move_(Vb->a[k], _dup(V2->a[k]));
    }
    for (int i=0; i<d; i++) {
      _move_(Vb->a[d+i], _dup(V1->a[i]));
    }
    _free(J);
    J = _radix_sort_bits(Vb);
    _bits W = AppInvPerm_bits(Vb, J);
    printf("W\n"); _print_bits(W);
    _ L = Grouping_bits(W);
    //printf("L "); _print(L);
//    _ L2 = B2A(L, 1<<d);
//    _free(L);
    _free_bits(Vb);
    _free_bits(W);
    _free_bits(V1);
    _free_bits(V2);

    for (int i=0; i<n+1; i++) {
      _setpublic_bits(Va, i, i);
    }
    _ L2 = B2A(L, 1<<d);
    for (int i=0; i<d; i++) {
      //_move_(Va->a[i], Propagate(L, Va->a[i]));
      _move_(Va->a[i], Propagate(L2, Va->a[i]));
    }
    h = h*2;
    //_ s = sum(L);
    _ s = sum(L2);
    _free(L);
    _free(L2);
    share_t r = RANDOM0(s->q);
    //_addpublic(s, 0, r);
    _ s2 = _reconstruct(s);
    //_addpublic(s2, 0, -r);
//    share_t s3 = s2->A[0];
    share_t s3 = pa_get(s2->A,0);
    _free(s);
    _free(s2);
    printf("h = %d #groups %d\n", h, (int)s3);
    if (s3 > n) break;
  }
  _free_bits(Va);
//  printf("final I "); _print(I);
  _ I = InvPerm(J);
  _free(J);
  return I;
}

///////////////////////////////////////////////////////
// Order of bits is two.
///////////////////////////////////////////////////////
_ SuffixSort2(_ T)
{
  int n = len(T);
  int d = blog(n+1-1)+1;
  _ allone = _const(n, 1, order(T));
  _ V = vadd(T, allone);
  _free(allone);
  _insert_tail_(V, 0);
  _bits Vb = _A2B(V, 2); // Bit-decompose T
  _free(V);
  // printf("Vb\n"); _print_bits(Vb);
  _ J = _radix_sort_bits(Vb); // permutation representing 1-character sort
  _bits W = AppInvPerm_bits_online(Vb, J); // result of 1-character sort (pos -> lex)
  _free_bits(Vb);
  // printf("I a "); _print(I);
  // printf("W\n"); _print_bits(W);
  _ L = Grouping_bits(W); // obtain group boundaries
  _free_bits(W);
  //printf("L "); _print(L);
  //_bits Va = _const_bits(n+1, 0, 1<<d, d);
  _bits Va = _const_bits(n+1, 0, 2, d);
  for (int i=0; i<n+1; i++) {
    _setpublic_bits(Va, i, i);
  }
  _ L2 = B2A(L, 1<<d);
  for (int i=0; i<d; i++) {
    _move_(Va->a[i], Propagate(L2, Va->a[i])); // label for substrings of length 1 (lexicographically)
  }
  //printf("I b "); _print(I);
  _ s = sum(L2);
  _free(L);
  _free(L2);

  int h = 1;
  printf("h = %d #groups %d\n", h, (int)pa_get(s->A,0));
  _free(s);

  while (h < n) {
    _bits V1 = AppPerm_bits_online(Va, J); // V1 is the labels sorted by text position
    //printf("V1\n"); _print_bits(V1);
    _ J2 = _const(n+1, 0, 1<<d);
    for (int i=0; i<n+1; i++) {
      _setshare(J2, i, J, (i+h) % (n+1));
    }
    //printf("J2 "); _print(J2);
    _bits V2 = AppPerm_bits_online(Va, J2); // V1[j]:V2[j] is the key for sorting the suffix at position j
    //printf("V2\n"); _print_bits(V2);
    _free(J2);
    //_bits Vb = _const_bits(n+1, 0, 1<<d, d*2);
    _bits Vb = _const_bits(n+1, 0, 2, d*2);
    for (int k=0; k<d; k++) {
      _move_(Vb->a[k], _dup(V2->a[k]));
    }
    for (int i=0; i<d; i++) {
      _move_(Vb->a[d+i], _dup(V1->a[i]));
    }
    _free(J);
    J = _radix_sort_bits(Vb);
    _bits W = AppInvPerm_bits_online(Vb, J);
    //printf("W\n"); _print_bits(W);
    _ L = Grouping_bits(W);
    //printf("L "); _print(L);
    _free_bits(Vb);
    _free_bits(W);
    _free_bits(V1);
    _free_bits(V2);

    for (int i=0; i<n+1; i++) {
      _setpublic_bits(Va, i, i);
    }
    _ L2 = B2A(L, 1<<d);
    for (int i=0; i<d; i++) {
      _move_(Va->a[i], Propagate(L2, Va->a[i])); // label for substrings of length 2h (lexicographically)
    }
    h = h*2;
    _ s = sum(L2);
    _free(L);
    _free(L2);
    //share_t r = RANDOM(s->q);
    //_addpublic(s, 0, r);
    _ s2 = _reconstruct(s);
    //_addpublic(s2, 0, -r);
//    share_t s3 = s2->A[0];
    share_t s3 = pa_get(s2->A,0);
    _free(s);
    _free(s2);
    printf("h = %d #groups %d\n", h, (int)s3);
    if (s3 > n) break;
  }
  _free_bits(Va);
//  printf("final I "); _print(I);
  _ I = InvPerm(J);
  _free(J);
  return I;
}

///////////////////////////////////////////////////////
// Using precompted values
///////////////////////////////////////////////////////
_ Inv_SuffixSort3_channel(_ T, int channel)
{
  int n = len(T);
  int d = blog(n+1-1)+1;
  _ allone = _const(n, 1, order(T));
  _ V = vadd(T, allone);
  _free(allone);
  _insert_tail_(V, 0);
  _bits Vb = share_A2B_channel(V, 2, channel); // Bit-decompose T (using precomputed values if available)
  _free(V);
  //printf("Vb\n"); _print_bits(Vb);
  _ J = _radix_sort_bits(Vb); // permutation representing 1-character sort
//  _bits W = AppInvPerm_online_channel(Vb, J, channel); // result of 1-character sort (pos -> lex)
  _bits W = AppInvPerm_bits_channel(Vb, J, channel); // result of 1-character sort (pos -> lex)
  _free_bits(Vb);
  //printf("I a "); _print(I);
  //printf("W\n"); _print_bits(W);
  _ L = Grouping_bits(W); // obtain group boundaries
  _free_bits(W);
  //printf("L "); _print(L);
  //_bits Va = _const_bits(n+1, 0, 1<<d, d);
  _bits Va = _const_bits(n+1, 0, 2, d);
  for (int i=0; i<n+1; i++) {
    _setpublic_bits(Va, i, i);
  }
  _ L2 = B2A(L, 1<<d);
  for (int i=0; i<d; i++) {
    _move_(Va->a[i], Propagate(L2, Va->a[i])); // label for substrings of length 1 (lexicographically)
  }
  //printf("I b "); _print(I);
  _ s = sum(L2);
  _free(L);
  _free(L2);

  int h = 1;
  printf("h = %d #groups %d\n", h, (int)pa_get(s->A,0));
  _free(s);

  while (h < n) {
    _bits V1 = AppPerm_bits_channel(Va, J, channel); // V1 is the labels sorted by text position
    //printf("V1\n"); _print_bits(V1);
    _ J2 = _const(n+1, 0, 1<<d);
    for (int i=0; i<n+1; i++) {
      _setshare(J2, i, J, (i+h) % (n+1));
    }
    //printf("J2 "); _print(J2);
    _bits V2 = AppPerm_bits_channel(Va, J2, channel); // V1[j]:V2[j] is the key for sorting the suffix at position j
    //printf("V2\n"); _print_bits(V2);
    _free(J2);
    //_bits Vb = _const_bits(n+1, 0, 1<<d, d*2);
    _bits Vb = _const_bits(n+1, 0, 2, d*2);
    for (int i=0; i<d; i++) {
      _move_(Vb->a[i], _dup(V2->a[i]));
    }
    for (int i=0; i<d; i++) {
      _move_(Vb->a[d+i], _dup(V1->a[i]));
    }
    _free(J);
    J = _radix_sort_bits(Vb);
  //  _bits W = AppInvPerm_bits_online_channel(Vb, J, channel);
    _bits W = AppInvPerm_bits_channel(Vb, J, channel);
    //printf("W\n"); _print_bits(W);
    _ L = Grouping_bits(W);
    //printf("L "); _print(L);
    _free_bits(Vb);
    _free_bits(W);
    _free_bits(V1);
    _free_bits(V2);

    for (int i=0; i<n+1; i++) {
      _setpublic_bits(Va, i, i);
    }
    _ L2 = B2A(L, 1<<d);
    for (int i=0; i<d; i++) {
      _move_(Va->a[i], Propagate(L2, Va->a[i])); // label for substrings of length 2h (lexicographically)
    }
    h = h*2;
    _ s = sum(L2);
    _free(L);
    _free(L2);
    //share_t r = RANDOM(s->q);
    //_addpublic(s, 0, r);
    _ s2 = _reconstruct(s);
    //_addpublic(s2, 0, -r);
//    share_t s3 = s2->A[0];
    share_t s3 = pa_get(s2->A,0);
    _free(s);
    _free(s2);
    printf("h = %d #groups %d\n", h, (int)s3);
    if (s3 > n) break;
  }
  _free_bits(Va);
//  printf("final I "); _print(I);
//  _ I = InvPerm(J);
//  _free(J);
//  return I;
  return J;
}
#define Inv_SuffixSort3(T) Inv_SuffixSort3_channel(T, 0)

_ SuffixSort3_channel(_ T, int channel)
{
  _ ISA = Inv_SuffixSort3_channel(T, channel);
  _ SA = InvPerm(ISA);
  _free(ISA);
  return SA;
}
#define SuffixSort3(T) SuffixSort3_channel(T, 0)


////////////////////////////////////////////////////
// 3 party
////////////////////////////////////////////////////
_ SuffixSort4_full(_ T, int full)
{
  int n = len(T);
  int d = blog(n+1-1)+1;
  _ allone = _const(n, 1, order(T));
  _ V = vadd(T, allone);
  _free(allone);
  _insert_tail_(V, 0);
  _pair tmp;
  //if (_num_parties == 4) {
  //  tmp = share_radix_sort_shamir(3, V);
  //} else {
  //  tmp = share_radix_sort(V);
  //}
  tmp = share_radix_sort2(V, 0);
  _ Jinv = tmp.y;
  _ J = InvPerm(Jinv);
  _free(Jinv);
  _ V2 = tmp.x;
  _free(V);
  _ L = Grouping(V2);
  _free(V2);
  _ Va = _const(n+1, 0, 1<<(1*d));
  for (int i=0; i<n+1; i++) {
    _setpublic(Va, i, i);
  }
  //_move_(Va, Propagate2(L, Va));
  _move_(Va, Propagate3(L, Va));

  int h = 1;
#if 0
  //_ s = sum(L);
  if (_party <= 2) {
    _ L2 = B2A(L, 1<<(1*d+1));
    _ s = sum(L2);
    _free(L2);
    printf("h = %d #groups %d\n", h, (int)pa_get(s->A,0));
    _free(s);
  }
#endif
////  _free(L);
  while (h < n) {
    _ V1 = AppPerm(Va, J);
    _ J2 = _const(n+1, 0, 1<<d);
    for (int i=0; i<n+1; i++) {
      _setshare(J2, i, J, (i+h) % (n+1));
    }
    _ V2 = AppPerm(Va, J2);
    //_pair tmp2 = share_radix_sort(V2);
    _pair tmp2;
    //if (_num_parties == 4) {
    //  tmp2 = share_radix_sort_shamir(3, V2);
    //} else {
    //  tmp2 = share_radix_sort(V2);
    //}
    tmp2 = share_radix_sort2(V2, 0);
    _ key = AppPerm(V1, tmp2.y);
    //_pair tmp1 = share_radix_sort(key);
    _pair tmp1;
    //if (_num_parties == 4) {
    //  tmp1 = share_radix_sort_shamir(3, key);
    //} else {
    //  tmp1 = share_radix_sort(key);
    //}
    tmp1 = share_radix_sort2(key, 0);
    _ Jtmp = AppPerm(tmp2.y, tmp1.y);
    _ V2_sorted = AppPerm(V2, Jtmp);
    ////_ V1_sorted = AppPerm(V1, Jtmp);
    _ L2tmp = Grouping(V2_sorted);
    ////_ L1tmp = Grouping(V1_sorted);
    ////_ L = OR(L1tmp, L2tmp);
    _ Ltmp = OR(L, L2tmp);
    _free(L);
    L = _move(Ltmp);
    ////_free(L);
    _free(key);
    ////_free(L1tmp); 
    _free(L2tmp);
    ////_free(V1_sorted); 
    _free(V2_sorted);
    _free(J); 
    _free(tmp1.x); _free(tmp1.y);
    _free(tmp2.x); _free(tmp2.y);
    J = InvPerm(Jtmp);
    _free(Jtmp);
    _free(V1); _free(V2);
    _free(J2);

    for (int i=0; i<n+1; i++) {
      _setpublic(Va, i, i);
    }
    _move_(Va, Propagate2(L, Va));
    //_move_(Va, Propagate3(L, Va));
    h = h*2;
    if (full == 0) {
      int si = 0;
      _ s2;
      if (_party <= 2) {
        _ L2 = B2A(L, 1<<(d+1));
        _ s = sum(L2);
        _free(L2);
        s2 = _reconstruct(s);
        _free(s);
        if (_num_parties == 4 && _party == 1) mpc_send_share(TO_PARTY3, s2);
      } else {
        s2 = _const_shamir(1, 0, 1<<(d+1));
        mpc_recv_share(FROM_PARTY1, s2);
      }
      share_t s3 = share_getraw(s2, 0);
      printf("h = %d #groups %d\n", h, (int)s3);
      fflush(stdout);
      si = (s3 == n+1);
      _free(s2);
      //_free(L);
      if (si) break;
    }
  }
  _free(L);
  _ I = InvPerm(J);
  _free(J);
  _free(Va);
  return I;
}
#define SuffixSort4(T) SuffixSort4_full(T, 0)


_pair SuffixSort4_LCP(_ T)
{
  int n = len(T);
  int d = blog(n+1-1)+1;
  _ allone = _const(n, 1, order(T));
  _ V = vadd(T, allone);
  _free(allone);
  _insert_tail_(V, 0);
  _pair tmp;
  //if (_num_parties == 4) {
  //  tmp = share_radix_sort_shamir(3, V);
  //} else {
  //  tmp = share_radix_sort(V);
  //}
  tmp = share_radix_sort2(V, 0);
  _ Jinv = tmp.y;
  _ J = InvPerm(Jinv);
  _free(Jinv);
  _ V2 = tmp.x;
  _free(V);
  _ L = Grouping(V2);
  _free(V2);
  _ Va = _const(n+1, 0, 1<<(1*d));
  for (int i=0; i<n+1; i++) {
    _setpublic(Va, i, i);
  }
  _move_(Va, Propagate2(L, Va));

  _free(L);

  _ *VV;
  NEWA(VV, _, d+1);
  for (int i=0; i<d+1; i++) VV[i] = NULL;
  VV[0] = Va; // label for substrings of length 1 (lexicographically)

  int h = 1;
  int logh = 0;
  while (h < n) {
    _ V1 = AppPerm(Va, J);
    _ J2 = _const(n+1, 0, 1<<d);
    for (int i=0; i<n+1; i++) {
      _setshare(J2, i, J, (i+h) % (n+1));
    }
    _ V2 = AppPerm(Va, J2);
    //_pair tmp2 = share_radix_sort(V2);
    _pair tmp2;
    //if (_num_parties == 4) {
    //  tmp2 = share_radix_sort_shamir(3, V2);
    //} else {
    //  tmp2 = share_radix_sort(V2);
    //}
    tmp2 = share_radix_sort2(V2, 0);
    _ key = AppPerm(V1, tmp2.y);
    //_pair tmp1 = share_radix_sort(key);
    _pair tmp1;
    //if (_num_parties == 4) {
    //  tmp1 = share_radix_sort_shamir(3, key);
    //} else {
    //  tmp1 = share_radix_sort(key);
    //}
    tmp1 = share_radix_sort2(key, 0);
    _ Jtmp = AppPerm(tmp2.y, tmp1.y);
    _ V2_sorted = AppPerm(V2, Jtmp);
    _ V1_sorted = AppPerm(V1, Jtmp);
    _ L2tmp = Grouping(V2_sorted);
    _ L1tmp = Grouping(V1_sorted);
    _ L = OR(L1tmp, L2tmp);
    _free(key);
    _free(L1tmp); _free(L2tmp);
    _free(V1_sorted); _free(V2_sorted);
    _free(J); 
    _free(tmp1.x); _free(tmp1.y);
    _free(tmp2.x); _free(tmp2.y);
    J = InvPerm(Jtmp);
    _free(Jtmp);
    _free(V1); _free(V2);
    _free(J2);

    Va = _const(n+1, 0, 1<<(1*d));
    for (int i=0; i<n+1; i++) {
      _setpublic(Va, i, i);
    }
    _move_(Va, Propagate2(L, Va));
    h = h*2;
    logh += 1;
    VV[logh] = Va; // label for substrings of length h (lexicographically)
    int si = 0;
#if 1
    //_ s = sum(L);
    _ s2;
    if (_party <= 2) {
      _ L2 = B2A(L, 1<<(d+1));
      _ s = sum(L2);
      _free(L2);
      s2 = _reconstruct(s);
      _free(s);
      //share_t s3 = pa_get(s2->A,0);
      //if (s3 > n) break;
      if (_num_parties == 4 && _party == 1) mpc_send_share(TO_PARTY3, s2);
    } else {
      s2 = _const_shamir(1, 0, 1<<(d+1));
      mpc_recv_share(FROM_PARTY1, s2);
    }
    share_t s3 = share_getraw(s2, 0);
    printf("h = %d #groups %d\n", h, (int)s3);
    si = (s3 == n+1);
    _free(s2);
#endif
    _free(L);
    if (si) break;
  }
  _ I = InvPerm(J);

  _ LCP = _const(n, 0, 1<<d);

  for (int d=logh-1; d>=0; d--) {
    _ c;

    _ V = AppPerm(VV[d], J);
    //printf("V\n"); _print(V);

    _ I1 = _slice(I, 0, n);
    //printf("I1\n"); _print(I1);
    _ I2 = _slice(I, 1, n+1);
    //printf("I2\n"); _print(I2);
    for (int i=0; i<n; i++) {
      _addshare(I1, i, LCP, i); // I1[i] := I[i]   + LCP[i]
      _addshare(I2, i, LCP, i); // I2[i] := I[i+1] + LCP[i]
    }
    _ V1 = BatchAccess(V, I1); // V1[i] := V[I1[i]] = V[I[i]  +LCP[i]]
    //printf("V1\n"); _print(V1);
    _ V2 = BatchAccess(V, I2); // V2[i] := V[I2[i]] = V[I[i+1]+LCP[i]]
    //printf("V2\n"); _print(V2);

    c = Equality(V1, V2);
    B2A_(c, order(LCP));
    //printf("c "); _print(c);
    smul_(1<<d, c);
  //  printf("c "); _print(c);
    vadd_(LCP, c);
    //printf("LCP "); _print(LCP);
    _free(V);
    _free(c);
    _free(I1);
    _free(I2);
    _free(V1);
    _free(V2);
  }

  _free(J);
  _pair ans = {I, LCP};

  for (int i=0; i<d+1; i++) {
    if (VV[i] != NULL) {
      _free(VV[i]);
    }
  }
  free(VV);

  return ans;
}


_pair SuffixSort_LCP(_ T)
{
  int n = len(T);
  int d = blog(n+1-1)+1;
  _ allone = _const(n, 1, order(T));
  _ V = vadd(T, allone);
  _free(allone);
  _insert_tail_(V, 0);
  _bits Vb = _A2B(V, 2); // Bit-decompose T
  _free(V);
  //printf("Vb\n"); _print_bits(Vb);
  _ J = _radix_sort_bits(Vb); // permutation representing 1-character sort
  _bits W = AppInvPerm_bits(Vb, J); // result of 1-character sort (pos -> lex)
  _free_bits(Vb);
  //printf("I a "); _print(I);
  //printf("W\n"); _print_bits(W);
  _ L = Grouping_bits(W); // obtain group boundaries
  //printf("L "); _print(L);
  _free_bits(W);
  //_bits Va = _const_bits(n+1, 0, 1<<d, d);
  _bits Va = _const_bits(n+1, 0, 2, d);
  for (int i=0; i<n+1; i++) {
    _setpublic_bits(Va, i, i);
  }
  _ L2 = B2A(L, 1<<d);
  _free(L);
  for (int i=0; i<d; i++) {
    _move_(Va->a[i], Propagate(L2, Va->a[i]));
  }
  //printf("I b "); _print(I);
  _ s = sum(L2);
  _free(L2);

  _bits *VV;
  NEWA(VV, _bits, d+1);
  for (int i=0; i<d+1; i++) VV[i] = NULL;
  VV[0] = Va; // label for substrings of length 1 (lexicographically)



  int h = 1;
  int logh = 0;
  printf("h = %d #groups %d\n", h, (int)pa_get(s->A,0));
  _free(s);
  while (h < n) {
    _bits V1 = AppPerm_bits(Va, J);
    //printf("V1\n"); _print_bits(V1);
    _ J2 = _const(n+1, 0, 1<<d);
    for (int i=0; i<n+1; i++) {
      _setshare(J2, i, J, (i+h) % (n+1));
    }
    //printf("J2 "); _print(J2);
    _bits V2 = AppPerm_bits(Va, J2);
    //printf("V2\n"); _print_bits(V2);
    _free(J2);
    //_bits Vb = _const_bits(n+1, 0, 1<<d, d*2);
    _bits Vb = _const_bits(n+1, 0, 2, d*2);
    for (int k=0; k<d; k++) {
      _move_(Vb->a[k], _dup(V2->a[k]));
    }
    for (int i=0; i<d; i++) {
      _move_(Vb->a[d+i], _dup(V1->a[i]));
    }
    _free(J);
    J = _radix_sort_bits(Vb);
    _bits W = AppInvPerm_bits(Vb, J);
    printf("W\n"); _print_bits(W);
    _ L = Grouping_bits(W);
    printf("L "); _print(L);
    _ L2 = B2A(L, 1<<d);
    _free(L);
    _free_bits(Vb);
    _free_bits(W);
    _free_bits(V1);
    _free_bits(V2);

    //Va = _const_bits(n+1, 0, 1<<d, d);
    Va = _const_bits(n+1, 0, 2, d);
    for (int i=0; i<n+1; i++) {
      _setpublic_bits(Va, i, i);
    }
    //_ L2 = B2A(L, 1<<d);
    for (int i=0; i<d; i++) {
      _move_(Va->a[i], Propagate(L2, Va->a[i]));
    }
    h = h*2;
    logh += 1;
    VV[logh] = Va; // label for substrings of length h (lexicographically)
    _ s = sum(L2);
    _free(L2);
    //_free(L2);
    //share_t r = RANDOM(s->q);
    //_addpublic(s, 0, r);
    _ s2 = _reconstruct(s);
    //_addpublic(s2, 0, -r);
//    share_t s3 = s2->A[0];
    share_t s3 = pa_get(s2->A,0);
    _free(s);
    _free(s2);
    printf("h = %d #groups %d\n", h, (int)s3);
    if (s3 > n) break;
  }
  //_free_bits(Va);
//  printf("final I "); _print(I);

  _ I = InvPerm(J);
  printf("I\n"); _print(I);

  _ LCP = _const(n, 0, 1<<d);

  for (int d=logh-1; d>=0; d--) {
    _ c;

    _bits V = AppPerm_bits(VV[d], J);
    printf("V\n"); _print_bits(V);

    _ I1 = _slice(I, 0, n);
    printf("I1\n"); _print(I1);
    _ I2 = _slice(I, 1, n+1);
    printf("I2\n"); _print(I2);
    for (int i=0; i<n; i++) {
      _addshare(I1, i, LCP, i); // I1[i] := I[i]   + LCP[i]
      _addshare(I2, i, LCP, i); // I2[i] := I[i+1] + LCP[i]
    }
    _bits V1 = BatchAccess_bits(V, I1); // V1[i] := V[I1[i]] = V[I[i]  +LCP[i]]
    printf("V1\n"); _print_bits(V1);
    _bits V2 = BatchAccess_bits(V, I2); // V2[i] := V[I2[i]] = V[I[i+1]+LCP[i]]
    printf("V2\n"); _print_bits(V2);

    c = Equality_bits(V1, V2);
    printf("c "); _print(c);
    B2A_(c, LCP->q);
  //  printf("c "); _print(c);
    smul_(1<<d, c);
  //  printf("c "); _print(c);
    vadd_(LCP, c);
    printf("LCP "); _print(LCP);
    _free_bits(V);
    _free(c);
    _free(I1);
    _free(I2);
    _free_bits(V1);
    _free_bits(V2);
  }

  _free(J);
  _pair ans = {I, LCP};

  for (int i=0; i<d+1; i++) {
    if (VV[i] != NULL) {
      _free_bits(VV[i]);
    }
  }
  free(VV);

  return ans;
}

_ merge(_ X, _ Y, _ rank_Y)
{
  if (len(Y) != len(rank_Y)) {
    printf("merge\n");
    _print(Y);
    _print(rank_Y);
    exit(1);
  }
  _ U = Unary(rank_Y, len(X)+1);
  _slice_(U, 1, 0);
  _ pi = StableSort(U);

  _ Z = _concat(X, Y);
//  _ ans = AppPerm(Z, pi);
  _ ans = AppPerm_channel(Z, pi, 0);
  _free(U);
  _free(pi);
  _free(Z);
  return ans;
}

_ getrank(_ sigma)
{
  int n2 = len(sigma) / 2;
  share_t q = order(sigma);
  _ B = _const(n2*2, 0, q);
  for (int i=0; i<n2; i++) {
    _setpublic(B, n2+i, 1);
  }
//  _ Ba = AppInvPerm(B, sigma);
  _ Ba = AppInvPerm_channel(B, sigma, 0);
  _ Bb = PrefixSum(Ba);
//  _ Bc = AppPerm(Bb, sigma);
  _ Bc = AppPerm_channel(Bb, sigma, 0);
  _ r = _slice(Bc, 0, n2);
  _free(B);
  _free(Ba);
  _free(Bb);
  _free(Bc);
  return r;
}

_ getrank2(_ sigma, share_t q)
{
  int n2 = len(sigma) / 2;
  _ B = _const(n2*2, 0, q);
  for (int i=0; i<n2; i++) {
    _setpublic(B, n2+i, 1);
  }
//  _ Ba = AppInvPerm(B, sigma);
  _ Ba = AppInvPerm_channel(B, sigma, 0);
  _ Bb = PrefixSum(Ba);
//  _ Bc = AppPerm(Bb, sigma);
  _ Bc = AppPerm_channel(Bb, sigma, 0);
  _ r = _slice(Bc, 0, n2);
  _free(B);
  _free(Ba);
  _free(Bb);
  _free(Bc);
  return r;
}



_ SuffixSort_DC3(_ T)
{
  int n = len(T);
  printf("SuffixSort_DC3 n = %d order = %d\n", n, order(T));

  if (n < 10) {
    return SuffixSort3(T);
  }

  // T12 を作る
  int n2 = (n+3)/3;
  share_t o = order(T) + 3;
  int k = max(o, n+1);
  int d = blog(k-1)+1;
  _ T12pos = _const(n2*2, 0, 1<<d);

  _ T12[3];

  int d2 = max(d, blog(o+1)+1);
  _ Tp = _extend(T, 1<<d2); // needs improved implementation
  _ Tp2 = _const(5, 0, 1<<d2);
  for (int i=0; i<n; i++) {
    _addpublic(Tp, i, 3);
  }
  _concat_(Tp, Tp2);
  _free(Tp2);
  _setpublic(Tp, n, 2);
  _setpublic(Tp, n+1, 1);

  for (int j=0; j<3; j++) {
    T12[j] = _const(n2*2, 0, 1<<d2);
  }
  for (int i=0; i<n2; i++) {
    for (int j=0; j<3; j++) {
      _setshare(T12[j],i, Tp, i*3+1+2-j);
      _setshare(T12[j],n2+i, Tp, i*3+2+2-j);
    }
    _setpublic(T12pos, i, i*3+1);
    _setpublic(T12pos, n2+i, i*3+2);
  }

  _bits T12a[3];
  for (int j=0; j<3; j++) {
    T12a[j] = share_A2B_channel(T12[j], 1<<d2, 0); // not efficient - Tp is already bit-decomposed 
  }
  _bits T12b0 = _vconcat_bits(T12a[0], T12a[1]);
  _bits T12b = _vconcat_bits(T12b0, T12a[2]);
  for (int j=0; j<3; j++) {
    _free(T12[j]);
    _free_bits(T12a[j]);
  }
  _free_bits(T12b0);


  _ J = _radix_sort_bits(T12b); // permutation representing 1-character sort

  _bits Wb = AppInvPerm_bits(T12b, J); // result of 1-character sort (pos -> lex)
//  printf("Wb "); _print_bits(Wb);
  _b L = Grouping_bits(Wb); // obtain group boundaries
  _free_bits(Wb);
  _free_bits(T12b);

//  printf("check L");
//  _check(L); _print(L);
  //exit(1);

  int d3 = blog(len(L)+2-1)+1;
#if 0
  _ V = Grouping_name(L, 1<<d3);
#else
//  _ Ltmp = _extend(L, 1<<d3);
    _ Ltmp = B2A(L, 1<<d3);
  _ V = rank1(Ltmp);
  _free(Ltmp);
#endif
  _free(L);

//  _ T12c = AppPerm(V, J);
  _ T12c = AppPerm_channel(V, J, 0);
  _slice_(T12c, 0, -1);
  _free(J);
  _free(V);

  _ SA12a = SuffixSort_DC3(T12c);
  _free(T12c);
//  printf("SA12 ");
//  _print(SA12a);
  printf("SuffixSort_DC3 n = %d order = %d\n", n, order(T));
  //printf("SA12a "); _print(SA12a);

#if 1
  _ SA12 = _extend(SA12a, 1<<d);
  _free(SA12a);
#else
  _ SA12 = _dup(SA12a);
#endif

  _ SA12inv = InvPerm(SA12);
//  printf("SA12inv ");
//  _print(SA12inv);
//  _ SA12tmp2 = AppPerm(T12pos, SA12);
  _ SA12tmp2 = AppPerm_channel(T12pos, SA12, 0);
  _free(SA12);
  _free(T12pos);

  int n02 = n2*2;

  _ T01 = _const(n02, 0, order(Tp));
//  _ R01 = _const(n02, 0, order(SA12inv));
  _ SA01 = _const(n02, 0, order(SA12inv));

  for (int i=0; i<n2; i++) {
    _setshare(T01, i, Tp, i*3+0);
    _setpublic(SA01, i, i*3+0);
  //  _setshare(R01, i, SA12inv, i);
    _setshare(T01, n2+i, Tp, i*3+1);
    _setpublic(SA01, n2+i, i*3+1);
  //  _setshare(R01, n2+i, SA12inv, n2+i);
  }
//  printf("R01 ");
//  _print(R01);

#if 0
  _bits T01b = _A2B(T01, 1<<d2);
  _bits R01b = _A2B(SA12inv, 1<<d2);
  _bits K01b = _vconcat_bits(R01b, T01b);
  _free_bits(R01b);
  _free(T01);
//  printf("K01b ");
//  _print_bits(K01b);
//  _free_bits(R01b);
//  _free_bits(T01b);
  _ sigma = _radix_sort_bits(K01b);
//  printf("sigma ");
//  _print(sigma);
  _free_bits(K01b);
  _free_bits(T01b);
#else
//  _ T01x = AppInvPerm(T01, SA12inv);
  _ T01x = AppInvPerm_channel(T01, SA12inv, 0);
  _free(T01);

  _bits T01xb = _A2B(T01x, 1<<d2);
  _ sigma2inv = _radix_sort_bits(T01xb);

//  _ sigma = AppPerm(sigma2inv, SA12inv);
  _ sigma = AppPerm_channel(sigma2inv, SA12inv, 0);
  _free(sigma2inv);
  _free(T01x);
  _free_bits(T01xb);
#endif

  _ B01 = _const(n2*2, 0, 1<<d);
  for (int i=0; i<n2; i++) {
    _setpublic(B01, n2+i, 1);
  }
//  _ B01a = AppInvPerm(B01, sigma);
  _ B01a = AppInvPerm_channel(B01, sigma, 0);
  _ B01b = PrefixSum(B01a);
//  _ B01c = AppPerm(B01b, sigma);
  _ B01c = AppPerm_channel(B01b, sigma, 0);
  _ r1 = _slice(B01c, 0, n2);
//  _ SA01a = AppInvPerm(SA01, sigma);
  _ SA01a = AppInvPerm_channel(SA01, sigma, 0);
  _ rho2 = StableSort(B01a);
//  _ SA01b = AppInvPerm(SA01a, rho2);
  _ SA01b = AppInvPerm_channel(SA01a, rho2, 0);
  _slice_(SA01b, 0, n2);
  _free(SA01);
  _free(rho2);
  _free(SA01a);
  _free(B01a);
  _free(B01b);
  _free(B01c);
  _free(B01);
  _free(sigma);


  _ T021 = _const(n02, 0, order(Tp));
  _ T022 = _const(n02, 0, order(Tp));
  _ R02 = _const(n02, 0, order(SA12inv));

  for (int i=0; i<n2; i++) {
    _setshare(T021, i, Tp, i*3+0);
    _setshare(T022, i, Tp, i*3+1);
    _setshare(R02, i, SA12inv, n2+i);
    _setshare(T021, n2+i, Tp, i*3+2);
    _setshare(T022, n2+i, Tp, i*3+3);
    if (i+1 < n2) _setshare(R02, n2+i, SA12inv, i+1);
  }
  _setshare(R02, n2*2-1, SA12inv, 0);
  _free(Tp);
  _free(SA12inv);

  _bits T021b = _A2B(T021, 1<<d2);
  _bits T022b = _A2B(T022, 1<<d2);
//  _bits R02b = _A2B(R02, 1<<d2);
  _free(T021);
  _free(T022);
//  _free(R02);
  _bits K02c = _vconcat_bits(T022b, T021b);

//  _bits T02x = AppInvPerm_bits(K02c, R02);
  _bits T02x = AppInvPerm_bits(K02c, R02);

  _free_bits(T022b);
  _free_bits(T021b);

  _ sigma4 = _radix_sort_bits(T02x);

//  printf("sigma2 ");
//  _print(sigma2);
//  _ sigma2 = AppPerm(sigma4, R02);
  _ sigma2 = AppPerm_channel(sigma4, R02, 0);
  _free_bits(K02c);
  _free_bits(T02x);
  _free(sigma4);
  _free(R02);

#if 0
  _ B02 = _const(n2*2, 0, 1<<d);
  for (int i=0; i<n2; i++) {
    _setpublic(B02, n2+i, 1);
  }
  _ B02a = AppInvPerm(B02, sigma2);
//  printf("B02a ");
//  _print(B02a);
  _ B02b = PrefixSum(B02a);
//  printf("B02b ");
//  _print(B02b);
  _ B02c = AppPerm(B02b, sigma2);
//  printf("B02c ");
//  _print(B02c);
  _ r2 = _slice(B02c, 0, n2);
//  printf("B02c ");
//  _print(B02c);
  _free(B02a);
  _free(B02b);
  _free(B02);
  _free(B02c);
#else
//  _ r2 = getrank(sigma2, order(r1));
  _ r2 = getrank(sigma2);
#endif
  _free(sigma2);


//  printf("B01c ");
//  _print(B01c);
//  printf("B02c ");
//  _print(B02c);
  _ R = vadd(r1, r2);
  //printf("R "); _print(R);
  _free(r1);
  _free(r2);
//  printf("R ");
//  _print(R);
#if 0
  _ U = Unary(R, n2*2+1);
  _slice_(U, 1, 0);
  _ pi = StableSort(U);
//  printf("pi ");
//  _print(pi);
  _free(U);

  _ SAtmp = _concat(SA12tmp2, SA01b);
  _ SA = AppPerm(SAtmp, pi);
  _free(SAtmp);
  _free(pi);
#else
  _ SA = merge(SA12tmp2, SA01b, R);
#endif
  _free(SA12tmp2);
  _free(SA01b);
  _free(R);

  if (len(SA) > n+1) {
    _slice_(SA, len(SA)-(n+1), 0);
  }

  return SA;
}



_ SuffixSort_DC3_new(_ T)
{
  int n = len(T);
  printf("SuffixSort_DC3_new n = %d order = %d\n", n, order(T));

  if (n < 10) {
    return SuffixSort4(T);
  }

  // T12 を作る
  int n2 = (n+3)/3;
#if 1
  share_t o = order(T) + 3;
#else
  share_t o = order(T);
#endif
  int k = max(o, n+1);
  int d = blog(k-1)+1;

  _ T12[3];

  int d2 = max(d, blog(o+1)+1);
//  int d2 = d;

#if 1
  _ T12pos = _const(n2*2, 0, 1<<d);
#else
  _ T12pos = _const(n2*2, 0, 1<<d2);
#endif

  _ Tp;
  if (order(T) < (1<<d2)) {
    printf("extend T %d -> %d\n", order(T), 1<<d2);
    Tp = _extend(T, 1<<d2); // ここを改良したい
  } else {
    Tp = _shrink(T, 1<<d2);
  }
  _ Tp2 = _const(5, 0, 1<<d2);
  for (int i=0; i<n; i++) {
    _addpublic(Tp, i, 3);
  }
  _concat_(Tp, Tp2);
  _free(Tp2);
  _setpublic(Tp, n, 2);
  _setpublic(Tp, n+1, 1);

  pa_iter t12tmp_iter;
  pa_iter t12_iter[3];


  for (int j=0; j<3; j++) {
    T12[j] = _const(n2*2, 0, 1<<d2);
  }
  for (int i=0; i<n2; i++) {
    //for (int j=0; j<3; j++) {
    //  _setshare(T12[j],i, Tp, i*3+1+2-j);
    //  _setshare(T12[j],n2+i, Tp, i*3+2+2-j);
    //}
    _setpublic(T12pos, i, i*3+1);
    _setpublic(T12pos, n2+i, i*3+2);
  }
#if 1
  for (int j=0; j<3; j++) {
    t12_iter[j] = pa_iter_new(T12[j]->A);
  }
  t12tmp_iter = pa_iter_pos_new(Tp->A, 1);
  for (int i=0; i<n2; i++) {
    //for (int j=0; j<3; j++) {
    //  pa_iter_set(t12_iter[j], share_getraw(Tp, i*3+1+2-j));
    //}
    pa_iter_set(t12_iter[2], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[1], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[0], pa_iter_get(t12tmp_iter));
  }
  pa_iter_free(t12tmp_iter);
  t12tmp_iter = pa_iter_pos_new(Tp->A, 2);
  for (int i=0; i<n2; i++) {
    //for (int j=0; j<3; j++) {
    //  pa_iter_set(t12_iter[j], share_getraw(Tp, i*3+2+2-j));
    //}
    pa_iter_set(t12_iter[2], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[1], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[0], pa_iter_get(t12tmp_iter));
  }
  for (int j=0; j<3; j++) {
    pa_iter_flush(t12_iter[j]);
  }
  pa_iter_free(t12tmp_iter);
#endif  

#if 0
  _bits T12a[3];
  for (int j=0; j<3; j++) {
    T12a[j] = share_A2B_channel(T12[j], 1<<d2, 0); // Tp を一度ビット分解してるのに再度やるのは無駄
  }
  _bits T12b0 = _vconcat_bits(T12a[0], T12a[1]);
  _bits T12b = _vconcat_bits(T12b0, T12a[2]);
  for (int j=0; j<3; j++) {
    _free(T12[j]);
    _free_bits(T12a[j]);
  }
  _free_bits(T12b0);


  _ J = _radix_sort_bits(T12b); //  1文字でのソートを表す置換

//  _bits Wb = AppInvPerm_bits(T12b, J); // 1文字のソート結果 (pos -> lex)
  _bits Wb = AppInvPerm_bits(T12b, J); // 1文字のソート結果 (pos -> lex)
//  printf("Wb "); _print_bits(Wb);
  _b L = Grouping_bits(Wb); // グループの境界を求める
  _free_bits(Wb);
  _free_bits(T12b);

#else

  _ J;
  _b L;
  {
#if 0
    _pair tmp3 = share_radix_sort(T12[0]);
    _ key2 = AppPerm(T12[1], tmp3.y);
    _pair tmp2 = share_radix_sort(key2);
    _ pi_tmp2 = AppPerm(tmp3.y, tmp2.y);
    _ key1 = AppPerm(T12[2], pi_tmp2);
    _pair tmp1 = share_radix_sort(key1);
    _ pi_tmp1 = AppPerm(pi_tmp2, tmp1.y);
    J = InvPerm(pi_tmp1);
#else
    //_pair tmp3 = share_radix_sort(T12[0]);
    _pair tmp3 = share_radix_sort2(T12[0], 0);
    _pair tmp2 = share_radix_sort_cont(tmp3.y, T12[1]);
    _pair tmp1 = share_radix_sort_cont(tmp2.y, T12[2]);
    J = InvPerm(tmp1.y);
#endif

#if 0
    _ W0 = AppInvPerm(T12[0], J);
    _ W1 = AppInvPerm(T12[1], J);
    _ W2 = AppInvPerm(T12[2], J);
#else
    _ T12tmp = _const(n2*2*3, 0, 1<<d2);
    t12tmp_iter = pa_iter_new(T12tmp->A);
    for (int j=0; j<3; j++) {
      t12_iter[j] = pa_iter_new(T12[j]->A);
    }
    for (int i=0; i<n2*2; i++) {
      for (int j=0; j<3; j++) {
        pa_iter_set(t12tmp_iter, pa_iter_get(t12_iter[j]));
      }
    }
    for (int j=0; j<3; j++) {
      pa_iter_free(t12_iter[j]);
    }
    pa_iter_flush(t12tmp_iter);

    _ Wtmp = block_AppPerm_inverse_channel(3, T12tmp, J, 0);
    _ W0 = _const(n2*2, 0, 1<<d2);
    _ W1 = _const(n2*2, 0, 1<<d2);
    _ W2 = _const(n2*2, 0, 1<<d2);
    pa_iter wtmp_iter = pa_iter_new(Wtmp->A);
    pa_iter w12_iter[3];
    w12_iter[0] = pa_iter_new(W0->A);
    w12_iter[1] = pa_iter_new(W1->A);
    w12_iter[2] = pa_iter_new(W2->A);
    for (int i=0; i<n2*2; i++) {
      for (int j=0; j<3; j++) {
        pa_iter_set(w12_iter[j], pa_iter_get(wtmp_iter));
      }
    }
    for (int j=0; j<3; j++) {
      pa_iter_flush(w12_iter[j]);
    }
    pa_iter_free(wtmp_iter);
#endif
    _ L0 = Grouping(W0);
    _ L1 = Grouping(W1);
    _ L2 = Grouping(W2);
    _ Ltmp = OR(L0, L1);
    L = OR(Ltmp, L2);
    _free(W0); _free(W1); _free(W2); _free(L0); _free(L1); _free(L2); _free(Ltmp);
    //_free(pi_tmp1); _free(pi_tmp2); _free(key1); _free(key2);
    //_free(tmp1.x); _free(tmp1.y); _free(tmp2.x); _free(tmp2.y); _free(tmp3.x); _free(tmp3.y);
    _free(tmp1.x); _free(tmp2.x); _free(tmp3.x); _free(tmp1.y);
    _free(T12[0]); _free(T12[1]); _free(T12[2]);
  }
#endif


  int d3 = blog(len(L)+2-1)+1;
  _ Ltmp = B2A(L, 1<<d3);
  _ V = rank1(Ltmp);
  _free(Ltmp);
  _free(L);

  _ T12c = AppPerm_channel(V, J, 0);
  _slice_(T12c, 0, -1);
  _free(J);
  _free(V);

  _ SA12a = SuffixSort_DC3_new(T12c);
  _free(T12c);
  printf("SuffixSort_DC3_new n = %d order = %d\n", n, order(T));
  //printf("SA12a "); _print(SA12a);

  _ SA12;
  if (order(SA12a) < (1<<d)) {
    printf("extend SA12a %d -> %d\n", order(SA12a), 1<<d);
    SA12 = _extend(SA12a, 1<<d);
  } else {
    SA12 = _shrink(SA12a, 1<<d);
  }
  _free(SA12a);

  _ SA12inv = InvPerm(SA12);
  _ SA12tmp2 = AppPerm_channel(T12pos, SA12, 0);
  _free(SA12);
  _free(T12pos);

  int n02 = n2*2;

  _ T01 = _const(n02, 0, order(Tp));
  _ SA01 = _const(n02, 0, order(SA12inv));

  for (int i=0; i<n2; i++) {
    _setshare(T01, i, Tp, i*3+0);
    _setpublic(SA01, i, i*3+0);
    _setshare(T01, n2+i, Tp, i*3+1);
    _setpublic(SA01, n2+i, i*3+1);
  }

  _ T01x = AppInvPerm_channel(T01, SA12inv, 0);
  _free(T01);

  _pair tmp = share_radix_sort2(T01x, 0);
  _ sigma2inv = InvPerm(tmp.y);
  _free(tmp.x); _free(tmp.y);

  _ sigma = AppPerm_channel(sigma2inv, SA12inv, 0);
  _free(sigma2inv);
  _free(T01x);

  _ B01 = _const(n2*2, 0, 1<<d);
  int d4 = blog(n2*2-1)+1;
  //_ B01 = _const(n2*2, 0, 1<<d4);
  //_ B01 = _const(n2*2, 0, order(Tp));
  for (int i=0; i<n2; i++) {
    _setpublic(B01, n2+i, 1);
  }
  _ B01a = AppInvPerm_channel(B01, sigma, 0);
  _ B01b = PrefixSum(B01a);
  _ B01c = AppPerm_channel(B01b, sigma, 0);
  _ r1 = _slice(B01c, 0, n2);
  _ SA01a = AppInvPerm_channel(SA01, sigma, 0);
  _ rho2 = StableSort(B01a);
  _ SA01b = AppInvPerm_channel(SA01a, rho2, 0);
  _slice_(SA01b, 0, n2);
  _free(SA01);
  _free(rho2);
  _free(SA01a);
  _free(B01a);
  _free(B01b);
  _free(B01c);
  _free(B01);
  _free(sigma);


  _ T021 = _const(n02, 0, order(Tp));
  _ T022 = _const(n02, 0, order(Tp));
  _ R02 = _const(n02, 0, order(SA12inv));

  for (int i=0; i<n2; i++) {
    _setshare(T021, i, Tp, i*3+0);
    _setshare(T022, i, Tp, i*3+1);
    _setshare(R02, i, SA12inv, n2+i);
    _setshare(T021, n2+i, Tp, i*3+2);
    _setshare(T022, n2+i, Tp, i*3+3);
    if (i+1 < n2) _setshare(R02, n2+i, SA12inv, i+1);
  }
  _setshare(R02, n2*2-1, SA12inv, 0);
  _free(Tp);
  _free(SA12inv);

  _ T021b = AppInvPerm(T021, R02);
  _ T022b = AppInvPerm(T022, R02);
  _pair tmp2 = share_radix_sort2(T022b, 0);
  _ key = AppPerm(T021b, tmp2.y);
  _pair tmp1 = share_radix_sort2(key, 0);
  _ sigma4inv = AppPerm(tmp2.y, tmp1.y);
  _ sigma4 = InvPerm(sigma4inv);
  _free(sigma4inv);
  _free(tmp2.x); _free(tmp2.y); _free(tmp1.x); _free(tmp1.y);
  _free(T021); _free(T022); _free(T021b); _free(T022b); _free(key);

  _ sigma2 = AppPerm_channel(sigma4, R02, 0);
  _free(sigma4);
  _free(R02);

  //_ r2 = getrank(sigma2);
  _ r2 = getrank2(sigma2, order(r1));
  _free(sigma2);

  //if (order(r1) > order(r2)) {
  //  _move_(r1, _shrink(r1, order(r2)));
  //}
  _ R = vadd(r1, r2);
  //printf("R "); _print(R);
  _free(r1);
  _free(r2);
  _ SA = merge(SA12tmp2, SA01b, R);
  _free(SA12tmp2);
  _free(SA01b);
  _free(R);

  if (len(SA) > n+1) {
    _slice_(SA, len(SA)-(n+1), 0);
  }

  return SA;
}

_ SuffixSort_DC3_new2_sub(_ T, share_t q)
{
  int n = len(T);
  printf("SuffixSort_DC3_new n = %d order = %d\n", n, q);
  fflush(stdout);

  if (n < 10) {
    return SuffixSort4(T);
  }

  // T12 を作る
  int n2 = (n+3)/3;

  _ T12[3];

  //share_t q = max(order(T), 1 << blog(n2*2-1)+1);
  //share_t q = 1 << blog(n2*2-1)+1;
  //share_t q = 1 << blog(n-1)+1;

  _ T12pos = _const(n2*2, 0, q);

  _ Tp;
  if (order(T) < q) {
    printf("extend T %d -> %d\n", order(T), q);
    Tp = _extend(T, q); // ここを改良したい
  } else {
    Tp = _shrink(T, q);
  }
  _ Tp2 = _const(5, 0, q);
  for (int i=0; i<n; i++) {
    _addpublic(Tp, i, 3);
  }
  _concat_(Tp, Tp2);
  _free(Tp2);
  _setpublic(Tp, n, 2);
  _setpublic(Tp, n+1, 1);

  pa_iter t12tmp_iter;
  pa_iter t12_iter[3];


  for (int j=0; j<3; j++) {
    T12[j] = _const(n2*2, 0, q);
  }
  for (int i=0; i<n2; i++) {
    //for (int j=0; j<3; j++) {
    //  _setshare(T12[j],i, Tp, i*3+1+2-j);
    //  _setshare(T12[j],n2+i, Tp, i*3+2+2-j);
    //}
    _setpublic(T12pos, i, i*3+1);
    _setpublic(T12pos, n2+i, i*3+2);
  }
#if 1
  for (int j=0; j<3; j++) {
    t12_iter[j] = pa_iter_new(T12[j]->A);
  }
  t12tmp_iter = pa_iter_pos_new(Tp->A, 1);
  for (int i=0; i<n2; i++) {
    //for (int j=0; j<3; j++) {
    //  pa_iter_set(t12_iter[j], share_getraw(Tp, i*3+1+2-j));
    //}
    pa_iter_set(t12_iter[2], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[1], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[0], pa_iter_get(t12tmp_iter));
  }
  pa_iter_free(t12tmp_iter);
  t12tmp_iter = pa_iter_pos_new(Tp->A, 2);
  for (int i=0; i<n2; i++) {
    //for (int j=0; j<3; j++) {
    //  pa_iter_set(t12_iter[j], share_getraw(Tp, i*3+2+2-j));
    //}
    pa_iter_set(t12_iter[2], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[1], pa_iter_get(t12tmp_iter));
    pa_iter_set(t12_iter[0], pa_iter_get(t12tmp_iter));
  }
  for (int j=0; j<3; j++) {
    pa_iter_flush(t12_iter[j]);
  }
  pa_iter_free(t12tmp_iter);
#endif  

  _ J;
  _b L;
  {
    share_t q2 = 1 << blog(n2*3+3)+1;
    //q2 = q;
    //_pair tmp3 = share_radix_sort(T12[0]);
    _pair tmp3 = share_radix_sort_channel4(T12[0], q2, 0);
    _pair tmp2 = share_radix_sort_cont4(tmp3.y, T12[1], q2);
    _pair tmp1 = share_radix_sort_cont4(tmp2.y, T12[2], q2);
    J = InvPerm(tmp1.y);

#if 0
    _ W0 = AppInvPerm(T12[0], J);
    _ W1 = AppInvPerm(T12[1], J);
    _ W2 = AppInvPerm(T12[2], J);
#else
    _ T12tmp = _const(n2*2*3, 0, q);
    t12tmp_iter = pa_iter_new(T12tmp->A);
    for (int j=0; j<3; j++) {
      t12_iter[j] = pa_iter_new(T12[j]->A);
    }
    for (int i=0; i<n2*2; i++) {
      for (int j=0; j<3; j++) {
        pa_iter_set(t12tmp_iter, pa_iter_get(t12_iter[j]));
      }
    }
    for (int j=0; j<3; j++) {
      pa_iter_free(t12_iter[j]);
    }
    pa_iter_flush(t12tmp_iter);

    _ Wtmp = block_AppPerm_inverse_channel(3, T12tmp, J, 0);
    _ W0 = _const(n2*2, 0, q);
    _ W1 = _const(n2*2, 0, q);
    _ W2 = _const(n2*2, 0, q);
    pa_iter wtmp_iter = pa_iter_new(Wtmp->A);
    pa_iter w12_iter[3];
    w12_iter[0] = pa_iter_new(W0->A);
    w12_iter[1] = pa_iter_new(W1->A);
    w12_iter[2] = pa_iter_new(W2->A);
    for (int i=0; i<n2*2; i++) {
      for (int j=0; j<3; j++) {
        pa_iter_set(w12_iter[j], pa_iter_get(wtmp_iter));
      }
    }
    for (int j=0; j<3; j++) {
      pa_iter_flush(w12_iter[j]);
    }
    pa_iter_free(wtmp_iter);
#endif
    _ L0 = Grouping(W0);
    _ L1 = Grouping(W1);
    _ L2 = Grouping(W2);
    _ Ltmp = OR(L0, L1);
    L = OR(Ltmp, L2);
    _free(W0); _free(W1); _free(W2); _free(L0); _free(L1); _free(L2); _free(Ltmp);
    //_free(pi_tmp1); _free(pi_tmp2); _free(key1); _free(key2);
    //_free(tmp1.x); _free(tmp1.y); _free(tmp2.x); _free(tmp2.y); _free(tmp3.x); _free(tmp3.y);
    _free(tmp1.x); _free(tmp2.x); _free(tmp3.x); _free(tmp1.y);
    _free(T12[0]); _free(T12[1]); _free(T12[2]);
  }

  int d3 = blog(len(L)+2-1)+1;
  _ Ltmp = B2A(L, q);
  _ V = rank1(Ltmp);
  _free(Ltmp);
  _free(L);

  _ T12c = AppPerm_channel(V, J, 0);
  _slice_(T12c, 0, -1);
  _free(J);
  _free(V);

  _ SA12a = SuffixSort_DC3_new2_sub(T12c, q);
  _free(T12c);
  printf("SuffixSort_DC3_new n = %d order = %d\n", n, order(T));
  //printf("SA12a "); _print(SA12a);

  _ SA12;
  if (order(SA12a) < q) {
    printf("extend SA12a %d -> %d\n", order(SA12a), q);
    SA12 = _extend(SA12a, q);
  } else {
    SA12 = _shrink(SA12a, q);
  }
  _free(SA12a);

  _ SA12inv = InvPerm(SA12);
  _ SA12tmp2 = AppPerm_channel(T12pos, SA12, 0);
  _free(SA12);
  _free(T12pos);

  int n02 = n2*2;

  _ T01 = _const(n02, 0, order(Tp));
  _ SA01 = _const(n02, 0, order(SA12inv));

  for (int i=0; i<n2; i++) {
    _setshare(T01, i, Tp, i*3+0);
    _setpublic(SA01, i, i*3+0);
    _setshare(T01, n2+i, Tp, i*3+1);
    _setpublic(SA01, n2+i, i*3+1);
  }

  _ T01x = AppInvPerm_channel(T01, SA12inv, 0);
  _free(T01);

  share_t q2 = 1 << blog(n2*3+3)+1;
  //_pair tmp = share_radix_sort2(T01x, 0);
  _pair tmp = share_radix_sort_channel4(T01x, q2, 0);
  _ sigma2inv = InvPerm(tmp.y);
  _free(tmp.x); _free(tmp.y);

  _ sigma = AppPerm_channel(sigma2inv, SA12inv, 0);
  _free(sigma2inv);
  _free(T01x);

  _ B01 = _const(n2*2, 0, q);
  int d4 = blog(n2*2-1)+1;
  //_ B01 = _const(n2*2, 0, 1<<d4);
  //_ B01 = _const(n2*2, 0, order(Tp));
  for (int i=0; i<n2; i++) {
    _setpublic(B01, n2+i, 1);
  }
  _ B01a = AppInvPerm_channel(B01, sigma, 0);
  _ B01b = PrefixSum(B01a);
  _ B01c = AppPerm_channel(B01b, sigma, 0);
  _ r1 = _slice(B01c, 0, n2);
  _ SA01a = AppInvPerm_channel(SA01, sigma, 0);
  _ rho2 = StableSort(B01a);
  _ SA01b = AppInvPerm_channel(SA01a, rho2, 0);
  _slice_(SA01b, 0, n2);
  _free(SA01);
  _free(rho2);
  _free(SA01a);
  _free(B01a);
  _free(B01b);
  _free(B01c);
  _free(B01);
  _free(sigma);


  _ T021 = _const(n02, 0, order(Tp));
  _ T022 = _const(n02, 0, order(Tp));
  _ R02 = _const(n02, 0, order(SA12inv));

  for (int i=0; i<n2; i++) {
    _setshare(T021, i, Tp, i*3+0);
    _setshare(T022, i, Tp, i*3+1);
    _setshare(R02, i, SA12inv, n2+i);
    _setshare(T021, n2+i, Tp, i*3+2);
    _setshare(T022, n2+i, Tp, i*3+3);
    if (i+1 < n2) _setshare(R02, n2+i, SA12inv, i+1);
  }
  _setshare(R02, n2*2-1, SA12inv, 0);
  _free(Tp);
  _free(SA12inv);

  _ T021b = AppInvPerm(T021, R02);
  _ T022b = AppInvPerm(T022, R02);
  //_pair tmp2 = share_radix_sort2(T022b, 0);
  _pair tmp2 = share_radix_sort_channel4(T022b, q2, 0);
  _ key = AppPerm(T021b, tmp2.y);
  //_pair tmp1 = share_radix_sort2(key, 0);
  _pair tmp1 = share_radix_sort_channel4(key, q2, 0);
  _ sigma4inv = AppPerm(tmp2.y, tmp1.y);
  _ sigma4 = InvPerm(sigma4inv);
  _free(sigma4inv);
  _free(tmp2.x); _free(tmp2.y); _free(tmp1.x); _free(tmp1.y);
  _free(T021); _free(T022); _free(T021b); _free(T022b); _free(key);

  _ sigma2 = AppPerm_channel(sigma4, R02, 0);
  _free(sigma4);
  _free(R02);

  //_ r2 = getrank(sigma2);
  _ r2 = getrank2(sigma2, order(r1));
  _free(sigma2);

  //if (order(r1) > order(r2)) {
  //  _move_(r1, _shrink(r1, order(r2)));
  //}
  _ R = vadd(r1, r2);
  //printf("R "); _print(R);
  _free(r1);
  _free(r2);
  _ SA = merge(SA12tmp2, SA01b, R);
  _free(SA12tmp2);
  _free(SA01b);
  _free(R);

  if (len(SA) > n+1) {
    _slice_(SA, len(SA)-(n+1), 0);
  }

  return SA;
}

_ SuffixSort_DC3_new2(_ T)
{
  int n = len(T);
  n = max(n, order(T)+3);
  share_t q = 1 << blog(n-1)+1;
  _ Tp = _extend(T, q);

  _ ans = SuffixSort_DC3_new2_sub(T, q);
  //_ ans = SuffixSort_DC3_new(Tp);
  _free(Tp);
  return ans;
}

_ SuffixSortConc(_ T1, _ T2)
{
  int n1 = len(T1);
  int n2 = len(T2);
  int max_n;
  if (n1 > n2) {
        max_n = n1;
    } else {
        max_n = n2;
  }
  int d = blog(n1 + n2 + 1) + 1;
  _ T = _concat(T1, T2);
  _ allone = _const(n1 + n2, 1, order(T));
  _ V = vadd(T, allone);
  _free(allone);
  _bits Vb = _A2B(V, 1<<d); // Bit-decompose T
  _free(V);
  // printf("Vb\n"); _print_bits(Vb);
  _ J = _radix_sort_bits(Vb); // permutation representing 1-character sort
  _bits W = AppInvPerm_bits(Vb, J); // result of 1-character sort (pos -> lex)
  _free_bits(Vb);
  // printf("I:"); _print(InvPerm(J));
  // printf("W\n"); _print_bits(W);
  _ L = Grouping_bits(W); // obtain group boundaries
  _free_bits(W);
  // printf("L "); _print(L);
  _bits Va = _const_bits(n1 + n2, 0, 1<<d, d);
  for (int i=0; i<n1 + n2; i++) {
    _setpublic_bits(Va, i, i);
  }
  for (int i=0; i<d; i++) {
    _move_(Va->a[i], Propagate(L, Va->a[i]));
  }
  // printf("Va\n"); _print_bits(Va);
  //printf("I b "); _print(I);

  int h = 1;
  _ s = sum(L);
  _free(L);
  // printf("h = %d #groups %d\n", h, (int)pa_get(s->A,0));
  while (h < max_n) {
    _bits V1 = AppPerm_bits(Va, J);
    // printf("V1\n"); _print_bits(V1);
    //_ J2 = _const(n1 + n2, 0, 1<<d);
    _ tmp = _const(n1 + n2, 0, 1<<d);

    for (int i=0; i<n1; i++) {
      _setpublic(tmp, i, (i+h) % n1);
    }
    for (int i=0; i<n2; i++) {
      _setpublic(tmp, n1 + i, n1 + ((i+h) % n2));
    }
    //J2 = AppInvPerm(tmp, J);
    // printf("tmp "); _print(tmp);
    //printf("J2 "); _print(J2);
    _bits V2 = AppPerm_bits(V1, tmp);
    _free(tmp);
    // printf("whileV1\n"); _print_bits(V1);
    // printf("whileV2\n"); _print_bits(V2);
    //_free(J2);
    _bits Vb = _const_bits(n1 + n2, 0, 1<<d, d*2);
    for (int k=0; k<d; k++) {
      _move_(Vb->a[k], _dup(V2->a[k]));
    }
    for (int i=0; i<d; i++) {
      _move_(Vb->a[d+i], _dup(V1->a[i]));
    }
    // printf("whileVb\n"); _print_bits(Vb);
    _free(J);
    J = _radix_sort_bits(Vb);
    // printf("I:"); _print(InvPerm(J));
    _bits W = AppInvPerm_bits(Vb, J);
    //printf("W\n"); _print_bits(W);
    _ L = Grouping_bits(W);
    //printf("L "); _print(L);
    _free_bits(Vb);
    _free_bits(W);
    _free_bits(V1);
    _free_bits(V2);

    for (int i=0; i<n1 + n2; i++) {
      _setpublic_bits(Va, i, i);
    }
    for (int i=0; i<d; i++) {
      _move_(Va->a[i], Propagate(L, Va->a[i]));
    }
    h = h*2;
    // printf("L "); _print(L);
    _ s = sum(L);
    // printf("s "); _print(s);
    _free(L);
    share_t r = RANDOM0(s->q);
    //_addpublic(s, 0, r);
    _ s2 = _reconstruct(s);
    //_addpublic(s2, 0, -r);
//    share_t s3 = s2->A[0];
    share_t s3 = pa_get(s2->A,0);
    _free(s);
    _free(s2);
    printf("h = %d #groups %d\n", h, (int)s3);
    //if (s3 > n1 + n2 - 1) break;
  }
  _free_bits(Va);
  _ I = InvPerm(J);
  printf("final I "); _print(I);
  _free(J);
  return I;
}

_pair BWSD_expectation(_ T1, _ T2)
{
  int n1 = len(T1);
  int n2 = len(T2);
  int d = blog(n1 + n2 + 1) + 1;
  _ I = SuffixSortConc(T1, T2);
  _ v = _const(n1 + n2, 0, 2);
  for (int i=0; i<n2; i++) {
    _setpublic(v, n1 + i, 1);
  }
  printf("v \n "); _print(v);
  _ alpha = AppPerm(v, I);
  //printf("a \n "); _print(a);
//_ alpha = _A2B(a, 1);
  printf("alpha \n "); _print(alpha);
//  _free(a);
  _ alpha2 = B2A(alpha, n1+n2+1);
  printf("alpha \n "); _print(alpha);
  _ f = vmul(alpha2, vsub(alpha2, lshift(alpha2, 0)));
  _ g = vmul(vneg(alpha2), vsub(vneg(alpha2), lshift(vneg(alpha2), 0)));
  _ sf = sum(f);
  _ sg = sum(g);
  printf("f \n "); _print(f);
  printf("g \n "); _print(g);
  printf("sf \n "); _print(sf);
  printf("sg \n "); _print(sg);
  _ s = _slice(sf, 0, 1);
  _addshare(s, 0, sg, 0);
  printf("s \n "); _print(s);
  _free(f);
  _free(g);
  _free(sf);
  _free(sg);
  _ a00 = _insert_head(alpha2, 1);
  _ a01 = _insert_head(vneg(alpha2), 0);
  _ a10 = _insert_head(vneg(alpha2), 1);
  _ a11 = _insert_head(alpha2, 0);
  _free(alpha);
  _ t0 = GroupSum(a00, a01);
  _ t1 = GroupSum(a10, a11);

//  printf("a00 \n "); _print(a00);
//  printf("a01 \n "); _print(a01);
//  printf("a10 \n "); _print(a10);
//  printf("a11 \n "); _print(a11);
//  printf("t0 \n "); _print(t0);
//  printf("t1 \n "); _print(t1);
  _free(a00);
  _free(a01);
  _free(a10);
  _free(a11);
  _ st0 = sum(t0);
  _ st1 = sum(t1);

  _ w = _slice(st0, 0, 1);
  _addshare(w, 0, st1, 0);
  _free(t0);
  _free(t1);
  _free(st0);
  _free(st1);
  _pair sw = {s, w};
  return sw;
}

_pair BWSD_entropy(_ T1, _ T2)
{
  int n1 = len(T1);
  int n2 = len(T2);
  int d = blog(n1 + n2 + 1) + 1;
  _ I = SuffixSortConc(T1, T2);
  _ v = _const(n1 + n2, 0, 2);
  for (int i=0; i<n2; i++) {
    _setpublic(v, n1 + i, 1);
  }
  _ alpha = AppPerm(v, I);
  //printf("a \n "); _print(a);
//_ alpha = _A2B(a, 1);
//  _free(a);
  _ alpha2 = B2A(alpha, 2*(n1+n2));
  _ f = vmul(alpha2, vsub(alpha2, lshift(alpha2, 0)));
  _ g = vmul(vneg(alpha2), vsub(vneg(alpha2), lshift(vneg(alpha2), 0)));
  _ sf = sum(f);
  _ sg = sum(g);
  _ s = _slice(sf, 0, 1);
  _addshare(s, 0, sg, 0);
  printf("s \n "); _print(s);
  _free(f);
  _free(g);
  _free(sf);
  _free(sg);
  _ a00 = _insert_head(alpha2, 1);
  _ a01 = _insert_head(vneg(alpha2), 0);
  _ a10 = _insert_head(vneg(alpha2), 1);
  _ a11 = _insert_head(alpha2, 0);
  _free(alpha);
  _ t0 = GroupSum(a00, a01);
  _ t1 = GroupSum(a10, a11);

  _ t = vadd(t0, t1);
  _free(t0);
  _free(t1);
  _bits tt = _A2B(t, 1<<d);
  _ sigma = _radix_sort_bits(tt);
  _bits l = AppInvPerm_bits(tt, sigma);
  _free_bits(tt);
  _free(sigma);
  _ L = Grouping_bits(l);
  _free_bits(l);
  _ allone = _const(len(t), 1, order(t));
  _ W = GroupSum(L, allone);
  _pair sw = {s, _slice(W, 1, n1+n2+1)};
  _free(L);
  _free(allone);
  _free(W);


  return sw;
}


#endif
