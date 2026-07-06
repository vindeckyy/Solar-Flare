/**
 * @file src/platform/linux/hermes_kms.cpp
 * @brief Hermes-KMS capture backend.
 *
 * Implements:
 *   1. Module + device detection (probe_hermes_kms)
 *   2. Source-selector entry points (display_names / display / verify)
 *   3. The capture loop: WAIT_FRAME -> ACQUIRE_FRAME -> push the DMA-BUF
 *      to the encoder consumer (VAAPI / NVENC / AMF). The consumer imports
 *      the fd via the existing encoder import path; no CPU readback.
 */
#include "hermes_kms.h"
#include "third-party/hermes-kms/include/uapi/drm/hermes_kms_drm.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#include "src/logging.h"

namespace platf {

  namespace {

    // Ponytail: read /sys/module/hermes_kms to detect the module is loaded.
    // find_hermes_kms_card() below scans /dev/dri/card* for the card node.
    // The card node is what Hermes-KMS ioctls work on — render nodes
    // (renderD*) pass through to the DRM render path which does not forward
    // Hermes-KMS ioctls.
    bool module_loaded() {
      std::error_code ec;
      return std::filesystem::exists("/sys/module/hermes_kms", ec);
    }

    int find_hermes_kms_card() {
      // ponytail: Hermes-KMS ioctls work on the card node, not the
      // render node. Scan /dev/dri/cardN.
      for (int i = 0; i < 16; ++i) {
        std::string path = "/dev/dri/card" + std::to_string(i);
        int fd = open(path.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) continue;
        struct drm_hermes_kms_version ver = {};
        if (ioctl(fd, DRM_IOCTL_HERMES_KMS_GET_VERSION, &ver) == 0) {
          close(fd);
          return i;
        }
        close(fd);
      }
      return -1;
    }

  }  // namespace

  hermes_kms_status_t probe_hermes_kms() {
    hermes_kms_status_t s;
    s.module_loaded = module_loaded();
    if (!s.module_loaded) {
      s.last_error = "hermes_kms kernel module not loaded";
      return s;
    }
    int rn = find_hermes_kms_card();
    if (rn < 0) {
      s.last_error = "no /dev/dri/card* device found";
      return s;
    }
    s.card_index = rn;

    // Open the card node and probe capabilities via the UAPI.
    std::string node_path = "/dev/dri/card" + std::to_string(rn);
    int fd = open(node_path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
      s.last_error = "cannot open " + node_path + ": " + strerror(errno);
      return s;
    }

    // GET_VERSION
    struct drm_hermes_kms_version ver = {};
    if (ioctl(fd, DRM_IOCTL_HERMES_KMS_GET_VERSION, &ver) < 0) {
      s.last_error = "GET_VERSION ioctl failed: " + std::string(strerror(errno));
      close(fd);
      return s;
    }
    s.uapi_version = ver.uapi_version;
    s.driver_version = std::to_string(ver.driver_major) + "."
                     + std::to_string(ver.driver_minor) + "."
                     + std::to_string(ver.driver_patch);

    // Refuse to load an older UAPI than what this build was compiled against.
    // The header contract: "refuses to load if the kernel module reports an
    // older version." Older structs would have different layouts and silent
    // miscompiles.
    if (s.uapi_version < HERMES_KMS_REQUIRED_UAPI) {
      s.last_error = "driver UAPI " + std::to_string(s.uapi_version)
                   + " is older than required " + std::to_string(HERMES_KMS_REQUIRED_UAPI);
      close(fd);
      return s;
    }

    // GET_CAPS
    struct drm_hermes_kms_caps caps = {};
    if (ioctl(fd, DRM_IOCTL_HERMES_KMS_GET_CAPS, &caps) < 0) {
      s.last_error = "GET_CAPS ioctl failed: " + std::string(strerror(errno));
      close(fd);
      return s;
    }
    s.caps.flags           = caps.flags;
    s.caps.min_width       = caps.min_width;
    s.caps.min_height      = caps.min_height;
    s.caps.max_width       = caps.max_width;
    s.caps.max_height      = caps.max_height;
    s.caps.preferred_width = caps.preferred_width;
    s.caps.preferred_height= caps.preferred_height;
    s.caps.max_refresh_hz  = caps.max_refresh_hz;

    close(fd);

    // ponytail: check that the device advertises the minimum set of
    // capabilities we need. A real capture implementation needs
    // DMABUF_EXPORT + FRAME_WAIT + FRAME_ACQUIRE.
    constexpr std::uint64_t NEEDED = HERMES_KMS_CAP_DMABUF_EXPORT
                                   | HERMES_KMS_CAP_FRAME_WAIT
                                   | HERMES_KMS_CAP_FRAME_ACQUIRE;
    if ((s.caps.flags & NEEDED) != NEEDED) {
      s.last_error = "device missing required caps (need DMABUF_EXPORT + FRAME_WAIT + FRAME_ACQUIRE)";
      // ponytail: signal failure by clearing card_index so callers (display_names,
      // verify_hermes_kms) treat the device as absent instead of partially usable.
      s.card_index = -1;
      return s;
    }
    return s;
  }

