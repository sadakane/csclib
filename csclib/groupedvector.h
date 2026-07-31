#ifndef _GROUPEDVECTOR_H
 #define _GROUPEDVECTOR_H
#include "share.h"

/*
  This is an implmentation of the grouped vector operations described in the following paper:

  Nuttapong Attrapadung, Kota Isayama, Kunihiko Sadakane, and Kazunari Tozawa.
  Secure Parallel Computation with Oblivious State Transitions. 
  In Proceedings of the 2024 on ACM SIGSAC Conference on Computer and Communications Security (CCS '24).
  https://doi.org/10.1145/3658644.3690315

*/


// Protocal 2
_ GroupCountSort(_ g_, _ b_)
{
  //printf("GroupCountSort\n");
  int n = len(b_);
  share_t q = 1 << (blog(n-1)+1);
  _ sigma = StableSort2(g_); // line 1
  _ b;
  if (order(b_) == 2) {
    b = B2A(b_, q); // line 2
  } else {
    b = _dup(b_);
  }
  _ g;
  if (order(g_) == 2) {
    g = B2A(g_, q);
  } else {
    g = _dup(g_);
  }
  _ r1 = PrefixSum(b); // line 3
  _ bneg = vneg(b);
  _ r0 = PrefixSum(bneg); // line 4
  _free(bneg); 
  _ r1r = rshift(r1, 0);
  _ r0r = rshift(r0, 0);
  //_ s1 = IfThen(g, r1r); // line 5
  //_ s0 = IfThen(g, r0r); // line 6
  _ g2 = ntimes(g, 2);
  _ r2 = _concat(r0r, r1r);
  _ s2 = IfThen(g2, r2);
  _ s0 = _slice(s2, 0, len(r0r));
  _ s1 = _slice(s2, len(r0r), len(r2));
  _free(g2); _free(r2); _free(s2);
  _free(r0r); _free(r1r); 
  //_ u1 = AppInvPerm(s1, sigma); // line 7
  //_ u0 = AppInvPerm(s0, sigma); // line 8
  _* u = AppInvPerm_list(sigma, s0, s1, NULL);
  _ u0 = u[0];
  _ u1 = u[1];
  free(u);
  _free(s0); _free(s1);
  _ w1 = Diff(u1, 0); // line 10
  _ w0 = lshift(u0, share_getraw(r0, n-1));
  vsub_(w0, u0); // line 11
  _free(u0); _free(u1);
  //_ y1 = AppPerm(w1, sigma); // line 15
  //_ y0 = AppPerm(w0, sigma); // line 16
  _* y = AppPerm_list(sigma, w0, w1, NULL);
  _ y0 = y[0];
  _ y1 = y[1];
  free(y);
  _free(w0); _free(w1); 
  _free(sigma);
  _addpublic(y0, 0, q-1);
  _addpublic(y1, 0, q-1);
  _ y1p = PrefixSum(y1);
  _ z0 = vadd(r0, y1p); // line 17
  _free(r0);
  _ y0p = PrefixSum(y0);
  _free(y0); _free(y1); 
  _ z1 = vadd(r1, y0p); // line 18
  _free(r1);
  _ pi = IfThenElse(b, z1, z0); // line 19
  _free(z0); _free(z1); 
  _free(y1p); _free(y0p);
  _free(g); _free(b); 
  return pi;
}

// Protocal 3
_ GroupStableSort(_ g, _bits B)
{
  int n = len(g);
  share_t q = 1 << (blog(n-1)+1);
  int W = depth_bits(B);
  _ pi = Perm_ID2(n, q); // line 1
  _ g_d = _dup(g); // line 2
  for (int d = W-1; d >= 0; d--) {
    printf("d = %d\n", d);
    _ x_d = AppInvPerm(B->a[d], pi); // line 4
    printf("x_d "); _print_debug(x_d);
    _ sigma_d = GroupCountSort(g_d, x_d); // line 5
    _ c_d = AppInvPerm(x_d, sigma_d); // line 6
    _ c_d0 = rshift(c_d, 0);
    _ c_tmp = XOR(c_d, c_d0); // line 7
    _ g_dtmp = OR(g_d, c_tmp); // line 8
    _move_(g_d, g_dtmp);
    _ pitmp = AppPerm(sigma_d, pi); // line 9
    _move_(pi, pitmp);
    _free(c_tmp); _free(c_d0); _free(c_d); _free(sigma_d); _free(x_d);
  }
  _free(g_d);
  return pi;
}

