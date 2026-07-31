#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define OF_MAX 4
#define ONEHOT_MAX 4

#include "LOUDS.h"
#include "BP.h"

#define RUN(s, command) \
    ts = time(NULL); \
    command \
    _sync(); \
    te = time(NULL); \
    fprintf(fp, "%s n = %d time:  %f\n", s, n, difftime(te, ts));

int main(int argc, char *argv[])
{
  if (argc>1) _party = atoi(argv[1]);

  mpc_start();

  FILE *fp = stdout;
  double ts, te;

  for (int n = 1<<4; n <= 1<<8; n*=2) {
    share_t *t = generate_random_BP(n);
    share_t *w;
    NEWA(w, share_t, n);
    for (int i=0; i<n; i++) w[i] = RANDOM0(1<<16);

    _ b = share_new(n*2, 2, t);
    _ v = share_new(n, 1<<16, w);
    free(t); free(w);

    _ LOUDS, sigma;

    //printf("BP "); _print(b);
    RUN("BP_to_LOUDS ",
    _pair tmp = BP_to_LOUDS(b);
    LOUDS = tmp.x;
    sigma = tmp.y;)
    //printf("LOUDS "); _print(LOUDS);
    //printf("sigma "); _print(sigma);

    // childlabelsum
    RUN("LOUDS_childlabelsum ", 
      _ L = _slice(LOUDS, 1, n*2);
      _ s = LOUDS_sum_children(v, L);)
    _free(s); _free(L);



    // LOUDS_contract
    RUN("LOUDS_contract ",
    _pair ans = LOUDS_contract(LOUDS);
    _free(ans.x); _free(ans.y);)

    // LOUDS_to_BP and LOUDS_postrank
    RUN("LOUDS_to_BP ", tmp = LOUDS_to_BP(LOUDS);)
    //printf("BP: ");  _print(tmp.x);
    //printf("pi: ");  _print(tmp.y);
    _free(tmp.x);
    _free(tmp.y);


    _free(LOUDS); _free(sigma);
  }
  mpc_end();

  return 0;
}
