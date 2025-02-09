#ifndef LAYER_H
#define LAYER_H

#include <sstream>
#include <string>

struct Layer {
    // CSV columns (example fields)
    std::string layerError;          // e.g., "SUCCESS"
    int         layerNumber;         // e.g., 1
    double      layerHeight;         // e.g., 0.2
    std::string materialType;        // e.g., "PLA"
    int         extrusionTemperature;// e.g., 210
    int         printSpeed;          // e.g., 50
    std::string layerAdhesionQuality;// e.g., "Good"
    int         infillDensity;       // e.g., 20
    std::string infillPattern;       // e.g., "Grid"
    int         shellThickness;      // e.g., 2
    int         overhangAngle;       // e.g., 45
    int         coolingFanSpeed;     // e.g., 50
    std::string retractionSettings;  // e.g., "5mm"
    double      zOffsetAdjustment;   // e.g., 0.05
    int         printBedTemperature; // e.g., 60
    std::string layerTime;           // e.g., "5min_12sec"
    std::string fileName;            // e.g., "fl_layer_200000.png"
    std::string imageUrl;            // e.g., "https://..."

    // Converts the layer data to a JSON-like string.
    std::string toString() const {
        std::stringstream ss;
        ss << "{ "
           << "\"layerError\": \"" << layerError << "\", "
           << "\"layerNumber\": " << layerNumber << ", "
           << "\"layerHeight\": " << layerHeight << ", "
           << "\"materialType\": \"" << materialType << "\", "
           << "\"extrusionTemperature\": " << extrusionTemperature << ", "
           << "\"printSpeed\": " << printSpeed << ", "
           << "\"layerAdhesionQuality\": \"" << layerAdhesionQuality << "\", "
           << "\"infillDensity\": " << infillDensity << ", "
           << "\"infillPattern\": \"" << infillPattern << "\", "
           << "\"shellThickness\": " << shellThickness << ", "
           << "\"overhangAngle\": " << overhangAngle << ", "
           << "\"coolingFanSpeed\": " << coolingFanSpeed << ", "
           << "\"retractionSettings\": \"" << retractionSettings << "\", "
           << "\"zOffsetAdjustment\": " << zOffsetAdjustment << ", "
           << "\"printBedTemperature\": " << printBedTemperature << ", "
           << "\"layerTime\": \"" << layerTime << "\", "
           << "\"fileName\": \"" << fileName << "\", "
           << "\"imageUrl\": \"" << imageUrl << "\" "
           << "}";
        return ss.str();
    }
};

#endif // LAYER_H
