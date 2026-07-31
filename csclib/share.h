#ifndef _SHARE_H
 #define _SHARE_H

// If you want to change the parameters below,
// define them before including share.h.

#ifndef MAX_PARTIES
 #define MAX_PARTIES 4
#endif

#ifndef MAX_CHANNELS
 #define MAX_CHANNELS 10
#endif

#ifndef OF_MAX
 #define OF_MAX 4
#endif

#ifndef ONEHOT_MAX
 #define ONEHOT_MAX 4
#endif



#include <stdio.h>
#include <stdlib.h>
#include "share_core.h"
#include "slice.h"
#include "local.h"

#include "if_then.h"
#include "logic.h"

#include "share_bits.h"
#include "decomposition.h" // bit decomposition



/////////////////////////////////////////////
// The include order below does not matter.
#include "field.h"  // GF(2^d) operations (optional)

#include "unitv.h" // unit vector (optional)

#include "func.h"

#include "compare.h"


#include "dshare.h"
/////////////////////////////////////////////
// The following depend on dshare.h.
#include "stablesort.h" // 1-bit sort
#include "radixsort.h" // k-bit sort
#include "propagate.h"
#include "batchaccess.h"
//#include "shamir.h"
//#include "rss.h"

extern unsigned long MT_init[4][5];

#define scmp(p, q) strncasecmp(p, q, strlen(q))

void PRG_initialize(int num_parties) {
  unsigned long init[5];
  unsigned long init2[5];
  int party = _party;
  if (party < 0) {
    party = 0;
    num_parties = 3;
  }
  MT_init[party][4] = party; // differs by party (needs revision)
  mt0 = MT_init_by_array(MT_init[party], 5); // random source known only to each party
  for (int i=0; i<5; i++) init[i] = MT_init[party][i];
  if (_party <= 0) {
    for (int p=0; p<num_parties; p++) {
      for (int i=0; i<5; i++) init[i] = MT_init[p][i];
      for (int c=0; c<_opt.channels; c++) {
        init[3] = MT_init[p][c]+c;
        mt_[p][c] = MT_init_by_array(init, 5); // random source shared by party 0 and p
        if (p==1) mt1[c] = mt_[p][c];
        if (p==2) mt2[c] = mt_[p][c];
        if (p==3) mt3[c] = mt_[p][c];
        if (p > 0 && _party == 0) mpc_send(p, init, sizeof(init[0])*5);
      }
    }
  } else {
    for (int c=0; c<_opt.channels; c++) {
      mpc_recv(FROM_SERVER, init, sizeof(init[0])*5);
      mt_[0][c] = MT_init_by_array(init, 5); // random source shared with party 0
      mts[c] = mt_[0][c];
    }
  }

  if (_party > 0) {
    int next = _party % (num_parties-1) + 1;
    int prev = (_party+num_parties-3) % (num_parties-1) + 1;
    for (int c=0; c<_opt.channels; c++) {
      mpc_send(next, MT_init[next], sizeof(MT_init[next][0])*5);
      mpc_recv(prev, init, sizeof(init[0])*5);
      for (int i=0; i<5; i++) {
        init2[i] = MT_init[prev][i] ^ init[i];
      }
      mt_[prev][c] = MT_init_by_array(init2, 5); // random source shared with the previous party
      if (prev==1) mt1[c] = mt_[prev][c];
      if (prev==2) mt2[c] = mt_[prev][c];
      if (prev==3) mt3[c] = mt_[prev][c];
      if (num_parties >= 4) {
        mpc_send(prev, MT_init[prev], sizeof(MT_init[prev][0])*5);
        mpc_recv(next, init, sizeof(init[0])*5);
        for (int i=0; i<5; i++) {
          init2[i] = MT_init[next][i] ^ init[i];
        }
        mt_[next][c] = MT_init_by_array(init2, 5); // random source shared with the next party
        if (next==1) mt1[c] = mt_[next][c];
        if (next==2) mt2[c] = mt_[next][c];
        if (next==3) mt3[c] = mt_[next][c];
      }
    }
  }
}

