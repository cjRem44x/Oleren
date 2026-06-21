# Audio

Gdev audio uses SDL_mixer. Initialize the audio subsystem before loading any sounds.

## Setup

```olrn
try mk.init_audio()
defer mk.close_audio()
```

## Sound effects

Short, one-shot audio clips.

```olrn
sfx :Sound = try mk.load_sound("assets/jump.wav")
defer mk.unload_sound(sfx)

mk.play_sound(sfx)
mk.sound_set_volume(sfx, 0.8)   # 0.0 – 1.0
mk.stop_sounds()                 # stop all playing sounds
```

## Music (streaming)

Long tracks streamed from disk — OGG, MP3, WAV.

```olrn
bgm :Music = try mk.load_music("assets/theme.ogg")
defer mk.unload_music(bgm)

mk.play_music(bgm, -1)    # -1 = loop forever, 0 = play once, N = N times
mk.music_set_volume(0.6)

if mk.music_playing() {
    mk.pause_music()
}
if mk.music_paused() {
    mk.resume_music()
}
mk.stop_music()
```

## Typical game loop integration

```olrn
try mk.init_window(800, 600, "Game")
defer mk.close_window()
try mk.init_audio()
defer mk.close_audio()

bgm := try mk.load_music("assets/bgm.ogg")
defer mk.unload_music(bgm)
mk.play_music(bgm, -1)

jump_sfx := try mk.load_sound("assets/jump.wav")
defer mk.unload_sound(jump_sfx)

while !mk.should_close() {
    if mk.key_pressed(mk.keys.SPACE) {
        mk.play_sound(jump_sfx)
    }
    mk.begin_draw()
        mk.clear_bg(mk.BLACK)
    mk.end_draw()
}
```
