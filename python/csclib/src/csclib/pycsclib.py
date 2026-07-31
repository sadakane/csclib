import sys
from ccsclib import *
from csclib import global_vals as _g

def Csclib_start(party):
    #print("pycsclib.py: Csclib_start party = ", party)
    _g.party = party
    Csclib_start2(party)

class Share(Csclib_core):
    def typecheck(self, x):
        if not(type(x) is Share):
            raise Exception(x, "is not share")
    def __init__(self, array=None, order=None):
    #    print("Csclib.init")
        super().__init__()
        if array != None:
            if order == None:
                print('order is not given.')
            if type(array) is list:
                self.array(array, order)
            elif type(array) is tuple:
                self.constarray(array, order)
            else:
                print('array is not list or tuple.')
        return None
    def print(self):
        print('n =', len(self), 'q =', self.order(), ':', self.get())
    def __str__(self):
        return self.str()
    def A2B(self, q):
        tmp = super().A2B(q)
        return Bits()._copy2(tmp)
    def __len__(self):
        return self.len()
    def __add__(self, other):
        if type(other) is int:
            other = Share([other] * self.len(), self.order())
            return self.vadd(other)
        elif type(other) is Share:
            return self.vadd(other)
        elif type(other) is list:
            ans = self.dup()
            #i = 0
            #while i < self.len():
            #    ans = ans.addpublic(i, other[i])
            #    i += 1
            other = Share(other, self.order())
            ans += other
            return ans
        else:
            print("add ???", self, type(self), other, type(other))
    def __radd__(self, other):
        if type(other) is int:
            other = Share([other] * self.len(), self.order())
            return self.vadd(other)
        elif type(other) is Share:
            return self.vadd(other)
        elif type(other) is list:
            ans = self.dup()
            #i = 0
            #while i < self.len():
            #    ans = ans.addpublic(i, other[i])
            #    i += 1
            other = Share(other, self.order())
            ans += other
            return ans

        else:
            print("radd ???", self, type(self), other, type(other))
    def __sub__(self, other):
        if type(other) is int:
            other = Share([other] * self.len(), self.order())
            return self.vsub(other)
        elif type(other) is Share:
            return self.vsub(other)
        elif type(other) is list:
            ans = self.dup()
            other = Share(other, self.order())
            ans -= other
            return ans
    def __rsub__(self, other):
        #print("rsub", self, type(self), other, type(other))
        if type(other) is int:
            other = Share([other] * self.len(), self.order())
            return other.vsub(self)
        elif type(other) is list:
            other = Share(other, self.order())
            ans = other - self
            return ans
        else:
            print("rsub ???", self, type(self), other, type(other))
    def __mul__(self, other):
        if type(other) is int:
            return self.smul(other)
        elif type(other) is Share:
            return self.vmul(other)
        elif type(other) is list:
            tmp = Share(other, self.order())
            return self.vmul(tmp)
        else:
            print("mul ???", self, type(self), other, type(other))
    def __rmul__(self, other):
        if type(other) is int:
            return self.smul(other)
        elif type(other) is Share:
            return self.vmul(other)
        elif type(other) is list:
            tmp = Share(other, self.order())
            return tmp.vmul(other)
        else:
            print("rmul ???", self, type(self), other, type(other))
    def __mod__(self, other):
        if type(other) is int:
            return self.smod(other)
        else:
            print("mod ???", self, type(self), other, type(other))
    def __lshift__(self, other):
        if type(other) is int:
            return self.lshift(other)
        else:
            print("lshift ???", self, type(self), other, type(other))
    def __rshift__(self, other):
        if type(other) is int:
            return self.rshift(other, self.order())
        else:
            print("rshift ???", self, type(self), other, type(other))   
    def __matmul__(self, other): # a @ b
        if type(other) is int:
            return self.insert_tail(other)
        elif type(other) is Share:
            return self.concat(other)
        elif type(other) is list:
            tmp = Share(other, self.order())
            return self.concat(tmp)
        else:
            print("matmul @ ???", self, type(self), other, type(other))
    def __imatmul__(self, other): # a @= b
        if type(other) is int:
            self.insert_tail_(other)
            return self
        elif type(other) is Share:
            self.concat_(other)
            return self
        elif type(other) is list:
            tmp = Share(other, self.order())
            self.concat_(tmp)
            return self
        else:
            #print("imatmul @ ???", self, type(self), other, type(other))
            self.concat_(other)
            return self
    def __rmatmul__(self, other): # a @ b
        if type(other) is int:
            return self.insert_head(other)
        elif type(other) is list:
            tmp = Share(other, self.order())
            #print("rmatmul tmp = ", tmp)
            return tmp.concat(self)
        else:
            print("rmatmul ???", self, type(self), other, type(other))
    def __pow__(self, other): # a ** n
        if type(other) is int:
            return self.ntimes(other)
        else:
            print("** ???", other)
    def __invert__(self): # ~a
        return self.vneg()
    def __and__(self, other): # a & b
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        return self.vmul(other)
    def __or__(self, other): # a | b
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        ap = self.vneg()
        bp = other.vneg()
        c = ap.vmul(bp)
        return c.vneg()
    def __xor__(self, other): # a ^ b
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        if self.order() == 2:
            ans = self.vadd(other)
        else:
            ans = self.XOR(other)
        return ans
    def __eq__(self, other):
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        elif type(other) is list:
            other = Share(other, self.order())
        if self.order() != 2 or other.order() != 2:
            #self2 = self.A2B(2)
            #other2 = other.A2B(2)
            #return self2 == other2
            return self.equality(other)
        else:
            return self.eq(other)
    def __ne__(self, other):
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        elif type(other) is list:
            other = Share(other, self.order())
        if self.order() != 2 or other.order() != 2:
            #self2 = self.A2B(2)
            #other2 = other.A2B(2)
            #return self2 != other2
            return self.equality(other).vneg()
        else:
            return self.eq(other).vneg()
    def __lt__(self, other):
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        elif type(other) is list:
            other = Share(other, self.order())
        if self.order() != 2 or other.order() != 2:
            #self2 = self.A2B(2)
            #other2 = other.A2B(2)
            #return self2 < other2
            return self.lessthan(other)
        else:
            return self.lt(other)
    def __ge__(self, other):
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        elif type(other) is list:
            other = Share(other, self.order())
        if self.order() != 2 or other.order() != 2:
            #self2 = self.A2B(2)
            #other2 = other.A2B(2)
            #return self2 >= other2
            return self.lessthan(self).vneg()
        else:
            return self.lt(other).vneg()
    def __gt__(self, other):
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        elif type(other) is list:
            other = Share(other, self.order())
        if self.order() != 2 or other.order() != 2:
            #self2 = self.A2B(2)
            #other2 = other.A2B(2)
            #return self2 > other2
            return other.lessthan(self)
        else:
            return other.lt(self)
    def __le__(self, other):
        if type(other) is int:
            other = Share([other]*self.len(), self.order())
        elif type(other) is list:
            other = Share(other, self.order())
        if self.order() != 2 or other.order() != 2:
            #self2 = self.A2B(2)
            #other2 = other.A2B(2)
            #return self2 <= other2
            return other.lessthan(self).vneg()
        else:
            return other.lt(self).vneg()
    def __bool__(self):
        if self.len() != 1:
            print("bool: length of self is ", self.len())
        c0 = self.reconstruct()
        c1 = c0.get()
        c = c1[0]
        return c == 1
    
    def __getitem__(self, index):
        if type(index) is int:
            return self.slice(index, index+1)
        elif type(index) is Share:
            return self.BatchAccess(index)
        elif type(index) is slice:
            start = index.start
            end = index.stop
            step = index.step
            if start == None:
                if step != None and step < 0:
                    start = len(self)-1
                else:
                    start = 0
            if end == None:
                if step != None and step < 0:
                    end = -1
                else:
                    end = len(self)
            if step == None:
                return self.slice(start, end)
            else:
                return self.slice_step(start, end, step)
        else:
            print("getitem?")
    def __setitem__(self, index, x):
        if type(index) is int: # a[i] = x
            if type(x) is int:
                return self.setpublic(index, x)
            elif type(x) is Share:
                if len(x) != 1:
                    print("setitem: length of x is ", len(x))
                return self.setshare(index, x, 0)
            elif type(x) is bool:
                c = 1 if x else 0
                return self.setpublic(index, c)
            else:
                print("setitem: type of x is ", type(x))
        elif type(index) is slice: # a[s:e] = x
            start = index.start
            end = index.stop
            if type(x) is int:
                i = start
                while i < end:
                    self.setpublic(i, x)
                    i += 1
                return self
            elif type(x) is list:
                i = start
                while i < end:
                    self.setpublic(i, x[i-start])
                    i += 1
                return self
            else:
                if start == None:
                    start = 0
                if end == None:
                    end = len(self)
                return self.setshares(start, end, x, 0)
    #def sort(self):
    #    return radix_sort(self)
    #def unzip(self, l, bs=1):
    #    return self.unzip_block(l, bs)
    def unzip(self, l, bs=None):
        if bs != None:
            if type(bs) is int:
                return super().unzip(l, [bs]*l)
            elif type(bs) is list:
                return super().unzip(l, bs)
            else:
                print("unzip ???", l, bs)
        else:
            if type(l) is int:
                return super().unzip(l, [1]*l)
            elif type(l) is list:
                return super().unzip(len(l), l)
            else:
                print("unzip ???", l)
    def _copy(self, other):
        #print("Share._copy")
        t = super()._copy(other)
        #return self #!!!
        return t

