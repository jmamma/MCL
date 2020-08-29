/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef RECORDING_ENCODER_H__
#define RECORDING_ENCODER_H__

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_encoders 
 *
 * @{
 *
 * \addtogroup gui_recording_encoder Recording Encoder Class
 *
 * @{
 *
 * \file
 * Recording Encoder implementation
 **/

/**
 * Create a recording encoder recording N values. The RecordingEncoder
 * is actually a frontend simulating an encoder, delegating most calls
 * to the actual encoder. However, it can be used to record movements
 * of this encoder and play them back.
 **/
template <int N>
class RecordingEncoder : public Encoder {
	/**
	 * \addtogroup gui_recording_encoder
	 * @{
	 **/
	
public:
  Encoder *realEnc;
  int value[N];
  bool recording;
  bool recordChanged;
  bool playing;
  int currentPos;

	/** Create a recording encoder wrapper for the actual encoder _realEnc. **/
  RecordingEncoder(Encoder *_realEnc = NULL) {
    initRecordingEncoder(_realEnc);
  }

  void initRecordingEncoder(Encoder *_realEnc) {
    realEnc = _realEnc;
    recording = false;
    playing = true;
    clearRecording();
    currentPos = 0;
  }

  void startRecording();
  void stopRecording();
  void clearRecording();
  void playback(uint8_t pos);

  virtual char *getName() {
    return realEnc->getName();
  }
  virtual void setName(char *_name) {
    realEnc->setName(_name);
  }
  virtual int update(encoder_t *enc);
  virtual void checkHandle() {
    realEnc->checkHandle();
  }
  virtual bool hasChanged() {
    return realEnc->hasChanged();
  }

  virtual int getValue() {
    return realEnc->getValue();
  }
  virtual int getOldValue() {
    return realEnc->getOldValue();
  }
  virtual void setValue(int _value, bool handle = false) {
    realEnc->setValue(_value, handle);
    redisplay = realEnc->redisplay;
  }

  virtual void displayAt(int i) {
    realEnc->displayAt(i);
  }

	/* @} */
};

/* RecordingEncoder */
template <int N>
void RecordingEncoder<N>::startRecording() {
  recordChanged = false;
  recording = true;
}

template <int N>
void RecordingEncoder<N>::stopRecording() {
  recordChanged = false;
  recording = false;
}

template <int N>
void RecordingEncoder<N>::clearRecording() {
  for (int i = 0; i < N; i++) {
    value[i] = -1;
  }  
}

template <int N>
int RecordingEncoder<N>::update(encoder_t *enc) {
  USE_LOCK();
  //  SET_LOCK();

  cur = realEnc->update(enc);
  redisplay = realEnc->redisplay;

  if (recording) {
    if (!recordChanged) {
      if (enc->normal != 0 || enc->button != 0) {
				recordChanged = true;
      }
    }
    if (recordChanged) {
      int pos = currentPos;
      value[pos] = cur;
    }
  }
  // CLEAR_LOCK();
  return cur;
}  

template <int N>
void RecordingEncoder<N>::playback(uint8_t pos) {
  if (!playing)
    return;

  currentPos = (pos % N);
  if (value[currentPos] != -1) {
    if (!(recording && recordChanged)) {
      realEnc->setValue(value[currentPos], true);
      redisplay = realEnc->redisplay;
    }
    // check if real encoder has change value XXX
  }
}

/* @} @} @} */

#endif /* RECORDING_ENCODER_H__ */
