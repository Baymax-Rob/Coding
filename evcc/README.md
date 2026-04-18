# evcc (ISO 15118-3 SLAC)

This repository contains a C implementation of the PEV-side SLAC flow for ISO 15118-3.

## Structure

- `iso15118_3/libmme`: low-level MME encode/decode and interface operations
- `iso15118_3/slac`: SLAC state machine and protocol flow
- `iso15118_3/interfaces/c_api`: library-friendly C API (`slac_client_*`, `slac_match_once`)
- `iso15118_3/slac_test_pev`: CLI test entry and argument parsing
- `main.c`: top-level launcher

## Build

From repository root:

```sh
make
```

### Notes

- Linux build requires `pcap.h` (libpcap development headers).
- On Windows, use a toolchain that provides GNU make and gcc.

### Linux dependencies

Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install -y build-essential libpcap-dev
```

RHEL/CentOS/Fedora:

```sh
sudo dnf install -y gcc make libpcap-devel
```

The top-level `make` builds in order:

1. `iso15118_3/libmme`
2. `iso15118_3/slac`
3. `iso15118_3/slac_test_pev`
4. root `main` binary

## Run

Example CLI usage:

```sh
./iso15118_3/slac_test_pev/obj/slac_pev -i <ifname> -f ./iso15118_3/slac_test_pev/pev1.example.txt
```

Use `-h` to print available options.

## Config

Use `iso15118_3/slac_test_pev/pev1.example.txt` as a template for NID/NMK values.
Do not commit production keys.
