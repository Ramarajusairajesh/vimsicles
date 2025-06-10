#!/bin/bash

# Determine the base distro family and the package manager
if [ -f /etc/os-release ]; then
	. /etc/os-release
	id_like=$(echo "$ID_LIKE" | tr '[:upper:]' '[:lower:]')
	id=$(echo "$ID" | tr '[:upper:]' '[:lower:]')

	if [[ $id == "arch" || $id_like == *"arch"* ]]; then
		echo "This system is based on Arch Linux."
		pkg_manager="pacman"
		install_cmd="sudo pacman -Sy --noconfirm"
	elif [[ $id == "debian" || $id_like == *"debian"* || $id_like == *"ubuntu"* ]]; then
		echo "This system is based on Debian (including Ubuntu)."
		pkg_manager="apt"
		install_cmd="sudo apt update && sudo apt install -y"
	elif [[ $id == "fedora" || $id_like == *"fedora"* || $id_like == *"rhel"* || $id_like == *"centos"* ]]; then
		echo "This system is based on Fedora/Red Hat."
		pkg_manager="dnf"
		install_cmd="sudo dnf install -y"
	elif [[ $id == "nixos" ]]; then
		echo "This system is NixOS-based."
		pkg_manager="nix-env"
		install_cmd="nix-env -iA nixpkgs"
	else
		echo "Unknown or unrecognized distro family: $NAME"
		exit 1
	fi
else
	echo "Could not detect the distribution family."
	exit 1
fi

echo
echo "Checking if the following packages are installed: tar, gunzip, g++, coreutils"

# g++ for compiling code, coreutils for md5sum
for pkg in tar gunzip g++ md5sum; do
	if command -v "$pkg" >/dev/null 2>&1; then
		echo "$pkg is installed."
	else
		echo "$pkg is NOT installed. Installing..."
		# General installation command for all packages
		$install_cmd "$pkg"

		# Verify installation
		if command -v "$pkg" >/dev/null 2>&1; then
			echo "$pkg successfully installed!"
		else
			echo "Failed to install $pkg. Exiting."
			exit 1 # Exit if a critical package fails to install
		fi
	fi
done

path=${HOME}/Downloads/vimsicles
mkdir -p path

tar -zvxcf $1 -C /Downloads/vimiscle
