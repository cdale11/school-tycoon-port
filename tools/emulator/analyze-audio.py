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
parser.add_argument('--maximum-clipped-fraction', type=float, help='Optional maximum clipped/nonzero sample fraction, from 0 to 1')
args = parser.parse_args()
if args.maximum_clipped_fraction is not None and not 0 <= args.maximum_clipped_fraction <= 1:
    parser.error('maximum clipped fraction must be between 0 and 1')
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
        nonzero_samples=nonzero, clipped_samples=clipped,
        clipped_nonzero_fraction=clipped / nonzero if nonzero else 0), sort_keys=True))
    if not samples or peak < args.minimum_peak:
        raise SystemExit('FAIL: empty, silent or only noise-level capture')

    if args.maximum_clipped_fraction is not None and nonzero and clipped / nonzero > args.maximum_clipped_fraction:
        raise SystemExit('FAIL: clipped sample fraction exceeds the requested limit')
