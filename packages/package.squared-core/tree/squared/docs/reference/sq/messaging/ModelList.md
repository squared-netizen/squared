# class ModelList

*Defined at [messaging/model_list.hpp:96](../../../../messaging/model_list.hpp#L96)*



**brief** The model's end of one virtualized list. It answers the list widget's `view` with the rows in it, and after a change sends only what changed on screen: a row whose item changed, and the fields of items new to the view.

 Telegrams, for a list named `inventory`:

 | Telegram | Sent | |---|---| | `state inventory.count 300` | at start, and when the count changes | | `state inventory[20] 17` | row 20 shows the item with uid 17 (row 0 is written `inventory`) | | the item's fields (`state item[17].name ...`) | by the source's `publish`, for items new to the view |

 Over storage its owner gives it: 4 bytes a row the view may hold, and a scratch area as large, which every list on one thread may share. The model's thread only.



## Functions

### ModelList

*public void ModelList(Name name, ListSource source, std::span<std::uint32_t> window, std::span<std::uint32_t> scratch)*



**brief** The list `name`, its rows from `source`. `window` is the most rows a view holds (4 bytes each, at least 1); `scratch` is at least as long, and may be shared by every list on this thread.

### start

*public void start(const ListOutput & out)*



**brief** Send the count. Once, before anything else. 

### view

*public void view(std::uint32_t first, std::uint32_t last, const ListOutput & out)*



**brief** The widget shows rows `first` to `last`, inclusive, margin included (`do inventory.view 20 52`). Clamped to the count and to the window's size. `last` before `first` is a reset, from a new widget that knows nothing: what was sent is forgotten, so the next view sends all of it, and the count is sent.

### changed

*public void changed(const ListOutput & out)*



**brief** The rows changed (a sort, an insert, a removal): the count, then the view. 

### changed

*public void changed(std::uint32_t uid, const ListOutput & out)*



**brief** One item's fields changed: published again if it is in the view. 

### name

*public Name name()*

*Defined at [messaging/model_list.hpp:125](../../../../messaging/model_list.hpp#L125)*

### count

*public std::uint32_t count()*

*Defined at [messaging/model_list.hpp:126](../../../../messaging/model_list.hpp#L126)*

### first

*public std::uint32_t first()*

*Defined at [messaging/model_list.hpp:128](../../../../messaging/model_list.hpp#L128)*



**brief** The rows in the view: from `first()`, `rows()` of them. 

### rows

*public std::uint32_t rows()*

*Defined at [messaging/model_list.hpp:129](../../../../messaging/model_list.hpp#L129)*



