/**
 * @file Defines all available commands for the CommandPalette component.
 */

export const NAV_COMMANDS = [
  { id: 'dashboard', label: 'Dashboard', icon: 'LayoutDashboard', route: '/' },
  { id: 'library', label: 'Apps / Library', icon: 'Gamepad2', route: '/apps' },
  { id: 'pairing', label: 'Pairing', icon: 'Link', route: '/pin' },
  { id: 'config', label: 'Configuration', icon: 'Settings', route: '/config' },
  { id: 'troubleshooting', label: 'Troubleshooting', icon: 'Wrench', route: '/troubleshooting' },
  { id: 'password', label: 'Change Password', icon: 'Key', route: '/password' },
];

export const SETTING_COMMANDS = [
  { id: 'config-general', label: 'Settings: General', parent: 'config', section: 'general' },
  { id: 'config-av', label: 'Settings: Audio/Video', parent: 'config', section: 'av' },
  { id: 'config-nv', label: 'Settings: NVENC Encoder', parent: 'config', section: 'nv' },
  { id: 'config-input', label: 'Settings: Input', parent: 'config', section: 'input' },
  { id: 'config-network', label: 'Settings: Network', parent: 'config', section: 'network' },
];

export const HOST_COMMANDS = [
  { id: 'host-restart', label: 'Restart', icon: 'RotateCcw', action: 'restart' },
  { id: 'host-quit', label: 'Quit', icon: 'LogOut', action: 'quit' },
];

/** @returns {Array} All commands combined into a single flat list. */
export function allCommands() {
  return [...NAV_COMMANDS, ...SETTING_COMMANDS, ...HOST_COMMANDS];
}
