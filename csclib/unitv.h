#ifndef _UNITV_H
#define _UNITV_H

/*
  An implementation of
    Nuttapong Attrapadung, Hiraku Morita, Kazuma Ohara, Jacob C. N. Schuldt, and Kazunari Tozawa. 
    Memory and Round-Efficient MPC Primitives in the Pre-Processing Model from Unit Vectorization. 
    In Proceedings of the 2022 ACM on Asia Conference on Computer and Communications Security (ASIA CCS '22),
    pp. 858–872, 2022. 
    https://doi.org/10.1145/3488932.3517407
*/

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int n;
    int old_q;
    int new_q;
    share_array r;
    share_array z;
    share_t *z_raw;
}* unitv_correlated_randomness;

typedef struct {
    int n;
    int l;
    int new_q;
    share_array r;
    share_array z;
    share_t *z_raw;
}* unitvb_correlated_randomness;

void unitv_cr_free(unitv_correlated_randomness cr) {
    share_free(cr->r);
    share_free(cr->z);
    if (cr->z_raw) free(cr->z_raw);
    free(cr);
}

void unitvb_cr_free(unitvb_correlated_randomness cr) {
    share_free(cr->r);
    share_free(cr->z);
    if (cr->z_raw) free(cr->z_raw);
    free(cr);
}

// Online correlated randomness computation
unitv_correlated_randomness Unitv_prep_channel(int n, share_t input_q, share_t output_q, int channel) {
    share_t *R=NULL, *Z=NULL;
    share_array r, e;
    if (_party <= 0) {
        NEWA(R, share_t, n);
        NEWA(Z, share_t, n*input_q);
        memset(Z, 0, sizeof(share_t) * n*input_q);
        for (int i = 0; i < n; ++i) {
            //R[i] = RANDOM(mt0, input_q);
            R[i] = RANDOM(mt_[0][channel], input_q);
            Z[i*input_q+R[i]] = 1;
        }
        
    }

    r = share_new_channel(n, input_q, R, channel);   // Want to parallelize these two communications here
    e = share_new_channel(n*input_q, output_q, Z, channel);

    NEWT(unitv_correlated_randomness, cr);
    if (_party <= 0) {
      free(R); 
      //free(Z);
      cr->z_raw = Z;      
    } else {
      cr->z_raw = pa_unpack(e->A);
    }

    cr->n = n;
    cr->old_q = input_q;
    cr->new_q = output_q;
    cr->r = r;
    cr->z = e;
    return cr;
}
#define Unitv_prep(n, m) Unitv_prep_channel(n, m, 0)


unitvb_correlated_randomness Unitvb_prep_channel(int n, int l, share_t new_q, int channel) {
    share_t *R=NULL, *Z=NULL;
    share_array r;
    share_array z;

    if (_party <= 0) {
        NEWA(R, share_t, n * l);
        NEWA(Z, share_t, n * (1<<l));
        memset(Z, 0, sizeof(share_t) * n * (1<<l));
        for (int i = 0; i < n; ++i) {
            share_t x = 0;
            for (int j = 0; j < l; ++j) {
                x <<= 1;
                //R[l * (i+1) - 1 - j] = RANDOM(mt0, 2);
                R[l * (i+1) - 1 - j] = RANDOM(mt_[0][channel], 2);
                x += R[l * (i + 1) - 1 - j];
            }
            Z[i * (1<<l) + x] = 1;
        }

        r = share_new_channel(n * l, 2, R, channel);
        z = share_new_channel(n * (1<<l), new_q, Z, channel);

        free(R);
        free(Z);
    }
    else {
        r = share_new_channel(n * l, 2, R, channel);
        z = share_new_channel(n * (1<<l), new_q, Z, channel);
    }

    NEWT(unitvb_correlated_randomness, cr);
    cr->n = n;
    cr->l = l;
    cr->new_q = new_q;
    cr->r = r;
    cr->z = z;
    cr->z_raw = NULL;

    return cr;
}

typedef struct UV_tables {
    int n;
    share_t old_q;
    share_t new_q;
    precomp_table r, z;
    MMAP *map;
}* UV_tables;

typedef struct uv_tbl_list {
    UV_tables tbl;
    int n;
    long count; // Declared with reference to dshare, but purpose is unclear
    struct uv_tbl_list *next;
}* uv_tbl_list;

uv_tbl_list PRE_UV_tbl[MAX_CHANNELS];
long PRE_UV_count[MAX_CHANNELS];

// Precomputation of correlated randomness
// Given a share of a vector x=(x_0, x_1, ..., x_{n-1}) of length n with modulus old_q,
// Want to return a share of an old_q-dimensional unit vector where the x_i-th element is 1 for i=0, 1, ..., n-1. Each element of the unit vector has modulus new_q.
// This function precomputes the correlated randomness needed there.
// Precompute and write to file: for i=0, 1, ..., n-1, a random integer r_i with modulus old_q and a share of the r_i-th unit vector.
void UV_tables_precomp(int n, share_t old_q, share_t new_q, char *fname) {
    // Prepare file to write precomputation results
    FILE *fp0;
    FILE *fp1;
    FILE *fp2;
    char *fname0 = precomp_fname(fname, 0);
    char *fname1 = precomp_fname(fname, 1);
    char *fname2 = precomp_fname(fname, 2);
    fp0 = fopen(fname0, "wb");
    fp1 = fopen(fname1, "wb");
    fp2 = fopen(fname2, "wb");
    if (fp0 == NULL) {
        printf("cannot open %s\n", fname0);
        exit(1);
    }  
    if (fp1 == NULL) {
        printf("cannot open %s\n", fname1);
        exit(1);
    }  
    if (fp2 == NULL) {
        printf("cannot open %s\n", fname2);
        exit(1);
    }

    writeuint(sizeof(int), n, fp1);
    writeuint(sizeof(int), n, fp2);
    writeuint(sizeof(share_t), old_q, fp1);
    writeuint(sizeof(share_t), old_q, fp2);
    writeuint(sizeof(share_t), new_q, fp1);
    writeuint(sizeof(share_t), new_q, fp2);

    // Prepare random seed for precomputation
    //unsigned long init[5]={0x123, 0x234, 0x345, 0x456, 0};
    //MT m0 = MT_init_by_array(init, 5);
    //MT mt0 = mt_[0][0];

    packed_array r1, r2;
    packed_array z1, z2;
    int k_old_q = blog(old_q - 1) + 1;
    int k_new_q = blog(new_q - 1) + 1;
    r1 = pa_new(n, k_old_q);
    r2 = pa_new(n, k_old_q);
    z1 = pa_new(n*old_q, k_new_q);
    z2 = pa_new(n*old_q, k_new_q);
    for (int i = 0; i < n; ++i) {
        share_t r = RANDOM(mt0, old_q);
        share_t r_ = RANDOM(mt0, old_q);
        pa_set(r1, i, r_);
        pa_set(r2, i, (r + old_q - r_) % old_q);
        for (int j = 0; j < old_q; ++j) {
            share_t z_ = RANDOM(mt0, new_q);
            if (j == r) {
                share_t x = z_, y = (new_q +1 - z_) % new_q;
                pa_set(z1, i*old_q + j, x);
                pa_set(z2, i*old_q + j, y);
            }
            else {
                share_t x = z_, y = (new_q - z_) % new_q;
                pa_set(z1, i*old_q + j, x);
                pa_set(z2, i*old_q + j, y);
            }
        }
    }
    precomp_write_pa(fp1, r1, old_q);
    precomp_write_pa(fp2, r2, old_q);
    precomp_write_pa(fp1, z1, new_q);
    precomp_write_pa(fp2, z2, new_q);
    
    pa_free(r1);
    pa_free(r2);
    pa_free(z1);
    pa_free(z2);
    fclose(fp0);
    fclose(fp1);
    fclose(fp2);
    free(fname0);
    free(fname1);
    free(fname2);
    
    return;
}

void UV_tables_free(UV_tables tbl) {
    if (tbl == NULL)
        return;
    if (_party < 0 || _party > 2)
        return;
    precomp_free(tbl->r);
    precomp_free(tbl->z);
    if (tbl->map != NULL)
        mymunmap(tbl->map);
    free(tbl);
}

UV_tables UV_tables_read(char *fname) {
    NEWT(UV_tables, tbl);

    if (_party <= 0 || _party > 2) {
        tbl->r = tbl->z = NULL;
        tbl->map = NULL;
        return tbl;
    }

    char *fname2 = precomp_fname(fname, _party);

    MMAP *map = NULL;
    map = mymmap(fname2);
    uchar *p = (uchar *)map->addr;//printf("p: %p\n", p);
    tbl->n = getuint(p, 0, sizeof(int)); p += sizeof(int);//printf("p: %p\n", p);
    tbl->old_q = getuint(p, 0, sizeof(share_t)); p += sizeof(share_t);//printf("p: %p\n", p);
    tbl->new_q = getuint(p, 0, sizeof(share_t)); p += sizeof(share_t);//printf("p: %p\n", p);
    tbl->r = precomp_read(&p);
    tbl->z = precomp_read(&p);
    tbl->map = map;

    free(fname2);

    return tbl;
}

unitv_correlated_randomness unitv_cr_new_party0(int n, share_t old_q, share_t new_q) {
    NEWT(unitv_correlated_randomness, cr);
    cr->n = n;
    cr->old_q = old_q;
    cr->new_q = new_q;
    cr->r = share_const(n, 0, old_q);
    cr->z = share_const(n*old_q, 0, new_q);
    NEWA(cr->z_raw, share_t, n*old_q);
    memset(cr->z_raw, 0, sizeof(share_t)*n*old_q);
    for (int i = 0; i < n; ++i) {
      share_setpublic(cr->z, i*old_q, 1);
      cr->z_raw[i*old_q] = 1;
    }
    return cr;
}

// Fetch from precomputed table
// n is the length of data share array. It's fine if n <= tbl->n.
static void unitv_new_precomp(UV_tables tbl, int n, share_t old_q, share_t new_q, unitv_correlated_randomness *cr_) {
    if (_party > 2)
        return;
    PRE_UV_count[0] += 1;
    if (_party <= 0) {
        *cr_ = unitv_cr_new_party0(n, old_q, new_q);
        return;
    }

    NEWT(unitv_correlated_randomness, cr);
    cr->n = n;
    cr->old_q = old_q;
    cr->new_q = new_q;
    cr->r = share_const(n, 0, cr->old_q);
    pa_iter itr = pa_iter_new(cr->r->A);
    for (int i = 0; i < n; ++i) {
        pa_iter_set(itr, precomp_get(tbl->r) % cr->old_q);
    }
    pa_iter_flush(itr);
    cr->z = share_const(n*cr->old_q, 0, cr->new_q);
    NEWA(cr->z_raw, share_t, n*cr->old_q);
    itr = pa_iter_new(cr->z->A);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < cr->old_q; ++j) {
            share_t x = precomp_get(tbl->z) % cr->new_q;
            pa_iter_set(itr, x);
            cr->z_raw[i*cr->old_q+j] = x;
        }
    }
    pa_iter_flush(itr);

    *cr_ = cr;
    return;
}

uv_tbl_list uv_tbl_list_insert(UV_tables tbl, int n, int old_q, int new_q, uv_tbl_list head) {
    NEWT(uv_tbl_list, list);
    list->tbl = tbl;
    list->tbl->n = n;
    list->tbl->old_q = old_q;
    list->tbl->new_q = new_q;
    list->n = n;
    list->count = 0;
    list->next = head;
    return list;
}

