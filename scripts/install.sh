#!/bin/bash

# --- Configuration ---
INSTALL_DIR="$HOME/.jack_toolchain"
SOURCE_DIR="$(cd "$(dirname "$0")" && pwd)"
OS="$(uname -s)"
COMMAND_NAME="jack"

echo "🔧 Installing Jack Compiler..."

# 1. Detect OS and Pick Binary Name
if [ "$OS" = "Darwin" ]; then
    BINARY_NAME="NAND2TETRIS_mac"
    echo "   -> Detected macOS"
elif [ "$OS" = "Linux" ]; then
    BINARY_NAME="NAND2TETRIS_linux"
    echo "   -> Detected Linux"
else
    echo "❌ Error: Unsupported OS: $OS"
    exit 1
fi

# 2. Check for Binary
if [ ! -f "$SOURCE_DIR/bin/$BINARY_NAME" ]; then
    echo "❌ Error: Could not find '$BINARY_NAME' in bin/ folder."
    exit 1
fi

# 3. Setup Install Directory
rm -rf "$INSTALL_DIR" 2>/dev/null
mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/tools"
mkdir -p "$INSTALL_DIR/os"

# 4. Copy Files (Keeping original filenames)
cp "$SOURCE_DIR/bin/$BINARY_NAME" "$INSTALL_DIR/bin/"
chmod +x "$INSTALL_DIR/bin/$BINARY_NAME"

cp -r "$SOURCE_DIR/tools/"* "$INSTALL_DIR/tools/"
if [ -d "$SOURCE_DIR/os" ]; then
    cp "$SOURCE_DIR/os/"*.jack "$INSTALL_DIR/os/"
fi

# 5. Create Wrapper Script
# This bridges 'jack' command -> 'NAND2TETRIS_linux' binary
cat <<EOF > "$INSTALL_DIR/wrapper.sh"
#!/bin/bash
"$INSTALL_DIR/bin/$BINARY_NAME" "\$@"
EOF
chmod +x "$INSTALL_DIR/wrapper.sh"

# --- NEW: Check Python ---
echo "🐍 Checking for Python (required for --viz tools)..."
if command -v python3 >/dev/null 2>&1; then
    echo "   -> Python 3 detected."
    echo "   -> Installing dependencies (textual, pywebview)..."

    # Try standard install, then try with 'bypass' flags for macOS/Modern Linux
    if ! pip3 install -r "$INSTALL_DIR/tools/requirements.txt" > /dev/null 2>&1; then
        if ! pip3 install -r "$INSTALL_DIR/tools/requirements.txt" --break-system-packages --user > /dev/null 2>&1; then
             echo "   ⚠️  Could not auto-install libraries. Run this later:"
             echo "       pip3 install -r $INSTALL_DIR/tools/requirements.txt"
        else
             echo "   ✅ Libraries installed (User-mode)."
        fi
    else
        echo "   ✅ Libraries installed."
    fi
else
    echo "   ⚠️  WARNING: Python 3 is not installed."
    echo "       The compiler will work, but visualization tools (--viz) will fail."
    echo "       👉 Install it here: https://www.python.org/downloads/"
fi

# 6. Create Global Link (Will ask for password)
echo "🔗 Creating global 'jack' command..."
if [ -L "/usr/local/bin/$COMMAND_NAME" ]; then
    sudo rm "/usr/local/bin/$COMMAND_NAME"
fi

if sudo ln -s "$INSTALL_DIR/wrapper.sh" "/usr/local/bin/$COMMAND_NAME" 2>/dev/null; then
    echo ""
    echo "🎉 SUCCESS! You can now run '$COMMAND_NAME' from anywhere."
else
    echo "⚠️  (Sudo skipped/failed)"
    echo "✅ Install complete. Please add '$INSTALL_DIR/bin' to your PATH manually."
fi