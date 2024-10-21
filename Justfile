[private]
default:
    just --list --justfile {{ justfile() }}

format:
    indent src/*.c src/*.h -linux -nut -i4

translate:
    meson compile -C ./.meson-builder com.konstantintutsch.Lock-pot
    meson compile -C ./.meson-builder com.konstantintutsch.Lock-update-po

local:
    flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    flatpak run org.flatpak.Builder --user --sandbox \
        --force-clean --ccache --install-deps-from=flathub \
        --repo=repo builddir \
        com.konstantintutsch.Lock.Devel.yaml
    flatpak build-bundle repo \
        bundle.flatpak \
        com.konstantintutsch.Lock.Devel
    flatpak install --user --reinstall --assumeyes --bundle \
        --include-sdk --include-debug \
        bundle.flatpak
    GTK_DEBUG=interactive flatpak run \
        com.konstantintutsch.Lock.Devel

debug:
    flatpak-coredumpctl \
        -m $(coredumpctl list -1 --no-pager --no-legend | grep -oE 'CEST ([0-9]+)' | awk '{print $2}') \
        com.konstantintutsch.Lock.Devel

setup:
    sudo dnf install -y indent
    sudo dnf install -y meson
    sudo dnf install -y libadwaita-devel
    sudo dnf install -y gpgme-devel
    flatpak install --user --assumeyes org.gnome.Platform//47
    flatpak install --user --assumeyes org.gnome.Sdk//47
    flatpak install --user --assumeyes org.flatpak.Builder