// Return correlated randomness to convert from length n, modulus old_q to modulus new_q
UV_tables uv_tbl_list_search(uv_tbl_list list, int n, share_t old_q, share_t new_q) {
    while (list != NULL) {
        if (list->tbl->n == n && list->tbl->old_q == old_q && list->tbl->new_q == new_q) {
            return list->tbl;
        }
        list = list->next;
    }
    return NULL;
}

// Return correlated randomness to convert from length >= n, modulus old_q to modulus new_q
UV_tables uv_tbl_list_search2(uv_tbl_list list, int n, share_t old_q, share_t new_q) {
    while (list != NULL) {
        if (/*list->tbl->n >= n &&*/ list->tbl->old_q == old_q && list->tbl->new_q % new_q == 0) {
            return list->tbl;
        }
        list = list->next;
    }
    return NULL;
}

void uv_tbl_list_free(uv_tbl_list list) {
    uv_tbl_list next;
    while (list != NULL) {
        next = list->next;
        UV_tables_free(list->tbl);
        free(list);
        list = next;
    }
}

void uv_tbl_init(void) {
    for (int i = 0; i < _opt.channels; ++i) {
        PRE_UV_tbl[i] = NULL;
        PRE_UV_count[i] = 0;
    }
}

void uv_tbl_read(int channel, int n, int old_q, int new_q, char *fname) {
    UV_tables tbl = UV_tables_read(fname);
    PRE_UV_tbl[channel] = uv_tbl_list_insert(tbl, n, old_q, new_q, PRE_UV_tbl[channel]);
}

////////////////////////////////////////////////////////
// Protocol 1: Unitv
////////////////////////////////////////////////////////
/**
 * @brief Converts a shared array to a unit vector representation using a specified channel.
 *
 * This function performs a unit vector conversion on a shared array `x` by utilizing
 * correlated randomness. It first attempts to use a precomputed table for efficiency,
 * falling back to online computation if no suitable table is found.
 *
 * @param x       The input shared array to be converted.
 * @param new_q   The new modulus value for the output shares.
 * @param channel The communication channel index to use for reconstruction.
 *
 * @return A pointer to an array of `share_array` representing the unit vector encoding
 *         of `x` under the new modulus `new_q`, or `NULL` if `_party > 2`.
 *
 * @note Only supports up to 2 parties. Returns `NULL` if `_party > 2`.
 * @note If no precomputed UV table matching `(n, old_q, new_q)` is found and
 *       `_opt.warn_precomp` is set, a warning message is printed to stdout.
 * @note The caller is responsible for freeing the returned array.
 */
share_array* Unitv_channel(share_array x, share_t new_q, int channel) {
    if (_party > 2)
        return NULL;
    
    int n = x->n, old_q = x->q;

    // Prepare correlated randomness
    unitv_correlated_randomness cr;
    UV_tables tbl = uv_tbl_list_search2(PRE_UV_tbl[channel], x->n, old_q, new_q);//printf("search ok\n");fflush(stdout);
    if (tbl != NULL) {  // When precomputed table matching conditions exists
      unitv_new_precomp(tbl, x->n, old_q, new_q, &cr);
    } else {
      if (_opt.warn_precomp) {
        printf("Unitv_channel1: without UV_table n = %d old_q = %d new_q = %d\n", x->n, x->q, new_q);fflush(stdout);
      }
      cr = Unitv_prep_channel(x->n, x->q, new_q, channel);
    }

    // Compute answer
    share_array *ans;
    NEWA(ans, share_array, n);
    share_array s = vsub(cr->r, x);
    share_array s_reconstructed = share_reconstruct_channel(s, channel);// !!!!!
    share_t *cr_tmp = pa_unpack(cr->z->A);
    for (int i = 0; i < x->n; ++i) {
      share_t m = pa_get(s_reconstructed->A, i);
      ans[i] = share_const(old_q, 0, new_q);
      //share_t *cr_tmp = pa_unpack(cr->z->A);
      pa_iter itr = pa_iter_new(ans[i]->A);
      for (int j = 0; j < old_q; ++j) {
        if (cr_tmp[old_q*i+(j+m) % old_q] != cr->z_raw[old_q*i+(j+m) % old_q]) {
          printf("%d %d\n", cr_tmp[old_q*i+(j+m) % old_q], cr->z_raw[old_q*i+(j+m) % old_q]); // debug code for debugging?
        }
        pa_iter_set(itr, cr_tmp[old_q*i+(j+m) % old_q]);
      }
      pa_iter_flush(itr);
      //free(cr_tmp);
    }
    free(cr_tmp);
    _free(s);
    _free(s_reconstructed);
    unitv_cr_free(cr);

    return ans;
}
#define Unitv(x, new_q) Unitv_channel(x, new_q, 0)

////////////////////////////////////////////////////////
// Concatenate (serialize) and return Unitv answer
////////////////////////////////////////////////////////
/**
 * @brief Converts a shared array to a new quantization level using unit vector representation.
 *
 * This function performs a share conversion from an old quantization level to a new one
 * using correlated randomness. It operates only for parties 0, 1, and 2 (_party <= 2).
 *
 * The conversion process:
 * 1. Looks up a precomputed table for the given channel and quantization parameters.
 * 2. If a precomputed table exists, uses it to generate correlated randomness efficiently.
 *    Otherwise, falls back to online preparation via Unitv_prep_channel().
 * 3. Reconstructs the masked value s = r - x over the specified channel.
 * 4. Constructs the answer by shifting the correlated randomness z by the reconstructed value.
 *
 * @param x       Input shared array with old quantization level.
 * @param new_q   The target quantization level for the output.
 * @param channel The communication channel index to use for reconstruction and preparation.
 *
 * @return A new share_array representing x in the new quantization level (new_q),
 *         or NULL if _party > 2.
 *
 * @note The caller is responsible for freeing the returned share_array.
 * @note If no precomputed table is found and warn_precomp option is enabled,
 *       a warning message is printed to stdout.
 */
share_array Unitv2_channel(share_array x, share_t new_q, int channel) {
    if (_party > 2)
        return NULL;
    
    int n = x->n, old_q = x->q;

    // Prepare correlated randomness
    unitv_correlated_randomness cr;
    UV_tables tbl = uv_tbl_list_search2(PRE_UV_tbl[channel], x->n, old_q, new_q);//printf("search ok\n");fflush(stdout);
    cr = NULL;
    if (tbl != NULL) {  // When precomputed table matching conditions exists
      unitv_new_precomp(tbl, x->n, old_q, new_q, &cr);
    } else {
      if (_opt.warn_precomp) {
        printf("Unitv_channel3: without UV_table n = %d old_q = %d new_q = %d\n", x->n, x->q, new_q);fflush(stdout);
      }
      cr = Unitv_prep_channel(x->n, x->q, new_q, channel);
    }

    // Compute answer
    share_array ans = share_const(n*old_q, 0, new_q);
    share_array s = vsub(cr->r, x);
    share_array s_reconstructed = share_reconstruct_channel(s, channel);
    pa_iter itr = pa_iter_new(ans->A);
    share_t *cr_tmp = pa_unpack(cr->z->A);
    for (int i = 0; i < x->n; ++i) {
      share_t m = pa_get(s_reconstructed->A, i);
      _ tmp = share_const(old_q, 0, new_q);
      _setshares(ans, i*old_q, (i+1)*old_q, tmp, 0);
      _free(tmp);
      //share_t *cr_tmp = pa_unpack(cr->z->A);
      for (int j = 0; j < old_q; ++j) {
        if (cr_tmp[old_q*i+(j+m) % old_q] != cr->z_raw[old_q*i+(j+m) % old_q]) {
          printf("%d %d\n", cr_tmp[old_q*i+(j+m) % old_q], cr->z_raw[old_q*i+(j+m) % old_q]); // debug code for debugging?
        }
        pa_iter_set(itr, cr_tmp[old_q*i+(j+m) % old_q]);
      }
      //free(cr_tmp);
    }
    free(cr_tmp);
    pa_iter_flush(itr);
    _free(s);
    _free(s_reconstructed);
    unitv_cr_free(cr);

    return ans;
}
#define Unitv2(x, new_q) Unitv2_channel(x, new_q, 0)

////////////////////////////////////////////////////////////////
// Compute m unit vectors together
////////////////////////////////////////////////////////////////
/**
 * @brief Computes batched Unit Vector (Unitv2) conversion for multiple shared arrays using a specified channel.
 *
 * This function performs a batched conversion of shared arrays from their original moduli to a new modulus,
 * using correlated randomness and optional prefix sum accumulation.
 * Only parties with index <= 2 are supported.
 *
 * @param m         Number of input shared arrays to process.
 * @param size      Array of output sizes (per element) for each input shared array.
 * @param x_        Array of input shared arrays to be converted.
 * @param new_q     The new modulus to convert the shared values into.
 * @param cum       If set to 1, applies prefix sum (PrefixSum) to each unit vector segment;
 *                  otherwise, copies segments directly.
 * @param channel   The communication channel index to use for reconstruction and preprocessing.
 *
 * @return          A share_array containing the batched converted result with modulus new_q,
 *                  or NULL if the party index is greater than 2.
 *
 * @note  Precomputed UV tables (PRE_UV_tbl) are used when available to optimize correlated
 *        randomness generation. A warning is printed if no matching precomputed table is found
 *        and _opt.warn_precomp is set.
 * @note  The caller is responsible for freeing the returned share_array.
 */
