# namespace messaging

## Records

* [DeliveryReport](DeliveryReport.md)
* [DispatchHandle](DispatchHandle.md)
* [DispatchResult](DispatchResult.md)
* [EndpointId](EndpointId.md)
* [MessageDispatcherConfig](MessageDispatcherConfig.md)
* [MessageDispatcher](MessageDispatcher.md)
* [MessageId](MessageId.md)
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

### operator==

*bool operator==(const EndpointId & , const EndpointId & )*

*Defined at [messaging/src/endpoint_id.hpp:35](../../../../messaging/src/endpoint_id.hpp#L35)*



**brief** Return whether two endpoints are textually equal. 

### operator==

*bool operator==(const MessageId & , const MessageId & )*

*Defined at [messaging/src/message_id.hpp:41](../../../../messaging/src/message_id.hpp#L41)*



**brief** Return whether two identifiers are textually equal. 

### receipt_message_id

*const MessageId & receipt_message_id()*



**brief** Framework message kind used for queued return receipts. 

### receipt_status_name

*int receipt_status_name(ReceiptStatus status)*



**brief** Stable lowercase receipt status name used in JSON payloads. 

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

