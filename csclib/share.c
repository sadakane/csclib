#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define OF_MAX 4
#define ONEHOT_MAX 4

#include "share.h"
#include "LOUDS.h"


int main(int argc, char *argv[])
{
  _party = -1;
  if (argc>1) _party = atoi(argv[1]);

//  int num_parties = 3;
//  int num_parties = 4;

//  mpc_start(num_parties);
  mpc_start();
//  PRG_initialize3(num_parties);
//  precomp_tables_new();

#if 0
{
  int n = 16;
  share_t *pA = malloc(sizeof(share_t)*n);
  for (int i=0; i<n; i++) pA[i] = i;
  _ A = share_new(n, 16, pA);
  printf("A "); _print(A);
  int pI[5] = {3, 0, 1, 2, 5};
  _ I = share_new(5, 16, pI);
  _ B = array_lookups(A, I);
  printf("B "); _print(B);
  _free(A); _free(B); _free(I);
  free(pA);
  return 0;
}
#endif


#if 1
{
  int n = 16;
  //_ A = share_const(n, 0, 8);
  //_randomize(A);
  share_t *pA = malloc(sizeof(share_t)*n);
  for (int i=0; i<n; i++) pA[i] = i;
  _ A = share_new(n, 16, pA);
  //_ B = smod(4, A);
  //_ B = Modulo(A, 2, 16);
  _ B = ChangeModulo(A, 64);
  //_ B = RightShift(A, 1, 16);
  //_pair bc = OverflowConst1_channel(A, 2, 0);
  //_ B = OverflowConst2_channel(A, 0);
  //_ B = Overflow(A, 2);
  //_ B = bc.y;
  //_free(bc.x);
  _ C = _reconstruct(B);
  printf("A "); _print(A);
  printf("B "); _print(B);
  printf("C "); _print(C);
  _free(A); _free(B); _free(C);
  free(pA);
  return 0;
}
#endif

#if 0
{
  share_t x0[4] = {0, 10, 20, 30};
  share_t x1[4] = {40, 50, 60, 70};
  _ A[2];
  A[0] = share_new(4, 256, x0);
  A[1] = share_new(4, 256, x1);
  _bits A_bits = block_share_to_bits(A[0], 2);
  printf("A\n"); _print(A[0]);
  printf("A_bits\n"); _print_bits(A_bits);
  _ A2 = bits_to_block_share(A_bits);
  printf("A2\n"); _print(A2);
  _free_bits(A_bits);
  _free(A2); 
}
#endif

#if 0
{
  share_t ppi1[4] = {1, 2, 3, 0};
  share_t ppi2[4] = {3, 0, 1, 2};
  share_t pB[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  _ pi1 = share_new(4, 4, ppi1);
  _ pi2 = share_new(4, 4, ppi2);
  _ B = share_new(8, 8, pB);
  int bs[] = {1};
  _ pi[2] = {pi1, pi2};
  _ *ans = multi_AppPerm(1, &B, bs, 2, pi);
  printf("ans "); _print(ans[0]);
  _free(pi1); _free(pi2); _free(B); _free(ans[0]); free(ans); 
  return 0;
}
#endif

#if 0
{
  share_t x0[20] = {0, 10, 20, 30, 0, 10, 20, 30, 0, 10, 20, 30, 0, 10, 20, 30, 0, 10, 20, 30};
  share_t pi[20] = {1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0};
  _ X = share_new(20, 64, x0);
  _ P = share_new(20, 4, pi);

  _ ans = batched_AppPerm_fwd_channel(5, 1, X, P, 0);
  //_ ans = block_AppPerm_fwd_channel_new(1, X, P, 0);
  printf("ans\n"); _print(ans);
  _ ans2 = _reconstruct(ans);
  printf("ans2\n"); _print(ans2);
  _free(ans);
  _free(ans2);
  _free(X); _free(P);
  return 0;
}
#endif

#if 0
{
  share_t x0[4] = {0, 1, 2, 3};
  share_t x1[4] = {0, 1, 2, 3};
  share_t x2[2] = {0, 2};
  _ A[3];
  A[0] = share_new(4, 16, x0);
  A[1] = share_new(4, 16, x1);
  A[2] = share_new(2, 4, x2);
  int size[3] = {4, 5, 4};

  _ ans = batched_Unitv2_cum(3, size, A, 2);
  printf("ans\n"); _print(ans);
  _free(ans);
  for (int i=0; i<3; i++) _free(A[i]);
  return 0;
}
#endif

#if 0
{
  share_t pA[4] = {0, 2, 5, 15};
  int n = 4;
  _ A = share_new(n, 16, pA);
  _print(A);
  for (int i=0; i<10; i++) {
    _ B = RandomPerm(n);
    printf("B "); _print(B);
    _free(B);
  }
  _free(A);
  return 0;
}
#endif

#if 0
{
  int A[4] = {1,2,3,4};
  int B[4] = {5,6,7,8};
  if (_party == 1) {
    mpc_send(3, A, 4*sizeof(int));
  }
  if (_party == 3) {
    mpc_recv(1, B, 4*sizeof(int));
    for (int i=0; i<4; i++) {
      printf("%d ", B[i]);
    }
  }  
}
#endif

#if 0
{
  int k = 4;
  for (int i=0; i<(1<<k); i++) {
    _ A = _const(k, 0, 2);
    for (int j=0; j<k; j++) {
      _setpublic(A, j, (i>>j)&1);
    }
    _randomize(A);
    _ O = AND_rec(A, 1);
    printf("A "); _print(A);
    printf("O "); _print(O);
    _free(A); _free(O);
  }
}
#endif

#if 0
{
  share_t A[4] = {1, 2, 3, 4};
  int n = 4;
  _ sA = share_new(n, 8, A);
  _print(sA);
}
#endif


#if 0
{
  printf("start\n");
  share_t A[4] = {1, 2, 3, 4};
  share_t B[4] = {2, 2, 2, 2};
  int n = 4;
  _ sA = share_rss_new(n, 16, A);
  _print(sA);
  _ sB = share_rss_new(n, 16, B);
  _print(sB);
  _s3 sC = vmul_rss(sA, sB);
  _print(sC);
  _ sD = shamir3_reconstruct(sC);
  _print(sD);
  _free(sA); _free(sB); _free(sC); _free(sD);
}
#endif


#if 0
{
  share_t A[4] = {1, 2, 3, 4};
  share_t B[4] = {2, 2, 2, 2};
  int n = 4;
  //_s sA = share_shamir_new(n, 8, A);
  //_s sB = share_shamir_new(n, 8, B);
  share_t irr_poly =0x0b; // X^3 + X + 1
  _s sA = share_shamir_GF_new_channel(n, 8, A, irr_poly, 0);
  _s sB = share_shamir_GF_new_channel(n, 8, B, irr_poly, 0);
  _print(sA);
  _print(sB);
  {_ tmp = shamir3_reconstruct_xor(sA, irr_poly); _print(tmp);}
  {_ tmp = shamir3_reconstruct_xor(sB, irr_poly); _print(tmp);}
  //_s3 sC = vmul_shamir(sA, sB);
  _s3 sC = vmul_shamir_GF(sA, sB, irr_poly);
  _print(sC);
//  {_ tmp = shamir3_reconstruct_xor(sC, irr_poly); _print(tmp);}
  //_s sD = onehotvec_shamir3_online_channel(sC, 2, 0, 0);
  _s sD = onehotvec_shamir3_online_channel(sA, order(sA), irr_poly, 0);
  _print(sD);
  //_ sE = shamir_reconstruct(sD);
  _ sE = shamir_reconstruct_xor(sD, irr_poly);
  _print(sE);
}
#endif

#if 0
  share_t A[4], B[4];
  int n = 4;
  for (int i=0; i<n; i++) A[i] = i+1;
  for (int i=0; i<n; i++) B[i] = 2;
  _ sA = share_shamir_new(n, 16, A);
  _ sB = share_shamir_new(n, 16, B);
  _print(sA);
  _print(sB);
  _ sC = vmul_shamir(sA, sB);
  _print(sC);
  _ sD = shamir3_reconstruct(sC);
  _print(sD);
  _ sE = shamir3_revert(sC);
  _print(sE);
//  _ sF = onehotvec_shamir3_table(4, sC, 16, 0, PRE_OHS3_tbl[4-1][0]);
//  _print(sF);
//  _ sG = shamir3_reconstruct(sF);
//  _print(sG);
  _ sH = shamir_reconstruct(sE);
  _print(sH);
  _ sI = shamir3_to_rss(sC);
  _print(sI);
//  _ sI = share_shamir_GF_new_channel(n, 16, A, 0x13, 0);
//  _print(sI);
//  _ sJ = shamir_reconstruct_xor(sI, 0x13);
//  _print(sJ);
  _free(sA); _free(sB); _free(sC); _free(sD);
  _free(sE); _free(sH);

  PRG_free();
  mpc_end();
  return 0;
#endif

#if 0

  share_t A[4];
  int n = 4;
  for (int i=0; i<n; i++) A[i] = i;
  _ sA = share_new(n, 4, A);

  _ oh = onehotvec_shamir_table_channel(2, sA, 16, PRE_OHS_tbl[2-1][0], 0);
  printf("oh  "); _print(oh);
  {_ tmp = shamir_reconstruct(oh); _print(tmp); _free(tmp);}
  _ oh2 = share_dup(oh);
  for (int i=1; i<len(oh2); i++) {
    _addshare_shamir(oh2, i, oh2, i-1);
  }
  printf("oh2 "); share_print(oh2);
  _ oh3 = vmul_shamir(oh, oh2);
  printf("oh3 "); share_print(oh3);
  _ oh4 = shamir_reduce(oh3, 4);
  printf("oh4 "); share_print(oh4);
  {_ tmp = shamir3_reconstruct(oh4); _print(tmp); _free(tmp);}
  _ oh5 = shamir_convert_channel(oh4, 0);
  printf("oh5 "); share_print(oh5);
  {_ tmp = _reconstruct(oh5); _print(tmp); _free(tmp);}

  _free(sA); _free(oh); _free(oh2); _free(oh3); _free(oh4); _free(oh5);

  mpc_end();
  return 0;
#endif



#if 0
{
  int n = 100;

  share_t *A;

  NEWA(A, share_t, n);
  if (_party <= 0) {
    for (int i=0; i<n; i++) A[i] = i;
  }
  _ sA = share_new(n, 256, A);
  _ sB = vmul(sA, sA);
  _check(sB);
  _print(sA);
  _print(sB);
  _free(sA);
  _free(sB);
//  return 0;
  free(A);
}
#endif

#if 0
{
  int i,n;
  n = 10;
  share_t *A;

  NEWA(A, share_t, n);
  if (_party <= 0) {
//    for (i=0; i<n; i++) A[i] = i;
    for (i=0; i<n; i++) A[i] = rand() % n;
    printf("input ");
    for (i=0; i<n; i++) printf("%d ", A[i]);
    printf("\n");
  }

  _ SA;
  SA = share_new(n, 128, A);
//  SA = share_new(n, 16, A);
  free(A);

  printf("party %d: ", _party);
  for (i=0; i<n; i++) printf("%d ", (int)pa_get(SA->A,i));
  printf("\n");
  share_check(SA);

  share_save(SA, "sa");

  _ SA2;
  SA2 = _reconstruct(SA);
  _free(SA2);

#if 0
  share_t *B;
  NEWA(B, share_t, n);
  if (_party <= 0) {
    for (i=0; i<n; i++) B[i] = i;
    printf("B ");
    for (i=0; i<n; i++) printf("%d ", B[i]);
    printf("\n");
  }
  _ SB;
  SB = share_new(n, 128, B);
  free(B);
  printf("SB party %d: ", _party);
  for (i=0; i<n; i++) printf("%d ", (int)pa_get(SB->A,i));
  printf("\n");

  _ SC;
  SC = vmul(SA, SB);
  printf("SC party %d: ", _party);
  for (i=0; i<n; i++) printf("%d ", (int)pa_get(SC->A,i));
  printf("\n");
  share_check(SC);

  _free(SB);
  _free(SC);
#endif


#if 0
{
//  _pair tmp = share_A2QB(SA,order(SA)/2, 2);
//  printf("b "); _print(tmp.y);
//  printf("x "); _print(tmp.x);
//  _free(tmp.x); _free(tmp.y);
  _pair tmp2 = share_radix_sort(SA);
  printf("pi "); _print(tmp2.y);
  printf("sorted "); _print(tmp2.x);
  _free(tmp2.x); _free(tmp2.y);
}
#endif
  _free(SA);
}
#endif

#if 0
{
  share_t g[] = {1,1,0,0,1,0,1,1};
  _ sg = share_new(8, 100, g);
  share_check(sg);
  _ sigma = StableSort(sg);
  share_check(sigma);

  share_t x[] = {0, 10, 20, 30, 40, 50, 60, 70};
  _ sx = share_new(8, 100, x);
  _ x2 = AppInvPerm(sx, sigma);
  share_check(x2);
  _free(x2);
  _free(sx);

  _pair ans = GenCycle(sg);
  _ pi = ans.x;
  _ pi_inv = ans.y;

  share_t v[] = {1,2,3,4,5,6,7,8};
  _ sv = share_new(8, 100, v);
  _ z = Propagate(sg, sv);
  printf("g "); _print(sg);
  printf("v "); _print(sv);
  printf("z "); _print(z);
  _free(z);
  _free(sv);

  _free(sg);
  _free(sigma);
  _free(ans.x);
  _free(ans.y);
}
#endif

#if 0
{
  share_t g[] = {1,0,0,1,1,0,0,0,1,0,1};
  share_t v[] = {2,4,5,0,2,3,3,7,1,2,3};
  _ sg = share_new(11, 100, g);
  _ sv = share_new(11, 100, v);
  _ a = GroupSum(sg,sv);
  printf("a "); _print(a);
  _free(sg);
  _free(sv);
  _free(a);
}
#endif

#if 0
{
  share_t g[] = {1,1,0,0,1,0,1,1};
  _ sg = share_new(8, 10, g);
  _ u = select1(sg);
  printf("select1 "); _print(u);
  _ v = select0(sg);
  printf("select0 "); _print(v);
  _free(sg);
  _free(u);
  _free(v);
}
#endif

#if 0
{
  share_t v[] = {100, 200, 300, 400, 500, 600};
  share_t idx[] = {0, 2, 3, 5};
  share_t I[] = {0, 1, 0, 0, 1, 0, 1, 0, 0, 1};
  _ sv = share_new(6, 1000, v);
  _ sI = share_new(10, 100, I);
  _ V = BatchAccessUnary(sv, sI);
  printf("V "); _print(V);
  _free(sv);
  _free(sI);
  _free(V);
}
#endif

#if 0
{
  share_t b[] = {1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
             0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1};
  _ sb = share_new(29, 1024, b);
  _ p = LOUDS_parent(sb);
  _ f = LOUDS_firstchild(sb);
  printf("parent "); _print(p);
  printf("firstchild "); _print(f);
  _free(sb);
  _free(p);
  _free(f);

}
#endif

#if 1
{
  share_t b[] = {1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1,
             0, 1, 1, 0, 0, 1, 1, 1, 1};
  _ sb = share_new(29, 1024, b);
  _pair ans = LOUDS_contract(sb);
  _ bb = ans.x;
  _ pi = ans.y;
             ans = LOUDS_contract_easy(sb);
  _ bb2 = ans.x;
  _ pi2 = ans.y;
  _ bb3 = LOUDS_contract_easy2(sb);
  _ bb4 = LOUDS_contract_supereasy(sb);
  printf("bb  "); _print(bb);
  printf("pi  "); _print(pi);
  printf("bb2 "); _print(bb2);
  printf("pi2 "); _print(pi2);
  printf("bb3 "); _print(bb3);
  printf("bb4 "); _print(bb4);

  _free(bb);
  _free(pi);
  _free(bb2);
  _free(pi2);
  _free(bb3);
  _free(bb4);
  _free(sb);
}
#endif

#if 1
{
  share_t L[17] = {1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1};
  share_t w[8] =  {1, 2, 3, 4, 5, 6, 7, 8};

  _ sL = share_new(17, 128, L);
  _ sw = share_new(8, 128, w);
  _ z = PathSum(sL, sw);
  printf("PathSum: ");  _print(z);
  _free(z);

  z = PathSum_supereasy(sL, sw);
  printf("PathSum_supereasy: ");  _print(z);
  _free(z);

  z = TreeSum(sL, sw);
  printf("TreeSum: ");  _print(z);
  _free(z);

  z = TreeSum_easy(sL, sw);
  printf("TreeSum_easy: ");  _print(z);
  _free(z);

  z = TreeSum_supereasy(sL, sw);
  printf("TreeSum_supereasy: ");  _print(z);
  _free(z);

  z = LOUDS_prerank(sL);
  printf("prerank: ");  _print(z);
  _free(z);

  _pair tmp = LOUDS_to_BP(sL);
  printf("BP: ");  _print(tmp.x);
  printf("pi: ");  _print(tmp.y);
  _free(tmp.x);
  _free(tmp.y);

  _free(sL);
  _free(sw);

}
#endif


#if 00
{
  int i,n;
//  n = 100000;
//  n = 1<<3;
  n = 5;
//  n = 10;
//  n = 3;
  share_t *A;

  NEWA(A, share_t, n);
  srand(100);
  if (_party <= 0) {
//    for (i=0; i<n; i++) A[i] = i;
    for (i=0; i<n; i++) A[i] = rand() % n;
  //  for (i=0; i<n; i++) A[i] = rand() % 4;
  //  for (i=0; i<n; i++) A[i] = rand() % (1<<3);
  //  printf("input ");
  //  for (i=0; i<n; i++) printf("%d ", A[i]);
  //  printf("\n");
  }

  _ SA;
//  SA = share_new(n, 1<<19, A);
  share_t q = 1<<(blog(n-1)+1);
//  share_t q = 8;
  SA = share_new(n, q, A);
//  SA = share_new(n, 4, A);
  free(A);

#if 0
  printf("a\n");
  _print(SA);

  _bits b = bit_decomposition_channel(SA, 8, 0);
  printf("b\n");
  _print_bits(b);

  _ c = _B2A_bits(b);
  printf("c\n");
  _print(c);
#endif

#if 0
  _ tmp2 = vmul(SA, SA);
  _print(tmp2);
  _free(tmp2);
#endif

#if 0
  _ b = _shrink(SA, 4);
  printf("a "); _print(SA);
  printf("b "); _print(b);
//  _ ohb = onehotvec_online(b, q, 0);
  _ ohb = onehotvec(b, q, 0);
  printf("ohb "); _print(ohb);
  _ ohb2 = _reconstruct(ohb);
  printf("ohb2 "); _print(ohb2);
  _ sum = _dup(ohb);
  for (int j=0; j<4; j++) {
    if (j != 0) _addshare(sum, j, sum, (j-1)+(n-1)*4);
    for (int i=1; i<n; i++) {
      _addshare(sum, j+i*4, sum, j+(i-1)*4);
    }
  }
  printf("sum "); _print(sum);
  _ m = vmul(ohb, sum);
  _ pi = share_const(n, 0, q);
  for (int i=0; i<n; i++) {
    for (int j=0; j<4; j++) {
      _addshare(pi, i, m, i*4+j);
    }
  }
  printf("pi "); _print(pi);
  _free(b); _free(ohb); _free(ohb2); _free(sum);
  _free(m); _free(pi);
#endif

//  printf("party %d: ", _party);
//  for (i=0; i<n; i++) printf("%d ", SA->A[i]);
//  printf("\n");
//  share_check(SA);

//  share_t table_overflow[] = {0, 0, 0, 1};
//  func1bit3_precomp(19, q, table_overflow, "PRE_OF.dat");

#if 0
{
  _pair tmp = share_A2QB(SA, order(SA), order(SA));
  printf("b "); _print(tmp.y);
//  share_check(tmp.y);
  printf("x "); _print(tmp.x);
//  share_check(tmp.x);
  _pair tmp2 = share_A2QB2(SA, order(SA), order(SA));
//  _reconstruct(tmp2.y);
  printf("b2 "); _print(tmp2.y);
  share_check(tmp2.y);
//  _reconstruct(tmp2.x);
  printf("x2 "); _print(tmp2.x);
  share_check(tmp2.x);
}
#endif

#if 1
  //_pair tmp = share_radix_sort(SA); // 2-party
  _pair tmp = share_radix_sort_shamir(1, SA); // 3-party
//  printf("pi "); _print(tmp.y);
  tmp.x = share_reconstruct(tmp.x);
//  share_check(tmp.x);
  printf("sorted "); _print(tmp.x);
  if (_party == 0) {
    for (i=1; i<n; i++) {
      if (pa_get(tmp.x->A,i-1) > pa_get(tmp.x->A,i)) {
        printf("A[%d] = %d, A[%d] = %d\n", i-1, (int)pa_get(tmp.x->A,i-1), i, (int)pa_get(tmp.x->A,i));
      }
    }
  }
  //_save(tmp.x, "sorted");
  _free(tmp.x); _free(tmp.y);
#endif

#if 0
  SA = share_xor_new(n, q, A);
  printf("SA "); _print(SA);
  tmp = share_radix_sort_xor(SA); // 2-party
  printf("pi "); _print(tmp.y);
  printf("sorted "); _print(tmp.x);
  tmp.x = share_reconstruct_xor(tmp.x);
  printf("sorted "); _print(tmp.x);
  if (_party == 0) {
    for (i=1; i<n; i++) {
      if (pa_get(tmp.x->A,i-1) > pa_get(tmp.x->A,i)) {
        printf("A[%d] = %d, A[%d] = %d\n", i-1, (int)pa_get(tmp.x->A,i-1), i, (int)pa_get(tmp.x->A,i));
      }
    }
  }
  //_save(tmp.x, "sorted");
  _free(tmp.x); _free(tmp.y);
#endif


#if 0

  int d = blog(len(SA)-1)+1;
//  _bits a = _A2B(SA, 1<<d);
  _bits a = _A2B(SA, 2);

  _ pi = _radix_sort_bits(a);
#if 0
  printf("pi "); _print(pi);
#endif
  AppInvPerm_(SA, pi);
//  printf("SA "); _print(SA);

  share_check(pi);
  if (_party == 0) {
    for (i=1; i<n; i++) {
      if (pa_get(SA->A,i-1) > pa_get(SA->A,i)) {
        printf("A[%d] = %d, A[%d] = %d\n", i-1, (int)pa_get(SA->A,i-1), i, (int)pa_get(SA->A,i));
      }
    }
  }
  _free(pi);

//  _ b = Grouping_bits(a);
//  _print(b);
//  _free(b);
  _free_bits(a);
#endif

#if 0
  _ x = _slice(SA, 0, len(SA)-1);
  _ y = _slice(SA, 1, len(SA));
  _bits bx = _A2B(x, 2);
  _bits by = _A2B(y, 2);

  _ eq = Equality_bits(bx, by);
  _ lt = LessThan_bits(bx, by);
  printf("eq "); _print(eq);
  printf("lt "); _print(lt);
#endif

  _free(SA);

}
#endif




#if 0
{
  share_t x[] = {0, 1, 1, 3, 3, 3, 4, 6};
  _ sx = share_new(8, 16, x);
  _ I = Unary(sx, 8);
  printf("x "); _print(sx);
  printf("I "); _print(I);
  share_check(I);
  _free(sx);
  _free(I);

}
#endif

#if 0
{
  share_t v[] = {100, 200, 300, 400, 500, 600};
  share_t idx[] = {0, 2, 3, 5};

  _ sv = share_new(6, 1024, v);
  _ sidx = share_new(4, 1024, idx);
  _ V = BatchAccess(sv, sidx);
  printf("V "); _print(V);
  share_check(V);
  _free(sv);
  _free(sidx);
  _free(V);

}
#endif

#if 1
{
  share_t BP[] = {0,0,1,0,0,0,1,1,0,1,0,0,1,1,1,1};
  _ sBP = share_new(16, 64, BP);
  printf("BP "); _print(sBP);
  _pair tmp = BP_to_LOUDS(sBP);
  _ LOUDS = tmp.x;
  _ sigma = tmp.y;
  printf("LOUDS "); _print(LOUDS);
  printf("sigma "); _print(sigma);

  tmp = LOUDS_to_BP(LOUDS);
  printf("BP2 "); _print(tmp.x);
  _free(LOUDS);
  _free(sigma);
  _free(sBP);
  _free(tmp.x);
  _free(tmp.y);
}
#endif


#if 0
{
  int i,n;
  n = 10;
  share_t *A;

  NEWA(A, share_t, n);
  if (_party <= 0) {
    for (i=0; i<n; i++) A[i] = rand() % n;
    printf("input ");
    for (i=0; i<n; i++) printf("%d ", A[i]);
    printf("\n");
  }

  _ SA;
  int k = blog(n-1)+1;
  int q = 1 << k;
  SA = share_new(n, q, A);
  free(A);

  printf("party %d: ", _party);
  printf("SA "); _print(SA);
  share_check(SA);

  _bits b = _A2B(SA, q);
  _ c = _B2A_bits(b);
  printf("c "); _print(c);
  share_check(c);

}
#endif

#if 0
{
  int i,n;
  n = 100;
  share_t *A;

  NEWA(A, share_t, n);
  if (_party <= 0) {
    for (i=0; i<n; i++) A[i] = rand() % 2;
  //  printf("input ");
  //  for (i=0; i<n; i++) printf("%d ", A[i]);
  //  printf("\n");
  }

  _ SA;
  int q = 2;
  SA = share_new(n, q, A);
  free(A);

  printf("party %d: ", _party);
  //printf("SA "); _print(SA);
  share_check(SA);

  _ b = B2A(SA, n);
  //printf("b "); _print(b);
  share_check(b);
  _free(b);
  _free(SA);

}
#endif

#if 0
{
  share_t x[] = {1,0,0,1,1,0,0,0,1,0,1};
  share_t y[] = {1,1,0,0,0,1,0,1,0,1,0};
  _ sx = share_new(11, 2, x);
  _ sy = share_new(11, 2, y);
  _ eq = Equality_bit(sx,sy);
  printf("eq "); _print(eq);
  _check(eq);
  _ lt = LessThan_bit(sx,sy);
  printf("lt "); _print(lt);
  _check(lt);
  _free(sx);
  _free(sy);
  _free(eq);
}
#endif

#if 0
{
  share_t x[] = {1,0,0,1,1,0,0,0,1,0,1};
  share_t y[] = {1,1,0,0,0,1,0,1,0,1,0};
  for (int i=0; i<11; i++) x[i] = RANDOM(16);
  for (int i=0; i<11; i++) y[i] = RANDOM(16);
  _ sx = share_new(11, 16, x);
  _ sy = share_new(11, 16, y);
  printf("x "); _print(sx);
  printf("y "); _print(sy);

  _bits bx = _A2B(sx, 2);
  _bits by = _A2B(sy, 2);
  printf("x "); _print_bits(bx);
  printf("y "); _print_bits(by);

  _ lt = LessThan_bits(bx, by);
  printf("lt "); _print(lt);
  _check(lt);
  _free(lt);
  _free(sx);
  _free(sy);
  _free_bits(bx);
  _free_bits(by);
}
#endif

//  BeaverTriple_free_tables(BT_tbl);
//  precomp_free_tables(PRE_OF_tbl);
//  precomp_tables_free(); 
  

//  PRG_free();
  mpc_end();
  printf("total btn %ld bt2 %ld perm %ld\n", total_btn, total_bt2, total_perm);
#if 0
  if (bt_mt) MT_free(bt_mt);
  if (bt_map) mymunmap(bt_map);
  if (bt_a && _party==2) free(bt_a);
#endif
  return 0;
}
