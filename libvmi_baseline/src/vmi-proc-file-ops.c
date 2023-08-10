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
    addr_t list_head = 0, cur_proc_dir_entry = 0;
    addr_t proc_fops = 0;
    addr_t write_func_ptr = 0, read_func_ptr = 0;
    addr_t llseek_func_ptr = 0, iterate_shared_func_ptr = 0;
    unsigned long proc_fops_offset = 0, proc_fops_write_offset = 0,
                  proc_fops_read_offset = 0, proc_fops_llseek_offset = 0, 
                  proc_fops_iterate_shared_offset = 0;
    status_t status = VMI_FAILURE;
    vmi_init_data_t* init_data = NULL;
    uint64_t domid = 0;
    uint8_t init = VMI_INIT_DOMAINNAME,
            config_type = VMI_CONFIG_GLOBAL_FILE_ENTRY;
    void *input = NULL, *config = NULL;
    int retcode = 1;

    if (argc < 2)
    {
        printf("Usage: %s\n", argv[0]);
        printf("\t -n/--name <domain name>\n");
        printf("\t -d/--domid <domain id>\n");
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
            {"socket", optional_argument, NULL, 's'},
            {NULL, 0, NULL, 0}};
        const char* opts = "n:d:s:";
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

    /* init syscall table related offsets */
    proc_fops_offset = 40;
    proc_fops_write_offset = 32;
    proc_fops_read_offset = 40;
    proc_fops_iterate_shared_offset = 56;
    proc_fops_llseek_offset = 8;

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

        printf("Check Linux /proc file operations for VM %s (id=%" PRIu64 ")\n",
               name2, id);
    }
    else
    {
        printf("Check Linux /proc file operations for file %s\n", name2);
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
        if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "proc_root", &list_head))
            goto error_exit;
    }
    else
    {
        printf(
            "We only support Linux OS now. Other OSes are not supported. \n");
        goto error_exit;
    }

    cur_proc_dir_entry = list_head;
    /* parse proc_fops */
    status = vmi_read_addr_va(vmi, cur_proc_dir_entry + proc_fops_offset, 0,
                              &proc_fops);
    if (status == VMI_FAILURE)
    {
        printf("Failed to read next pointer in loop at %" PRIx64 "\n",
               cur_proc_dir_entry);
        goto error_exit;
    }

    /* parse write and read func pointers in file_operations */
    vmi_read_addr_va(vmi, proc_fops + proc_fops_write_offset, 0,
                     &write_func_ptr);
    vmi_read_addr_va(vmi, proc_fops + proc_fops_read_offset, 0, &read_func_ptr);
    vmi_read_addr_va(vmi, proc_fops + proc_fops_llseek_offset, 0,
                     &llseek_func_ptr);
    vmi_read_addr_va(vmi, proc_fops + proc_fops_iterate_shared_offset, 0,
                     &iterate_shared_func_ptr);
    printf("write func addr: %" PRIx64 "\n, read func addr: %" PRIx64 "\n",
           write_func_ptr, read_func_ptr);
    printf("llseek func addr: %" PRIx64 "\n, iterate_shared func addr: %" PRIx64 "\n",
           llseek_func_ptr, iterate_shared_func_ptr);
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