  std::vector<std::string> hermes_kms_display_names(mem_type_e) {
    auto status = probe_hermes_kms();
    if (!status.module_loaded || status.card_index < 0) return {};
    // Hermes-KMS exports exactly one virtual output named "HERMES-1".
    return {"HERMES-1"};
  }

  /**
   * @brief Build a display_t bound to the Hermes-KMS card node.
   * @details Opens the card node, re-runs probe_hermes_kms() to confirm the
   *          device is present, and constructs a hermes_kms_display_t that
   *          runs the WAIT_FRAME -> ACQUIRE_FRAME capture loop on the fd.
   *          Returns nullptr (with a log line) if the probe fails.
   */
  // ponytail: minimal DMA-BUF-backed image. The data pointer is nullptr
  // because pixel data lives in a GPU buffer imported by the encoder.
  struct hermes_kms_img_t : platf::img_t {
    int dma_buf_fd = -1;
    ~hermes_kms_img_t() override {
      if (dma_buf_fd >= 0) close(dma_buf_fd);
    }
  };

  // ponytail: minimal display_t that captures from a Hermes-KMS card.
  // The capture loop is WAIT_FRAME → ACQUIRE_FRAME → push the DMA-BUF
  // as an hermes_kms_img_t. The encoder consumer imports the fd via
  // VAAPI/CUDA — no CPU readback.
  struct hermes_kms_display_t : platf::display_t {
    int card_fd = -1;
    std::uint32_t width;
    std::uint32_t height;

    hermes_kms_display_t(int fd, std::uint32_t w, std::uint32_t h)
      : card_fd(fd), width(w), height(h) {}
    ~hermes_kms_display_t() override {
      if (card_fd >= 0) {
        close(card_fd);
        card_fd = -1;
      }
    }

    std::shared_ptr<platf::img_t> alloc_img() override {
      return std::make_shared<hermes_kms_img_t>();
    }

    int dummy_img(platf::img_t *img) override {
      img->width = (std::int32_t)width;
      img->height = (std::int32_t)height;
      img->row_pitch = (std::int32_t)(width * 4);
      img->pixel_pitch = 4;
      img->data = nullptr;  // DMA-BUF images have no CPU data
      return 0;
    }