_ adjustlen(_ x, int n)
{
  _ ans;
  if (len(x) < n) {
    ans = _const(n-len(x), 0, order(x));
    _concat_(ans, x);
  } else if (len(x) > n) {
    ans = _slice(x, len(x)-n, len(x));
  } else {
    ans = _dup(x);
  }
  return ans;
}

// Protocal 16 (mode = dup)
_ DynAccess(_ g, _ f, _ v)
{
  int N = len(g);
  _ tau = StableSort2(f); // line 1
  _ vp = IfThen_b(f, v); // line 4
  _ w0 = AppInvPerm(vp, tau); // line 10
  _ w = adjustlen(w0, N);
  _free(w0);
  _ x = Diff(w, 0); // line 19
  _ sigma = StableSort2(g); // line 2
  _ y = AppPerm(x, sigma); // line 20
  _ z = PrefixSum(y); // line 21
  _free(x); _free(y); _free(tau); _free(sigma); _free(vp); _free(w);
  return z;
}
#define StateAccess_dup DynAccess

// Protocal 16 (mode = mov)
_ StateAccess_mov(_ g, _ f, _ v)
{
  int N = len(g);
  if (N > 1) {
    //printf("total send %ld\n", get_total_send());
    _ tau = StableSort2(f); // line 1
    //printf("tau: total send %ld\n", get_total_send());
    _ sigma = StableSort2(g); // line 2
    //printf("sigma: total send %ld\n", get_total_send());
    _ vp = IfThen_b(f, v); // line 3
    //printf("vp: total send %ld\n", get_total_send());
    _ w0 = AppInvPerm(vp, tau); // line 10
    //printf("w0: total send %ld\n", get_total_send());
    _ w = adjustlen(w0, N);
    //printf("w: total send %ld\n", get_total_send());
    _free(w0);
    _ z = AppPerm(w, sigma); // line 17
    //printf("z: total send %ld\n", get_total_send());
    _free(tau); _free(sigma); _free(vp); _free(w);
    return z;
  } else {
    _ vp = IfThen_b(f, v); // line 3
    _ z = sum(vp);
    _free(vp);
    return z;
  }
}

// Protocal 16 (mode = next)
_ StateAccess_last(_ g, _ f, _ v)
{
  int N = len(g);
  int M = len(f);
  if (N > 1) {
    _ tau = StableSort2(f); // line 1
    _ sigma = StableSort2(g); // line 2
    _ vs = rshift(v, 0);
    _ vp = IfThen_b(f, vs); // line 6 ?
    _ w0 = AppInvPerm(vp, tau); // line 10
    _ w = adjustlen(w0, N);
    _free(w0);
    _ x = lshift(w, share_getraw(v, M-1)); // line 23, 24
    _ z = AppPerm(x, sigma); // line 25
    _free(x); _free(tau); _free(sigma); _free(vs); _free(vp); _free(w);
    return z;
  } else {
    _ z = share_const(1,0,order(v));
    _setshare(z,0,v,M-1);
    return z;
  }
}

_pair StateAccess_last_mov(_ g, _ f, _ vl, _ vm)
{
  int N = len(g);
  int M = len(f);
  if (N > 1) {
    _ tau = StableSort2(f); // line 1
    _ sigma = StableSort2(g); // line 2
    _ vsl = rshift(vl, 0);
    _ vpl = IfThen_b(f, vsl); // line 6 ?
    _free(vsl); 
    _ vpm = IfThen_b(f, vm); // line 3

    //_ w0l = AppInvPerm(vpl, tau); // line 10
    //_ w0m = AppInvPerm(vpm, tau); // line 10
    _* w0 = AppInvPerm_list(tau, vpl, vpm, NULL);
    _ w0l = w0[0];
    _ w0m = w0[1];
    free(w0);
    _free(vpm); _free(vpl); _free(tau); 

    _ wl = adjustlen(w0l, N);
    _ wm = adjustlen(w0m, N);
    _free(w0l);
    _free(w0m);
    _ xl = lshift(wl, share_getraw(vl, M-1)); // line 23, 24
    //_ zl = AppPerm(xl, sigma); // line 25
    //_ zm = AppPerm(wm, sigma); // line 17
    _* z = AppPerm_list(sigma, xl, wm, NULL);
    _ zl = z[0];
    _ zm = z[1];
    free(z);

    _free(xl); _free(sigma); _free(wl); _free(wm);
    _pair ans = {zl, zm};
    return ans;
  } else {
    _ vp = IfThen_b(f, vm); // line 3
    _ zm = sum(vp);
    _free(vp);

    _ zl = share_const(1,0,order(vl));
    _setshare(zl,0,vl,M-1);
    _pair ans = {zl, zm};
    return ans;
  }
}

