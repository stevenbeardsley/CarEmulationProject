#pragma once

namespace ecm::engine::models
{
    class ThermalModel
    {
    public:
        // Constructor sets up our "Option A" tuned parameters
        ThermalModel(double ambientC = 20.0,
            double heatRate = 4.5,
            double coolRate = 0.05,
            double overheatC = 105.0);

        void update(double rpm, double throttle, double idleRpm, double redlineRpm, double dtSeconds);

        double getCoolantTempC() const;
        bool isOverheating() const;

    private:
        static double clampd(double v, double lo, double hi);

        // State variables
        double m_tempC;
        bool m_overheating;

        // Tuned "Arcade" Constants
        double m_ambientC;
        double m_heatRate;  // Degrees C added per second at 100% load
        double m_coolRate;  // Percentage of heat delta lost per second
        double m_overheatC;
    };
}