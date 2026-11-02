# struct ListOutput

*Defined at [messaging/model_list.cppm:68](../../../../messaging/model_list.cppm#L68)*



**brief** Where a list's own telegrams go (its count, the uid at a row): the Telegraph, or a test's stand-in. `context` is also what a source's `publish` is given.



## Members

public void * context

public void (*)(void *, const Telegram &) noexcept send



