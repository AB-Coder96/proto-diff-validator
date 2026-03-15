# ZetaPacket

ZetaPacket is a Linux packet-ingest prototype that exposes a single runtime switch for selecting the backend:

- `kernel_udp`
- `af_xdp`
- `dpdk`

## Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

If `libdpdk` is installed and discoverable via `pkg-config`, the DPDK backend is compiled in. Otherwise the binary still builds, but `--backend=dpdk` exits with an explanatory runtime error.

## Run

```bash
./zetapacket --backend=kernel_udp --iface eth0 --port 9000
./zetapacket --backend=af_xdp --iface eth0 --queue 0
./zetapacket --backend=dpdk --iface eth0 --queue 0 --dpdk_args="-l 0-1 -n 4"
```

AF_XDP zero-copy can be requested with:

```bash
./zetapacket --backend=af_xdp --af_xdp_zero_copy --iface eth0 --queue 0
```
