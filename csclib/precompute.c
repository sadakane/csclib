#include <stdio.h>
#include <stdlib.h>

typedef int share_t;
#include "share.h"
#include "beaver.h"
#include "shamir.h"

#include "unitv.h"
#include "field.h"

#include "precompute2.h"


int main(int argc, char *argv[])
{
  _party=-1;
  mpc_start();

//  long n = 100;
  long n = 10000;
  if (argc > 1) n = atoi(argv[1]);

//  int w = 8;
  int w = 30;
  if (argc > 2) w = atoi(argv[2]);

  printf("n = %ld w = %d\n", n, w);

  FILE *fin = fopen("config.txt", "r");
  if (fin != NULL) {
    precomp_read_config(n, 1<<w, fin);
    fclose(fin);
  }


#if 0
//  BeaverTriple_precompute(n, 1<<w, w);
  BeaverTriple_precomp(n, 1<<w, "PRE_BT.dat");

  share_t table_xor[] = {0, 1, 
                         1, 0};
  func1bit3_precomp(n, 1<<w, table_xor, "PRE_B2A.dat");

  share_t table_overflow1[] = {0, 0, 
                               0, 1};
  func1bit3_precomp(n, 1<<w, table_overflow1, "PRE_OF1.dat");

  share_t table_overflow2[] = {0, 0, 0, 0,
                               0, 0, 0, 1,
                               0, 0, 1, 1,
                               0, 1, 1, 1};
  funckbit_precomp(2, n, 1<<w, table_overflow2, "PRE_OF2.dat");
  share_t table_overflow3[] = {0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 1,
                               0, 0, 0, 0, 0, 0, 1, 1,
                               0, 0, 0, 0, 0, 1, 1, 1,
                               0, 0, 0, 0, 1, 1, 1, 1,
                               0, 0, 0, 1, 1, 1, 1, 1,
                               0, 0, 1, 1, 1, 1, 1, 1,
                               0, 1, 1, 1, 1, 1, 1, 1};
  funckbit_precomp(3, n, 1<<w, table_overflow3, "PRE_OF3.dat");
  share_t table_overflow4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
                               0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                               0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  funckbit_precomp(4, n, 1<<w, table_overflow4, "PRE_OF4.dat");

#endif

#if 0
  onehotvec_precomp(1, n, 1<<w, "PRE_OHA1.dat", 0);
  onehotvec_precomp(2, n, 1<<w, "PRE_OHA2.dat", 0);
  onehotvec_precomp(3, n, 1<<w, "PRE_OHA3.dat", 0);
  onehotvec_precomp(4, n, 1<<w, "PRE_OHA4.dat", 0);
  onehotvec_precomp(1, n, 1<<w, "PRE_OHX1.dat", 1);
  onehotvec_precomp(2, n, 1<<w, "PRE_OHX2.dat", 1);
  onehotvec_precomp(3, n, 1<<w, "PRE_OHX3.dat", 1);
  onehotvec_precomp(4, n, 1<<w, "PRE_OHX4.dat", 1);
  onehotvec_shamir_precomp(1, n, 1<<w, "PRE_OHS1.dat");
  onehotvec_shamir_precomp(2, n, 1<<w, "PRE_OHS2.dat");
  onehotvec_shamir_precomp(3, n, 1<<w, "PRE_OHS3.dat");
  onehotvec_shamir_precomp(4, n, 1<<w, "PRE_OHS4.dat");
#endif

#if 0
  onehotvec_shamir3_precomp(4, n, 1<<w, 0,  "PRE_OHS3_0.dat");
  onehotvec_shamir3_precomp(4, n, 1<<4, 0x13,  "PRE_OHS3_0x13.dat");
#endif

#if 0
  char fname[100];
  w = 30;
  for (int n=1; n<=20; n++) {
    sprintf(fname, "PRE_DS_n%d_w%d.dat", n, w);
    dshare_precomp(1, 1<<n, 1<<w, 0, fname);
    sprintf(fname, "PRE_DSi_n%d_w%d.dat", n, w);
    dshare_precomp(1, 1<<n, 1<<w, 1, fname);
  }
//  dshare_precomp(1, 1<<12, 1<<w, 0, "PRE_DS_n12_w30.dat");
#endif


#if 0
  bit_decomposition_precomp(10000, 1<<30, "PRE_BITDECOMP_n10000_30.dat");
  bit_decomposition_precomp(100, 1<<3, "PRE_BITDECOMP_n100_3.dat");

#endif

#if 0
  BeaverTriple_GF_precomp(n, 0x13, "PRE_GF_13.dat"); // X^4 + X + 1
  BeaverTriple_GF_precomp(n, 0x11b, "PRE_GF_11b.dat"); // X^8 + X^4 + X^3 + X + 1
#endif

#if 0
  shamir3_revert_precomp(n, 16, 0x13, "PRE_RE_13.dat");
#endif

  mpc_end();

  return 0;
}