class Bits(Csclib_bits):
    def typecheck(self, x):
        if not(type(x) is Bits):
            raise Exception(x, "is not bit share")
    def __init__(self, array=None, order=None, order2=None):
        super().__init__()
        if array != None:
            if order == None or order2 == None:
                print('order is not given.')
            tmp = Share().array(array, order)
            tmp2 = tmp.A2B(order2)
            self._copy2(tmp2)
        return None
    def print(self):
        tmp = super().get()
        i = 0
        for x in tmp:
            print('i =', i, end=" ")
            Share()._copy(x).print()
            i += 1
    def __str__(self):
        return self.str()
    def get(self):
        #print("Bits.get")
        tmp = super().get()
        ans = []
        for x in tmp:
            t = Share()._copy(x)
            #print("t", t, type(t))
            ans.append(t)
        return ans
    def set(self, list):
        #print("Bits.set", file=sys.stderr)
        tmp = super().set(list)
        return Bits()._copy2(tmp)
    def new(self, list):
        tmp = super().set(list)
        return Bits()._copy(tmp)
    def __pow__(self, other):
        return Bits().new(self.get() + other.get())
    def __len__(self):
        return self.len()
    def B2A(self):
        tmp = super().B2A()
        return Share()._copy(tmp)
    def __getitem__(self, index):
        if type(index) is int:
            return self.slice(index, index+1) 
        elif type(index) is Share:
            return self.BatchAccess(index)
        elif type(index) is slice:
            start = index.start
            end = index.stop
            if start == None:
                start = 0
            if end == None:
                #end = 0
                end = len(self)
            return self.slice(start, end)
        else:
            print("getitem?")
    def __setitem__(self, index, x):
        if type(index) is int: # a[i] = x
            if type(x) is int:
                return self.setpublic(index, x)
            else:
                if x.len() != 1:
                    print("setitem: length of x is ", x.len())
                return self.setshare(index, x, 0)
        elif type(index) is slice: # a[s:e] = x
            start = index.start
            end = index.stop
            if type(x) is int:
                i = start
                while i < end:
                    self.setpublic(i, x)
                    i += 1
                return self
            elif type(x) is list:
                i = start
                while i < end:
                    self.setpublic(i, x[i-start])
                    i += 1
                return self
            else:
                if start == None:
                    start = 0
                if end == None:
                    end = len(self)
                return self.setshares(start, end, x, 0)
        else:
            print("setitem?")
    def __eq__(self, other):
        tmp = super().eq(other)
        return Share()._copy(tmp)
    def __ne__(self, other):
        tmp = super().eq(other)
        return Share()._copy(tmp.vneg())
    def __lt__(self, other):
        tmp = super().lt(other)
        return Share()._copy(tmp)
    def __ge__(self, other):
        tmp = super().lt(other)
        return Share()._copy(tmp.vneg())
    def __gt__(self, other):
        tmp = other.lt(self)
        return Share()._copy(tmp)
    def __le__(self, other):
        tmp = other.lt(self)
        return Share()._copy(tmp.vneg())
    def sort(self):
        return radix_sort_bits(self)