    capture_e capture(const push_captured_image_cb_t &push_captured_image_cb,
                      const pull_free_image_cb_t &pull_free_image_cb,
                      bool * /*cursor*/) override {
      while (true) {
        // WAIT_FRAME — blocks until the compositor pushes a new frame
        struct drm_hermes_kms_wait_frame wait = {};
        wait.timeout_ms = 500;
        if (ioctl(card_fd, DRM_IOCTL_HERMES_KMS_WAIT_FRAME, &wait) < 0) {
          if (errno == EINTR) continue;
          BOOST_LOG(error) << "hermes_kms: WAIT_FRAME failed: " << strerror(errno);
          return capture_e::error;
        }

        // ACQUIRE_FRAME — get the DMA-BUF fd and metadata
        struct drm_hermes_kms_acquire_frame frame = {};
        frame.flags = HERMES_KMS_FRAME_REQUEST_DMABUF;
        if (ioctl(card_fd, DRM_IOCTL_HERMES_KMS_ACQUIRE_FRAME, &frame) < 0) {
          BOOST_LOG(error) << "hermes_kms: ACQUIRE_FRAME failed: " << strerror(errno);
          return capture_e::error;
        }
        // Guard against the driver claiming success without handing back a
        // DMA-BUF fd. Without this, an hk_img with fd=-1 but populated
        // width/height/pitch would be pushed into the encoder, which would
        // then attempt to import -1 and fail in a much harder-to-diagnose
        // place.
        if (frame.dma_buf_fd[0] < 0) {
          BOOST_LOG(error) << "hermes_kms: ACQUIRE_FRAME returned no DMA-BUF fd";
          return capture_e::error;
        }

        // Pull a free image from the pool
        std::shared_ptr<platf::img_t> img;
        if (!pull_free_image_cb(img) || !img) {
          // Driver gave us a DMA-BUF fd but we're not consuming it now —
          // close it ourselves to avoid leaking the descriptor.
          if (frame.dma_buf_fd[0] >= 0) close(frame.dma_buf_fd[0]);
          return capture_e::interrupted;
        }

        auto *hk_img = dynamic_cast<hermes_kms_img_t *>(img.get());
        if (!hk_img) {
          BOOST_LOG(error) << "hermes_kms: image pool returned non-Hermes-KMS image";
          if (frame.dma_buf_fd[0] >= 0) close(frame.dma_buf_fd[0]);
          return capture_e::error;
        }

        // Close any previous DMA-BUF before storing the new one
        if (hk_img->dma_buf_fd >= 0) close(hk_img->dma_buf_fd);
        hk_img->dma_buf_fd = frame.dma_buf_fd[0];  // take ownership
        hk_img->width = (std::int32_t)frame.width;
        hk_img->height = (std::int32_t)frame.height;
        hk_img->row_pitch = (std::int32_t)frame.pitch[0];
        hk_img->pixel_pitch = 4;  // ponytail: DRM_FORMAT_XRGB8888 = 4 bytes per pixel
        hk_img->frame_timestamp = std::chrono::steady_clock::now();

        // Push the image to the consumer (encoder)
        if (!push_captured_image_cb(std::move(img), true)) {
          return capture_e::ok;  // consumer asked us to stop
        }
      }
    }
  };

  std::shared_ptr<display_t> hermes_kms_display(mem_type_e hwdevice_type,
                                                const std::string &display_name,
                                                const video::config_t &config) {
    auto status = probe_hermes_kms();
    if (!status.module_loaded) {
      BOOST_LOG(error) << "hermes_kms: kernel module not loaded";
      return nullptr;
    }
    if (status.card_index < 0) {
      BOOST_LOG(error) << "hermes_kms: no card node found";
      return nullptr;
    }
    std::string node_path = "/dev/dri/card" + std::to_string(status.card_index);
    int fd = open(node_path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
      BOOST_LOG(error) << "hermes_kms: cannot open " << node_path << ": " << strerror(errno);
      return nullptr;
    }
    return std::make_shared<hermes_kms_display_t>(fd,
                                                  status.caps.preferred_width,
                                                  status.caps.preferred_height);
  }


  bool verify_hermes_kms() {
    auto status = probe_hermes_kms();
    return status.module_loaded && status.card_index >= 0;
  }

}  // namespace platf
