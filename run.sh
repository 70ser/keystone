load mmc 0:3 ${kernel_addr_r} /boot/Image
load mmc 0:3 ${fdt_addr_r} /boot/dtbs/jh7110-milkv-mars-cm-emmc.dtb
booti ${kernel_addr_r} - ${fdt_addr_r}

mount -o remount,rw /

load mmc 0:3 ${fdt_addr_r} /boot/usb.dtb

dd if=/dev/zero of=/root/a.bin bs=1M count=100
mount -t configfs none /sys/kernel/config
mkdir /sys/kernel/config/usb_gadget/g1
echo "0x1d6b" > /sys/kernel/config/usb_gadget/g1/idVendor
echo "0x0001" > /sys/kernel/config/usb_gadget/g1/idProduct
mkdir /sys/kernel/config/usb_gadget/g1/strings/0x409
mkdir /sys/kernel/config/usb_gadget/g1/functions/mass_storage.usb0
echo /root/a.bin > /sys/kernel/config/usb_gadget/g1/functions/mass_storage.usb0/lun.0/file
mkdir /sys/kernel/config/usb_gadget/g1/configs/c.1
echo 0xc0 > /sys/kernel/config/usb_gadget/g1/configs/c.1/bmAttributes
echo 500 > /sys/kernel/config/usb_gadget/g1/configs/c.1/MaxPower
mkdir /sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409
ln -s /sys/kernel/config/usb_gadget/g1/functions/mass_storage.usb0/ /sys/kernel/config/usb_gadget/g1/configs/c.1/
ls /sys/class/udc/ | xargs echo > /sys/kernel/config/usb_gadget/g1/UDC



bootargs="console=ttyS0,115200 debug rootwait earlycon=sbi root=/dev/mmcblk0p3 rw cma=0"


# for debug

mount -t debugfs none /sys/kernel/debug
cd /sys/kernel/debug/usb
cat devices