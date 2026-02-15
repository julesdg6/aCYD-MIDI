# Raga Mode: 2nd-Order Markov Chain Implementation

## Overview

The Raga mode in aCYD-MIDI implements authentic Indian classical music generation using a **2nd-order Markov chain** for Raga Bhairavi. This approach captures the characteristic "movements" of the raga, including the signature oscillations on Re (r) and Dha (d), which are not possible with simpler random walk or 1st-order Markov models.

## What is a 2nd-Order Markov Chain?

In a **1st-order Markov chain**, the probability of the next note depends only on the current note:
```
P(note_n | note_n-1)
```

In a **2nd-order Markov chain**, the probability of the next note depends on the previous TWO notes:
```
P(note_n | note_n-1, note_n-2)
```

This allows the system to capture **phrases** and **melodic patterns** rather than just single-note transitions.

## Raga Bhairavi Scale

Bhairavi is a **Sampoorna** (7-note) raga using all four **Komal** (flat) notes:

| Swara | Western | Interval | Role |
|-------|---------|----------|------|
| S     | C       | 0        | Samvadi (Minister) |
| r     | Db      | 1        | Komal Re (flat) |
| g     | Eb      | 3        | Komal Ga (flat) |
| m     | F       | 5        | **Vadi (King)** |
| P     | G       | 7        | Perfect Fifth |
| d     | Ab      | 8        | Komal Dha (flat) |
| n     | Bb      | 10       | Komal Ni (flat) |

### Important Notes

- **Vadi (m/Madhyam)**: The "king" note - given highest weight in transitions
- **Samvadi (S/Shadja)**: The "minister" note - given second highest weight

## Key Phrases (Pakad)

The implementation encodes three signature phrases of Bhairavi:

1. **S r g m** - Ascending phrase establishing the raga
2. **m g r S** - Descending phrase returning to tonic
3. **g m d n S'** - Upper octave phrase

These phrases are encoded in the transition probability matrix by giving higher weights to note sequences that match these patterns.

## Implementation Details

### Data Structure

The 2nd-order Markov chain is stored as a 3-dimensional array:

```cpp
kBhairavi2ndOrderWeights[prev_prev_note][prev_note][next_note]
```

Each element contains a probability **weight** (0-50) that determines how likely a particular note sequence is to occur.

### Transition Probability Design

The transition weights were carefully designed to:

1. **Emphasize Vadi (m)**: Higher weights when transitioning to or from Madhyam
2. **Emphasize Samvadi (S)**: Second-highest weights for Shadja
3. **Encode Pakad phrases**: Boost weights for the three signature patterns
4. **Enable oscillations**: Higher weights for note repetitions on Re (r) and Dha (d)
5. **Natural melodic movement**: Prefer stepwise motion over large jumps
6. **Strong phrase endings**: High weights for returning to Sa (S) after phrases

### Example Transition

From the phrase **m → g → ?**:

```cpp
// Weights for: S   r   g   m   P   d   n
{25, 35, 25, 25, 10,  5,  5}
```

This gives a 35% relative probability of continuing to **r**, encoding the **"m g r S"** pakad phrase.

### Algorithm

1. **Initialize** with S → S (both starting notes are Sa/tonic)
2. For each note in the phrase:
   - Look up weights for `[prev_prev_note][prev_note]`
   - Calculate total weight
   - Generate random number in [0, total_weight)
   - Select note based on cumulative weight distribution
   - Update history: `prev_prev = prev; prev = selected`
3. Occasional octave shifts (8% probability vs. 10% in random walk)

### Code Structure

```cpp
// Main generation function - dispatches to 2nd-order for Bhairavi
static void generateRagaPhrase()

// 2nd-order Markov chain generator (Bhairavi only)
static void generateBhairavi2ndOrderPhrase()

// Weighted random selection based on 2nd-order history
static int selectNextNoteBhairavi2ndOrder(int prevPrev, int prev)

// Probability weights matrix (7×7×7)
static const uint8_t kBhairavi2ndOrderWeights[7][7][7]
```

## Benefits Over Simple Random Walk

The original random walk implementation:
- ✗ No awareness of phrases or patterns
- ✗ Equal probability for all movements
- ✗ No emphasis on important notes (Vadi/Samvadi)
- ✗ Cannot capture characteristic oscillations

The 2nd-order Markov chain:
- ✓ Encodes authentic raga phrases (Pakad)
- ✓ Emphasizes structurally important notes
- ✓ Creates musically meaningful note sequences
- ✓ Captures the "movement" characteristics of Bhairavi

## Future Enhancements

Potential improvements for other ragas:
- Implement 2nd-order chains for other ragas (Yaman, Todi, etc.)
- Add time-of-day awareness (morning/evening ragas)
- Implement aroha/avaroha (ascending/descending) patterns
- Add gamaka (ornament) generation
- Support for Vakra (crooked/zigzag) phrases

## References

- Raga Bhairavi characteristics and Pakad phrases
- 2nd-order Markov chain music generation theory
- Indian classical music theory (Vadi/Samvadi concepts)

## Implementation Date

February 2026 - Implemented as part of issue regarding alternative ideas for raga generation.
