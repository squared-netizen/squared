# namespace messaging

## Namespaces

* [detail](detail/index.md)


## Records

* [Address](Address.md)
* [DeliveryReport](DeliveryReport.md)
* [DispatchHandle](DispatchHandle.md)
* [DispatchResult](DispatchResult.md)
* [EndpointId](EndpointId.md)
* [MessageDispatcherConfig](MessageDispatcherConfig.md)
* [MessageDispatcher](MessageDispatcher.md)
* [MessageId](MessageId.md)
* [NameTableProbe](NameTableProbe.md)
* [NameTable](NameTable.md)
* [Name](Name.md)
* [PendingMessageView](PendingMessageView.md)
* [PendingRestoreResult](PendingRestoreResult.md)
* [PendingSnapshotResult](PendingSnapshotResult.md)
* [ProviderResult](ProviderResult.md)
* [SubscriptionResult](SubscriptionResult.md)
* [Subscription](Subscription.md)
* [TelegramProvider](TelegramProvider.md)
* [Telegram](Telegram.md)
* [Telegraph](Telegraph.md)


## Functions

### name_hash

*int name_hash(int text, int seed)*

*Defined at [messaging/name.hpp:17](../../../../messaging/name.hpp#L17)*



**brief** 64-bit FNV-1a of a name's text, continuing from `seed`.

 The id a Name carries. Continuing from a seed lets a name be hashed in pieces, as `item[17].name` is: "item", then ".name", with nothing copied.

### operator<=>

*auto operator<=>(Name , Name )*

*Defined at [messaging/name.hpp:101](../../../../messaging/name.hpp#L101)*

### operator==

*bool operator==(Name , Name )*

*Defined at [messaging/name.hpp:100](../../../../messaging/name.hpp#L100)*

### operator==

*bool operator==(const Address & , const Address & )*

*Defined at [messaging/address.hpp:24](../../../../messaging/address.hpp#L24)*

### operator==

*bool operator==(const EndpointId & , const EndpointId & )*

*Defined at [messaging/src/endpoint_id.hpp:35](../../../../messaging/src/endpoint_id.hpp#L35)*



**brief** Return whether two endpoints are textually equal. 

### operator==

*bool operator==(const MessageId & , const MessageId & )*

*Defined at [messaging/src/message_id.hpp:41](../../../../messaging/src/message_id.hpp#L41)*



**brief** Return whether two identifiers are textually equal. 

### parse_address

*int parse_address(int text)*



**brief** An address from its text form, with no name table: the name is hashed.

 `player.hp`, `item[17].name`, `inventory[20]`: the uid in brackets after the first segment, in decimal. `#` and 16 hex digits stand for a name's id written without its text, with the uid after it (`#0123456789abcdef[17]`), which is how an unregistered name is written. `[0]` reads as no uid.

**return** The address, or nothing when `text` is not one (not a valid name, a uid past 4294967295, brackets anywhere else).

### receipt_message_id

*const MessageId & receipt_message_id()*



**brief** Framework message kind used for queued return receipts. 

### receipt_status_name

*int receipt_status_name(ReceiptStatus status)*



**brief** Stable lowercase receipt status name used in JSON payloads. 

### valid_name

*bool valid_name(int text)*

*Defined at [messaging/name.hpp:35](../../../../messaging/name.hpp#L35)*



**brief** Whether `text` is a name: dot-separated segments of `a`-`z`, `0`-`9` and `_`, none empty, at most 255 bytes in all (`player.hp`, `item.name`).

 One spelling per name, so one id per name: no capitals, no spaces. Which object a telegram is about never goes in the name; that is the uid.

## Enums

| enum class DispatchStatus |

--

| Queued |
| InvalidTelegram |
| InvalidDelay |
| QueueFull |
| HandleExhausted |


*Defined at [messaging/src/dispatch_status.hpp:9](../../../../messaging/src/dispatch_status.hpp#L9)*



**brief** Outcome category of one dispatch operation. 

| enum class PendingRestoreStatus |

--

| Restored |
| DispatcherNotEmpty |
| InvalidSnapshot |
| CapacityReached |
| TimeOverflow |
| HandleExhausted |


*Defined at [messaging/src/pending_restore_status.hpp:9](../../../../messaging/src/pending_restore_status.hpp#L9)*



**brief** Outcome category of one pending-queue restore. 

| enum class PendingSnapshotStatus |

--

| Ready |
| SubscriptionTargetedDelivery |


*Defined at [messaging/src/pending_snapshot_status.hpp:9](../../../../messaging/src/pending_snapshot_status.hpp#L9)*



**brief** Outcome category of one pending-queue snapshot. 

| enum class ProviderStatus |

--

| Provided |
| NoCurrentState |
| Failed |


*Defined at [messaging/src/provider_status.hpp:9](../../../../messaging/src/provider_status.hpp#L9)*



**brief** Outcome category of one authoritative current-state request. 

| enum class ReceiptStatus |

--

| Handled |
| Unhandled |
| ReceiverUnavailable |
| Cancelled |


*Defined at [messaging/src/receipt_status.hpp:9](../../../../messaging/src/receipt_status.hpp#L9)*



**brief** Outcome of delivering one Telegram to its subscribers. 

| enum class SubscriptionStatus |

--

| Registered |
| InvalidId |
| AlreadyRegistered |
| CapacityReached |
| ProviderFailed |
| InitialStateQueueFull |
| HandleExhausted |


*Defined at [messaging/src/subscription_status.hpp:9](../../../../messaging/src/subscription_status.hpp#L9)*



**brief** Outcome category of one registration operation. 

