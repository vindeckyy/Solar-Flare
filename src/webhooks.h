// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file src/webhooks.h
 * @brief Outbound HTTP webhook notifications for stream lifecycle events.
 *
 * When a streaming session starts or ends, SolarFlare POSTs a JSON payload
 * to every configured webhook URL (fire-and-forget, short timeout). This
 * lets users automate around streams: turn on lights, notify Discord, sync
 * save files, etc. URLs and an optional HMAC secret are configured via the
 * @c webhook_url_* and @c webhook_secret config keys.
 */
#pragma once

// standard includes
#include <string>

// local includes
#include "session_history.h"

namespace sunshine::webhooks {

  /**
   * @brief Notify all configured webhook URLs of a session lifecycle event.
   *
   * Serializes @p event with @p record as a JSON body and POSTs it to every
   * URL from config::nvhttp.webhook_urls (fire-and-forget; failures are
   * logged, never fatal). If config::nvhttp.webhook_secret is non-empty, the
   * body is signed with HMAC-SHA256 in the @c X-Solarflare-Signature header.
   *
   * @param event Event name: "stream.start" or "stream.end".
   * @param record The session record describing the event.
   */
  void notify(const std::string &event, const session_history::record_t &record);

  /**
   * @brief Return whether any webhook URLs are configured.
   *
   * @return True when config::nvhttp.webhook_urls is non-empty.
   */
  bool enabled();

}  // namespace sunshine::webhooks
