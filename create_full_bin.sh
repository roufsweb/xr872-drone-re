#!/bin/bash
IMAGE_DIR="sdk/project/custom/drone_camera/image/xr872"
OUTPUT="full_flash_512k.bin"

# Create a blank 512KB file
dd if=/dev/zero of=$OUTPUT bs=1k count=512

# Write components at specified offsets
dd if=$IMAGE_DIR/boot_26M.bin     of=$OUTPUT bs=1k seek=0   conv=notrunc
dd if=$IMAGE_DIR/app.bin          of=$OUTPUT bs=1k seek=16  conv=notrunc
dd if=$IMAGE_DIR/app_xip.bin      of=$OUTPUT bs=1k seek=64  conv=notrunc
dd if=$IMAGE_DIR/app_psram.bin    of=$OUTPUT bs=1k seek=400 conv=notrunc
dd if=$IMAGE_DIR/wlan_bl.bin      of=$OUTPUT bs=1k seek=416 conv=notrunc
dd if=$IMAGE_DIR/wlan_fw.bin      of=$OUTPUT bs=1k seek=420 conv=notrunc
dd if=$IMAGE_DIR/wlan_sdd_26M.bin of=$OUTPUT bs=1k seek=460 conv=notrunc

echo "Created $OUTPUT for direct CH341A flashing."