typedef struct {
  share_array v, phi, g, f;
} share_statefuldata;

// Protocol 4
share_statefuldata GroupBranch(share_statefuldata *x, _ sc)
{
  int n = len(x->v);
  int m = len(x->f);

  _ sphi = x->phi;
  _ sv = x->v;
  _ sg = x->g;
  _ sf = x->f;

  //printf("GroupBranch: n = %d m = %d\n", n, m);
  //printf("v: "); _print_debug(sv);
  //printf("phi: "); _print_debug(sphi);
  //printf("g: "); _print_debug(sg);
  //printf("f: "); _print_debug(sf);
  //printf("sc: "); _print_debug(sc);

  _ rho = GroupCountSort(sg, sc); // line 1
  _ sc1 = AppInvPerm(sc, rho); // line 4
  _ sc1neg = vneg(sc1);
  _ d = Diff(sc1,0); // line 5
  _ g_out = OR(sg,d); // line 6
  //_ v_out = AppInvPerm(sv, rho); // line 2
  //_ phi_out = AppInvPerm(sphi, rho); // line 3
  _* v_phi = AppInvPerm_list(rho, sv, sphi, NULL);
  _ v_out = v_phi[0];
  _ phi_out = v_phi[1];
  free(v_phi);
  //_ sr = StateAccess_last(sf, sg, sc1); // line 8
  //_ l = StateAccess_mov(sf, sg, sc1neg); // line 7
  _pair sr_l = StateAccess_last_mov(sf, sg, sc1, sc1neg);
  _ sr = sr_l.x;
  _ l = sr_l.y;
  _ f_out = share_const(2*m, 0, 2);
  for (int i = 0; i < m; i++) {
    _setshare(f_out, 2*i, l, i); // line 9
    _setshare(f_out, 2*i+1, sr, i); // line 9
  }
  //printf("f_out: "); _print_debug(f_out);
  //printf("g_out: "); _print_debug(g_out);
  //printf("phi_out: "); _print_debug(phi_out);
  //printf("v_out: "); _print_debug(v_out);
  _free(rho); _free(sc1); _free(sc1neg); _free(d);
  _free(sr); _free(l); // _free(sphi); _free(sv); _free(sg); _free(sf); _free(sc);

  share_statefuldata tmp = {v_out, phi_out, g_out, f_out};
//  _free(v_out); _free(phi_out); _free(g_out); _free(f_out);
  return tmp;
}

typedef struct share_segtreedata {
  _ SegTree;
  _ *h;
} share_segtreedata;

int beta(int D, int d, int i) {
  return (i >> (D-d-1)) % 2;
}

int ls(int D, int d, int i) {
  //if ((i >> (D-d-1)) != (i >> (D-d)<<1)) {
  //  printf("ls: D = %d d = %d i = %d\n", D, d, i);
  //  printf("ls: (i >> (D-d-1)) = %d (i >> (D-d)<<1) = %d\n", (i >> (D-d-1)), (i >> (D-d)<<1));
  //  //exit(1);
  // }
  return (i >> (D-d-1));
  //return (i >> (D-d)<<1);
}

int get_pos(int d, int j) {
  return (1<<d)+(j);
}

