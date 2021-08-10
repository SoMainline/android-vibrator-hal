EVDEV-based Android Vibrator HAL
=

`IVibrator` HAL implementation for vibrators exposed through Linux [force feedback](https://www.kernel.org/doc/html/latest/input/ff.html) devices available on the [input event interface](https://www.kernel.org/doc/html/latest/input/input.html#evdev). Currently only supports on-off style vibration through the `FF_RUMBLE` effect, as provided by the likes of the [`gpio-vibrator`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/input/misc/gpio-vibra.c) kernel driver.

## Usage

Add the following to your Android device tree:

```Makefile
$(call inherit-product, the/path/to/vibrator.mk)
```

## Planned improvements

- Support for patterns and haptics-style feedback
- Support for higher HAL versions (1.3)
- Conversion to Rust in Android 12
- Caching and reuse of created effects
- Concurrency (can multiple patterns be started in parallel?)
