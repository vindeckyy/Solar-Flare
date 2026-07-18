<template>
  <teleport to="body">
    <div v-if="visible" class="command-palette-backdrop" @click.self="close">
      <div class="command-palette" @click.stop>
        <div class="command-palette-input-wrapper">
          <Search :size="18" class="command-palette-search-icon" />
          <input
            ref="searchInput"
            type="text"
            class="command-palette-input"
            v-model="query"
            placeholder="Type a command..."
            @keydown="onKeydown"
          />
          <kbd class="command-palette-kbd-hint">Esc</kbd>
        </div>
        <div class="command-palette-results" ref="resultsContainer">
          <div v-if="filteredNav.length" class="command-palette-group">
            <div class="command-palette-group-label">Navigation</div>
            <div
              v-for="(item, index) in filteredNav"
              :key="item.id"
              class="command-palette-item"
              :class="{ 'command-palette-item-selected': selectedIndex === getGlobalIndex(index, 'nav') }"
              @click="execute(item)"
              @mouseenter="selectedIndex = getGlobalIndex(index, 'nav')"
            >
              <component :is="item.icon" :size="18" class="command-palette-item-icon" />
              <span v-html="highlightLabel(item.label)"></span>
            </div>
          </div>
          <div v-if="filteredSettings.length" class="command-palette-group">
            <div class="command-palette-group-label">Settings</div>
            <div
              v-for="(item, index) in filteredSettings"
              :key="item.id"
              class="command-palette-item"
              :class="{ 'command-palette-item-selected': selectedIndex === getGlobalIndex(index, 'settings') }"
              @click="execute(item)"
              @mouseenter="selectedIndex = getGlobalIndex(index, 'settings')"
            >
              <Settings :size="18" class="command-palette-item-icon" />
              <span v-html="highlightLabel(item.label)"></span>
            </div>
          </div>
          <div v-if="filteredHost.length" class="command-palette-group">
            <div class="command-palette-group-label">Host Controls</div>
            <div
              v-for="(item, index) in filteredHost"
              :key="item.id"
              class="command-palette-item"
              :class="{ 'command-palette-item-selected': selectedIndex === getGlobalIndex(index, 'host') }"
              @click="execute(item)"
              @mouseenter="selectedIndex = getGlobalIndex(index, 'host')"
            >
              <component :is="item.icon" :size="18" class="command-palette-item-icon" />
              <span v-html="highlightLabel(item.label)"></span>
            </div>
          </div>
          <div v-if="!totalResults" class="command-palette-empty">
            No results found
          </div>
        </div>
        <div class="command-palette-footer">
          <span class="command-palette-footer-hint">
            <kbd>&uarr;</kbd><kbd>&darr;</kbd> to navigate
          </span>
          <span class="command-palette-footer-hint">
            <kbd>&crarr;</kbd> to select
          </span>
          <span class="command-palette-footer-hint">
            <kbd>Esc</kbd> to close
          </span>
        </div>
      </div>
    </div>
  </teleport>
</template>

<script>
import {
  Search,
  LayoutDashboard,
  Gamepad2,
  Link,
  Settings,
  Wrench,
  Key,
  RotateCcw,
  LogOut,
} from '@lucide/vue'
import { NAV_COMMANDS, SETTING_COMMANDS, HOST_COMMANDS } from './command-actions'