// Protocol 19
share_segtreedata RangeSearch(_ sx_) 
{
  int n_ = len(sx_);
  int dtmp = blog(n_-1)+1;
  int ntmp = 1 << dtmp; // 2のべき乗に切り上げた長さ
  //printf("n_ = %d n = %d\n", n_, n);
  _ sx = share_const(ntmp, 0, order(sx_));
  _setshares(sx, 0, len(sx_), sx_, 0);

  int n = len(sx);
  int D = blog(n-1)+1+1; // 1つ大きくする
  // segment tree construction
  _ SegTree = share_const(2*n, 0, order(sx));
  for (int i=0; i<n; i++) {
    _setshare(SegTree, n+i, sx, i); // line 1
  }
  _ *h;
  NEWA(h, _, D+1);
  for (int d=0; d<D+1; d++) h[d] = share_const(n, 0, 2);

  int len_ = n;
  for (int d=D-1; d>0; d--){ // line 4
    len_ = len_ >> 1;
    //printf("d = %d len = %d\n", d, len_);
    //int lentmp = len_*4;
#if 0
    _ Vlc = share_const(len_, 0, order(sx));
    _ Vrc = share_const(len_, 0, order(sx));
    for (int j=0; j<len_; j++) {
      //_setshare(Vlc, j, SegTree, 2*len_+2*j);
      //_setshare(Vrc, j, SegTree, 2*len_+2*j+1);
      _setshare(Vlc, j, SegTree, get_pos(d, j*2));
      _setshare(Vrc, j, SegTree, get_pos(d, j*2+1));
    }
    printf("Vlc "); _print_debug(Vlc);
    printf("Vrc "); _print_debug(Vrc);
#else
    _ Vlc = share_slice_step(SegTree, get_pos(d, 0), get_pos(d, len_*2), 2);
    _ Vrc = share_slice_step(SegTree, get_pos(d, 1), get_pos(d, len_*2), 2);
#endif
    _ c = LessThan(Vlc, Vrc); // line 5
    _ t = IfThenElse2(c, Vlc, Vrc); // line 6
    _free(Vrc);
    //for (int j=0; j<len_; j++) {
    //  _setshare(SegTree, len_+j, Vmin, j); // line 7
    //}
    _setshares(SegTree, len_, len_+len_, t, 0); // line 7
    _free(t); _free(c); 
    //printf("SegTree "); _print_debug(SegTree);
    int k = 0;
    _ sls = share_const((n >> 1), 0, order(sx));
    _ x2 = share_const((n >> 1), 0, order(sx));
    _ p2 = share_const((n >> 1), 0, 2);
    k = 0;
    for (int j=0; j<n; j++) {
      if (beta(D, d, j) == 1){
        _setshare(sls, k, Vlc, ls(D, d, k));
        _setshare(x2, k, sx, j);
        _setshare(p2, k, h[d+1], j);
        k++;
      }
    }
    _free(Vlc); 
    //printf("Vls2 "); _print_debug(Vls2);
    //printf("Vrx2 "); _print_debug(Vrx2);
    //printf("n = %d k = %d\n", n, k);
    _ b = LessThan(sls, x2); // line 9
    _ p = OR(b, p2); // line 10
    _free(sls); _free(x2); _free(p2);
    //_ htmp = share_const(n, 0, 2);
    k = 0;
    for (int j=0; j<n; j++) {
      if (beta(D, d, j) == 1){
        _setshare(h[d], j, p, k); // line 10
        //_setshare(htmp, j, Vp2, k); // line 10
        k++;
      } else {
        _setshare(h[d], j, h[d+1], j); // line 12
        //_setshare(htmp, j, h[d+1], j); // line 12
      }
    }
    _free(b); _free(p); 
  }
  for (int d=1; d<D; d++){
    vadd_(h[d], h[d+1]); // line 13, 14
    printf("h[%d] ", d); _print_debug(h[d]);
  }
  _free(sx);
  share_segtreedata ans = {SegTree, h};
  return ans;
}