share_array batched_Unitv2_channel(int m, int *size, share_array *x_, share_t new_q, int cum, int channel) {
    if (_party > 2)
        return NULL;

    share_t max_q = 0;
    for (int j = 0; j < m; j++) {
        if (x_[j]->q > max_q) {
            max_q = x_[j]->q;
        }
    }

    int s_length = 0, total_elem = 0, total_length = 0;
    for (int j = 0; j < m; j++) {
        _ x = x_[j];
        s_length += x->q * x->n;
        total_elem += x->n;
        total_length += size[j] * x->n;
    }

    unitv_correlated_randomness *cr;
    NEWA(cr, unitv_correlated_randomness, m);

    int pos;
    _ s_all = _const(s_length, 0, max_q);
    pos = 0;
    for (int j = 0; j < m; j++) {
        _ x = x_[j];
        int n = x->n, old_q = x->q;

        // Prepare correlated randomness
        UV_tables tbl = uv_tbl_list_search2(PRE_UV_tbl[channel], x->n, old_q, new_q);
        if (tbl != NULL) {  // When precomputed table matching conditions exists
          unitv_new_precomp(tbl, x->n, old_q, new_q, &cr[j]);
        } else {
          if (_opt.warn_precomp) {
            printf("Unitv_channel2: without UV_table n = %d old_q = %d new_q = %d\n", x->n, old_q, new_q);fflush(stdout);
          }
          cr[j] = Unitv_prep_channel(x->n, old_q, new_q, channel);
        }
        share_array s = vsub(cr[j]->r, x);
        for (int i = 0; i < s->n; ++i) {
            share_setraw(s_all, pos + i, share_getraw(s, i));
        }
        _free(s);
        pos += n;
    }

    _ s_reconstructed = share_reconstruct_channel(s_all, channel);
    _free(s_all);
    _ ans = _const(total_length, 0, new_q);

    pos = 0;
    int x_pos = 0;
    for (int j = 0; j < m; j++) {
        _ x = x_[j];
        int n = x->n, old_q = x->q;
        // Compute answer
        share_array ans_tmp = share_const(n*old_q, 0, new_q);
        pa_iter itr = pa_iter_new(ans_tmp->A);
        share_t *cr_tmp = pa_unpack(cr[j]->z->A);
        for (int i = 0; i < x->n; ++i) {
            share_t m = pa_get(s_reconstructed->A, x_pos++);
            m = m % old_q;
            //share_t *cr_tmp = pa_unpack(cr[j]->z->A);
            for (int jj = 0; jj < old_q; ++jj) {
                //if (cr_tmp[old_q*i+(jj+m) % old_q] != cr[j]->z_raw[old_q*i+(jj+m) % old_q]) {
                //    printf("%d %d\n", cr_tmp[old_q*i+(jj+m) % old_q], cr[j]->z_raw[old_q*i+(jj+m) % old_q]); // debug code for debugging?
                //}
                pa_iter_set(itr, cr_tmp[old_q*i+(jj+m) % old_q]);
            }
            //free(cr_tmp);
        }
        free(cr_tmp);
        pa_iter_flush(itr);
        if (cum == 1) {
            for (int i = 0; i < x->n; ++i) {
                _ ans_tmp2 = _slice(ans_tmp, i*old_q, i*old_q + size[j]);
                _ ans_tmp3 = PrefixSum(ans_tmp2);
                printf("i %d ", i); _print(ans_tmp3);
                _setshares(ans, pos, pos + size[j], ans_tmp3, 0);
                _free(ans_tmp2);
                _free(ans_tmp3);
                pos += size[j];
            }
        } else {
            for (int i = 0; i < x->n; ++i) {
                _setshares(ans, pos, pos + size[j], ans_tmp, i*old_q);
                pos += size[j];
            }
        }
        unitv_cr_free(cr[j]);
        _free(ans_tmp);
    }
    _free(s_reconstructed);
    free(cr);

    return ans;
}
#define batched_Unitv2(m, size, x, new_q) batched_Unitv2_channel(m, size, x, new_q, 0, 0)
#define batched_Unitv2_cum(m, size, x, new_q) batched_Unitv2_channel(m, size, x, new_q, 1, 0)

////////////////////////////////////////////////////////
// Protocol 2: UnitvB
////////////////////////////////////////////////////////
/**
 * @brief Computes Unit vector shares for multiple secret-shared binary inputs using a specific communication channel.
 *
 * This function converts each binary secret-shared input `x[i]` of bit-length `l` into a
 * secret-shared unit vector of length `2^l` over modulus `new_q`, using correlated randomness
 * prepared for the specified channel.
 *
 * For each input value `p`, the returned array `ans[i]` satisfies:
 * - `ans[i][p] = 1`
 * - all other entries are `0`
 * in secret-shared form over `new_q`.
 *
 * @param n Number of input values.
 * @param x Array of `n` secret-shared inputs. Each `x[i]` must have order 2 and the same length.
 * @param new_q Modulus/order of the output shares.
 * @param channel Channel identifier used for preprocessing and reconstruction.
 *
 * @return An array of `n` secret-shared unit vectors, where each vector has length `2^l`,
 *         or `NULL` if the current party index is greater than 2.
 *
 * @note This function requires `order(x[0]) == 2`. If this condition is not met,
 *       the function prints an error message and terminates the process.
 *
 * @warning The caller is responsible for freeing the returned array and each contained `share_array`.
 */
share_array* Unitvb_channel(int n, share_array *x, share_t new_q, int channel) {
    if (_party > 2)
        return NULL;

    if (order(x[0]) != 2) {
        printf("Unitvb order(x[0]) = %d\n", order(x[0]));
        exit(1);
    }

    int l = len(x[0]);
    
    // Generate correlated randomness
    unitvb_correlated_randomness cr = Unitvb_prep_channel(n, l, new_q, channel);

    // Compute answer
    share_array serialized_p = share_const(n * l, 0, 2);
    // NEWA(p, share_array, n);
    for (int i = 0; i < n; ++i) {
        share_setshares(serialized_p, i * l, (i + 1) * l, x[i], 0);
    }
    vadd_(serialized_p, cr->r);
    share_array serialized_p_re = _reconstruct_channel(serialized_p, channel);

    share_array *ans;
    NEWA(ans, share_array, n);
    for (int i = 0; i < n; ++i) {
        ans[i] = share_const(1<<l, 0, new_q);
        share_array p_re = _slice(serialized_p_re, i * l, (i + 1) * l);
        share_t p = 0;
        for (int j = 0; j < l; ++j) {
            p <<= 1;
            p += share_getraw(p_re, l - 1 - j);
        }
        for (int j = 0; j < 1<<l; ++j) {
            share_t id = j ^ p;
            share_t x = share_getraw(cr->z, i * (1<<l) + id);
            share_setraw(ans[i], j, x);
        }

        share_free(p_re);
    }

    unitvb_cr_free(cr);
    share_free(serialized_p);
    share_free(serialized_p_re);

    return ans;
}

////////////////////////////////////////////////////////
// UnitvB with input converted to bits
////////////////////////////////////////////////////////
/**
 * @brief Computes a batched unit-vector representation of secret-shared 2-bit ordered values on a specified channel.
 *
 * This function converts each input value in @p x into a one-hot (unit-vector) encoding over
 * a domain of size `2^l`, where `l = depth_bits(x)`. The computation is performed using
 * correlated randomness prepared for the given communication @p channel and reconstructs
 * masked intermediate values in order to select the appropriate rotated entry from the
 * precomputed table.
 *
 * The input is expected to have order 2. If the current number of parties exceeds 2, the
 * function returns `NULL`. If the order of @p x is not 2, the function reports the error
 * and terminates the process.
 *
 * For each element `x_i`, the result contains a block of length `m = 2^l`, where exactly one
 * position corresponds to the encoded value derived from `x_i` under the protocol.
 *
 * @param x        Input secret-shared bit representation. Must have order 2.
 * @param new_q    Modulus/order of the output shares.
 * @param channel  Channel identifier used for correlated randomness generation and reconstruction.
 *
 * @return A newly allocated share array of length `len_bits(x) * (1 << depth_bits(x))`
 *         containing the unit-vector encoding, or `NULL` if the protocol is not supported
 *         for the current number of parties.
 *
 * @note The caller is responsible for freeing the returned share object.
 * @note This function internally allocates temporary buffers and correlated randomness state.
 */
_ UnitvB_channel(_bits x, share_t new_q, int channel) 
{
  if (_party > 2) return NULL;

  if (order_bits(x) != 2) {
    printf("UnitvB order(x[0]) = %d\n", order_bits(x));
    exit(1);
  }

  int n = len_bits(x);
  int l = depth_bits(x);
  int m = 1<<l;
    
  // Generate correlated randomness
  unitvb_correlated_randomness cr = Unitvb_prep_channel(n, l, new_q, channel);

  // Compute answer
  _ serialized_p = share_const(n * l, 0, 2);
  // NEWA(p, share_array, n);
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < l; j++) {
      share_setshare(serialized_p, i * l + j, x->a[j], i); // l consecutive bits represent one value x_i
    }
  }
  vadd_(serialized_p, cr->r);
  _ serialized_p_re = _reconstruct_channel(serialized_p, channel);

  _ ans = share_const(m*n, 0, new_q);

  for (int i = 0; i < n; ++i) {
    share_t p = 0;
    for (int j = 0; j < l; ++j) {
      p <<= 1;
      p += share_getraw(serialized_p_re, i * l+ (l - 1 - j));
    }
    for (int j = 0; j < m; ++j) {
      share_t id = j ^ p;
      share_t x = share_getraw(cr->z, i * m + id);
      share_setraw(ans, i * m + j, x);
    }
  }

  unitvb_cr_free(cr);
  share_free(serialized_p);
  share_free(serialized_p_re);

  return ans;
}



/**
 * @brief Performs batched (parallel) Unit-vector conversion for multiple shared inputs on a specific channel.
 *
 * This routine processes `l` secret-shared values `x[i]`, each with possibly different lengths/orders,
 * and converts them from `old_q[i] = order(x[i])` to `new_q[i]` using precomputed UV tables when available,
 * or freshly prepared correlated randomness otherwise.
 *
 * The protocol is two-party only (`_party <= 2`); party 0 locally reconstructs intermediate values,
 * while parties 1/2 exchange masked differences over `channel` (optionally via send queue).
 *
 * @param l       Number of input items in the batch.
 * @param x       Array of input shared arrays (length `l`).
 * @param new_q   Target order for each input item (length `l`).
 * @param channel Communication channel identifier used for send/recv and table lookup.
 *
 * @return
 * - On success: an array `ans` of length `l`, where each `ans[i]` is an array of `n[i]` shares
 *   (with `n[i] = len(x[i])`), each converted to order `new_q[i]`.
 * - If `_party > 2`: returns `NULL`.
 *
 * @note
 * - The caller owns the returned structure and must free all allocated shares/arrays.
 * - Uses precomputation cache `PRE_UV_tbl[channel]` when available.
 * - Emits a warning when precomputed tables are missing if `_opt.warn_precomp` is enabled.
 * - Assumes all communication peers follow the same protocol and batch dimensions.
 */
share_array** ParallelUnitv_channel(int l, share_array *x, int *new_q, int channel) {
    if (_party > 2)
        return NULL;
    
    int *n, *old_q;
    NEWA(n, int, l);
    NEWA(old_q, int, l);
    for (int i = 0; i < l; ++i) {
        n[i] = len(x[i]);
        old_q[i] = order(x[i]);
    }

    unitv_correlated_randomness *cr;
    UV_tables tbl;
    NEWA(cr, unitv_correlated_randomness, l);
    for (int i = 0; i < l; ++i) {
        tbl = uv_tbl_list_search2(PRE_UV_tbl[channel], n[i], old_q[i], new_q[i]);

        if (tbl != NULL) {
            unitv_new_precomp(tbl, n[i], old_q[i], new_q[i], cr + i);
        }
        else {
            if (_opt.warn_precomp) {
                printf("Unitv_channel4: without UV_table n = %d old_q = %d new_q = %d\n", x[i]->n, x[i]->q, new_q[i]);fflush(stdout);
            }
            cr[i] = Unitv_prep_channel(n[i], old_q[i], new_q[i], channel);
        }
    }

    share_array **ans;
    NEWA(ans, share_array*, l);
    share_array *s;
    share_array *s_r;
    NEWA(s, share_array, l);
    NEWA(s_r, share_array, l);

    // parallel reconstruct
    // TODO: Check
    if (_party == 0){
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            s_r[i] = share_const(len(s[i]), 0, order(s[i]));
            share_setshares(s_r[i], 0, len(s[i]), s[i], 0);
        }
    }
    else if (_party == 1 || _party == 2) {
      if (_opt.send_queue == 1) {
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            mpc_send_queue_channel(TO_PAIR, s[i]->A->B, pa_size(s[i]->A), channel);
        }
        mpc_send_flush_channel(TO_PAIR, channel);
      } else {
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            mpc_send_channel(TO_PAIR, s[i]->A->B, pa_size(s[i]->A), channel);
        }
      }
      for (int i = 0; i < l; ++i) {
        s_r[i] = share_const(len(s[i]), 0, order(s[i]));
        mpc_recv_channel(FROM_PAIR, s_r[i]->A->B, pa_size(s_r[i]->A), channel);
        vadd_(s_r[i], s[i]);
      }
    }
    
    for (int i = 0; i < l; ++i) {
        NEWA(ans[i], share_array, n[i]);
        for (int j = 0; j < n[i]; ++j) {
            share_t m = share_getraw(s_r[i], j);
            ans[i][j] = share_const(old_q[i], 0, new_q[i]); // Too many here
            pa_iter itr_ans = pa_iter_new(ans[i][j]->A);
            for (int k = 0; k < old_q[i]; ++k) {
                share_t a = share_getraw(cr[i]->z, old_q[i] * j + (k + m) % old_q[i]);
                pa_iter_set(itr_ans, a);
            }
            pa_iter_flush(itr_ans);
        }
        
        share_free(s[i]);
        share_free(s_r[i]);
        unitv_cr_free(cr[i]);
    }
    free(n);    free(cr);   free(s);    free(s_r);
    free(old_q);
    return ans;
}