def blog(x):
    l = -1
    while x > 0:
        x >>= 1
        l += 1
    return l

def partition(x, low):
    q = x.order()
    B = x.A2B(2).get()
    R = B[:low]
    Q = B[low:]
    i = 0
    while i < len(R):
        R[i] = R[i].extend(1<<low)
        i += 1
    i = 0
    while i < len(Q):
        Q[i] = Q[i].extend(q)
        i += 1
    rb = Bits().new(R)
    qb = Bits().new(Q)

    L = rb.B2A()
    H = qb.B2A()
    return (H, L)

def PrefixSum(v_, bs=1):
    if bs > 1:
        return PrefixSumBlock(v_, bs)
    n = len(v_)
    v = v_.dup()
    ans = v.PrefixSum()
    return ans

def PrefixSumBlock(v, block_size):
    n = len(v) // block_size
    ans = v.dup()
    for i in range(1,n):
        ans[i*block_size:(i+1)*block_size] += ans[(i-1)*block_size:i*block_size]
    return ans

def PrefixXorBlock(v, block_size):
    n = len(v) // block_size
    ans = v.dup()
    for i in range(1,n):
        ans[i*block_size:(i+1)*block_size] ^= ans[(i-1)*block_size:i*block_size]
    return ans

def SuffixSum(v_, bs=1):
    if bs > 1:
        return SuffixSumBlock(v_, bs)

    n = len(v_)
    v = v_.dup()
    ans = v.SuffixSum()
    return ans

