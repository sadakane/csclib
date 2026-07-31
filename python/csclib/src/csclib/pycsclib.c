// https://cpp-learning.com/python_c_api_step1/

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include "share.h"
#include "precompute2.h"

typedef struct {
  PyObject_HEAD
  share_array a;
} CsclibObject;

typedef struct {
  PyObject_HEAD
  _bits a;
} CsclibBObject;

static PyTypeObject CsclibType;
static PyTypeObject CsclibBType;

static PyModuleDef csclibmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "csclib",
    .m_doc = "csclib module.",
    .m_size = -1,
};


static void
Csclib_dealloc(CsclibObject *self)
{
  if (self->a) _free(self->a);
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static void
Csclib_Bdealloc(CsclibBObject *self)
{
  if (self->a) _free_bits(self->a);
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Csclib_start2(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  int i;
  printf("Csclib_start\n");

  _party = -1;

  if (PyArg_ParseTuple(args, "i", &i)){
    _party = i;
  }


  printf("initialize party = %d\n", _party);
  mpc_start();

  Py_RETURN_NONE;
}

static PyObject *
Csclib_precompute(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  int i;
  printf("Csclib_precompute\n");

  _party = -1;
  mpc_start();

  long n = 10000;
  int w = 30;

  FILE *fin = fopen("config.txt", "r");
  if (fin == NULL) {
    printf("config.txt is not found. precomp tables are not computed.\n");
  } else {
    precomp_read_config(n, 1<<w, fin);
    fclose(fin);
  }

  Py_RETURN_NONE;
}

static PyObject* Csclib_total_comm(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  long2 comm = total_comm();
  return Py_BuildValue("ll", comm.x[0], comm.x[1]);
}

static int
Csclib_init(CsclibObject *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

static int
Csclib_Binit(CsclibBObject *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

static PyObject* Csclib_array(PyObject* self, PyObject* args)
{
  int n, q;
  PyObject *c_list, *item;
  share_t *A;

  if (_party <= 0 || 1) {
    if (!PyArg_ParseTuple(args, "Oi", &c_list, &q)){
        printf("array\n");
        return NULL;
    }
    if (PyList_Check(c_list)) {
      n = PyList_Size(c_list);
    } else {
      return NULL;
    }
  }
  NEWA(A, share_t, n);

  if (_party <= 0 || 1) {
    for (int i = 0; i < n; i++){
      item = PyList_GetItem(c_list, i);
      share_t x = PyLong_AsLong(item);
      x = MOD(x);
      A[i] = x;
    }
  }

  //printf("Csclib_array: n=%d q=%d\n", n, q);
  _ ans = share_new(n, q, A);
  free(A);

  CsclibObject *p = (CsclibObject*)self;
  if (p->a != NULL) {
    //printf("array free ");
    _free(p->a);
  }
  p->a = ans;
  return Py_BuildValue("O", self);
}

static PyObject* Csclib_constarray(PyObject* self, PyObject* args)
{
  int n, q;
  PyObject *c_list, *item;
  share_t *A;

  if (_party <= 0 || 1) {
    if (!PyArg_ParseTuple(args, "Oi", &c_list, &q)){
        printf("array\n");
        return NULL;
    }
    if (PyList_Check(c_list)) {
      n = PyList_Size(c_list);
      NEWA(A, share_t, n);
      for (int i = 0; i < n; i++){
        item = PyList_GetItem(c_list, i);
        share_t x = PyLong_AsLong(item);
        x = MOD(x);
        A[i] = x;
      }
    } else if (PyTuple_Check(c_list)) {
      n = PyTuple_Size(c_list);
      NEWA(A, share_t, n);
      for (int i = 0; i < n; i++){
        item = PyTuple_GetItem(c_list, i);
        share_t x = PyLong_AsLong(item);
        x = MOD(x);
        A[i] = x;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Csclib_constarray: Expected a list or tuple");
      return NULL;
    }
  }

  _ ans = share_new_const(n, q, A);
  //printf("Csclib_constarray: n=%d q=%d\n", n, q); _print(ans);
  free(A);

  CsclibObject *p = (CsclibObject*)self;
  if (p->a != NULL) {
    //printf("array free ");
    _free(p->a);
  }
  p->a = ans;
  return Py_BuildValue("O", self);
}


static PyObject* Csclib_const(PyObject* self, PyObject* args)
{
  int n, v, q;
  if (!PyArg_ParseTuple(args, "iii", &n, &v, &q)){
      printf("const\n");
      return NULL;
  }
  _ ans = share_const(n, v, q);
  CsclibObject *p = (CsclibObject*)self;
  if (p->a != NULL) {
    //printf("array free ");
    _free(p->a);
  }
  p->a = ans;
  return Py_BuildValue("O", self);
}

static PyObject* Csclib_get(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  _ p = self->a;
  if (p != NULL) {
    PyObject* c_list = PyList_New(p->n);
    for (int i=0; i<p->n; i++) {
      PyList_SET_ITEM(c_list, i, PyLong_FromLong(pa_get(p->A, i)));
    }
    return Py_BuildValue("O", c_list);
  } else {
    Py_RETURN_NONE;
  }
}

#if 0
static PyObject* Csclib_set(CsclibObject* self, PyObject* args)
{
  PyObject *c_list, *item;
  int n;
  share_t q;

  if (!PyArg_ParseTuple(args, "O", &c_list)){
    printf("set\n");
    return NULL;
  }
  if (PyList_Check(c_list)) {
    n = PyList_Size(c_list);
  } else {
    return NULL;
  }
  _ p = self->a;
  if (p->n != n) {
    printf("set: p->n = %d n = %d\n", p->n, n);
    return NULL;
  }
  q = p->q;
  if (_party <= 0 || 1) {
    for (int i = 0; i < n; i++){
      item = PyList_GetItem(c_list, i);
      share_t x = PyLong_AsLong(item);
      x = MOD(x);
      pa_set(p->A, i, x);
    }
  }
  return Py_BuildValue("O", self);
}
#endif

static PyObject* Csclib_setraw(CsclibObject* self, PyObject* args)
{
  PyObject *c_list, *item;
  int n;
  share_t q;

  if (!PyArg_ParseTuple(args, "O", &c_list)){
    printf("setraw\n");
    return NULL;
  }
  _ p = self->a;
  q = p->q;
  if (PyList_Check(c_list)) {
    n = PyList_Size(c_list);
    if (p->n != n) {
      printf("setraw: p->n = %d n = %d\n", p->n, n);
      return NULL;
    }
    for (int i = 0; i < n; i++){
      item = PyList_GetItem(c_list, i);
      share_t x = PyLong_AsLong(item);
      x = MOD(x);
      pa_set(p->A, i, x);
    }
  } else if (PyTuple_Check(c_list)) {
    n = PyTuple_Size(c_list);
    if (p->n != n) {
      printf("setraw: p->n = %d n = %d\n", p->n, n);
      return NULL;
    }
    for (int i = 0; i < n; i++){
      item = PyTuple_GetItem(c_list, i);
      share_t x = PyLong_AsLong(item);
      x = MOD(x);
      pa_set(p->A, i, x);
    }
  } else {
    printf("setraw2\n");
    return NULL;
  }
  return Py_BuildValue("O", self);
}


static PyObject* Csclib_separate(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  _ p = self->a;

  _pair tmp = share_separate(p);

  CsclibObject *new_x = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_x->a = tmp.x;
  CsclibObject *new_y = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_y->a = tmp.y;

  PyObject* c_list = PyList_New(2);
  PyList_SET_ITEM(c_list, 0, (PyObject*)new_x);
  PyList_SET_ITEM(c_list, 1, (PyObject*)new_y);

  return c_list;
  //return Py_BuildValue("O", c_list); // Py_BuildValue は ref count を増やす
}




#define FUNC_NEWSHARE(func) \
static PyObject* Csclib_ ## func(PyObject* self, PyObject *Py_UNUSED(ignored)) \
{ \
  _ p = ((CsclibObject*)self)->a; \
  if (p != NULL) { \
    _ ans = func(p); \
    CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0); \
    new_obj->a = ans; \
    return (PyObject*)new_obj; \
  } else { \
    Py_RETURN_NONE; \
  } \
}

#define FUNC_NEWSHARE_O(func) \
static PyObject* Csclib_ ## func(PyObject* self, PyObject* args) \
{ \
  PyObject* arg1; \
  if (!PyArg_ParseTuple(args, "O", &arg1)){ \
    printf("Csclib_ ## func"); \
    return NULL; \
  } \
  _ a = ((CsclibObject*)self)->a; \
  _ b = ((CsclibObject*)arg1)->a; \
  _ ans = func(a, b); \
  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0); \
  new_obj->a = ans; \
  return (PyObject*)new_obj; \
}

#define FUNC_NEWSHARE_i(func) \
static PyObject* Csclib_ ## func(PyObject* self, PyObject* args) \
{ \
  int arg1; \
  if (!PyArg_ParseTuple(args, "i", &arg1)){ \
    printf("Csclib_ ## func"); \
    return NULL; \
  } \
  _ a = ((CsclibObject*)self)->a; \
  _ ans = func(a, arg1); \
  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0); \
  new_obj->a = ans; \
  return (PyObject*)new_obj; \
}

#define FUNC_O(func) \
static PyObject* Csclib_ ## func(PyObject* self, PyObject* args) \
{ \
  PyObject* arg1; \
  if (!PyArg_ParseTuple(args, "O", &arg1)){ \
    return NULL; \
  } \
  _ a = ((CsclibObject*)self)->a; \
  _ b = ((CsclibObject*)arg1)->a; \
  func(a, b); \
  Py_RETURN_NONE; \
}

#define FUNC_i(func) \
static PyObject* Csclib_ ## func(PyObject* self, PyObject* args) \
{ \
  int arg1; \
  if (!PyArg_ParseTuple(args, "i", &arg1)){ \
    return NULL; \
  } \
  _ a = ((CsclibObject*)self)->a; \
  func(a, arg1); \
  Py_RETURN_NONE; \
}

#define FUNC_(func) \
static PyObject* Csclib_ ## func(PyObject* self, PyObject* *Py_UNUSED(ignored)) \
{ \
  _ a = ((CsclibObject*)self)->a; \
  func(a); \
  Py_RETURN_NONE; \
}

static PyObject* Csclib_print(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  _ p = self->a;
  if (p != NULL) _print(p);
  Py_RETURN_NONE;
}

static PyObject* Csclib_print_str(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  _ p = self->a;
  if (p == NULL) {
    return Py_BuildValue("s", "NULL");
  }
  char *buf;
  buf = share_print_str(p);
  PyObject *tmp = Py_BuildValue("s", buf);
  free(buf);
  return tmp;
}

static PyObject* Csclib_len(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  _ p = self->a;
  if (p != NULL) {
    return Py_BuildValue("i", p->n);
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_order(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  _ p = self->a;
  if (p != NULL) {
    return Py_BuildValue("i", p->q);
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_check(CsclibObject* self, PyObject *Py_UNUSED(ignored))
{
  printf("check rec count %d\n", (int)Py_REFCNT(self));
  _ p = self->a;
  if (p != NULL) {
    _check(p);
    Py_RETURN_NONE;
  } else {
    Py_RETURN_NONE;
  }
}


static PyObject* Csclib_send(PyObject* self, PyObject* args)
{
  int party;
  if (!PyArg_ParseTuple(args, "i", &party)){
    printf("Csclib_send: Invalid arguments\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  mpc_send_share(party, a);
  Py_RETURN_NONE;
}

static PyObject* Csclib_recv(PyObject* self, PyObject* args)
{
  int party;
  if (!PyArg_ParseTuple(args, "i", &party)){
    printf("Csclib_recv: Invalid arguments\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  mpc_recv_share(party, a);
  Py_RETURN_NONE;
}


FUNC_NEWSHARE(_reconstruct)
#define Csclib_reconstruct Csclib__reconstruct

FUNC_NEWSHARE(PrefixSum)

FUNC_NEWSHARE(SuffixSum)
FUNC_NEWSHARE(rank0)
FUNC_NEWSHARE(rank1)
FUNC_NEWSHARE(select0)
FUNC_NEWSHARE(select1)
FUNC_NEWSHARE(sum)
FUNC_NEWSHARE(_dup)
FUNC_NEWSHARE(StableSort)
#define Csclib_dup Csclib__dup

FUNC_(_randomize)
#define Csclib_randomize Csclib__randomize

static PyObject* Csclib_dup_bits(PyObject* self, PyObject *Py_UNUSED(ignored))
{
  _bits p = ((CsclibBObject*)self)->a;
  if (p != NULL) {
    _bits ans = _dup_bits(p);

    CsclibBObject *new_obj = (CsclibBObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    if (new_obj == NULL) {
      printf("dup_bits null?\n");
    }
    new_obj->a = ans;
    return (PyObject*)new_obj;
  } else {
    printf("dup_bits?\n");
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_copy(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("copy\n");
    return NULL;
  }
  _ q = ((CsclibObject*)p)->a;
  if (q != NULL) {
    _ ans = _dup(q);

    CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    if (new_obj == NULL) {
      printf("copy null?\n");
    }
    new_obj->a = ans;
    return (PyObject*)new_obj;
  } else {
    printf("copy?\n");
    Py_RETURN_NONE;
  }
}


static PyObject* Csclib_setpublic(PyObject* self, PyObject* args)
{
  int i, x;
  if (!PyArg_ParseTuple(args, "ii", &i, &x)){
    printf("setpublic\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _setpublic(a, i, x);
  Py_RETURN_NONE;
}

static PyObject* Csclib_setshare(PyObject* self, PyObject* args)
{
  int i, j;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "iOi", &i, &p, &j)){
    printf("setshare\n");
    return NULL;
  }

  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _setshare(a, i, b, j);
  Py_RETURN_NONE;
}

static PyObject* Csclib_public(PyObject* self, PyObject* args)
{
  int x;
  if (!PyArg_ParseTuple(args, "i", &x)){
    printf("public\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  share_t q = a->q;

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = _const(1, x, q);
  return (PyObject*)new_obj;
}

static PyObject* Csclib_setshares(PyObject* self, PyObject* args)
{
  int is, ie, js;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "iiOi", &is, &ie, &p, &js)){
    printf("setshares\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _setshares(a, is, ie, b, js);
  Py_RETURN_NONE;
}

static PyObject* Csclib_addpublic(PyObject* self, PyObject* args)
{
  int i, x;
  if (!PyArg_ParseTuple(args, "ii", &i, &x)){
    return NULL;
  }
  PyObject* new_obj = Csclib__dup(self, NULL);

  _ a = ((CsclibObject*)new_obj)->a;
  _addpublic(a, i, x);
  return new_obj;
}

static PyObject* Csclib_addpublic_all(PyObject* self, PyObject* args)
{
  int i, x;
  if (!PyArg_ParseTuple(args, "i", &x)){
    return NULL;
  }
  PyObject* new_obj = Csclib__dup(self, NULL);

  _ a = ((CsclibObject*)new_obj)->a;
  _addpublic_all(a, x);
  return new_obj;
}

static PyObject* Csclib_subpublic(PyObject* self, PyObject* args)
{
  int i, x;
  if (!PyArg_ParseTuple(args, "ii", &i, &x)){
    return NULL;
  }
  PyObject* new_obj = Csclib__dup(self, NULL);

  _ a = ((CsclibObject*)new_obj)->a;
  _subpublic(a, i, x);
  return new_obj;
}

static PyObject* Csclib_subpublic_all(PyObject* self, PyObject* args)
{
  int i, x;
  if (!PyArg_ParseTuple(args, "i", &x)){
    return NULL;
  }
  PyObject* new_obj = Csclib__dup(self, NULL);

  _ a = ((CsclibObject*)new_obj)->a;
  _subpublic_all(a, x);
  return new_obj;
}

static PyObject* Csclib_addshare(PyObject* self, PyObject* args)
{
  int i, j;
  printf("Csclib_addshare\n");
  PyObject* p;
  if (!PyArg_ParseTuple(args, "iOi", &i, &p, &j)){
    printf("addshare\n");
    return NULL;
  }
  PyObject* new_obj = Csclib__dup(self, NULL);

  _ a = ((CsclibObject*)new_obj)->a;
  _ b = ((CsclibObject*)p)->a;
  _addshare(a, i, b, j);
  return new_obj;
}

static PyObject* Csclib_subshare(PyObject* self, PyObject* args)
{
  int i, j;
  printf("Csclib_subshare\n");
  PyObject* p;
  if (!PyArg_ParseTuple(args, "iOi", &i, &p, &j)){
    printf("subshare\n");
    return NULL;
  }
  PyObject* new_obj = Csclib_dup(self, NULL);

  _ a = ((CsclibObject*)new_obj)->a;
  _ b = ((CsclibObject*)p)->a;
  _subshare(a, i, b, j);
  return new_obj;
}

static PyObject* Csclib_mulpublic(PyObject* self, PyObject* args)
{
  int i, x;
  if (!PyArg_ParseTuple(args, "ii", &i, &x)){
    printf("mulpublic\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _mulpublic(a, i, x);
  Py_RETURN_NONE;
}

static PyObject* Csclib_slice(PyObject* self, PyObject* args)
{
  int start, end;
  if (!PyArg_ParseTuple(args, "ii", &start, &end)){
    printf("slice\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = _slice(a, start, end);
  
  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_slice_step(PyObject* self, PyObject* args)
{
  int start, end, step;
  if (!PyArg_ParseTuple(args, "iii", &start, &end, &step)){
    printf("slice\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = share_slice_step_raw(a, start, end, step);
  
  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

FUNC_NEWSHARE_O(_concat)
FUNC_O(_concat_)

FUNC_NEWSHARE_i(Diff)
FUNC_O(addall)
FUNC_(setperm)

FUNC_NEWSHARE_i(_insert_head)
FUNC_NEWSHARE_i(_insert_tail)
FUNC_i(_insert_tail_)

FUNC_NEWSHARE_O(_vadd)
FUNC_NEWSHARE_O(_vsub)
FUNC_NEWSHARE_O(_vmul)

FUNC_NEWSHARE_O(_xor)

FUNC_NEWSHARE_i(AND_rec)
FUNC_NEWSHARE_i(OR_rec)

static PyObject* Csclib_vneg(PyObject* self, PyObject *Py_UNUSED(ignored))
{
  _ a = ((CsclibObject*)self)->a;
  _ ans = _vneg(a);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

FUNC_NEWSHARE_i(python_smul)

FUNC_NEWSHARE_i(python_smod)

static PyObject* Csclib_lshift(PyObject* self, PyObject* args)
{
  int k;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "i", &k)){
    printf("Csclib_lshift\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = share_lshift_extend(a, k);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_rshift(PyObject* self, PyObject* args)
{
  int k, new_q;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "ii", &k, &new_q)){
    printf("Csclib_rshift\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = RightShift(a, k, new_q);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_split(PyObject* self, PyObject* args)
{
  int k;
  if (!PyArg_ParseTuple(args, "i", &k)){
    printf("split\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ *ans = deserialize_share_array(a, k);


  PyObject* out_list = PyList_New(k);
  for (int i=0; i<k; i++) {
    CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_obj->a = ans[i];

    PyList_SET_ITEM(out_list, i, (PyObject*)new_obj);
  }
  free(ans);
  return Py_BuildValue("O", out_list);
}

static PyObject* Csclib_serialize(PyObject* self, PyObject* args)
{
  int l, q;
  PyObject *c_list, *item;

  if (!PyArg_ParseTuple(args, "O", &c_list)){
    fprintf(stderr, "Csclib_serialize\n");
    return NULL;
  }

  int n_list = 0;
  if (PyList_Check(c_list)) {
    n_list = PyList_Size(c_list);
  } else {
    fprintf(stderr, "Csclib_serialize: c_list must be a list\n");
    return NULL;
  }

  _ *arr;
  NEWA(arr, _, n_list);
  for (int i = 0; i < n_list; i++) {
    item = PyList_GetItem(c_list, i);
    if (!PyObject_TypeCheck(item, &CsclibType)) {
      fprintf(stderr, "Csclib_serialize: type error in c_list\n");
      return NULL;
    }
    arr[i] = ((CsclibObject*)item)->a;
  }

  _ ans = serialize_share_arrays(n_list, arr);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  free(arr);
  return (PyObject*)new_obj;
}


FUNC_NEWSHARE_i(ntimes)
FUNC_NEWSHARE_i(_stretch)

static PyObject* Csclib_AppPerm(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("AppPerm\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _ ans = AppPerm(a, b);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_AppInvPerm(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("AppInvPerm\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;  
  _ ans = AppInvPerm(a, b);

  CsclibObject *new_obj = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_BlockAppPerm(PyObject* self, PyObject* args)
{
  int i;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "iO", &i, &p)){
    printf("set\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _ ans = block_AppPerm_fwd_channel(i, a, b, 0);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_multi_AppPerm_sub(PyObject* self, PyObject* args, int inverse)
{
  int l, q;
  PyObject *c_list, *bs_list, *sigma_list, *item;

  if (!PyArg_ParseTuple(args, "OOO", &c_list, &bs_list, &sigma_list)){
    fprintf(stderr, "Csclib_multi_AppPerm\n");
    return NULL;
  }

  int n_sigma = 0;
  if (PyList_Check(sigma_list)) {
    n_sigma = PyList_Size(sigma_list);
  } else {
    fprintf(stderr, "Csclib_multi_AppPerm: sigma must be a list\n");
    return NULL;
  }

  _ *sigma_array;
  NEWA(sigma_array, _, n_sigma);
  for (int i = 0; i < n_sigma; i++) {
    item = PyList_GetItem(sigma_list, i);
    if (!PyObject_TypeCheck(item, &CsclibType)) {
      fprintf(stderr, "Csclib_multi_AppPerm: type error in sigma list\n");
      return NULL;
    }
    sigma_array[i] = ((CsclibObject*)item)->a;
  }

  if (PyList_Check(c_list)) {
    l = PyList_Size(c_list);
  } else {
    return NULL;
  }
  if (PyList_Check(bs_list)) {
    int l2 = PyList_Size(bs_list);
    if (l != l2) {
      fprintf(stderr, "Csclib_multi_AppPerm: list size mismatch %d %d\n", l, l2);
      return NULL;
    }
  } else {
    return NULL;
  }
  _ *x;
  NEWA(x, _, l);
  int *bs;
  NEWA(bs, int, l);

  for (int i = 0; i < l; i++){
    item = PyList_GetItem(c_list, i);
    if (!PyObject_TypeCheck(item, &CsclibType)) {
      fprintf(stderr, "Csclib_multi_AppPerm: type error\n");
      return NULL;
    }
    x[i] = ((CsclibObject*)item)->a;
    item = PyList_GetItem(bs_list, i);
    bs[i] = PyLong_AsLong(item);
  }

  _ *ans = multi_AppPerm_channel(l, x, bs, n_sigma, sigma_array, inverse, 0);

  PyObject* out_list = PyList_New(l);
  for (int i=0; i<l; i++) {
    CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_obj->a = ans[i];

    PyList_SET_ITEM(out_list, i, (PyObject*)new_obj);
  }
  free(x);
  free(sigma_array);
  free(bs);
  free(ans);
  return Py_BuildValue("O", out_list);
}

static PyObject* Csclib_multi_AppPerm(PyObject* self, PyObject* args)
{
  return Csclib_multi_AppPerm_sub(self, args, 0);
}

static PyObject* Csclib_multi_AppInvPerm(PyObject* self, PyObject* args)
{
  return Csclib_multi_AppPerm_sub(self, args, 1);
}

static PyObject* Csclib_BlockAppInvPerm(PyObject* self, PyObject* args)
{
  int i;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "iO", &i, &p)){
    printf("set\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _ ans = block_AppPerm_inverse_channel(i, a, b, 0);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_GroupSum(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("GroupSum\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ g = ((CsclibObject*)p)->a;
  _ ans = GroupSum(g, a);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_GroupSumBlock(PyObject* self, PyObject* args)
{
  int bs;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "Oi", &p, &bs)){
    printf("GroupSumBlock\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ g = ((CsclibObject*)p)->a;
  _ ans = GroupSumBlock(g, a, bs);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_AppPerm_bits(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("AppPerm_bits\n");
    return NULL;
  }
  _bits a = ((CsclibBObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _bits ans = AppPerm_bits(a, b);

  CsclibBObject *new_obj = (CsclibBObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_AppInvPerm_bits(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("AppInvPerm_bits\n");
    return NULL;
  }
  _bits a = ((CsclibBObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;  
  _bits ans = AppInvPerm_bits(a, b);

  CsclibBObject *new_obj = (CsclibBObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}



static PyObject* Csclib_A2QB(CsclibObject* self, PyObject* args)
{
  int q, qb;
  _ p = self->a;
  if (p != NULL) {
    if (!PyArg_ParseTuple(args, "ii", &q, &qb)){
      printf("A2QB\n");
      return NULL;
    }
    _pair tmp = _A2QB(p, q, qb);

    CsclibObject *new_x = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_x->a = tmp.x;
    CsclibObject *new_y = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_y->a = tmp.y;

    PyObject* c_list = PyList_New(2);
    PyList_SET_ITEM(c_list, 0, (PyObject*)new_x);
    PyList_SET_ITEM(c_list, 1, (PyObject*)new_y);
    return c_list;
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_B2A(PyObject* self, PyObject* args)
{
  int q;
  if (!PyArg_ParseTuple(args, "i", &q)){
    printf("B2A\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = B2A(a, q);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_B2A_bits(PyObject* self, PyObject* args)
{
  _bits a = ((CsclibBObject*)self)->a;
  _ ans = _B2A_bits(a);

  CsclibObject *new_obj = (CsclibObject *)CsclibType.tp_alloc(&CsclibType, 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_A2B(PyObject* self, PyObject* args)
{
  int q;
  if (!PyArg_ParseTuple(args, "i", &q)){
    printf("A2B\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _bits ans = _A2B(a, q);

  CsclibBObject *new_obj = (CsclibBObject *)CsclibBType.tp_alloc(&CsclibBType, 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_GenCycle(CsclibObject* self, PyObject* *Py_UNUSED(ignored))
{
  _ p = self->a;
  if (p != NULL) {
    _pair tmp = GenCycle(p);

    CsclibObject *new_x = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_x->a = tmp.x;
    CsclibObject *new_y = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_y->a = tmp.y;

    PyObject* c_list = PyList_New(2);
    PyList_SET_ITEM(c_list, 0, (PyObject*)new_x);
    PyList_SET_ITEM(c_list, 1, (PyObject*)new_y);
    return Py_BuildValue("O", c_list);
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_RadixSort(CsclibObject* self, PyObject* *Py_UNUSED(ignored))
{
  _ p = self->a;
  if (p != NULL) {
    _pair tmp = share_radix_sort(p);

    CsclibObject *new_x = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_x->a = tmp.x;
    CsclibObject *new_y = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_y->a = tmp.y;

    PyObject* c_list = PyList_New(2);
    PyList_SET_ITEM(c_list, 0, (PyObject*)new_x);
    PyList_SET_ITEM(c_list, 1, (PyObject*)new_y);
    return Py_BuildValue("O", c_list);
  } else {
    Py_RETURN_NONE;
  }
}



static PyObject* Csclib_extend(PyObject* self, PyObject* args)
{
  int qb;
  if (!PyArg_ParseTuple(args, "i", &qb)){
    printf("extend\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = _extend(a, qb);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_shrink(PyObject* self, PyObject* args)
{
  int qb;
  if (!PyArg_ParseTuple(args, "i", &qb)){
    printf("shrink\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = _shrink(a, qb);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}


static PyObject* Csclib_equality_bit(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("vadd\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _ ans = Equality_bit(a, b);
  
  CsclibObject *new_obj = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_equality(PyObject* self, PyObject* args)
{
//  int n, v, q;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("vadd\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _ ans = EqualityConst2_channel(a, b, 2, 0);
  
  CsclibObject *new_obj = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_equality_bits(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("vadd\n");
    return NULL;
  }
  _bits a = ((CsclibBObject*)self)->a;
  _bits b = ((CsclibBObject*)p)->a;
  _ ans = Equality_bits(a, b);
  
  CsclibObject *new_obj = (CsclibObject *)CsclibType.tp_alloc(&CsclibType, 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_lessthan_bit(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("vadd\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _ ans = LessThan_bit(a, b);
  
  CsclibObject *new_obj = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_lessthan(PyObject* self, PyObject* args)
{
//  int n, v, q;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("vadd\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ b = ((CsclibObject*)p)->a;
  _ ans_tmp = Comparison2_channel(b, a, 0);
  _ ans = vneg(ans_tmp);
  _free(ans_tmp);
  
  CsclibObject *new_obj = (CsclibObject*)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_lessthan_bits(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("vadd\n");
    return NULL;
  }
  _bits a = ((CsclibBObject*)self)->a;
  _bits b = ((CsclibBObject*)p)->a;
  _ ans = LessThan_bits(a, b);
  
  CsclibObject *new_obj = (CsclibObject *)CsclibType.tp_alloc(&CsclibType, 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_BatchAccess(PyObject* self, PyObject* args)
{
  int qb;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("BatchAccess\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ idx = ((CsclibObject*)p)->a;
  _ ans = BatchAccess(a, idx);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_BatchAccess_bits(PyObject* self, PyObject* args)
{
  int qb;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("BatchAccess\n");
    return NULL;
  }
  _bits a = ((CsclibBObject*)self)->a;
  _ idx = ((CsclibObject*)p)->a;
  _bits ans = BatchAccess_bits(a, idx);

  CsclibBObject *new_obj = (CsclibBObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_Unary(PyObject* self, PyObject* args)
{
  int qb;
  int U;
  if (!PyArg_ParseTuple(args, "i", &U)){
    printf("Unary\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = Unary(a, U);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_Unitv(PyObject* self, PyObject* args)
{
  int qb;
  int new_q;
  if (!PyArg_ParseTuple(args, "i", &new_q)){
    printf("Unary\n");
    return NULL;
  }
  _ a = ((CsclibObject*)self)->a;
  _ ans = Unitv2(a, new_q);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

static PyObject* Csclib_zip(PyObject* self, PyObject* args)
{
  int l, block_size = 1;
  PyObject *c_list, *item;

  PyObject *bs_list = NULL;

  if (!PyArg_ParseTuple(args, "OO", &c_list, &bs_list)){
    //printf("zip 1\n");
    bs_list = NULL;
    if (!PyArg_ParseTuple(args, "Oi", &c_list, &block_size)){
      if (!PyArg_ParseTuple(args, "O", &c_list)){
        printf("zip\n");
        return NULL;
      }
    }
  }
  if (!PyList_Check(bs_list)) {
    bs_list = NULL;
    if (!PyArg_ParseTuple(args, "Oi", &c_list, &block_size)){
      printf("zip 2\n");
      return NULL;
    }
  }

  if (PyList_Check(c_list)) {
    l = PyList_Size(c_list);
  } else {
    return NULL;
  }
  _ *x;
  NEWA(x, _, l);

  for (int i = 0; i < l; i++){
    item = PyList_GetItem(c_list, i);
    if (!PyObject_TypeCheck(item, &CsclibType)) {
      printf("zip type error\n");
      return NULL;
    }
    x[i] = ((CsclibObject*)item)->a;
  }
  _ ans;
  if (bs_list != NULL) {
    int *bs_array;
    if (PyList_Check(bs_list)) {
      int l2 = PyList_Size(bs_list);
      if (l != l2) {
        fprintf(stderr, "Csclib_zip: list size mismatch %d %d\n", l, l2);
        return NULL;
      }
      NEWA(bs_array, int, l);
      for (int i = 0; i < l; i++) {
        item = PyList_GetItem(bs_list, i);
        block_size = PyLong_AsLong(item);
        if (block_size <= 0) {
          fprintf(stderr, "Csclib_zip: block size must be positive\n");
          return NULL;
        }
        bs_array[i] = block_size;
      }
    } else {
      fprintf(stderr, "Csclib_zip: bs_list must be a list\n");
      return NULL;
    }
    ans = _zip_block2(l, x, bs_array);
    free(bs_array);
  } else {
    ans = _zip_block(l, x, block_size);
  }
  free(x);

  CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}

#if 0
static PyObject* Csclib_unzip(PyObject* self, PyObject* args)
{
  int l, block_size = 1;
  PyObject *c_list, *item;

  if (!PyArg_ParseTuple(args, "ii", &l, &block_size)){
    if (!PyArg_ParseTuple(args, "i", &l)){
      printf("unzip\n");
      return NULL;
    }
  }

  _ *x;
  _ z = ((CsclibObject*)self)->a;

  x = _unzip_block(z, l, block_size);

  PyObject* out_list = PyList_New(l);
  for (int i=0; i<l; i++) {
    CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_obj->a = x[i];

    PyList_SET_ITEM(out_list, i, (PyObject*)new_obj);
  }
  free(x);
  return Py_BuildValue("O", out_list);
}
#else
static PyObject* Csclib_unzip(PyObject* self, PyObject* args)
{
  int l, block_size = 1;
  PyObject *c_list, *item;

  PyObject *bs_list = NULL;
  int *bs_array = NULL;

  if (!PyArg_ParseTuple(args, "iO", &l, &bs_list)) {
    bs_list = NULL;
    if (!PyArg_ParseTuple(args, "ii", &l, &block_size)) {
      printf("unzip 3\n");
      bs_list = NULL;
      if (!PyArg_ParseTuple(args, "i", &l)){
        printf("unzip\n");
        return NULL;
      }
    }
  }
  //printf("unzip l=%d\n", l);

  if (bs_list != NULL) {
    //printf("bs_list\n");
    if (PyList_Check(bs_list)) {
      int l2 = PyList_Size(bs_list);
      if (l != l2) {
        fprintf(stderr, "Csclib_unzip: list size mismatch %d %d\n", l, l2);
        return NULL;
      }
      NEWA(bs_array, int, l);
      for (int i = 0; i < l; i++) {
        item = PyList_GetItem(bs_list, i);
        block_size = PyLong_AsLong(item);
        if (block_size <= 0) {
          fprintf(stderr, "Csclib_zip: block size must be positive\n");
          return NULL;
        }
        bs_array[i] = block_size;
        //printf("bs_array[%d] = %d\n", i, bs_array[i]);
      }
    } else {
      //printf("bs_list is not a list\n");
      if (!PyArg_ParseTuple(args, "i", &l)){
        printf("unzip\n");
        return NULL;
      }
    } 
  }

  _ *x;
  _ z = ((CsclibObject*)self)->a;

  if (bs_array != NULL) {
    x = _unzip_block2(z, l, bs_array);
    free(bs_array);
  } else {
    x = _unzip_block(z, l, block_size);
  }

  PyObject* out_list = PyList_New(l);
  for (int i=0; i<l; i++) {
    //_print(x[i]);
    CsclibObject *new_obj = (CsclibObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    new_obj->a = x[i];

    PyList_SET_ITEM(out_list, i, (PyObject*)new_obj);
  }
  free(x);
  return Py_BuildValue("O", out_list);
}
#endif

static PyObject* Csclib_Bprint(CsclibBObject* self, PyObject *Py_UNUSED(ignored))
{
  _bits p = self->a;
  if (p != NULL) _print_bits(p);
  Py_RETURN_NONE;
}

static PyObject* Csclib_Bprint_str(CsclibBObject* self, PyObject *Py_UNUSED(ignored))
{
  _bits p = self->a;
  char *buf;
  buf = share_print_bits_str(p);
  return Py_BuildValue("s", buf);
}

static PyObject* Csclib_Blen(CsclibBObject* self, PyObject *Py_UNUSED(ignored))
{
  _bits p = self->a;
  if (p != NULL) {
    return Py_BuildValue("i", p->a[0]->n);
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_Border(CsclibBObject* self, PyObject *Py_UNUSED(ignored))
{
  _bits p = self->a;
  if (p != NULL) {
    return Py_BuildValue("i", p->a[0]->q);
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_Bdepth(CsclibBObject* self, PyObject *Py_UNUSED(ignored))
{
  _bits p = self->a;
  if (p != NULL) {
    return Py_BuildValue("i", p->d);
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_Bget0(CsclibBObject* self, PyObject *args)
{
  int i;
  if (!PyArg_ParseTuple(args, "i", &i)){
    printf("get\n");
    return NULL;
  }
  _bits p = (_bits)self->a;
  if (p != NULL) {
    if (i < 0 || i >= p->d) {
      printf("get i=%d d=%d\n", i, p->d);
      return NULL;
    }
    CsclibObject *new_obj = (CsclibObject *)CsclibType.tp_alloc(&CsclibType, 0);
    new_obj->a = _dup(p->a[i]);
    return (PyObject*)new_obj;
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_Bget(CsclibBObject* self, PyObject *Py_UNUSED(ignored))
{
  int i;
  _bits p = (_bits)self->a;

  PyObject* c_list = PyList_New(p->d);
  for (int i=0; i<p->d; i++) {
    CsclibObject *new_obj = (CsclibObject *)CsclibType.tp_alloc(&CsclibType, 0);
    new_obj->a = _dup(p->a[i]);
    PyList_SET_ITEM(c_list, i, Py_BuildValue("O", new_obj));
  }
  return Py_BuildValue("O", c_list);
}

static PyObject* Csclib_Bset0(CsclibBObject* self, PyObject *args)
{
  int i;
  PyObject *q;
  if (!PyArg_ParseTuple(args, "iO", &i, &q)){
    printf("set\n");
    return NULL;
  }
  _bits p = (_bits)self->a;
  if (p != NULL) {
    if (i < 0 || i >= p->d) {
      printf("set i=%d d=%d\n", i, p->d);
      return NULL;
    }
    p->a[i] = ((_bits)q)->a[i];
    Py_RETURN_NONE;
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_Bset(CsclibBObject* self, PyObject *args)
{
  int d;
  PyObject *c_list;
  if (!PyArg_ParseTuple(args, "O", &c_list)){
    printf("set\n");
    return NULL;
  }
  if (PyList_Check(c_list)) {
    d = PyList_Size(c_list);
  } else {
    return NULL;
  }
  NEWT(_bits, ans);
  ans->d = d;
  NEWA(ans->a, _, d);

  for (int i = 0; i < d; i++){
    CsclibObject *item = (CsclibObject *)PyList_GetItem(c_list, i);
    ans->a[i] = _dup(item->a);
  }
  CsclibBObject *new_obj = (CsclibBObject *)CsclibBType.tp_alloc(&CsclibBType, 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;

}


static PyObject* Csclib_copy_bits(PyObject* self, PyObject* args)
{
  PyObject* p;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("copy\n");
    return NULL;
  }
  _bits q = ((CsclibBObject*)p)->a;
  if (q != NULL) {
    _bits ans = _dup_bits(q);

    CsclibBObject *new_obj = (CsclibBObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
    if (new_obj == NULL) {
      printf("copy_bits null?\n");
    }
    new_obj->a = ans;
    return (PyObject*)new_obj;
  } else {
    printf("copy_bits?\n");
    Py_RETURN_NONE;
  }
}

static PyObject* Csclib_copy_bits2(PyObject* self, PyObject* args)
{
  PyObject* p;
  CsclibBObject* self2 = (CsclibBObject*)self;
  if (!PyArg_ParseTuple(args, "O", &p)){
    printf("copy\n");
    return NULL;
  }
  _bits q = ((CsclibBObject*)p)->a;

  if (q != NULL) {
    _bits ans = _dup_bits(q);

    if (self2->a != NULL) {
      printf("copy_bits2 free\n");
      _print_bits(self2->a);
      _free_bits(self2->a);
    }
    self2->a = ans;
    Py_INCREF(self);
    return self;
  } else {
    printf("copy_bits2?\n");
    Py_RETURN_NONE;
  }
}


static PyObject* Csclib_setpublic_bits(PyObject* self, PyObject* args)
{
  int i, x;
  if (!PyArg_ParseTuple(args, "ii", &i, &x)){
    printf("setpublic\n");
    return NULL;
  }
  _bits a = ((CsclibBObject*)self)->a;
  _setpublic_bits(a, i, x);
  Py_RETURN_NONE;
}

static PyObject* Csclib_const_bits(PyObject* self, PyObject* args)
{
  int n, v, q, d;
  if (!PyArg_ParseTuple(args, "iiii", &n, &v, &q, &d)){
      printf("const_bits\n");
      return NULL;
  }
  _bits ans = share_const_bits(n, v, q, d);
  ((CsclibBObject*)self)->a = ans;
  return Py_BuildValue("O", self);
}

static PyObject* Csclib_RandomPerm(PyObject* self, PyObject* args)
{
  int n;
  if (!PyArg_ParseTuple(args, "i", &n)){
      printf("RandomPerm\n");
      return NULL;
  }
  _ ans = RandomPerm(n);
  ((CsclibObject*)self)->a = ans;
  return Py_BuildValue("O", self);
}

static PyObject* Csclib_setshare_bits(PyObject* self, PyObject* args)
{
  int i, j;
  PyObject* p;
  if (!PyArg_ParseTuple(args, "iOi", &i, &p, &j)){
    printf("setshare\n");
    return NULL;
  }

  _bits a = ((CsclibBObject*)self)->a;
  _bits b = ((CsclibBObject*)p)->a;
  _setshare_bits(a, i, b, j);
  Py_RETURN_NONE;
}

static PyObject* Csclib_slice_bits(PyObject* self, PyObject* args)
{
  int start, end;
  if (!PyArg_ParseTuple(args, "ii", &start, &end)){
    printf("slice\n");
    return NULL;
  }
  _bits a = ((CsclibBObject*)self)->a;
  _bits ans = _slice_bits(a, start, end);
  
  CsclibBObject *new_obj = (CsclibBObject *)Py_TYPE(self)->tp_alloc(Py_TYPE(self), 0);
  new_obj->a = ans;
  return (PyObject*)new_obj;
}


static PyMethodDef Csclib_methods[] = {
    {"total_comm", (PyCFunction)Csclib_total_comm, METH_NOARGS},
    {"send", (PyCFunction)Csclib_send, METH_VARARGS},
    {"recv", (PyCFunction)Csclib_recv, METH_VARARGS},
    {"array", (PyCFunction)Csclib_array, METH_VARARGS},
    {"constarray", (PyCFunction)Csclib_constarray, METH_VARARGS},
    {"const", (PyCFunction)Csclib_const, METH_VARARGS},
    {"get", (PyCFunction)Csclib_get, METH_NOARGS},
    //{"set", (PyCFunction)Csclib_set, METH_VARARGS},
    {"set", (PyCFunction)Csclib_setraw, METH_VARARGS},
    {"getraw", (PyCFunction)Csclib_get, METH_NOARGS},
    {"setraw", (PyCFunction)Csclib_setraw, METH_VARARGS},
    {"separate", (PyCFunction)Csclib_separate, METH_NOARGS},
    {"print", (PyCFunction)Csclib_print, METH_NOARGS},
    {"str", (PyCFunction)Csclib_print_str, METH_NOARGS},
    {"len", (PyCFunction)Csclib_len, METH_NOARGS},
    {"order", (PyCFunction)Csclib_order, METH_NOARGS},
    {"check", (PyCFunction)Csclib_check, METH_NOARGS},
    {"reconstruct", (PyCFunction)Csclib_reconstruct, METH_NOARGS},
    {"reveal", (PyCFunction)Csclib_reconstruct, METH_NOARGS},
    {"randomize", (PyCFunction)Csclib_randomize, METH_NOARGS},
    {"_dup", (PyCFunction)Csclib__dup, METH_NOARGS},
    {"_copy", (PyCFunction)Csclib_copy, METH_VARARGS},
    {"public", (PyCFunction)Csclib_public, METH_VARARGS},
    {"setpublic", (PyCFunction)Csclib_setpublic, METH_VARARGS},
    {"setshare", (PyCFunction)Csclib_setshare, METH_VARARGS},
    {"setshares", (PyCFunction)Csclib_setshares, METH_VARARGS},
    {"addpublic", (PyCFunction)Csclib_addpublic, METH_VARARGS},
    {"subpublic", (PyCFunction)Csclib_subpublic, METH_VARARGS},
    {"addpublicall", (PyCFunction)Csclib_addpublic_all, METH_VARARGS},
    {"subpublicall", (PyCFunction)Csclib_subpublic_all, METH_VARARGS},
    {"addshare", (PyCFunction)Csclib_addshare, METH_VARARGS},
    {"subshare", (PyCFunction)Csclib_subshare, METH_VARARGS},
    {"mulpublic", (PyCFunction)Csclib_mulpublic, METH_VARARGS},
    {"slice", (PyCFunction)Csclib_slice, METH_VARARGS},
    {"slice_step", (PyCFunction)Csclib_slice_step, METH_VARARGS},
    {"insert_head", (PyCFunction)Csclib__insert_head, METH_VARARGS},
    {"insert_tail", (PyCFunction)Csclib__insert_tail, METH_VARARGS},
    {"insert_tail_", (PyCFunction)Csclib__insert_tail_, METH_VARARGS},
    {"append", (PyCFunction)Csclib__insert_tail_, METH_VARARGS},
    {"vadd", (PyCFunction)Csclib__vadd, METH_VARARGS},
    {"vsub", (PyCFunction)Csclib__vsub, METH_VARARGS},
    {"vmul", (PyCFunction)Csclib__vmul, METH_VARARGS},
    {"vneg", (PyCFunction)Csclib_vneg, METH_VARARGS},
    {"smul", (PyCFunction)Csclib_python_smul, METH_VARARGS},
    {"smod", (PyCFunction)Csclib_python_smod, METH_VARARGS},
    {"XOR", (PyCFunction)Csclib__xor, METH_VARARGS},
    {"AND_rec", (PyCFunction)Csclib_AND_rec, METH_VARARGS},
    {"OR_rec", (PyCFunction)Csclib_OR_rec, METH_VARARGS},
    {"lshift", (PyCFunction)Csclib_lshift, METH_VARARGS},
    {"rshift", (PyCFunction)Csclib_rshift, METH_VARARGS},
    {"ntimes", (PyCFunction)Csclib_ntimes, METH_VARARGS},
    {"zip", (PyCFunction)Csclib_zip, METH_VARARGS},
    {"unzip", (PyCFunction)Csclib_unzip, METH_VARARGS},
    {"AppPerm", (PyCFunction)Csclib_AppPerm, METH_VARARGS},
    {"AppInvPerm", (PyCFunction)Csclib_AppInvPerm, METH_VARARGS},
    {"MultiAppPerm", (PyCFunction)Csclib_multi_AppPerm, METH_VARARGS},
    {"MultiAppInvPerm", (PyCFunction)Csclib_multi_AppInvPerm, METH_VARARGS},
    {"AppPermBlock", (PyCFunction)Csclib_BlockAppPerm, METH_VARARGS},
    {"AppInvPermBlock", (PyCFunction)Csclib_BlockAppInvPerm, METH_VARARGS},
    {"GroupSum", (PyCFunction)Csclib_GroupSum, METH_VARARGS},
    {"GroupSumBlock", (PyCFunction)Csclib_GroupSumBlock, METH_VARARGS},
    {"RandomPerm", (PyCFunction)Csclib_RandomPerm, METH_VARARGS},
    {"A2QB", (PyCFunction)Csclib_A2QB, METH_VARARGS},
    {"B2A", (PyCFunction)Csclib_B2A, METH_VARARGS},
    {"A2B", (PyCFunction)Csclib_A2B, METH_VARARGS},
    {"extend", (PyCFunction)Csclib_extend, METH_VARARGS},
    {"shrink", (PyCFunction)Csclib_shrink, METH_VARARGS},
    {"stretch", (PyCFunction)Csclib__stretch, METH_VARARGS},
    {"split", (PyCFunction)Csclib_split, METH_VARARGS},
    {"serialize", (PyCFunction)Csclib_serialize, METH_VARARGS},
    {"BatchAccess", (PyCFunction)Csclib_BatchAccess, METH_VARARGS},
    {"Unary", (PyCFunction)Csclib_Unary, METH_VARARGS},
    {"Unitv", (PyCFunction)Csclib_Unitv, METH_VARARGS},
    {"eq", (PyCFunction)Csclib_equality_bit, METH_VARARGS},
    {"lt", (PyCFunction)Csclib_lessthan_bit, METH_VARARGS},
    {"equality", (PyCFunction)Csclib_equality, METH_VARARGS},
    {"lessthan", (PyCFunction)Csclib_lessthan, METH_VARARGS},
    {"PrefixSum", (PyCFunction)Csclib_PrefixSum, METH_NOARGS},
    {"SuffixSum", (PyCFunction)Csclib_SuffixSum, METH_NOARGS},
    {"rank0", (PyCFunction)Csclib_rank0, METH_NOARGS},
    {"rank1", (PyCFunction)Csclib_rank1, METH_NOARGS},
    {"select0", (PyCFunction)Csclib_select0, METH_NOARGS},
    {"select1", (PyCFunction)Csclib_select1, METH_NOARGS},
    {"sum", (PyCFunction)Csclib_sum, METH_NOARGS},
    {"dup", (PyCFunction)Csclib__dup, METH_NOARGS},
    {"Diff", (PyCFunction)Csclib_Diff, METH_VARARGS},
    {"concat", (PyCFunction)Csclib__concat, METH_VARARGS},
    {"concat_", (PyCFunction)Csclib__concat_, METH_VARARGS},
    {"addall", (PyCFunction)Csclib_addall, METH_VARARGS},
    {"setperm", (PyCFunction)Csclib_setperm, METH_NOARGS},
    {"StableSort", (PyCFunction)Csclib_StableSort, METH_NOARGS},
    {"sort", (PyCFunction)Csclib_RadixSort, METH_NOARGS},
    {"GenCycle", (PyCFunction)Csclib_GenCycle, METH_NOARGS},
    {NULL}  /* Sentinel */
};

static PyMemberDef Csclib_members[] = {
    {NULL}  /* Sentinel */
};

static PyGetSetDef Csclib_getsetters[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject CsclibType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "csclib.Csclib_core",
    .tp_doc = PyDoc_STR("Csclib core objects"),
    .tp_basicsize = sizeof(CsclibObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Csclib_init,
    .tp_dealloc = (destructor) Csclib_dealloc,
    .tp_methods = Csclib_methods,
    .tp_members = Csclib_members,
};


static PyMethodDef Csclib_Bmethods[] = {
    {"len", (PyCFunction)Csclib_Blen, METH_NOARGS},
    {"order", (PyCFunction)Csclib_Border, METH_NOARGS},
    {"depth", (PyCFunction)Csclib_Bdepth, METH_NOARGS},
    {"get", (PyCFunction)Csclib_Bget, METH_NOARGS},
    {"set", (PyCFunction)Csclib_Bset, METH_VARARGS},
    {"print", (PyCFunction)Csclib_Bprint, METH_NOARGS},
    {"str", (PyCFunction)Csclib_Bprint_str, METH_NOARGS},
    {"setpublic", (PyCFunction)Csclib_setpublic_bits, METH_VARARGS},
    {"const", (PyCFunction)Csclib_const_bits, METH_VARARGS},
    {"setshare", (PyCFunction)Csclib_setshare_bits, METH_VARARGS},
    {"slice", (PyCFunction)Csclib_slice_bits, METH_VARARGS},
    {"B2A", (PyCFunction)Csclib_B2A_bits, METH_VARARGS},
    {"AppPerm", (PyCFunction)Csclib_AppPerm_bits, METH_VARARGS},
    {"AppInvPerm", (PyCFunction)Csclib_AppInvPerm_bits, METH_VARARGS},
    {"dup", (PyCFunction)Csclib_dup_bits, METH_NOARGS},
    {"_copy", (PyCFunction)Csclib_copy_bits, METH_VARARGS},
    {"_copy2", (PyCFunction)Csclib_copy_bits2, METH_VARARGS},
    {"BatchAccess", (PyCFunction)Csclib_BatchAccess_bits, METH_VARARGS},
    {"eq", (PyCFunction)Csclib_equality_bits, METH_VARARGS},
    {"lt", (PyCFunction)Csclib_lessthan_bits, METH_VARARGS},
    {NULL}  /* Sentinel */
};

static PyMemberDef Csclib_Bmembers[] = {
    {NULL}  /* Sentinel */
};

static void Csclib_Bdealloc(CsclibBObject *self);
static int Csclib_Binit(CsclibBObject *self, PyObject *args, PyObject *kwds);

static PyTypeObject CsclibBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ccsclib.Csclib_bits",
    .tp_doc = PyDoc_STR("Csclib bits objects"),
    .tp_basicsize = sizeof(CsclibBObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Csclib_Binit,
    .tp_dealloc = (destructor) Csclib_Bdealloc,
    .tp_methods = Csclib_Bmethods,
    .tp_members = Csclib_Bmembers,
};

static PyTypeObject CsclibType_start = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ccsclib.start",
    .tp_doc = PyDoc_STR("ccsclib.start objects"),
    .tp_basicsize = sizeof(CsclibObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Csclib_start2,
};

static PyTypeObject CsclibType_precompute = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ccsclib.precompute",
    .tp_doc = PyDoc_STR("ccsclib.precompute objects"),
    .tp_basicsize = sizeof(CsclibObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Csclib_precompute,
};


PyMODINIT_FUNC
PyInit_ccsclib(void)
{
  PyObject *m;

  m = PyModule_Create(&csclibmodule);
  if (m == NULL)
    return NULL;

  if (PyType_Ready(&CsclibType) < 0)
    return NULL;
  Py_INCREF(&CsclibType);
  if (PyModule_AddObject(m, "Csclib_core", (PyObject *) &CsclibType) < 0) {
      Py_DECREF(&CsclibType);
      Py_DECREF(m);
      return NULL;
  }

  if (PyType_Ready(&CsclibBType) < 0)
    return NULL;
  Py_INCREF(&CsclibBType);
  if (PyModule_AddObject(m, "Csclib_bits", (PyObject *) &CsclibBType) < 0) {
      Py_DECREF(&CsclibBType);
      Py_DECREF(m);
      return NULL;
  }

  if (PyType_Ready(&CsclibType_start) < 0)
    return NULL;
  Py_INCREF(&CsclibType_start);
  if (PyModule_AddObject(m, "Csclib_start2", (PyObject *) &CsclibType_start) < 0) {
      Py_DECREF(&CsclibType_start);
      Py_DECREF(m);
      return NULL;
  }

  if (PyType_Ready(&CsclibType_precompute) < 0)
    return NULL;
  Py_INCREF(&CsclibType_precompute);
  if (PyModule_AddObject(m, "Csclib_precompute", (PyObject *) &CsclibType_precompute) < 0) {
      Py_DECREF(&CsclibType_precompute);
      Py_DECREF(m);
      return NULL;
  }

  return m;
}
