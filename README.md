# csclib
A secure computation library based on secret sharing.
Two-party additive sharing, three-party Shamir's sharing, and three-party replicated sharing are supported.

## Protocols
The library supports protocols proposed in the following papers.
- Nuttapong Attrapadung, Goichiro Hanaoaka, Takahiro Matsuda, Hiraku Morita, Kazuma Ohara, Jacob C. N. Schuldt, Tadanori Teruya, 
  and Kazunari Tozawa. Oblivious Linear Group Actions and Applications. In Proceedings of the 2021 ACM SIGSAC Conference on Computer and
  Communications Security (CCS '21). Association for Computing Machinery, New York, NY, USA, 630–650. 
  https://doi.org/10.1145/3460120.3484584 
- Nuttapong Attrapadung, Hiraku Morita, Kazuma Ohara, Jacob C. N. Schuldt, and Kazunari Tozawa. 
  Memory and Round-Efficient MPC Primitives in the Pre-Processing Model from Unit Vectorization. 
  In Proceedings of the 2022 ACM on Asia Conference on Computer and Communications Security (ASIA CCS '22),
  pp. 858–872, 2022. 
  https://doi.org/10.1145/3488932.3517407
- Nuttapong Attrapadung, Hiraku Morita, Kazuma Ohara, Jacob C. N. Schuldt, Tadanori Teruya, and Kazunari Tozawa.
  Secure Parallel Computation on Privately Partitioned Data and Applications. In Proceedings of the 2022 ACM SIGSAC 
  Conference on Computer and Communications Security (CCS '22). Association for Computing Machinery, New York, NY, USA, 151–164. 
  https://doi.org/10.1145/3548606.3560695
- Nuttapong Attrapadung, Kota Isayama, Kunihiko Sadakane, and Kazunari Tozawa.
  Secure Parallel Computation with Oblivious State Transitions. 
  In Proceedings of the 2024 on ACM SIGSAC Conference on Computer and Communications Security (CCS '24).
  https://doi.org/10.1145/3658644.3690315
- Isayama, K., Jimbo, K., Okamoto, N., Sadakane, K., Tozawa, K. 
  Oblivious Suffix Sorting: A Multi-Party Computation Scheme for Secure and Efficient Suffix Sorting. 
  In: Fischlin, M., Moonsamy, V. (eds) Applied Cryptography and Network Security. ACNS 2025. 
  Lecture Notes in Computer Science, vol 15825. Springer, Cham. 
  https://doi.org/10.1007/978-3-031-95761-1_10

## Environment
Computation is performed on 3 PCs (or 1 PC). They are called `server`, `party_1`, and `party_2`.
The server receives input data in plaintext, converts it into shares, and sends them to party 1 and 2. It also generates correlated randomness and sends it to party 1 and 2.
Currently, the server also performs all computations in plaintext for answer checking.
`party_1` and `party_2` perform computation while communicating with each other.

## Compilation
Use it in C or C++ with `#include "share.h"`.
If you run with party ID `-1`, all computations are executed on one machine (in plaintext).
This is useful for algorithm verification and evaluating MPC overhead.

## Execution
Set the IP addresses and ports of the 3 PCs in `config.txt`.
```config.txt
127.0.0.1 9800 # server
127.0.0.1 9810 # party 1
127.0.0.1 9820 # party 2
```
Each line specifies a PC IP address and base port. Three ports are used starting from that value.
(In this example, the server uses 9800, 9801, 9802.)
When running on one PC, set all IP addresses to localhost (127.0.0.1). All port numbers must be different.
When using multiple PCs, specify each IP address. Also, if communicating with other PCs, firewall settings may need to be changed.

Assuming the executable is `share.out`, run the following on each of the 3 PCs (terminals):
```
@server:$ ./share.out 0
@party_1:$ ./share.out 1
@party_2:$ ./share.out 2
```

## Python
Compilation and execution instructions.
```bash
@server:$ python3 -m venv env
@server:$ source env/bin/activate
@server:$ cd python/csclib
@server:$ make
@server:$ pip3 install .
```
To compute precomputed tables, in the same directory as config.txt
```bash
@server:$ mkdir PRE
@server:$ python3 -c "from csclib import *; Csclib_precompute()"
```


## `config.txt` format
```
[options]
parties 3                   # number of parties (party 0, 1, 2)
channels 1                  # number of channels when using multithreading
comm_no_delay 1             # use whichever communication mode is faster between 1 and 0
warn_precomp 1              # warn when precomputation tables do not exist
[parties]
127.0.0.1 9800 # server      # IP address and port number for party 0
127.0.0.1 9810 # party 1     # IP address and port number for party 1
127.0.0.1 9820 # party 2     # IP address and port number for party 2
[mt_seeds] # party seed*5
0 123 456 789 0 0            # random seeds used by party 0 (5 integers)
1 234 567 890 0 1
2 345 678 234 0 2
3 456 789 345 0 3
[pre_bt]                    # correlated randomness for Beaver triples
0 PRE/PRE_BT.dat            # file storing correlated randomness used by channel 0
[pre_of] # bits channel filename
1 0 PRE/PRE_OF1.dat         # whether overflow occurs for 1-bit values (whether both party bits are 1)
[pre_b2a] # bit expansion (convert 1-bit value to log q bits)
0 PRE/PRE_B2A.dat
[pre_onehot] # bits xor channel filename
1 0 0 PRE/PRE_OHA1.dat
[pre_onehot_shamir] # bits channel filename
1 0 PRE/PRE_OHS1.dat
[pre_onehot_shamir3] # bits irr_poly channel filename
4 13 0 PRE/PRE_OHS3_0x13.dat
[pre_onehot_rss] # bits irr_poly channel filename
4 13 0 PRE/PRE_OHR_0x13.dat
[pre_ds] # n bs inverse channel filename      # double shares for permutation; n = permutation length, bs = block size
2 1 0 0 PRE/PRE_DS_n1_w30.dat
4 1 0 0 PRE/PRE_DS_n2_w30.dat
8 1 0 0 PRE/PRE_DS_n3_w30.dat
2 1 1 0 PRE/PRE_DSi_n1_w30.dat
4 1 1 0 PRE/PRE_DSi_n2_w30.dat
8 1 1 0 PRE/PRE_DSi_n3_w30.dat
[pre_uv] #n old_q new_q channel fname         # create unit vectors
2 32 4 0 PRE/PRE_UV_n2_oq32_nq4.dat
```
