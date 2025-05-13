from mido import Message, MidiFile, MidiTrack, MetaMessage

# Create a new MIDI file and add a track
mid = MidiFile(ticks_per_beat=480) # Standard TPQN
track = MidiTrack()
mid.tracks.append(track)

# Add a tempo meta message (optional, defaults to 120 BPM if not set in many players)
track.append(MetaMessage('set_tempo', tempo=500000)) # 500000 microseconds per beat = 120 BPM

# Notes for C major scale (MIDI note numbers)
# C4=60, D4=62, E4=64, F4=65, G4=67, A4=69, B4=71, C5=72
notes = [60, 62, 64, 65, 67, 69, 71, 72]
velocity = 100
duration_ticks = 480  # Quarter note at 480 TPQN

time_elapsed = 0
for note in notes:
    track.append(Message('note_on', note=note, velocity=velocity, time=0 if time_elapsed == 0 else 0)) # Delta time from previous event
    track.append(Message('note_off', note=note, velocity=0, time=duration_ticks))
    time_elapsed += duration_ticks # Accumulate for proper timing if needed for more complex sequences

# Add an end of track meta message
track.append(MetaMessage('end_of_track', time=0))

# Save the MIDI file
mid.save('test_song.mid')
print("test_song.mid created successfully.")