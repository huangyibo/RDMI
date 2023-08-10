#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <libvmi/libvmi.h>

typedef uint32_t vmi_uid_t;
typedef uint32_t vmi_gid_t;

int main(int argc, char** argv)
{
    vmi_instance_t vmi = {0};
    addr_t list_head = 0, cur_list_entry = 0, next_list_entry = 0;
    status_t status = VMI_FAILURE;
    vmi_init_data_t* init_data = NULL;
    uint64_t domid = 0;
    uint8_t init = VMI_INIT_DOMAINNAME,
            config_type = VMI_CONFIG_GLOBAL_FILE_ENTRY;
    void *input = NULL, *config = NULL;
    int retcode = 1;
    unsigned int i = 0;

    /* tty related variables */
    /* in include/linux/tty_driver.h -- tty_driver struct */
    addr_t cur_tty_driver = 0;
    unsigned int devices_num = 0;
    char* name = NULL;
    addr_t ttys = 0;
    unsigned long tty_drivers_offset = 0;
    unsigned long ttys_offset = 0;
    unsigned long num_offset = 0;
    unsigned long name_offset = 0;
    /* in include/linux/tty.h -- tty_struct */
    addr_t cur_tty_addr = 0, cur_tty = 0, ldisc = 0;
    unsigned long ldisc_offset = 0;
    unsigned long ptr_offset = 0;
    /* in include/linux/tty_ldisc.h -- struct tty_ldisc */
    addr_t tty_ldisc_ops = 0;
    /* in struct tty_ldisc_ops  */
    addr_t receive_buf2 = 0;
    addr_t receive_buf = 0;
    unsigned long receive_buf2_offset = 0;
    unsigned long receive_buf_offset = 0;
    unsigned long tty_ops_count = 0;

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

    /* init tty related offsets */
    ptr_offset = sizeof(void*);
    tty_drivers_offset = 168;
    ttys_offset = 128;
    num_offset = 52;
    ldisc_offset = 88;
    name_offset = 24;
    receive_buf2_offset = 128;
    receive_buf_offset = 104;

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

        printf("Check TTY for VM %s (id=%" PRIu64 ")\n", name2, id);
    }
    else
    {
        printf("Check TTY for file %s\n", name2);
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
        if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "tty_drivers", &list_head))
            goto error_exit;
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

    /* walk the tty_driver list */
    while (1)
    {
        cur_tty_driver = cur_list_entry - tty_drivers_offset;

        /* read num in current tty_driver */
        vmi_read_32_va(vmi, cur_tty_driver + num_offset, 0,
                       (uint32_t*)&devices_num);

        /* read name in tty_driver */

        // patch: don't read the name
        //name = vmi_read_str_va(vmi, cur_tty_driver + name_offset, 0);
        //printf("Driver name: %s, devices num: %u \n", name, devices_num);

        if (devices_num > 0)
        {
            /* read ttys in current tty_driver */
            vmi_read_addr_va(vmi, cur_tty_driver + ttys_offset, 0, &ttys);

            /* traverse the tty_struct array */
            cur_tty_addr = ttys;

            // patch: if ttys == 0: do not execute read
            if(cur_tty_addr == 0){
                goto next; // read the next on the list
            }


            for (i = 0; i < devices_num; i++)
            {
                /* read current tty info */
                vmi_read_addr_va(vmi, cur_tty_addr, 0, &cur_tty);

                if (cur_tty > 0)
                {
                    /* read tty_ldisc info in current tty */
                    vmi_read_addr_va(vmi, cur_tty + ldisc_offset, 0, &ldisc);

                    /* read tty_ldisc_ops in current tty ldisc */
                    vmi_read_addr_va(vmi, ldisc, 0, &tty_ldisc_ops);

                    /* read receive_buf2 info in current tty_ldisc_ops */
                    vmi_read_addr_va(vmi, tty_ldisc_ops + receive_buf2_offset,
                                     0, &receive_buf2);

                    /* read receive_buf info in current tty_ldisc_ops */
                    vmi_read_addr_va(vmi, tty_ldisc_ops + receive_buf_offset,
                                     0, &receive_buf);

                    /* print out receive_buf2 func pointer addr */
                    tty_ops_count++;
                    printf("[%ld] receive_buf2 addr: %" PRIx64 ")\n",
                           tty_ops_count, receive_buf2);
                    printf("[%ld] receive_buf1 addr: %" PRIx64 ")\n",
                           tty_ops_count, receive_buf1);
                }

                cur_tty_addr += ptr_offset;
            }
        }

        if (name)
        {
            free(name);
        }
next:
        /* follow the next pointer */
        cur_list_entry = next_list_entry;
        status = vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry);
        if (status == VMI_FAILURE)
        {
            printf("Failed to read next pointer in loop at %" PRIx64 "\n",
                   cur_list_entry);
            goto error_exit;
        }

        if (VMI_OS_LINUX == os && cur_list_entry == list_head)
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
