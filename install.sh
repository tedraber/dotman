#!/usr/bin/env bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Dotman-cli Installer ===${NC}\n"

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)

# Install dependencies
echo -e "${YELLOW}Installing dependencies...${NC}"

if [[ "$OS" == "linux" ]]; then
    # Detect package manager
    if command -v apt &> /dev/null; then
        echo "Detected apt package manager"
        sudo apt update
        sudo apt install -y libyaml-cpp-dev build-essential git
    elif command -v dnf &> /dev/null; then
        echo "Detected dnf package manager"
        sudo dnf install -y yaml-cpp-devel gcc-c++ git
    elif command -v pacman &> /dev/null; then
        echo "Detected pacman package manager"
        sudo pacman -S --noconfirm yaml-cpp base-devel git
    else
        echo -e "${RED}Could not detect package manager. Please install libyaml-cpp-dev manually.${NC}"
        exit 1
    fi
elif [[ "$OS" == "macos" ]]; then
    if ! command -v brew &> /dev/null; then
        echo -e "${RED}Homebrew not found. Please install Homebrew first: https://brew.sh${NC}"
        exit 1
    fi
    brew install yaml-cpp
else
    echo -e "${RED}Unsupported OS${NC}"
    exit 1
fi

# Install xmake if not present
if ! command -v xmake &> /dev/null; then
    echo -e "${YELLOW}Installing xmake...${NC}"
    curl -fsSL https://xmake.io/shget.text | bash
    source ~/.xmake/profile
fi

# Build the project
echo -e "${YELLOW}Building dotman-cli...${NC}"
xmake config --require=y
xmake

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Create bin directory if it doesn't exist
mkdir -p ~/.local/bin

# Copy binary
echo -e "${YELLOW}Installing binary...${NC}"
cp build/linux/x86_64/release/dotman-cli ~/.local/bin/dotman-cli 2>/dev/null || \
cp build/macosx/x86_64/release/dotman-cli ~/.local/bin/dotman-cli 2>/dev/null || \
cp build/macosx/arm64/release/dotman-cli ~/.local/bin/dotman-cli 2>/dev/null

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to copy binary. Please copy manually from build/ directory${NC}"
    exit 1
fi

chmod +x ~/.local/bin/dotman-cli

# Detect shell and add to PATH
SHELL_RC=""
SHELL_NAME=""

if [[ "$SHELL" == *"zsh"* ]]; then
    SHELL_RC="$HOME/.zshrc"
    SHELL_NAME="zsh"
elif [[ "$SHELL" == *"bash"* ]]; then
    # Check if .bashrc exists, otherwise use .bash_profile
    if [ -f "$HOME/.bashrc" ]; then
        SHELL_RC="$HOME/.bashrc"
    else
        SHELL_RC="$HOME/.bash_profile"
    fi
    SHELL_NAME="bash"
else
    echo -e "${YELLOW}Could not detect shell. Please add ~/.local/bin to your PATH manually.${NC}"
    SHELL_RC=""
fi

# Add to PATH if not already there
if [ -n "$SHELL_RC" ]; then
    if ! grep -q 'export PATH="$HOME/.local/bin:$PATH"' "$SHELL_RC"; then
        echo -e "${YELLOW}Adding ~/.local/bin to PATH in $SHELL_RC${NC}"
        echo '' >> "$SHELL_RC"
        echo '# Added by dotman-cli installer' >> "$SHELL_RC"
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$SHELL_RC"
        
        echo -e "${GREEN}✓ Added to PATH${NC}"
        echo -e "${YELLOW}Please run: source $SHELL_RC${NC}"
        echo -e "${YELLOW}Or restart your terminal${NC}"
    else
        echo -e "${GREEN}✓ PATH already configured${NC}"
    fi
fi

echo -e "\n${GREEN}=== Installation Complete! ===${NC}"
echo -e "Run ${YELLOW}dotman-cli${NC} to get started"
echo -e "\nIf 'dotman-cli' command is not found, run:"
echo -e "  ${YELLOW}source $SHELL_RC${NC}"
echo -e "or restart your terminal\n"