/**
 * @brief Performs a batched/parallel Unit-v conversion on multiple shared arrays over a specific communication channel.
 *
 * This routine converts each input share vector `x[i]` from its current order/modulus setting
 * (`order(x[i])`) to `new_q[i]`, using either precomputed UV tables (when available) or freshly
 * prepared correlated randomness. Processing is done for `l` independent instances in one call.
 *
 * Protocol behavior depends on the executing party:
 * - Party `<= 0`: computes local reconstruction shares directly.
 * - Party `1` or `2`: exchanges masked differences with its pair peer (queued or immediate send,
 *   depending on runtime options), then reconstructs combined values.
 * - Party `> 2`: unsupported; returns `NULL`.
 *
 * @param l       Number of parallel instances (length of `x` and `new_q`).
 * @param x       Array of input shared vectors to be converted.
 * @param new_q   Target order/modulus per instance.
 * @param channel Communication channel identifier used for send/receive and table lookup.
 *
 * @return A newly allocated array of `share_array` of length `l`, where each element contains the
 *         converted output for the corresponding input instance; returns `NULL` when `_party > 2`.
 *
 * @note The caller owns the returned array and each contained `share_array`, and is responsible for freeing them.
 * @note Intermediate buffers and correlated randomness created inside this function are released before return.
 * @note May emit a warning when no precomputed UV table is found (depending on runtime options).
 */
share_array* ParallelUnitv2_channel(int l, share_array *x, int *new_q, int channel) {
    if (_party > 2)
        return NULL;
    
    int *n, *old_q;
    NEWA(n, int, l);
    NEWA(old_q, int, l);
    for (int i = 0; i < l; ++i) {
        n[i] = len(x[i]);
        old_q[i] = order(x[i]);
    }

    unitv_correlated_randomness *cr;
    UV_tables tbl;
    NEWA(cr, unitv_correlated_randomness, l);
    for (int i = 0; i < l; ++i) {
        tbl = uv_tbl_list_search2(PRE_UV_tbl[channel], n[i], old_q[i], new_q[i]);

        if (tbl != NULL) {
            unitv_new_precomp(tbl, n[i], old_q[i], new_q[i], cr + i);
        }
        else {
            if (_opt.warn_precomp) {
                printf("Unitv_channel5: without UV_table n = %d old_q = %d new_q = %d\n", x[i]->n, x[i]->q, new_q[i]);fflush(stdout);
            }
            cr[i] = Unitv_prep_channel(n[i], old_q[i], new_q[i], channel);
        }
    }

    share_array **ans;
    share_array *ans2;
    NEWA(ans2, share_array, l);
    share_array *s;
    share_array *s_r;
    NEWA(s, share_array, l);
    NEWA(s_r, share_array, l);

    // parallel reconstruct
    // TODO: Check
    if (_party <= 0){
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            s_r[i] = share_const(len(s[i]), 0, order(s[i]));
            share_setshares(s_r[i], 0, len(s[i]), s[i], 0);
        }
    }
    else if (_party == 1 || _party == 2) {
      if (_opt.send_queue) {
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            mpc_send_queue_channel(TO_PAIR, s[i]->A->B, pa_size(s[i]->A), channel);
        }
        mpc_send_flush_channel(TO_PAIR, channel);        
      } else {
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            mpc_send_channel(TO_PAIR, s[i]->A->B, pa_size(s[i]->A), channel);
        }
      }
      for (int i = 0; i < l; ++i) {
        s_r[i] = share_const(s[i]->n, 0, s[i]->q);
        mpc_recv_channel(FROM_PAIR, s_r[i]->A->B, pa_size(s_r[i]->A), channel);
        vadd_(s_r[i], s[i]);
      }
    }
    
    for (int i = 0; i < l; ++i) {
        ans2[i] = share_const(n[i]*old_q[i], 0, new_q[i]);
        pa_iter itr_ans2 = pa_iter_new(ans2[i]->A);
        for (int j = 0; j < n[i]; ++j) {
            share_t m = share_getraw(s_r[i], j);
            for (int k = 0; k < old_q[i]; ++k) {
                share_t a = share_getraw(cr[i]->z, old_q[i] * j + (k + m) % old_q[i]);
                pa_iter_set(itr_ans2, a);
            }
        }
        pa_iter_flush(itr_ans2);
        
        share_free(s[i]);
        share_free(s_r[i]);
        unitv_cr_free(cr[i]);
    }
    free(n);    free(cr);   free(s);    free(s_r);
    free(old_q);
    return ans2;
}

/**
 * @brief Computes parallel unit vector operations with channel-based communication.
 *
 * This function performs parallel unit vector transformations on an array of shared arrays,
 * utilizing correlated randomness and MPC (Multi-Party Computation) protocols.
 * Supports up to 3 parties (party index 0, 1, or 2).
 *
 * @param l        The number of shared arrays to process in parallel.
 * @param x        Pointer to an array of input shared arrays of length `l`.
 * @param new_q    Pointer to an integer array of length `l` specifying the new modulus
 *                 for each corresponding shared array.
 * @param channel  The communication channel index used for MPC send/receive operations.
 *
 * @return A `share_array` of size `l * n[0]` containing the concatenated unit vector
 *         results with modulus `new_q[0]`, or `NULL` if `_party > 2`.
 *
 * @note
 * - Party 0 performs local reconstruction by zeroing shares and copying.
 * - Parties 1 and 2 perform mutual reconstruction via send/receive over the specified channel.
 * - If a precomputed UV table is found via `uv_tbl_list_search2`, it is used for efficiency;
 *   otherwise, `Unitv_prep_channel` is called as a fallback.
 * - If `_opt.warn_precomp` is set, a warning is printed when no precomputed table is found.
 * - If `_opt.send_queue` is set, queued sending is used for batched communication.
 * - Memory allocated internally is freed before returning.
 *
 * @warning Assumes all input shared arrays in `x` have the same length `n[0]`.
 */
share_array ParallelUnitv3_channel(int l, share_array *x, int *new_q, int channel) {
    if (_party > 2)
        return NULL;
    
    int *n, *old_q;
    NEWA(n, int, l);
    NEWA(old_q, int, l);
    for (int i = 0; i < l; ++i) {
        n[i] = len(x[i]);
        old_q[i] = order(x[i]);
    }

    unitv_correlated_randomness *cr;
    UV_tables tbl;
    NEWA(cr, unitv_correlated_randomness, l);
    for (int i = 0; i < l; ++i) {
        tbl = uv_tbl_list_search2(PRE_UV_tbl[channel], n[i], old_q[i], new_q[i]);

        if (tbl != NULL) {
            unitv_new_precomp(tbl, n[i], old_q[i], new_q[i], cr + i);
        }
        else {
            if (_opt.warn_precomp) {
                printf("Unitv_channel6: without UV_table n = %d old_q = %d new_q = %d\n", x[i]->n, x[i]->q, new_q[i]);fflush(stdout);
            }
            cr[i] = Unitv_prep_channel(n[i], old_q[i], new_q[i], channel);
        }
    }

    share_array ans3;
    share_array *s;
    share_array *s_r;
    NEWA(s, share_array, l);
    NEWA(s_r, share_array, l);

    // parallel reconstruct
    // TODO: Check
    if (_party == 0){
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            s_r[i] = share_const(len(s[i]), 0, order(s[i]));
            share_setshares(s_r[i], 0, len(s[i]), s[i], 0);
        }
    }
    else if (_party == 1 || _party == 2) {
      if (_opt.send_queue) {
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            mpc_send_queue_channel(TO_PAIR, s[i]->A->B, pa_size(s[i]->A), channel);
        }
        mpc_send_flush_channel(TO_PAIR, channel);
      } else {
        for (int i = 0; i < l; ++i) {
            s[i] = vsub(cr[i]->r, x[i]);
            mpc_send_channel(TO_PAIR, s[i]->A->B, pa_size(s[i]->A), channel);
        }
      }
      for (int i = 0; i < l; ++i) {
        s_r[i] = share_const(s[i]->n, 0, s[i]->q);
        mpc_recv_channel(FROM_PAIR, s_r[i]->A->B, pa_size(s_r[i]->A), channel);
        vadd_(s_r[i], s[i]);
      }
    }
    
    ans3 = share_const(l*n[0], 0, new_q[0]);
    pa_iter itr_ans3 = pa_iter_new(ans3->A);
    for (int i = 0; i < l; ++i) {
        for (int j = 0; j < n[i]; ++j) {
            share_t m = share_getraw(s_r[i], j);
            for (int k = 0; k < old_q[i]*0+1; ++k) {
                share_t a = share_getraw(cr[i]->z, old_q[i] * j + (k + m) % old_q[i]);
                pa_iter_set(itr_ans3, share_getraw(cr[i]->z, old_q[i] * j + (k + m) % old_q[i]));
            }
        }
        
        share_free(s[i]);
        share_free(s_r[i]);
        unitv_cr_free(cr[i]);
    }
    pa_iter_flush(itr_ans3);
    free(n);    free(cr);   free(s);    free(s_r);
    free(old_q);
    return ans3;
}

typedef struct Partition {
    int l;  // the input (share_array) bit-length
    int m;  // the length of del
    int *del;   // sum of del[i] is l
}* Partition;

void FreePartition(Partition par) {
    free(par->del);
    free(par);
}

//////////////////////////////////////////////////////////
// Divide binary representation of x according to p
//////////////////////////////////////////////////////////
share_t* Expand(share_t x, share_t q, Partition p) {
    if (blog(q - 1) + 1 != p->l) {
        printf("Expand q: %d p->l: %d\n", q, p->l);
        exit(1);
    }
    if (1 << (blog(q - 1) + 1) != q) {
        printf("Expand q: %d\n", q);
        exit(1);
    }

    share_t *ans;
    NEWA(ans, share_t, p->m);
    int s = 0;
    for (int i = 0; i < p->m; ++i) {
        int mask = (1<<p->del[i]) - 1;
        ans[i] = (x >> s) & mask;
        
        s += p->del[i];
    }

    return ans;
}

