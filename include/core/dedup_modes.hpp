#pragma once

#include <string>

/**
 * @brief Deduplication modes for different speed/accuracy trade-offs
 *
 * This file documents the three deduplication modes and their library requirements.
 * The actual implementation will be added later.
 */

enum class DedupMode
{
    FAST,     // Speed optimized - dHash + FFmpeg
    BALANCED, // Balanced speed/accuracy - pHash + libvips + FFmpeg
    QUALITY   // Quality optimized - CNN embeddings + FFmpeg
};

class DedupModes
{
public:
    /**
     * @brief Get the library stack for a specific mode
     * @param mode The deduplication mode
     * @return String description of the library stack
     */
    static std::string getLibraryStack(DedupMode mode)
    {
        switch (mode)
        {
        case DedupMode::FAST:
            return "OpenCV (dHash) + FFmpeg";
        case DedupMode::BALANCED:
            return "libvips + OpenCV (pHash) + FFmpeg";
        case DedupMode::QUALITY:
            return "ONNX Runtime + CNN Embeddings (ResNet/CLIP) + FFmpeg";
        default:
            return "Unknown mode";
        }
    }

    /**
     * @brief Get the reason for choosing a specific mode
     * @param mode The deduplication mode
     * @return String description of the mode's characteristics
     */
    static std::string getModeDescription(DedupMode mode)
    {
        switch (mode)
        {
        case DedupMode::FAST:
            return "Fast scanning, acceptable quality, low resource use";
        case DedupMode::BALANCED:
            return "Good balance of speed and accuracy";
        case DedupMode::QUALITY:
            return "Highest accuracy, computationally intensive (GPU recommended)";
        default:
            return "Unknown mode";
        }
    }

    /**
     * @brief Get the mode name as string
     * @param mode The deduplication mode
     * @return String representation of the mode
     */
    static std::string getModeName(DedupMode mode)
    {
        switch (mode)
        {
        case DedupMode::FAST:
            return "FAST";
        case DedupMode::BALANCED:
            return "BALANCED";
        case DedupMode::QUALITY:
            return "QUALITY";
        default:
            return "UNKNOWN";
        }
    }

    /**
     * @brief Convert string to DedupMode enum
     * @param mode_str String representation of the mode
     * @return DedupMode enum value
     */
    static DedupMode fromString(const std::string &mode_str)
    {
        if (mode_str == "FAST" || mode_str == "fast")
            return DedupMode::FAST;
        else if (mode_str == "BALANCED" || mode_str == "balanced")
            return DedupMode::BALANCED;
        else if (mode_str == "QUALITY" || mode_str == "quality")
            return DedupMode::QUALITY;
        else
            return DedupMode::BALANCED; // Default to balanced
    }
};

// TODO: IMPLEMENTATION NOTES
//
// FAST MODE Implementation:
// - Images: Use OpenCV dHash for fast perceptual hashing
// - Videos: Extract key frames with FFmpeg, then apply dHash
// - Libraries: OpenCV, FFmpeg
//
// BALANCED MODE Implementation:
// - Images: Use libvips for thumbnail generation + OpenCV pHash
// - Videos: Extract key frames with FFmpeg, then apply pHash
// - Libraries: libvips, OpenCV, FFmpeg
//
// QUALITY MODE Implementation:
// - Images: Use CNN embeddings (ResNet/CLIP) via ONNX Runtime
// - Videos: Extract key frames with FFmpeg, then apply CNN embeddings
// - Libraries: ONNX Runtime, FFmpeg, CNN models (ResNet/CLIP)
//
// All modes will use the same file scanning infrastructure from FileUtils
// and will integrate with the existing API endpoints.