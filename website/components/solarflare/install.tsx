import { Terminal, PackageOpen, CheckCircle2, TriangleAlert } from 'lucide-react'
import { CodeBlock } from './code-block'

export function Install() {
  return (
    <section id="install" className="relative border-t border-border py-20 md:py-28">
      <div className="mx-auto max-w-6xl px-4 md:px-6">
        <div className="max-w-2xl">
          <p className="font-mono text-xs uppercase tracking-wider text-primary">
            Install
          </p>
          <h2 className="mt-3 text-balance text-3xl font-semibold tracking-tight text-foreground md:text-4xl">
            Build from source for new installs
          </h2>
          <p className="mt-4 text-pretty leading-relaxed text-muted-foreground">
            New users should always run{' '}
            <span className="font-mono text-primary">./scripts/linux-install.sh</span>.
            The installer detects Arch/CachyOS, Debian/Ubuntu, Fedora-family,
            openSUSE, Bazzite, and NixOS. Then open{' '}
            <span className="font-mono text-primary">https://localhost:47990</span>,
            accept the host-local certificate, and pair a Moonlight client.
          </p>
        </div>

        <div className="mt-8 flex items-start gap-3 rounded-xl border border-destructive/40 bg-destructive/10 p-5">
          <TriangleAlert className="mt-0.5 h-5 w-5 shrink-0 text-destructive" aria-hidden="true" />
          <p className="text-sm leading-relaxed text-foreground">
            Release binaries are only for updating an already working SolarFlare
            install. Prefer Update now in the Web UI (tarball) when available.
            The bare{' '}
            <span className="font-mono text-foreground">sunshine-x86_64</span>{' '}
            download is the executable only and does not install the Web UI,
            desktop files, shaders, udev rules, or the systemd user unit.
          </p>
        </div>

        <div className="mt-12 grid gap-6 lg:grid-cols-2">
          <div className="min-w-0">
            <div className="mb-3 flex items-center gap-2 text-sm font-medium text-foreground">
              <Terminal className="h-4 w-4 text-primary" aria-hidden="true" />
              Fresh source installation
            </div>
            <CodeBlock
              lines={[
                'git clone --recursive https://github.com/vindeckyy/Solar-Flare.git',
                'cd Solar-Flare',
                './scripts/linux-install.sh',
                'systemctl --user enable --now app-dev.lizardbyte.app.Sunshine.service',
              ]}
            />
          </div>

          <div className="min-w-0">
            <div className="mb-3 flex items-center gap-2 text-sm font-medium text-foreground">
              <PackageOpen className="h-4 w-4 text-primary" aria-hidden="true" />
              Existing-install binary fallback
            </div>
            <CodeBlock
              lines={[
                'systemctl --user stop app-dev.lizardbyte.app.Sunshine.service',
                'sudo curl --fail --location --output /usr/local/bin/sunshine \\',
                '  https://github.com/vindeckyy/Solar-Flare/releases/latest/download/sunshine-x86_64',
                'sudo chmod 0755 /usr/local/bin/sunshine',
                "sudo setcap 'cap_sys_admin,cap_sys_nice+p' /usr/local/bin/sunshine",
                'systemctl --user start app-dev.lizardbyte.app.Sunshine.service',
              ]}
            />
          </div>
        </div>

        <div className="mt-6 flex items-start gap-3 rounded-xl border border-border bg-card/60 p-5">
          <CheckCircle2 className="mt-0.5 h-5 w-5 shrink-0 text-primary" aria-hidden="true" />
          <p className="text-sm leading-relaxed text-muted-foreground">
            SolarFlare keeps the executable name, service identifier, ports,
            state format, and{' '}
            <span className="font-mono text-foreground">~/.config/sunshine</span>{' '}
            directory used by Sunshine, so existing Moonlight pairings keep
            working after the switch.
          </p>
        </div>
      </div>
    </section>
  )
}