// Divide every 3 bits
Partition MakeOverflowPatition1(int l) {
    NEWT(Partition, par);
    par->l = l;
    par->m = (l + 3 - 1) / 3;   // Round up by 3
    NEWA(par->del, int, par->m);
    for (int i = 0; i < par->m; ++i) {
        if (i < par->m - 1) {
            par->del[i] = 3;
        }
        else {
            if (l % 3 == 0) {
                par->del[i] = 3;
            }
            else {
                par->del[i] = l % 3;
            }
        }
    }

    return par;
}

Partition MakeOverflowPatition2(int m) {
    NEWT(Partition, T);
    T->m = m - 1;
    T->l = m;
    NEWA(T->del, int, T->m);
    for (int i = 0; i < T->m - 1; ++i) {
        T->del[i] = 1;
    }
    T->del[T->m - 1] = 2;

    return T;
}

Partition MakeOverflowPatition3(int m) {
    NEWT(Partition, T);
    T->m = m;
    T->l = m;
    NEWA(T->del, int, T->m);
    for (int i = 0; i < T->m; ++i) {
        T->del[i] = 1;
    }
    //T->del[T->m - 1] = 2;

    return T;
}

int CalcParamInOverflow2(Partition T) {
    int N = T->m + 2; // Original digit count + 1
    N = 1 << (blog(N - 1) + 1); // Smallest power of 2 >= N
    return N;
}

int CalcParamInOverflow3(Partition T) {
    //printf("CalcParamInOverflow3\n");
    int N = T->m + 2; // Appropriate
    //int N = 64; // Appropriate
    N = 1 << (blog(N - 1) + 1); // Smallest power of 2 >= N
    return N;
}

////////////////////////////////////////////////////////
// Protocol 5: Overflow1
////////////////////////////////////////////////////////
/**
 * @brief Computes overflow-related indicator shares for each element using a unit-vector conversion on a doubled modulus.
 *
 * This routine expects the input array modulus (`old_q`) to be a power of two.
 * It first embeds `x` into a temporary array with modulus `2 * old_q`, then calls
 * `Unitv_channel(...)` to obtain per-element unit-vector shares under `new_q`.
 *
 * From each resulting vector:
 * - `b[i]` is set from index `old_q - 1` (single extracted component),
 * - `c[i]` is accumulated as the sum of components in the range
 *   `[old_q, 2 * old_q)`.
 *
 * The returned pair `(b, c)` can be used as overflow/carry-related signals for
 * subsequent secure computation steps.
 *
 * @param x        Input share array.
 * @param new_q    Target modulus for the output share arrays.
 * @param channel  Communication channel identifier passed to `Unitv_channel`.
 * @return         A `share_pair` where:
 *                 - `first` (`b`) contains extracted indicator components,
 *                 - `second` (`c`) contains accumulated upper-half components.
 *
 * @note Terminates the process if `order(x)` is not a power of two.
 * @warning This function currently emits debug prints for intermediate values.
 */
share_pair OverflowConst1_channel(share_array x, share_t new_q, int channel) {
    share_t old_q = order(x);
    int n = len(x);
    if (old_q != 1 << (blog(old_q - 1) + 1)) {
        printf("OverflowConst1_channel: old_q = %d\n", old_q);
        exit(1);
    }

    share_array z = share_const(n, 0, 2 * old_q);
    for (int i = 0; i < n; ++i) {
        share_setraw(z, i, share_getraw(x, i));
    }

    share_array *v = Unitv_channel(z, new_q, channel);
    printf("z: ");  share_print(z);
    share_free(z);
    share_array b = share_const(n, 0, new_q);
    share_array c = share_const(n, 0, new_q);
    for (int i = 0; i < n; ++i) {
        share_t r = share_getraw(v[i], old_q - 1);
        share_setraw(b, i, r);
        for (int j = old_q; j < 2 * old_q; ++j) { // This one is correct
            share_addshare(c, i, v[i], j);
        }
        share_free(v[i]);
    }

    free(v);
    printf("c: ");  share_print(c);
    
    return (share_pair){b, c};
}

/**
 * @brief Computes overflow decomposition terms for multiple channels in parallel.
 *
 * For each input share array `x[i]` (with modulus/order `old_q[i]`), this routine:
 * 1. Verifies `old_q[i]` is a power of two.
 * 2. Lifts each input into modulus `2 * old_q[i]`.
 * 3. Runs `ParallelUnitv2_channel()` on the lifted inputs.
 * 4. Extracts two output share arrays per input:
 *    - `ans[i].x` (`b[i]`): bit/value at position `old_q[i] - 1` for each element.
 *    - `ans[i].y` (`c[i]`): sum of positions `[old_q[i], 2*old_q[i)-1]` for each element.
 *
 * The resulting arrays are allocated with modulus `new_q[i]`.
 *
 * @param m
 *   Number of share arrays (channels).
 * @param x
 *   Input array of `m` share arrays.
 * @param new_q
 *   Per-channel output modulus/order; length must be `m`.
 * @param channel
 *   Channel identifier forwarded to `ParallelUnitv2_channel()`.
 *
 * @return
 *   Pointer to an allocated array of `m` `share_pair` values.
 *   For each `i`, `ans[i].x` and `ans[i].y` are newly allocated share arrays.
 *
 * @note
 *   This function terminates the process (`exit(1)`) if any input order is not a power of two.
 *
 * @warning
 *   The caller owns the returned `share_pair` array and all nested share arrays,
 *   and must free them appropriately to avoid memory leaks.
 */
share_pair* ParallelOverflowConst1_channel(int m, share_array *x, share_t *new_q, int channel) {
    share_t *old_q;
    int *n;
    NEWA(old_q, share_t, m);
    NEWA(n, int, m);
    for (int i = 0; i < m; ++i) {
        old_q[i] = order(x[i]);
        n[i] = len(x[i]);
        if (old_q[i] != 1<< (blog(old_q[i] - 1) + 1)) {
            printf("ParallelOverflowConst1_channel: old_q[%d] = %d\n", i, old_q[i]);
            exit(1);
        }
    }

    share_array *z = NULL;
    NEWA(z, share_array, m);
    z[0] = NULL; // Avoid compiler warning about uninitialized variable
    for (int i = 0; i < m; ++i) {
        z[i] = share_const(n[i], 0, 2 * old_q[i]);
        for (int j = 0; j < n[i]; ++j) {
            share_setraw(z[i], j, share_getraw(x[i], j));
        }
    }

    share_t Z = order(z[0]);
    share_array *v2 = ParallelUnitv2_channel(m, z, new_q, channel);
    for (int i = 0; i < m; ++i) {
        share_free(z[i]);
    }
    free(z);

    share_array *b, *c;
    NEWA(b, share_array, m);
    NEWA(c, share_array, m);
    for (int i = 0; i < m; ++i) {
        b[i] = share_const(n[i], 0, new_q[i]);
        c[i] = share_const(n[i], 0, new_q[i]);
        for (int j = 0; j < n[i]; ++j) {
            share_t r = share_getraw(v2[i], Z * j + old_q[i] - 1);
            share_setraw(b[i], j, r);
            for (int k = old_q[i]; k < 2 * old_q[i]; ++k) {
                share_addshare(c[i], j, v2[i], Z*j + k);
            }
        }
        _free(v2[i]);
    }

    free(n);
    free(old_q);
    free(v2);

    share_pair *ans;
    NEWA(ans, share_pair, m);
    for (int i = 0; i < m; ++i) {
        ans[i].x = b[i];
        ans[i].y = c[i];
    }

    free(b);    free(c);

    return ans;
}


////////////////////////////////////////////////////////
// Protocol 6: Overflow
// Output modulus is 2
////////////////////////////////////////////////////////
/**
 * @brief Computes an overflow-detection / unit-value aggregation over a shared array using channel-aware parallel primitives.
 *
 * This routine decomposes each element of @p y into bit partitions, applies
 * parallel overflow processing on the lower-level partitions, reconstructs
 * grouped intermediate values, and then evaluates them with a parallel unit-value
 * procedure. The final result is a binary shared array whose entries indicate
 * whether the corresponding input element satisfies the overflow-related condition
 * represented by this construction.
 *
 * Internally, the function:
 * - derives a bit-length from the modulus of @p y,
 * - builds hierarchical partitions,
 * - expands each shared value into partitioned components,
 * - runs @c ParallelOverflowConst1_channel() on the first partition level,
 * - reconstructs grouped values for the second partition level,
 * - runs @c ParallelUnitv2_channel() on those grouped values,
 * - accumulates the resulting indicators into a single output share array.
 *
 * @param y
 *   Input shared array. Each element is processed independently under the same
 *   modulus/order carried by the array.
 * @param channel
 *   Communication or execution channel identifier passed through to the
 *   channel-aware parallel subroutines.
 *
 * @return
 *   A shared array of length @c len(y) with modulus 2, containing the final
 *   per-element overflow/unit indicator.
 *
 * @note
 * The implementation assumes the partitioning and intermediate modulus
 * computations are compatible with the order of @p y.
 *
 * @warning
 * A TODO in the implementation indicates that for small bit-lengths this routine
 * should eventually dispatch to @c OverflowConst1_channel() instead.
 */
share_array OverflowConst2_channel(share_array y, int channel) {
    // int l = order(y);
    int n = len(y);
    share_t old_q = order(y);
    int l = blog(old_q - 1) + 1;    // TODO: Add change to activate OverflowConst1_channel when l is short.
    Partition par = MakeOverflowPatition3(l);
    Partition T = MakeOverflowPatition3(par->m);

    share_t **partitioned_y_raw;
    NEWA(partitioned_y_raw, share_t*, n);
    for (int i = 0; i < n; ++i) {
        partitioned_y_raw[i] = Expand(share_getraw(y, i), old_q, par);
    }

    share_t N = CalcParamInOverflow3(T);
    share_t *mid_q;
    NEWA(mid_q, share_t, par->m);
    for (int i = 0; i < par->m; ++i) {
        mid_q[i] = N;
    }

    share_array *partitioned_y;
    NEWA(partitioned_y, share_array, par->m);
    for (int i = 0; i < par->m; ++i) {
        partitioned_y[i] = share_const(n, 0, 1<<par->del[i]);
        for (int j = 0; j < n; ++j) {
            share_setraw(partitioned_y[i], j, partitioned_y_raw[j][i]);
        }
    }
    for (int i = 0; i < n; ++i) {
        free(partitioned_y_raw[i]);
    }
    free(partitioned_y_raw);

    share_pair *pr = ParallelOverflowConst1_channel(par->m, partitioned_y, mid_q, channel);

    for (int i = 0; i < par->m; ++i) {
        share_free(partitioned_y[i]);
    }
    free(partitioned_y);
    free(mid_q);

    share_array *z;
    NEWA(z, share_array, T->m);
    int *t;
    NEWA(t, int, T->m);
    NEWA(mid_q, share_t, T->m);
    t[0] = 0;
    for (int k = 0; k < T->m; ++k) {
        mid_q[k] = 2;
        if (k == 0) {
            t[k] = T->del[k] - 1;
        }
        else {
            t[k] = T->del[k] + t[k-1];
        }
        z[k] = share_const(n, 0, N);
        for (int i = 0; i < n; ++i) {
            share_t r = 0;
            for (int j = 0; j < T->del[k]; ++j) {
                if (t[k] - j + 1 >= par->m) {
                    r = (r + (1 << (T->del[k] - 1 - j)) * share_getraw(pr[t[k] - j].y, i)) % N;
                }
                else {
                    r = (r + (1 << (T->del[k] - 1 - j)) * (share_getraw(pr[t[k] - j + 1].x, i) + share_getraw(pr[t[k] - j].y, i))) % N;
                }
            }
            for (int j = t[k]+2; j < par->m; ++j) {
                r = (r + (1 << (T->del[k] - 1)) * share_getraw(pr[j].x, i)) % N;
            }
            share_setraw(z[k], i, r);
        }
    }

    share_array *f2 = ParallelUnitv2_channel(T->m, z, mid_q, channel);
    for (int k = 0; k < T->m; ++k) {
        share_free(z[k]);
    }
    free(z);
    free(mid_q);

    share_array *g, of = share_const(n, 0, 2);
    NEWA(g, share_array, T->m);
    for (int k = 0; k < T->m; ++k) {
        g[k] = share_const(n, 0, 2);
        pa_iter itr_g = pa_iter_new(g[k]->A);
        share_t q = order(g[k]);
        int js = (1 << (T->del[k] - 1)) * (par->m - t[k]);
        for (int i = 0; i < n; ++i) {
          pa_iter itr_f = pa_iter_pos_new(f2[k]->A, i*N + js);
          share_t z = 0;
          for (int j = js; j < N; ++j) {
            z += pa_iter_get(itr_f);
          }
          pa_iter_set(itr_g, z % q);
          pa_iter_free(itr_f);
        }
        pa_iter_flush(itr_g);
    }

    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < T->m; ++k) {
            share_addshare(of, i, g[k], i);
        }
    }

    for (int k = 0; k < par->m; ++k) {
        share_free(pr[k].x);
        share_free(pr[k].y);
    }

    for (int k = 0; k < T->m; ++k) {
        share_free(g[k]);
        _free(f2[k]);
    }
    free(pr);
    FreePartition(par);
    FreePartition(T);
    free(t);
    free(f2);
    free(g);

    return of;
}

