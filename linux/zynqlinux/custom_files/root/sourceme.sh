cd /

# mount NFS share
#./mount_nfs.sh

# mount second partition on SD card
./mount_storage.sh

# go to mounted partition
cd /mnt/storage

# source adaptable startup script
if [ -f startup.sh ]; then
  source ./startup.sh
fi
