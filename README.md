Collection of tools for NTO contest.

Unfortunately we had to use Arduino platform.
`compile_commands.json` generator, config generator, makefile and shell scripts
make life easier.

Beside that, the following features are included:

* Better backtraces and script to automatically find source locations by looking at ELF debug info.
* Insecure, but fast OTA updates using TCP (700 KiB in ~7 seconds vs ~6.5 on wire).
* UDP/TCP logger that can send messages to multiple subscribed clients at once with session tracking.
* Task monitor.
* Convinient wrappers for ESP IDF libraries.

I would never recommend someone use these tools - just use ESP IDF infrastructure.
Our case was very specific.
