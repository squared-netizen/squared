# struct PendingRestoreResult

*Defined at [messaging/src/pending_restore_result.hpp:12](../../../../messaging/src/pending_restore_result.hpp#L12)*



**brief** Owned result of one pending-queue restore. 



## Members

public PendingRestoreStatus status

public int restored_count

public int detail



## Functions

### operator bool

*public bool operator bool()*

*Defined at [messaging/src/pending_restore_result.hpp:24](../../../../messaging/src/pending_restore_result.hpp#L24)*



**brief** Return whether the snapshot was atomically restored. 



