"""Physical models for design.py. Each one simulates the thing making the sound, rather than describing the sound:

  Body + hertz(): an object as its modes, struck by a mallet through a Hertzian contact (F = K d^1.5, with Hunt-
    Crossley damping for skin or felt) that is integrated together with the object's own motion. The contact's length,
    and so the brightness, comes out of the masses and stiffnesses; a soft object lengthens it; a mallet can bounce.
  scrape(): a body driven by a surface's roughness read at the speed something slides along it.
  plucked_string(): a string's two planes of vibration (they tune and die apart: the double decay of a real string),
    released from a triangle at the pluck point, through a soundboard's modes.
  membrane(): a drumhead's modes from Bessel zeros, their shapes at the strike radius.
  air_tube(): a tube of air as a digital waveguide (a delay line whose length is the tube's) driven by turbulence.
  vocal_tract(): a Kelly-Lochbaum tube of ~60 sections shaped like a throat and mouth, driven by glottal pulses.
  brass(), flute(): the lip-reed and jet-drive waveguide instruments of Cook and Scavone's Synthesis ToolKit.
  room_ir(): a shoebox room by image sources (each wall's reflection, to the fourth order, weaker in the highs), with a
    diffuse tail after them.
"""
import numpy as np
from scipy import signal as sg
from scipy import special

R = 48000
PI = np.pi
C_AIR = 343.0


def N(sec):
    return int(round(sec * R))


def tt(n):
    return np.arange(n) / R


# ---- Struck and scraped bodies -------------------------------------------------------------------------------------

class Body:
    """An object as its modes: frequency (Hz), decay time constant (s), the mode's shape where it is struck, how well
    it radiates, and the object's modal mass (relative to the mallet's)."""

    def __init__(self, f, tau, at, rad, mass=1.0):
        f, tau, at, rad = (np.broadcast_to(np.asarray(x, float), np.shape(f)).copy() for x in (f, tau, at, rad))
        keep = f < R * 0.45
        self.f, self.tau, self.at, self.rad, self.mass = f[keep], tau[keep], at[keep], rad[keep], mass

    def response(self, n, bend=None):
        """The radiated sound per unit force impulse at the strike point: each mode's acceleration (what a small
        vibrating thing pushes the air with), weighted; in units of a 1 kHz mode's."""
        t = tt(n)
        clock = np.cumsum(bend) / R if bend is not None else t
        out = np.zeros(n)
        for k in range(0, len(self.f), 64):
            sl = slice(k, k + 64)
            w = (self.at[sl] * self.rad[sl])[:, None]
            ph = 2 * PI * self.f[sl, None] * clock[None, :]
            om = 2 * PI * self.f[sl, None] / (2 * PI * 1000.0)
            decay = np.exp(-t[None, :] / self.tau[sl, None])
            out -= np.sum(w * decay * (om * np.sin(ph) + np.cos(ph) / (self.tau[sl, None] * 2 * PI * 1000.0)), 0)
        return out / self.mass

    def sound(self, force, n, bend=None):
        """The response to a force. The modal sum leaves out the step the contact itself puts in at t = 0 (its DC),
        which only shows as rumble below the lowest mode: a high-pass at half of it takes that away."""
        y = sg.fftconvolve(force, self.response(n, bend))[:n]
        cut = max(30.0, 0.5 * float(np.min(self.f)))
        return sg.sosfilt(sg.butter(2, cut / (R / 2), "highpass", output="sos"), y)


def hertz(body, contact_ms, speed=1.0, softness=0.0, over=8, window=0.03):
    """The force a mallet (mass 1) puts into `body` striking it at `speed`: a Hertzian contact whose stiffness gives
    `contact_ms` on a rigid surface at speed 1, integrated with the body's motion at the strike point. softness: Hunt-
    Crossley damping (s per unit), the give of skin or felt. Returns the force at R."""
    rs = R * over
    dt = 1.0 / rs
    k = 1.25 * (2.94 / (contact_ms / 1000.0)) ** 2.5
    w = 2 * PI * body.f
    r = np.exp(-dt / body.tau)
    a1, a2 = 2 * r * np.cos(w * dt), -r * r
    b = dt * r * np.sin(w * dt) / (w * body.mass) * body.at
    steps = int(window * rs)
    q1 = np.zeros(len(w))
    q2 = np.zeros(len(w))
    z, v, u_prev, f_prev, apart = 0.0, speed, 0.0, 0.0, 0
    force = np.zeros(steps)
    for i in range(steps):
        q = a1 * q1 + a2 * q2 + b * f_prev
        u = float(body.at @ q)
        d = z - u
        if d > 0:
            rate = v - (u - u_prev) / dt
            f = max(0.0, k * d ** 1.5 * (1.0 + softness * rate))
            apart = 0
        else:
            f = 0.0
            apart += 1
        v -= f * dt
        z += v * dt
        force[i] = f
        f_prev, q2, q1, u_prev = f, q1, q, u
        if apart > rs * 0.004 and v < 0 and i > rs * 0.002:
            break
    return sg.resample_poly(force, 1, over)


