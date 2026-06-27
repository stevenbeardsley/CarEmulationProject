#include <gtest/gtest.h>

#include "Config.h"

#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace
{
    // Create a unique temp file path for each test
    fs::path MakeTempPath(const std::string& filename)
    {
        const auto dir = fs::temp_directory_path() / "config_gtests";
        fs::create_directories(dir);

        // Use test name + random-ish suffix
        const auto unique =
            std::string(::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name()) + "_" +
            std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()) + "_" +
            std::to_string(std::hash<std::string>{}(filename + std::to_string(::testing::UnitTest::GetInstance()->random_seed())));

        return dir / (unique + "_" + filename);
    }

    void WriteTextFile(const fs::path& p, const std::string& text)
    {
        std::ofstream out(p, std::ios::binary);
        ASSERT_TRUE(out.is_open()) << "Failed to open temp file for write: " << p.string();
        out << text;
        out.close();
        ASSERT_TRUE(out.good()) << "Failed writing temp file: " << p.string();
    }

    const char* kValidJson = R"json(
{
  "engine": {
    "id": "engine_1L",
    "displacement_l": 1,
    "idle_rpm": 850,
    "max_rpm": 6000,
    "max_torque_nm": 110
  },
  "transmission": {
    "id": "transmission_5spd",
    "gears": 5,
    "final_drive": 3.9,
    "gear_ratios": [3.91, 2.14, 1.36, 1.03, 0.84],
    "reverse_ratio": -3.54
  }
}
)json";

    const char* kInvalidJson = R"json(
{
  "engine": { "id": "engine_1L"  // missing closing braces etc...
)json";

    const char* kMissingFieldJson = R"json(
{
  "engine": {
    "id": "engine_1L",
    "displacement_l": 1,
    "idle_rpm": 850,
    "max_rpm": 6000,
    "max_torque_nm": 110
  },
  "transmission": {
    "id": "transmission_5spd",
    "gears": 5,
    "final_drive": 3.9,
    "gear_ratios": [3.91, 2.14, 1.36, 1.03, 0.84]
    // reverse_ratio missing
  }
}
)json";

    const char* kGearCountMismatchJson = R"json(
{
  "engine": {
    "id": "engine_1L",
    "displacement_l": 1,
    "idle_rpm": 850,
    "max_rpm": 6000,
    "max_torque_nm": 110
  },
  "transmission": {
    "id": "transmission_5spd",
    "gears": 6,
    "final_drive": 3.9,
    "gear_ratios": [3.91, 2.14, 1.36, 1.03, 0.84],
    "reverse_ratio": -3.54
  }
}
)json";
} // namespace

// -------------------- Happy path --------------------

TEST(ConfigTests, LoadFromFile_ParsesEngineAndTransmission)
{
    const fs::path p = MakeTempPath("config.json");
    WriteTextFile(p, kValidJson);

    shared::config::Config cfg;
    ASSERT_NO_THROW(cfg.LoadFromFile(p.string()));

    const auto& e = cfg.getEngineConfig();
    EXPECT_EQ(e.id, "engine_1L");
    EXPECT_DOUBLE_EQ(e.displacement_l, 1.0);
    EXPECT_EQ(e.idle_rpm, 850);
    EXPECT_EQ(e.max_rpm, 6000);
    EXPECT_DOUBLE_EQ(e.max_torque_nm, 110.0);

    const auto& t = cfg.getTransmissionConfig();
    EXPECT_EQ(t.m_id, "transmission_5spd");
    EXPECT_EQ(t.m_gears, 5);
    EXPECT_DOUBLE_EQ(t.m_finalDrive, 3.9);
    ASSERT_EQ(t.m_gearRatios.size(), 5u);
    EXPECT_DOUBLE_EQ(t.m_gearRatios[0], 3.91);
    EXPECT_DOUBLE_EQ(t.m_gearRatios[1], 2.14);
    EXPECT_DOUBLE_EQ(t.m_gearRatios[2], 1.36);
    EXPECT_DOUBLE_EQ(t.m_gearRatios[3], 1.03);
    EXPECT_DOUBLE_EQ(t.m_gearRatios[4], 0.84);
    EXPECT_DOUBLE_EQ(t.m_reverseRatio, -3.54);

    // cleanup
    std::error_code ec;
    fs::remove(p, ec);
}

// Ensures getters return references to the internal stored config (not copies)
TEST(ConfigTests, Getters_ReturnReferencesToInternalStorage)
{
    const fs::path p = MakeTempPath("config.json");
    WriteTextFile(p, kValidJson);

    shared::config::Config cfg;
    cfg.LoadFromFile(p.string());

    const auto* e1 = &cfg.getEngineConfig();
    const auto* e2 = &cfg.getEngineConfig();
    EXPECT_EQ(e1, e2);

    const auto* t1 = &cfg.getTransmissionConfig();
    const auto* t2 = &cfg.getTransmissionConfig();
    EXPECT_EQ(t1, t2);

    std::error_code ec;
    fs::remove(p, ec);
}

// -------------------- Error paths --------------------

TEST(ConfigTests, LoadFromFile_ThrowsIfFileMissing)
{
    const fs::path p = MakeTempPath("does_not_exist.json");

    shared::config::Config cfg;
    EXPECT_THROW(cfg.LoadFromFile(p.string()), std::runtime_error);
}

TEST(ConfigTests, LoadFromFile_ThrowsOnInvalidJson)
{
    const fs::path p = MakeTempPath("config.json");
    WriteTextFile(p, kInvalidJson);

    shared::config::Config cfg;

    // nlohmann::json throws parse_error which derives from std::exception (not std::runtime_error).
    EXPECT_THROW(cfg.LoadFromFile(p.string()), std::exception);

    std::error_code ec;
    fs::remove(p, ec);
}

TEST(ConfigTests, LoadFromFile_ThrowsOnMissingRequiredField)
{
    const fs::path p = MakeTempPath("config.json");
    WriteTextFile(p, kMissingFieldJson);

    shared::config::Config cfg;

    // j.at("reverse_ratio") will throw nlohmann::json::out_of_range (derives std::exception)
    EXPECT_THROW(cfg.LoadFromFile(p.string()), std::exception);

    std::error_code ec;
    fs::remove(p, ec);
}

TEST(ConfigTests, LoadFromFile_ThrowsOnGearRatiosCountMismatch)
{
    const fs::path p = MakeTempPath("config.json");
    WriteTextFile(p, kGearCountMismatchJson);

    shared::config::Config cfg;

    EXPECT_THROW(cfg.LoadFromFile(p.string()), std::runtime_error);

    std::error_code ec;
    fs::remove(p, ec);
}
