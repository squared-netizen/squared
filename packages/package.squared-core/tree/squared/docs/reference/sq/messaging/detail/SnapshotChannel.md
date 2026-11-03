# class SnapshotChannel

*Defined at [messaging/src/snapshot_channel.cppm:21](../../../../../messaging/src/snapshot_channel.cppm#L21)*

 Three buffers between one writer (the model's thread) and one reader (the UI's): the writer fills its own, then trades it for the middle one; the reader, when the middle one is newer than its own, trades its own for it. Neither ever waits, and the reader always gets the newest whole snapshot: a publish the reader never saw is simply replaced.

 Each buffer belongs to one side at a time, so the writer may make its own larger before filling it (a terminal after a rotation), and only the writer ever allocates. The middle buffer's index and whether it is newer than the reader's are one atomic byte.



## Functions

### SnapshotChannel

*public void SnapshotChannel()*

*Defined at [messaging/src/snapshot_channel.cppm:24](../../../../../messaging/src/snapshot_channel.cppm#L24)*

### SnapshotChannel

*public void SnapshotChannel(SnapshotChannel && )*

*Defined at [messaging/src/snapshot_channel.cppm:27](../../../../../messaging/src/snapshot_channel.cppm#L27)*

### SnapshotChannel

*public void SnapshotChannel(const SnapshotChannel & )*

*Defined at [messaging/src/snapshot_channel.cppm:25](../../../../../messaging/src/snapshot_channel.cppm#L25)*

### back

*public std::span<std::byte> back(std::size_t bytes)*

*Defined at [messaging/src/snapshot_channel.cppm:32](../../../../../messaging/src/snapshot_channel.cppm#L32)*

 The writer's buffer, at least `bytes` long: allocated only past its size. 

### latest

*public std::optional<SnapshotView> latest()*

*Defined at [messaging/src/snapshot_channel.cppm:55](../../../../../messaging/src/snapshot_channel.cppm#L55)*

 The reader's newest: its own, or the middle one when that is newer. 

### operator=

*public SnapshotChannel & operator=(SnapshotChannel && )*

*Defined at [messaging/src/snapshot_channel.cppm:28](../../../../../messaging/src/snapshot_channel.cppm#L28)*

### operator=

*public SnapshotChannel & operator=(const SnapshotChannel & )*

*Defined at [messaging/src/snapshot_channel.cppm:26](../../../../../messaging/src/snapshot_channel.cppm#L26)*

### publish

*public void publish(std::size_t bytes)*

*Defined at [messaging/src/snapshot_channel.cppm:43](../../../../../messaging/src/snapshot_channel.cppm#L43)*

 The writer's first `bytes` are the newest: traded for the middle buffer. 

### ~SnapshotChannel

*public void ~SnapshotChannel()*

*Defined at [messaging/src/snapshot_channel.cppm:29](../../../../../messaging/src/snapshot_channel.cppm#L29)*

