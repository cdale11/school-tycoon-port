#!/usr/bin/env python3
"""Measure captured PCM16 WAV output without publishing original game audio."""
import argparse
import array
import json
import math
import sys
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('capture')
parser.add_argument('--minimum-peak', type=int, default=32, help='Reject silence and +/-1 LSB resampler dither')
args = parser.parse_args()
if not 1 <= args.minimum_peak <= 32768:
    parser.error('minimum peak must be between 1 and 32768')
with wave.open(args.capture, 'rb') as audio:
    if audio.getsampwidth() != 2 or audio.getcomptype() != 'NONE':
        parser.error('Expected uncompressed signed 16-bit PCM')
    channels, rate = audio.getnchannels(), audio.getframerate()
    samples = nonzero = clipped = peak = squares = 0
    while True:
        block = audio.readframes(8192)
        if not block:
            break
        values = array.array('h', block)
        if sys.byteorder != 'little':
            values.byteswap()
        samples += len(values)
        for value in values:
            magnitude = abs(value)
            peak = max(peak, magnitude)
            nonzero += value != 0
            clipped += magnitude >= 32767
            squares += value * value
    print(json.dumps(dict(channels=channels, rate_hz=rate,
        frames=samples // channels, seconds=samples / channels / rate,
        peak=peak, rms=math.sqrt(squares / samples) if samples else 0,
        nonzero_samples=nonzero, clipped_samples=clipped), sort_keys=True))
    if not samples or peak < args.minimum_peak:
        raise SystemExit('FAIL: empty, silent or only noise-level capture')
