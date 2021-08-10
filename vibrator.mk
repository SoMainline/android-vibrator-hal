PRODUCT_PACKAGES += \
	android.hardware.vibrator@1.0-service.evdev

# TODO: LOCAL_PATH doesn't work here
BOARD_VENDOR_SEPOLICY_DIRS += device/mainline/common/vibrator/sepolicy