// TODO: Implementation of function to handle multiple share arrays with large input length in bulk
//      Used for Comparison2 implementation

// Divide every 1 bit
Partition MakeEqualityPatition(int l) {
    NEWT(Partition, par);
    par->l = l;
    par->m = l - 1;
    NEWA(par->del, int, par->m);
    for (int i = 0; i < par->m - 1; ++i) {
        par->del[i] = 1;
    }
    par->del[par->m - 1] = 2;

    return par;
}

Partition MakeEqualityPatition3(int l) {
    NEWT(Partition, par);
    par->l = l;
    par->m = l;
    NEWA(par->del, int, par->m);
    for (int i = 0; i < par->m; ++i) {
        par->del[i] = 1;
    }
    //par->del[par->m - 1] = 2;

    return par;
}

void Partition_free(Partition par)
{
  free(par->del);
  free(par);
}

////////////////////////////////////////////////////////
// Protocol: Equality1
////////////////////////////////////////////////////////
/**
 * @brief Computes element-wise equality check between two shared arrays using unit vectors.
 *
 * For each position i, computes whether x[i] == y[i] by computing the unit vector
 * of their difference (x[i] - y[i]) and extracting the zero-position component.
 *
 * @param x         First input shared array.
 * @param y         Second input shared array. Must have the same length and order as x.
 * @param new_q     The new modulus (order) for the output shared array.
 * @param channel   The communication channel to use for the unit vector computation.
 *
 * @return A shared array of length n where each element is 1 if x[i] == y[i], 0 otherwise,
 *         under modulus new_q. Returns NULL if _party > 2.
 *
 * @note The order of x and y must be a power of 2.
 * @note Exits with an error if:
 *       - x and y have different lengths.
 *       - x and y have different orders.
 *       - The order of x is not a power of 2.
 */
share_array EqualityConst1_channel(share_array x, share_array y, share_t new_q, int channel) {
    if (_party > 2) {
        return NULL;
    }
    if (len(x) != len(y)) {
        printf("EqualityConst_channel len(x): %d    len(y): %d\n", len(x), len(y));
        exit(1);
    }
    if (order(x) != order(y)) {
        printf("EqualityConst_channel order(x): %d    order(y): %d\n", order(x), order(y));
        exit(1);
    }
    if (order(x) != 1<<(blog(order(x) - 1) + 1)) {
        printf("EqualityConst_channel order(x): %d\n", order(x));
        exit(1);
    }

    int n = len(x);
    share_t old_q = order(x);
    share_array d = vsub(x, y);
    share_array *e = Unitv_channel(d, new_q, channel);
    share_array ans = share_const(n, 0, new_q);
    for (int i = 0; i < n; ++i) {
        share_setshare(ans, i, e[i], 0);
        share_free(e[i]);
    }

    share_free(d);
    free(e);

    return ans;
}

/**
 * @brief Computes element-wise equality shares for two arrays of secret-shared vectors over a channel.
 *
 * For each index `i` in `[0, m)`, this function computes `d[i] = x[i] - y[i]`, then applies
 * `ParallelUnitv3_channel` to obtain unit-vector-style equality indicators, and finally packs the
 * corresponding slice back into `ans[i]`. Each `ans[i][j]` is set to the computed share indicating
 * whether the corresponding elements of `x[i]` and `y[i]` are equal.
 *
 * @param m        Number of vectors in `x` and `y`.
 * @param x        Input array of secret-shared vectors.
 * @param y        Input array of secret-shared vectors (same shape as `x`).
 * @param new_q    Per-vector modulus/context array used for creating output shares.
 * @param channel  Communication channel identifier used by parallel secure subroutines.
 *
 * @return Newly allocated array of `m` secret-shared vectors containing equality results.
 *         Caller is responsible for freeing the returned array and each contained share object.
 *
 * @note Assumes `x` and `y` have compatible lengths for each index `i`.
 * @note Intermediate temporaries are released internally.
 */
share_array *ParallelEqualityConst1_channel(int m, share_array *x, share_array *y, share_t *new_q, int channel) {
    share_array *d;
    NEWA(d, share_array, m);
    for (int i = 0; i < m; ++i) {
        d[i] = vsub(x[i], y[i]);
    }

    share_array e3 = ParallelUnitv3_channel(m, d, new_q, channel);
    share_array *ans;
    NEWA(ans, share_array, m);
    int idx = 0;
    for (int i = 0; i < m; ++i) {
        _free(d[i]);
        ans[i] = share_const(len(x[i]), 0, new_q[i]);
        for (int j = 0; j < len(x[i]); ++j) {
            share_setshare(ans[i], j, e3, i*len(x[i])+j);
        }
    }

    _free(e3);
    free(d);

    return ans;
}

int CalcParamInEquality(Partition par) {
    int N = par->m + 1;
    N = 1 << (blog(N - 1) + 1);
    return N;
}

////////////////////////////////////////////////////////
// Protocol 14: Equality
////////////////////////////////////////////////////////
/**
 * @brief Computes element-wise equality comparison between two shared arrays using channel-based communication.
 *
 * This function compares two shared arrays `x` and `y` element-wise and returns a new shared array
 * where each element indicates whether the corresponding elements of `x` and `y` are equal.
 * The result is encoded in a new modular ring of order `new_q`.
 *
 * The function uses a partitioned equality protocol based on bit decomposition and unit vector
 * computation to securely evaluate equality in a multi-party computation setting.
 *
 * @param x         The first input shared array.
 * @param y         The second input shared array. Must have the same length and order as `x`.
 * @param new_q     The modulus (order) of the output shared array.
 * @param channel   The communication channel identifier used for MPC protocol messages.
 *
 * @return A shared array of length `len(x)` with modulus `new_q`, where each element is:
 *         - 1 if the corresponding elements of `x` and `y` are equal,
 *         - 0 otherwise.
 *
 * @pre `len(x)` must equal `len(y)`.
 * @pre `order(x)` must equal `order(y)`.
 * @pre `order(x)` must be a power of 2.
 *
 * @note When `_party == -1`, the function operates in plaintext mode and directly compares values.
 * @note Party roles (0, 1, 2) affect how the difference `d = x - y` or `d = y - x` is computed
 *       and how partitioned shares are assigned.
 *
 * @warning Exits with an error if preconditions are violated or an invalid party is specified.
 */
share_array EqualityConst2_channel(share_array x, share_array y, share_t new_q, int channel) {
    if (len(x) != len(y)) {
        printf("LongEqualityConst_channel:  len(x) = %d len(y)  = %d\n", len(x), len(y));
        exit(1);
    }
    if (order(x) != order(y)) {
        printf("LongEqualityConst_channel: order(x) = %d order(y) = %d\n", order(x), order(y));
        exit(1);
    }
    if (order(x) != 1 << (blog(order(x) - 1) + 1)) {
        printf("LongEqualityConst_channel: order(x) = %d\n", order(x));
        exit(1);
    }

    if (_party == -1) {
      int n = len(x);
      share_array ans = _const(n, 0, new_q);
      NEWITER(itr_x, x);
      NEWITER(itr_y, y);
      NEWITER(itr_ans, ans);
      for (int i = 0; i < n; ++i) {
        share_t z = (pa_iter_get(itr_x) == pa_iter_get(itr_y));
        pa_iter_set(itr_ans, z);
      }
      pa_iter_flush(itr_ans);
      pa_iter_free(itr_x);
      pa_iter_free(itr_y);
      return ans;
    }

    share_t old_q = order(x);
    int l = blog(old_q - 1) + 1;
    int n = len(x);

    Partition par = MakeEqualityPatition(l);

    share_array d;
    if (_party == 0 || _party == 1) {  // TODO: What about party0?
        d = vsub(x, y);
    } else if (_party == 2) {
        d = vsub(y, x);
    } else {
        printf("Invalid party: %d\n", _party);
        d = NULL;
        exit(1);
    }

    share_t *raw_d;
    NEWA(raw_d, share_t, n);
    share_t **partitioned_raw_d;
    NEWA(partitioned_raw_d, share_t*, n);
    for (int i = 0; i < n; ++i) {
        raw_d[i] = share_getraw(d, i);
        partitioned_raw_d[i] = Expand(raw_d[i], old_q, par);
    }
    _free(d);
    free(raw_d);

    share_array *p, *q;
    NEWA(p, share_array, par->m);
    NEWA(q, share_array, par->m);
    for (int i = 0; i < par->m; ++i) {
        p[i] = share_const(n, 0, 1<<par->del[i]);
        q[i] = share_const(n, 0, 1<<par->del[i]);
        for (int j = 0; j < n; ++j) {
            if (_party == 0 || _party == 1) {  // TODO: What about party0?
                share_setraw(p[i], j, partitioned_raw_d[j][i]);
            }
            else if (_party == 2) {
                share_setraw(q[i], j, partitioned_raw_d[j][i]);
            }
        }
    }
    for (int i = 0; i < n; ++i) {
      free(partitioned_raw_d[i]);
    }
    free(partitioned_raw_d);

    share_t *mid_q;
    NEWA(mid_q, share_t, par->m);
    share_t N = CalcParamInEquality(par);
    for (int i = 0; i < par->m; ++i) {
        mid_q[i] = N;
    }

    share_array *f = ParallelEqualityConst1_channel(par->m, p, q, mid_q, channel);
    free(mid_q);

    for (int i = 0; i < par->m; ++i) {
        _free(p[i]);
        _free(q[i]);
    }
    free(p);
    free(q);

    share_array a = share_const(n, 0, N);

    for (int i = 0; i < n; ++i) {
        share_t r = 0;
        for (int j = 0; j < par->m; ++j) {
            r = (r + share_getraw(f[j], i)) % N;
        }
        share_setraw(a, i, r);
    }
    for (int j = 0; j < par->m; ++j) _free(f[j]);
    free(f);

    share_array *g = Unitv_channel(a, new_q, channel);
    _free(a);
    share_array ans = share_const(n, 0, new_q);
    for (int i = 0; i < n; ++i) {
        if (_party == 0) {
            if (share_getraw(x, i) == share_getraw(y, i)) {
                share_setraw(ans, i, 1);
            }
            else {
                share_setraw(ans, i, 0);
            }
            share_free(g[i]);
        }
        else {
            share_setshare(ans, i, g[i], par->m);
            share_free(g[i]);
        } 
    }

    Partition_free(par);
    free(g);

    return ans;
}
#define Equality(x, y) EqualityConst2_channel(x, y, 2, 0)
#define EqualityConst(x, y) EqualityConst2_channel(x, y, 2, 0)
#define EqualityConst_channel(x, y, channel) EqualityConst2_channel(x, y, 2, channel)


