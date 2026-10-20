# class Telegraph

*Defined at [messaging/telegraph.hpp:9](../../../../messaging/telegraph.hpp#L9)*



**brief** Receiver of Telegram values on the dispatcher's calling thread. 



## Functions

### handle_message

*public bool handle_message(const Telegram & telegram)*



**brief** Handle one Telegram.

**telegram** Immutable-access envelope to handle; valid for the duration of the call.

**return** true when the message was handled; false otherwise.

**throws**exception propagates to the dispatcher caller; keep the handler noexcept-safe.

**note** Invoked synchronously on the dispatcher's calling thread. Must not invoke dispatcher methods recursively.

### ~Telegraph

*public void ~Telegraph()*

*Defined at [messaging/telegraph.hpp:12](../../../../messaging/telegraph.hpp#L12)*



