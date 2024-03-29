#ifndef APU_H
#define APU_H

#include <SDL.h>
#include "types.h"
#include <mutex>

// APU code taken from https://github.com/Dooskington/GameLad/pull/111
class APU
{
public:
    APU();
    ~APU();

    void update(unsigned int cycles);
    void AudioDeviceCallback(Uint8* pStream, int length);

    // IMemoryUnit
    byte readByte(const word address);
    bool writeByte(const word address, const byte val);
    
    void setSoundDisabled(const bool soundDisabled);
    bool isSoundDisabled() const;
    
private:
    void LoadAudioDevice(SDL_AudioCallback callback);

    typedef enum
    {
        CHANNEL_1,
        CHANNEL_2,
        CHANNEL_3,
        CHANNEL_4
    } APUChannel;

    // Byte ring buffer for streaming audio data
    class Buffer
    {
    public:
        Buffer(size_t element_count, size_t element_size);
        ~Buffer();

        void Reset();
        void Put(Uint8* element);
        Uint8* Get();

    private:
        size_t m_ElementCount;
        size_t m_ElementSize;
        size_t m_BufferSize;
        Uint8* m_Bytes;
        Uint8* m_DefaultBytes;
        size_t m_ReadIndex;
        size_t m_WriteIndex;
        std::mutex mutex;

        void AdvanceReadIndex();
        void AdvanceWriteIndex();
    };

    // Base class for sound channel sample generators
    class SoundGenerator
    {
    public:
        SoundGenerator(
            const APUChannel channel,
            const byte* soundOnOffRegister
        );
        virtual ~SoundGenerator() = default;
        float NextSample();

    protected:
        const APUChannel m_Channel;
        const byte* m_SoundOnOffRegister;

        bool m_Enabled = true;
        double m_FrequencyHz = 1.0;
        double m_Phase = 0.0;
        bool m_CounterModeEnabled = false;
        double m_SoundLengthSeconds;
        bool m_SweepModeEnabled = false;
        double m_SweepDirection;
        int m_SweepShiftFrequencyExponent;
        double m_SweepStepLengthSeconds;
        bool m_EnvelopeModeEnabled = false;
        double m_EnvelopeDirection;
        double m_EnvelopeStartVolume = 1.0;
        double m_EnvelopeStepLengthSeconds;
        double m_SoundLengthTimerSeconds = 0.0;
        bool m_SoundLengthExpired = false;
        unsigned int m_FrequencyRegisterData = 0;

        virtual float NextWaveformSample() = 0;
        virtual void UpdateFrequency(unsigned int freqencyRegValue) = 0;

        void RestartSound();
        void SetSoundOnOffFlag();
        void ResetSoundOnOffFlag();
    };

    class SquareWaveGenerator : public SoundGenerator
    {
    public:
        SquareWaveGenerator(
            const APUChannel channel,
            const byte* soundOnOffRegister,
            const byte* sweepRegister,
            const byte* soundLengthRegister,
            const byte* volumeEnvelopeRegister,
            const byte* frequencyLoRegister,
            const byte* frequencyHiRegister
        );
        ~SquareWaveGenerator() = default;

        void TriggerSweepRegisterUpdate();
        void TriggerSoundLengthRegisterUpdate();
        void TriggerVolumeEnvelopeRegisterUpdate();
        void TriggerFrequencyLoRegisterUpdate();
        void TriggerFrequencyHiRegisterUpdate();

    private:
        // Maximum number of harmonics used to generate the square wave
        static const int MaxHarmonicsCount = 52;

        const byte* m_SweepRegister;
        const byte* m_SoundLengthRegister;
        const byte* m_VolumeEnvelopeRegister;
        const byte* m_FrequencyLoRegister;
        const byte* m_FrequencyHiRegister;

        double m_DutyCycle = 0.5;
        int m_HarmonicsCount = 0;
        double m_Coefficients[MaxHarmonicsCount];

