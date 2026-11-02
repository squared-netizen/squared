# class ListWindow

*Defined at [messaging/list_window.cppm:30](../../../../messaging/list_window.cppm#L30)*



**brief** The list widget's end of a virtualized list: which rows to ask the model for. The window is the rows on screen and a screen more on each side; it is asked for again only when the rows on screen come within half a screen of its edge, or the count changes, so a slow scroll asks every half screen and a fling at most once a frame.

 Holds no rows itself: they are in the latest-value cache, by row, and their fields by uid. The UI thread only.



## Functions

### ListWindow

*public void ListWindow(std::uint32_t most_rows)*



**brief** A window of at most `most_rows` rows (`TELEGRAPH_LIST_VIEW`; at least 1). 

### count

*public std::uint32_t count()*

*Defined at [messaging/list_window.cppm:49](../../../../messaging/list_window.cppm#L49)*

### set_count

*public void set_count(std::uint32_t count)*



**brief** How many rows the list has, from `state <list>.count`. 

### visible

*public std::optional<RowSpan> visible(std::uint32_t first, std::uint32_t rows)*



**brief** Each frame: the first row on screen and how many fit. The rows to ask the model for, when they should be asked for; nothing otherwise, or when the list is empty.

### window

*public RowSpan window()*

*Defined at [messaging/list_window.cppm:48](../../../../messaging/list_window.cppm#L48)*



**brief** The rows last asked for: the ones to keep bound in the cache. 

