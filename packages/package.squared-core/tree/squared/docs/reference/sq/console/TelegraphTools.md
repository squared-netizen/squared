# class TelegraphTools

*Defined at [console/telegraph_tools.cppm:28](../../../../console/telegraph_tools.cppm#L28)*



**brief** The Telegraph's development tools, on the UI's end of a line: its Console commands, the name table its text form uses, the tap that writes every telegram as a line, and recording and replaying what the UI sends.

 | Typed | Does | |---|---| | `telegraph` | each capacity and its highest fill since start | | `telegraph state` | every value the UI holds, one line each, ordered by address | | `telegraph tap [on \| off]` | every telegram as a line (`ui>model set settings.sound false`), or not; off at first | | `telegraph record [file] \| record stop` | what the UI sends, each line with the milliseconds since the start, to `file` (`replay.txt`) in the files directory | | `telegraph replay [file] [fast] \| replay stop` | send a recording again, at its pace or as fast as the ring takes it (`update` paces it) |

 For Console builds: about 15 KB inline (the name table, `TELEGRAPH_NAMES`, most of it), nothing allocated. One to a line, on the UI thread; it must outlive the line's use of it, and it detaches itself when it goes.



## Functions

### TelegraphTools

*public void TelegraphTools(TelegraphTools && )*

*Defined at [console/telegraph_tools.cppm:43](../../../../console/telegraph_tools.cppm#L43)*

### TelegraphTools

*public void TelegraphTools(const TelegraphTools & )*

*Defined at [console/telegraph_tools.cppm:41](../../../../console/telegraph_tools.cppm#L41)*

### TelegraphTools

*public void TelegraphTools(messaging::TelegraphUi & ui, const char * directory)*



**brief** The tools for `ui`; they keep its name table from now on.

**directory** Where `record` writes and `replay` reads: the app's `files/` directory. nullptr: neither is offered.

### add_commands

*public std::size_t add_commands(Console & console)*



**brief** `telegraph`, and the groups `telegraph` and `state`. How many did not fit. 

### names

*public const messaging::NameTable & names()*

*Defined at [console/telegraph_tools.cppm:67](../../../../console/telegraph_tools.cppm#L67)*

### operator=

*public TelegraphTools & operator=(TelegraphTools && )*

*Defined at [console/telegraph_tools.cppm:44](../../../../console/telegraph_tools.cppm#L44)*

### operator=

*public TelegraphTools & operator=(const TelegraphTools & )*

*Defined at [console/telegraph_tools.cppm:42](../../../../console/telegraph_tools.cppm#L42)*

### record

*public bool record(std::string_view file)*



**brief** Record to `file` in the directory; false when it cannot be opened. 

### recording

*public bool recording()*

*Defined at [console/telegraph_tools.cppm:70](../../../../console/telegraph_tools.cppm#L70)*

### replay

*public bool replay(std::string_view file, bool fast)*



**brief** Replay `file`, paced by `update` (or `fast`); false when it cannot be opened. 

### replaying

*public bool replaying()*

*Defined at [console/telegraph_tools.cppm:71](../../../../console/telegraph_tools.cppm#L71)*

### set_sink

*public void set_sink(Sink sink, void * context)*



**brief** Where the tap's lines go; nullptr (the default): the Console's scrollback. 

### stop_recording

*public std::uint32_t stop_recording()*



**brief** Stop recording; how many telegrams were written. 

### stop_replay

*public void stop_replay()*

### tap

*public void tap(bool on)*



**brief** Turn the tap on or off. 

### tapping

*public bool tapping()*

*Defined at [console/telegraph_tools.cppm:69](../../../../console/telegraph_tools.cppm#L69)*

### ui

*public const messaging::TelegraphUi & ui()*

*Defined at [console/telegraph_tools.cppm:68](../../../../console/telegraph_tools.cppm#L68)*

### update

*public void update(time::TimePoint now)*



**brief** Once a frame: the replay's next telegrams, as their time comes. 

### ~TelegraphTools

*public void ~TelegraphTools()*

