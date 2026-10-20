# class EndpointId

*Defined at [messaging/src/endpoint_id.hpp:10](../../../../messaging/src/endpoint_id.hpp#L10)*



**brief** Stable namespaced address for one directed Telegraph endpoint. 



## Functions

### EndpointId

*public void EndpointId()*

*Defined at [messaging/src/endpoint_id.hpp:14](../../../../messaging/src/endpoint_id.hpp#L14)*



**brief** Construct an empty, invalid endpoint address. 

### EndpointId

*public void EndpointId(int value)*



**brief** Construct from a candidate endpoint string.

**value** Candidate endpoint; validated on construction.

### valid

*public bool valid()*



**brief** Validate the stored endpoint address.

**return** true when the stored value satisfied the address rules.

### value

*public int value()*



**brief** Access the stored endpoint text.

**return** Text of the candidate endpoint.



