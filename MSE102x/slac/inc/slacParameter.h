#ifndef slac_inc_slacParameter_h
#define slac_inc_slacParameter_h
/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */

#define C_EV_match_MNBC 10
#define C_EV_match_retry 2                              // Max 2 nbr
#define C_EV_match_signalattn_direct 10
#define C_EV_match_signalattn_indirect 20
#define C_EV_start_atten_char_inds 3
#define TP_amp_map_exchange 100                 // Max 100 ms
#define TP_EV_batch_msg_interval 20             // Min 20 ms  Max 50 ms
#define TP_EV_match_session 500                 // Max 500 ms
#define TP_EV_SLAC_init 10000                       // Max 10 sec
#define TP_EVSE_avg_atten_calc 100              // Max 100 ms
#define TP_link_ready_notification 1000             // Min 0.2 sec  Max 1 sec
#define TP_match_response 100                       // Max 100 ms


#define TT_amp_map_exchange 200                 // Max 200 ms
#define TT_EV_atten_results 1200                    // Max 1200 ms
#define TT_EVSE_match_MNBC 0x06                  //100 = 0x01, Min val=600
#define TT_EVSE_match_session 10000             // Max 10 sec
#define TT_EVSE_SLAC_init 50000                     // Min 20 sec  Max 50 sec
#define TT_match_join 12000                             // Max 12 sec
#define TT_match_response 200                       // Max 200 ms
#define TT_match_sequence 400                       // Max 400 ms
#define TT_matching_repetition 10000                // Min 10 sec
#define TT_matching_rate 400                            // Min 400 ms

#endif /* slac_inc_slacParameter_h */
