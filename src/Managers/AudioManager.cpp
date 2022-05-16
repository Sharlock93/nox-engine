/* ======================================================================================== */
/* AudioManager.cpp																			*/
/*                                                                                          */
/* A basic audio engine that uses FMOD as the middleware/backend - essentially              */
/* a wrapper for simpler access/modification to sounds in the game engine.                  */
/*                                                                                          */
/* This is built upon Cody Claborn's tutorial from											*/
/* https://codyclaborn.me/tutorials/making-a-basic-fmod-audio-engine-in-c/ .                */
/* License: MIT License.                                                                    */
/*                                                                                          */
/* DSP effects and ChannelGroups are introduced in Guy Somberg's book:			            */
/* Game Audio Programming, Chapter 6.                                                       */
/*                                                                                          */
/* Geometry occlusion (audio ray casting) is introduced in Guy Somberg's book:              */
/* Game Audio Programming 2, Chapter 8.                                                     */
/* More information can be found in Chapter 17 and 18 (for further development).            */
/*                                                                                          */
/* Some low-level desirable functions:														*/
/* https://github.com/Flix01/imgui/blob/imgui_with_addons/addons/imguiyesaddons/imguisoloud.h */
/*                                                                                          */
/* @author: Chan Wai Lou															        */
/* ======================================================================================== */

#include <Managers/AudioManager.h>
#include <Managers/GameManager.h>

#include <iostream>
#include <fmod_common.h>
#include <fmod_errors.h>

#include <Core/Mesh.h>
#include <Core/Entity.h>

#include "Components/AudioSourceComponent.h"
#include "Components/AudioGeometryComponent.h"
#include "Components/TransformComponent.h"

#include <Utils/Utils.h>

using namespace NoxEngine;
using namespace NoxEngineUtils;


// Constant values for audio types
std::map<DSP_TYPE, const char*> kDSPNamesMap = {
	{ Distortion,	"Distortion" },
	{ Echo,			"Echo" },
	{ Fader,		"Fader (Volume Control)" },
	{ Flange,		"Flange" },
	{ Limiter,		"Limiter" },
	{ PitchShift,	"Pitch Shift" },
	{ Tremolo,		"Tremolo" }
};

// Conversion from supported DSP type to FMOD internal DSP type
std::map<DSP_TYPE, FMOD_DSP_TYPE> kDSPTypeMap = {
	{ Distortion,	FMOD_DSP_TYPE_DISTORTION },
	{ Echo,			FMOD_DSP_TYPE_ECHO },
	{ Fader,		FMOD_DSP_TYPE_FADER },
	{ Flange,		FMOD_DSP_TYPE_FLANGE },
	{ Limiter,		FMOD_DSP_TYPE_LIMITER },
	{ PitchShift,	FMOD_DSP_TYPE_PITCHSHIFT },
	{ Tremolo,		FMOD_DSP_TYPE_TREMOLO }
};


/////////////////////
//   CTOR / DTOR   //
/////////////////////

AudioManager::AudioManager() :
	mnNextChannelId(0),
	nextDSPId(0),
	coreSystem(nullptr),
	studioSystem(nullptr) {

}

AudioManager::~AudioManager() {

}



////////////////////////////
/*   Core API Functions   */
////////////////////////////

void AudioManager::init(float maxChannels, float maxWorldSize, FMOD_INITFLAGS flags) {

	// We're using OpenGL, so it's always right handed
	flags |= FMOD_INIT_3D_RIGHTHANDED;

	// Create the main system object & Initialize
	ERRCHK( FMOD::System_Create(&coreSystem) );
	ERRCHK( coreSystem->init(maxChannels, flags, 0) );

	// TODO: Initialize studio system
	// can try FMOD_STUDIO_INIT_LIVEUPDATE

	// Various settings
	coreSystem->setGeometrySettings(maxWorldSize);
	//coreSystem->set3DSettings();
	//coreSystem->setAdvancedSettings();
}


