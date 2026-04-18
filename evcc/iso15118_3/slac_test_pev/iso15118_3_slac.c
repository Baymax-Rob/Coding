/*  plc hle utils project {{{
*
* Copyright (C) 2018 MStar Semiconductor
*
* <<<Licence>>>
*
* }}} */
#include "iso15118_3_slac.h"

#include "pev_slac.h"
#include "slac_api.h"
#include "timer.h"

static int
parse_alias_mac (char *specific_mac, uint8_t alias_dest_mac[6])
{
    const char *delim = ":";
    char *pch;
    int i = 0;

    if (specific_mac == NULL || strlen (specific_mac) == 0)
    {
        return -1;
    }

    pch = strtok (specific_mac, delim);
    while (pch != NULL && i < 6)
    {
        alias_dest_mac[i] = (uint8_t)strtol (pch, NULL, 16);
        other_mac_link_local[i] = alias_dest_mac[i];
        i++;
        pch = strtok (NULL, delim);
    }

    return i == 6 ? 0 : -1;
}

static int
parse_key_file (char *filename, uint8_t nid[7], uint8_t nmk[16])
{
    char *file_nid[7];
    char *file_nmk[16];
    char info[128];
    int i;
    FILE *fptr;

    if (filename == NULL || strlen (filename) == 0)
    {
        return -1;
    }

    fptr = fopen (filename, "r");
    if (fptr == NULL)
    {
        return -1;
    }

    while (fgets (info, sizeof (info), fptr) != NULL)
    {
        const char *delim = "=:";
        char *pch;
        i = 0;
        pch = strtok (info, delim);
        if (pch == NULL)
        {
            continue;
        }

        if (memcmp (pch, "NID", 3) == 0)
        {
            pch = strtok (NULL, delim);
            while (pch != NULL && i < 7)
            {
                file_nid[i] = pch;
                nid[i] = (uint8_t)strtol (file_nid[i], NULL, 16);
                i++;
                pch = strtok (NULL, delim);
            }
        }
        else if (memcmp (pch, "NMK", 3) == 0)
        {
            pch = strtok (NULL, delim);
            while (pch != NULL && i < 16)
            {
                file_nmk[i] = pch;
                nmk[i] = (uint8_t)strtol (file_nmk[i], NULL, 16);
                i++;
                pch = strtok (NULL, delim);
            }
        }
    }

    fclose (fptr);
    return 0;
}

int
slac_test_pev_run (const slac_test_pev_options_t *options)
{
    slac_test_pev_options_t run_options;
    slac_config_t cfg;
    slac_client_t *client;
    slac_if_t sif;
    slac_match_request_t req;
    slac_match_result_t out;
    slac_err_t rc;

    if (options == NULL || options->ifname == NULL || strlen (options->ifname) == 0)
    {
        return 1;
    }

    memcpy (&run_options, options, sizeof (run_options));

    /* Same as pev_main(): loop test requires disconnect path on PEV. */
    if (run_options.loop)
        run_options.exec_disconnection = 1;

    cfg.log_level = (slac_log_level_t)run_options.debug_display;
    client = slac_client_create (&cfg);
    if (!client)
    {
        printf ("Error: slac_client_create() failed\n");
        return 1;
    }

    sif.ifname = run_options.ifname;
    rc = slac_client_open (client, &sif);
    if (rc != SLAC_OK)
    {
        printf ("Error: slac_client_open() failed (%d)\n", (int)rc);
        slac_client_destroy (client);
        return 1;
    }

    memset (&req, 0, sizeof (req));
    memcpy (req.key.nid, run_options.nid, sizeof (run_options.nid));
    memcpy (req.key.nmk, run_options.nmk, sizeof (run_options.nmk));
    req.alias_mac_compatible = run_options.alias_mac_compatible;
    memcpy (req.alias_dest_mac, run_options.alias_dest_mac,
        sizeof (run_options.alias_dest_mac));
    req.vendor_status_check = run_options.mstar_test_flag ? 1 : 0;
    req.exec_disconnection = run_options.exec_disconnection;

    memset (&out, 0, sizeof (out));

    do
    {
        rc = slac_match_once (client, &req, &out);
        if (rc != SLAC_OK || !out.matched)
        {
            printf ("SLAC match failed (%d)\n", (int)rc);
        }
        else
        {
            printf ("SLAC matched. EVSE MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
                out.selected_evse_mac[0], out.selected_evse_mac[1],
                out.selected_evse_mac[2], out.selected_evse_mac[3],
                out.selected_evse_mac[4], out.selected_evse_mac[5]);
        }
        if (run_options.loop)
            sleep_ms (8000);
    } while (run_options.loop);

    slac_client_close (client);
    slac_client_destroy (client);

    return 0;
}

int
slac_test_pev_main (int argc, char *argv[])
{
    slac_test_pev_options_t options;

    memset (&options, 0, sizeof (options));
    options.ifname = "\0";
    options.config_file = "\0";
    options.specific_mac = "\0";
    options.mstar_test_flag = 1;
    options.loop = 0;
    options.debug_display = 0;
    options.exec_disconnection = 0;
    options.alias_mac_compatible = 0;

#ifdef __linux__
    char ch;
    struct option longopts[] =
    {
        { "help", no_argument, NULL, 'h' },
    };

    while ( (ch = getopt_long (argc, argv, "a:cdf:hi:lmv", longopts, NULL)) != -1)
    {
        switch (ch)
        {
        case 'c':
            options.exec_disconnection = 1;
            break;
        case 'l':
            options.loop = 1;
            break;
        case 'i':
            options.ifname = optarg;
            break;
        case 'd':
            options.debug_display = 1;
            break;
        case 'v':
            options.debug_display = 2;
            break;
        case 'f':
            options.config_file = optarg;
            break;
        case 'm':
            options.mstar_test_flag = 0;
            break;
        case 'a':
            options.alias_mac_compatible = 1;
            options.specific_mac = optarg;
            break;
        case 'h':
            pev_print_usage ();
            break;
        default:
            pev_print_usage ();
        }
    }
#endif //__linux__

    if (strlen (options.ifname) == 0)
    {
        printf ("please specify your network interface. \
[-i <network interface name>]\n");
        pev_print_usage ();
    }
#ifdef __linux__
    if (options.alias_mac_compatible)
    {
        if (parse_alias_mac (options.specific_mac, options.alias_dest_mac) != 0)
        {
            printf ("invalid alias mac format, expected xx:xx:xx:xx:xx:xx\n");
            return 1;
        }
    }

    if (strlen (options.config_file) == 0)
    {
        printf ("please specify your NID and NMK configuration file. \
[-f <file name>]\n");
        pev_print_usage ();
    }

    if (options.mstar_test_flag)
    {
        if (parse_key_file (options.config_file, options.nid, options.nmk) != 0)
        {
            perror ("open_file_error!!!\n");
            return 1;
        }
    }
#endif // __linux__

    return slac_test_pev_run (&options);
}