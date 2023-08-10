#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <libvmi/libvmi.h>

int main(int argc, char** argv)
{
    vmi_instance_t vmi = {0};
    addr_t list_head = 0, cur_list_entry = 0, next_list_entry = 0;
    addr_t current_process = 0;

    char* file_iname = NULL;
    vmi_pid_t pid = 0;
    addr_t proc_files = 0, files_fdt = 0, fdtable_fd = 0, cur_fdt_fd = 0;
    addr_t cur_file_array_entry = 0;
    addr_t file_fpath = 0, path_dentry = 0;
    unsigned int fdtable_max_fds = 0;
    unsigned long tasks_offset = 0, pid_offset = 0, name_offset = 0;
    unsigned long files_offset = 0, files_fdt_offset = 0,
                  files_fdt_fd_offset = 0;
    unsigned long file_struct_fpath_offset = 0, path_struct_dentry_offset = 0;
    unsigned long dentry_struct_iname_offset = 0;
    int ptr_size = 0;
    status_t status = VMI_FAILURE;
    vmi_init_data_t* init_data = NULL;
    uint64_t domid = 0;
    uint8_t init = VMI_INIT_DOMAINNAME,
            config_type = VMI_CONFIG_GLOBAL_FILE_ENTRY;
    void *input = NULL, *config = NULL;
    int retcode = 1;
    unsigned int i = 0;
    int open_file_count = 0;

    if (argc < 2)
    {
        printf("Usage: %s\n", argv[0]);
        printf("\t -n/--name <domain name>\n");
        printf("\t -d/--domid <domain id>\n");
        printf("\t -j/--json <path to kernel's json profile>\n");
        printf("\t -s/--socket <path to KVMI socket>\n");
        return retcode;
    }

    // left for compatibility
    if (argc == 2)
        input = argv[1];

    if (argc > 2)
    {
        const struct option long_opts[] = {
            {"name", required_argument, NULL, 'n'},
            {"domid", required_argument, NULL, 'd'},
            {"json", required_argument, NULL, 'j'},
            {"socket", optional_argument, NULL, 's'},
            {NULL, 0, NULL, 0}};
        const char* opts = "n:d:j:s:";
        int c;
        int long_index = 0;

        while ((c = getopt_long(argc, argv, opts, long_opts, &long_index)) !=
               -1)
            switch (c)
            {
                case 'n':
                    input = optarg;
                    break;
                case 'd':
                    init = VMI_INIT_DOMAINID;
                    domid = strtoull(optarg, NULL, 0);
                    input = (void*)&domid;
                    break;
                case 'j':
                    config_type = VMI_CONFIG_JSON_PATH;
                    config = (void*)optarg;
                    break;
                case 's':
                    // in case we have multiple '-s' argument, avoid memory leak
                    if (init_data)
                    {
                        free(init_data->entry[0].data);
                    }
                    else
                    {
                        init_data = malloc(sizeof(vmi_init_data_t) +
                                           sizeof(vmi_init_data_entry_t));
                    }
                    init_data->count = 1;
                    init_data->entry[0].type = VMI_INIT_DATA_KVMI_SOCKET;
                    init_data->entry[0].data = strdup(optarg);
                    break;
                default:
                    printf("Unknown option\n");
                    if (init_data)
                    {
                        free(init_data->entry[0].data);
                        free(init_data);
                    }
                    return retcode;
            }
    }

    /* initialize the libvmi library */
    if (VMI_FAILURE == vmi_init_complete(&vmi, input, init, init_data,
                                         config_type, config, NULL))
    {
        printf("Failed to init LibVMI library.\n");
        goto error_exit;
    }

    /* init the offset values */
    if (VMI_OS_LINUX == vmi_get_ostype(vmi))
    {
        if (VMI_FAILURE == vmi_get_offset(vmi, "linux_tasks", &tasks_offset))
            goto error_exit;
        if (VMI_FAILURE == vmi_get_offset(vmi, "linux_name", &name_offset))
            goto error_exit;
        if (VMI_FAILURE == vmi_get_offset(vmi, "linux_pid", &pid_offset))
            goto error_exit;
    }
    else
    {
        printf(
            "We only support Linux OS now. Other OSes are not supported. \n");
        goto error_exit;
    }

    /* init file related offset values */
    files_offset = 2704;
    files_fdt_offset = 32;
    files_fdt_fd_offset = 8;
    file_struct_fpath_offset = 16;
    path_struct_dentry_offset = 8;
    dentry_struct_iname_offset = 56;
    ptr_size = sizeof(void*);

    /* pause the vm for consistent memory access */
    if (vmi_pause_vm(vmi) != VMI_SUCCESS)
    {
        printf("Failed to pause VM\n");
        goto error_exit;
    }

    /* demonstrate name and id accessors */
    char* name2 = vmi_get_name(vmi);
    vmi_mode_t mode;

    if (VMI_FAILURE == vmi_get_access_mode(vmi, NULL, 0, NULL, &mode))
        goto error_exit;

    if (VMI_FILE != mode)
    {
        uint64_t id = vmi_get_vmid(vmi);

        printf("Check opened files for VM %s (id=%" PRIu64 ")\n", name2, id);
    }
    else
    {
        printf("Check opened files for file %s\n", name2);
    }
    free(name2);

    os_t os = vmi_get_ostype(vmi);

    /* get the head of the list */
    if (VMI_OS_LINUX == os)
    {
        /* Begin at PID 0, the 'swapper' task. It's not typically shown by OS
         *  utilities, but it is indeed part of the task list and useful to
         *  display as such.
         */
        if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "init_task", &list_head))
            goto error_exit;

        list_head += tasks_offset;
    }
    else
    {
        printf(
            "We only support Linux OS now. Other OSes are not supported. \n");
        goto error_exit;
    }

    cur_list_entry = list_head;
    if (VMI_FAILURE ==
        vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry))
    {
        printf("Failed to read next pointer in loop at %" PRIx64 "\n",
               cur_list_entry);
        goto error_exit;
    }

    /* walk the task list */
    while (1)
    {
        current_process = cur_list_entry - tasks_offset;

        /* See include/linux/sched.h for mode details */
        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&pid);

        /* parse opened files */
        vmi_read_addr_va(vmi, current_process + files_offset, 0, &proc_files);

        vmi_read_addr_va(vmi, proc_files + files_fdt_offset, 0, &files_fdt);

        vmi_read_32_va(vmi, files_fdt, 0, (uint32_t*)&fdtable_max_fds);
        vmi_read_addr_va(vmi, files_fdt + files_fdt_fd_offset, 0, &fdtable_fd);

        cur_fdt_fd = fdtable_fd;
        for (i = 0; i < fdtable_max_fds; i++)
        {
            /* read the virtual address of current file */
            vmi_read_addr_va(vmi, cur_fdt_fd, 0, &cur_file_array_entry);

            if (cur_file_array_entry)
            {
                file_fpath = cur_file_array_entry + file_struct_fpath_offset;
                vmi_read_addr_va(vmi, file_fpath + path_struct_dentry_offset, 0,
                                 &path_dentry);

                file_iname = vmi_read_str_va(
                    vmi, path_dentry + dentry_struct_iname_offset, 0);
                if (!file_iname)
                {
                    printf("Failed to find file name\n");
                    goto error_exit;
                }

                /* print out the file name */
                open_file_count++;
                printf("[%5d -- %d] file_iname: %s \n", pid, open_file_count,
                       file_iname);
                if (file_iname)
                {
                    free(file_iname);
                    file_iname = NULL;
                }
            }

            cur_fdt_fd += ptr_size;
        }

        if (VMI_OS_FREEBSD == os && next_list_entry == list_head)
        {
            break;
        }

        /* follow the next pointer */
        cur_list_entry = next_list_entry;
        status = vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry);
        if (status == VMI_FAILURE)
        {
            printf("Failed to read next pointer in loop at %" PRIx64 "\n",
                   cur_list_entry);
            goto error_exit;
        }
        /* In Windows, the next pointer points to the head of list, this pointer
         * is actually the address of PsActiveProcessHead symbol, not the
         * address of an ActiveProcessLink in EPROCESS struct. It means in
         * Windows, we should stop the loop at the last element in the list,
         * while in Linux, we should stop the loop when coming back to the first
         * element of the loop
         */
        if (VMI_OS_WINDOWS == os && next_list_entry == list_head)
        {
            break;
        }
        else if (VMI_OS_LINUX == os && cur_list_entry == list_head)
        {
            break;
        }
    };

    retcode = 0;
error_exit:
    /* resume the vm */
    vmi_resume_vm(vmi);

    /* cleanup any memory associated with the LibVMI instance */
    vmi_destroy(vmi);

    if (init_data)
    {
        free(init_data->entry[0].data);
        free(init_data);
    }

    return retcode;
}
