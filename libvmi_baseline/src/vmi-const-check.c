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
    vmi_init_data_t* init_data = NULL;
    uint64_t domid = 0;
    uint8_t init = VMI_INIT_DOMAINNAME,
            config_type = VMI_CONFIG_GLOBAL_FILE_ENTRY;
    void *input = NULL, *config = NULL;
    int retcode = 1;

    /* const related variables */
    /* in include/net/tcp.h -- struct tcp_seq_afinfo */
    addr_t const_afinfo = 0;
    addr_t seq_ops = 0;
    unsigned long seq_ops_offset = 0;
    unsigned long seq_fops_write_offset = 0;
    unsigned long seq_fops_read_iter_offset = 0;
    /* in include/linux/seq_file.h -- struct seq_operations */
    addr_t stop_fn = 0, show_fn = 0, start_fn = 0;
    addr_t read_fn = 0, read_iter_fn = 0, llseek_fn = 0;
    unsigned long seq_stop_offset = 0;
    unsigned long seq_start_offset = 0;
    unsigned long seq_show_offset = 0;

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

    /* init const related offsets */
    seq_ops_offset = 16;
    seq_fops_write_offset = 24;
    seq_fops_read_iter_offset = 32;
    seq_start_offset = 24
    seq_stop_offset = 32;
    seq_show_offset = 48;

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

        printf("Check const data structures for VM %s (id=%" PRIu64 ")\n",
               name2, id);
    }
    else
    {
        printf("Check const data structures for file %s\n", name2);
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

        // patch: use tcp4_seq_afinfo
        if (VMI_FAILURE ==
            vmi_translate_ksym2v(vmi, "tcp4_seq_afinfo", &const_afinfo))
            goto error_exit;
    }
    else
    {
        printf(
            "We only support Linux OS now. Other OSes are not supported. \n");
        goto error_exit;
    }

    /* read seq_ops info */
    //seq_ops = const_afinfo + seq_ops_offset;

    vmi_read_addr_va(vmi, const_afinfo + seq_start_offset, 0, &start_fn);
    vmi_read_addr_va(vmi, const_afinfo + seq_stop_offset, 0, &stop_fn);
    vmi_read_addr_va(vmi, const_afinfo + seq_show_offset, 0, &show_fn)
    printf("const tcp4_seq_afinfo -- start func addr: %" PRIx64
           " stop func addr: %" PRIx64 "\n"
           " show func addr: %" PRIx64 "\n",
           start_fn, stop_fn, show_fn);

    //  get the seq_ops address
    vmi_read_addr_va(vmi, const_afinfo + seq_ops_offset, 0, &seq_ops);
    vmi_read_addr_va(vmi, seq_ops + seq_write_offset, 0, &write_fn);
    vmi_read_addr_va(vmi, seq_ops + seq_read_iter_offset, 0, &read_iter_fn);
    printf("const tcp4_seq_afinfo -- write func addr: %" PRIx64
           " read_iter func addr: %" PRIx64 "\n",
           write_fn, read_iter_fn);


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