void AudioManager::destroy() {

	// Unload all sounds
	for (auto it = mSounds.begin(), itEnd = mSounds.end(); it != itEnd; it++) {
		const int nameLen = 64;
		char name[nameLen];

		it->second->getName(name, nameLen);

		LOG_DEBUG("Unloading sound %s...", name);
		unLoadSound(name);
		LOG_DEBUG(" - Finished.", name);
	}

	// Free all geometries
	for (int i = 0; i < geometries.size(); i++) {
		geometries[i]->release();
	}

	// Shutdown FMOD
	coreSystem->release();
}


void AudioManager::update() {

	// State of geometries (e.g. enable, transform, etc) is updated in GameManager

	/*   If a sound is stopped, remove it from the SoundMap so that it can be played again   */
	//std::vector<SoundChannelIdMap::iterator> pStoppedSoundChannelIds;
	//for (auto it = mSoundChannelIds.begin(), itEnd = mSoundChannelIds.end(); it != itEnd; ++it) {

	//	int channelId = it->second;
	//	bool bIsPlaying = false;	 mChannels[channelId]->isPlaying(&bIsPlaying);

	//	if (!bIsPlaying) {
	//		pStoppedSoundChannelIds.push_back(it);
	//	}
	//}

	//for (auto& it : pStoppedSoundChannelIds) {
	//	mSoundChannelIds.erase(it);
	//}


	/*   Mark stopped channels for removal   */
	std::vector<ChannelMap::iterator> pStoppedChannels;
	for (auto it = mChannels.begin(), itEnd = mChannels.end(); it != itEnd; ++it) {

		bool bIsPlaying = false;	it->second->isPlaying(&bIsPlaying);
		if (!bIsPlaying) {
			pStoppedChannels.push_back(it);
		}

		// TODO: Handle occlusion ourselves
		//float fDirectOcclusion = 0.0f, fReverbOcclusion = 0.0f;
		//coreSystem->getGeometryOcclusion(
		//	mListenerPosition,
		//	source.Position,
		//	&fDirectOcclusion,
		//	&fReverbOcclusion);
		//it->second->set3DOcclusion(fDirectOcclusion, fReverbOcclusion);
	}

	for (auto& it : pStoppedChannels) {
		int nDSPs = 0;		 
		it->second->getNumDSPs(&nDSPs);

		for (int i = 0; i < nDSPs; i++) {
			FMOD::DSP* dsp = nullptr;	
			it->second->getDSP(i, &dsp);
			it->second->removeDSP(dsp);
		}
		mChannels.erase(it);
	}

	// fmod needs to be updated at least once per game tick
	ERRCHK( coreSystem->update() );
}


void AudioManager::loadSound(const IAudioSource* src) {

	loadSound(src->filePath, src->is3D, src->isLooping, src->isStream);
}


void AudioManager::loadSound(const String& fileName, bool is3d, bool isLooping, bool isStream) {

	// first check that this sound hasn't been loaded into the sound map
	auto soundItr = this->mSounds.find(fileName);
	if (soundItr != mSounds.end()) return;

	// create a mode object based on the flag parameters
	FMOD_MODE eMode = FMOD_DEFAULT;
	eMode |= is3d		? FMOD_3D : FMOD_2D;								// 2D / 3D sound
	eMode |= isLooping	? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;					// Loop or not

	// Stream: To account for slow media that might cause buffer underrun 
	// (skipping / stuttering / repeating blocks of audio) with sounds 
	// created with FMOD_CREATESTREAM, use System::setStreamBufferSize to 
	// increase read ahead.
	eMode |= isStream	? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;	// Stream / load compressed sound into memory

	// more flags: FMOD_NONBLOCKING, FMOD_OPENMEMORY_POINT

	/*   create the sound and store it in the sound map   */
	FMOD::Sound* sound = nullptr;					// sound object
	FMOD_CREATESOUNDEXINFO* exinfo = nullptr;		// extended info
	ERRCHK( coreSystem->createSound(fileName.c_str(), eMode, exinfo, &sound) );
	if (sound) {
		mSounds[fileName] = sound;
	}
}


void AudioManager::unLoadSound(const String& strSoundName) {

	unLoadSound(strSoundName.c_str());
}


