#pragma once

class CharacterControllerHitReport : public PxUserControllerHitReport {
public:
    void onShapeHit(const PxControllerShapeHit& hit) override;
    void onControllerHit(const PxControllersHit& hit) override;
    void onObstacleHit(const PxControllerObstacleHit& hit) override;
};
