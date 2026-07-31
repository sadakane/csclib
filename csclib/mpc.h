////////////////////////////////////////////////////////
// General MPC processing (excluding share-specific logic)
////////////////////////////////////////////////////////

#ifndef _MPC_H
 #define _MPC_H

#include <pthread.h>
#include "comm.h" // socket-related processing


typedef void *(*thread_func)(void *);

typedef struct parties {
  char *addr;
  int port;
}* parties;

extern parties *_Parties;
extern int _party;

#ifndef _Parties_VAR
 #define _Parties_VAR
 parties *_Parties;
 int _num_parties;
 int _party = 0;
#endif


comm *_C;
#define TO_PARTY1 1
#define TO_PARTY2 2
#define FROM_PARTY1 1
#define FROM_PARTY2 2
#define TO_PARTY3 3
#define FROM_PARTY3 3
#define TO_SERVER 0
#define FROM_SERVER 0
#define TO_PAIR (3-_party)
#define FROM_PAIR (3-_party)

int *_send_queue_idx;
char **_send_queue;

typedef struct thread_param {
  pthread_t th;
  thread_func func; // unused?
  void *args;       // unused?
}* thread_param;

thread_param thread_new(thread_func func, void *args)
{
  NEWT(thread_param, param);
  param->func = func;
  param->args = args;

  int ret = pthread_create(&param->th, NULL, func, args);
  if (ret != 0) {
    printf("thread_new: ret %d\n", ret);
    exit(1);
  }
  return param;
}

void thread_end(thread_param param)
{
  int ret;
  ret = pthread_join(param->th, NULL);
  if (ret != 0) {
    printf("thread_end: ret %d\n", ret);
  }
  free(param);
}

typedef struct comm_init_args {
  int server;
  int port;
  char *dest_name;
  comm ans;
}* comm_init_args;

void *comm_init_thread(void *arg_)
{
  comm_init_args arg = (comm_init_args)arg_;
  if (arg->server) {
    arg->ans = comm_init_server(arg->port);
  } else {
    arg->ans = comm_init_client(arg->dest_name, arg->port);
  }
  return NULL;
}


int _comm_flag = 0;


static void mpc_send_channel(int party_to, void *buf, int size, int channel)
{
  if (party_to >= _opt.parties * _opt.channels) {
    printf("mpc_send: party_to %d NP %d\n", party_to, _opt.parties);
    exit(1);
  }
  if (_party < 0) return;

  comm c = _C[channel*_num_parties+party_to];

  int s = 0;
  while (s < size) {
    s += comm_send_block(c, buf+s, size-s);
  }
  c->total_send += size;
  c->total_send_rounds += 1;  // maybe this should count comm_send_block calls instead?
}
#define mpc_send(party_to, buf, size) mpc_send_channel(party_to, buf, size, 0)

static void mpc_send_flush_channel(int party_to, int channel)
{
  mpc_send_channel(party_to, _send_queue[channel*_num_parties+party_to], _send_queue_idx[channel*_num_parties+party_to], channel);
  _send_queue_idx[channel*_num_parties+party_to] = 0;
}
#define mpc_send_flush(party_to) mpc_send_flush_channel(party_to, 0)

static void mpc_send_queue_channel(int party_to, void *buf, int size, int channel)
{
  if (_send_queue_idx[channel*_num_parties+party_to] + size > BUFFER_SIZE) {
    mpc_send_flush(channel*_num_parties+party_to);
    mpc_send_channel(party_to, buf, size, channel);
    return;
  }
  int p = _send_queue_idx[channel*_num_parties+party_to];
  char *b = (char *)buf;
  for (int i=0; i<size; i++) {
    _send_queue[channel*_num_parties+party_to][p+i] = b[i];
  }
  _send_queue_idx[channel*_num_parties+party_to] += size;
}
#define mpc_send_queue(party_to, buf, size) mpc_send_queue_cannel(party_to, buf, size, 0)

static void mpc_recv_channel(int party_from, void *buf, int size, int channel)
{
  if (party_from >= _opt.parties * _opt.channels) {
    printf("mpc_recv: party_from %d NP %d\n", party_from, _opt.parties);
    exit(1);
  }
  if (_party < 0) return;
  comm c = _C[channel*_num_parties+party_from];
  int r = 0;
  while (r < size) {
    r += comm_recv_block(c, buf+r, size-r);
  }
  c->total_recv += size;
  c->total_recv_rounds += 1;  // maybe this should count comm_recv_block calls instead?
}
#define mpc_recv(party_from, buf, size) mpc_recv_channel(party_from, buf, size, 0)


static void mpc_exchange_channel(void *buf_send, void *buf_recv, int size, int channel)
{
  if (_party <= 0) return;
  comm c = _C[_num_parties*channel+TO_PAIR];
  int r = 0, s = 0;
  while (r < size || s < size) {
    if (r < size) r += comm_recv_block(c, buf_recv+r, size-r);
    if (s < size) s += comm_send_block(c, buf_send+s, size-s);
  }
  c->total_send += size;
  c->total_recv += size;
  c->total_send_rounds += 1;
  c->total_recv_rounds += 1;
}
#define mpc_exchange(buf_send, buf_recv, size) mpc_exchange_channel(buf_send, buf_recv, size, 0)




#endif
