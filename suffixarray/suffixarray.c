#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define OF_MAX 4
#define ONEHOT_MAX 4

#include "share.h"
#include "suffixarray.h"
#include "groupedvector.h"

//parties *_Parties;
//int _party = 0;
//long total_btn = 0, total_bt2 = 0;
//long total_perm = 0;

// ミリ秒単位で時間差を計算するdifftime関数のバージョン
double difftime_msec(struct timespec time2, struct timespec time1) {
  return (time2.tv_sec - time1.tv_sec) * 1000.0 + (time2.tv_nsec - time1.tv_nsec) / 1000000.0;
}

int main(int argc, char *argv[])
{
  if (argc>1) _party = atoi(argv[1]);

  char *fname;
  if (argc < 2) {
    printf("%s [party] filename\n", argv[0]);
    return 0;
  }
  if (argc>2) {
    fname = argv[2];
  } else {
    fname = argv[1];
  }

//  int num_parties = 3;
//  int num_parties = 4;
  mpc_start();

#if 0
{
  share_t x[] = {0, 3, 8, 5, 1, 4, 4, 3, 1, 2, 7, 8, 4, 3, 7, 9};
  int n = sizeof(x)/sizeof(share_t);
  _ s = share_new(n, 16, x);

  share_t ppos[] = {0, 0, 1, 3, 3, 2, 5, 6, 7, 4, 8, 9, 5, 3, 7, 9};
  _ prev = share_new(n, 16, ppos);

  _pair ans5 = RMQ(s, prev/*, prev*/);
  _print(ans5.x);
  _print(ans5.y);
  _free(ans5.x);
  _free(ans5.y);
  return 0;

  _pair ans = ANSV(s);
  _ p = ans.x;
  _ v = ans.y;
  printf("v: "); _print(v);
  printf("p: "); _print(p);
  _free(s); _free(p); _free(v);
  return 0;
}
#endif


#if 0
{
  int n = 32;
  for (int i=0; i<n; i++) {
    for (int j=0; j<n; j++) {
      printf("i=%d j=%d\n", i, j);
      _ a = share_const(1, i, n);
      _ b = share_const(1, j, n);
      _ c = LessThan(a, b);
      int x = (int)share_getraw(c, 0);
      printf("LessThan(%d, %d) = %d\n", i, j, x);
      _free(a); _free(b); _free(c);
    }
  }
  return 0;
}
#endif

#if 0
{
  int n=10;
  share_t q = 32;
  int A[10], B[10];
  for (int i=0; i<n; i++) {
    A[i] = rand() % q;
    B[i] = rand() % q;
  }
  _ sA = share_new(n, q, A);
  _ sB = share_new(n, q, B);
  _ sC = vmul(sA, sB);
  _check(sC);
  printf("A "); _print(sA);
  printf("B "); _print(sB);
  printf("C "); _print(sC);
}
#endif

#if 0
  int A[4] = {0, 1, 0, 1};
  int B[4] = {1, 0, 1, 1};
  _ sA = share_new(4, 16, A);
  printf("sA "); _print(sA);
  _ sB = share_new(4, 16, B);
  printf("sB "); _print(sB);
  _ eq = Equality_bit(sA, sB);
  printf("eq "); _print(eq);
  exit(1);
#endif

#if 1
{

// ファイルの読み込み
  share_t *T;
  int n;
  if (1) {
    FILE *fp = fopen(fname, "rb");
    if (fp == NULL) {
      printf("??? %s\n",fname);
      exit(1);
    }
    fseek(fp,0,SEEK_END);
    n = ftell(fp);
    fseek(fp,0,SEEK_SET);

    T = malloc((n+1) * sizeof(share_t));
    if (T == NULL) {
      printf("not enough memory.\n");
      exit(1);
    }
    for (int i=0; i<n; i++) T[i] = fgetc(fp);
    T[n] = 0;
    fclose(fp);
  }
  _ sT = share_new(n, 1<<8, T);
  if (1) free(T);

  clock_t start_clock, end_clock;
#if 0
  start_clock = clock();
  _ ans = SuffixSort3(sT);
  _sync();
  end_clock = clock();
  printf("SuffixSort     time %f\n", (double)(end_clock - start_clock) / CLOCKS_PER_SEC);
  _check(ans);
#endif

#if 1
  start_clock = clock();
  _pair ans = SuffixSort4_LCP(sT);
  _sync();
  end_clock = clock();
  printf("SuffixSort     time %f\n", (double)(end_clock - start_clock) / CLOCKS_PER_SEC);
  n = len(sT);
  _ sa = ans.x;
  _ lcp = ans.y;
  _move_(lcp, share_insert_head(lcp, 0));
  for (int i=0; i<n+1; i++) {
    int x = (int)share_getraw(sa, i);
    printf("SA[%d] = %d\n", i, x);
  }
  for (int i=0; i<n+1; i++) {
    //if (i>0) _addpublic(lcp, i, 1);
    int x = (int)share_getraw(lcp, i);
    printf("LCP[%d] = %d\n", i, x);
  }
#endif

#if 0
  _pair ans2 = ANSV(lcp);
  //_sync();
  _ p = ans2.x;
  _ v = ans2.y;
  for (int i=0; i<len(p); i++) {
    int x = (int)share_getraw(p, i);
    int y = (int)share_getraw(v, i);
    printf("p[%d] = %d v[%d] = %d\n", i, x, i, y);
  }
#endif

#if 0
  start_clock = clock();
  _ ans2 = SuffixSort_DC3(sT);
  _sync();
  end_clock = clock();
  printf("SuffixSort_DC3 time %f\n", (double)(end_clock - start_clock) / CLOCKS_PER_SEC);
  _check(ans2);
  FILE *fp = fopen("output.sa", "wb");
  _save_binary(ans2, fp);
  fclose(fp);
#endif

#if 0
  if (_party <= 0) {
    for (int i=0; i<=n; i++) {
      if (pa_get(ans->A, i) != pa_get(ans2->A, i)) {
        printf("i=%d %d %d\n", i, (int)pa_get(ans->A, i), (int)pa_get(ans2->A, i));
      }
    }
  }
  _free(ans);
  _free(ans2);
#endif
  _free(sT);
  return 0;
}
#endif

#if 0
{
  char* str = "tobeornottobe";
  share_t T[14];
  for (int i=0; i<13; i++) T[i] = str[i];
  _ sT = share_new(13, 1<<8, T);
  printf("T "); _print(sT);
  _pair ans = SuffixSort_LCP(sT);
  _ I = ans.x;
  _ LCP = ans.y;
  printf("I   "); _print(I);
  printf("LCP "); _print(LCP);
  _free(sT);
  _free(I);
  _free(LCP);
}
#endif

#if 0
{
  char* str = "tobeornottobe";
  share_t T[14];
  for (int i=0; i<13; i++) T[i] = str[i];
  _ sT = share_new(13, 1<<8, T);
  printf("T "); _print(sT);
  _ I0 = SuffixSort(sT);
  printf("I  "); _print(I0);
  _ I = SuffixSort2(sT);
  printf("I  "); _print(I);
  _ I2 = SuffixSort_DC3(sT);
  printf("I2 "); _print(I2);
  _free(sT);
  _free(I);
  _free(I2);
}
#endif

#if 0
{
  int i,n;
  n = 10;
  share_t *A;

  NEWA(A, share_t, n);
  if (_party <= 0) {
    for (i=0; i<n; i++) A[i] = i;
//    for (i=0; i<n; i++) A[i] = rand() % n;
    printf("input ");
    for (i=0; i<n; i++) printf("%d ", A[i]);
    printf("\n");
  }

  _ SA;
  SA = share_new(n, 16, A);
//  SA = share_new(n, 16, A);
  free(A);

  printf("party %d: ", _party);
  for (i=0; i<n; i++) printf("%d ", SA->A[i]);
  printf("\n");

  share_array *ans = share_A2B(SA, 100);
  int k = blog(order(SA)-1)+1;
  for (int i=0; i<k; i++) {
    _print(ans[i]);
    _free(ans[i]);
  }
  _free(SA);
  free(ans);
}
#endif


#if 0
{
  int n1 = 6;
  int n2 = 4;
//  int n = 1000;
  share_t *T1;
  share_t *T2;
  NEWA(T1, share_t, n1);
  NEWA(T2, share_t, n2);
  for (int i=0; i<n1; i++) T1[i] = 'a'+RANDOM(5);
  for (int i=0; i<n2; i++) T2[i] = 'a'+RANDOM(5);
  _ sT1 = share_new(n1, 1<<8, T1);
  _ sT2 = share_new(n2, 1<<8, T2);
  printf("T1 "); _print(sT1);
  printf("T1 "); _print(sT2);
  _ I = SuffixSortConc(sT1, sT2);

  printf("I "); _print_bits(I);
  _free(sT1);
  _free(sT2);
  _free(I);
  free(T1);
  free(T2);
}
#endif

#if 0
{
  int n1 = 14;
  int n2 = 14;

  char* str1 = "tobeornottobe";
  char* str2 = "tobeornottobe";
//  int n = 1000;
  share_t T1[14];
  share_t T2[14];
  for (int i=0; i<13; i++) T1[i] = str1[i];
  _ sT1 = share_new(13, 1<<8, T1);
  for (int i=0; i<13; i++) T2[i] = str2[i];
  _ sT2 = share_new(13, 1<<8, T2);
//  _ sT1 = share_new(n1, 1<<8, T1);
//  _ sT2 = share_new(n2, 1<<8, T2);
  printf("T1 "); _print(sT1);
  printf("T2 "); _print(sT2);
  _pair bwsd = BWSD_expectation(sT1, sT2);

  printf("bwsd_s "); _print(bwsd.x);
  printf("bwsd_k "); _print(bwsd.y);

  _ s = _reconstruct(bwsd.x);
  int a = pa_get(s->A,0);
  _ k = _reconstruct(bwsd.y);
  int b = pa_get(k->A,0);
  float d = (float)b / (float)a;
  printf("%f\n", d);

  _free(sT1);
  _free(sT2);
  _free(bwsd.x);
//  free(T1);
//  free(T2);
}
#endif

#if 0
{
  int n1 = 8;
  int n2 = 8;

  char* str1 = "tobeorna";
  char* str2 = "tobeorna";
//  int n = 1000;
  share_t T1[8];
  share_t T2[8];
  for (int i=0; i<8; i++) T1[i] = str1[i];
  _ sT1 = share_new(8, 1<<8, T1);
  for (int i=0; i<8; i++) T2[i] = str2[i];
  _ sT2 = share_new(8, 1<<8, T2);
//  _ sT1 = share_new(n1, 1<<8, T1);
//  _ sT2 = share_new(n2, 1<<8, T2);
  printf("T1 "); _print(sT1);
  printf("T2 "); _print(sT2);
  _pair bwsd = BWSD_entropy(sT1, sT2);

  printf("bwsd_s "); _print(bwsd.x);
  printf("bwsd_k "); _print(bwsd.y);

  _ s = _reconstruct(bwsd.x);
  float a = pa_get(s->A,0);
  _ k = _reconstruct(bwsd.y);
  float dist_ent = 0;

  for (int i = 0; i < len(k); i++){
    if ((int)pa_get(k->A,i) != 0){
        dist_ent += ((float)pa_get(k->A,i)/a) * log2f((float)(pa_get(k->A,i)/a));
    }
  }
  dist_ent =  dist_ent * (-1);
  printf("%f\n", dist_ent);
  _free(sT1);
  _free(sT2);
  _free(bwsd.x);
//  free(T1);
//  free(T2);
}
#endif

#if 1
{
  int n = 10000;
  if (argc > 2) n = atoi(argv[2]);
//  int n = 100;
  share_t *T;
  NEWA(T, share_t, n);
//  for (int i=0; i<n; i++) T[i] = 'a'+rand() % 10;
  for (int i=0; i<n; i++) T[i] = 'a'+rand() % 2;
  _ sT = share_new(n, 1<<8, T);
  //_ sT = share_new(n, 1<<14, T);

  struct timespec start_time, end_time;
  long total_send, total_recv;
#if 1
  // 開始時間を取得
  clock_gettime(CLOCK_REALTIME, &start_time);
  //clock_gettime(CLOCK_MONOTONIC, &start_time);

  //printf("T "); _print(sT);
  //_ I = SuffixSort(sT);
  //_ I = SuffixSort2(sT);
  //_ I3 = SuffixSort3(sT);
  //printf("I3 "); _print(I3);
  total_send = get_total_send();
  total_recv = get_total_recv();
  //_ I = SuffixSort4(sT);
  _ I = SuffixSort4_full(sT, 0);

  if (_num_parties == 3) {
    _sync();
  }
  if (_num_parties == 4) {
    _sync3();
  }

  // 終了時間を取得
  clock_gettime(CLOCK_REALTIME, &end_time);
  printf("SuffixSort4: %.2f ms\n", difftime_msec(end_time, start_time));
  printf("total send %ld recv %ld\n", get_total_send() - total_send, get_total_recv() - total_recv);
  fflush(stdout);

  //_ Idec = _reconstruct(I);
  _free(I);
#endif

#if 1
  // 開始時間を取得
  clock_gettime(CLOCK_REALTIME, &start_time);

  //printf("T "); _print(sT);
  //_ I = SuffixSort(sT);
  //_ I = SuffixSort2(sT);
  //_ I3 = SuffixSort3(sT);
  //printf("I3 "); _print(I3);
  total_send = get_total_send();
  total_recv = get_total_recv();
  //_ I = SuffixSort4(sT);
  I = SuffixSort4_full(sT, 1);

  if (_num_parties == 3) {
    _sync();
  }
  if (_num_parties == 4) {
    _sync3();
  }

  // 終了時間を取得
  clock_gettime(CLOCK_REALTIME, &end_time);
  printf("SuffixSort4_full: %.2f ms\n", difftime_msec(end_time, start_time));
  printf("total send %ld recv %ld\n", get_total_send() - total_send, get_total_recv() - total_recv);
  fflush(stdout);

  //_ Idec = _reconstruct(I);
  _free(I);
#endif

  //printf("I4 "); _print(I4);
  //SuffixSort4_LCP(sT);
  //_ I5 = SuffixSort_DC3(sT);
  //printf("I5 "); _print(I5);
#if 0
  clock_gettime(CLOCK_REALTIME, &start_time);

  //_ I2 = SuffixSort_DC3(sT);
  //_ I2 = SuffixSort_DC3_new(sT);
  total_send = get_total_send();
  total_recv = get_total_recv();
  _ I2 = SuffixSort_DC3_new2(sT);

  if (_num_parties == 3) {
    _sync();
  }
  if (_num_parties == 4) {
    _sync3();
  }

  clock_gettime(CLOCK_REALTIME, &end_time);
  printf("SuffixSort_DC3_new: %.2f ms\n", difftime_msec(end_time, start_time));
  printf("total send %ld recv %ld\n", get_total_send() - total_send, get_total_recv() - total_recv);
  fflush(stdout);
  //printf("I "); _print(I);
  //_free(I);
  _ I2dec = _reconstruct(I2);
  _free(I2);
#endif
  _free(sT);
  free(T);

#if 0
  if (_party <= 0) {
    for (int i=0; i<n; i++) {
      share_t x1 = share_getraw(Idec, i);
      share_t x2 = share_getraw(I2dec, i);
      if (x1 != x2) {
        printf("%d: %d != %d\n", i, x1, x2);
      }
    }
  }
#endif

}
#endif

#if 0
{
//  share_t x[] = {0, 1, 1, 3, 3, 3, 4, 6};
  share_t x[] = {2, 0, 4, 3};
  _ sx = share_new(4, 16, x);
  _pair I = Unary2(sx, 5);
  printf("x "); _print(sx);
  printf("I "); _print(I.x);
//  share_check(I.x);
  _free(sx);
  _free(I.x);
  //_free(I.y);

}
#endif

#if 0
{
  share_t v[] = {100, 200, 300, 400, 500, 600};
  share_t idx[] = {3, 2, 0, 5};

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

  //printf("total send %ld\n", get_total_send());
  //printf("total recv %ld\n", get_total_recv());


  mpc_end();
  //printf("total btn %ld bt2 %ld perm %ld\n", total_btn, total_bt2, total_perm);
#if 0
  if (bt_mt) MT_free(bt_mt);
  if (bt_map) mymunmap(bt_map);
  if (bt_a && _party==2) free(bt_a);
#endif
  return 0;
}
