# struct ProviderResult

*Defined at [messaging/src/provider_result.hpp:13](../../../../messaging/src/provider_result.hpp#L13)*



**brief** Owned result of one authoritative current-state request. 



## Members

public ProviderStatus status

public JsonValue payload

public int detail



## Functions

### provided

*public static ProviderResult provided(sq::data::JsonValue value)*

*Defined at [messaging/src/provider_result.hpp:30](../../../../messaging/src/provider_result.hpp#L30)*



**brief** Build a result carrying authoritative current state.

**value** Owned JSON payload; ownership is transferred into the result.

**return** Result with Provided status and the transferred payload.

### no_current_state

*public static ProviderResult no_current_state()*

*Defined at [messaging/src/provider_result.hpp:39](../../../../messaging/src/provider_result.hpp#L39)*



**brief** Build a result declaring that no current state exists.

**return** Default result with NoCurrentState status and an empty payload.

### failed

*public static ProviderResult failed(int reason)*

*Defined at [messaging/src/provider_result.hpp:46](../../../../messaging/src/provider_result.hpp#L46)*



**brief** Build a failed result.

**reason** Human-readable failure detail.

**return** Result with Failed status and the transferred detail text.



