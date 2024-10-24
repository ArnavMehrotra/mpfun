#include <iostream>
#include <vector>
#include <chrono>

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
		quantized[i] = roundf(coeffs[i]) / 2.0f;
	}

	return quantized;
}

void scaleByConstant(std::vector<float>& samples, float c) {
	for(int i = 0; i < samples.size(); i++) {
		samples[i] *= c;
	}
}

void sinWindow(std::vector<float>& samples, int start) {
	for(int i = 0; i < BLOCK_SIZE; i++) {
		samples[i + start] *= sinf(M_PI * i / BLOCK_SIZE);
	}
}

void hannWindow(std::vector<float>& samples, int start) {
	for(int i = 0; i < BLOCK_SIZE; i++) {
		samples[i + start] *= 0.5f * (1 - cosf(2 * M_PI * i / BLOCK_SIZE));
	}
}

void hammingWindow(std::vector<float>& samples, int start) {
	for(int i = 0; i < BLOCK_SIZE; i++) {
		samples[i + start] *= (0.54f - (0.46f * cosf(2 * M_PI * i / BLOCK_SIZE)));
	}
}


void blackmanWindow(std::vector<float>& samples, int start) {
	for(int i = 0; i < BLOCK_SIZE; i++) {
		samples[i + start] *= (0.54f - (0.46f * cosf(2 * M_PI * i / BLOCK_SIZE)) + (0.08f * cosf(4 * M_PI * i / BLOCK_SIZE)));
	}
}


//TODO why not inversable?
std::vector<float> mdct(std::vector<float>& samples) {
	//zero pad samples to have a clean number of blocks of 512 samples
	size_t padding = samples.size() % static_cast<int>(BLOCK_SIZE);
	if(padding) {
		samples.insert(samples.end(), BLOCK_SIZE - padding, 0.0f);
	}
	
	std::vector<float> mdctOut(samples.size() / 2, 0.0f);
	
	//MDCT over Blocks of size 512

	const float scaleFactor = sqrt(BLOCK_SIZE / 2);

	for(int i = 0; i < samples.size() / BLOCK_SIZE; i++) {
		//pick your favorite window function

		//sinWindow(samples, i * BLOCK_SIZE);
		//blackmanWindow(samples, i * BLOCK_SIZE);
		//hannWindow(samples, i * BLOCK_SIZE);

		// hammingWindow(samples, i * BLOCK_SIZE);

		//for every sample in the block, iterate over every other sample in the block
		for(int j = 0; j < BLOCK_SIZE / 2; j++) {
			for(int k = 0; k < BLOCK_SIZE; k++) {
				//calculate partial mdct term and add to sum
				float cosTerm = (M_PI / BLOCK_SIZE) * (k + 0.5f + (BLOCK_SIZE / 2.0f)) * (j + 0.5f);
				mdctOut[(i * BLOCK_SIZE / 2) + j] += samples[(i * BLOCK_SIZE) + k] * cosf(cosTerm);
			}

			//scale MDCT coefficient to conserve energy (?)
			mdctOut[(i * BLOCK_SIZE / 2) + j] /= scaleFactor;
		}
	}

	printf("applied mdct to %zu samples\n", samples.size());

	return mdctOut;
}

//TODO make match up with mdct!
std::vector<float> imdct(std::vector<float>& samples) {
	size_t padding = samples.size() % static_cast<int>(BLOCK_SIZE);
	if(padding) {
		samples.insert(samples.end(), BLOCK_SIZE - padding, 0.0f);
	}

	std::vector<float> imdctOut(samples.size() * 2);


	const float scaleFactor = sqrt(BLOCK_SIZE / 2);

	for(int i = 0; i < samples.size() / BLOCK_SIZE; i++) {
		
		//for every mdct coefficient in the block, iterate over every other coefficient in the block
		for(int j = 0; j < BLOCK_SIZE; j++) {
			for(int k = 0; k < BLOCK_SIZE;k++) {	
				float pcm = samples[(i * BLOCK_SIZE) + k] / scaleFactor;
				for(int l = 0; l < 2; l++) {
					float cosTerm = (M_PI / BLOCK_SIZE) * (k + (BLOCK_SIZE / 2) + 0.5f) * (k + 0.5f);
					imdctOut[(i * BLOCK_SIZE * 2) + j + l] += pcm * cosf(cosTerm) * (2 / BLOCK_SIZE);
				}
			}

		}
	}

	return imdctOut;
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

//TODO figure out why this doesn't work (well) with certain wet values
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
