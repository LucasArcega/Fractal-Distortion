#pragma once

#include "TubeDistortion.h"
#include <JuceHeader.h>

namespace DSP {

    class DistortionEngine {
    public:
        enum class Type { Tube = 0, SoftClip, HardClip };

        void prepare(double sampleRate) noexcept { this->tube.prepare(sampleRate); }

        void reset() noexcept { this->tube.reset(); }

        void setType(Type newType) noexcept { this->type = newType; }

        void setDriveDb(float db) noexcept {

            // Evita recalcular caso o valor seja praticamente o mesmo (evita artefatos de áudio)
            if (std::abs(db - driveDb) < 0.001f)
                return;

            this->driveDb = db;
            this->driveGain = juce::Decibels::decibelsToGain(db);
            this->tube.setDrive(db);
        }

        void setBias(float newBias) noexcept { tube.setBias(newBias); }

        void setToneHz(float hz) noexcept { tube.setToneHz(hz); }

        float processSample(float input) noexcept {
            switch (type) {
            case Type::Tube:
                return tube.processSample(input);

            case Type::SoftClip:
                return std::tanh(input * driveGain);

            case Type::HardClip:
                return juce::jlimit(-1.0f, 1.0f, input * driveGain);

            default:
                return input;
            }
        }

    private:
        Type type = Type::Tube;

        float driveDb = 6.0f;
        float driveGain = 1.0f;

        TubeDistortion tube;
    };
} // namespace DSP