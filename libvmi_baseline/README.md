# LibVMI Baseline 

LibVMI is a virtual machine introspection library. We use LibVMI as one primary baseline relative to RDMi.
We use LibVMI to implement 11 security policies which have been supported in RDMi, so that we can directly 
evaluate LibVMI and RDMi and prove the benefits of RDMi.


# Dependencies

The following libraries are used in building this code:

- CMake (>= 3.1)
- libvmi (latest, e.g., v0.14.0) from https://github.com/libvmi/libvmi

We assume libvmi and corresponding KVM-Qemu has been pre-installed successfully at target machines.
If you plan to run the LibVMI baseline experiments on a new machine, please refer to 
[this google docs link](https://docs.google.com/document/d/1CTr6i2KkSksUBEy-LF5cyH0JS1DcNpI_SqCbwd9RRDs/edit?usp=sharing) 
to deploy LibVMI on Bare-metal Setting.
 

# Security Policies

- PS_list: scan running process.

- Cred_list: scan process credentials.

- Netfilter: check registered netfilter.

- Proc check: check /proc file operations

- Syscall check: check syscall table.

- Open file: scan open file.

- Modlist: scan inserted kernel module.

- VM area check: scan process vm area information.

- TTY check: check tty information.

- Key logger check: check keyboard keylogger.

