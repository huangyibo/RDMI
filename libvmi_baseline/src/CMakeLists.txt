add_executable(vmi-dump-memory vmi-dump-memory.c)
target_link_libraries(vmi-dump-memory vmi)

add_executable(vmi-module-list vmi-module-list.c)
target_link_libraries(vmi-module-list vmi)

add_executable(vmi-process-list vmi-process-list.c)
target_link_libraries(vmi-process-list vmi)

add_executable(vmi-credential-list vmi-credential-list.c)
target_link_libraries(vmi-credential-list vmi)

add_executable(vmi-syscall-table-check vmi-syscall-table-check.c)
target_link_libraries(vmi-syscall-table-check vmi)

add_executable(vmi-proc-file-ops vmi-proc-file-ops.c)
target_link_libraries(vmi-proc-file-ops vmi)

add_executable(vmi-open-file-check vmi-open-file-check.c)
target_link_libraries(vmi-open-file-check vmi)

add_executable(vmi-netfilter-func-check vmi-netfilter-func-check.c)
target_link_libraries(vmi-netfilter-func-check vmi)

add_executable(vmi-tty-check vmi-tty-check.c)
target_link_libraries(vmi-tty-check vmi)

add_executable(vmi-vm-area-check vmi-vm-area-check.c)
target_link_libraries(vmi-vm-area-check vmi)

add_executable(vmi-keylogger-check vmi-keylogger-check.c)
target_link_libraries(vmi-keylogger-check vmi)

add_executable(vmi-const-check vmi-const-check.c)
target_link_libraries(vmi-const-check vmi)
