#include <iostream>
#include <vector>
#include <chrono>


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

template<typename T>
void sinWindow(std::vector<T>& samples) {
	int N = samples.size();
	for(int i = 0; i < N; i++) {
		samples[i] *= static_cast<T>(sin(M_PI * i / N));
	}
}

void hannWindow(std::vector<float>& samples) {
	int N = samples.size();
	for(int i = 0; i < N; i++) {
		samples[i] *= 0.5f * (1 - cosf(2 * M_PI * i / N));
	}
}

void hammingWindow(std::vector<float>& samples) {
	int N = samples.size();
	for(int i = 0; i < N; i++) {
		samples[i] *= (0.54f - (0.46f * cosf(2 * M_PI * i / N)));
	}
}


void blackmanWindow(std::vector<float>& samples) {
	int N = samples.size();
	for(int i = 0; i < N; i++) {
		samples[i] *= (0.54f - (0.46f * cosf(2 * M_PI * i / N)) + (0.08f * cosf(4 * M_PI * i / N)));
	}
}


//TODO why not inversable?
std::vector<float> mdct(std::vector<float> samples) {
	//zero pad samples
	if(samples.size() % 2) samples.push_back(0.0f);

	const float N = samples.size();
	const float piOverN = M_PI / N;
	const float halfN = N / 2;

	std::vector<float> mdctOut(N / 2, 0.0f);

	hammingWindow(samples);

	for(int k = 0; k < N / 2; k++) {
		for(int n = 0; n < N; n++) {
			float cosTerm = piOverN * (n + 0.5f + halfN) * (k + 0.5f);
			mdctOut[k] += samples[n] * cosf(cosTerm);	
		}

		mdctOut[k] /= 1.24f;
	}

	printf("applied mdct to %.2f samples\n", N);

	return mdctOut;
}

//TODO make match up with mdct!
std::vector<float> imdct(std::vector<float> samples) {

	const float N = samples.size();
	const float halfN = N / 2;
	const float piOverN = M_PI / N;

	std::vector<float> imdctOut(N * 2, 0.0f);

	for(int n = 0; n < N * 2; n++) {
		for(int k = 0; k < N; k++) {
			float cosTerm = piOverN * (n + 0.5f + halfN) * (k + 0.5f);
			imdctOut[n] += samples[k] * cosf(cosTerm);
		}

		imdctOut[n] /= N;
	}

	hammingWindow(imdctOut);

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
