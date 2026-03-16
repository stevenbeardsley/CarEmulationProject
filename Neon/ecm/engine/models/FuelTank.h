#ifndef ECM_ENGINE_MODELS_FUELTANK_H
#define ECM_ENGINE_MODELS_FUELTANK_H

namespace ecm::engine::models
{
class FuelTank
{
public:
    FuelTank() = default;
    void initialize(double displacementLitres);
    void update(double rpm,
        double throttle01,
        double idleRpm,
        double maxRpm,
        double dtSeconds);
	[[nodiscard]] bool isEmpty() const;
    [[nodiscard]] double getFuelLitres() const;
    [[nodiscard]] double getFuelPercent() const;
    [[nodiscard]] double getCapacityLitres() const;
    void addFuelLitres(double litres);

private:
    double m_displacementL{ 1.0 };
    double m_capacityL{ 50.0 };
    double m_fuelLevelL{ 50.0 };
};
}

#endif 