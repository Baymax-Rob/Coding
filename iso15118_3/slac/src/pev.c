/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
#include "pev_slac.h"

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
    exit (-1);
}


int
pev_main (char *iname, unsigned char nid[7], unsigned char nmk[16],
    int test_flag, int alias_mac_compatible, int debug, int loop,
    int exec_disconnection)
{
    DEBUG_DISPLAY = debug;
    /**
     * For loop test, you must execute set key to disconnect.
     * This setting is only for PEV side.
     */
    if (loop)
        exec_disconnection = 1;

    do
    {
        LOG_PRINT (LOG_DEBUG, "-----  [This is PEV host] -----\n");
        int result = 0;
        iface_error_t channel_result = 0;
        int i, error_flag = 0;

        pev pev =
        {
            .state = DISCONNECTED,
            .current_state = SENT_CM_SLAC_PARM_REQ,
            .cm_slac_parm_timer_flag = 1,
            .cm_slac_parm_req_retry = C_EV_match_retry,
            .potential_EVSE = 0,
            .cm_start_atten_char_timer_flag = 1,
            .cm_slac_match_req_retry = C_EV_match_retry,
            .evse_item_head = NULL
        };
        pev.evse_item_head = (evse_item *)malloc (sizeof (evse_item));
        pev.evse_item_head->next = NULL;

        memcpy (pev.nid, nid, 7);
        memcpy (pev.nmk, nmk, 16);

        LOG_PRINT (LOG_VERBOSE, "Print PEV's NID\n");
        for (i = 0; i < 7; i++)
        {
            LOG_PRINT (LOG_VERBOSE, "0x%02x ", pev.nid[i]);
        }
        LOG_PRINT (LOG_VERBOSE, "Print PEV's NMK\n");
        for (i = 0; i < 16; i++)
        {
            LOG_PRINT (LOG_VERBOSE, "0x%02x ", pev.nmk[i]);
        }
        LOG_PRINT (LOG_VERBOSE, "\n");

        void *opened_interface = NULL;
#ifdef _WIN32
        hpav_channel_t *channel =
            (hpav_channel_t *)malloc (sizeof (hpav_channel_t));
        opened_interface = channel;
#endif // _WIN32
#ifdef __linux__
        iface_t *iface = (iface_t *)malloc (sizeof (iface_t));
        opened_interface = iface;
#endif // __linux__

        channel_result = open_interface (iname, opened_interface);
        if (channel_result != 0)
        {
            exit (-1);
        }

        if (test_flag)
        {
            if (alias_mac_compatible)
            {

                result = pev_cm_set_key_req_disconnect
                    (opened_interface, &pev, other_mac_link_local);
            }
            else
            {
                result = pev_cm_set_key_req_disconnect
                    (opened_interface, &pev, mac_link_local);
            }

            if (result != MME_SUCCESS)
            {
                LOG_PRINT (LOG_DEBUG,
        "Error: Fails to set key, please check the network interface!\n");
                exit (-1);
            }
        }

        if (pev.state == DISCONNECTED)
        {
            if (test_flag && !alias_mac_compatible)
            {
                result = vs_get_status_req_send_cnf_receive (opened_interface);
                if (result == CONNECT_LINK)
                {
                    LOG_PRINT
                    (LOG_DEBUG, "Error: Your PEV is connected to EVSE now.");
                    LOG_PRINT
                    (LOG_DEBUG, "Please execute this program again.\n");
                    exit (-1);
                }
                if (result == ERROR_LINK)
                {
                    LOG_PRINT (LOG_VERBOSE,
                        "Error: vs_get_status_req_send_cnf_receive()\n");
                    exit (-1);
                }
            }
            pev.state = UNMATCHED;
        }

        if (pev.state == UNMATCHED)
        {
            while ( (pev.current_state == 0) &&
                (pev.cm_slac_parm_req_retry > 0))
            {
                result = pev_cm_slac_parm_req_send (opened_interface, &pev);
                pev.cm_slac_parm_timer_flag = 1;
                pev.cm_slac_parm_req_retry--;

                /**
                 * This timer was used to stop receive CM_SLAC_PARM.CNF
                 * by setting cm_slac_parm_timer_flag to 0.
                 */
                result = make_pev_timer (&pev, TT_match_response, 0);

                while (pev.cm_slac_parm_timer_flag)
                {
                    result =
                    pev_cm_slac_parm_cnf_receive (opened_interface, &pev);
                    if (result == MME_SUCCESS)
                    {
                        pev.current_state = RECEIVED_CM_SLAC_PARM_CNF;
                    }
                }

                if (pev.cm_slac_parm_req_retry == 0 && pev.current_state != 1)
                {
                    LOG_PRINT (LOG_DEBUG,
                        "[PEV] fails to retry CM_SLAC_PARM_REQ_SEND \n");
                    error_flag = 1;
                }
            }
            if (pev.current_state == 1)
            {
                /* This timer is used to receive CM_ATTEN_CHAR.IND */
                result = make_pev_timer (&pev, TT_EV_atten_results, 0);
                result = pev_cm_start_atten_char_ind_send (opened_interface);
                result = pev_cm_mnbc_sound_ind_send (opened_interface);
                pev.current_state = SENT_CM_START_ATTEN_CHAR_IND;
            }

            atten_char_ind atten_ind;
            unsigned char rsp_dest[ETH_ALEN];
            if (pev.current_state == 2)
            {
                while (pev.cm_start_atten_char_timer_flag)
                {
                    result = pev_cm_atten_char_ind_receive (opened_interface,
                            &pev, &atten_ind, &rsp_dest[0]);
                    if (result == MME_SUCCESS)
                    {
                        pev.current_state = RECEIVED_CM_ATTEN_CHAR_IND;
                        result = pev_cm_atten_char_rsp_send
                            (opened_interface, &atten_ind, &rsp_dest[0]);
                    }
                }
                /**
                 * Within TT_EV_atten_result, if PEV didn't receive
                 * CM_ATTEN_CHAR.IND, PEV should consider that the
                 * matching process failed.
                 */
                if (pev.current_state != 3)
                {
                    LOG_PRINT (LOG_DEBUG,
                        "[PEV] didn't receive CM_ATTEN_CHAR.IND!\n");
                    error_flag = 1;
                }
            }

            while (pev.current_state == 3 && pev.cm_slac_match_req_retry > 0)
            {
                evse_item *min_node =
                    select_evse_item_list (pev.evse_item_head);
                result =
                    pev_cm_slac_match_req_send (opened_interface, min_node);
                result =
                    pev_cm_slac_match_cnf_receive (opened_interface, &pev);
                if (result == MME_SUCCESS)
                {
                    pev.current_state = RECEIVED_CM_SLAC_MATCH_CNF;
                }
                pev.cm_slac_match_req_retry--;

                /**
                 * A.9.4.3.2 [V2G3-A09-94]
                 * If after the retransmission of CM_SLAC_MATCH.REQ and
                 * EV has not received valid response, the matching process
                 * shall be considered as FAILED.
                 */
                if (pev.cm_slac_match_req_retry == 0)
                {
                    error_flag = 1;
                }
            }

            if (pev.current_state == 4)
            {
                // Sending CM_SET_KEY.REQ to connect with EVSE
                if (alias_mac_compatible)
                {
                    result = pev_cm_set_key_req_send
                        (opened_interface, &pev, other_mac_link_local);
                }
                else
                {
                    result = pev_cm_set_key_req_send
                    (opened_interface, &pev, mac_link_local);
                }
                if (test_flag && !alias_mac_compatible)
                {
                    result = vs_get_status_cnf_receive (opened_interface);
                    if (result == CONNECT_LINK)
                    {
                        pev.state = MATCHED;
                        LOG_PRINT
                        (LOG_DEBUG, "------------------------------------\n");
                        LOG_PRINT (LOG_DEBUG, "[PEV] is connected to EVSE.\n");
                        LOG_PRINT
                        (LOG_DEBUG, "------------------------------------\n");
                    }
                    else if (result == ERROR_LINK)
                    {
                        result =
                        vs_get_status_req_send_cnf_receive (opened_interface);
                        if (result == CONNECT_LINK)
                        {
                            pev.state = MATCHED;
                        }
                        else
                        {
                            error_flag = 1;
                        }
                    }
                    else if (result == DISCONNECT_LINK)
                    {
                        error_flag = 1;
                    }
                }
                else if (alias_mac_compatible)
                {
                    pev.state = MATCHED;
                }
            }
        }
        if (pev.state == MATCHED)
        {
            if (exec_disconnection)
            {
                LOG_PRINT (LOG_DEBUG, "[PEV] Charging...\n\n");
                sleep_ms (CHARGING_TIME);
                LOG_PRINT (LOG_DEBUG, "[PEV] Charging complete!\n");
                pev_cm_set_key_req_disconnect
                    (opened_interface, &pev, mac_link_local);
                LOG_PRINT (LOG_DEBUG, "[PEV] was disconnected from EVSE\n");
                result = vs_get_status_cnf_receive (opened_interface);
                if (result == ERROR_LINK)
                {
                    sleep_ms (2000);    //Wait the response...
                    result =
                        vs_get_status_req_send_cnf_receive (opened_interface);
                    if (result != DISCONNECT_LINK)
                    {
                        error_flag = 1;
                    }
                }
                else if (result == CONNECT_LINK)
                {
                    error_flag = 1;
                }
            }
        }

        LOG_PRINT (LOG_VERBOSE, "\n\nTest Result:  ");
        if (error_flag)
        {
            LOG_PRINT (LOG_VERBOSE, "Test Failed\n\n");
        }
        else
        {
            LOG_PRINT (LOG_VERBOSE, "Test Succeeded\n\n");
        }
        close_interface (&opened_interface);

        /**
         * PEV will sleep 8 secs before restart to run matching process.
         * (User can decide how long you want to wait by yourself)
         */
        if (loop)   sleep_ms (8000);

    }while (loop);

    return 0;
}