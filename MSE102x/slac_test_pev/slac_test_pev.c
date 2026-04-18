/*  plc hle utils project {{{
*
* Copyright (C) 2018 MStar Semiconductor
*
* <<<Licence>>>
*
* }}} */
#include "pev_slac.h"

int
main (int argc, char *argv[])
{
    char *file_nid[7];
    char *file_nmk[16];
    char *iname = "\0";
    char *filename = "\0";
    char *specific_mac = "\0";

    int i = 0;
    /* This flag is for Mstar internal test. */
    int mstar_test_flag = 1;
    /* Indicate if execute test constantly. */
    int loop = 0;
    /* Indicate if print the debug information */
    DEBUG_DISPLAY = 0;
    /* Indicate if set key for disconnection after SLAC test */
    int exec_disconnection = 0;
    uint8_t nid[7];
    uint8_t nmk[16];
    /* Indicate if set key on different alias MAC address */
    int alias_mac_compatible = 0;

#ifdef __linux__
    char ch;
    char info[128];
    struct option longopts[] =
    {
        { "help", no_argument, NULL, 'h' },
    };

    while ( (ch = getopt_long (argc, argv, "a:cdf:hi:lmv", longopts, NULL)) != -1)
    {
        switch (ch)
        {
        case 'c':
            exec_disconnection = 1;
            break;
        case 'l':
            loop = 1;
            break;
        case 'i':
        {
            iname = optarg;
        }
        break;
        case 'd':
            DEBUG_DISPLAY = 1;
            break;
        case 'v':
            DEBUG_DISPLAY = 2;
            break;
        case 'f':
        {
            filename = optarg;
        }
        break;
        case 'm':
        {
            mstar_test_flag = 0;
        }
        break;
        case 'a':
        {
            alias_mac_compatible = 1;
            specific_mac = optarg;
        }
        break;
        case 'h':
            pev_print_usage ();
            break;
        default:
            pev_print_usage ();
        }
    }
#endif //__linux__

#ifdef _WIN32
    setbuf (stdout, NULL);

    iname = argv[1];
    DEBUG_DISPLAY = atoi (argv[2]);
    loop = atoi (argv[3]);
    char *nmk_input = argv[4];
    char *nid_input = argv[5];
    exec_disconnection = atoi(argv[6]);

#endif //_WIN32

    if (strlen (iname) == 0)
    {
        printf ("please specify your network interface. \
[-i <network interface name>]\n");
        pev_print_usage ();
    }
#ifdef __linux__
    if (alias_mac_compatible)
    {
        const char *delim = ":";
        char *pch;
        i = 0;
        pch = strtok (specific_mac, delim);
        while (pch != NULL)
        {
            other_mac_link_local[i++] = (int)strtol (pch, NULL, 16);
            pch = strtok (NULL, delim);
        }
    }

    if (strlen (filename) == 0)
    {
        printf ("please specify your NID and NMK configuration file. \
[-f <file name>]\n");
        pev_print_usage ();
    }

    if (mstar_test_flag)
    {
        FILE *fptr;
        if ( (fptr = fopen (filename, "r")) == NULL)
        {
            perror ("open_file_error!!!\n");
            system("pause");
            exit (1);
        }

        while (fgets (info, 128, fptr) != NULL)
        {
            const char *delim = "=:";
            char *pch;
            i = 0;
            pch = strtok (info, delim);
            if (pch != NULL)
            {
                if (memcmp (pch, "NID", 3) == 0)
                {
                    pch = strtok (NULL, delim);
                    while (pch != NULL)
                    {
                        file_nid[i] = pch;
                        nid[i] = (int)strtol (file_nid[i], NULL, 16);
                        i++;
                        pch = strtok (NULL, delim);
                    }
                }
                else if (memcmp (pch, "NMK", 3) == 0)
                {
                    pch = strtok (NULL, delim);
                    while (pch != NULL)
                    {
                        file_nmk[i] = pch;
                        nmk[i] = (int)strtol (file_nmk[i], NULL, 16);
                        i++;
                        pch = strtok (NULL, delim);
                    }
                }
            }
        }
        fclose (fptr);
    }
#endif // __linux__

#ifdef _WIN32

    char *delim = "-";
    unsigned char *pch;
    i = 0;

    pch = strtok (nmk_input, delim);
    while (pch != NULL)
    {
        file_nmk[i] = pch;
        nmk[i] = (int)strtol (file_nmk[i], NULL, 16);
        i++;
        pch = strtok (NULL, delim);
    }
    i = 0;
    pch = strtok (nid_input, delim);
    while (pch != NULL)
    {
        file_nid[i] = pch;
        nid[i] = (int)strtol (file_nid[i], NULL, 16);
        i++;
        pch = strtok (NULL, delim);
    }

#endif // _WIN32

    if (pev_main (iname, nid, nmk, mstar_test_flag, alias_mac_compatible,
                    DEBUG_DISPLAY, loop, exec_disconnection) != 0)
    {
        printf ("Error: evse_main() return -1.\n");
    }

    return 0;
}