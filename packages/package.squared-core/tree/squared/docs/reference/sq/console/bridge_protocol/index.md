# namespace bridge_protocol

## Functions

### good_name

*bool good_name(std::string_view name)*

*Defined at [console/src/bridge_protocol.hpp:53](../../../../../console/src/bridge_protocol.hpp#L53)*

 A name a file may be sent under: 1 to name_max bytes, not "." or "..", no '/', no control characters. Both ends check it.

