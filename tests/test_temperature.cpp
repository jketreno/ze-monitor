#include <catch2/catch_all.hpp>
#include "src/temperature.h"  // Update path if needed
#include <memory>
#include "ze_mock.h"

TEST_CASE("TemperatureMonitor initialization", "[temperature]") {
    // Create a dummy device handle
    zes_device_handle_t dummyDevice = reinterpret_cast<zes_device_handle_t>(123);
    
    SECTION("Successful initialization") {
        resetMocks();
        g_sensorCount = 3;
        
        REQUIRE_NOTHROW(TemperatureMonitor(dummyDevice));
        
        // Create monitor for further testing
        TemperatureMonitor monitor(dummyDevice);
        REQUIRE(monitor.getSensorCount() == 3);
    }
    
    SECTION("No sensors available") {
        resetMocks();
        g_sensorCount = 0;
        
        REQUIRE_NOTHROW(TemperatureMonitor(dummyDevice));
        
        TemperatureMonitor monitor(dummyDevice);
        REQUIRE(monitor.getSensorCount() == 0);
    }
    
    SECTION("Enumeration failure - first call") {
        resetMocks();
        g_enumSensorsResult = ZE_RESULT_ERROR_DEVICE_LOST;
        
        REQUIRE_THROWS_AS(TemperatureMonitor(dummyDevice), std::runtime_error);
    }
    
    SECTION("Enumeration failure - second call") {
        resetMocks();
        g_enumSensorsCalledOnce = true; // First call successful
        g_enumSensorsResult = ZE_RESULT_ERROR_DEVICE_LOST; // Second call fails
        
        REQUIRE_THROWS_AS(TemperatureMonitor(dummyDevice), std::runtime_error);
    }
}

TEST_CASE("TemperatureMonitor temperature updates", "[temperature]") {
    // Create a dummy device handle
    zes_device_handle_t dummyDevice = reinterpret_cast<zes_device_handle_t>(123);
    
    SECTION("Successful temperature update") {
        resetMocks();
        g_sensorCount = 3;
        g_mockTemperatures = {45.5, 52.8, 39.2};
        
        TemperatureMonitor monitor(dummyDevice);
        ze_result_t result = monitor.updateTemperatures();
        
        REQUIRE(result == ZE_RESULT_SUCCESS);
        REQUIRE(monitor.getSensorCount() == 3);
        REQUIRE(monitor.getTemperature(0) == Catch::Approx(45.5));
        REQUIRE(monitor.getTemperature(1) == Catch::Approx(52.8));
        REQUIRE(monitor.getTemperature(2) == Catch::Approx(39.2));
    }
    
    SECTION("Temperature update failure") {
        resetMocks();
        g_sensorCount = 3;
        g_getTempResult = ZE_RESULT_ERROR_DEVICE_LOST;
        
        TemperatureMonitor monitor(dummyDevice);
        ze_result_t result = monitor.updateTemperatures();
        
        REQUIRE(result == ZE_RESULT_ERROR_DEVICE_LOST);
    }
    
    SECTION("Partial temperature update failure") {
        resetMocks();
        g_sensorCount = 3;
        
        TemperatureMonitor monitor(dummyDevice);
        
        // Make the second sensor fail
        g_getTempResult = ZE_RESULT_SUCCESS;
        monitor.updateTemperatures();
        REQUIRE(monitor.getTemperature(0) == Catch::Approx(45.5));
        
        // Now make it fail on sensor 1
        auto tempFailure = [](zes_temp_handle_t hTemperature, double* pTemperature) {
            uintptr_t index = reinterpret_cast<uintptr_t>(hTemperature) - 1;
            if (index == 1) return ZE_RESULT_ERROR_DEVICE_LOST;
            
            *pTemperature = g_mockTemperatures[index];
            return ZE_RESULT_SUCCESS;
        };
        
        // Override the mock behavior
        g_getTempResult = ZE_RESULT_ERROR_DEVICE_LOST;
        
        ze_result_t result = monitor.updateTemperatures();
        REQUIRE(result == ZE_RESULT_ERROR_DEVICE_LOST);
    }
}

TEST_CASE("TemperatureMonitor edge cases", "[temperature]") {
    // Create a dummy device handle
    zes_device_handle_t dummyDevice = reinterpret_cast<zes_device_handle_t>(123);
    
    SECTION("Changing temperature values") {
        resetMocks();
        g_sensorCount = 2;
        g_mockTemperatures = {35.0, 40.0};
        
        TemperatureMonitor monitor(dummyDevice);
        
        // First update
        monitor.updateTemperatures();
        REQUIRE(monitor.getTemperature(0) == Catch::Approx(35.0));
        REQUIRE(monitor.getTemperature(1) == Catch::Approx(40.0));
        
        // Change temperatures and update again
        g_mockTemperatures = {38.5, 42.3};
        monitor.updateTemperatures();
        
        REQUIRE(monitor.getTemperature(0) == Catch::Approx(38.5));
        REQUIRE(monitor.getTemperature(1) == Catch::Approx(42.3));
    }
    
    SECTION("Extreme temperature values") {
        resetMocks();
        g_sensorCount = 2;
        g_mockTemperatures = {-10.5, 120.0}; // Extreme low and high
        
        TemperatureMonitor monitor(dummyDevice);
        monitor.updateTemperatures();
        
        REQUIRE(monitor.getTemperature(0) == Catch::Approx(-10.5));
        REQUIRE(monitor.getTemperature(1) == Catch::Approx(120.0));
    }
}