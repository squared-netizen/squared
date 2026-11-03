# class SnapshotKey

*Defined at [messaging/snapshot_key.cppm:23](../../../../messaging/snapshot_key.cppm#L23)*



**brief** The name of a snapshot: bulk state the model publishes whole and the UI reads the newest of (a terminal's screen, a canvas's pixels), the third channel beside input and telegrams.

 ```cpp inline constexpr sq::SnapshotKey screen{ "term.screen" }; ```

 A snapshot has no type the compiler checks: it is bytes, laid out by the model and read by the actor that draws it, which agree on the layout (a `term::Terminal` writes and reads its own, `term::write_snapshot`). 16 bytes: the name and its text, for the dev tools.



## Functions

### SnapshotKey

*public void SnapshotKey(const char (&)[Length] text)*

*Defined at [messaging/snapshot_key.cppm:28](../../../../messaging/snapshot_key.cppm#L28)*

 NOLINTNEXTLINE(google-explicit-constructor): a literal is a key, as in "term.screen"

### name

*public Name name()*

*Defined at [messaging/snapshot_key.cppm:33](../../../../messaging/snapshot_key.cppm#L33)*

### text

*public std::string_view text()*

*Defined at [messaging/snapshot_key.cppm:34](../../../../messaging/snapshot_key.cppm#L34)*

