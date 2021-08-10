/*
 * Copyright (C) 2021 Marijn Suijten
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.vibrator@1.0-service.evdev"

#include <android/hardware/vibrator/1.0/IVibrator.h>
#include <hidl/HidlTransportSupport.h>

#include <linux/input.h>
#include <log/log.h>

// using namespace android::hardware;
using android::NO_ERROR;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::Return;
using android::hardware::Void;
using namespace android::hardware::vibrator::V1_0;

struct Vibrator : public IVibrator {
    Vibrator()
    {
        int ret;

        std::string dev_input("/dev/input/");

        DIR *directory = opendir(dev_input.c_str());
        if (!directory) {
            ALOGE("Failed to open %s: %s", dev_input.c_str(), strerror(errno));
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(directory))) {
            auto target = dev_input + entry->d_name;
            if (entry->d_type != DT_CHR)
                continue;

            fd = open(target.c_str(), O_RDWR);
            if (fd < 0) {
                ALOGE("Failed to open %s: %s", target.c_str(), strerror(errno));
                continue;
            }

            unsigned char features[FF_RUMBLE / 8 + 1];
            ret = ioctl(fd, EVIOCGBIT(EV_FF, sizeof(features)), &features);
            if (ret < 0) {
                ALOGE("EVIOCGBIT failed: %s", strerror(ret));
                close(fd);
                continue;
            }
            if (ret != sizeof(features)) {
                ALOGE("EVIOCGBIT did not retrieve all feature bits");
                close(fd);
                continue;
            }

            if (!(features[FF_RUMBLE / 8] & (1 << (FF_RUMBLE % 8)))) {
                close(fd);
                continue;
            }

            ALOGI("Using %s with FF_RUMBLE support", target.c_str());
            closedir(directory);

            return;
        }

        ALOGW("Failed to find an input device with FF_RUMBLE support");
        closedir(directory);
        exit(1);
    }

    Return<Status> playEffect(bool on)
    {
        if (effect_id == -1)
            return Status::BAD_VALUE;

        struct input_event ie;
        ie.type = EV_FF;
        ie.code = effect_id;
        ie.value = on;

        ALOGV("%s effect %d", on ? "Playing" : "Stopping", effect_id);

        if (write(fd, &ie, sizeof(ie)) != sizeof(ie)) {
            ALOGE("Failed to turn %s", on ? "on" : "off");
            return Status::UNKNOWN_ERROR;
        }

        return Status::OK;
    }

    // Methods from ::android::hardware::vibrator::V1_0::IVibrator follow.

    Return<Status> on(uint32_t timeoutMs) override
    {
        /* Initially effect_id=-1, but after that we re-use and update
         * the same effect with a possibly different duration */
        // TODO: Can possibly cache multiple effects, or at least
        // store the timeout of the last stored effect.
        struct ff_effect effect {
            .type = FF_RUMBLE,
            .id = (int16_t)effect_id,
            .u.rumble.strong_magnitude = 1,
            .replay.length = (uint16_t)timeoutMs,
        };

        ALOGV("Enabling for %dms", timeoutMs);

        int ret = ioctl(fd, EVIOCSFF, &effect);

        if (ret < 0) {
            ALOGE("Failed to write ff_effect: %s", strerror(ret));
            return Status::UNKNOWN_ERROR;
        }

        effect_id = effect.id;

        return playEffect(true);
    }

    Return<Status> off() override
    {
        return playEffect(false);
    }

    Return<bool> supportsAmplitudeControl() override
    {
        return false;
    }

    Return<Status> setAmplitude(uint8_t /* amplitude */) override
    {
        return Status::UNSUPPORTED_OPERATION;
    }

    Return<void> perform(Effect /* effect */, EffectStrength /* strength */, perform_cb _hidl_cb) override
    {
        _hidl_cb(Status::UNSUPPORTED_OPERATION, 0);
        return Void();
    }

private:
    int fd = -1;
    uint16_t effect_id = -1;
};

int main()
{
    configureRpcThreadpool(1, true /*callerWillJoin*/);
    auto service = new Vibrator();
    status_t status = service->registerAsService();
    if (status != NO_ERROR) {
        ALOGE("Cannot start vibrator service: %d", status);
        return 1;
    }
    joinRpcThreadpool();
    return 0;
}
