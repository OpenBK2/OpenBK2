# Two SDL audio bugs found under ThreadSanitizer, ready to file

Found 2026-10-13 running a replay verify of this engine against a TSan build.
Both are in SDL itself, not in our use of it: we call the audio API from the
main thread only, which is what it documents. Neither is fixed in
`release-3.4.18`, the newest release at the time of writing, nor on `main` 400
commits past the revision `cmake/sdl.cmake` pins (`f3815ede2`). The two code
sites are byte-identical across all three, so there is nothing to bump to.

Line numbers below are from the pinned revision and match `main`.

What we did locally: `launch.sh` gives any `--verify` run
`SDL_AUDIO_DRIVER=dummy`, which avoids the second bug entirely. No patch is
carried against the pinned SDL. The race is left unsuppressed in `tsan.supp`
deliberately, since SDL is built from source here and the report is a true one.

---

## 1. Data race on `bytes_requested` in the PulseAudio backend

**Where.** `src/audio/pulseaudio/SDL_pulseaudio.c`.

`struct SDL_PrivateAudioData::bytes_requested` is maintained under the
PulseAudio threaded-mainloop lock in two of the three places that touch it:

- `WriteCallback` (`:409`) increments it. Pulse invokes stream callbacks with
  the mainloop lock held.
- `PULSEAUDIO_WaitDevice` (`:423`) reads it in its wait predicate with the lock
  held (`:421` to `:434`).

The other two accesses take no lock at all:

- `PULSEAUDIO_GetDeviceBuf` (`:465`) reads it:
  `const size_t reqsize = (size_t) SDL_min(*buffer_size, h->bytes_requested);`
- `PULSEAUDIO_PlayDevice` (`:456`) decrements it *after* releasing the lock it
  took for `pa_stream_write` at `:447` to `:449`:
  `h->bytes_requested -= buffer_size;`

Both run on SDL's playback thread while the Pulse mainloop thread can be in
`WriteCallback`.

**Consequence.** A lost increment under-reports what the server wants, so the
device writes short; a torn read feeds `pa_stream_begin_write` a bogus size.
`SDL_assert(h->bytes_requested >= buffer_size)` at `:445` is also evaluated
unlocked, so a debug build can trip it spuriously.

**What TSan says**, with both stacks:

```
WARNING: ThreadSanitizer: data race
  Write of size 4 ... WriteCallback SDL_pulseaudio.c:409
  Previous read of size 4 ... PULSEAUDIO_GetDeviceBuf SDL_pulseaudio.c:465
```

**Suggested fix.** Move the decrement inside the region `PlayDevice` already
locks, and take the lock around the read in `GetDeviceBuf`. Making the field
atomic would silence the report without fixing it: the wait predicate at `:423`
pairs the field with the mainloop condition variable, so the lock is what the
design needs, not atomicity.

---

## 2. Shutdown deadlock closing an idle PulseAudio device

**Where.** `src/audio/SDL_audio.c`, `ClosePhysicalAudioDevice`, and
`src/audio/pulseaudio/SDL_pulseaudio.c`, `PULSEAUDIO_CloseDevice`.

The close path is:

1. `SDL_SetAtomicInt(&device->shutdown, 1)` (`SDL_audio.c:1653`)
2. unlock the device
3. `SDL_WaitThread(device->thread, NULL)` -- join the playback thread
4. `current_audio.impl.CloseDevice(device)`

The playback thread, if it is parked in `PULSEAUDIO_WaitDevice`, sits in
`pa_threaded_mainloop_wait` (`SDL_pulseaudio.c:425`) and rechecks
`device->shutdown` only after something signals the mainloop. Setting an atomic
signals nothing. The only call that exists for this is in
`PULSEAUDIO_CloseDevice` (`:593`), commented "in case the device thread is
waiting somewhere, this will unblock it" -- and step 4 runs *after* the join in
step 3.

**So the wake-up is ordered after the thing it is supposed to wake.** If the
stream is idle when the device is closed -- no outstanding write callback, so
`bytes_requested == 0` and the predicate at `:423` is false -- nothing ever
signals, and `SDL_WaitThread` blocks forever.

**Reproducer.** Open a playback device, let it go quiet, close it. For us it is
deterministic at the end of a headless replay: all sounds have finished, so the
device is always idle by then. Observed stacks:

```
Thread 1 (main)        SDL_SYS_WaitThread -> pthread_join          SDL_systhread.c:287
Thread 2 (SDLAudioP27) pthread_cond_wait on the Pulse mainloop condvar
```

**Suggested fix.** Signal the mainloop between setting `shutdown` and the join.
Either a backend hook called after `shutdown` is set and before `SDL_WaitThread`
(the ALSA and PipeWire backends have the same shape of wait and would want it
too), or, narrowly for Pulse, signal from the same place that sets the flag.
`WaitDevice` already rechecks the flag on wake, so one extra signal is enough
and spurious wake-ups are harmless.
