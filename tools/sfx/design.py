"""Design the mod's cues (the sonar's seven and the six signals) as files, level them with ffmpeg, and build preview
reels with spoken labels.

  python tools/sfx/design.py [--old DIR]

DIR holds a set to compare against as <name>.wav (and <name>_brief.wav); the reels play each of those ("before"),
then the new one. Out: build/sfx/new/*.wav (48 kHz, 24-bit mono), build/sfx/reels/*.wav, build/sfx/spectro/.

Every cue but the fragment is a simulation of the thing making it (physics.py: struck and scraped bodies through a
Hertzian contact, a plucked string on a soundboard, a drumhead, air in a tube, a vocal tract, STK's brass and flute,
a room by image sources). The fragment (the user liked it, 2026-11-13) is the first set's: modal synthesis with a
half-sine contact and spectrogram painting (noise drawn on a time-frequency grid, given random phases); its code and
the helpers it uses (modal, doublets, paint, place, finish) are kept as they were, so it comes out bit for bit the same.
Every cue starts and ends on silence, is one event, and the set is matched by loudness with ffmpeg's EBU R128 meter.
"""
import os
import re
import shutil
import subprocess
import sys

import numpy as np
from scipy import signal as sg
from scipy.io import wavfile

import physics as ph

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "build", "sfx")
R = 48000
PI = np.pi


# ---- Building blocks -----------------------------------------------------------------------------------------------

def N(sec):
    return int(round(sec * R))


def tt(n):
    return np.arange(n) / R


def rms(x):
    return float(np.sqrt(np.mean(np.square(x)))) + 1e-12


def unit(x):
    return x / (np.max(np.abs(x)) + 1e-12)


def ease(x):
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3.0 - 2.0 * x)


def env_ad(t, attack, tau):
    """A soft rise (raised cosine, no click) over `attack`, then an exponential decay."""
    rise = np.where(t < attack, 0.5 - 0.5 * np.cos(PI * t / attack), 1.0)
    return rise * np.exp(-np.maximum(0.0, t - attack) / tau)


def env_note(t, length, attack, release):
    rise = np.where(t < attack, 0.5 - 0.5 * np.cos(PI * t / attack), 1.0)
    left = length - t
    fall = np.where(left < release, 0.5 - 0.5 * np.cos(PI * np.maximum(0.0, left) / release), 1.0)
    return rise * fall


def drift(n, hz, rng):
    """Slow random wander in about -1..1, changing about `hz` times a second: a player's unsteadiness."""
    k = max(4, int(n / R * hz) + 4)
    pts = rng.standard_normal(k)
    x = np.linspace(0, k - 3, n)
    return np.interp(x, np.arange(k), pts) / 1.5


def contact(f, T):
    """How much of frequency f a strike lasting T seconds (a half-sine force pulse) puts in, 1 at DC. A soft, long
    contact (a knuckle, a felt mallet) is dark; a short, hard one (metal on glass) is bright."""
    x = 2.0 * f * T
    with np.errstate(divide="ignore", invalid="ignore"):
        g = np.cos(PI * f * T) / (1.0 - x * x)
    g = np.where(np.abs(1.0 - x) < 1e-6, PI / 4.0, g)
    return np.abs(g)


def modal(n, modes, T, rng=None, bend=None, onset=0.0006):
    """Struck modes: (hz, tau, amp) each. bend: a pitch multiplier over time (a glide), shared by every mode."""
    t = tt(n)
    out = np.zeros(n)
    cum = np.cumsum(bend) / R if bend is not None else t
    for hz, tau, amp in modes:
        if hz >= R * 0.45:
            continue
        ph = rng.uniform(-0.15, 0.15) if rng is not None else 0.0   # strikes are never perfectly in step
        out += amp * float(contact(hz, T)) * np.sin(2 * PI * hz * cum + ph) * np.exp(-t / tau)
    return out * (1.0 - np.exp(-t / onset))


def doublets(modes, split, rng, ratio=0.7):
    """Real bells, glasses and bars ring each mode as a close pair (they are never perfectly round): the pair beats."""
    out = []
    for hz, tau, amp in modes:
        d = split * rng.uniform(0.6, 1.4)
        out += [(hz, tau, amp), (hz + d, tau * rng.uniform(0.8, 1.1), amp * ratio)]
    return out


