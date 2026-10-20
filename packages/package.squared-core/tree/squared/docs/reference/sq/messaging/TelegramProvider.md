# class TelegramProvider

*Defined at [messaging/src/telegram_provider.hpp:17](../../../../messaging/src/telegram_provider.hpp#L17)*



**brief** Generates authoritative current state for a new subscriber.

 Providers replace generic retained-message replay, which can deliver stale historical envelopes. The dispatcher queues a fresh result only for the newly registered subscriber.



## Functions

### provide

*public ProviderResult provide(const MessageId & message)*



**brief** Produce the authoritative current state for one message kind.

**message** Kind for which current state is requested.

**return** Owned result carrying the current state, no state, or a failure; ownership of any payload transfers to the caller.

**note** Called synchronously on the dispatcher's calling thread during subscriber registration. Must not invoke dispatcher methods.

### ~TelegramProvider

*public void ~TelegramProvider()*

*Defined at [messaging/src/telegram_provider.hpp:20](../../../../messaging/src/telegram_provider.hpp#L20)*



