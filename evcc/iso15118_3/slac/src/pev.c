/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
#include "pev_slac.h"
#include "slac_api.h"

void
pev_print_usage ()
{
    printf ("Usage: sudo ./slac_pev [Options]\n");
    printf ("-i <network interface name>\tspecify your network interface \
(necessary)\n");
    printf ("-f <file name>\t\t\tspecify the file name including \
NID and NMK (necessary)\n");
    printf ("-a\t\t\t\tcompatible with specific alias MAC address\n");
    printf ("-c\t\t\t\texecute the disconnection after SLAC test\n");
    printf ("-d\t\t\t\tprint the debug message\n");
    printf ("-h\t\t\t\tprint the help message\n");
    printf ("-l\t\t\t\texecute the test constantly\n");
    printf ("-v\t\t\t\tprint the debug message in verbose mode\n");
    return;
}


int
pev_main (char *iname, unsigned char nid[7], unsigned char nmk[16],
    int test_flag, int alias_mac_compatible, int debug, int loop,
    int exec_disconnection)
{
    slac_config_t cfg;
    slac_client_t *client;
    slac_if_t sif;
    slac_match_request_t req;
    slac_match_result_t out;
    slac_err_t rc;

    if (!iname)
        return -1;

    cfg.log_level = (slac_log_level_t)debug;
    client = slac_client_create (&cfg);
    if (!client)
        return -1;

    sif.ifname = iname;
    rc = slac_client_open (client, &sif);
    if (rc != SLAC_OK)
    {
        slac_client_destroy (client);
        return -1;
    }

    memset (&req, 0, sizeof (req));
    memcpy (req.key.nid, nid, sizeof (req.key.nid));
    memcpy (req.key.nmk, nmk, sizeof (req.key.nmk));
    req.alias_mac_compatible = alias_mac_compatible;
    req.vendor_status_check = test_flag ? 1 : 0;
    req.exec_disconnection = exec_disconnection;

    if (loop)
        req.exec_disconnection = 1;

    do
    {
        rc = slac_match_once (client, &req, &out);
        if (rc != SLAC_OK || !out.matched)
        {
            slac_client_close (client);
            slac_client_destroy (client);
            return -1;
        }
        if (loop)
            sleep_ms (8000);
    } while (loop);

    slac_client_close (client);
    slac_client_destroy (client);
    return 0;
}