void AudioManager::unLoadSound(const char *strSoundName) {

	// If the specified sound isn't loaded, don't do anything
	auto sound = this->mSounds.find(strSoundName);
	if (sound == mSounds.end()) return;

	// free the memory occupied by the sound object
	ERRCHK(sound->second->release());
	mSounds.erase(sound);
}


int AudioManager::playSounds(const IAudioSource* src, const vec3 &pos) {
	return playSounds(src->filePath, pos, src->volume, src->dspChain);
}

int AudioManager::playSounds(const String& strSoundName, const vec3& vPos, float fVolumedB, const Array<int> &dspChain) {

	// Has this sound been loaded?
	auto sound = mSounds.find(strSoundName);

	// Sound hasn't been loaded, so load it
	if (sound == mSounds.end()) {
		loadSound(strSoundName);
		sound = mSounds.find(strSoundName);

		// if not successfully loaded, return error code
		if (sound == mSounds.end()) {
			return -1;
		}
	}

	// At this point, the sound has to be successfully loaded.
	// Look up which channel this sound resides in.
	auto channelIdItr = mSoundChannelIds.find(strSoundName);
	int channelId = -1;

	// If the sound hasn't been allocated a channel, do so 
	if (/* channelIdItr == mSoundChannelIds.end() */ true) {

		// get the next available channel id
		channelId = mnNextChannelId++;

		// match this sound name with the channel id
		mSoundChannelIds[strSoundName] = channelId;
		LOG_DEBUG("Play sound: %s housed in channel %i\n", strSoundName.c_str(), channelId);

		// Create a new channel for the sound and play it. 
		// Start paused ("true") so we don't get a pop when we change the parameters
		// TODO: Differentiate type of sound with an enum and allocate it to the appropriate channel group
		FMOD::Channel* channel = nullptr;
		FMOD::ChannelGroup* channelGroup = nullptr;		// master channel group
		ERRCHK( coreSystem->playSound(sound->second, channelGroup, true, &channel) );

		// Channel successfully allocated
		if (channel) {
			FMOD_MODE eMode;
			sound->second->getMode(&eMode);

			// TODO: Handle occlusion ourselves - disable ray cast by FMOD
			//eMode |= FMOD_3D_IGNOREGEOMETRY;
			//channel->setMode(eMode);

			if (eMode & FMOD_3D) {
				FMOD_VECTOR position = vectorToFmod(vPos);
				ERRCHK( channel->set3DAttributes(&position, nullptr) );
			}

			// update parameters 
			ERRCHK(channel->setVolume(fVolumedB));

			// Connect DSP chain
			// TODO: restructure
			{
				// TODO: Implement time-invariant pitch shifting? Seems hard (phase vocoder?)
				//float freq;
				//channel->getFrequency(&freq);
				//channel->setFrequency(freq * 2);

				// Create DSP chain in this channel
				for (int pos = 0; pos < dspChain.size(); pos++) {
					int dspId = dspChain[pos];
					DSP* engineDSP = mDSPs[dspId];
					FMOD::DSP* dsp = engineDSP->dsp;

					if (engineDSP->type == Limiter) {

						ERRCHK(dsp->setParameterFloat(FMOD_DSP_LIMITER_RELEASETIME, engineDSP->params[FMOD_DSP_LIMITER_RELEASETIME]));
						ERRCHK(dsp->setParameterFloat(FMOD_DSP_LIMITER_CEILING, engineDSP->params[FMOD_DSP_LIMITER_CEILING]));
						ERRCHK(dsp->setParameterFloat(FMOD_DSP_LIMITER_MAXIMIZERGAIN, engineDSP->params[FMOD_DSP_LIMITER_MAXIMIZERGAIN]));
						ERRCHK(dsp->setParameterBool(FMOD_DSP_LIMITER_MODE, engineDSP->params[FMOD_DSP_LIMITER_MODE]));
					}

					else {

						// Apply all parameters. The parameter values are initialized when the DSP engine handle is created,
						// so we just need to iterate through this map, discarding all type information.
						for (const auto& itr : engineDSP->params) {
							ERRCHK(dsp->setParameterFloat(itr.first, itr.second));
						}
					}

					// Add DSP to this channel
					// TODO: option to add to channel group
					ERRCHK( channel->addDSP(0, dsp) );
				}
			}

			// unpause
			ERRCHK(channel->setPaused(false));
			mChannels[channelId] = channel;
		}
	}

	// Otherwise (sound has channel), the channel is already playing, so don't do anything.


	// return channel id at the end
	return channelId;
}


