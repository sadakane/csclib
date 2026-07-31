#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "BP.h"

#define RUN(s, command) \
    ts = time(NULL); \
    command \
    _sync(); \
    te = time(NULL); \
    fprintf(fp, "%s n = %d time:  %f\n", s, n, difftime(te, ts));


int main(int argc, char *argv[]) {
    _party = -1;
    if (argc > 1) _party = atoi(argv[1]);
    
    mpc_start();

    FILE *fp = stdout;
    double ts, te;

    for (int n = 1<<14; n <= 1<<20; n*=2) {
        share_t *t = generate_random_BP(n);
        share_t *w;
        NEWA(w, share_t, n);
        for (int i=0; i<n; i++) w[i] = RANDOM0(1<<16);

        _ b = share_new(n*2, 2, t);
        _ v = share_new(n, 1<<16, w);
        free(t); free(w);

        _ z, d, pi, louds, s, c, y;

        RUN("parentlabel ", z = BP_parentlabel(b, v);)
        //printf("parent label: ");   _print(z);
        _free(z);

        // Compute BP_depth
        //printf("b: ");   _print(b);
        RUN("depth ", d = BP_depth(b);)
        //printf("depth: ");  _print(d);
        _free(d);

        // Compute BPtoLOUDS
        RUN("BPtoLOUDS", pi = BPtoLOUDS(b);
            louds = AppPerm(b, pi);)
        //printf("LOUDS: "); _print(louds);
        //printf("pi: "); _print(pi);
        _free(pi); _free(louds);

        // BP_pathsum
        RUN("pathsum ", s = BP_pathsum(b, v);)
        //printf("pathsum: "); _print(s);
        _free(s);


        // BP_treesum
        RUN("treesum ", s = BP_treesum(b, v);)
        //printf("treesum: "); _print(s);
        _free(s);

        // BP_patheval
        RUN("patheval ", c = _const(n, 0, 1<<16);
        y = BP_patheval(b, c, v);)
        //printf("patheval: "); _print(y);
        _free(y); _free(c);

        // BP_LCA
        RUN("LCA ", c = _const(n, 0, 1<<16);
        y = BP_LCA(b, c);)
        //printf("LCA: "); _print(y);
        _free(y); _free(c);
        
    }


    mpc_end();
    return 0;
}
