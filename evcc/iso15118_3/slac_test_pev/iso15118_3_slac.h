#ifndef ISO15118_3_SLAC_TEST_PEV_ISO15118_3_SLAC_H
#define ISO15118_3_SLAC_TEST_PEV_ISO15118_3_SLAC_H

#include <stdint.h>

typedef struct slac_test_pev_options
{
    char *ifname;
    char *config_file;
    char *specific_mac;
    int mstar_test_flag;
    int loop;
    int debug_display;
    int exec_disconnection;
    int alias_mac_compatible;
    uint8_t nid[7];
    uint8_t nmk[16];
    uint8_t alias_dest_mac[6];
} slac_test_pev_options_t;

int slac_test_pev_main(int argc, char *argv[]);
int slac_test_pev_run(const slac_test_pev_options_t *options);

#endif /* ISO15118_3_SLAC_TEST_PEV_ISO15118_3_SLAC_H */