bool AudioManager::pauseUnpauseSound(IAudioSource* src) {

	// If this channel does not exist, then you can't pause
	auto channel = mChannels.find(src->channelId);
	if (channel == mChannels.end()) return false;

	bool paused = false;	channel->second->getPaused(&paused);
	ERRCHK( channel->second->setPaused(!paused) );
	return !paused;		// return new state
}


bool AudioManager::stopSound(IAudioSource* src) {

	// If this channel does not exist, then you can't stop
	auto channel = mChannels.find(src->channelId);
	if (channel == mChannels.end()) return false;

	return !ERRCHK( channel->second->stop() );
}

bool AudioManager::isPlaying(int nChannelId) const {

	// If this channel does not exist, then it's not playing
	auto channel = mChannels.find(nChannelId);
	if (channel == mChannels.end()) return false;

	// Otherwise, fetch from fmod
	bool isPlaying;
	ERRCHK(channel->second->isPlaying(&isPlaying));
	return isPlaying;
}


/*   Occlusion   */
int AudioManager::createGeometry(int nMaxPolygons, int nMaxVertices, const vec3 &pos, const vec3 &forward, const vec3 &up, const vec3 &scale) {

	FMOD::Geometry* geo = nullptr;
	int err = ERRCHK( coreSystem->createGeometry(nMaxPolygons, nMaxVertices, &geo) );

	if (err || geo == nullptr) return -1;

	u32 geoId = geometries.size();
	geometries.push_back(geo);

	orientGeometry(geoId, pos, forward, up, scale);

	return geoId;
}

int AudioManager::createGeometry(Entity* ent) {

	// Pass if the entity doesn't have AudioGeometryComponent
	if (!ent->containsComps<AudioGeometryComponent>()) return -1;

	// Get mesh data
	AudioGeometryComponent* geoComp = ent->getComp<AudioGeometryComponent>();

	int nFaces = geoComp->getNumOfFaces();
	int nVertsPerFace = 3;	// Assume mesh contains triangles
	int nVertices = nFaces * nVertsPerFace;	// number of face indices, not unique vertices
	// Also assume winding order is CCW

	// Default transform data
	vec3 pos( 0.0f );
	vec3 forward( 0.0f, 0.0f, 1.0f );
	vec3 up( 0.0f, 1.0f, 0.0f );
	vec3 scale( 1.0f );

	// Get transform if the entity has one
	ITransform* itrans = ent->getComp<TransformComponent>();
	if (itrans && ent->isEnabled<TransformComponent>()) {

		mat4 rotation = glm::eulerAngleXYZ(itrans->rx, itrans->ry, itrans->rz);

		pos		= vec3(itrans->x, itrans->y, itrans->z);
		forward = rotation * vec4(forward, 1.0f);
		up		= rotation * vec4(up, 1.0f);
		scale	= vec3(itrans->sx, itrans->sy, itrans->sz);
	}

	int geoId = createGeometry(nFaces, nVertices, pos, forward, up, scale);

	Array<vec3> vVertices = geoComp->getVertices();
	Array<ivec3> faceIndices = geoComp->getFaces();

	for (int f = 0; f < nFaces; f++) {

		// TODO: Get custom data for each face from mesh
		float fDirectOcclusion = 1.0f;
		float fReverbOcclusion = 0.5f;

		// Form an array of the vertices for this face, assuming each face has exactly 3 vertices
		Array<vec3> faceVertices(nVertsPerFace);
		for (int faceVertex = 0; faceVertex < nVertsPerFace; faceVertex++) {
			faceVertices[faceVertex] = vVertices[ faceIndices[f][faceVertex] ];
		}

		// Add this face
		addPolygon(geoId, fDirectOcclusion, fReverbOcclusion, true, nVertsPerFace, faceVertices);
	}

	// Set Geometry id on the component. For better separation, this should be done in GameManager
	geoComp->geometryId = geoId;

	return geoId;
}

