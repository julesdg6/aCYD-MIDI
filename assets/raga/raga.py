# raga_midi.py
from mido import Message, MidiFile, MidiTrack, bpm2tempo
import random

TONIC = 60  # Sa = C4

RAGAS = {
    "Bhairavi":  [0, 1, 3, 5, 7, 8, 10],
    "Lalit":     [0, 1, 4, 5, 6, 9, 11],
    "Bhupali":   [0, 2, 4, 7, 9],
    "Todi":      [0, 1, 3, 6, 7, 8, 11],
    "Madhuvanti":[0, 2, 3, 6, 7, 9, 11],
    "Meghmalhar":[0, 2, 5, 7, 10],
    "Yaman":     [0, 2, 4, 6, 7, 9, 11],
    "Malkauns":  [0, 3, 5, 8, 10],
}

def generate_phrase(scale, length=16, start_octave=0):
    """Very simple random-walk phrase on a raga scale."""
    phrase = []
    current_degree = random.choice(range(len(scale)))
    octave = start_octave

    for _ in range(length):
        # Random step: -1, 0, or +1 in the scale
        step = random.choice([-1, 0, 1])
        current_degree = max(0, min(len(scale) - 1, current_degree + step))

        # Occasionally jump octave for a bit of contour
        if random.random() < 0.1:
            octave += random.choice([-1, 1])
            octave = max(-1, min(1, octave))

        semitones = scale[current_degree] + 12 * octave
        phrase.append(semitones)

    return phrase

def raga_to_midi(raga_name, bars=16, notes_per_bar=4, bpm=90,
                 tonic=TONIC, filename=None, channel=0):
    # Validate raga_name to provide helpful error messages to callers
    if raga_name not in RAGAS:
        available = ', '.join(sorted(RAGAS.keys()))
        raise ValueError(f"Unknown raga '{raga_name}'. Available ragas: {available}")
    scale = RAGAS[raga_name]
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0, channel=channel))  # Acoustic Grand
    ticks_per_beat = mid.ticks_per_beat
    dur = ticks_per_beat  # quarter notes
    tempo = bpm2tempo(bpm)
    track.append(Message('control_change', control=7, value=100, time=0, channel=channel))  # volume
    # tempo meta
    from mido import MetaMessage
    track.append(MetaMessage('set_tempo', tempo=tempo, time=0))

    total_notes = bars * notes_per_bar
    phrase = generate_phrase(scale, length=total_notes, start_octave=0)

    for semitones in phrase:
        # compute note and clamp to valid MIDI range 0-127
        note = tonic + semitones
        note = max(0, min(127, int(note)))
        track.append(Message('note_on', note=note, velocity=80,
                             time=0, channel=channel))
        track.append(Message('note_off', note=note, velocity=0,
                             time=dur, channel=channel))

    if filename is None:
        filename = f"{raga_name}_improv.mid"
    mid.save(filename)
    print(f"Saved {filename}")

if __name__ == "__main__":
    for name in RAGAS:
        raga_to_midi(name, bars=8, bpm=80)
