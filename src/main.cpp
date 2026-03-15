#include <iostream>
#include <string>

#include "zetapacket/cli.h"
#include "zetapacket/csv.h"
#include "zetapacket/modes.h"
#include "zetapacket/util.h"

using namespace zetapacket;

static const char* backend_str(Backend backend) {
  switch (backend) {
    case Backend::KernelUdp:
      return "kernel_udp";
    case Backend::AfXdp:
      return "af_xdp";
    case Backend::Dpdk:
      return "dpdk";
  }
  return "unknown";
}

int main(int argc, char** argv) {
  Options opt;
  if (!parse_cli(argc, argv, opt)) {
    return 2;
  }

  if (opt.cpu >= 0) {
    pin_thread_to_cpu(opt.cpu);
  }

  RunStats stats;
  bool ok = false;
  switch (opt.backend) {
    case Backend::KernelUdp:
      ok = run_kernel_udp_backend(opt, stats);
      break;
    case Backend::AfXdp:
      ok = run_af_xdp_backend(opt, stats);
      break;
    case Backend::Dpdk:
      ok = run_dpdk_backend(opt, stats);
      break;
  }
  if (!ok) {
    return 1;
  }

  CsvRow row;
  row.kv["backend"] = backend_str(opt.backend);
  row.kv["iface"] = opt.iface;
  row.kv["queue"] = std::to_string(opt.queue);
  row.kv["port"] = std::to_string(opt.port);
  row.kv["seconds"] = std::to_string(opt.seconds);
  row.kv["batch"] = std::to_string(opt.batch);
  row.kv["busy_poll"] = opt.busy_poll ? "1" : "0";
  row.kv["af_xdp_zero_copy"] = opt.af_xdp_zero_copy ? "1" : "0";
  row.kv["flows"] = std::to_string(opt.flows);
  row.kv["packet_size"] = std::to_string(opt.packet_size);
  row.kv["rx_pkts"] = std::to_string(stats.rx_pkts);
  row.kv["gaps"] = std::to_string(stats.gaps);
  row.kv["bad_pkts"] = std::to_string(stats.bad_pkts);
  row.kv["measured_pps"] = std::to_string(stats.measured_pps());
  row.kv["p50_ns"] = std::to_string(stats.p50_ns());
  row.kv["p99_ns"] = std::to_string(stats.p99_ns());
  row.kv["p999_ns"] = std::to_string(stats.p999_ns());
  row.kv["config_hash"] = std::to_string(fnv1a64_str(opt.config_string()));
#ifdef ZETAPACKET_GIT_SHA
  row.kv["git_sha"] = ZETAPACKET_GIT_SHA;
#else
  row.kv["git_sha"] = "unknown";
#endif
  row.kv["kernel"] = uname_string();
  row.kv["cpu_model"] = cpu_model_string();

  if (!ensure_parent_dir(opt.out)) {
    std::cerr << "Failed to create output dir for: " << opt.out << "\n";
    return 1;
  }
  if (!write_csv_summary(opt.out, row)) {
    std::cerr << "Failed to write CSV: " << opt.out << "\n";
    return 1;
  }

  std::cout << "Wrote: " << opt.out << "\n";
  std::cout << "backend=" << backend_str(opt.backend)
            << " pps=" << stats.measured_pps()
            << " p50_ns=" << stats.p50_ns()
            << " p99_ns=" << stats.p99_ns()
            << " p999_ns=" << stats.p999_ns()
            << " gaps=" << stats.gaps
            << " rx=" << stats.rx_pkts << "\n";
  return 0;
}