void AudioManager::addPolygon(int nGeometryId, float fDirectOcclusion, float fReverbOcclusion, bool isDoubleSided, int nVertices, const Array<vec3> &vVertices) {

	FMOD::Geometry* geo = geometries[nGeometryId];

	// Convert vertices to FMOD_VECTOR
	std::vector<FMOD_VECTOR> vertices(nVertices);
	for (int i = 0; i < nVertices; i++) {
		vertices[i] = vectorToFmod(vVertices[i]);
	}

	ERRCHK( geo->addPolygon(fDirectOcclusion, fReverbOcclusion, isDoubleSided, nVertices, vertices.data(), nullptr) );
}

void AudioManager::setGeometryActive(int nGeometryId, bool isActive) {

	if (nGeometryId < 0) return;
	assert(nGeometryId < geometries.size());

	FMOD::Geometry* geo = geometries[nGeometryId];
	ERRCHK( geo->setActive(isActive) );
}

void AudioManager::orientGeometry(int nGeometryId, const vec3 &pos, const vec3 &forward, const vec3 &up, const vec3 &scale) {

	FMOD_VECTOR fmPos	= vectorToFmod(pos);
	FMOD_VECTOR fmFwd	= vectorToFmod(forward);
	FMOD_VECTOR fmUp	= vectorToFmod(up);
	FMOD_VECTOR fmScale = vectorToFmod(scale);

	fmScale.x = std::max(0.0f, fmScale.x);
	fmScale.y = std::max(0.0f, fmScale.y);
	fmScale.z = std::max(0.0f, fmScale.z);

	FMOD::Geometry* geo = geometries[nGeometryId];
	ERRCHK( geo->setPosition(&fmPos)		);
	ERRCHK( geo->setRotation(&fmFwd, &fmUp) );
	ERRCHK( geo->setScale	(&fmScale)		);
}

void AudioManager::saveGeometry(int nGeometryId, const String& filePath) {
	throw std::exception("Implement");
}
int AudioManager::loadGeometry(const String& filePath) {
	throw std::exception("Implement");
	return -1;
}


//////////////////////////
/*    DSP FUNCTIONS     */
//////////////////////////

