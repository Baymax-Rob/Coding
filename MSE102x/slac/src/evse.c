/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */

#include "evse_slac.h"


void
evse_print_usage ()
{
    printf ("Usage: sudo ./slac_evse [Options]\n");
    printf ("-i <network interface name>\tspecify your network interface \
(necessary)\n");
    printf ("-f <file name>\t\t\tspecify the file name including NID and NMK \
(necessary)\n");
    printf ("-a\t\t\t\tcompatible with specific alias MAC address\n");
    printf ("-c\t\t\t\texecute the disconnection after SLAC test\n");
    printf ("-d\t\t\t\tprint the debug message\n");
    printf ("-h\t\t\t\tprint the help message\n");
    printf ("-l\t\t\t\texecute the test constantly\n");
    printf ("-v\t\t\t\tprint the debug message in verbose mode\n");

    exit (-1);
}


int
evse_main (char *iname, unsigned char nid[7], unsigned char nmk[16],
    int alias_mac_compatible, int debug, int loop, int exec_disconnection)
{
    int result = 0, i;

    void *opened_interface = NULL;
    unsigned char *evse_mac;
    DEBUG_DISPLAY = debug;

    evse evse =
    {
        .pev_item_head = NULL
    };
    evse.pev_item_head = (pev_item *)malloc (sizeof (pev_item));
    evse.pev_item_head->next = NULL;

    memcpy (evse.nid, nid, 7);
    memcpy (evse.nmk, nmk, 16);

    LOG_PRINT (LOG_DEBUG, "-----  [This is EVSE host] -----\n");

    LOG_PRINT (LOG_VERBOSE, "Print EVSE's NID\n");
    for (i = 0; i < 7; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "0x%02x  ", evse.nid[i]);
    }

    LOG_PRINT (LOG_VERBOSE, "Print EVSE's NMK\n");
    for (i = 0; i < 16; i++){
        LOG_PRINT (LOG_VERBOSE, "0x%02x  ", evse.nmk[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n");


#ifdef _WIN32
    hpav_channel_t *channel =
        (hpav_channel_t *)malloc (sizeof (hpav_channel_t));
    opened_interface = channel;
    evse_mac = channel->mac_addr;
#endif // _WIN32
#ifdef __linux__
    iface_t *iface = (iface_t *)malloc (sizeof (iface_t));
    opened_interface = iface;
    evse_mac = iface->mac;
#endif // __linux__

    if (open_interface (iname, opened_interface) != 0)
    {
        exit (-1);
    }

    if (!alias_mac_compatible)
        result = evse_cm_set_key_req_cnf (opened_interface, &evse, mac_link_local);
    else
        result = evse_cm_set_key_req_cnf (opened_interface, &evse, other_mac_link_local);

    if (result != MME_SUCCESS)
    {
        LOG_PRINT (LOG_INFO,
        "Error: Fails to set key, please check the network interface!\n");
        exit (-1);
    }

    if (!alias_mac_compatible)
    {
        result = vs_get_status_req_send_cnf_receive (opened_interface);
        if (result == CONNECT_LINK)
        {
            LOG_PRINT
                (LOG_INFO,"Error: Your EVSE is connected to PEV now.\n");
            exit (-1);
        }
        else if (result == ERROR_LINK)
        {
            LOG_PRINT
            (LOG_INFO, "Error: vs_get_status_req_send_cnf_receive()\n");
            exit (-1);
        }
    }

    LOG_PRINT (LOG_INFO, "[EVSE] starts listening...\n");

    while (1)
    {
        mme_ctx_t cnf_ctx;
        unsigned char recv_buff[ETH_DATA_LEN];
        unsigned int  pkt_len = 0, num_len = 0;
        int result;
        unsigned char *raw_pkt;

        //rewind raw_pkt pointer to begining of the packet holder
        unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
        raw_pkt = pkt_holder;

        // Todo not CM_SLAC_MATCH_CNF
        result = mme_init
            (&cnf_ctx, CM_SLAC_MATCH_CNF, recv_buff, ETH_DATA_LEN);
        if ( (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 1, 0))
                                                    == MME_SUCCESS)
        {
            MME_t *recv_mme = (MME_t *) raw_pkt;

            // Check if destination mac address is same as my own
            if (mme_check_dest_mac (recv_mme->mme_dest, evse_mac) == 0)
            {
                mme_remove_header
                    (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

                if (recv_mme->mmtype == CM_SLAC_PARM_REQ)
                {
                    result = evse_cm_slac_parm_req_cnf
                    (opened_interface, &evse, &cnf_ctx, recv_mme->mme_src);
                }
                else if (recv_mme->mmtype == CM_START_ATTEN_CHAR_IND)
                {
                    result = evse_cm_start_atten_char_ind_receive
                                    (&cnf_ctx, recv_mme->mme_src);
                    pev_item *tmp = search_pev_item_list
                            (evse.pev_item_head, recv_mme->mme_src);
                    if ( (tmp != NULL) && (tmp->matching_state == 0))
                    {
                        tmp->matching_state =
                            RECEIVED_CM_START_ATTEN_CHAR_IND;
                        /**
                         * TT_EVSE_match_MNBC=600ms, TT_match_response=200ms.
                         * Wait 600ms to send first CM_ATTEN_CHAR.IND
                         * and then resend CM_ATTEN_CHAR.IND every 200ms
                         * (C_EV_match retry times) if EVSE
                         *  didnot receive CM_ATTEN_CHAR.RSP.
                         */
                        make_evse_timer (tmp, TT_EVSE_match_MNBC * 100,
                                                TT_match_response);
                    }
                    if (tmp == NULL)
                    {
                        LOG_PRINT (LOG_VERBOSE,
                            "[EVSE] receives CM_START_ATTEN_CHAR_IND\
 from unknown PEV that didn't send CM_SLAC_PARM.REQ.\n");
                    }
                }
                else if (recv_mme->mmtype == CM_MNBC_SOUND_IND)
                {
                    result = evse_cm_mnbc_sound_ind_receive (&cnf_ctx);
                }
                else if (recv_mme->mmtype == CM_ATTEN_PROFILE_IND)
                {
                    result =
                        evse_cm_atten_profile_ind_receive (&evse, &cnf_ctx);
                }
                else if (recv_mme->mmtype == CM_ATTEN_CHAR_RSP)
                {
                    result =
                        evse_cm_atten_char_rsp_receive (&evse, &cnf_ctx);
                }
                else if (recv_mme->mmtype == CM_SLAC_MATCH_REQ)
                {
                    pev_item *tmp = search_pev_item_list
                        (evse.pev_item_head, recv_mme->mme_src);
                    if (tmp != NULL)
                    {
                        result = evse_cm_slac_match_req_receive_cnf_send
                                    (opened_interface, &cnf_ctx, &evse, tmp);
                        if (!result)
                        {
                            tmp->matching_state = SENT_CM_SLAC_MATCH_CNF;
                            if (alias_mac_compatible)
                            {
                                if (loop != EXEC_LOOP_TEST)
                                    break;
                            }
                            else
                            {
                                /**
                                 * Assign interface_ptr so later timer handler
                                 * can check if channel was closed.
                                 * If closed, we don't need to send
                                 * VS_GET_STATUS.REQ to check.
                                 */
                                tmp->interface_ptr = opened_interface;
                                /**
                                 * TT_match_join is the max time between
                                 * CM_SLAC_MATCH.CNF and link establishment.
                                 * This timer is to send VS_GET_STATUS.REQ.
                                 */
                                make_evse_timer (tmp, TT_match_join, 0);
                            }
                        }
                    }
                    else if (tmp == NULL)
                    {
                        LOG_PRINT (LOG_VERBOSE,
                 "[EVSE] receives CM_SLAC_MATCH.REQ from unknown PEV.\n");
                    }

                }
                else if (recv_mme->mmtype == VS_GET_STATUS_CNF)
                {
                    LOG_PRINT (LOG_DEBUG,
                        "[EVSE] receives VS_GET_STATUS.CNF\n");
                    vs_get_status_cnf vs_cnf;
                    memset (&vs_cnf, 0, sizeof (vs_cnf));
                    result =
                    mme_pull (&cnf_ctx, &vs_cnf, sizeof (vs_cnf), &num_len);
                    if (result != MME_SUCCESS)
                    {
                        LOG_PRINT (LOG_VERBOSE,
                            "mme_pull() error, result=%d\n", result);
                        continue;
                    }
                    LOG_PRINT (LOG_DEBUG, "VS_GET_STATUS_CNF.Link_State \
= %d (1:connected,  0:disconnected)\n", vs_cnf.link_state);
                    if (vs_cnf.link_state)
                    {
                        LOG_PRINT
                        (LOG_DEBUG, "-----------------------------------\n");
                        LOG_PRINT (LOG_DEBUG, "[EVSE] is connected to PEV.\n");
                        LOG_PRINT
                        (LOG_VERBOSE, "[EVSE] Switched to Matched state.\n");
                        LOG_PRINT
                        (LOG_DEBUG, "-----------------------------------\n");
                        LOG_PRINT (LOG_DEBUG, "[EVSE] Charging...\n\n");
                        sleep_ms (CHARGING_TIME);
                        if (exec_disconnection)
                        {
                            nid[3]++;
                            nmk[3]++;
                            memcpy (evse.nid, nid, 7);
                            memcpy (evse.nmk, nmk, 16);
                            /* For windows Tool */
                            if (!alias_mac_compatible)
                                result = evse_cm_set_key_req_cnf
                                (opened_interface, &evse, mac_link_local);
                            else
                                result = evse_cm_set_key_req_cnf
                                    (opened_interface, &evse,
                                    other_mac_link_local);
                        }
                        result =
                            vs_get_status_cnf_receive (opened_interface);
                        if (result == ERROR_LINK)
                        {
                            LOG_PRINT (LOG_VERBOSE,
                                "After charging, VS_GET_STATUS_CNF received timeout!\n");
                        }
                        if (result == DISCONNECT_LINK)
                        {
                            LOG_PRINT (LOG_DEBUG, "[EVSE] Charging complete!\n");
                            LOG_PRINT (LOG_DEBUG, "[EVSE] was disconnected from PEV.\n");
                        }
                        if (loop != EXEC_LOOP_TEST)  break;
                    }
                }
                else
                {
                    LOG_PRINT (LOG_VERBOSE,
                        "received the wrong mmtype mme, mmtype = 0x%x\n",
                        recv_mme->mmtype);
                }
            }
        }
    }
    close_interface (&opened_interface);

    return 0;
}
