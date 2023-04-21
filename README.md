# TUN SERVER
This project is  about the server side of a vpn, that:
* Reads packets from a tunnel and establish the necessary connection on its side,
* Write back any data sent from application server

The server does this instead of just modifying the packet's header and act us a two-way bridge, simply because that requires **root** / **Administrative** privileges. And this is the main objective of this project. A vpn server that could be **run by a normal user** with no special permission of drivers.

The way how this server acts is much more like a **socks proxy**, but without any co-operation with the vpn client.

#### _I know smart enough, right!_