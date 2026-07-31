#ifndef PRECOMPUTE2_H
 #define PRECOMPUTE2_H
static share_t table_xor[] = {0, 1, 
                              1, 0};
static share_t table_overflow1[] = {0, 0, 
                                    0, 1};

static share_t table_overflow2[] = {0, 0, 0, 0,
                                    0, 0, 0, 1,
                                    0, 0, 1, 1,
                                    0, 1, 1, 1};
static share_t table_overflow3[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 1,
                                    0, 0, 0, 0, 0, 0, 1, 1,
                                    0, 0, 0, 0, 0, 1, 1, 1,
                                    0, 0, 0, 0, 1, 1, 1, 1,
                                    0, 0, 0, 1, 1, 1, 1, 1,
                                    0, 0, 1, 1, 1, 1, 1, 1,
                                    0, 1, 1, 1, 1, 1, 1, 1};
static share_t table_overflow4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

static void precomp_read_config(int n, share_t q, FILE *fin)
{
  char buf[1000];
  char fname[1000];
  int channel;
  if (fgets(buf, 1000, fin) == NULL) return;
  while (1) {
    if (scmp(buf, "[mt_seeds_precomp]") == 0) {
      unsigned long init[5];
      int party;
      while (1) {
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %ld %ld %ld %ld %ld", &party, &init[0], &init[1], &init[2], &init[3], &init[4]) != 6) break;
        if (party < 0 || party > 3) {
          printf("error party %d\n", party);
          exit(1);
        }
        for (int i=0; i<5; i++) MT_init[party][i] = init[i];
      }
    } else if (scmp(buf, "[pre_bt]") == 0) {
      while (1) {
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %s", &channel, fname) != 2) break;
        printf("BT_tbl %d %s\n", channel, fname);
        BeaverTriple_precomp(n, q, fname);
      }
    } else if (scmp(buf, "[pre_of]") == 0) {
      while (1) {
        int d;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %s", &d, &channel, fname) != 3) break;
        printf("PRE_OF_tbl bits=%d channel=%d %s\n", d, channel, fname);
        if (d == 1) func1bit3_precomp(n, q, table_overflow1, fname);
        else if (d == 2) funckbit_precomp(2, n, q, table_overflow2, fname);
        else if (d == 3) funckbit_precomp(3, n, q, table_overflow3, fname);
        else if (d == 4) funckbit_precomp(4, n, q, table_overflow4, fname);
        else {
          printf("d = %d is not supported.\n", d);
        }
      }
    } else if (scmp(buf, "[pre_b2a]") == 0) {
      while (1) {
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %s", &channel, fname) != 2) break;
        printf("PRE_B2A_tbl %d %s\n", channel, fname);
        func1bit3_precomp(n, q, table_xor, fname);
      }
    } else if (scmp(buf, "[pre_onehot]") == 0) {
      while (1) {
        int d, xor;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %d %s", &d, &xor, &channel, fname) != 4) break;
        printf("PRE_OH_tbl bits=%d xor=%d channel=%d %s\n", d, xor, channel, fname);
        onehotvec_precomp(d, n, q, fname, xor);
      }
#ifdef _SHAMIR_H
    } else if (scmp(buf, "[pre_onehot_shamir]") == 0) {
      while (1) {
        int d, xor;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %s", &d, &channel, fname) != 3) break;
        printf("PRE_OHS_tbl bits=%d channel=%d %s\n", d, channel, fname);
        onehotvec_shamir_precomp(d, n, q, fname);
      }
    } else if (scmp(buf, "[pre_onehot_shamir3]") == 0) {
      while (1) {
        int d, xor;
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %x %d %s", &d, &irr_poly, &channel, fname) != 4) break;
        printf("PRE_OHS3_tbl bits=%d irr_poly=%x channel=%d %s\n", d, irr_poly, channel, fname);
        onehotvec_shamir3_precomp(d, n, q, irr_poly, fname);
      }
    } else if (scmp(buf, "[pre_onehot_rss]") == 0) {
      while (1) {
        int d, xor;
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %x %d %s", &d, &irr_poly, &channel, fname) != 4) break;
        printf("PRE_OHR_tbl bits=%d irr_poly=%x channel=%d %s\n", d, irr_poly, channel, fname);
        onehotvec_shamir3_type_precomp(d, n, q, irr_poly, SHARE_T_RSS, fname);
      }
    } else if (scmp(buf, "[pre_shamir3_revert]") == 0) {
      while (1) {
        int d, xor;
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %x %d %s", &d, &irr_poly, &channel, fname) != 4) break;
        printf("PRE_RE_tbl bits=%d irr_poly=%x channel=%d %s\n", d, irr_poly, channel, fname);
        shamir3_revert_precomp(n, 1<<d, irr_poly, fname);
      }
#endif
    } else if (scmp(buf, "[pre_ds]") == 0) {
      while (1) {
        int len, bs, inverse;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %d %d %s", &len, &bs, &inverse, &channel, fname) != 5) break;
        printf("PRE_DS_tbl n=%d bs=%d inverse=%d channel=%d %s\n", len, bs, inverse, channel, fname);
        //dshare_precomp(1, 1<<n, q, inverse, fname);
        dshare_precomp(1, len, q, inverse, fname);
      }
#ifdef _UNITV_H
    } else if (scmp(buf, "[pre_uv]") == 0) {
      while (1) {
        int n, old_q, new_q, channel;
        if (fgets(buf, 1000, fin) == NULL)  return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %d %d %s", &n, &old_q, &new_q, &channel, fname) != 5) break;
        //uv_tbl_read(channel, n, old_q, new_q, fname);
        printf("PRE_UV_tbl %d %s\n", channel, fname);
        UV_tables_precomp(n, old_q, new_q, fname);
      } 
#endif
#ifdef _FIELD_H
    } else if (scmp(buf, "[pre_gf]") == 0) {
      while (1) {
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) return;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %x %d %s", &irr_poly, &channel, fname) != 3) break;
        printf("PRE_GF_tbl irr_poly=%x channel=%d %s\n", irr_poly, channel, fname);
        BeaverTriple_GF_precomp(n, irr_poly, fname);
      }
#endif
    } else if (buf[0] == '#') { // skip comments
      if (fgets(buf, 1000, fin) == NULL) break;
    } else {
      if (fgets(buf, 1000, fin) == NULL) break;
    }
  }
}

#endif