////////////////////////////////////////////////////////
// Protocol 7-1: Comparison1
// return [x <= y]
// order of outputs is two
////////////////////////////////////////////////////////
/**
 * @brief Compares two shared arrays element-wise using a specified channel.
 *
 * Computes an element-wise less-than-or-equal comparison between two shared arrays
 * `x` and `y`, returning a shared array `b` where each element is 1 if x[i] <= y[i],
 * and 0 otherwise.
 *
 * The function uses overflow detection via parallel overflow computation
 * (ParallelOverflowConst1_channel) on the difference `d = x - y`, as well as
 * on `x` and `y` individually, to determine the comparison result.
 *
 * @param x       The first input shared array.
 * @param y       The second input shared array.
 * @param channel The communication channel to use for overflow computation.
 *
 * @return A shared array `b` of the same length as `x` and `y`, with order 2,
 *         where b[i] = 1 if x[i] <= y[i], and b[i] = 0 otherwise.
 *
 * @note Both arrays must have the same length and order, and the order must be
 *       a power of 2. The function will print an error message and call exit(1)
 *       if these conditions are not met.
 *
 * @warning The caller is responsible for freeing the returned shared array.
 */
share_array Comparison1_channel(share_array x, share_array y, int channel) {
    int n = len(x);
    int q = order(x);
    int l = blog(q - 1) + 1;

    if (len(x) != len(y)) {
        printf("Comparison1  len(x) = %d len(y) = %d\n", len(x), len(y));
        exit(1);
    }
    if (order(x) != order(y)) {
        printf("Comparison1  order(x) = %d order(y) = %d\n", order(x), order(y));
        exit(1);
    }
    if (q != 1 << (blog(q - 1) + 1)) {
        printf("Comparison1  order(x) = %d\n", order(x));
        exit(1);
    }
    
    share_array lt = share_const(n, 0, 2);
    for (int i = 0; i < n; ++i) {
        share_t z = (share_t)(share_getraw(x, i) <= share_getraw(y, i));
        share_setraw(lt, i, z);
    }

    share_array d = vsub(x, y);
    //share_array d = vsub(y, x); // Is this one correct?
    share_array ofd, ofx, ofy;

    share_array dxy[3];
    dxy[0] = share_dup(d);
    dxy[1] = share_dup(x);
    dxy[2] = share_dup(y);
    share_t qs[3] = {2, 2, 2};

    share_pair *ofs = ParallelOverflowConst1_channel(3, dxy, qs, channel);
    ofd = ofs[0].y;
    ofx = ofs[1].y;
    ofy = ofs[2].y;

    free(ofs[0].x);
    free(ofs[1].x);
    free(ofs[2].x);
    share_free(dxy[0]);
    share_free(dxy[1]);
    share_free(dxy[2]);

    share_array b = share_const(n, 1, 2);
    vadd_(b, ofd);
    vadd_(b, ofx);
    vadd_(b, ofy);

    share_free(d);
    share_free(lt);
    share_free(ofd);
    share_free(ofx);
    share_free(ofy);

    if (_party == 0) {
        for (int i = 0; i < n; ++i) {
            if (share_getraw(x, i) <= share_getraw(y, i)) {
                share_setraw(b, i, 1);
            }
            else {
                share_setraw(b, i, 0);
            }
        }
    }

    return b;
}

//////////////////////////////////////////////////////////
// Protocol 7: Comparison
// Return 1 when x <= y
// Modulus of x, y is a power of 2 >= 8
// Value is less than half the modulus
// order of outputs is two
//////////////////////////////////////////////////////////
/**
 * @brief Securely compares two shared arrays element-wise and returns whether
 * each element of @p x is less than or equal to the corresponding element of @p y.
 *
 * This function performs a vectorized comparison over secret-shared values and
 * returns a binary shared array containing one result per element:
 * - 1 if x[i] <= y[i]
 * - 0 otherwise
 *
 * The computation assumes both input arrays have the same length and share order.
 * The share order must be a power of two and at least 8.
 *
 * In local/plain evaluation mode (`_party == -1`), the comparison is computed
 * directly. In secure execution mode, the function derives the result using
 * subtraction and overflow detection, with overflow checks batched through the
 * specified communication channel.
 *
 * @param x First shared input array.
 * @param y Second shared input array.
 * @param channel Communication or processing channel used by the overflow
 * detection routine.
 *
 * @return A shared binary array of length `len(x)` whose i-th element is the
 * secret-shared result of `(x[i] <= y[i])`.
 *
 * @note The function terminates the program if:
 * - `len(x) != len(y)`
 * - `order(x) != order(y)`
 * - the share order is not a power of two, or is less than 8
 *
 * @warning The inputs must be compatible secret-shared arrays created under the
 * same sharing scheme and modulus.
 */
share_array Comparison2_channel(share_array x, share_array y, int channel) 
{
    int n = len(x);
    int q = order(x);
    int l = blog(q - 1) + 1;

    if (len(x) != len(y)) {
        printf("Comparison2  len(x) = %d len(y) = %d\n", len(x), len(y));
        exit(1);
    }
    if (order(x) != order(y)) {
        printf("Comparison2  order(x) = %d order(y) = %d\n", order(x), order(y));
        exit(1);
    }
    if (q != 1 << (blog(q - 1) + 1) || q < 8) {
        printf("Comparison2  order(x) = %d\n", order(x));
        exit(1);
    }
    if (_party == -1) {
      int n =len(x);
      _ b = share_const(n, 0, 2);
      NEWITER(itr_b, b);
      NEWITER(itr_x, x);
      NEWITER(itr_y, y);
      for (int i = 0; i < n; ++i) {
        pa_iter_set(itr_b, pa_iter_get(itr_x) <= pa_iter_get(itr_y));
      }
      pa_iter_flush(itr_b);
      pa_iter_free(itr_x);
      pa_iter_free(itr_y);
      return b;
    }
    
    share_array lt = share_const(n, 0, 2);
    for (int i = 0; i < n; ++i) {
        share_t z = (share_t)(share_getraw(x, i) <= share_getraw(y, i));
        share_setraw(lt, i, z);
    }

    share_array d = vsub(y, x);

    // Write function implementing parallel processing of overflow2 here
    _ tmp = _const(n*3, 0, order(d));
    pa_iter itr_tmp = pa_iter_new(tmp->A); 
    pa_iter itr_d = pa_iter_new(d->A); 
    for (int i=0; i<n; i++) pa_iter_set(itr_tmp, pa_iter_get(itr_d));
    pa_iter_free(itr_d);
    pa_iter itr_x = pa_iter_new(x->A); 
    for (int i=0; i<n; i++) pa_iter_set(itr_tmp, pa_iter_get(itr_x));
    pa_iter_free(itr_x);
    pa_iter itr_y = pa_iter_new(y->A); 
    for (int i=0; i<n; i++) pa_iter_set(itr_tmp, pa_iter_get(itr_y));
    pa_iter_free(itr_y);
    pa_iter_flush(itr_tmp);
    _ of = OverflowConst2_channel(tmp, channel);
    _ b = share_const(n, 0, 2);
    itr_d = pa_iter_pos_new(of->A, 0);
    itr_x = pa_iter_pos_new(of->A, n);
    itr_y = pa_iter_pos_new(of->A, 2*n);
    pa_iter itr_lt = pa_iter_new(lt->A);
    pa_iter itr_b = pa_iter_new(b->A);
    for (int i=0; i<n; i++) {
      share_t z = 0;
      if (_party <= 1) z = 1;
      z ^= pa_iter_get(itr_d) ^ pa_iter_get(itr_x) ^ pa_iter_get(itr_y) ^ pa_iter_get(itr_lt); 
      pa_iter_set(itr_b, z);
    }
    pa_iter_flush(itr_b);
    pa_iter_free(itr_d);
    pa_iter_free(itr_x);
    pa_iter_free(itr_y);
    pa_iter_free(itr_lt);
    _free(tmp);
    _free(of);

    share_free(d);
    share_free(lt);

    if (_party == 0) {
        for (int i = 0; i < n; ++i) {
            if (share_getraw(x, i) <= share_getraw(y, i)) {
                share_setraw(b, i, 1);
            }
            else {
                share_setraw(b, i, 0);
            }
        }
    }

    return b;
}
//#define LessEqual_channel Comparison2_channel
#define LessEqual(x, y) Comparison2_channel(x, y, 0)
#define GreaterThan_channel(x, y, channel) Comparison2_channel(y, x, channel)
#define GreaterThan(x, y) GreaterThan_channel(x, y, 0)

//////////////////////////////////////////////////////////
// Protocol 3: MSNZB1
//////////////////////////////////////////////////////////
/**
 * @brief Computes the Most Significant Non-Zero Bit (MSNZB) for each element
 *        in a shared bit array, operating on a specific channel.
 *
 * @param b        The input shared bit array to process.
 * @param new_q    The modulus used for share arithmetic operations.
 * @param channel  The channel index to select from the UnitvB computation.
 *
 * @return A share array of size n*t, where each group of t elements represents
 *         the MSNZB encoding for the corresponding element in b.
 *         The caller is responsible for freeing the returned share array.
 *
 * @note Internally uses UnitvB_channel to compute intermediate values,
 *       then aggregates over bit-level groups of size 2^i for i in [0, t).
 *       The intermediate array v is freed before returning.
 *
 * @see UnitvB_channel
 */
_ MSNZB1_channel(_bits b, share_t new_q, int channel)
{
  int n = len_bits(b);
  int t = depth_bits(b);
  int m = 1 << t;

  share_array v = UnitvB_channel(b, new_q, channel);

  _ c = share_const(n*t, 0, new_q);
  for (int p = 0; p < n; ++p) {
    for (int i = 0; i < t; ++i) {
      share_t x = 0;
      for (int j = (1<<i); j < (1<<(i+1)); ++j) {
        x = (x + share_getraw(v, p * m + j)) % new_q;
      }
      share_setraw(c, p * t + i, x);
      }
   }
   _free(v);

   return c;
}

