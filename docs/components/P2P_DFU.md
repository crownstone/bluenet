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

# Process

This is an in depth flow chart of the dfu process.

```mermaid
flowchart TD;
    %% Phases corresponding to the MeshDfuHost::Phase enum
    N[None]
    I[Idle]
    
    %% function calls and branches
    _i(["init()"])
    _c(["copyFirmwareTo()"])

    %% transitions
    N --> _i -->|success| I
    _i -->|fail| N

    I --> _c -->|success| M
    _c -->|fail| I

    subgraph M["ConnectTargetInDfuMode"]
        direction LR
        M_S[start]
        M_C[complete]
        M_A[abort]
        M_CON(["connect()"])
        M_CHE(["checkConnection()"])
        M_RET{retry?}
        
        M_S --> M_CON-->|timeout| M_CHE
        M_CON -->|EVT_BLE_CENTRAL_CONNECT_RESULT| M_CHE

        M_CHE -->|fail| M_RET
        M_CHE -->|success| M_C
        M_RET -->|retry count-=1| M_S
        M_RET -->|retry count == 0| M_A
    end

    M -->M_COMPLETE([completeConnectTargetInDfuMode]) --> D

    subgraph D["DiscoverDfuCharacteristics"]
        direction LR
        D_S[start]
        D_C[complete]
        D_A[abort]
        D_R[restart]
        D_RET{retry?}
        D_DIS(["discoverServices()"])
        D_RES(["onResult()"])
        D_CHE{dfu mode?}
        D_BTD(["disconnect()"])

        D_S --> D_DIS -->|not ERR_WAIT_FOR_SUCCESS| D_RET
        D_DIS -->|EVT_BLE_CENTRAL_DISCOVERY_RESULT| D_RES
        D_DIS -->|timeout| D_C
        D_RET -->|retry count-=1| D_R
        D_R -->|sleep| D_S
        D_RET -->|retry count == 0| D_A
        D_RES -->|not ERR_SUCCESS| D_A
        D_RES --> D_CHE 
        D_CHE -->|yes| D_C
        D_CHE -->|no| D_BTD --> D_C
    end

    D --> D_COMPLETE([completeDiscoverDfuCharacteristics])
    %% D_COMPLETE -->|target is in dfu mode| preparing
    %% D_COMPLETE -->|not tried dfu| trigger dfu
    %% D_COMPLETE -->|else| abort 

    subgraph W["WaitForTargetReboot"]
        direction LR
        W_S[start]
        W_C[complete]
        W_A[abort]

        W_S --> W_C & W_A
    end

```

TargetTriggerDfuMode            M
WaitForTargetReboot             W
ConnectTargetInDfuMode          C
DiscoverDfuCharacteristics      D
TargetPreparing                 P
TargetInitializing              T
TargetUpdating                  U
TargetVerifying                 V
Aborting                        A