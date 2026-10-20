# struct PendingSnapshotResult

*Defined at [messaging/src/pending_snapshot_result.hpp:12](../../../../messaging/src/pending_snapshot_result.hpp#L12)*



**brief** Owned result of one pending-queue snapshot. 



## Members

public PendingSnapshotStatus status

public JsonValue snapshot

public int detail



## Functions

### operator bool

*public bool operator bool()*

*Defined at [messaging/src/pending_snapshot_result.hpp:24](../../../../messaging/src/pending_snapshot_result.hpp#L24)*



**brief** Return whether a usable snapshot was produced. 



