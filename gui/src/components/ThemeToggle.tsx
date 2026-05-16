// Toggles `.dark` on <html> and persists the choice in localStorage.
// The class is applied by an inline bootstrap script in
// `routes/__root.tsx` *before* React hydrates. Since SSR can't know the
// user's stored preference, we treat the DOM as the single source of
// truth: state is subscribed to the live `.dark` class via
// `useSyncExternalStore`, and `toggle` reads from the DOM (not a stale
// React closure) so the first click can never be a no-op.

import { Moon, Sun } from 'lucide-react'
import { useSyncExternalStore } from 'react'
import { Button } from '#/components/ui/button'

type Theme = 'light' | 'dark'

const STORAGE_KEY = 'az-gui-theme'

function readDomTheme(): Theme {
  return document.documentElement.classList.contains('dark') ? 'dark' : 'light'
}

function applyTheme(theme: Theme) {
  const root = document.documentElement
  if (theme === 'dark') root.classList.add('dark')
  else root.classList.remove('dark')
}

function subscribeToHtmlClass(onChange: () => void): () => void {
  const observer = new MutationObserver(onChange)
  observer.observe(document.documentElement, {
    attributes: true,
    attributeFilter: ['class'],
  })
  return () => observer.disconnect()
}

function getServerSnapshot(): Theme {
  return 'light'
}

export function ThemeToggle({ className }: { className?: string }) {
  const theme = useSyncExternalStore(
    subscribeToHtmlClass,
    readDomTheme,
    getServerSnapshot,
  )

  const toggle = () => {
    const next: Theme = readDomTheme() === 'dark' ? 'light' : 'dark'
    applyTheme(next)
    try {
      window.localStorage.setItem(STORAGE_KEY, next)
    } catch {
      // localStorage may be unavailable (private mode, etc.) — the toggle
      // still works for the session.
    }
  }

  return (
    <Button
      variant="ghost"
      size="icon"
      onClick={toggle}
      aria-label={`Switch to ${theme === 'dark' ? 'light' : 'dark'} mode`}
      suppressHydrationWarning
      className={className}
    >
      {theme === 'dark' ? (
        <Sun className="size-4" />
      ) : (
        <Moon className="size-4" />
      )}
    </Button>
  )
}
