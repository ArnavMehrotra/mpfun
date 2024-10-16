#include <iostream>
#include <vector>

#define BLOCK_SIZE 512.0f

std::vector<float> sanitySin(float frequency, float duration, int sampleRate, int numChannels) {
	int numSamples = static_cast<int>(duration * sampleRate);
	std::vector<float> wave(numSamples * numChannels);

	for (int i = 0; i < numSamples; i++) {
		float sample = sinf(2.0f * M_PI * frequency * i / sampleRate);
		for (int ch = 0; ch < numChannels; ch++) {
			wave[i * numChannels + ch] = sample;  // Same sample for all channels
		}
	}

	return wave;
}

std::vector<int> quantize(const std::vector<float> coeffs, const std::vector<float> scaleFactors) {
	std::vector<int> quantized(coeffs.size());
	for(int i = 0; i < coeffs.size(); i++) {
		quantized[i] = static_cast<int>(coeffs[i] / powf(2.0f, scaleFactors[i]));
	}

	return quantized;
}

std::vector<float> mdct(std::vector<float>& samples) {

	//zero pad samples to have a clean number of blocks of 512 samples
	size_t padding = samples.size() % static_cast<int>(BLOCK_SIZE);
	samples.insert(samples.end(), padding, 0.0f);

	std::vector<float> mdctOut(samples.size() / 2, 0.0f);
	
	//MDCT over Blocks of size 512
	for(int i = 0; i < samples.size() / BLOCK_SIZE; i++) {
		for(int j = 0; j < BLOCK_SIZE / 2; j++) {
			for(int k = 0; k < BLOCK_SIZE; k++) {
				//calculate partial mdct term and add to sum
				float cosTerm = (M_PI / BLOCK_SIZE) * (k + 0.5f + (BLOCK_SIZE / 2.0f)) * (j + 0.5f);
				mdctOut[(i * BLOCK_SIZE / 2) + j] += samples[(i * BLOCK_SIZE) + k] * cosf(cosTerm);
			}

			//scale MDCT coefficient to conserve energy (?)
			mdctOut[(i * BLOCK_SIZE / 2) + j] /= (sqrt(BLOCK_SIZE / 2));
		}
	}

	return mdctOut;
}

std::vector<float> sinWindow(const std::vector<float> samples) {
	std::vector<float> windowedSamples(samples.size());
	for(int i = 0; i < samples.size(); i++) {
		windowedSamples[i] = samples[i] * sinf(M_PI * i / samples.size());
	}

	return windowedSamples;
}

void filter(std::vector<float>& audio, int sampleRate, float cutoff) {
	//time constant
	float rc = 1.0f / (2.0f * cutoff * M_PI);

	//time between samples
	float dt = 1.0f / sampleRate;

	//alpha value
	float alpha = dt / (dt + rc);

	float prev = 0.0f;
	
	for(int i = 0; i < audio.size(); i++) {
		audio[i] = (alpha * audio[i]) + ((1.0f - alpha) * prev);
		prev = audio[i];
	}

}

void reverb(std::vector<float>& audio, int sampleRate, float delayTime, float decay) {
	int delaySamples = static_cast<int>(delayTime * sampleRate);
	std::vector<float> buff(audio.size(), 0.0f);

	for(int i = 0; i < audio.size(); i++) {
		buff[i] = audio[i];
		if(i >= delaySamples) {
			buff[i] += audio[i - delaySamples] * decay;
		}
	}

	for(int i = 0; i < audio.size(); i++) {
		audio[i] += buff[i];
	}
}

void chorus(std::vector<float>& audio, int sampleRate, float depth, float rate, float delay, float wet) {
	int delaySamples = static_cast<int>(delay * sampleRate);
	std::vector<float> buff(audio.size(), 0.0f);

	float lfoPhase = 0.0f;
	float lfoStep = (2.0f * M_PI * rate) / sampleRate;

	for(int i = 0; i < audio.size(); i++) {
		int modDelay = static_cast<int>(delaySamples + (depth * delaySamples) + sinf(lfoPhase));
		if(i >= modDelay) {
			float wetSignal = audio[i - modDelay];
			buff[i] = ((1.0f - wet) * audio[i]) + (wet * wetSignal); 

		}
		else {
			buff[i] = audio[i];
		}

		lfoPhase += lfoStep;
		if(lfoPhase > 2.0f * M_PI) lfoPhase -= (2.0f * M_PI);
	}

	audio = buff;

}
