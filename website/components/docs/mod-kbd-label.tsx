'use client'

import { useEffect, useState } from 'react'

/**
 * @brief Platform-aware search shortcut label: ⌘K on Apple devices, Ctrl K elsewhere.
 */
export function ModKbdLabel() {
  const [label, setLabel] = useState('⌘K')

  useEffect(() => {
    if (!/Mac|iPhone|iPad|iPod/.test(navigator.platform)) {
      setLabel('Ctrl K')
    }
  }, [])

  return <>{label}</>
}