def paint(n, mag, rng, nfft=1024, hop=None):
    """Spectrogram painting: mag(t, f) -> magnitude on the STFT grid (t in seconds, f in Hz, broadcast), given random
    phases and overlap-added back into sound. Smaller nfft: sharper in time, blurrier in pitch."""
    hop = hop or nfft // 8
    frames = (n + nfft) // hop + 2
    centres = (np.arange(frames) * hop - nfft // 2) / R
    f = np.fft.rfftfreq(nfft, 1.0 / R)
    m = np.asarray(mag(centres[:, None], f[None, :]), dtype=float)
    m = np.broadcast_to(m, (frames, len(f)))
    spec = m * np.exp(1j * rng.uniform(0, 2 * PI, m.shape))
    win = np.hanning(nfft)
    out = np.zeros(frames * hop + nfft)
    for i in range(frames):
        out[i * hop:i * hop + nfft] += np.fft.irfft(spec[i], nfft) * win
    start = nfft // 2
    return out[start:start + n]


def band(f, centre, octaves):
    """A smooth bump around `centre`, `octaves` wide (a Gaussian in log frequency)."""
    return np.exp(-0.5 * (np.log2(np.maximum(f, 1.0) / centre) / octaves) ** 2)


def bandpass_tv(x, hz, q):
    """A two-pole band-pass whose centre follows hz (an array, per sample)."""
    y = np.zeros_like(x)
    x1 = x2 = y1 = y2 = 0.0
    for i in range(len(x)):
        w = 2 * PI * hz[i] / R
        alpha = np.sin(w) / (2 * q)
        a0 = 1 + alpha
        v = (alpha * x[i] - alpha * x2 + 2 * np.cos(w) * y1 - (1 - alpha) * y2) / a0
        x2, x1, y2, y1 = x1, x[i], y1, v
        y[i] = v
    return y


def resonators(x, formants):
    """Parallel resonances (hz, bandwidth, gain): a mouth, a body."""
    out = np.zeros_like(x)
    for hz, bw, g in formants:
        r = np.exp(-PI * bw / R)
        w = 2 * PI * hz / R
        b = [(1 - r * r) * 0.5, 0, -(1 - r * r) * 0.5]
        a = [1, -2 * r * np.cos(w), r * r]
        out += g * sg.lfilter(b, a, x)
    return out


def room_ir(rng, size=1.0, bright=1.0):
    """A small room's response (no direct sound): early reflections, then a diffuse tail painted on a spectrogram,
    its highs dying sooner than its lows (air and soft walls take the highs). Unit energy."""
    length = 0.30 * size
    n = N(length)
    rt_lo, rt_hi = 0.32 * size, 0.10 * size * bright

    def mag(t, f):
        lf = np.clip(np.log2(np.maximum(f, 125.0) / 125.0) / 7.0, 0, 1)
        rt = rt_lo * (rt_hi / rt_lo) ** lf
        start = 0.009 * size
        m = np.exp(-6.91 * np.maximum(t - start, 0) / rt) * (1 - np.exp(-np.maximum(t - start, 0) / 0.006))
        return np.where(t < start, 0.0, m) * (1.0 / (1.0 + (f / 9000.0) ** 2))

    tail = paint(n, mag, rng, nfft=512)
    tail /= rms(tail) * np.sqrt(n)
    early = np.zeros(n)
    for ms, g in ((3.1, 0.55), (4.7, 0.45), (6.9, 0.40), (8.3, 0.33), (11.2, 0.27), (13.9, 0.22), (17.3, 0.17)):
        early[min(n - 1, N(ms * size / 1000))] += g * rng.choice([-1, 1])
    early = sg.lfilter(*sg.butter(1, 6000 / (R / 2)), early)
    ir = early / np.sqrt(np.sum(early ** 2)) * 0.7 + tail * 0.7
    return ir / np.sqrt(np.sum(ir ** 2))


def place(v, rng, mix, size=1.0, bright=1.0):
    """The cue heard in a small room: dry plus `mix` of the room (by energy)."""
    wet = sg.fftconvolve(v, room_ir(rng, size, bright))[:len(v)]
    return v + mix * wet * (rms(v) / rms(wet))


def finish(v, fade_in_ms=1.2, fade_out_ms=18.0):
    """No DC or rumble, silence at both ends (soft fades: nothing clicks)."""
    v = sg.sosfilt(sg.butter(2, 30 / (R / 2), "highpass", output="sos"), v)
    a, b = N(fade_in_ms / 1000), N(fade_out_ms / 1000)
    v[:a] *= 0.5 - 0.5 * np.cos(PI * np.arange(a) / a)
    v[len(v) - b:] *= (0.5 - 0.5 * np.cos(PI * np.arange(b) / b))[::-1]
    return unit(v) * 0.5


# ---- The sonar's seven ---------------------------------------------------------------------------------------------
# Each cue is a simulation of the thing making it (physics.py), heard in a room by image sources.

def exit_cue(brief, rng):
    """A way out: a gust of air pushed through an opening, resonating in it as it widens. Gusty turbulence (its
    strength wanders) drives a tube of air (a waveguide) that shortens from 66 to 15 cm, so its resonances climb; the
    jet's own hiss rides over it; and as the air gives, the push of its pressure, a soft low puff."""
    L = 0.085 if brief else 0.15
    n = N(L)
    t = tt(n)
    u = t / L
    flow = np.sin(PI * np.clip(u, 0, 1) ** 0.6) ** 2
    turb = sg.sosfilt(sg.butter(2, 5000 / (R / 2), output="sos"), rng.standard_normal(n))
    turb *= flow ** 1.5 * (1 + 0.35 * drift(n, 90, rng))
    tube = np.diff(ph.air_tube(turb, 0.66 / 4.4 ** u, loss=0.9, damp=0.3), prepend=0.0)
    hiss = sg.sosfilt(sg.butter(2, 2500 / (R / 2), "highpass", output="sos"), turb)
    puff = np.diff(np.where(t < 0.014, np.sin(PI * t / 0.014) ** 2, 0.0), prepend=0.0)
    v = unit(tube) + 0.12 * unit(hiss) + 0.12 * unit(puff)
    return ph.in_room(v, rng, refl_lo=0.85, refl_hi=0.6)


def door_cue(brief, rng):
    """A room's door: a knuckle knocking on a hollow wooden door, in a corridor. The door is a plate 0.82 x 2.03 m:
    ~250 modes (f grows with (m/a)^2 + (n/b)^2), each moving as sin(m pi x / a) sin(n pi y / b) where the knuckle lands,
    each radiating by its net volume (odd-odd modes push air, the rest mostly slosh it round), wood's damping (and the
    hollow core's air) setting each decay. The knuckle is skin over bone: a soft Hertzian contact of ~1.3 ms against a
    body 25 times its mass. The bolt rattles in its keep."""
    L = 0.08 if brief else 0.12
    n = N(L)
    a, b, x0, y0 = 0.82, 2.03, 0.31 * 0.82, 0.63 * 2.03
    cst = 95.0 / ((1 / a) ** 2 + (1 / b) ** 2)
    f, tau, at, rad = [], [], [], []
    for m in range(1, 16):
        for k in range(1, 60):
            fr = cst * ((m / a) ** 2 + (k / b) ** 2)
            if fr > 6000:
                break
            fr *= rng.uniform(0.985, 1.015)   # wood is never uniform
            eta = 0.03 + 0.04 / (1 + (fr / 300) ** 2)
            f.append(fr)
            tau.append((0.7 if brief else 1.0) / (PI * fr * eta))
            at.append(np.sin(m * PI * x0 / a) * np.sin(k * PI * y0 / b))
            rad.append(((m % 2) * (k % 2) / (m * k) + 0.08 / np.sqrt(m * k)) * min(1.0, fr / 300.0))
    door = ph.Body(f, tau, at, rad, mass=25.0)
    v = unit(ph.strike(door, n, 1.1 if brief else 1.3, softness=0.5))
    bolt = ph.Body([2380, 4110, 6230, 8870], [0.018, 0.011, 0.007, 0.004], 1.0, [1.0, 0.7, 0.5, 0.3], mass=0.5)
    rattle = ph.strike(bolt, n, 0.06, speed=0.08, at=0.006) + ph.strike(bolt, n, 0.06, speed=0.05, at=0.0105)
    v += 0.06 * unit(rattle)
    return ph.in_room(v, rng, dims=(7.0, 1.8, 2.6), src=(3.0, 0.2, 1.3), lis=(5.0, 1.0, 1.6), refl_lo=0.85, refl_hi=0.6)


def bandpass_q(x, hz, q):
    """bandpass_tv with a Q that may change too."""
    hz = np.broadcast_to(hz, x.shape)
    q = np.broadcast_to(q, x.shape)
    y = np.zeros_like(x)
    x1 = x2 = y1 = y2 = 0.0
    for i in range(len(x)):
        w = 2 * PI * hz[i] / R
        alpha = np.sin(w) / (2 * q[i])
        a0 = 1 + alpha
        v = (alpha * x[i] - alpha * x2 + 2 * np.cos(w) * y1 - (1 - alpha) * y2) / a0
        x2, x1, y2, y1 = x1, x[i], y1, v
        y[i] = v
    return y


def person_cue(brief, rng):
    """Someone to talk to: a whistled 'yoo-hoo' in one breath, scooping into E6 and falling to C#6, as a whistle
    really starts: the breath first, hissing through the lips' resonance as it sharpens (its Q climbing from 4 to 30),
    the tone locking in ~7 ms later; the pitch riding the blowing pressure, a slow wobble and the breath's
    unsteadiness; the octave stronger when it is louder; the lips' little puff."""
    L = 0.09 if brief else 0.15
    n = N(L)
    t = tt(n)
    u = t / L
    lock = 0.007
    press = env_note(np.maximum(t - lock, 0), L - lock, 0.018, L * 0.35) * (t >= lock)
    press = press * (1 + 0.06 * drift(n, 35, rng))
    fall = (1109.0 / 1319.0) ** ease((u - 0.35) / 0.45)
    scoop = 1 - 0.06 * np.exp(-t / 0.016)
    hz = 1319.0 * fall * scoop * (1 + 0.012 * (press - 1)) * (1 + 0.006 * np.sin(2 * PI * 5.5 * t) + 0.003 * drift(n, 25, rng))
    phase = 2 * PI * np.cumsum(hz) / R
    tone = press * (np.sin(phase) + 0.06 * press ** 2 * np.sin(2 * phase + 0.4) + 0.01 * np.sin(3 * phase))
    breath_env = env_note(t, L, 0.004, L * 0.35) * (0.7 + 1.4 * np.exp(-t / 0.018))
    q = 4 + 26 * np.clip(t / 0.02, 0, 1)
    breath = bandpass_q(rng.standard_normal(n), hz, q) * breath_env
    hiss = sg.sosfilt(sg.butter(2, [2500 / (R / 2), 9000 / (R / 2)], "bandpass", output="sos"), rng.standard_normal(n))
    hiss *= breath_env * np.exp(-t / 0.03)
    puff = np.diff(np.where(t < 0.006, np.sin(PI * t / 0.006) ** 2, 0.0), prepend=0.0)
    v = tone + breath * 0.22 * rms(tone) / rms(breath) + hiss * 0.05 * rms(tone) / rms(hiss) + 0.04 * unit(puff)
    return ph.in_room(v, rng, src=(1.2, 2.6, 1.6), lis=(3.4, 1.4, 1.6))


def bell_body(nominal, ring, rng):
    """A small bell: a real bell's partials (hum, prime, tierce, quint, nominal and above), the low ones faint in a
    little one, each a beating pair; struck at the rim, where every mode moves."""
    parts = [(0.25, 1.0, 0.03), (0.5, 0.9, 0.07), (0.6, 0.75, 0.10), (0.75, 0.55, 0.05), (1.0, 0.65, 1.0),
             (1.5, 0.42, 0.45), (2.0, 0.30, 0.30), (2.51, 0.22, 0.16), (2.66, 0.20, 0.14), (3.01, 0.16, 0.10),
             (4.06, 0.10, 0.05), (5.2, 0.07, 0.03)]
    f, tau, rad = [], [], []
    for r, k, amp in parts:
        base = nominal * r * rng.uniform(0.997, 1.003)
        for df, share in ((0.0, 1.0), (4.0 * rng.uniform(0.6, 1.4), 0.75)):
            f.append(base + df)
            tau.append(0.22 * ring * k * rng.uniform(0.85, 1.1))
            rad.append(amp * share)
    return ph.Body(f, tau, 1.0, rad, mass=4.0)


def shop_cue(brief, rng):
    """A shop: the bells over a shop's door, two small bells a fifth apart (C6, G6) on one spring. Each brass clapper
    strikes (a hard contact, ~0.12 ms) and bounces back for a light second touch, the second bell a hair after the
    first; the spring sways them, which wavers the sound a little."""
    L = 0.09 if brief else 0.15
    n = N(L)
    t = tt(n)
    ring = 0.5 if brief else 1.0
    b1, b2 = bell_body(1047.0, ring, rng), bell_body(1568.0, ring, rng)
    v = ph.strike(b1, n, 0.12) + ph.strike(b1, n, 0.12, speed=0.3, at=0.021)
    v += ph.strike(b2, n, 0.12, speed=0.75, at=0.004) + ph.strike(b2, n, 0.12, speed=0.2, at=0.027)
    v *= 1 + 0.05 * np.sin(2 * PI * 5.3 * t + 1.0)
    return ph.in_room(v, rng, dims=(5.5, 4.0, 3.0), refl_lo=0.85, refl_hi=0.65)


def thing_cue(brief, rng):
    """Something to check or open: a harp string at A5 plucked by a fingertip. The string's two planes ring a hair
    apart and die apart (the one into the board sooner): the real string's double decay; its harmonics a little sharp
    (stiffness), those at the pluck point missing; the bridge drives a soundboard (ten modes, 105 Hz to 3.5 kHz)."""
    L = 0.085 if brief else 0.13
    n = N(L)
    board = ph.Body([105, 215, 380, 560, 740, 1020, 1400, 1900, 2600, 3500],
                    [0.03, 0.02, 0.015, 0.012, 0.01, 0.008, 0.006, 0.005, 0.004, 0.003], 1.0,
                    [0.6, 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.25])
    v = ph.plucked_string(n, 880.0, 0.18, 0.09 if brief else 0.25, rng, nail_ms=0.25, board=board)
    snap = ph.strike(ph.Body([3200, 5100, 7400], [0.002, 0.0015, 0.001], 1.0, 1.0), n, 0.05, speed=0.1)
    return ph.in_room(unit(v) + 0.04 * unit(snap), rng)


def event_cue(brief, rng):
    """A spot that starts a fight or a scene: a talking drum, struck with a hooked stick and squeezed, so its pitch
    climbs a fourth (F4 to B flat 4: 'bwip'). The head's modes from Bessel zeros, struck two thirds out; struck hard,
    the head is stretched sharp for an instant (tension modulation)."""
    L = 0.085 if brief else 0.13
    n = N(L)
    t = tt(n)
    at, over = (0.013, 0.026) if brief else (0.02, 0.04)
    bend = (466.0 / 349.0) ** ease((t - at) / over) * (1 + 0.035 * np.exp(-t / 0.01))
    head = ph.membrane(349.0, 0.62, 0.07 if brief else 0.16, mass=0.6)
    v = ph.strike(head, n, 0.45, softness=0.08, bend=bend)
    return ph.in_room(v, rng)


def fragment_cue(brief, rng):
    """A Twilight Fragment: one crystal struck with a tiny metal rod at C7. A goblet's modes (1, 2.61, 4.86, 7.6 times
    the lowest), each a beating pair (the shimmer), the rod's hard contact making it bright; a glint of high noise as
    it is struck, and three motes of glitter in its first 30 ms, all one strike."""
    L = 0.09 if brief else 0.15
    n = N(L)
    tau = 0.045 if brief else 0.085
    hz = 2093.0
    modes = [(hz, tau, 1.0), (hz * 2.61, tau * 0.5, 0.45), (hz * 4.86, tau * 0.3, 0.22), (hz * 7.6, tau * 0.18, 0.1)]
    glass = unit(modal(n, doublets(modes, 9.0, rng, 0.8), T=0.00012, rng=rng))
    t = tt(n)
    glint = unit(paint(n, lambda tm, f: band(f, 11000, 0.6) * np.where(tm >= 0, np.exp(-tm / 0.003), 0),
                       rng, nfft=128, hop=8))
    glitter = np.zeros(n)
    for at, f in ((0.006, 9400), (0.014, 12600), (0.027, 10900)):
        s = t - at
        glitter += np.where(s >= 0, np.sin(2 * PI * f * s) * np.exp(-np.maximum(s, 0) / 0.004), 0)
    return place(glass + 0.3 * glint + 0.12 * glitter, rng, 0.28, size=1.3, bright=1.6)


# ---- The other signals ---------------------------------------------------------------------------------------------

def shadow_cue(rng):
    """A Shadow near: a big throat's snarl, 0.28 s. A glottis's pulses (Rosenberg's shape) at ~98 Hz sinking to ~84,
    unsteady (jitter, shimmer), every other pulse weaker (the period-doubled rasp of a growl), the false folds
    buzzing against them a little higher; the breath's turbulence where the folds part; through a vocal tract of 64
    sections (23 cm: bigger than a man's) shaped for a snarled 'aah'. The old cue's 38 Hz rattle is in the breath."""
    L = 0.28
    n = N(L)
    t = tt(n)
    u = t / L
    press = (1 - np.exp(-t / 0.008)) * np.minimum(1.0, (1.0 - u) * 1.5)
    press *= 0.62 + 0.38 * np.cos(2 * PI * 38.0 * t)
    src = np.zeros(n)
    for base, level in ((98.0, 1.0), (104.5, 0.45)):
        f0 = (base - 14.0 * u) * (1 + 0.02 * drift(n, 40, rng))
        cyc = np.cumsum(f0) / R
        k = np.floor(cyc).astype(int)
        fr = cyc - k
        g = np.where(fr < 0.42, 0.5 * (1 - np.cos(PI * fr / 0.42)),
                     np.where(fr < 0.6, np.cos(PI / 2 * (fr - 0.42) / 0.18), 0.0))
        shimmer = 1 + 0.1 * rng.standard_normal(k.max() + 2)
        src += level * g * np.where(k % 2 == 0, 1.0, 0.55) * shimmer[k]
    turb = sg.sosfilt(sg.butter(2, 5000 / (R / 2), output="sos"), rng.standard_normal(n))
    src = (src + 0.12 * turb * src) * press
    x = np.linspace(0, 1, 64)
    areas = np.interp(x, [0, 0.08, 0.14, 0.3, 0.45, 0.55, 0.7, 0.88, 1.0], [0.4, 0.5, 1.2, 0.7, 0.9, 3.0, 5.0, 4.5, 2.4])
    v = ph.vocal_tract(src, areas)
    # The breath's flow is not sound; and the growl must carry on small headphones, so its weight is kept above 140 Hz.
    return sg.sosfilt(sg.butter(2, 140 / (R / 2), "highpass", output="sos"), v)


def glass_body(hz, tau, mass=3.0):
    """A thin glass: a goblet's modes (1, 2.32, 4.25, 6.63 times the lowest), each a beating pair."""
    f, taus, rad = [], [], []
    for r, k, amp in ((1.0, 1.0, 1.0), (2.32, 0.55, 0.32), (4.25, 0.3, 0.12), (6.63, 0.2, 0.05)):
        for df, share in ((0.0, 1.0), (6.0 * r ** 0.5, 0.6)):
            f.append(hz * r + df)
            taus.append(tau * k)
            rad.append(amp * share)
    return ph.Body(f, taus, 1.0, rad, mass)


def blade_body(hz, tau, rng):
    """A thin steel blade: a free bar's bending modes (1, 2.756, 5.404, 8.933, 13.34, 18.64) and its twisting ones
    between them, each a beating pair, each moving a different amount where it is touched."""
    f, taus, at, rad = [], [], [], []
    for r in (1.0, 1.65, 2.756, 3.3, 4.95, 5.404, 8.933, 13.34, 18.64):
        a = rng.uniform(0.35, 1.0)
        for df, share in ((0.0, 1.0), (0.003 * hz * r, 0.8)):
            f.append(hz * r + df)
            taus.append(tau / r ** 0.6)
            at.append(a)
            rad.append(share / r ** 0.3)
    return ph.Body(f, taus, at, rad, mass=1.0)


def ambush_cue(rng):
    """The strike will land from behind: a blade swung past the ear. Its wake sheds vortices at 0.2 v / d (a 3 mm
    edge at 12 m/s: ~800 Hz), loud as v^3 and near, its pitch Doppler-shifted up as it comes and down as it goes; at
    the closest point its steel sings, falling as it leaves."""
    L = 0.11
    n = N(L)
    t = tt(n)
    tc = 0.035
    x = 12.0 * (t - tc)
    r = np.sqrt(0.35 ** 2 + x ** 2)
    doppler = ph.C_AIR / (ph.C_AIR + 12.0 * x / r)
    speed = np.exp(-0.5 * ((t - tc) / 0.016) ** 2)
    fc = 800.0 * (0.4 + 0.6 * speed) * doppler
    amp = speed ** 3 * 0.35 / r
    wake = bandpass_q(rng.standard_normal(n), fc, 2.5) + 0.4 * bandpass_q(rng.standard_normal(n), 2.2 * fc, 2.0)
    wake *= amp
    ring = ph.strike(blade_body(2637.0, 0.03, rng), n, 0.08, speed=0.5, at=tc, bend=np.where(t > tc, doppler, 1.0))
    return ph.in_room(unit(wake) + 0.35 * unit(ring), rng)


def layer_cue(rng):
    """The controller's layer opened: a wooden stick tapping a glass lightly at D#6."""
    n = N(0.08)
    v = ph.strike(glass_body(1245.0, 0.05), n, 0.25, speed=0.6)
    return ph.in_room(v, rng, refl_lo=0.8, refl_hi=0.5)


def reach_cue(rng):
    """In the strike's reach: a blade drawn a hair from its scabbard and clearing it. The scabbard's mouth rubs it
    (roughness read at a speed rising to 1.2 m/s over 12 ms, so the scrape climbs), then the blade clears and rings;
    the steel's modes do all the talking."""
    n = N(0.10)
    t = tt(n)
    blade = blade_body(1976.0, 0.04, rng)
    speed = np.where(t < 0.012, 1.2 * np.sin(PI / 2 * t / 0.012), 0.0)
    rub = ph.scrape(blade, n, speed, rng)
    clear = ph.strike(blade, n, 0.08, speed=0.5, at=0.012)
    return ph.in_room(0.5 * unit(rub) + unit(clear), rng, refl_lo=0.82, refl_hi=0.62)


def spotted_cue(rng):
    """A Shadow has seen the hero: two horns blasting a note that slurs up a tritone, E flat 5 to A5, each a physical
    model (lips against the bore's pressure: STK's Brass), a few cents apart."""
    L = 0.14
    n = N(L)
    t = tt(n)
    # Blown straight in, 6 ms: an accent on top (or a slower start) knocks the lips out of their buzz for 10-30 ms
    press = env_note(t, L, 0.006, 0.03)
    v = np.zeros(n)
    for cents in (-5.0, 5.0):
        hz = 622.0 * (880.0 / 622.0) ** ease((t - 0.035) / 0.02) * 2 ** (cents / 1200.0)
        v += ph.brass(hz * (1 + 0.0015 * drift(n, 25, rng)), press * (1 + 0.02 * drift(n, 30, rng)), rng)
    v = sg.sosfilt(sg.butter(2, 400 / (R / 2), "highpass", output="sos"), v)   # the lips' slow wobble under the note
    return ph.in_room(v, rng)


def lost_cue(rng):
    """It lost the hero: a wooden flute (STK's Flute: a jet driving a bore) sliding down a fourth, D5 to A4, as the
    breath behind it eases off; the noise of the breath is part of the model. A little farther away."""
    L = 0.16
    n = N(L)
    t = tt(n)
    hz = 587.0 * (440.0 / 587.0) ** ease((t - 0.03) / 0.07)
    # a firm first puff so it speaks within ~30 ms (a flute takes 50 left alone), easing off as it slides
    press = 0.95 * env_note(t, L, 0.008, 0.05) * (1.0 - 0.06 * t / L) * (1 + 0.3 * np.exp(-t / 0.02))
    v = ph.flute(hz * (1 + 0.002 * drift(n, 20, rng)), press, rng)
    v = sg.sosfilt(sg.butter(2, 250 / (R / 2), "highpass", output="sos"), v)   # the breath's steady push, not sound
    return ph.in_room(v, rng, src=(0.8, 3.0, 1.5), lis=(3.8, 1.0, 1.6))


SONAR = [("exit", exit_cue), ("door", door_cue), ("person", person_cue), ("shop", shop_cue), ("thing", thing_cue),
         ("event", event_cue), ("fragment", fragment_cue)]
SIGNALS = [("shadow", shadow_cue), ("ambush", ambush_cue), ("layer", layer_cue), ("reach", reach_cue),
           ("spotted", spotted_cue), ("lost", lost_cue)]


# ---- Levelling with ffmpeg -----------------------------------------------------------------------------------------

def ffmpeg(*args):
    return subprocess.run(["ffmpeg", "-hide_banner", "-nostdin", "-y", *args], capture_output=True, text=True)


def momentary_max(path):
    """EBU R128 momentary loudness (LUFS), the loudest 400 ms: for a cue shorter than that, its whole K-weighted
    energy, which is how loud a short sound is heard."""
    r = ffmpeg("-v", "verbose", "-i", path, "-af", "apad=pad_dur=0.6,ebur128=framelog=verbose", "-f", "null", "-")
    vals = [float(m) for m in re.findall(r"\bM:\s*(-?[\d.]+)", r.stderr)]
    return max(vals) if vals else None


def true_peak_db(path):
    r = ffmpeg("-i", path, "-af", "ebur128=peak=true", "-f", "null", "-")
    m = re.findall(r"Peak:\s*(-?[\d.]+|-inf)\s*dBFS", r.stderr)
    return float(m[-1]) if m and m[-1] != "-inf" else -120.0


def write_f32(path, v):
    wavfile.write(path, R, v.astype(np.float32))


def level(raw_path, out_path, target, ceiling_db=-1.0):
    """ffmpeg: measure, gain to the target loudness (backing off if the true peak would pass the ceiling), resample-
    safe 24-bit out."""
    now = momentary_max(raw_path)
    gain = target - now
    tp = true_peak_db(raw_path)
    if tp + gain > ceiling_db:
        gain = ceiling_db - tp
    ffmpeg("-i", raw_path, "-af", f"volume={gain:.3f}dB", "-ar", str(R), "-c:a", "pcm_s24le", out_path)
    return now, gain, momentary_max(out_path), true_peak_db(out_path)


# ---- Reels ---------------------------------------------------------------------------------------------------------

def speak(labels, folder):
    """Spoken labels with Windows' own voice (System.Speech), levelled with ffmpeg's loudnorm."""
    os.makedirs(folder, exist_ok=True)
    lines = ["Add-Type -AssemblyName System.Speech", "$s = New-Object System.Speech.Synthesis.SpeechSynthesizer",
             "$s.Rate = 1"]
    for key, text in labels.items():
        raw = os.path.join(folder, key + ".raw.wav").replace("'", "''")
        lines += [f"$s.SetOutputToWaveFile('{raw}')", f"$s.Speak('{text}')"]
    lines += ["$s.SetOutputToNull()", "$s.Dispose()"]
    subprocess.run(["powershell", "-NoProfile", "-Command", "; ".join(lines)], check=True)
    for key in labels:
        raw = os.path.join(folder, key + ".raw.wav")
        ffmpeg("-i", raw, "-af", "loudnorm=I=-24:TP=-2:LRA=7,aresample=48000", "-ac", "1", "-c:a", "pcm_f32le",
               os.path.join(folder, key + ".wav"))
        os.remove(raw)


def read(path):
    r, x = wavfile.read(path)
    scale = {np.dtype(np.int16): 2 ** 15, np.dtype(np.int32): 2 ** 31}.get(x.dtype, 1)   # scipy reads 24-bit as int32
    x = x.astype(np.float64) / scale
    if x.ndim > 1:
        x = x.mean(axis=1)
    if r != R:
        x = sg.resample_poly(x, R, r)
    return x


def silence(sec):
    return np.zeros(N(sec))


def reel(path, sections):
    """sections: [(label wav, [(label wav or None, cue wav, times), ...]), ...]"""
    parts = [silence(0.3)]
    for head, takes in sections:
        parts += [read(head), silence(0.35)]
        for lab, cue, times in takes:
            if lab:
                parts += [read(lab), silence(0.25)]
            c = read(cue)
            for _ in range(times):
                parts += [c, silence(0.42)]
            parts.append(silence(0.25))
        parts.append(silence(0.5))
    v = np.concatenate(parts)
    wavfile.write(path, R, (np.clip(v, -1, 1) * 32767).astype(np.int16))


def main(args):
    old = args[args.index("--old") + 1] if "--old" in args else None
    raw_dir, new_dir, reel_dir, lab_dir, spec_dir = (os.path.join(OUT, d) for d in ("raw", "new", "reels", "labels", "spectro"))
    for d in (raw_dir, new_dir, reel_dir, spec_dir):
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)

    made = []
    for i, (name, fn) in enumerate(SONAR):
        for brief in (False, True):
            key = name + ("_brief" if brief else "")
            write_f32(os.path.join(raw_dir, key + ".wav"), finish(fn(brief, np.random.default_rng(1000 + i))))
            made.append(key)
    for i, (name, fn) in enumerate(SIGNALS):
        v = fn(np.random.default_rng(2000 + i))
        write_f32(os.path.join(raw_dir, name + ".wav"), finish(v, fade_out_ms=30 if name == "shadow" else 18))
        made.append(name)

    # The target: the old set's average, so the new set sits in the mix where the old one did.
    if old:
        olds = [momentary_max(os.path.join(old, k + ".wav")) for k in made if os.path.exists(os.path.join(old, k + ".wav"))]
        target = float(np.mean(olds))
    else:
        target = -21.0
    print(f"target momentary loudness {target:.1f} LUFS")
    for key in made:
        before, gain, after, tp = level(os.path.join(raw_dir, key + ".wav"), os.path.join(new_dir, key + ".wav"), target)
        dur = len(read(os.path.join(new_dir, key + ".wav"))) / R
        print(f"  {key:16s} {dur * 1000:4.0f} ms  {before:6.1f} -> {after:6.1f} LUFS  gain {gain:+5.1f} dB  true peak {tp:5.1f} dBTP")
        for tag, src in (("new", new_dir), ("old", old)):
            p = os.path.join(src, key + ".wav") if src else None
            if p and os.path.exists(p):
                ffmpeg("-i", p, "-lavfi", "apad=pad_dur=0.05,showspectrumpic=s=600x400:legend=1:scale=log:fscale=log:color=intensity",
                       os.path.join(spec_dir, f"{key}.{tag}.png"))

    if old:
        names = {"exit": "Exit", "door": "Door", "person": "Person", "shop": "Shop", "thing": "Thing",
                 "event": "Event", "fragment": "Fragment", "shadow": "Shadow near", "ambush": "Ambush",
                 "layer": "Layer opened", "reach": "In reach", "spotted": "Spotted", "lost": "Lost you"}
        labels = {k: v for k, v in names.items()}
        labels.update({"_old": "before", "_new": "new", "_brief": "new, brief"})
        speak(labels, lab_dir)
        lab = lambda k: os.path.join(lab_dir, k + ".wav")
        sonar = [(lab(k), [(lab("_old"), os.path.join(old, k + ".wav"), 3), (lab("_new"), os.path.join(new_dir, k + ".wav"), 3),
                           (lab("_brief"), os.path.join(new_dir, k + "_brief.wav"), 3)]) for k, _ in SONAR]
        sigs = [(lab(k), [(lab("_old"), os.path.join(old, k + ".wav"), 3), (lab("_new"), os.path.join(new_dir, k + ".wav"), 3)])
                for k, _ in SIGNALS]
        reel(os.path.join(reel_dir, "1 sonar cues, before then new.wav"), sonar)
        reel(os.path.join(reel_dir, "2 signals, before then new.wav"), sigs)
        reel(os.path.join(reel_dir, "3 new set only.wav"),
             [(lab(k), [(None, os.path.join(new_dir, k + ".wav"), 2)]) for k, _ in SONAR + SIGNALS])
    shutil.rmtree(raw_dir, ignore_errors=True)


if __name__ == "__main__":
    main(sys.argv[1:])
