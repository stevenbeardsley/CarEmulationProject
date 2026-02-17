#ifndef ECM_ENGINE_MODELS_THERMAL_MODEL
#define ECM_ENGINE_MODELS_THERMAL_MODEL

namespace ecm::engine::models
{

class ThermalModel
{
    public:
        ThermalModel() = default;

        void update(double rpm,
            double throttle,
            double idleRpm,
            double redlineRpm,
            double dtSeconds);

        double getCoolantTempC() const;
        bool isOverheating() const;

    private:
        static double clampd(double v, double lo, double hi);

    private:
        double m_tempC{ 90.0 };
        double m_ambientC{ 20.0 };

        double m_overheatC{ 115.0 };
        bool   m_overheating{ false };

        double m_heatRate{ 35.0 };
        double m_coolRate{ 0.25 };
    };
}

#endif 
