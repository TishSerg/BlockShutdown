# BlockShutdown

A tiny app allowing you to block Windows from accidental shutdown/reboot/logoff preserving your apps from being closed by the system and allowing you to continue your work.

Usage:

- Just run `BlockShutdown.exe` - Will block shutdown infinitely (until killed) with default message.
- `BlockShutdown.exe -?` - Get usage help text.
- `BlockShutdown.exe ["Reason string" [timeout (sec)]]` - Will block shutdown showing specified reason string on Windows shutdown screen optionally unblocking after specified time. Use '%s' placeholder for timeout due time in the reason string else a default one will be appended. Note: in my tests, Windows gives up on the shutdown procedure if the timeout is about of 1 minute or more.
  - `BlockShutdown.exe "Did you accidentally press the power button?"`
  - `BlockShutdown.exe "Shutdown was blocked for 30 seconds just in case" 30`
  - `BlockShutdown.exe "You can cancel Shutdown until %s" 45`

- The app sits in the system tray while working. You can stop it from there.
