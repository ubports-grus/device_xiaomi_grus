/*
 * Copyright (C) 2019 The LineageOS Project
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

#define LOG_TAG "FingerprintInscreenService"

#include "FingerprintInscreen.h"

#include <android-base/logging.h>
#include <fstream>
#include <cmath>

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 3
#define PARAM_NIT_NONE 0

#define DISPPARAM_PATH "/sys/class/drm/card0-DSI-1/disp_param"
#define DISPPARAM_FOD_BACKLIGHT_HBM "0x1D007FF"
#define DISPPARAM_FOD_BACKLIGHT_RESET "0x2D01000"

#define FOD_STATUS_PATH "/sys/devices/virtual/touch/tp_dev/fod_status"
#define FOD_STATUS_ON 1
#define FOD_STATUS_OFF 0

#define FOD_SENSOR_X 455
#define FOD_SENSOR_Y 1920
#define FOD_SENSOR_SIZE 173

#define BRIGHTNESS_PATH "/sys/class/backlight/panel0-backlight/brightness"

namespace {

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;
    file >> result;
    return file.fail() ? def : result;
}

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

} // anonymous namespace

namespace vendor {
namespace lineage {
namespace biometrics {
namespace fingerprint {
namespace inscreen {
namespace V1_0 {
namespace implementation {

/*  
    map the polynomial function here based on the discovered points

    ALPHA = 1.0 | BRIGHTNESS = 0
    ALPHA = 0.7 | BRIGHTNESS = 150
    ALPHA = 0.5 | BRIGHTNESS = 475
    ALPHA = 0.3 | BRIGHTNESS = 950
    ALPHA = 0.0 | BRIGHTNESS = 2047
*/

float p1 = 7.747 * pow(10, -8);
float p2 = -0.0004924;
float p3 = 0.6545;
float p4 = 58.82;
float q1 = 58.82;

FingerprintInscreen::FingerprintInscreen() {
    xiaomiFingerprintService = IXiaomiFingerprint::getService();
    this->mPressed = false;
}

Return<int32_t> FingerprintInscreen::getPositionX() {
    return FOD_SENSOR_X;
}

Return<int32_t> FingerprintInscreen::getPositionY() {
    return FOD_SENSOR_Y;
}

Return<int32_t> FingerprintInscreen::getSize() {
    return FOD_SENSOR_SIZE;
}

Return<void> FingerprintInscreen::onStartEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::onFinishEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::onPress() {
    if (!this->mPressed) {
        set(DISPPARAM_PATH, DISPPARAM_FOD_BACKLIGHT_HBM);
        xiaomiFingerprintService->extCmd(COMMAND_NIT, PARAM_NIT_FOD);
	this->mPressed = true;
    }
    return Void();
}

Return<void> FingerprintInscreen::onRelease() {
    if (this->mPressed) {
        set(DISPPARAM_PATH, DISPPARAM_FOD_BACKLIGHT_RESET);
        xiaomiFingerprintService->extCmd(COMMAND_NIT, PARAM_NIT_NONE);
	this->mPressed = false;
    }
    return Void();
}

Return<void> FingerprintInscreen::onShowFODView() {
    set(FOD_STATUS_PATH, FOD_STATUS_ON);
    return Void();
}

Return<void> FingerprintInscreen::onHideFODView() {
    xiaomiFingerprintService->extCmd(COMMAND_NIT, PARAM_NIT_NONE);
    set(DISPPARAM_PATH, DISPPARAM_FOD_BACKLIGHT_RESET);
    set(FOD_STATUS_PATH, FOD_STATUS_OFF);
    return Void();
}

Return<bool> FingerprintInscreen::handleAcquired(int32_t acquiredInfo, int32_t vendorCode) {
    LOG(ERROR) << "acquiredInfo: " << acquiredInfo << ", vendorCode: " << vendorCode << "\n";
    return false;
}

Return<bool> FingerprintInscreen::handleError(int32_t error, int32_t vendorCode) {
    LOG(ERROR) << "error: " << error << ", vendorCode: " << vendorCode << "\n";
    return false;
}

Return<void> FingerprintInscreen::setLongPressEnabled(bool) {
    return Void();
}

Return<int32_t> FingerprintInscreen::getDimAmount(int32_t /*brightness*/) {
    int realBrightness = get(BRIGHTNESS_PATH, 0);
    float alpha;

    alpha = (p1 * pow(realBrightness, 3) + p2 * pow(realBrightness, 2) + p3 * realBrightness + p4) / (realBrightness + q1);

    return 255 * alpha;
}

Return<bool> FingerprintInscreen::shouldBoostBrightness() {
    return false;
}

Return<void> FingerprintInscreen::setCallback(const sp<::vendor::lineage::biometrics::fingerprint::inscreen::V1_0::IFingerprintInscreenCallback>& callback) {
    (void) callback;
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace inscreen
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace lineage
}  // namespace vendor