void PRG_free(void)
{
  MT_free(mt0);
  if (_party == -1) _opt.parties = 3;
  if (_party <= 0) {
    for (int i=0; i<_opt.channels; i++) {
      for (int p=0; p<_opt.parties; p++) {
        MT_free(mt_[p][i]);
      }
    }
  }
  if (_party > 0) {
    for (int i=0; i<_opt.channels; i++) {
      printf("MTS[%d] %ld\n", i, mts[i]->count);
      MT_free(mts[i]);
    }
  }
}



parties* read_config(FILE *fin)
{
  char buf[1000];
  char fname[1000];
  int channel;
  parties *P;
  NEWA(P, parties, MAX_PARTIES);
  for (int i=0; i<MAX_PARTIES; i++) P[i] = NULL;

  if (fgets(buf, 1000, fin) == NULL) goto end;
  while (1) {
    if (scmp(buf, "[options]") == 0) {
      int x;
      while (1) {
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %999s %d", fname, &x) != 2) break;
        if (scmp(fname, "parties") == 0) {
          if (x > MAX_PARTIES) {
            printf("MAX_PARTIES = %d parties = %d\n", MAX_PARTIES, x);
            exit(1);
          }
          _opt.parties = x;
          printf("number of parties = %d\n", x);
          _num_parties = _opt.channels;
        } else if (scmp(fname, "channels") == 0) {
          if (x > MAX_CHANNELS) {
            printf("MAX_CHANNELS = %d parties = %d\n", MAX_CHANNELS, x);
            exit(1);
          }
          _opt.channels = x;
          printf("number of channels = %d\n", x);
        } else if (scmp(fname, "comm_no_delay") == 0) {
          _opt.comm_no_delay = x;
          printf("opt.comm_no_delay = %d\n", x);
        } else if (scmp(fname, "warn_precomp") == 0) {
          _opt.warn_precomp = x;
          printf("opt.warn_precomp = %d\n", x);
        } else if (scmp(fname, "send_queue") == 0) {
          _opt.send_queue = x;
          printf("opt.send_queue = %d\n", x);
        } else if (scmp(fname, "oram_check_overflow") == 0) {
          _opt.oram_check_overflow = x;
          printf("opt.oram_check_overflow = %d\n", x);
        } else {
          printf("??? %s\n", fname);
        }
      }
    }
    if (_party == -1) {
      if (fgets(buf, 1000, fin) == NULL) break;
      continue;
    }
    if (scmp(buf, "[parties]") == 0) {
      int i = 0;
      while (i < MAX_PARTIES) {
        char addr[100];
        int port;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue; // skip comments
        //if (buf[0] == '[') continue; // skip
        if (sscanf(buf, " %99s %d", addr, &port) != 2) break;
        P[i] = (parties)malloc(sizeof(*P[i]));
        P[i]->addr = strdup(addr);
        P[i]->port = port;
        printf("party %d %s:%d\n", i, addr, port);
        i++;
      }
      if (i < _opt.parties) {
        printf("warning: opt.parties = %d i = %d\n", _opt.parties, i);
      }
    } else if (scmp(buf, "[mt_seeds]") == 0) { // TODO: split by channel
      unsigned long init[5];
      int party;
      while (1) {
        if (fgets(buf, 1000, fin) == NULL) goto end;
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
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %s", &channel, fname) != 2) break;
        if (channel >= _opt.channels) {
          continue; // skip unnecessary tables
          //printf("error channel %d MAX_CHANNELS %d\n", channel, MAX_CHANNELS);
          //exit(1);
        }
        if (_party <= 2) {
          bt_tbl_read(channel, fname);
          printf("BT_tbl %d %s\n", channel, fname);
        }
      }
    } else if (scmp(buf, "[pre_of]") == 0) {
      while (1) {
        int d;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %s", &d, &channel, fname) != 3) break;
        if (channel >= _opt.channels) {
          continue;
        }
        if (_party <= 2) {
          of_tbl_read(d, channel, fname);
          printf("PRE_OF_tbl bits=%d channel=%d %s\n", d, channel, fname);
        }
      }
    } else if (scmp(buf, "[pre_b2a]") == 0) {
      while (1) {
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %s", &channel, fname) != 2) break;
        if (channel >= _opt.channels) {
          continue;
        }
        if (_party <= 2) {
          b2a_tbl_read(channel, fname);
          printf("PRE_B2A_tbl %d %s\n", channel, fname);
        }
      }
    } else if (scmp(buf, "[pre_onehot]") == 0) {
      while (1) {
        int d, xor;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %d %s", &d, &xor, &channel, fname) != 4) break;
        if (channel >= _opt.channels) {
          continue;
        }
        if (_party <= 2) {
          onehot_tbl_read(d, xor, channel, fname);
          printf("PRE_OH_tbl bits=%d xor=%d channel=%d %s\n", d, xor, channel, fname);
        }
      }
    } else if (scmp(buf, "[pre_onehot_shamir]") == 0) {
      while (1) {
        int d, xor;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %s", &d, &channel, fname) != 3) break;
        if (channel >= _opt.channels) {
          continue;
        }
        onehot_shamir_tbl_read(d, channel, fname);
        printf("PRE_OHS_tbl bits=%d channel=%d %s\n", d, channel, fname);
      }
    } else if (scmp(buf, "[pre_onehot_shamir3]") == 0) {
      while (1) {
        int d, xor;
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %x %d %s", &d, &irr_poly, &channel, fname) != 4) break;
        if (channel >= _opt.channels) {
          continue;
        }
        onehot_shamir3_tbl_read(d, irr_poly, channel, fname);
        printf("PRE_OHS3_tbl bits=%d irr_poly=%x channel=%d %s\n", d, irr_poly, channel, fname);
      }
    } else if (scmp(buf, "[pre_onehot_rss]") == 0) {
      while (1) {
        int d, xor;
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %x %d %s", &d, &irr_poly, &channel, fname) != 4) break;
        if (channel >= _opt.channels) {
          continue;
        }
        onehot_rss_tbl_read(d, irr_poly, channel, fname);
        printf("PRE_OHR_tbl bits=%d irr_poly=%x channel=%d %s\n", d, irr_poly, channel, fname);
      }
    } else if (scmp(buf, "[pre_shamir3_revert]") == 0) {
      while (1) {
        int d, xor;
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') break;
        if (sscanf(buf, " %d %x %d %s", &d, &irr_poly, &channel, fname) != 4) break;
        if (channel >= _opt.channels) {
          continue;
        }
        shamir3_revert_tbl_read(d, irr_poly, channel, fname);
        printf("PRE_RE_tbl bits=%d irr_poly=%x channel=%d %s\n", d, irr_poly, channel, fname);
      }
    } else if (scmp(buf, "[pre_ds]") == 0) {
      while (1) {
        int n, bs, inverse;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %d %d %s", &n, &bs, &inverse, &channel, fname) != 5) break;
        if (channel >= _opt.channels) {
          continue;
        }
        if (_party <= 2) {
          ds_tbl_read(channel, n, bs, inverse, fname);
          printf("PRE_DS_tbl n=%d bs=%d inverse=%d channel=%d %s\n", n, bs, inverse, channel, fname);
        }
      }
#ifdef _UNITV_H
    } else if (scmp(buf, "[pre_uv]") == 0) {
      while (1) {
        int n, old_q, new_q, channel;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %d %d %d %d %s", &n, &old_q, &new_q, &channel, fname) != 5) break;
        if (channel > _opt.channels) {
          continue;
        }
        uv_tbl_read(channel, n, old_q, new_q, fname);
        printf("PRE_UV_tbl %d %s\n", channel, fname);
      } 
#endif
#ifdef _FIELD_H
    } else if (scmp(buf, "[pre_gf]") == 0) {
      while (1) {
        share_t irr_poly;
        if (fgets(buf, 1000, fin) == NULL) goto end;
        if (buf[0] == '#') continue;
        if (sscanf(buf, " %x %d %s", &irr_poly, &channel, fname) != 3) break;
        if (channel >= _opt.channels) {
          continue;
        }
        if (_party <= 2) {
          GF_tbl_read(channel, irr_poly, fname);
          printf("PRE_GF_tbl irr_poly=%x channel=%d %s\n", irr_poly, channel, fname);
        }
      }
#endif
    } else if (buf[0] == '#') { // skip comments
      if (fgets(buf, 1000, fin) == NULL) break;
    } else {
      if (fgets(buf, 1000, fin) == NULL) break;
    }
  }
  end:;
  if (_party == -1) {
    _opt.warn_precomp = 0;
  }
  return P;
}

