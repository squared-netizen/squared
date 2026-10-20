# class Subscription

*Defined at [messaging/subscription.hpp:16](../../../../messaging/subscription.hpp#L16)*



**brief** Move-only scoped endpoint registration or broadcast subscription.

 Destroying or resetting the handle unregisters it. A subscription may safely outlive its dispatcher, but a referenced Telegraph or TelegramProvider must remain alive while the subscription is active.



## Functions

### Subscription

*public void Subscription()*

*Defined at [messaging/subscription.hpp:20](../../../../messaging/subscription.hpp#L20)*



**brief** Construct an inactive, empty subscription handle. 

### ~Subscription

*public void ~Subscription()*



**brief** Unregister from the dispatcher when still active. 

### Subscription

*public void Subscription(Subscription && other)*



**brief** Move-construct from another handle.

**other** Handle to assume; left inactive afterward.

### Subscription

*public void Subscription(const Subscription & )*

*Defined at [messaging/subscription.hpp:39](../../../../messaging/subscription.hpp#L39)*

### operator=

*public Subscription & operator=(Subscription && other)*



**brief** Move-assign from another handle.

**other** Handle to assume; left inactive afterward.

**return** Reference to this handle.

**note** Releases the current registration first, if any.

### operator=

*public Subscription & operator=(const Subscription & )*

*Defined at [messaging/subscription.hpp:40](../../../../messaging/subscription.hpp#L40)*

### reset

*public void reset()*



**brief** Unregister from the dispatcher, if still active. 

### active

*public bool active()*



**brief** Check registration state.

**return** true while the subscription is still registered.



