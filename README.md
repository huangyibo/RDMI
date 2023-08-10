# Remote Direct Memory Introspection


*Hope that you'd be glad to add a star if you think this list is helpful!*


## Overview

RDMI develops a defense system targetting for memory introspection, leveraging
programmable data planes and RDMA NICs. The RDMI compiler compiles the policies specified
in domain specific language into lower level configurations. The master P4 switch program 
takes in the configurations and enforce the introspection policies for different security tasks.
This repo contains implementation of the system. Please refer to each ``readme`` under those subdirectories for more
informations.

## Compiler

The ``compiler`` directory contains the implementation of the compiler. It also includes the policy dsl used for 
encoding the introspection logic. 

## Switch

The ``switch`` directory contains the master P4 program as well as control rules and triggers of the introspection. 

## Connection

The ``connection`` directory contains the connection setup program for establish connections. 

## Experimental workflows:

1. Establish the RDMA connections(refer to ``connection``).
2. Compile the policy and generate the corresponding configuration files(refer to ``compiler``).
3. Configure the switch and run the program(refer to ``switch``).

## libVMI Baseline 

We take libVMI for VM-based cloud as the baseline and use it to implement a set of introspection tasks as follows. Note that the `libvmi_baseline` directory contains the implemented libVMI based introspection tasks.

1. vmi dump memory policy
2. kernel module list policy
3. process list policy
4. credential list policy
5. syscall table checker policy
6. proc file ops checking policy
7. open file checking policy
8. netfilter checking policy
9. tty checking policy
10. vm area checking policy
11. keyboad logger checking policy
12. const structs checking policy

## Reference implementations

Some of the implementation used in this repo is based on existing open-source
project, including [redmark](https://github.com/spcl/redmark.git),
[Pythia](https://github.com/WukLab/Pythia.git), [SCADET](https://github.com/sabbaghm/SCADET.git),
[Bedrock](https://github.com/alex1230608/Bedrock.git) and some examples codes provided in Tofino switch SDE.

## License
The code is released under the [MIT License](https://opensource.org/license/mit/).

If you use our RDMI or related codes in your research, please cite our paper:

```bib
@inproceedings{liu2023remote,
  title={Remote Direct Memory Introspection},
  author={Liu, Hongyi and Xing, Jiarong and Huang, Yibo and Devadas, Srinivas and Chen, Ang},
  booktitle={32st USENIX Security Symposium (USENIX Security 23)},
  year={2023}
}
```


## Academic and Reference Papers

[**USENIX Security**] [Remote Direct Memory Introspection](https://www.usenix.org/system/files/usenixsecurity23-liu-hongyi.pdf).
 Hongyi Liu, Jiarong Xing, and Yibo Huang, Rice University; Danyang Zhuo, Duke University; Srinivas Devadas, Massachusetts Institute of Technology; Ang Chen, Rice University. The 32nd USENIX Security Symposium, Anaheim, CA, USA, August 9–11, 2023.