def SuffixSumBlock(v, block_size):
    n = len(v) // block_size
    ans = v.dup()
    for i in range(n-2, -1, -1):
        ans[i*block_size:(i+1)*block_size] += ans[(i+1)*block_size:(i+2)*block_size]
    return ans

def SuffixXorBlock(v, block_size):
    n = len(v) // block_size
    ans = v.dup()
    for i in range(n-2, -1, -1):
        ans[i*block_size:(i+1)*block_size] ^= ans[(i+1)*block_size:(i+2)*block_size]
    return ans

def Diff(v, z=0):
    n = len(v)
    return v.Diff(z)

def rank1(v_):
    n = len(v_)
    if v_.order() < n+1:
        k2 = blog(n+1-1)+1
        v = v_.shrink(2).extend(1<<k2)
    else:
        v = v_.dup()
    ans = v.rank1()
    return ans

def rank0(v_):
    n = len(v_)
    if v_.order() < n+1:
        k2 = blog(n+1-1)+1
        v = v_.shrink(2).extend(1<<k2)
    else:        
        v = v_.dup()
    ans = v.rank0()
    return ans

def Sum(v_):
    n = len(v_)
    if v_.order() < n+1:
        k2 = blog(n+1-1)+1
        v = v_.extend(1<<k2)
    else:
        v = v_.dup()
    ans = v.sum()
    return ans

def rshift(v, z=0):
    ans = z @ v[:-1]
    return ans

def lshift(v, z=0):
    ans = v[1:] @ z
    return ans

def rrotate(v):
    n = len(v)
    ans = v[n-1] @ v[:-1]
    return ans

def lrotate(v):
    n = len(v)
    ans = v[1:] @ v[0]
    return ans

