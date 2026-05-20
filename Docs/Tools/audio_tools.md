# Audio Tools

Tools for inspecting audio assets and audio components on actors.

## list_audio_assets

List audio assets in the content browser.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path to search |
| `asset_type` | string | `SoundWave` | `SoundWave`, `SoundCue`, `MetaSoundSource`, `SoundAttenuation`, `SoundClass`, `SoundMix` |
| `recursive` | bool | `true` | Search subdirectories |

## get_sound_wave_info

Get detailed info about a SoundWave  - duration, sample rate, channels, looping, streaming status.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Full asset path to the SoundWave |

## get_sound_cue_info

Get info about a SoundCue  - duration, volume/pitch multipliers, attenuation, sound class.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Full asset path to the SoundCue |

## get_sound_attenuation_info

Get SoundAttenuation settings  - falloff distance, shape, spatialization.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Full asset path to the SoundAttenuation |

## get_actor_audio_components

Get audio components on a level actor  - attached sounds and their settings.

| Parameter | Type | Description |
|-----------|------|-------------|
| `actor_name` | string | Name of the actor to inspect |

## play_sound_at_location

Play a sound at a world location in the editor (preview).

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sound_path` | string | | Path to SoundWave or SoundCue |
| `location` | float[3] | `[0,0,0]` | World position |
| `volume` | float | `1.0` | Volume multiplier |
| `pitch` | float | `1.0` | Pitch multiplier |
