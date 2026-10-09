# Crashpad integration tests

Build `crashpad_integration_probe`, then run `ctest --test-dir <build> -L crashpad --output-on-failure`.
The probe is also included in the `unittests` target. Python 3.7 or newer is required
for these tests; game builds without Python skip their registration.

The suite launches isolated child processes and reads the actual minidumps. It
checks the dump header, directory bounds, exception context, thread list and
module list. On Windows it also checks exception codes. Each probe runs from an
unrelated working directory; executable and database paths contain spaces and
Unicode characters.

Cases cover access violations on the main and a worker thread, `abort`,
`std::terminate`, a caught fatal C++ exception, a missing handler, and an
unavailable primary database with an isolated per-user fallback. Windows adds
CRT invalid-parameter and pure-call failures, plus heap corruption. The heap
case installs a later handler that swallows the exception: only a live Crashpad
vectored handler can capture it, which detects the original client-lifetime bug.
Startup status files must report whether capture is enabled and the absolute
handler/database paths. Dumps and child output remain under `<test-build>/reports`.

For a small standalone build, configure this directory with
`-DOBK2_DEPENDENCY_BUILD_DIR=<existing-game-build>` to reuse its fetched Crashpad
source and Boost headers. The standalone project rebuilds just the Crashpad
dependencies and probe, without fetching dependencies or launching the game.
On Windows, use Visual Studio's bundled CMake from its Developer Environment,
as for the main project.
