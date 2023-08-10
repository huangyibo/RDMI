#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <libvmi/libvmi.h>

int main(int argc, char** argv)
{
    vmi_instance_t vmi = {0};
    addr_t next_module = 0;
    addr_t list_head = 0;
    // init_data for KVM socket, if needed
    vmi_init_data_t* init_data = NULL;
    int retcode = 1;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <Name of VM> [socket]\n\n", argv[0]);
        return retcode;
    }

    /* this is the VM or file that we are looking at */
    char* name = argv[1];

    /* KVMi socket ? */
    if (argc == 3)
    {
        char* path = argv[2];

        init_data =
            malloc(sizeof(vmi_init_data_t) + sizeof(vmi_init_data_entry_t));
        init_data->count = 1;
        init_data->entry[0].type = VMI_INIT_DATA_KVMI_SOCKET;
        init_data->entry[0].data = strdup(path);
    }

    /* initialize the libvmi library */
    if (VMI_FAILURE ==
        vmi_init_complete(&vmi, name, VMI_INIT_DOMAINNAME, init_data,
                          VMI_CONFIG_GLOBAL_FILE_ENTRY, NULL, NULL))
    {
        printf("Failed to init LibVMI library.\n");
        goto error_exit;
    }

    /* pause the vm for consistent memory access */
    vmi_pause_vm(vmi);

    switch (vmi_get_ostype(vmi))
    {
        case VMI_OS_LINUX:
            vmi_read_addr_ksym(vmi, "modules", &next_module);
            break;
        case VMI_OS_WINDOWS:
            vmi_read_addr_ksym(vmi, "PsLoadedModuleList", &next_module);
            break;
        default:
            goto error_exit;
    }

    list_head = next_module;

    /* walk the module list */
    while (1)
    {

        /* follow the next pointer */
        addr_t tmp_next = 0;

        vmi_read_addr_va(vmi, next_module, 0, &tmp_next);

        /* if we are back at the list head, we are done */
        if (list_head == tmp_next)
        {
            break;
        }

        /* print out the module name */

        /* Note: the module struct that we are looking at has a string
         * directly following the next / prev pointers.  This is why you
         * can just add the length of 2 address fields to get the name.
         * See include/linux/module.h for mode details */
        if (VMI_OS_LINUX == vmi_get_ostype(vmi))
        {
            char* modname = NULL;

            if (VMI_PM_IA32E == vmi_get_page_mode(vmi, 0))
            { // 64-bit paging
                modname = vmi_read_str_va(vmi, next_module + 16, 0);
            }
            else
            {
                modname = vmi_read_str_va(vmi, next_module + 8, 0);
            }
            printf("%s\n", modname);
            free(modname);
        }
        else if (VMI_OS_WINDOWS == vmi_get_ostype(vmi))
        {

            unicode_string_t* us = NULL;

            /*
             * The offset 0x58 and 0x2c is the offset in the
             * _LDR_DATA_TABLE_ENTRY structure to the BaseDllName member. These
             * offset values are stable (at least) between XP and Windows 7.
             */

            if (VMI_PM_IA32E == vmi_get_page_mode(vmi, 0))
            {
                us = vmi_read_unicode_str_va(vmi, next_module + 0x58, 0);
            }
            else
            {
                us = vmi_read_unicode_str_va(vmi, next_module + 0x2c, 0);
            }

            unicode_string_t out = {0};
            //         both of these work
            if (us &&
                VMI_SUCCESS == vmi_convert_str_encoding(us, &out, "UTF-8"))
            {
                printf("%s\n", out.contents);
                //            if (us &&
                //                VMI_SUCCESS == vmi_convert_string_encoding
                //                (us, &out, "WCHAR_T")) { printf ("%ls\n",
                //                out.contents);
                free(out.contents);
            } // if
            if (us)
                vmi_free_unicode_str(us);
        }
        next_module = tmp_next;
    }

    retcode = 0;
error_exit:
    /* resume the vm */
    vmi_resume_vm(vmi);

    /* cleanup any memory associated with the libvmi instance */
    vmi_destroy(vmi);

    if (init_data)
    {
        free(init_data->entry[0].data);
        free(init_data);
    }

    return retcode;
}
