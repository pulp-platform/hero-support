# DEPRECATED

This repository is part of a deprecated version of HERO.  Please refer to the [current HERO repository](https://github.com/pulp-platform/hero) for the current Linux kernel driver, user-space runtime libraries, and support files.

# HERO Support

This repository contains various software support infrastructure for HERO. In particular:
- The PULP Linux kernel-level driver, which enables shared virtual memory (SVM) between host and PULP.
- The PULP user-space software library (`libpulp`) for interfacing the driver with user-space application code.
- Support files and scripts to build Linux for the host.
- PULP standalone application launcher.
- PULP UART reader.
- Linux user-space utilities.

This repository is a submodule of the [HERO SDK](https://github.com/pulp-platform/hero-sdk). The build of the various components is controlled by the HERO SDK.

For further information about HERO, refer to our HERO paper at
https://arxiv.org/abs/1712.06497 .