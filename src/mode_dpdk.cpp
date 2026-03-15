#include "zetapacket/modes.h"

#include "zetapacket/payload.h"
#include "zetapacket/seq.h"
#include "zetapacket/util.h"

#include <iostream>

#ifdef ZETAPACKET_HAS_DPDK
#include <rte_byteorder.h>
#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_udp.h>

#include <sstream>
#include <string>
#include <vector>
#endif

namespace zetapacket {

#ifdef ZETAPACKET_HAS_DPDK
static std::vector<std::string> split_dpdk_args(const std::string& s) {
  std::vector<std::string> out;
  std::istringstream iss(s);
  std::string tok;
  while (iss >> tok) {
    out.push_back(tok);
  }
  return out;
}

static bool init_dpdk_port(uint16_t port_id, uint16_t queue_id, uint16_t rx_ring_size,
                           rte_mempool* mbuf_pool) {
  rte_eth_conf port_conf{};
  port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE;

  int rc = rte_eth_dev_configure(port_id, 1, 0, &port_conf);
  if (rc < 0) {
    std::cerr << "rte_eth_dev_configure failed: " << rc << "\n";
    return false;
  }

  rc = rte_eth_rx_queue_setup(port_id, queue_id, rx_ring_size,
                              rte_eth_dev_socket_id(port_id), nullptr, mbuf_pool);
  if (rc < 0) {
    std::cerr << "rte_eth_rx_queue_setup failed: " << rc << "\n";
    return false;
  }

  rc = rte_eth_dev_start(port_id);
  if (rc < 0) {
    std::cerr << "rte_eth_dev_start failed: " << rc << "\n";
    return false;
  }

  rte_eth_promiscuous_enable(port_id);
  return true;
}
#endif

bool run_dpdk_backend(const Options& opt, RunStats& stats) {
#ifndef ZETAPACKET_HAS_DPDK
  (void)opt;
  (void)stats;
  std::cerr << "DPDK backend requested, but libdpdk was not found at build time.\n"
            << "Reconfigure with libdpdk installed, or pass -DZETAPACKET_ENABLE_DPDK=OFF to build without it.\n";
  return false;
#else
  std::vector<std::string> extra = split_dpdk_args(opt.dpdk_args);
  std::vector<std::string> args;
  args.push_back("zetapacket");
  args.insert(args.end(), extra.begin(), extra.end());

  std::vector<char*> argv;
  argv.reserve(args.size());
  for (std::string& arg : args) {
    argv.push_back(arg.data());
  }

  int eal_rc = rte_eal_init(static_cast<int>(argv.size()), argv.data());
  if (eal_rc < 0) {
    std::cerr << "rte_eal_init failed\n";
    return false;
  }

  const uint16_t port_id = 0;
  if (rte_eth_dev_count_avail() == 0) {
    std::cerr << "No DPDK ports available\n";
    return false;
  }

  rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
      "ZETAPACKET_MBUF_POOL", opt.dpdk_mbufs, 256, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
      rte_socket_id());
  if (!mbuf_pool) {
    std::cerr << "rte_pktmbuf_pool_create failed\n";
    return false;
  }

  if (!init_dpdk_port(port_id, static_cast<uint16_t>(opt.queue),
                      static_cast<uint16_t>(opt.rx_ring), mbuf_pool)) {
    return false;
  }

  SeqTracker seq(opt.flows);
  std::vector<rte_mbuf*> burst(opt.dpdk_burst);

  stats.start_ns = now_ns();
  const uint64_t end_target = stats.start_ns + uint64_t(opt.seconds) * 1000000000ull;

  while (now_ns() < end_target) {
    const uint16_t nb_rx = rte_eth_rx_burst(port_id, static_cast<uint16_t>(opt.queue),
                                            burst.data(), opt.dpdk_burst);
    if (nb_rx == 0) {
      continue;
    }

    for (uint16_t i = 0; i < nb_rx; ++i) {
      rte_mbuf* mbuf = burst[i];
      const uint64_t t0 = now_ns();

      if (rte_pktmbuf_data_len(mbuf) < sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) +
                                       sizeof(rte_udp_hdr) + sizeof(PayloadHeader)) {
        stats.bad_pkts += 1;
        rte_pktmbuf_free(mbuf);
        continue;
      }

      auto* eth = rte_pktmbuf_mtod(mbuf, rte_ether_hdr*);
      if (rte_be_to_cpu_16(eth->ether_type) != RTE_ETHER_TYPE_IPV4) {
        rte_pktmbuf_free(mbuf);
        continue;
      }

      auto* ip = reinterpret_cast<rte_ipv4_hdr*>(eth + 1);
      if (ip->next_proto_id != IPPROTO_UDP) {
        rte_pktmbuf_free(mbuf);
        continue;
      }

      const uint8_t ihl = static_cast<uint8_t>(ip->version_ihl & 0x0F) * 4;
      auto* udp = reinterpret_cast<rte_udp_hdr*>(reinterpret_cast<uint8_t*>(ip) + ihl);
      auto* ph = reinterpret_cast<PayloadHeader*>(reinterpret_cast<uint8_t*>(udp) + sizeof(rte_udp_hdr));
      seq.observe(ph->flow_id, ph->seq);

      const uint64_t t1 = now_ns();
      stats.ingest_hist.add(t1 - t0);
      stats.rx_pkts += 1;
      rte_pktmbuf_free(mbuf);
    }
  }

  stats.end_ns = now_ns();
  stats.gaps = seq.total_gaps();

  rte_eth_dev_stop(port_id);
  rte_eth_dev_close(port_id);
  rte_eal_cleanup();
  return true;
#endif
}

}  // namespace zetapacket