int AudioManager::createDSP(DSP_TYPE engineType, int channel, int position) {

	// Convert from engine DSP type to fmod's internal DSP type
	const auto internalTypeItr = kDSPTypeMap.find(engineType);
	if (internalTypeItr == kDSPTypeMap.end()) return -1;

	// Create the DSP
	FMOD::DSP* internalHandle;
	ERRCHK( coreSystem->createDSPByType(internalTypeItr->second, &internalHandle) );

	DSP *engineDSP = new DSP();
	engineDSP->dspId		= nextDSPId;
	engineDSP->type			= engineType;
	engineDSP->dsp			= internalHandle;

	// Unsupported DSP filters:
	// Channel Mix
	// Chorus
	// Compressor
	// Convolutional Reverb
	// Delay

	// FFT

	// IT Echo
	// IT Lowpass
	// (?) Loudness meter
	// Multiband equalizer / Three Equalizer
	// Normalize (normalize volume)
	// Object pan
	// Oscillator (simple signal generator)
	// Pan (possible but we don't have speakers in the demo, so let's not implement it)

	// Return / Send

	// SFX Reverb: tons of customization
	// TODO: lookup

	// Transceiever

	switch (engineDSP->type) {
	case Distortion:
		engineDSP->params[FMOD_DSP_DISTORTION_LEVEL] = 0.5f;	// Default: 0.5		Range: [0, 1]	Unit: Linear
		break;

	case Echo:
		// Test values: 250, 10, 0, -10
		engineDSP->params[FMOD_DSP_ECHO_DELAY]		= 250.f;	// Default: 500		Range: [1, 5000]	Unit: ms
		engineDSP->params[FMOD_DSP_ECHO_FEEDBACK]	= 10.f;		// Default: 50		Range: [0, 100]		Unit: Percentage
		engineDSP->params[FMOD_DSP_ECHO_DRYLEVEL]	= 0.f;		// Default: 0		Range: [-80, 10]	Unit: dB
		engineDSP->params[FMOD_DSP_ECHO_WETLEVEL]	= -10.f;	// Default: 0		Range: [-80, 10]	Unit: dB
		break;

	case Fader:
		engineDSP->params[FMOD_DSP_FADER_GAIN]		= 0.f;		// Default: 0		Range: [-80, 10]	Unit: dB
		break;

	case Flange:	// (wooshing, sweeping effect)
		// Test values: 50, 1, 0.6
		engineDSP->params[FMOD_DSP_FLANGE_MIX]		= 50.f;		// Default: 50		Range: [1, 100]		Unit: Percentage
		engineDSP->params[FMOD_DSP_FLANGE_DEPTH]	= 1.0f;		// Default: 1		Range: [0.01, 1]	Unit: Linear
		engineDSP->params[FMOD_DSP_FLANGE_RATE]		= 0.6f;		// Default: 0.1		Range: [0, 20]		Unit: Hertz
		break;

	case Limiter:
		engineDSP->params[FMOD_DSP_LIMITER_RELEASETIME]		= 10.f;		// Default: 10		Range: [1, 100]		Unit: ms
		engineDSP->params[FMOD_DSP_LIMITER_CEILING]			= 0.f;		// Default: 0		Range: [-12, 0]		Unit: dB
		engineDSP->params[FMOD_DSP_LIMITER_MAXIMIZERGAIN]	= 0.f;		// Default: 0		Range: [0, 12]		Unit: dB
		engineDSP->params[FMOD_DSP_LIMITER_MODE]			= 0.f;		// Default: false
		break;

	case PitchShift:
		// Test values: 2, 1024, 0
		engineDSP->params[FMOD_DSP_PITCHSHIFT_PITCH]		= 1.f;		// Default: 1		Range: [0.5, 2]		Unit: Octave
		engineDSP->params[FMOD_DSP_PITCHSHIFT_FFTSIZE]		= 1024.f;	// Default: 1024	Set: {256, 512, 1024, 2048, 4096}
		engineDSP->params[FMOD_DSP_PITCHSHIFT_MAXCHANNELS]	= 0.f;		// Default: 0		Range: [0, 16]
		break;

	case Tremolo:
		// Test values: skew 0.5
		engineDSP->params[FMOD_DSP_TREMOLO_FREQUENCY] = 5.f;	// Default: 5		Range: [0.1, 20]	Unit: Hertz
		engineDSP->params[FMOD_DSP_TREMOLO_DEPTH]	= 1.f;		// Default: 1		Range: [0, 1]		Unit: Linear
		engineDSP->params[FMOD_DSP_TREMOLO_SHAPE]	= 1.f;		// Default: 1		Range: [0, 1]		Unit: Linear (morph between triangle and sine)
		engineDSP->params[FMOD_DSP_TREMOLO_SKEW]	= 0.f;		// Default: 0		Range: [-1, 1]		Unit: Linear
		engineDSP->params[FMOD_DSP_TREMOLO_DUTY]	= 0.5f;		// Default: 0.5		Range: [0, 1]		Unit: Linear
		engineDSP->params[FMOD_DSP_TREMOLO_SQUARE]	= 1.f;		// Default: 1		Range: [0, 1]		Unit: Linear
		engineDSP->params[FMOD_DSP_TREMOLO_PHASE]	= 0.f;		// Default: 0		Range: [0, 1]		Unit: Linear
		engineDSP->params[FMOD_DSP_TREMOLO_SPREAD]	= 0.f;		// Default: 0		Range: [-1, 1]		Unit: Linear
		break;

	default:	break;
	}

	// Add to the map
	mDSPs[nextDSPId] = engineDSP;
	
	//mDSPs[channel].insert(mDSPs[channel].begin() + position, dsp);

	//ERRCHK(dsp->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, 0.8));
	//ERRCHK( channel->addDSP(position, dsp) );

	// Return a unique id for this DSP for reference, increment for next DSP
	return nextDSPId++;
}

