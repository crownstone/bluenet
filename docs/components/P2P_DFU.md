# Peer to peer firmware updates

This component enables crownstones to update their peers on command.
Currently it doesn't synchronize or do version checking except for checks
that the bootloader already executes.


# Testing

```mermaid
sequenceDiagram
    participant L as laptop
    participant H as cs-host
    participant T as cs-target
    L->>L: build targets
    L->>H: erase
    L->>T: erase
    
    L->>T: load bootloader with logs + master firmware (version X)
    T->>T: reboot (prints version X)
    L->>H: load bootloader no logs + p2p dfu feature firmware (version Y)
    H->>H: reboot (prints version Y)

    H->>T: dfu
    T->>T: on error: bootloader prints
    T->>T: on success: reboots (prints version Y)
```