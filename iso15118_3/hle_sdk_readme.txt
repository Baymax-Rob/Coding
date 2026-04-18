HLE SDK Readme version 1.0

1. Introduction
1.1. Overview
    HLE SDK is developed for Matching EV-EVSE process. It is developed based on ISO 15118-3. In this SDK, both EV and EVSE will
perform the following steps:

1.  Parameter exchange for signal strength measurement: before the signal measure strength measurement, the EV broadcasts the
    MME including parameters used for the following steps. Any unmatched EVSE will send a response if it received the broadcast
    parameters from EV.
2.  Signal strength measurement: by means of signal strength measurement, EV can choose which EVSE is the right one to connect.
3.  Logical network parameter exchange: in order to join the logical network of the EVSE, EV will request the parameters from
    EVSE.
4.  Joining the logical network: EV joins the logical network of EVSE.

1.2.    Architecture
    HLE SDK includes four parts: libmme, slac, slac_test_pev and slac_test_evse.
1.2.1   libmme
    This folder provides libraries that handle MME operations. In our sdk, EVSE and PEV are developed based on these libraries.
Developers can use libmme to handle any different MMEs if you need. In other words, developers can design your own EVSE and PEV to
run SLAC process by using libmme.

1.2.2   slac
    This folder provides main programs of EVSE and PEV and the functions used by EVSE and PEV to handle MME.

1.2.3   slac_test_pev and slac_test_evse
    These folders are used to handle command line arguments and pass them to call main programs of EVSE and PEV.


2.  Build the SDK
    Go to folder named "hle_sdk". Use the following command to build library and executable file.
    $ make

    If you want to build the executable file for different target platform, you can use the following command:
    $ make CROSS_COMPILE=arm-linux-     # use ARM toolchain as example


3.  Usage
3.1.    EVSE Host
    You need to start the EVSE first. Go to folder named "slac_test_evse". Use the following command to start EVSE.

    $ sudo ./obj/slac_evse -i <network interface name> -f <file name>
    -i  specify the network interface connected to your device. (Necessary)
    -f  specify the file name including NID and NMK. We provide the sample file in slac_test_evse/slac_test_pev folder. (Necessary)
    -a  compatible with specific alias MAC address. (Optional)
    -c  execute the disconnection after SLAC test. (Optional)
    -d  print more detail information for debug. (Optional)
    -h  print the help message
    -l  execute the SLAC process constantly. (Optional)
    -v  print the debug message in verbose mode. (Optional)


3.2.    EV Host
    Then you can use the following command to start PEV. Go to folder named "slac_test_pev". PEV will start to run the matching process to find matched EVSE.

    $ sudo ./obj/slac_pev -i <network interface name> -f <file name>
    -i  specify the network interface connected to your device. (Necessary)
    -f  specify the file name including NID and NMK. We provide the sample file in evse/pev folder. (Necessary)
    -a  compatible with specific alias MAC address. (Optional)
    -c  execute the disconnection after SLAC test. (Optional)
    -d  print more detail information for debug. (Optional)
    -h  print the help message
    -l  execute the SLAC process constantly. (Optional)
    -v  print the debug message in verbose mode. (Optional)


4.  Revision History
Release Note 1.0:
    Initial Release.
