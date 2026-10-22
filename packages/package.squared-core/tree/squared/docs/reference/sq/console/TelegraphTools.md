# class TelegraphTools

*Defined at [console/telegraph_tools.hpp:35](../../../../console/telegraph_tools.hpp#L35)*



**brief** The Telegraph's development tools, on the UI's end of a line: its Console commands, the name table its text form uses, the tap that writes every telegram as a line, and recording and replaying what the UI sends.

 | Typed | Does | |---|---| | `telegraph` | each capacity and its highest fill since start | | `telegraph state` | every value the UI holds, one line each, ordered by address | | `telegraph tap [on \| off]` | every telegram as a line (`ui>model set settings.sound false`), or not; off at first | | `telegraph record [file] \| record stop` | what the UI sends, each line with the milliseconds since the start, to `file` (`replay.txt`) in the files directory | | `telegraph replay [file] [fast] \| replay stop` | send a recording again, at its pace or as fast as the ring takes it (`update` paces it) |

 For Console builds: about 15 KB inline (the name table, `TELEGRAPH_NAMES`, most of it), nothing allocated. One to a line, on the UI thread; it must outlive the line's use of it, and it detaches itself when it goes.



## Functions

### TelegraphTools

*public void TelegraphTools(messaging::TelegraphUi & ui, const char * directory)*



**brief** The tools for `ui`; they keep its name table from now on.

**directory** Where `record` writes and `replay` reads: the app's `files/` directory. nullptr: neither is offered.

### TelegraphTools

*public void TelegraphTools(const TelegraphTools & )*

*Defined at [console/telegraph_tools.hpp:48](../../../../console/telegraph_tools.hpp#L48)*

### operator=

*public TelegraphTools & operator=(const TelegraphTools & )*

*Defined at [console/telegraph_tools.hpp:49](../../../../console/telegraph_tools.hpp#L49)*

### TelegraphTools

*public void TelegraphTools(TelegraphTools && )*

*Defined at [console/telegraph_tools.hpp:50](../../../../console/telegraph_tools.hpp#L50)*

### operator=

*public TelegraphTools & operator=(TelegraphTools && )*

*Defined at [console/telegraph_tools.hpp:51](../../../../console/telegraph_tools.hpp#L51)*

### ~TelegraphTools

*public void ~TelegraphTools()*

### add_commands

*public std::size_t add_commands(Console & console)*



**brief** `telegraph`, and the groups `telegraph` and `state`. How many did not fit. 

### names

*public const messaging::NameTable & names()*

*Defined at [console/telegraph_tools.hpp:74](../../../../console/telegraph_tools.hpp#L74)*

### set_sink

*public void set_sink(Sink sink, void * context)*



**brief** Where the tap's lines go; nullptr (the default): the Console's scrollback. 

### update

*public void update(time::TimePoint now)*



**brief** Once a frame: the replay's next telegrams, as their time comes. 

### tap

*public void tap(bool on)*



**brief** Turn the tap on or off. 

### record

*public bool record(std::string_view file)*



**brief** Record to `file` in the directory; false when it cannot be opened. 

### stop_recording

*public std::uint32_t stop_recording()*



**brief** Stop recording; how many telegrams were written. 

### replay

*public bool replay(std::string_view file, bool fast)*



**brief** Replay `file`, paced by `update` (or `fast`); false when it cannot be opened. 

### stop_replay

*public void stop_replay()*

### ui

*public const messaging::TelegraphUi & ui()*

*Defined at [console/telegraph_tools.hpp:75](../../../../console/telegraph_tools.hpp#L75)*

### tapping

*public bool tapping()*

*Defined at [console/telegraph_tools.hpp:76](../../../../console/telegraph_tools.hpp#L76)*

### recording

*public bool recording()*

*Defined at [console/telegraph_tools.hpp:77](../../../../console/telegraph_tools.hpp#L77)*

### replaying

*public bool replaying()*

*Defined at [console/telegraph_tools.hpp:78](../../../../console/telegraph_tools.hpp#L78)*



