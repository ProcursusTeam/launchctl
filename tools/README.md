# xpchook

This little dylib uses DYLD interposing to let you watch what messages are being sent using `xpc_pipe_routine()`. It is useful for analyzing the messages that `launchctl` sends.
