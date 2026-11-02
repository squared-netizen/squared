# class ModelList

*Defined at [messaging/model_list.cppm:92](../../../../messaging/model_list.cppm#L92)*

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

*Defined at [messaging/model_list.cppm:121](../../../../messaging/model_list.cppm#L121)*

### count

*public std::uint32_t count()*

*Defined at [messaging/model_list.cppm:122](../../../../messaging/model_list.cppm#L122)*

### first

*public std::uint32_t first()*

*Defined at [messaging/model_list.cppm:124](../../../../messaging/model_list.cppm#L124)*

### rows

*public std::uint32_t rows()*

*Defined at [messaging/model_list.cppm:125](../../../../messaging/model_list.cppm#L125)*



