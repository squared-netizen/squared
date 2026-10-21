# struct ListOutput

*Defined at [messaging/model_list.hpp:72](../../../../messaging/model_list.hpp#L72)*



**brief** Where a list's own telegrams go (its count, the uid at a row): the Telegraph, or a test's stand-in. `context` is also what a source's `publish` is given.



## Members

public void * context

public void (*)(void *, const Telegram &) noexcept send



