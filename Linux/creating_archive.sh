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

# Determine a unique temporary directory name for vimsicles
base_dir="/tmp/vimsicles"
vimsicles_dir="$base_dir"
counter=0

# Loop until a unique directory name is found (e.g., /tmp/vimsicles, /tmp/vimsicles0, /tmp/vimsicles1, ...)
while [ -d "$vimsicles_dir" ]; do
	vimsicles_dir="${base_dir}${counter}"
	counter=$((counter + 1))
done

echo "Using temporary directory: $vimsicles_dir"
mkdir -p "$vimsicles_dir" # Create the unique temporary directory

# Check if any files were provided as arguments
if [ -z "$@" ]; then
	echo "No files provided to archive. Please run the script with files (e.g., ./script.sh file1.txt folder/)."
	rmdir "$vimsicles_dir" # Remove the newly created empty directory
	exit 1
fi

# Copy all provided files and directories into the unique temporary directory
echo "Copying provided files into $vimsicles_dir/"
cp -r "$@" "$vimsicles_dir/" # Use -r for recursive copy for directories

# Get the base name of the temporary directory (e.g., vimsicles, vimsicles0)
archive_base_name=$(basename "$vimsicles_dir")
archive_path="/tmp/${archive_base_name}.tar.gz"

# Create the tar.gz archive. -C /tmp ensures the archive contains the directory directly
echo "Creating archive: $archive_path"
tar -czvf "$archive_path" -C /tmp "$archive_base_name"

# Ensure the /tar directory exists for the final archive
mkdir -p /tar

# Get the MD5 hash for the created archive file
md5sum_hash=$(md5sum "$archive_path" | awk '{print $1}')
final_archive_name="${md5sum_hash}.tar.gz"
final_archive_path="/tar/$final_archive_name"

# Move the temporary archive to its final destination with the MD5 hash as its name
echo "Moving archive to: $final_archive_path"
mv "$archive_path" "$final_archive_path"

echo "Archive successfully created and moved: $final_archive_name"

# Clean up the temporary directory
echo "Cleaning up temporary directory: $vimsicles_dir"
rm -rf "$vimsicles_dir"

echo "Script finished."