static parties* party_read(void)
{
  if (_party == -1) {
    _opt.parties = 1;
  }

  FILE *fin;
  char buf[1000];
  int  i;

  _opt.parties = 3; // default
  _opt.channels = 1; // default

  fin = fopen("config.txt", "r");
  if (fin == NULL && _party >= 0) {
    printf("cannot open config.txt\n");
    exit(1);  
  }
  parties *P = read_config(fin);
  fclose(fin);
  _num_parties = _opt.parties; // remove in the future
  return P;
}

static void party_free(parties *P, int num_parties)
{
  for (int i=0; i<MAX_PARTIES; i++) {
    if (P[i]) {
      free(P[i]->addr);
      free(P[i]);
    }
  }
  free(P);
}

static void mpc_start(void)
{
  precomp_tables_new();


  printf("party %d\n", _party);
  if (_party < 0) {
    _opt.parties = 1;
    _opt.channels = 1;
    PRG_initialize(1);
    //_Parties = party_read(0);
    parties *P;
    NEWA(P, parties, MAX_PARTIES);
    for (int i=0; i<MAX_PARTIES; i++) P[i] = NULL;
    _Parties = P;
    return;
  }
  _Parties = party_read();

  int num_parties = _opt.parties;
  int num_channels = _opt.channels;

  NEWA(_C, comm, num_channels*num_parties);
  for (int i=0; i<num_channels*num_parties; i++) _C[i] = NULL;


  thread_param *params;
  NEWA(params, thread_param, num_channels*num_parties);

  struct comm_init_args *args;
  NEWA(args, struct comm_init_args, num_channels*num_parties);

  if (_party == 0) { // P0 acts as server for the other parties
    for (int j=1; j<num_parties; j++) {
      for (int i=0; i<num_channels; i++) {
        int x = i*num_parties+j;
        args[x].server = 1;
        args[x].port = _Parties[0]->port + x;
        params[x] = thread_new(comm_init_thread, &args[x]);
      }
    }
  } else {
    for (int i=0; i<num_channels; i++) { // connect to P0
      int x = i*num_parties+TO_SERVER;
      args[x].server = 0;
      args[x].dest_name = _Parties[0]->addr;
      args[x].port = _Parties[0]->port + i*num_parties+_party;
      params[x] = thread_new(comm_init_thread, &args[x]);
      if (_party != num_parties-1) { // except the last party, serve the next party
        int x = i*num_parties + _party+1;
        args[x].server = 1;
        args[x].port = _Parties[_party]->port + x;
        params[x] = thread_new(comm_init_thread, &args[x]);
      } 
      if (_party == 1 && num_parties > 3) { // P1 also serves the last party
        int x = i*num_parties + num_parties-1; // last party
        args[x].server = 1;
        args[x].port = _Parties[_party]->port + (i*num_parties + num_parties-1);
        params[x] = thread_new(comm_init_thread, &args[x]);
      }
      if (_party != 1) {
        int x = i*num_parties + _party-1; // connect to previous party
        args[x].server = 0;
        args[x].dest_name = _Parties[_party-1]->addr;
        args[x].port = _Parties[_party-1]->port + (i*num_parties + _party);
        params[x] = thread_new(comm_init_thread, &args[x]);
      }
      if (_party == num_parties-1 && num_parties > 3) {
        int x = i*num_parties + 1; // connect to P1
        args[x].server = 0;
        args[x].dest_name = _Parties[1]->addr;
        args[x].port = _Parties[1]->port + (i*num_parties + _party);
        params[x] = thread_new(comm_init_thread, &args[x]);
      }
    }
  }

  if (_party == 0) {
    for (int j=1; j<num_parties; j++) {
      for (int i=0; i<num_channels; i++) {
        int x = i*num_parties+j;
        thread_end(params[x]);
        _C[x] = args[x].ans;
        _C[x]->total_send = 0;
        _C[x]->total_recv = 0;
        _C[x]->total_send_rounds = 0;
        _C[x]->total_recv_rounds = 0;
      }
    }
  } else {
    for (int i=0; i<num_channels; i++) {
      int x = i*num_parties+TO_SERVER;
      thread_end(params[x]);
      _C[x] = args[x].ans;
      _C[x]->total_send = 0;
      _C[x]->total_recv = 0;
      _C[x]->total_send_rounds = 0;
      _C[x]->total_recv_rounds = 0;
      if (_party != num_parties-1) {
        int x = i*num_parties + _party+1;
        thread_end(params[x]);
        _C[x] = args[x].ans;
        _C[x]->total_send = 0;
        _C[x]->total_recv = 0;
        _C[x]->total_send_rounds = 0;
        _C[x]->total_recv_rounds = 0;
      }
      if (_party == 1 && num_parties > 3) {
        int x = i*num_parties + num_parties-1;
        thread_end(params[x]);
        _C[x] = args[x].ans;
        _C[x]->total_send = 0;
        _C[x]->total_recv = 0;
        _C[x]->total_send_rounds = 0;
        _C[x]->total_recv_rounds = 0;
      }
      if (_party != 1) {
        int x = i*num_parties + _party-1; // connect to previous party
        thread_end(params[x]);
        _C[x] = args[x].ans;
        _C[x]->total_send = 0;
        _C[x]->total_recv = 0;
        _C[x]->total_send_rounds = 0;
        _C[x]->total_recv_rounds = 0;
      }
      if (_party == num_parties-1 && num_parties > 3) {
        int x = i*num_parties + 1;
        thread_end(params[x]);
        _C[x] = args[x].ans;
        _C[x]->total_send = 0;
        _C[x]->total_recv = 0;
        _C[x]->total_send_rounds = 0;
        _C[x]->total_recv_rounds = 0;
      }
    }
  }

  NEWA(_send_queue_idx, int, num_parties * num_channels);
  NEWA(_send_queue, char*, num_parties * num_channels);
  for (int i = 0; i < num_parties * num_channels; ++i) {
    _send_queue_idx[i] = 0;
    NEWA(_send_queue[i], char, BUFFER_SIZE);
  }

  free(params);
  free(args);

  PRG_initialize(_opt.parties);
}