        float NextWaveformSample() override;
        void UpdateFrequency(unsigned int freqencyRegValue) override;

        void RegenerateCoefficients();
    };

    class NoiseGenerator : public SoundGenerator
    {
    public:
        NoiseGenerator(
            const APUChannel channel,
            const byte* soundOnOffRegister,
            const byte* soundLengthRegister,
            const byte* volumeEnvelopeRegister,
            const byte* polynomialCounterRegister,
            const byte* counterRegister
        );
        ~NoiseGenerator() = default;

        void TriggerSoundLengthRegisterUpdate();
        void TriggerVolumeEnvelopeRegisterUpdate();
        void TriggerPolynomialCounterRegisterUpdate();
        void TriggerCounterRegisterUpdate();

    private:
        const byte* m_SoundLengthRegister;
        const byte* m_VolumeEnvelopeRegister;
        const byte* m_PolynomialCounterRegister;
        const byte* m_CounterRegister;

        float m_Signal = 0.5;
        double m_PreviousSamplePhase = 0.0;

        unsigned int m_shiftRegisterMSB = 14;
        unsigned int m_shiftRegister = 0xFF;

        float NextWaveformSample() override;
        void UpdateFrequency(unsigned int freqencyRegValue) override;
    };

    class WaveformGenerator : public SoundGenerator
    {
    public:
        WaveformGenerator(
            const APUChannel channel,
            const byte* soundOnOffRegister,
            const byte* channelSoundOnOffRegister,
            const byte* soundLengthRegister,
            const byte* selectOutputLevelRegister,
            const byte* frequencyLoRegister,
            const byte* frequencyHiRegister,
            const byte* waveBuffer
        );
        ~WaveformGenerator() = default;

        void TriggerChannelSoundOnOffRegisterUpdate();
        void TriggerSoundLengthRegisterUpdate();
        void TriggerSelectOutputLevelRegisterUpdate();
        void TriggerFrequencyLoRegisterUpdate();
        void TriggerFrequencyHiRegisterUpdate();

    private:
        const byte* m_ChannelSoundOnOffRegister;
        const byte* m_SoundLengthRegister;
        const byte* m_SelectOutputLevelRegister;
        const byte* m_FrequencyLoRegister;
        const byte* m_FrequencyHiRegister;
        const byte* m_WaveBuffer;

        byte m_VolumeShift = 0;

        float NextWaveformSample() override;
        void UpdateFrequency(unsigned int freqencyRegValue) override;
    };

private:
    // Audio Registers
    byte m_Channel1Sweep;
    byte m_Channel1SoundLength;
    byte m_Channel1VolumeEnvelope;
    byte m_Channel1FrequencyLo;
    byte m_Channel1FrequencyHi;

    byte m_Channel2SoundLength;
    byte m_Channel2VolumeEnvelope;
    byte m_Channel2FrequencyLo;
    byte m_Channel2FrequencyHi;

    byte m_Channel3SoundOnOff;
    byte m_Channel3SoundLength;
    byte m_Channel3SelectOutputLevel;
    byte m_Channel3FrequencyLo;
    byte m_Channel3FrequencyHi;
    byte m_WavePatternRAM[0x0F + 1];

    byte m_Channel4SoundLength;
    byte m_Channel4VolumeEnvelope;
    byte m_Channel4PolynomialCounter;
    byte m_Channel4Counter;

    byte m_ChannelControlOnOffVolume;
    byte m_OutputTerminal;

    byte m_SoundOnOff;

    // Synthesis
    SquareWaveGenerator m_Channel1SoundGenerator;
    SquareWaveGenerator m_Channel2SoundGenerator;
    WaveformGenerator m_Channel3SoundGenerator;
    NoiseGenerator m_Channel4SoundGenerator;

    // Output
    SDL_AudioDeviceID m_AudioDevice;
    double m_AudioFrameRemainder;
    Buffer m_OutputBuffer;
    bool m_Initialized;
    bool m_SoundDisabled;
};

#endif
