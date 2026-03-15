#include "zetapacket/cli.h"

#include <getopt.h>

#include <iostream>
#include <sstream>

namespace zetapacket {

static void usage(const char* argv0) {
  std::cerr
      << "Usage: " << argv0 << " --backend kernel_udp|af_xdp|dpdk --iface eth0 [options]\n"
      << "Options:\n"
      << "  --backend kernel_udp|af_xdp|dpdk\n"
      << "  --iface interface name (default eth0)\n"
      << "  --port UDP port (default 9000)\n"
      << "  --queue RX queue (AF_XDP/DPDK)\n"
      << "  --seconds run duration\n"
      << "  --batch batch size (default 64)\n"
      << "  --cpu pin RX thread to CPU\n"
      << "  --busy_poll enable busy polling (kernel UDP only)\n"
      << "  --flows expected number of flows (for seq tracking)\n"
      << "  --packet_size expected UDP payload size (for metadata)\n"
      << "  --out CSV output path\n"
      << "  --xdp_obj path to XDP object (default: build output)\n"
      << "  --af_xdp_zero_copy request AF_XDP zero-copy bind\n"
      << "  --umem_frames UMEM frame count\n"
      << "  --frame_size UMEM frame size\n"
      << "  --rx_ring RX ring size\n"
      << "  --fill_ring FILL ring size\n"
      << "  --dpdk_args \"EAL args\"\n"
      << "  --dpdk_burst DPDK RX burst size\n"
      << "  --dpdk_mbufs DPDK mempool size\n\n"
      << "Examples:\n"
      << "  ./zetapacket --backend=kernel_udp\n"
      << "  ./zetapacket --backend=af_xdp\n"
      << "  ./zetapacket --backend=dpdk --dpdk_args=\"-l 0-1 -n 4\"\n";
}

static bool parse_backend(const std::string& s, Backend& backend) {
  if (s == "kernel_udp") {
    backend = Backend::KernelUdp;
    return true;
  }
  if (s == "af_xdp") {
    backend = Backend::AfXdp;
    return true;
  }
  if (s == "dpdk") {
    backend = Backend::Dpdk;
    return true;
  }
  return false;
}

std::string Options::config_string() const {
  std::ostringstream oss;
  oss << "backend=" << static_cast<int>(backend)
      << ";iface=" << iface
      << ";port=" << port
      << ";queue=" << queue
      << ";seconds=" << seconds
      << ";batch=" << batch
      << ";cpu=" << cpu
      << ";busy_poll=" << (busy_poll ? 1 : 0)
      << ";af_xdp_zero_copy=" << (af_xdp_zero_copy ? 1 : 0)
      << ";umem_frames=" << umem_frames
      << ";frame_size=" << frame_size
      << ";rx_ring=" << rx_ring
      << ";fill_ring=" << fill_ring
      << ";dpdk_args=" << dpdk_args
      << ";dpdk_burst=" << dpdk_burst
      << ";dpdk_mbufs=" << dpdk_mbufs
      << ";flows=" << flows
      << ";packet_size=" << packet_size;
  return oss.str();
}

bool parse_cli(int argc, char** argv, Options& opt) {
  static option long_opts[] = {
      {"backend", required_argument, nullptr, 'k'},
      {"iface", required_argument, nullptr, 'i'},
      {"port", required_argument, nullptr, 'p'},
      {"queue", required_argument, nullptr, 'q'},
      {"seconds", required_argument, nullptr, 's'},
      {"batch", required_argument, nullptr, 'b'},
      {"cpu", required_argument, nullptr, 'c'},
      {"busy_poll", no_argument, nullptr, 'B'},
      {"flows", required_argument, nullptr, 'f'},
      {"packet_size", required_argument, nullptr, 'S'},
      {"out", required_argument, nullptr, 'o'},
      {"xdp_obj", required_argument, nullptr, 'x'},
      {"af_xdp_zero_copy", no_argument, nullptr, 1000},
      {"umem_frames", required_argument, nullptr, 1001},
      {"frame_size", required_argument, nullptr, 1002},
      {"rx_ring", required_argument, nullptr, 1003},
      {"fill_ring", required_argument, nullptr, 1004},
      {"dpdk_args", required_argument, nullptr, 1005},
      {"dpdk_burst", required_argument, nullptr, 1006},
      {"dpdk_mbufs", required_argument, nullptr, 1007},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0},
  };

  int ch;
  while ((ch = getopt_long(argc, argv, "k:i:p:q:s:b:c:f:S:o:x:hB", long_opts, nullptr)) != -1) {
    switch (ch) {
      case 'k': {
        Backend backend;
        if (!parse_backend(optarg, backend)) {
          usage(argv[0]);
          return false;
        }
        opt.backend = backend;
        break;
      }
      case 'i':
        opt.iface = optarg;
        break;
      case 'p':
        opt.port = static_cast<uint16_t>(std::stoi(optarg));
        break;
      case 'q':
        opt.queue = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 's':
        opt.seconds = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 'b':
        opt.batch = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 'c':
        opt.cpu = std::stoi(optarg);
        break;
      case 'B':
        opt.busy_poll = true;
        break;
      case 'f':
        opt.flows = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 'S':
        opt.packet_size = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 'o':
        opt.out = optarg;
        break;
      case 'x':
        opt.xdp_obj_path = optarg;
        break;
      case 1000:
        opt.af_xdp_zero_copy = true;
        break;
      case 1001:
        opt.umem_frames = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 1002:
        opt.frame_size = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 1003:
        opt.rx_ring = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 1004:
        opt.fill_ring = static_cast<uint32_t>(std::stoul(optarg));
        break;
      case 1005:
        opt.dpdk_args = optarg;
        break;
      case 1006:
        opt.dpdk_burst = static_cast<uint16_t>(std::stoul(optarg));
        break;
      case 1007:
        opt.dpdk_mbufs = static_cast<uint16_t>(std::stoul(optarg));
        break;
      case 'h':
        usage(argv[0]);
        return false;
      default:
        usage(argv[0]);
        return false;
    }
  }

  return true;
}

}  // namespace zetapacket
