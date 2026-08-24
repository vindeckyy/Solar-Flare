# Performance Tuning

In addition to the options available in the [Configuration](configuration.md) section, there are a few additional
system options that can improve SolarFlare performance. Fork-specific network,
scheduling, capture, and audio controls are documented in
[SolarFlare Configuration](CONFIGURATION.md).

## Linux Host Tuning

SolarFlare is designed primarily for Linux hosts and includes several automated and system-level performance optimizations:

### GPU Clocks and Governors
During streaming, SolarFlare automatically requests the GPU `performance` power state on AMD DRM devices via `gpu_governor = true` (reverting to `auto` upon stream teardown). On NVIDIA GPUs, clock stability can be enforced using `nvidia-smi -lgc <boost_clock>` (see [`packaging/linux/redesign`](../packaging/linux/redesign/README.md)).

### CPU Pinning and Real-Time Scheduling
Enabling `cpu_pinning = true` elevates the capture worker to `SCHED_RR` real-time scheduling policy and pins the thread to a non-IRQ, non-SMT core, eliminating context-switch jitter during 120+ FPS capture.

### PipeWire Audio Latency
SolarFlare provides the `pipewire_latency_ms` setting (default `8` ms, tunable down to `1` ms for low-latency audio setups) which requests a small quantum buffer from the PipeWire compositor.

### Socket Buffering and Low-Latency Polling
- `enet_4mib_buffer = true`: Expands the UDP socket send and receive buffers to 4 MiB to prevent dropped packets during high-bitrate frame bursts.
- `busy_poll_us = 50`: Enables `SO_BUSY_POLL` socket polling, reducing receive-side wakeup latency on wireless networks.
- `dscp_qos = true`: Tags outgoing ENet streaming packets with DSCP CS3 for Quality-of-Service prioritization on upstream routers.

## Windows GPU Settings

### AMD

In Windows, enabling *Enhanced Sync* in AMD's settings may help reduce the latency by an additional frame. This
applies to `amfenc` and `libx264`.

### NVIDIA

Enabling *Fast Sync* in Nvidia settings may help reduce latency.

<div class="section_buttons">

| Previous            |          Next |
|:--------------------|--------------:|
| [Guides](guides.md) | [API](api.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