def IfThenElse(f_, a, b):
    if f_.order() < a.order():
        f = f_.shrink(2).extend(a.order())
    else:
        f = f_
    n = len(f)
    if n !=len(a) or n != len(b):
        print("IfThenElse f->n =", n, "a->n = ", len(a), "b->n = ", len(b))
    ans = (a - b)*f + b
    return ans

def IfThen(f_, a):
    if f_.order() < a.order():
        f = f_.shrink(2).extend(a.order())
    else:        
        f = f_
    n = len(f)
    if n !=len(a):
        print("IfThen f->n =", n, "a->n = ", len(a))
    ans = a * f
    return ans

def MultiStableSort(g_list):
  max_k = 0
  for g in g_list:
    k = blog(len(g)-1)+1
    max_k = max(max_k, k)
  max_q = 1 << max_k+1
  S0 = Share([], max_q)
  S1 = Share([], max_q)
  G = Share([], max_q)
  for g_ in g_list:
    g = g_.shrink(2).extend(max_q)
    n = len(g)
    r0 = rank0(g)
    r1 = rank1(g)
    s0 = rshift(r0, 0)
    s1 = rshift(r1, 0)
    s1 = s1 + (r0[n-1] ** n)
    S0 @= s0
    S1 @= s1
    G @= g
  Sigma = IfThenElse(G, S1, S0)
  ans = []
  pos = 0
  for g in g_list:
    k = blog(len(g)-1)+1
    q = 1 << k
    ans.append(Sigma[pos:pos+len(g)].shrink(q))
    pos += len(g)
  return ans

def StableSort(g_):
    if type(g_) is list:
        return MultiStableSort(g_)
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    return g.StableSort()