long get_total_send(void)
{
  long total_send = 0;
  if (_C == 0) return total_send;

  for (int i=0; i<_opt.channels*_opt.parties; i++) {
    if (_C[i] != NULL) {
      total_send += _C[i]->total_send;
    }
  }
  return total_send;
}

long get_total_recv(void)
{
  long total_recv = 0;
  if (_C == 0) return total_recv;

  for (int i=0; i<_opt.channels*_opt.parties; i++) {
    if (_C[i] != NULL) {
      total_recv += _C[i]->total_recv;
    }
  }
  return total_recv;
}

static void mpc_end()
{
  PRG_free();
  precomp_tables_free();
  if (_party < 0) {
    party_free(_Parties, _opt.parties);
    return;
  }
  long total_send = 0;
  long total_recv = 0;
  long total_send_rounds = 0;
  long total_recv_rounds = 0;

  for (int i=0; i<_opt.channels*_opt.parties; i++) {
    if (_C[i] != NULL) {
      total_send += _C[i]->total_send;
      total_recv += _C[i]->total_recv;
      total_send_rounds += _C[i]->total_send_rounds;
      total_recv_rounds += _C[i]->total_recv_rounds;
      comm_close(_C[i]);
    }
  }
  printf("total send %ld bytes\n", total_send);
  printf("total recv %ld bytes\n", total_recv);
  printf("total send rounds %ld\n", total_send_rounds);
  printf("total recv rounds %ld\n", total_recv_rounds);

  for (int i = 0; i < _opt.parties * _opt.channels; ++i) {
    free(_send_queue[i]);
  }
  free(_send_queue_idx);
  free(_send_queue);

  party_free(_Parties, _opt.parties);
  free(_C);
}

typedef struct long2 {
  long x[2];
} long2;

static long2 total_comm(void)
{
  long2 ans;
  ans.x[0] = get_total_send();
  ans.x[1] = get_total_recv();
  return ans;
}

#ifndef _TESTVAR
 #define _TESTVAR
 long total_btn = 0, total_bt2 = 0;
 long total_perm = 0;

#endif

#endif
