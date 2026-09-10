'use client'

import { useState } from 'react'
import { Check, Copy } from 'lucide-react'

/**
 * @brief Fenced code block with a language label and copy-to-clipboard button.
 */
export function DocsCodeBlock({ code, language }: { code: string; language: string }) {
  const [copied, setCopied] = useState(false)

  const handleCopy = () => {
    navigator.clipboard.writeText(code)
    setCopied(true)
    setTimeout(() => setCopied(false), 2000)
  }

  return (
    <div className="relative group my-2 rounded-xl border border-border bg-[#0d0c0a] overflow-hidden shadow-lg shadow-black/20">
      <div className="flex items-center justify-between px-4 py-2 border-b border-border/40 bg-muted/20 text-xs font-mono text-muted-foreground">
        <span>{language}</span>
        <button
          onClick={handleCopy}
          className="flex items-center gap-1 px-2 py-1 rounded bg-muted/40 hover:bg-muted text-foreground transition-colors"
          aria-label="Copy code"
        >
          {copied ? (
            <>
              <Check className="h-3.5 w-3.5 text-green-500" />
              <span className="text-green-500 text-[11px]">Copied</span>
            </>
          ) : (
            <>
              <Copy className="h-3.5 w-3.5" />
              <span className="text-[11px]">Copy</span>
            </>
          )}
        </button>
      </div>
      <pre className="p-4 overflow-x-auto text-xs sm:text-sm font-mono text-[#f3ede2] leading-relaxed">
        <code>{code}</code>
      </pre>
    </div>
  )
}