DSP* AudioManager::getDSP(int dspId) {

	const auto itr = mDSPs.find(dspId);
	return itr == mDSPs.end() ? nullptr : itr->second;
}



//////////////////////////
/*   SETTER FUNCTIONS   */
//////////////////////////

void AudioManager::set3dListenerAttributes(const vec3& vPos, const vec3& vVel, const vec3& vForward, const vec3& vUp) {

	// convert to fmod vectors
	FMOD_VECTOR pos = vectorToFmod(vPos);
	FMOD_VECTOR forward = vectorToFmod(vForward);
	FMOD_VECTOR up = vectorToFmod(vUp);
	FMOD_VECTOR vel = {0.0f, 0.0f, 0.0f };   // TODO: Take this into account to enable Doppler

	// create and set the listener
	// TODO: This is for studio API
	//FMOD_3D_ATTRIBUTES listener;
	//listener.position = pos;
	//listener.forward = forward;
	//listener.up = up;
	//listener.velocity = vel;

	ERRCHK(coreSystem->set3DListenerAttributes(
				0,			// Listener ID. For singleplayer this is always 0
				&pos,
				&vel,
				&forward,
				&up
				));		// Core API
	//ERRCHK(studioSystem->setListenerAttributes(0, &listener));		// studio API
	//std::cout << "Position: " << listener.forward.x << "," << listener.forward.y << "," << listener.forward.z << std::endl;
}

void AudioManager::set3dSettings(float dopplerScale, float distanceFactor, float rolloffScale) {

	ERRCHK(coreSystem->set3DSettings(dopplerScale, distanceFactor, rolloffScale));
}


void AudioManager::setChannel3dPosition(int nChannelId, ITransform *ipos) {
	if (ipos) setChannel3dPosition(nChannelId, vec3{ipos->x, ipos->y, ipos->z});
}

void AudioManager::setChannel3dPosition(int nChannelId, const vec3& vPosition) {

	// find the specified channel
	auto channelItr = mChannels.find(nChannelId);
	if (channelItr == mChannels.end()) return;

	FMOD_MODE eMode;
	channelItr->second->getMode(&eMode);

	// update position only if channel is 3D 
	// (calling set3DAttributes on a 2D channel returns an error: FMOD_ERR_NEEDS3D)
	if (eMode & FMOD_3D) {
		FMOD_VECTOR position = vectorToFmod(vPosition);
		ERRCHK( channelItr->second->set3DAttributes(&position, nullptr) );
	}
}


void AudioManager::setChannelVolume(int nChannelId, float fNormalizedVolume) {

	// find the specified channel
	auto channelItr = mChannels.find(nChannelId);
	if (channelItr == mChannels.end()) return;

	// update volume
	ERRCHK(channelItr->second->setVolume(fNormalizedVolume));
}



//////////////////////////
/*   HELPER FUNCTIONS   */
//////////////////////////

float AudioManager::dbToVolume(float dB) {
	return powf(10.0f, 0.05f * dB);
}

float AudioManager::volumeTodB(float volume) {
	return 20.0f * log10f(volume);
}

FMOD_VECTOR AudioManager::vectorToFmod(const vec3& v) {
	FMOD_VECTOR fVec;
	fVec.x = v.x;
	fVec.y = v.y;
	fVec.z = v.z;
	return fVec;
}

int AudioManager::ERRCHK(FMOD_RESULT result, bool bPrint, bool bExit) {
	if (FMOD_OK != result) {

		if (bPrint) {
			LOG_DEBUG("FMOD Error (%d): %s\n", result, FMOD_ErrorString(result));
		}

		if (bExit) {
			throw std::exception("FMOD Error");
		}

		return 1;
	}

	return 0;
}