export default {
  components: {
    Search,
    LayoutDashboard,
    Gamepad2,
    Link,
    Settings,
    Wrench,
    Key,
    RotateCcw,
    LogOut,
  },
  data() {
    return {
      visible: false,
      query: '',
      selectedIndex: 0,
    }
  },
  computed: {
    filteredNav() {
      return this.filterCommands(NAV_COMMANDS)
    },
    filteredSettings() {
      return this.filterCommands(SETTING_COMMANDS)
    },
    filteredHost() {
      return this.filterCommands(HOST_COMMANDS)
    },
    allResults() {
      return [...this.filteredNav, ...this.filteredSettings, ...this.filteredHost]
    },
    totalResults() {
      return this.allResults.length
    },
  },
  mounted() {
    document.addEventListener('keydown', this.onGlobalKeydown)
  },
  beforeUnmount() {
    document.removeEventListener('keydown', this.onGlobalKeydown)
  },
  methods: {
    /**
     * @brief Handle global keydown to toggle the palette with Ctrl+K or Cmd+K.
     */
    onGlobalKeydown(e) {
      if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
        e.preventDefault()
        if (this.visible) {
          this.close()
        } else {
          this.open()
        }
      }
    },
    /**
     * @brief Open the command palette.
     */
    open() {
      this.visible = true
      this.query = ''
      this.selectedIndex = 0
      this.$nextTick(() => {
        if (this.$refs.searchInput) {
          this.$refs.searchInput.focus()
        }
      })
    },
    /**
     * @brief Close the command palette.
     */
    close() {
      this.visible = false
      this.query = ''
      this.selectedIndex = 0
    },
    /**
     * @brief Filter commands by the current search query.
     * @param {Array} commands The list of command objects.
     * @returns {Array} Filtered commands.
     */
    filterCommands(commands) {
      if (!this.query.trim()) return commands
      const q = this.query.toLowerCase()
      return commands.filter((cmd) => cmd.label.toLowerCase().includes(q))
    },
    /**
     * @brief Compute the global index for keyboard navigation.
     * @param {number} localIndex Index within the specific group.
     * @param {string} group Group name ('nav', 'settings', 'host').
     * @returns {number} Global index across all results.
     */
    getGlobalIndex(localIndex, group) {
      if (group === 'nav') return localIndex
      if (group === 'settings') return this.filteredNav.length + localIndex
      return this.filteredNav.length + this.filteredSettings.length + localIndex
    },
    /**
     * @brief Highlight matching text in the label.
     * @param {string} label The command label.
     * @returns {string} HTML string with highlighted matches.
     */
    highlightLabel(label) {
      if (!this.query.trim()) return this.escapeHtml(label)
      const escapedQuery = this.escapeRegex(this.query)
      const regex = new RegExp(`(${escapedQuery})`, 'gi')
      return this.escapeHtml(label).replace(regex, '<mark class="command-palette-highlight">$1</mark>')
    },
    /**
     * @brief Escape HTML entities in a string.
     * @param {string} str The string to escape.
     * @returns {string} Escaped string.
     */
    escapeHtml(str) {
      return str
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
    },
    /**
     * @brief Escape regex special characters in a string.
     * @param {string} str The string to escape.
     * @returns {string} Regex-escaped string.
     */
    escapeRegex(str) {
      return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
    },
    /**
     * @brief Handle keyboard navigation within the palette.
     * @param {KeyboardEvent} e The keydown event.
     */
    onKeydown(e) {
      if (e.key === 'ArrowDown') {
        e.preventDefault()
        if (this.selectedIndex < this.totalResults - 1) {
          this.selectedIndex++
        }
        this.scrollToSelected()
      } else if (e.key === 'ArrowUp') {
        e.preventDefault()
        if (this.selectedIndex > 0) {
          this.selectedIndex--
        }
        this.scrollToSelected()
      } else if (e.key === 'Enter') {
        e.preventDefault()
        if (this.allResults[this.selectedIndex]) {
          this.execute(this.allResults[this.selectedIndex])
        }
      } else if (e.key === 'Escape') {
        e.preventDefault()
        this.close()
      }
    },
    /**
     * @brief Scroll the results container to keep the selected item visible.
     */
    scrollToSelected() {
      this.$nextTick(() => {
        const container = this.$refs.resultsContainer
        if (!container) return
        const selected = container.querySelector('.command-palette-item-selected')
        if (selected) {
          selected.scrollIntoView({ block: 'nearest' })
        }
      })
    },
    /**
     * @brief Execute a command - navigate or perform an action.
     * @param {Object} command The command object to execute.
     */
    execute(command) {
      if (command.route) {
        globalThis.location.href = command.route
      } else if (command.parent === 'config') {
        globalThis.location.href = '/config#' + command.section
      } else if (command.action === 'restart') {
        if (confirm('Are you sure you want to restart SolarFlare?')) {
          fetch('./api/restart', { method: 'POST' }).catch(() => {})
          this.close()
        }
      } else if (command.action === 'quit') {
        if (confirm('Are you sure you want to quit SolarFlare?')) {
          fetch('./api/quit', { method: 'POST' }).catch(() => {})
          this.close()
        }
      }
      this.close()
    },
  },
}
</script>