// Protocol 20 ANSV
_pair ANSV(_ x_)
{
  // parameter initialization
  int n_ = len(x_);
//  int n = len(sx_);
  int dtmp = blog(n_-1)+1;
  int n = 1 << dtmp; // 2のべき乗に切り上げた長さ
  //printf("n_ = %d n = %d\n", n_, n);
  _ x = share_const(n, 0, order(x_));
  _setshares(x, 0, len(x_), x_, 0);

  //int d = blog(n_-1+1)+1;
  //int d = blog(n_-1)+1+1;
  int D = blog(n-1)+1+1; // 1つ大きくする
  //printf("D = %d\n", D);
  //int d = blog(n+1-1)+1;
  //int n = 1 << d; // これ以降，n は2のべき乗
  //n = n_;
  //share_t q = 1 << d+1;
  share_t q = 1 << dtmp; // d 以上なら何でも良い?

  // output data
  _ Lp = share_const(n, 0, q);
  //_ Lv = share_const(n, 0, order(sx));

  share_segtreedata segtreedata = RangeSearch(x); // line 1
  _ SegTree = segtreedata.SegTree;
  _ *h = segtreedata.h;

  // top-down traversal
  // set initial state
  //_ x0 = share_const(n, 0, order(x)); // line 2
  //_setshares(x0,0,n,x,0); // line 2
  _ x0 = _dup(x); // line 2
  _ g0 = share_const(n, 0, 2); // line 3
  share_addpublic(g0, 0, 1); // line 3
  _ phi0 = share_const(n, 0, q); // line 4
  for (int j = 0; j < n; j++) share_addpublic(phi0, j, j); // line 4
  _ Vf = share_const(n, 0, 2);
  //share_addpublic(Vf, 0, 1);
  //_ sf = share_const(1, 0, 2);
  //_setshares(sf,0,1,Vf,0);
  //_ f0 = _slice(Vf, 0, 1); // line 4
  _ f0 = _const(1, 0, 2); // line 4
  share_statefuldata S0 = {x0, phi0, g0, f0}; // line 4
   //_ sc = share_const(n, 0, 2); // line 5
  //_setshares(sc,0,n,h[1],0); // line 5
  _ c0 = _dup(h[1]); // line 5
  for (int j = 0; j < n; j++) {
    //printf("j = %d, beta = %d\n", (j >> (D-2)), ls(D, 1, j));
    //share_addpublic(sc, j, (j >> (D-2)) % 2); // line 5
    share_addpublic(c0, j, beta(D, 1, j)); // line 5
  }
  share_statefuldata S = GroupBranch(&S0, c0); // line 6
  _free(g0); _free(phi0); _free(x0); _free(f0);
  _ sca = B2A(c0,q); // line 7
  _free(c0);
  //for (int j = 0; j < n; j++) share_mulpublic(sca, j, (1 << D-2)); // line 7
  smul_(1 << (D - 2), sca); // line 7
  vadd_(Lp,sca); // line 7
  _free(sca);
  _ xd = S.v;
  _ phi = S.phi; 
  _ g = S.g;
  _setshares(Vf,0,len(S.f),S.f,0); // line 6
  _free(S.f);
  for (int d=1; d<D-1; d++){ // line 8
    int len_ = 1 << d;
    //printf("Vf "); _print_debug(Vf);
    //_ sf = share_const(1<<d,0,2);
    //_setshares(sf,0,1<<d,Vf,0);
    _ f = _slice(Vf, 0, len_);
    _ v = share_slice_step(SegTree, get_pos(d+1, 1), get_pos(d+1, 2*len_), 2); // line 9
    //printf("sg "); _print_debug(sg);
    //printf("sf "); _print_debug(sf);
    _ y = StateAccess_dup(g,f,v); // line 10 右の子の値を取ってくる
    _ a = LessThan(y, xd); // line 11
    _free(y); _free(v); 
    //printf("Yd "); _print_debug(Yd);
    //printf("sv "); _print_debug(sv);
    //printf("Va "); _print_debug(Va);
    //_ st = share_const(n, 0, 2); // line 12
    //_setshares(st,0,n,h[d+1],0); // line 12
    _ t = _slice(h[d+1], 0, n); // line 12
    //for (int j = 0; j < n; j++) share_addpublic(st, j, (j >> (D-2-d)) % 2); // line 12
    for (int j = 0; j < n; j++) share_addpublic(t, j, beta(D, d+1, j)); // line 12
    _ b = AppPerm(t,phi); // line 13
    _free(t); 
    _ h2 = share_const(n, 0, 2); // line 14
    for (int k=1; k<d+1; k++) vadd_(h2, h[k]); // line 14
    _ k = AppPerm(h2,phi); // line 14
    _ c = IfThenElse2(k, a, b); // line 15
    _free(a); _free(b); _free(k); _free(h2);
    share_statefuldata S0 = {xd, phi, g, f};
    share_statefuldata S = GroupBranch(&S0, c); // line 16
    AppInvPerm_(c, phi); // line 17
    _free(phi); _free(g); _free(xd);
    xd = S.v; phi = S.phi; g = S.g;
    //_setshares(Vf,0,1<<(d+1),tmp.f,0);
    int lenf = min(1 << (d+1), len(Vf));
    _setshares(Vf,0,lenf,S.f,0); // line 16
    _free(S.f); _free(f);
    sca = B2A(c,q); // line 18
    _free(c);
    //for (int j = 0; j < n; j++) share_mulpublic(sca, j, (1 << D-2-d)); // line 19
    smul_(1 << (D - 2 - d), sca); // line 19
    vadd_(Lp,sca); // line 19
    _free(sca);
  }
  _ w = StateAccess_dup(g, Vf, x); // line 20
  _ Lv = AppInvPerm(w, phi); // line 21
  _free(xd); _free(Vf);
  _free(phi); _free(g);
  _free(w);

  _free(SegTree);
  for (int d=0; d<D+1; d++) {
    if (h[d] != NULL) {
      _free(h[d]);
    }
  }
  free(h);

  if (len(x_) < len(x)) {
    _move_(Lp, _slice(Lp, 0, len(x_)));
    _move_(Lv, _slice(Lv, 0, len(x_)));
  }
  _free(x);

  _pair out = {Lp, Lv};
  return out;
}


#endif // GROUPEDVECTOR_H