//////////////////////////////////////////////////////////
// Protocol 4: MSNZB
//////////////////////////////////////////////////////////
/**
 * @brief Computes channel-specific aggregated level indicators from a bit-encoded input.
 *
 * This function derives a per-element, per-level summary from the vector returned by
 * `UnitvB_channel()`. For each input position `p` and each level `i` in
 * `[0, depth_bits(b))`, it sums the entries in the range
 * `j = 2^i .. 2^(i+1)-1` of the corresponding block in `v`, modulo `new_q`,
 * and stores the result in the output share array.
 *
 * Conceptually, the result encodes, for each input item, whether the selected
 * channel contributes within each power-of-two bucket of the unit-vector
 * expansion.
 *
 * @param b        Bit-encoded input sequence.
 * @param new_q    Modulus of the returned shares.
 * @param channel  Channel index to extract from the input.
 *
 * @return A share array containing the aggregated values for each input position
 *         and each level, reduced modulo `new_q`.
 *
 * @note The temporary vector produced by `UnitvB_channel()` is freed before return.
 */
_ MSNZB_channel(_bits b, share_t new_q, int channel)
{

  int n = len_bits(b);
  int l = depth_bits(b);

  Partition par = MakeOverflowPatition2(l);

  int m = 1 << l;

  share_array v = UnitvB_channel(b, new_q, channel);

  _ c = share_const(n*m, 0, new_q);
  for (int p = 0; p < n; ++p) {
    for (int i = 0; i < l; ++i) {
      share_t x = 0;
      for (int j = (1<<i); j < (1<<(i+1)); ++j) {
        x = (x + share_getraw(v, p * m + j)) % new_q;
      }
      share_setraw(c, p * l + i, x);
      }
   }
   _free(v);

   return c;
}

_ sum(_ v); // in share.h


////////////////////////////////////////////////////////////
// Return a[index]
////////////////////////////////////////////////////////////
/**
 * @brief Securely retrieves an element from a secret-shared array using a single index and channel-specific equality.
 *
 * This function performs oblivious array lookup by:
 * 1. Validating that `index` has length 1.
 * 2. Expanding the index to match the array length.
 * 3. Building an ID permutation from the index ordering.
 * 4. Computing channel-aware equality masks.
 * 5. Multiplying the mask with the array and summing to extract the selected element.
 *
 * @param a       Secret-shared input array.
 * @param index   Single-element index container indicating the target position.
 * @param channel Channel identifier used for channel-specific equality computation.
 * @return _      The selected secret-shared element as a scalar/aggregate of type `_`.
 *
 * @note The function terminates the process if `len(index) != 1`.
 * @note Intermediate temporary objects are explicitly freed before return.
 */
_ array_lookup_channel(_ a, _ index, int channel)
{
  if (len(index) != 1) {
    printf("array_lookup: len(index) = %d\n", len(index));
    exit(1);
  }
  int n = len(a);

  _ eindex = extend_share_array(n, index);
  _ p = Perm_ID2(n, order(index));
  //_ eq = Equality(p, eindex);
  //_ s = IfThen_b(eq, a);
  _ eq = EqualityConst2_channel(p, eindex, order(a), channel);
  _ s = vmul(eq, a);
  _ ans = sum(s);
  _free(eindex); _free(p); _free(eq); _free(s);
  return ans;
}
#define array_lookup(a, index) array_lookup_channel(a, index, 0)

/**
 * @brief Performs batched secure/shared array lookups on a specific communication channel.
 *
 * Computes `a[index[i]]` for each element in `index` using permutation/selection primitives
 * over shared arrays, then returns the resulting array of looked-up values.
 *
 * @param a       Source shared array to read from (length `n`).
 * @param index   Shared array of indices to look up (length `m`).
 * @param channel Channel identifier used by channel-aware equality evaluation.
 *
 * @return A shared array of length `m` containing lookup results corresponding to `index`.
 *
 * @note This routine allocates intermediate shared arrays and frees them internally
 *       before returning.
 */
_ array_lookups_channel(_ a, _ index, int channel)
{
  int n = len(a);
  int m = len(index);

  _ eindex = extend_share_array(n, index);
  _ p = Perm_ID2(n, order(index));
  _ ep = ntimes(p, m);
  _ ea = ntimes(a, m);
  _ eq = EqualityConst2_channel(ep, eindex, order(a), channel);
  _ s = vmul(eq, ea);
  _ ans = _reduce(s, n);
  _free(eindex); _free(p); _free(ep); _free(ea); _free(eq); _free(s);
  return ans;
}
#define array_lookups(a, index) array_lookups_channel(a, index, 0)


////////////////////////////////////////////////////////////
// Return overflow to i-th bit
////////////////////////////////////////////////////////////
/**
 * @brief Reduces a share value and computes an overflow representation on a specific channel,
 *        with optional conversion to a requested modulus.
 *
 * This function validates `i`, shrinks `x` by `2^i`, computes an overflow-constant form
 * on `channel`, and (when `new_q != 2`) converts the intermediate result from modulus 2
 * to `new_q`. Temporary values are released before returning.
 *
 * @param x        Input share.
 * @param i        Bit-shift exponent used for shrinking (`2^i`); must be > 0 and within `order(x)`.
 * @param new_q    Target modulus for the result. If `2`, no modulus conversion is performed.
 * @param channel  Channel identifier used for channel-specific operations.
 *
 * @return A newly allocated share containing the overflow result (possibly converted to `new_q`).
 *
 * @note The function terminates the process with an error message if:
 *       - `i <= 0`
 *       - `2^i > order(x)`
 */
_ Overflow_q_channel(_ x, int i, share_t new_q, int channel)
{
    if (i <= 0) {
        printf("Overflow_channel: i = %d\n", i);
        exit(1);
    }
    if ((1 << i) > order(x)) {
        printf("Overflow_channel: 1 << i = %d order(x) = %d\n", 1 << i, order(x));
        exit(1);
    }
    _ y = _shrink(x, 1 << i);
    _ t = OverflowConst2_channel(y, channel);
    if (new_q != 2) {
      _ t2 = B2A_channel(t, new_q, channel);
      _move_(t, t2);
    }
    _free(y);
    return t;
}
#define Overflow_q(x, i, new_q) Overflow_q_channel(x, i, new_q, 0)
#define Overflow(x, i) Overflow_q_channel(x, i, 2, 0)

////////////////////////////////////////////////////////
// Protocol 16: Modulo
// return (x mod 2^i) in [0, new_q)
////////////////////////////////////////////////////////
/**
 * @brief Securely computes `x mod 2^i` on a specific communication channel under a new modulus.
 *
 * This routine derives the low `i` bits of each element in secret-shared vector `x` by:
 * 1. Computing an overflow/carry indicator with `Overflow_q_channel(...)`.
 * 2. Applying the correction `((x % 2^i) - 2^i * overflow)` element-wise.
 * 3. Reducing the result with `MOD(...)` into the target ring `new_q`.
 *
 * @param x        Input secret-shared vector.
 * @param i        Bit width for the power-of-two modulus (`k = 2^i`).
 * @param new_q    Target modulus (ring order) for the output shares.
 * @param channel  Communication/runtime channel used by channel-aware subroutines.
 *
 * @return A newly allocated secret-shared vector where each entry is the modulo result in `new_q`.
 *
 * @note Assumes `Overflow_q_channel` is compatible with the same channel and modulus conversion flow.
 * @note Intermediate buffers/iterators are managed internally; caller is responsible for freeing the returned object.
 */
_ Modulo_channel(_ x, int i, share_t new_q, int channel) {
  int n = len(x);
  share_t k = 1 << i;

  _ t = Overflow_q_channel(x, i, new_q, channel);
  _ z = _const(n, 0, new_q);

  share_t q = new_q;

  pa_iter itr_z = pa_iter_new(z->A);
  pa_iter itr_x = pa_iter_new(x->A);
  pa_iter itr_t = pa_iter_new(t->A);
  for (int i = 0; i < n; ++i) {
    pa_iter_set(itr_z, MOD((pa_iter_get(itr_x) % k) - k * pa_iter_get(itr_t)));
  }
  pa_iter_flush(itr_z);
  pa_iter_free(itr_x);
  pa_iter_free(itr_t);
  _free(t);
  return z;
}
#define Modulo(x, i, new_q) Modulo_channel(x, i, new_q, 0)

////////////////////////////////////////////////////////
// Protocol 15: RightShift
// return (x >> i) in [0, new_q)
////////////////////////////////////////////////////////
/**
 * @brief Securely computes a right-shift of shared values with channel-aware overflow correction.
 *
 * This routine performs an arithmetic transformation equivalent to shifting each element of
 * the shared array `x` right by `i` bits, while preserving correctness under modular sharing
 * by compensating for overflow terms with respect to both the original modulus and `new_q`.
 *
 * Internally, it:
 * - Computes overflow indicators at bit positions `l` (bit-length of original order) and `i`.
 * - Combines the shifted value and overflow corrections per element:
 *   `z_i = (x_i >> i) + b_i - (t_i << (l - i))`
 * - Reduces each result modulo `new_q`.
 *
 * @param x        Input shared vector.
 * @param i        Number of bits to shift right.
 * @param new_q    Target modulus/order for the output shares.
 * @param channel  Communication/evaluation channel identifier used by channel-aware primitives.
 *
 * @return A newly allocated shared vector containing the corrected right-shifted values modulo `new_q`.
 *
 * @note The caller is responsible for freeing the returned shared object.
 * @note Assumes `0 <= i <= l`, where `l` is derived from the original order of `x`.
 */
_ RightShift_channel(_ x, int i, share_t new_q, int channel) {
  int n = len(x);
  share_t q = order(x);
  int l = blog(q - 1) + 1;

  _ t = Overflow_q_channel(x, l, new_q, channel); // TODO: Parallelize
  _ b = Overflow_q_channel(x, i, new_q, channel);
  _ z = _const(n, 0, new_q);

  q = new_q;

  pa_iter itr_z = pa_iter_new(z->A);
  pa_iter itr_x = pa_iter_new(x->A);
  pa_iter itr_t = pa_iter_new(t->A);
  pa_iter itr_b = pa_iter_new(b->A);
  for (int j = 0; j < n; ++j) {
    share_t x_i = pa_iter_get(itr_x);
    share_t t_i = pa_iter_get(itr_t);
    share_t b_i = pa_iter_get(itr_b);
    share_t z_i = (x_i >> i) + b_i - (t_i << (l - i));
    pa_iter_set(itr_z, MOD(z_i));
  }
  pa_iter_flush(itr_z);
  pa_iter_free(itr_x);
  pa_iter_free(itr_t);
  pa_iter_free(itr_b);
  _free(t);  _free(b);
  return z;
}
#define RightShift(x, i, new_q) RightShift_channel(x, i, new_q, 0)

/**
 * @brief Changes the modulus context of a shared value for a specific channel.
 *
 * This routine adjusts the modulus of `x` to `new_q` on the given `channel`
 * by first creating an extended left-shifted intermediate and then applying
 * a channel-aware right shift under the target modulus.
 *
 * @param x        Input shared value.
 * @param new_q    Target modulus to apply during conversion.
 * @param channel  Channel index used for channel-specific shifting behavior.
 * @return Converted shared value under `new_q` for the specified channel.
 *
 * @note The intermediate temporary value is released internally before return.
 */
_ ChangeModulo_channel(_ x, share_t new_q, int channel) {
  _ y = share_lshift_extend(x, 1);
  _ ans = RightShift_channel(y, 1, new_q, channel);
  _free(y);
  return ans;
}
#define ChangeModulo(x, new_q) ChangeModulo_channel(x, new_q, 0)


#endif 