<style scoped>
.command-palette-backdrop {
  position: fixed;
  inset: 0;
  z-index: 9999;
  display: flex;
  justify-content: center;
  padding-top: 12vh;
  background-color: rgba(0, 0, 0, 0.6);
  backdrop-filter: blur(4px);
  -webkit-backdrop-filter: blur(4px);
}

.command-palette {
  width: 100%;
  max-width: 560px;
  max-height: 60vh;
  display: flex;
  flex-direction: column;
  background-color: var(--color-surface, #1e1e2e);
  border: 1px solid var(--color-border, #313244);
  border-radius: 12px;
  box-shadow: 0 16px 70px rgba(0, 0, 0, 0.5);
  overflow: hidden;
  margin: 0 16px;
}

.command-palette-input-wrapper {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 14px 16px;
  border-bottom: 1px solid var(--color-border, #313244);
}

.command-palette-search-icon {
  flex-shrink: 0;
  color: var(--color-text-subtle, #6c7086);
}

.command-palette-input {
  flex: 1;
  border: none;
  outline: none;
  background: transparent;
  color: var(--color-text-base, #cdd6f4);
  font-size: 1rem;
  font-family: inherit;
}

.command-palette-input::placeholder {
  color: var(--color-text-subtle, #6c7086);
}

.command-palette-kbd-hint {
  flex-shrink: 0;
  font-size: 0.7rem;
  padding: 2px 6px;
  border-radius: 4px;
  background-color: var(--color-bg-muted, #45475a);
  color: var(--color-text-muted, #a6adc8);
  font-family: inherit;
  border: 1px solid var(--color-border, #585b70);
  line-height: 1.4;
}

.command-palette-results {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.command-palette-group {
  margin-bottom: 4px;
}

.command-palette-group-label {
  padding: 6px 12px 4px;
  font-size: 0.7rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: var(--color-text-subtle, #6c7086);
}

.command-palette-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 12px;
  border-radius: 6px;
  cursor: pointer;
  color: var(--color-text-base, #cdd6f4);
  font-size: 0.9rem;
  transition: background-color 0.1s ease;
}

.command-palette-item:hover,
.command-palette-item-selected {
  background-color: var(--color-bg-muted, #45475a);
}

.command-palette-item-icon {
  flex-shrink: 0;
  color: var(--color-text-muted, #a6adc8);
}

.command-palette-highlight {
  background-color: rgba(245, 158, 11, 0.3);
  color: var(--color-text-base, #cdd6f4);
  border-radius: 2px;
  padding: 0 2px;
}

.command-palette-empty {
  padding: 24px 16px;
  text-align: center;
  color: var(--color-text-subtle, #6c7086);
  font-size: 0.9rem;
}

.command-palette-footer {
  display: flex;
  gap: 16px;
  padding: 8px 16px;
  border-top: 1px solid var(--color-border, #313244);
  font-size: 0.75rem;
  color: var(--color-text-subtle, #6c7086);
}

.command-palette-footer-hint {
  display: flex;
  align-items: center;
  gap: 4px;
}

.command-palette-footer kbd {
  font-size: 0.65rem;
  padding: 1px 5px;
  border-radius: 3px;
  background-color: var(--color-bg-muted, #45475a);
  color: var(--color-text-muted, #a6adc8);
  font-family: inherit;
  border: 1px solid var(--color-border, #585b70);
  line-height: 1.5;
}
</style>
