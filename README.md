# NekoCurX

Convert Windows cursor themes (`.ani` / `.cur`) into Linux XCursor themes.

NekoCurX is a command-line utility designed to simplify cursor theme migration between Windows and Linux. It automatically detects cursor roles, converts supported formats, generates Linux-compatible themes, and preserves compatibility aliases used across modern desktop environments.

<p align="center">
  <img src=".meta/assets/logo.png" width="70%" alt="NekoCurX">
</p>

<p align="center">
  <a href="https://github.com/AikoAii/NekoCurX/stargazers">
    <img src="https://img.shields.io/github/stars/AikoAii/NekoCurX?style=for-the-badge&logo=github" alt="Stars" />
  </a>

  <a href="https://github.com/AikoAii/NekoCurX/releases">
    <img src="https://img.shields.io/github/v/release/AikoAii/NekoCurX?style=for-the-badge&logo=github" alt="Release" />
  </a>

  <a href="https://github.com/AikoAii/NekoCurX/commits">
    <img src="https://img.shields.io/github/last-commit/AikoAii/NekoCurX?style=for-the-badge&logo=git&label=Last%20Commit" alt="Last Commit" />
  </a>

  <a href="./LICENSE">
    <img src="https://img.shields.io/github/license/AikoAii/NekoCurX?style=for-the-badge" alt="License" />
  </a>
</p>

<p align="center">
  <a href="#features">Features</a> •
  <a href="#installation">Installation</a> •
  <a href="#usage">Usage</a> •
  <a href="#output">Output</a>
</p>

---

## Features

* ANI (`.ani`) cursor conversion
* CUR (`.cur`) cursor conversion
* Automatic cursor mapping
* `install.inf` support
* Linux XCursor generation
* Compatibility alias generation
* Theme metadata generation
* One-command installation
* Batch theme conversion

---

## Installation

### Download Release

Download the latest release and extract it:

```bash
tar -xf NekoCurX-v0.1.0-linux-x86_64.tar.gz
```

Run:

```bash
./nkx --help
```

### Build From Source

```bash
git clone https://github.com/AikoAii/NekoCurX.git

cd NekoCurX

cmake -B build
cmake --build build
```

Run:

```bash
./build/src/nkx --help
```

---

## Usage

Scan a theme:

```bash
nkx scan ThemeFolder/
```

Inspect a cursor:

```bash
nkx inspect Busy.ani
```

Convert a theme:

```bash
nkx convert ThemeFolder/
```

Convert with custom metadata:

```bash
nkx convert ThemeFolder/ \
  --theme "My Cursor Theme" \
  --comment "Custom Linux cursor theme" \
  --author "Aiko"
```

Install directly:

```bash
nkx install ThemeFolder/
```

---

## Output

```text
ThemeName-Linux/

├── cursors/
├── extras/
├── metadata.json
└── index.theme
```

Generated themes can be installed manually or directly through the `install` command.

---

## Support

If you find this project useful and would like to support its development:

<p align="left">
  <a href="https://trakteer.id/aikoaii" target="_blank">
    <img src="https://cdn.trakteer.id/images/embed/trbtn-red-1.png" height="40" alt="Trakteer"/>
  </a>

  <a href="https://ko-fi.com/aikoai" target="_blank">
    <img src="https://ko-fi.com/img/githubbutton_sm.svg" height="40" alt="Ko-fi"/>
  </a>
</p>

---

## License

MIT License. See `LICENSE` for details.
