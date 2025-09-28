

#include "Resampler.h"

void Resampler::prepareToPlay(double inSourceSampleRate, int inMaxBlockSize, double inTargetSampleRate)
{
    mSourceSampleRate = inSourceSampleRate;
    // Protezione contro divisione per zero
    jassert(inTargetSampleRate > 0.0); // Solo in debug
    if (inTargetSampleRate <= 0.0)
        inTargetSampleRate = 44100.0; // Valore di fallback

    mTargetSampleRate = inTargetSampleRate;
    mSpeedRatio = mSourceSampleRate / mTargetSampleRate;

    mInternalBuffer.setSize(1, inMaxBlockSize + 2 * static_cast<int>(LagrangeInterpolator::getBaseLatency()) + 1);

    // Lowpass filter stuffs if downsamples
    if (mTargetSampleRate < mSourceSampleRate) {
        auto filter_coeffs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
            static_cast<float>(mTargetSampleRate / 2.0), mSourceSampleRate, 4);

        mLowpassFilters.resize(static_cast<size_t>(filter_coeffs.size()));

        for (size_t i = 0; i < mLowpassFilters.size(); i++) {
            mLowpassFilters[i].coefficients = filter_coeffs[(int) i];
        }
    } else {
        mLowpassFilters.clear();
    }

    reset();
}

void Resampler::reset()
{
    mInternalBuffer.clear();
    mNumInputSamplesAvailable = mInitPadding;
    mInterpolator.reset();

    for (auto& lowpass_filter: mLowpassFilters)
        lowpass_filter.reset();
}

int Resampler::processBlock(const float* inBuffer, float* outBuffer, int inNumSamples)
{
    jassert(mNumInputSamplesAvailable + inNumSamples <= mInternalBuffer.getNumSamples());

    mInternalBuffer.copyFrom(0, mNumInputSamplesAvailable, inBuffer, inNumSamples);
    float* internal_buffer_ptr = mInternalBuffer.getWritePointer(0);

    // Lowpass filter only if downsamples if necessary
    if (mTargetSampleRate < mSourceSampleRate) {
        for (int i = 0; i < inNumSamples; i++) {
            for (auto& lowpass_filter: mLowpassFilters) {
                internal_buffer_ptr[mNumInputSamplesAvailable + i] =
                    lowpass_filter.processSample(internal_buffer_ptr[mNumInputSamplesAvailable + i]);
            }
        }
    }

    mNumInputSamplesAvailable += inNumSamples;

    int num_out_samples_to_produce = static_cast<int>(std::floor(mNumInputSamplesAvailable / mSpeedRatio));

    int num_input_samples_used =
        mInterpolator.process(mSpeedRatio, mInternalBuffer.getReadPointer(0), outBuffer, num_out_samples_to_produce);

    jassert(num_input_samples_used <= mNumInputSamplesAvailable);

    mNumInputSamplesAvailable -= num_input_samples_used;

    // Substitute slow full cycle with speed copy
    mInternalBuffer.copyFrom(
    0,                              // canale 0
    0,                              // inizio destinazione
    mInternalBuffer,                // buffer sorgente
    0,                              // canale sorgente
    num_input_samples_used,         // inizio sorgente
    mNumInputSamplesAvailable       // quanti campioni copiare
);

    return num_out_samples_to_produce;
}

int Resampler::getNumOutSamplesOnNextProcessBlock(int inNumSamples) const
{
    return static_cast<int>(std::floor((mNumInputSamplesAvailable + inNumSamples) / mSpeedRatio));
}