def StableSortBlock(g_, block_size):
    n = len(g_)
    m = n // block_size
    if g_.order() < block_size+1:
        k2 = blog(block_size-1+1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    S0 = Share([], g.order())
    S1 = Share([], g.order())
    for i in range(m):
        gi = g[i*block_size:(i+1)*block_size]
        r0 = rank0(gi)
        r1 = rank1(gi)
        s0 = rshift(r0, 0)
        s1 = rshift(r1, 0)
        s1.addall(r0[block_size-1])
        S0 @= s0
        S1 @= s1
    sigma = IfThenElse(g, S1, S0)
    k = blog(block_size-1)+1
    sigma = sigma.shrink(1<<k)
    return sigma


def Perm_ID(v):
    n = len(v)
    ans = v.dup()
    ans.setperm()
    return ans

def Perm_ID2(n, q):
    w = blog(q-1)+1
    ans = Share().const(n, 0, 1<<w)
    return Perm_ID(ans)

def InvPerm(sigma):
    perm = Perm_ID(sigma)
    ans = perm.AppInvPerm(sigma)
    return ans

def RandomPerm(n):
    ans = Share().RandomPerm(n)
    return ans

def GenCycle(g_):
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    return g.GenCycle()

def Propagate(g_, v):
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    (pi, tmp_y) = GenCycle(g)

    x = v.AppInvPerm(pi)
    x[0] = 0
    v2 = v - x
    z = PrefixSum(v2)
    return z

def GroupSum(g_, v, bs=1):
    if bs > 1:
        return GroupSumBlock(g_, v, bs)
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    (tmp_x, pi_inv) = GenCycle(g)
    s = SuffixSum(v)
    t = s.dup()
    t[0] = 0
    y = t.AppInvPerm(pi_inv)
    s = s - y
    return s

def GroupSumBlock(g_, v, block_size):
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    (tmp_x, pi_inv) = GenCycle(g)
    n = len(g)
    s = v.dup()
    sum = Share([0]*block_size, v.order())
    i = n
    while i > 0:
        sum += v[(i-1)*block_size:i*block_size]
        s[(i-1)*block_size:i*block_size] = sum
        i -= 1

    t = s.dup()
    t[0:block_size] = 0
    y = t.AppInvPermBlock(block_size, pi_inv)
    s = s - y
    return s

def select1(g_):
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    return g.select1()

def select0(g_):
    if g_.order() < g_.len():
        k2 = blog(g_.len()-1)+1
        g = g_.shrink(2).extend(1<<k2)
    else:
        g = g_.dup()
    return g.select0()

#def radix_sort(a_):
#    a = a_.dup()
#    w = blog(len(a)-1)+1
#    pi = Perm_ID2(len(a), 1 << w)
#    q = a.order()
#    qb = 1 << w
#    k = 1
#    while k < q:
#        (tmp_x, tmp_y) = a.A2QB(q//k, q)
#        sigma = StableSort(tmp_y)
#        if tmp_x.order() > 1:
#            a = tmp_x.AppInvPerm(sigma)
#        pi = pi.AppInvPerm(sigma)
#        k *= 2
#    x = a_.AppPerm(pi)
#    return (x, pi)


def radix_sort_bits(a):
    d = a.depth()
    A = a.get()
    pi = StableSort(A[0])
    k = 1
    while k < d:
        ap = A[k].AppInvPerm(pi)
        sigma = StableSort(ap)
        pi = sigma.AppPerm(pi)
        k += 1
    return pi


def Grouping_bit(V):
    Vp = rshift(V, 1)
    Vp[0] -= V[0]
    ans = ~(V == Vp)
    return ans

def Grouping_bits(V):
    B = V.get()
    d = len(B)
    b = Grouping_bit(B[0])
    i = 1
    while i < d:
        g = Grouping_bit(B[i])
        b = b | g
        i += 1
    return b

def Propagate_bits(g, V):
    B = V.get()
    ans = []
    for b in B:
        tmp = Propagate(g, b)
        ans.append(Propagate(g, b))
    return Bits().new(ans)

def Grouping_name(L, q):
    V = Share([0] * L.len(), q)
    V.setperm()
    return Propagate(L, V)


def BatchAccessUnary(v, idx):
    U = len(v)
    N = len(idx)
    sigma = StableSort(idx)
    zeros = Share([0]*(N-U), v.order())
    X = v @ zeros
    Y = X.AppPerm(sigma)
    nidx = ~idx
    Z = Propagate(nidx, Y)
    W = Z.AppInvPerm(sigma)
    ans = W[U:N] # W[U:]
    return ans

def Zip(A, bs=1):
    #print("Zip A = ", A, "bs = ", bs, type(bs))
    #if type(bs) is int:
    #    bs = [bs] * len(A)
    return Share().zip(A, bs)

def AppPerm(A, sigma, bs=None):
    if type(A) is list:
        if bs == None:
            bs = [1] * len(A)
        if type(sigma) is Share:
            sigma = [sigma]
        return Share().MultiAppPerm(A, bs, sigma)
    else:
        if bs == None:
            if type(sigma) is Share:
                return A.AppPerm(sigma)
            else: # list
                tmp = Share().MultiAppPerm([A], [1], sigma)
                return tmp[0]
        else:
            if type(sigma) is Share:
                return A.AppPermBlock(bs, sigma)
            else: # list
                if type(bs) is int:# test
                    bs = [bs]
                tmp = Share().MultiAppPerm([A], bs, sigma)
                return tmp[0]

def AppInvPerm(A, sigma, bs=None):
    if type(A) is list:
        if bs == None:
            bs = [1] * len(A)
        elif type(bs) is int:
            bs = [bs] * len(A)
        if type(sigma) is Share:
            sigma = [sigma]
        return Share().MultiAppInvPerm(A, bs, sigma)
    else:
        if bs == None:
            if type(sigma) is Share:
                return A.AppInvPerm(sigma)
            else: # list
                tmp = Share().MultiAppInvPerm([A], [1], sigma)
                return tmp[0]
        else:
            #if type(bs) is int:
            #    bs = [bs]
            if type(sigma) is Share:
                return A.AppInvPermBlock(bs, sigma)
            else: # list
                if type(bs) is int:# test
                    bs = [bs]
                tmp = Share().MultiAppInvPerm([A], bs, sigma)
                return tmp[0]
