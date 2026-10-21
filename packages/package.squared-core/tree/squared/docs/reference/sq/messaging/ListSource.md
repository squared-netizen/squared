# struct ListSource

*Defined at [messaging/model_list.hpp:39](../../../../messaging/model_list.hpp#L39)*



**brief** A model's list as three plain functions over it: what `ModelList` calls. Made by `list_source`; no virtual call, nothing allocated.



## Members

public void * source

public std::uint32_t (*)(const void *) noexcept count

public std::uint32_t (*)(const void *, std::uint32_t) noexcept uid_at

public void (*)(void *, void *, std::uint32_t) noexcept publish



