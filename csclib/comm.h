////////////////////////////////////////////////////////
// Communication-related processing
////////////////////////////////////////////////////////


#ifndef _COMM_H
 #define _COMM_H

#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>

#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
#define SOL_TCP IPPROTO_TCP
#endif

#ifndef NEWA
 #define NEWA(p,t,n) {p = (t*)malloc((n)*sizeof(*p));if ((p)==NULL) {printf("not enough memory\n"); exit(1);};}
#endif

#ifndef NEWT
 #define NEWT(t, p) \
  t p; \
  p = (t)malloc(sizeof(*p)); \
  if ((p)==NULL) {printf("not enough memory\n"); exit(1);}
#endif

typedef struct {
  int Socket;
// Peer address
  char *dstname;
  struct sockaddr_in dstAddr;
  int dstport;

// Local address
//  char *srcname;
  struct sockaddr_in srcAddr;
  int srcport;

  long total_send, total_recv;
  long total_send_rounds, total_recv_rounds;

// recv buffer
  char *recv_buf;
  int recv_buf_size;
  int recv_buf_idx;
  int recv_buf_data_size;
}* comm;

#define BUFFER_SIZE (1<<16)


static comm comm_init_server(int recv_port)
{
  NEWT(comm, C);
  int sock;
  int ret;
  C->dstname = NULL;
////////////////////////////////////////////////////////////
// Server-side setup
////////////////////////////////////////////////////////////
  memset(&C->srcAddr, 0, sizeof(C->srcAddr));
  C->srcAddr.sin_family = AF_INET;
  C->srcAddr.sin_port = htons(recv_port);
  C->srcAddr.sin_addr.s_addr = INADDR_ANY;
  C->srcport = recv_port;

  /* Create socket */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    free(C);
    return NULL;
  }
  //printf("socket %d\n", sock);

  int opt = 1;
//  int opt = SO_REUSEADDR | SO_LINGER;
  if (_opt.comm_no_delay) {
    printf("NODELAY\n");
    if (setsockopt(sock, SOL_TCP, TCP_NODELAY, (const char *)&opt, sizeof(opt)) < 0) {
      perror("setsockopt");
      exit(1);
    }
  } else {
    printf("REUSEADDR\n");
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0) {
      perror("setsockopt");
      exit(1);
    }
  }

  /* Bind socket */
  if (bind(sock, (struct sockaddr *) &C->srcAddr, sizeof(C->srcAddr)) < 0) {
    perror("bind");
    free(C);
    exit(1);
    return NULL;
  }

  /* Allow incoming connections */
  if (listen(sock, 3) < 0) {
    perror("listen:");
  }

  unsigned int dstAddrSize = sizeof(struct sockaddr_in);
  while (1) {
    ret = accept(sock, (struct sockaddr *) &C->dstAddr, &dstAddrSize);
    if (ret != -1) {
      C->Socket = ret;
      break;
    } else {
      perror("accept");
    }
    sleep(1);
  }
  close(sock);

  C->recv_buf = NULL;
  C->recv_buf_size = 0;
  C->recv_buf_idx = 0;
  C->recv_buf_data_size = 0;
  return C;
}

