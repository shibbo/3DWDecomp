#pragma once

namespace al {
class HitSensor;
}  // namespace al

namespace npc {
bool isSensorBall(const al::HitSensor* pSensor);
bool isSensorBird(const al::HitSensor* pSensor);
bool isSensorNeko(const al::HitSensor* pSensor);
bool isSensorNekoDisaster(const al::HitSensor* pSensor);
}  // namespace npc
