# class MessageDispatcher

*Defined at [messaging/message_dispatcher.hpp:40](../../../../messaging/message_dispatcher.hpp#L40)*



**brief** Bounded deterministic Telegram router for one time domain.

 The dispatcher borrows a Clock, owns no thread, and invokes Telegraphs on the thread calling update() or send_now(). Ordinary send() is queued.



## Records

PendingDelivery



## Functions

### MessageDispatcher

*public void MessageDispatcher(const sq::time::Clock & clock, MessageDispatcherConfig config)*



**brief** Construct a bounded router bound to one time domain.

**clock** Borrowed clock used for deadlines and queueing; must outlive the dispatcher. Non-owning.

**config** Capacity and per-update delivery limits.

### MessageDispatcher

*public void MessageDispatcher(const MessageDispatcher & )*

*Defined at [messaging/message_dispatcher.hpp:55](../../../../messaging/message_dispatcher.hpp#L55)*

### operator=

*public MessageDispatcher & operator=(const MessageDispatcher & )*

*Defined at [messaging/message_dispatcher.hpp:56](../../../../messaging/message_dispatcher.hpp#L56)*

### MessageDispatcher

*public void MessageDispatcher(MessageDispatcher && )*

*Defined at [messaging/message_dispatcher.hpp:57](../../../../messaging/message_dispatcher.hpp#L57)*

### operator=

*public MessageDispatcher & operator=(MessageDispatcher && )*

*Defined at [messaging/message_dispatcher.hpp:58](../../../../messaging/message_dispatcher.hpp#L58)*

### ~MessageDispatcher

*public void ~MessageDispatcher()*



**brief** Release all registrations and pending deliveries. 

### register_endpoint

*public SubscriptionResult register_endpoint(EndpointId endpoint, Telegraph & telegraph)*



**brief** Attach a Telegraph to a directed endpoint address.

**endpoint** Candidate endpoint address, validated then required to be unique.

**telegraph** Non-owning receiver; must outlive the subscription.

**return** Registration result; keep the returned Subscription alive to stay registered.

**note** No Lua binding exists.

### subscribe

*public SubscriptionResult subscribe(MessageId message, Telegraph & telegraph)*



**brief** Enroll a Telegraph as a broadcast subscriber of one message kind.

**message** Candidate message kind, validated before matching.

**telegraph** Non-owning receiver; must outlive the subscription.

**return** Subscription result; keep the returned Subscription alive to stay registered.

### register_provider

*public SubscriptionResult register_provider(MessageId message, EndpointId provider_endpoint, TelegramProvider & provider)*



**brief** Register the sole authoritative provider for one message kind.

**message** Message kind the provider covers.

**provider_endpoint** Endpoint receiving provider-generated current state deliveries.

**provider** Non-owning generator invoked synchronously during registration; must outlive the subscription.

**return** Registration result; keep the returned Subscription alive to stay registered.

**note** No Lua binding exists.

### send

*public DispatchResult send(Telegram telegram)*



**brief** Queue ordinary delivery at the current domain time.

**telegram** Envelope to deliver; copied into the pending queue.

**return** Queued/results; use the handle for later cancellation.

### schedule

*public DispatchResult schedule(int delay, Telegram telegram)*



**brief** Queue delivery after a non-negative domain-time delay.

**delay** Non-negative delay in domain time.

**telegram** Envelope to deliver; copied into the pending queue.

**return** Queued/results; use the handle for later cancellation.

### send_now

*public DeliveryReport send_now(const Telegram & telegram)*



**brief** Deliver synchronously for controlled internal use.

 Any requested receipt still enters the ordinary queue.

**telegram** Envelope to deliver using the current domain time.

**return** Compressed delivery outcome.

**note** Synchronous; bypasses the pending queue and delivery cap.

### cancel

*public bool cancel(DispatchHandle handle)*



**brief** Cancel one pending delivery and queue its receipt if requested.

**handle** Stable handle returned by a prior send/schedule call.

**return** Whether the delivery was found and cancelled.

### inspect_pending

*public void inspect_pending(Visitor && visitor)*

*Defined at [messaging/message_dispatcher.hpp:159](../../../../messaging/message_dispatcher.hpp#L159)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)

### update

*public int update()*



**brief** Deliver due Telegrams using one captured clock value.

**return** Number of deliveries drained this call, capped by config.

### pending_count

*public int pending_count()*



**brief** Count pending deliveries.

**return** Number of currently pending deliveries, bounded by capacity.

### pending_capacity

*public int pending_capacity()*



**brief** Report the pending delivery capacity.

**return** Configured pending delivery capacity.

### snapshot_pending

*public PendingSnapshotResult snapshot_pending()*



**brief** Create deterministic versioned JSON using remaining delays.

**return** Snapshot encoding every pending envelope with its remaining delay; Ready unless a subscription-targeted delivery exists.

**note** No Lua binding exists.

### restore_pending

*public PendingRestoreResult restore_pending(const sq::data::JsonValue & snapshot)*



**brief** Atomically restore a snapshot into an empty pending queue.

**snapshot** Snapshot produced by snapshot_pending().

**return** Restored/count; the queue must be empty beforehand.

**note** No Lua binding exists.