static comm comm_init_client(char *dest_name, int dest_port)
{
  NEWT(comm, C);
  C->dstname = strdup(dest_name);

////////////////////////////////////////////////////////////
// Client-side setup
////////////////////////////////////////////////////////////
  while (1) {
  /* Set up sockaddr_in structure */
    memset(&C->dstAddr, 0, sizeof(C->dstAddr));
    C->dstAddr.sin_port = htons(dest_port);
    C->dstAddr.sin_family = AF_INET;
    C->dstAddr.sin_addr.s_addr = inet_addr(dest_name);

  /* Create socket */
    if ((C->Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket");
      free(C->dstname);
      free(C);
      return NULL;
    }

////////////////////////////////////////////////////////////
// Connect
////////////////////////////////////////////////////////////

    //printf("client: Trying to connect to %s:%d \n", dest_name, dest_port);
    int ret = connect(C->Socket, (struct sockaddr *) &C->dstAddr, sizeof(C->dstAddr));
    if (ret == 0) { // cannot connect until the peer starts listening
      //printf("connected to %s:%d \n", dest_name, dest_port);
      break;
    }
    if (ret < 0) {
      //perror("connect");
    }
    close(C->Socket);
    sleep(1);
  }
  //printf("done\n");
  C->recv_buf = NULL;
  C->recv_buf_size = 0;
  C->recv_buf_idx = 0;
  C->recv_buf_data_size = 0;

  return C;
}

static void comm_close(comm C)
{
  if (C == NULL) return;
  if (C->dstname != NULL) { // client
    if (shutdown(C->Socket, SHUT_WR)) perror("shutdown ");
  } else { // server
    char buf[1];
    ssize_t size = 1;
    while (size > 0) {
      size = recv(C->Socket, buf, size, 0);
      if (size > 0) {
        printf("??? recv %d\n", (int)buf[0]);
      }
    }
  }
  close(C->Socket);
//  if (closesocket(C->Socket)) perror("closesocket ");
  if (C->dstname != NULL) free(C->dstname);
  free(C);
}

static void comm_send(comm C, char *buf, int len)
{
  ssize_t size;
  size = send(C->Socket, buf, len, 0);
  if (size < 0) {
    perror("comm_send:send");
  }
  if (size < len) {
    printf("comm_send: sent %ld < %d\n", size, len);
  }
}

#if 0
static void comm_recv(comm C, char *buf, int len)
{
  ssize_t size;
  size = recv(C->Socket, buf, len, 0);
  if (size < 0) {
    perror("comm_recv:recv");
  }
  if (size < len) {
    printf("comm_recv: received %ld < %d\n", size, len);
  }
}
#endif

static int comm_recv_block(comm C, char *buffer, int len)
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  int b;

  if (C->recv_buf_data_size > 0) {
    int copy_size = len <= C->recv_buf_data_size ? len : C->recv_buf_data_size;
    memcpy(buffer, C->recv_buf + C->recv_buf_idx, copy_size);
    C->recv_buf_idx += copy_size;
    C->recv_buf_data_size -= copy_size;

    if (C->recv_buf_data_size == 0) {
      free(C->recv_buf);
      C->recv_buf = NULL;
      C->recv_buf_size = 0;
      C->recv_buf_idx = 0;
    }

    return copy_size;
  }


  FD_ZERO(&rfds);

  tv.tv_sec = 0;
  //tv.tv_usec = 500;
  tv.tv_usec = 1; // test

  b = 0;
  FD_SET(C->Socket, &rfds);
  retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
  if (retval < 0) {
    perror("select()");
  } else if(retval > 0) {
    if (FD_ISSET(C->Socket,&rfds)) {
      FD_CLR(C->Socket, &rfds);
      if (len > BUFFER_SIZE) len = BUFFER_SIZE;
      b = recv(C->Socket, buffer, len, 0); 
      if (b == -1) {
        perror("recv");
        printf("??? b %d\n", b);
      }
    }
  }
  return b;
}

static int comm_check_recv(comm C)
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  int b;

  FD_ZERO(&rfds);

  tv.tv_sec = 0;
  tv.tv_usec = 1;

  b = 0;
  FD_SET(C->Socket, &rfds);
  retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
  if (retval < 0) {
    perror("select()");
  } else if(retval > 0) {
    if (FD_ISSET(C->Socket,&rfds)) {
      FD_CLR(C->Socket, &rfds);
      if (C->recv_buf == NULL) {
        NEWA(C->recv_buf, char, BUFFER_SIZE);
        C->recv_buf_size = BUFFER_SIZE;
        C->recv_buf_idx = 0;
        C->recv_buf_data_size = 0;
      }
      int len = BUFFER_SIZE;
      int new_buf_size = C->recv_buf_idx + C->recv_buf_data_size + len;
      if (new_buf_size > C->recv_buf_size) {
        C->recv_buf = (char *)realloc(C->recv_buf, new_buf_size);
        if (C->recv_buf == NULL) {
          printf("comm_check_recv: not enough memory\n");
          exit(1);
        }
      }
      //printf("comm_check_recv: recv_buf_idx = %d recv_buf_data_size = %d len = %d\n", C->recv_buf_idx, C->recv_buf_data_size, len);
      b = recv(C->Socket, C->recv_buf + C->recv_buf_idx + C->recv_buf_data_size, len, 0); 
      //printf("comm_check_recv: received %d bytes\n", b);
      if (b == -1) {
        perror("recv");
        printf("??? b %d\n", b);
        exit(1); // if we do not exit, a buffer underflow will occur
      }
      C->recv_buf_data_size += b;
    }
  }
  return b;
}

static int comm_send_block(comm C, char *buf, int len)
{

  comm_check_recv(C); // check if there is incoming data to avoid deadlock

  if (len > BUFFER_SIZE) len = BUFFER_SIZE;
  send(C->Socket, buf, len, 0);
  return len;
}


#endif