def strike(body, n, contact_ms, speed=1.0, softness=0.0, at=0.0, bend=None, over=8):
    f = hertz(body, contact_ms, speed, softness, over)
    f = np.concatenate([np.zeros(N(at)), f])[:n]
    return body.sound(f, n, bend)


def scrape(body, n, speed, rng, grain=4e-6, beta=1.6, load=None):
    """A body driven by friction against a rough surface: the roughness (a 1/k^beta profile, `grain` metres per
    point) read at the sliding speed (an array, m/s) gives the force; frequency rises with speed, as it does."""
    x = np.cumsum(speed) / R
    pts = int(x[-1] / grain) + 8
    spec = rng.standard_normal(pts // 2 + 1) + 1j * rng.standard_normal(pts // 2 + 1)
    k = np.arange(pts // 2 + 1)
    spec *= np.where(k > 0, 1.0 / np.maximum(k, 1) ** (beta / 2), 0)
    profile = np.fft.irfft(spec, pts)
    h = np.interp(x / grain, np.arange(pts), profile)
    force = np.diff(h, prepend=h[0]) * R * (load if load is not None else 1.0)
    return body.sound(force, n)


# ---- Strings and membranes -----------------------------------------------------------------------------------------

def plucked_string(n, f0, pluck_at, tau1, rng, inharm=1.5e-4, nail_ms=0.3, damp_hz=5000.0, board=None):
    """A string plucked (released from a triangle) at `pluck_at` of its length. Two planes of vibration a hair apart
    in pitch, the one into the board dying faster; the bridge force through the soundboard `board` (a Body)."""
    t = tt(n)
    force = np.zeros(n)
    for k in range(1, 40):
        f = k * f0 * np.sqrt(1 + inharm * k * k)
        if f > 16000:
            break
        amp = np.sin(PI * k * pluck_at) / (k * k * pluck_at * (1 - pluck_at)) * k   # slope at the bridge
        amp *= abs(np.cos(PI * f * nail_ms / 1000)) if f * nail_ms / 1000 < 0.5 else 0.0   # the nail's release
        tau = tau1 / (1 + (f / damp_hz) ** 1.5)
        for plane, detune, life in ((0.62, 1.0, 1.0), (0.38, 1.0 + 0.0009 * rng.uniform(0.7, 1.3), 0.45)):
            force += plane * amp * np.cos(2 * PI * f * detune * t) * np.exp(-t / (tau * life))
    if board is None:
        return force
    body = sg.fftconvolve(force, board.response(n))[:n]
    return body / np.max(np.abs(body)) + 0.15 * force / np.max(np.abs(force))


def membrane(f1, r0, tau0, mass=1.0, count=40):
    """A circular drumhead: modes (m, k) at Bessel zeros (relative to the lowest), shapes J_m at the strike radius r0
    (0 centre, 1 rim); the symmetric modes radiate as a monopole, the rest weakly."""
    f, tau, at, rad = [], [], [], []
    z01 = special.jn_zeros(0, 1)[0]
    for m in range(0, 9):
        for k, z in enumerate(special.jn_zeros(m, 5)):
            ratio = z / z01
            f.append(f1 * ratio)
            tau.append(tau0 / ratio ** 1.4)
            at.append(special.jv(m, z * r0))
            rad.append(1.0 if m == 0 else 0.35 * ratio ** 0.5 / m)
    order = np.argsort(f)[:count]
    return Body(np.array(f)[order], np.array(tau)[order], np.array(at)[order], np.array(rad)[order], mass)


# ---- Air -------------------------------------------------------------------------------------------------------------

def air_tube(excite, length_m, loss=0.92, damp=0.35):
    """A tube open at both ends as a waveguide: the excitation plus what comes back from a round trip (2 L / c,
    following length_m over time), each trip a little weaker and duller (the open ends radiate the highs)."""
    n = len(excite)
    delay = 2 * np.asarray(length_m) / C_AIR * R
    buf = np.zeros(n)
    lp = 0.0
    for i in range(n):
        d = delay[i]
        j = i - d
        back = 0.0
        if j >= 1:
            j0 = int(j)
            fr = j - j0
            back = buf[j0] * (1 - fr) + buf[j0 + 1] * fr
        lp = (1 - damp) * back + damp * lp
        buf[i] = excite[i] + loss * lp
    return buf


def vocal_tract(source, areas, over=2):
    """A Kelly-Lochbaum tube: `areas` from glottis to lips, one section per sample at R*over each way. The glottis
    reflects 0.75 and the lips -0.85 (an open end), with a little loss along the walls; out: what reaches the lips (the
    pressure, low-passed a little: the tract's walls and the air soak up the highs)."""
    src = sg.resample_poly(source, over, 1)
    a = np.asarray(areas, float)
    k = (a[:-1] - a[1:]) / (a[:-1] + a[1:])
    n_sec = len(a)
    right = np.zeros(n_sec)
    left = np.zeros(n_sec)
    out = np.zeros(len(src))
    lip_lp = 0.0
    for i in range(len(src)):
        jr = np.empty(n_sec + 1)
        jl = np.empty(n_sec + 1)
        jr[0] = left[0] * 0.75 + src[i]
        lip_lp = 0.5 * right[-1] + 0.5 * lip_lp          # the lips reflect the lows more than the highs
        jl[n_sec] = -0.85 * lip_lp
        w = k * (right[:-1] + left[1:])
        jr[1:n_sec] = right[:-1] - w
        jl[1:n_sec] = left[1:] + w
        right = jr[:n_sec] * 0.999
        left = jl[1:] * 0.999
        out[i] = right[-1]
    y = sg.sosfilt(sg.butter(1, 4500 / (R * over / 2), output="sos"), out)
    return sg.resample_poly(y, 1, over)


class Delay:
    """A delay line read with linear interpolation (its length may change every sample)."""

    def __init__(self, size):
        self.buf = np.zeros(size)
        self.at = 0
        self.last = 0.0

    def tick(self, x, d):
        self.buf[self.at] = x
        pos = self.at - d
        i0 = int(np.floor(pos))
        fr = pos - i0
        y = self.buf[i0 % len(self.buf)] * (1 - fr) + self.buf[(i0 + 1) % len(self.buf)] * fr
        self.at = (self.at + 1) % len(self.buf)
        self.last = y
        return y


def brass(hz, pressure, rng, lip_r=0.997):
    """STK's Brass (Cook): the lips a resonant filter driven by mouth minus bore pressure, their opening the square of
    its output (saturating), the bore a delay line. hz and pressure are per sample. The model plays sharp, 1.5% at
    622 Hz and 2.9% at 880 (measured), so it is asked for a little less."""
    n = len(hz)
    hz = np.asarray(hz) * (1 - (0.015 + (np.asarray(hz) - 622.0) * 0.014 / 258.0))
    line = Delay(4096)
    out = np.zeros(n)
    x1 = x2 = y1 = y2 = 0.0
    dc_x = dc_y = 0.0
    for i in range(n):
        d = 2.0 * R / hz[i] + 3.0
        w = 2 * PI * hz[i] / R
        a1, a2 = -2 * lip_r * np.cos(w), lip_r * lip_r
        mouth = 0.3 * pressure[i]
        bore = 0.85 * line.last
        x = 0.03 * (mouth - bore)
        y = x - a1 * y1 - a2 * y2
        y2, y1 = y1, y
        open_ = min(1.0, y * y)
        s = open_ * mouth + (1 - open_) * bore
        dc = s - dc_x + 0.99 * dc_y
        dc_x, dc_y = s, dc
        out[i] = line.tick(dc, d)
    return out


def flute(hz, pressure, rng, noise=0.15, jet_ratio=0.5):
    """STK's Flute (Cook): a jet (a delay and a cubic) driving a bore (a delay with a lossy, inverting reflection);
    the breath's noise rides on the pressure. hz and pressure per sample (keep the pressure near 1: much higher and
    it overblows). STK tunes its bore to 2/3 of the note to be overblown to it; at these pressures it plays the bore's
    own note, 2.7% flat (measured), so the bore is tuned to the note itself; and the jet is half the bore (STK: 0.32),
    which holds it in that register through a slide (at 0.32 it jumped an octave up partway)."""
    n = len(hz)
    bore, jet = Delay(4096), Delay(4096)
    out = np.zeros(n)
    pole = 0.7 - 0.1 * 22050.0 / R
    lp = 0.0
    dc_x = dc_y = 0.0
    for i in range(n):
        d = R / (hz[i] * 1.027) - 2.0
        breath = pressure[i] * (1 + noise * rng.uniform(-1, 1))
        lp = -(1 - pole) * bore.last + pole * lp
        temp = -lp
        dc = temp - dc_x + 0.99 * dc_y
        dc_x, dc_y = temp, dc
        diff = breath - 0.5 * dc
        diff = jet.tick(diff, d * jet_ratio)
        diff = np.clip(diff * (diff * diff - 1.0), -1, 1) + 0.5 * dc
        out[i] = 0.3 * bore.tick(diff, d)
    return out


# ---- The room --------------------------------------------------------------------------------------------------------

def room_ir(rng, dims=(4.6, 3.7, 2.7), src=(1.4, 2.3, 1.4), lis=(3.1, 1.5, 1.6), refl_lo=0.82, refl_hi=0.55,
            order=4, rt=(0.3, 0.12), tail=0.5, length=0.4, wet=0.4):
    """A shoebox room by image sources: the direct sound (gain 1) and every reflection to `order` (1/distance, each
    wall bounce taking refl_lo below 1.5 kHz and refl_hi above), then a diffuse tail (rt60 low, high) from where the
    reflections thin out, `tail` of their energy. All that comes back is scaled to `wet` of the direct sound's energy
    (a small room heard from 2 m; left alone, four orders of image sources give more than the direct sound)."""
    n = N(length)
    lo, hi = np.zeros(n), np.zeros(n)
    d0 = np.linalg.norm(np.subtract(src, lis))
    rng_o = range(-order, order + 1)
    for nx in rng_o:
        for ny in rng_o:
            for nz in rng_o:
                o = abs(nx) + abs(ny) + abs(nz)
                if o == 0 or o > order:
                    continue
                img = []
                for nn, s, L in ((nx, src[0], dims[0]), (ny, src[1], dims[1]), (nz, src[2], dims[2])):
                    img.append(nn * L + s if nn % 2 == 0 else (nn + 1) * L - s)
                d = np.linalg.norm(np.subtract(img, lis))
                j = N((d - d0) / C_AIR)
                if 0 <= j < n:
                    g = d0 / d
                    lo[j] += g * refl_lo ** o
                    hi[j] += g * refl_hi ** o
    early = sg.sosfilt(sg.butter(2, 1500 / (R / 2), output="sos"), lo) + \
        sg.sosfilt(sg.butter(2, 1500 / (R / 2), "highpass", output="sos"), hi)
    start = N(0.012)   # the tail takes over 12-24 ms in (it swelled late when this grew with the room)
    t = tt(n)
    f = np.fft.rfftfreq(512, 1.0 / R)
    noise = rng.standard_normal(n + 512)
    # The tail: noise whose decay rate depends on frequency (two bands, crossfaded).
    lo_t = sg.sosfilt(sg.butter(2, 1500 / (R / 2), output="sos"), noise[:n]) * np.exp(-6.91 * t / rt[0])
    hi_t = sg.sosfilt(sg.butter(2, 1500 / (R / 2), "highpass", output="sos"), noise[512:512 + n]) * np.exp(-6.91 * t / rt[1])
    ramp = np.clip((np.arange(n) - start) / start, 0, 1)
    late = (lo_t + hi_t) * ramp
    late *= np.sqrt(tail * np.sum(early ** 2) / (np.sum(late ** 2) + 1e-12))
    ir = early + late
    ir *= np.sqrt(wet / (np.sum(ir ** 2) + 1e-12))
    ir[0] += 1.0
    return ir


def in_room(v, rng, **kw):
    return sg.fftconvolve(v, room_ir(rng, **kw))[:len(v)]
