cd /

# mount NFS share
#./mount_nfs.sh

# mount second partition on SD card
./mount_storage.sh

# go to mounted NFS share
cd /mnt/storage

# execute adaptable startup script
if [ -f startup.sh ]; then
  ./startup.sh
fi
