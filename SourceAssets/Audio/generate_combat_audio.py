"""Generate Emberdeep's deterministic placeholder combat sound kit.

These intentionally crunchy, short sounds are original synthesized assets. They
are placeholders with stable filenames so a sound designer can replace them
without changing gameplay code or Unreal references.
"""

from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path


SAMPLE_RATE = 44_100
OUTPUT = Path(__file__).resolve().parent / "generated"


def clamp(value: float) -> float:
    return max(-1.0, min(1.0, value))


def smoothstep(value: float) -> float:
    value = max(0.0, min(1.0, value))
    return value * value * (3.0 - 2.0 * value)


def write_sound(name: str, duration: float, synth) -> None:
    count = int(SAMPLE_RATE * duration)
    rng = random.Random(name)
    values = [clamp(synth(i / SAMPLE_RATE, duration, rng)) for i in range(count)]
    peak = max(0.001, max(abs(value) for value in values))
    gain = 0.88 / peak
    payload = b"".join(struct.pack("<h", int(clamp(value * gain) * 32767)) for value in values)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT / f"{name}.wav"), "wb") as sound:
        sound.setnchannels(1)
        sound.setsampwidth(2)
        sound.setframerate(SAMPLE_RATE)
        sound.writeframes(payload)


def decaying_noise(t: float, duration: float, rng: random.Random, power: float = 2.0) -> float:
    return (rng.random() * 2.0 - 1.0) * (1.0 - t / duration) ** power


def light_swing(t: float, duration: float, rng: random.Random) -> float:
    x = t / duration
    envelope = math.sin(math.pi * x) ** 1.7
    frequency = 1050.0 - 760.0 * smoothstep(x)
    tone = math.sin(2.0 * math.pi * frequency * t + 9.0 * x * x)
    return envelope * (0.38 * tone + 0.62 * decaying_noise(t, duration, rng, 0.35))


def heavy_swing(t: float, duration: float, rng: random.Random) -> float:
    x = t / duration
    envelope = math.sin(math.pi * x) ** 1.25
    frequency = 420.0 - 300.0 * smoothstep(x)
    tone = math.sin(2.0 * math.pi * frequency * t + 14.0 * x * x)
    rumble = math.sin(2.0 * math.pi * 74.0 * t) * (x ** 0.6)
    return envelope * (0.42 * tone + 0.28 * rumble + 0.30 * decaying_noise(t, duration, rng, 0.25))


def bone_hit(t: float, duration: float, rng: random.Random) -> float:
    x = t / duration
    crack = decaying_noise(t, duration, rng, 5.5)
    wood = math.sin(2.0 * math.pi * 680.0 * t) * math.exp(-19.0 * t)
    click = math.sin(2.0 * math.pi * 1880.0 * t) * math.exp(-34.0 * t)
    return 0.48 * crack + 0.36 * wood + 0.26 * click * (1.0 - x)


def heavy_impact(t: float, duration: float, rng: random.Random) -> float:
    x = t / duration
    low = math.sin(2.0 * math.pi * (72.0 - 20.0 * x) * t) * math.exp(-7.0 * t)
    mid = math.sin(2.0 * math.pi * 210.0 * t) * math.exp(-14.0 * t)
    debris = decaying_noise(t, duration, rng, 3.2)
    return 0.62 * low + 0.30 * mid + 0.44 * debris


def dodge(t: float, duration: float, rng: random.Random) -> float:
    x = t / duration
    envelope = math.sin(math.pi * x) ** 1.4
    whistle = math.sin(2.0 * math.pi * (760.0 + 520.0 * x) * t)
    return envelope * (0.28 * whistle + 0.72 * (rng.random() * 2.0 - 1.0))


def enemy_windup(t: float, duration: float, rng: random.Random) -> float:
    x = t / duration
    envelope = smoothstep(x) * (1.0 - 0.45 * x)
    frequency = 92.0 + 185.0 * x * x
    growl = math.sin(2.0 * math.pi * frequency * t)
    harmonic = math.sin(2.0 * math.pi * frequency * 2.03 * t)
    return envelope * (0.64 * growl + 0.22 * harmonic + 0.14 * (rng.random() * 2.0 - 1.0))


def player_hurt(t: float, duration: float, rng: random.Random) -> float:
    x = t / duration
    thud = math.sin(2.0 * math.pi * (128.0 - 55.0 * x) * t) * math.exp(-9.0 * t)
    armor = math.sin(2.0 * math.pi * 530.0 * t) * math.exp(-22.0 * t)
    return 0.70 * thud + 0.24 * armor + 0.20 * decaying_noise(t, duration, rng, 3.5)


def enemy_death(t: float, duration: float, rng: random.Random) -> float:
    value = 0.25 * math.sin(2.0 * math.pi * 66.0 * t) * math.exp(-4.5 * t)
    for impact_time, frequency in ((0.0, 520.0), (0.075, 760.0), (0.145, 430.0), (0.235, 890.0)):
        local = t - impact_time
        if local >= 0.0:
            value += 0.34 * math.sin(2.0 * math.pi * frequency * local) * math.exp(-27.0 * local)
            value += 0.22 * (rng.random() * 2.0 - 1.0) * math.exp(-31.0 * local)
    return value


def main() -> None:
    sounds = {
        "S_FighterSwingLight": (0.18, light_swing),
        "S_FighterSwingHeavy": (0.34, heavy_swing),
        "S_BoneHit": (0.24, bone_hit),
        "S_HeavyImpact": (0.42, heavy_impact),
        "S_Dodge": (0.26, dodge),
        "S_EnemyWindup": (0.38, enemy_windup),
        "S_PlayerHurt": (0.30, player_hurt),
        "S_EnemyDeath": (0.55, enemy_death),
    }
    for name, (duration, synth) in sounds.items():
        write_sound(name, duration, synth)
        print(f"generated {name}.wav")


if __name__ == "__main__":
    main()
