# Custom Files for Yocto-Based PetaLinux Root File System

Custom files such as ssh keys, `known_hosts` and NFS mount scripts be copied inside root file system generated by PetaLinux and Yocto.

How to install:

1. In your petalinux workspace directory type

   ```petalinux-create -t apps -n custom-files --enable```

   to create and enable a custom app called custom-files and register it with Yocto.

2. Navigate to

   ```project-spec/meta-user/recipes-apps/custom-files```

   and open the file `custom-files.bb`.

3. Modify this file according to `custom-files.bb` in this repo.

4. Copy the content of `files`, i.e., the files mentioned in the modified `custom-files.bb` to `project-spec/meta-user/recipes-apps/custom-files/files/`.

5. Type

   ```petalinux-build -c rootfs/custom-files```

   to build the app (can probably be skipped).

6. Type

   ```petalinux-build -c rootfs```

   to build the root file system.

7. Type

   ```petalinux-package --image```

   to repack the FIT image `image.ub` containing the modified root file system, the kernel and the device tree blob.