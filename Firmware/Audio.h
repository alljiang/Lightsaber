#include "AmINyanCat.h"

#ifndef AUDIO_H_
#define AUDIO_H_

#ifndef NYANCAT

#define FONTID_A_EXTEND                         0
#define FONTID_A_HUM                            1
#define FONTID_A_RETRACT                        2
#define FONTID_A_SWING1H                        3
#define FONTID_A_SWING1L                        4
#define FONTID_A_SWING2H                        5
#define FONTID_A_SWING2L                        6
#define FONTID_B_EXTEND                         7
#define FONTID_B_HUM                            8
#define FONTID_B_RETRACT                        9
#define FONTID_B_SWING1H                        10
#define FONTID_B_SWING1L                        11
#define FONTID_B_SWING2H                        12
#define FONTID_B_SWING2L                        13

#else

#define FONTID_NYAN_HUM                         0
#define FONTID_NYAN_WAVE                        1

#endif

extern void Audio_initialize();
extern void Audio_sendData(uint16_t number);
extern void Audio_play(uint8_t fontID, uint8_t volume, uint8_t loop);
extern int Audio_isPlaying(uint8_t fontID);
extern void Audio_setVolume(uint8_t fontID, uint8_t volume);
extern void Audio_stop(uint8_t fontID);


#endif /* AUDIO_H_ */
