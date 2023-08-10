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
    // status_t status = VMI_FAILURE;
    vmi_init_data_t* init_data = NULL;
    uint64_t domid = 0;
    uint8_t init = VMI_INIT_DOMAINNAME,
            config_type = VMI_CONFIG_GLOBAL_FILE_ENTRY;
    void *input = NULL, *config = NULL;
    int retcode = 1;
    int i = 0, j = 0, k = 0;

    /* netfilter related variables */
    /* in include/net/net_namespace.h */
    addr_t init_net = 0, init_net_nf = 0;
    unsigned long net_nf_offset = 0;
    /* in include/net/netns/netfilter.h */
    addr_t netns_nf_hooks = 0;
    addr_t cur_hook_entries_addr = 0;
    addr_t cur_hook_entries = 0;
    unsigned long netns_nf_hooks_offset = 0, ptr_offset = 0;
    int nf_proto_num = 12, nf_max_hooks_num = 8;
    /* in include/linux/netfilter.h */
    uint16_t num_hook_entries = 0;
    addr_t entries_hooks = 0;
    addr_t cur_hook_entry = 0;
    unsigned long entries_hooks_offet = 0;
    /* in nf_hook_entry struct (include/linux/netfilter.h) */
    addr_t cur_hookfn = 0;
    unsigned long hookfn_count = 0;
    unsigned long hook_entry_size = 0;

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

    /* init net related offsets */
    net_nf_offset = 3592;
    netns_nf_hooks_offset = 0;
    entries_hooks_offet = 8;
    ptr_offset = sizeof(void*);
    hook_entry_size = 2 * sizeof(void*);

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

        printf("Check netfilters for VM %s (id=%" PRIu64 ")\n", name2, id);
    }
    else
    {
        printf("Check netfilters for file %s\n", name2);
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
        if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "init_net", &init_net))
            goto error_exit;
    }
    else
    {
        printf(
            "We only support Linux OS now. Other OSes are not supported. \n");
        goto error_exit;
    }

    init_net_nf = init_net + net_nf_offset;
    netns_nf_hooks = init_net_nf + netns_nf_hooks_offset;

    for (i = 0; i < nf_proto_num; i++)
    {
        for (j = 0; j < nf_max_hooks_num; j++)
        {
            /* caculate the address of current hooks in 2-dimension array */
            cur_hook_entries_addr = netns_nf_hooks +
                                    i * ptr_offset * nf_max_hooks_num +
                                    j * ptr_offset;

            /* extract the corresponding nf_hook_entries pointer */
            vmi_read_addr_va(vmi, cur_hook_entries_addr, 0, &cur_hook_entries);

            /* extract num_hook_entries in one nf_hook_entries */
            vmi_read_16_va(vmi, cur_hook_entries, 0, &num_hook_entries);

            /* extract the address of hook array in nf_hook_entries struct */
            entries_hooks = cur_hook_entries + entries_hooks_offet;

            /* traverse the nf_hook_entry array in  nf_hook_entries struct */
            if (num_hook_entries > 0)
            {
                cur_hook_entry = entries_hooks;
                for (k = 0; k < num_hook_entries; k++)
                {
                    vmi_read_addr_va(vmi, cur_hook_entry, 0, &cur_hookfn);
                    hookfn_count++;

                    /* print out the hook func */
                    printf("[%ld] hookfn addr: %" PRIx64 ")\n", hookfn_count,
                           cur_hookfn);

                    cur_hook_entry += hook_entry_size;
                }
            }
        }
    }

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
