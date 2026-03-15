#include "zetapacket/modes.h"

#include "zetapacket/payload.h"
#include "zetapacket/seq.h"
#include "zetapacket/util.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>

namespace zetapacket {

static bool set_busy_poll(int fd) {
#ifdef SO_BUSY_POLL
  int usec = 50;
  if (setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, &usec, sizeof(usec)) != 0) {
    return false;
  }
  return true;
#else
  (void)fd;
  return false;
#endif
}

bool run_kernel_udp_backend(const Options& opt, RunStats& stats) {
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
    return false;
  }

  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  if (opt.busy_poll) {
    set_busy_poll(fd);
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(opt.port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    std::cerr << "bind() failed: " << std::strerror(errno) << "\n";
    ::close(fd);
    return false;
  }

  std::vector<uint8_t> buf(std::max(opt.packet_size, 64u));
  SeqTracker seq(opt.flows);
  const uint32_t batch = std::max(opt.batch, 1u);

  std::vector<std::vector<uint8_t>> bufs(batch, std::vector<uint8_t>(buf.size()));
  std::vector<iovec> iovs(batch);
  std::vector<mmsghdr> msgs(batch);
  for (uint32_t i = 0; i < batch; ++i) {
    iovs[i].iov_base = bufs[i].data();
    iovs[i].iov_len = bufs[i].size();
    std::memset(&msgs[i], 0, sizeof(mmsghdr));
    msgs[i].msg_hdr.msg_iov = &iovs[i];
    msgs[i].msg_hdr.msg_iovlen = 1;
  }

  stats.start_ns = now_ns();
  const uint64_t end_target = stats.start_ns + uint64_t(opt.seconds) * 1000000000ull;

  while (now_ns() < end_target) {
    const uint64_t t0 = now_ns();
    timespec timeout{};
    timeout.tv_nsec = 100 * 1000 * 1000;

    int rcvd = ::recvmmsg(fd, msgs.data(), batch, 0, &timeout);
    if (rcvd <= 0) {
      if (rcvd < 0 && errno == EINTR) {
        continue;
      }
      continue;
    }

    for (int i = 0; i < rcvd; ++i) {
      const size_t n = msgs[i].msg_len;
      if (n < sizeof(PayloadHeader)) {
        stats.bad_pkts += 1;
        continue;
      }
      auto* ph = reinterpret_cast<PayloadHeader*>(bufs[i].data());
      seq.observe(ph->flow_id, ph->seq);
      const uint64_t t2 = now_ns();
      stats.rx_pkts += 1;
      stats.ingest_hist.add(t2 - t0);
    }
  }

  stats.end_ns = now_ns();
  stats.gaps = seq.total_gaps();
  ::close(fd);
  return true;
}

}  // namespace zetapacket
