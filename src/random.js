/*
 * random.js -- the Random page, drawn by the module.
 *
 *     OSC    FILT    AMP    MOD        the Random page's knobs, top row
 *   <         Pad              >       the generator: jog left / right
 *     MOTN   SPACE   WILD   FRESH      the Random page's knobs, bottom row
 *
 * Click into the page, turn the jog to choose what to make (Vary Current, or
 * a category), click to make it, click again for another. The knobs work
 * whether or not you have clicked in: they are ordinary parameters on this
 * page, turned by the host. Back, while a new sound is playing that you have
 * not kept, asks Keep or Revert: Revert puts back exactly the sound you had
 * before the first click.
 *
 * The section knobs (OSC .. SPACE) and FRESH are bipolar -- the middle is the
 * generator's own amount -- so their faders grow from the centre. WILD starts
 * at zero and only goes up.
 *
 * The plugin does the work: this page sends one command key and reads one
 * status key (see tenebrae.c, "The Random page's one command key"). The
 * status also carries the knob values, so the page can draw them on any host.
 *
 * WHAT THE HOST GIVES THIS PAGE (an `as_page` + `enterable` canvas):
 *   - un-entered, it is a DOOR: the jog pages past it and a click enters.
 *     Entered, jog and click arrive here as CC 14 / CC 3, Back asks
 *     handleBack (true = stay), and ctx.close() leaves.
 *   - the ENTERING click is the host's and nothing reports it, so "entered"
 *     is inferred: any gesture that reaches onMidi proves it. Until then the
 *     generator line is drawn un-entered, inside the host's corner brackets.
 *   - Shift+jog leaves without asking. The page is then not drawn, so a long
 *     gap in draws means you came back; a sound left playing that way is
 *     still not kept, and Back next time still offers Revert.
 *   - the draw cannot read params on every host: the status arrives as this
 *     page's `extra_keys` value, and onMidi reads it directly after a write.
 *   - a host may re-send a write it did not see confirmed. A command carries
 *     a nonce, so a re-sent one is the same value, which the plugin ignores.
 */

const CC_JOG = 14;
const CC_CLICK = 3;

const TYPES = ['Vary Current', 'Bass', 'Lead', 'Pad', 'Keys', 'Pluck', 'Brass', 'Voice', 'Stab', 'Sequence', 'FX'];
const OP_KEEP = 0, OP_REVERT = 1, OP_VARY = 2, OP_ROLL = 10;

/* The knobs, in the plugin's order (tenebrae_random.h TB_RK) */
const KNOBS = [
    { key: 'rnd_osc', label: 'OSC', bipolar: true }, { key: 'rnd_flt', label: 'FILT', bipolar: true },
    { key: 'rnd_amp', label: 'AMP', bipolar: true }, { key: 'rnd_mod', label: 'MOD', bipolar: true },
    { key: 'rnd_mot', label: 'MOTN', bipolar: true }, { key: 'rnd_spc', label: 'SPACE', bipolar: true },
    { key: 'rnd_wild', label: 'WILD', bipolar: false }, { key: 'rnd_fresh', label: 'FRESH', bipolar: true },
];

/* A page that has not been drawn for this long was not on screen. Generous:
 * a busy host can skip frames for a few hundred ms, and mistaking that for
 * "you left" drops you out of the page mid-use. */
const AWAY_MS = 1000;

/* ---- the host's knob-label font (font4x5.mjs), copied: a page script cannot
 * import host modules. [advance, yOff, w, h, ...rows], bit0 = leftmost. ---- */
const CHARS = " !\"'()+,-./:0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ%<>=?*#&_\\^";
const G = [
    [3, 0, 0, 5],                       /* ' ' — advance only */
    [2, 0, 1, 5, 1, 1, 1, 0, 1],        /* ! */
    [4, 0, 3, 5, 5, 5, 0, 0, 0],        /* " */
    [2, 0, 1, 5, 1, 1, 0, 0, 0],        /* ' */
    [3, 0, 2, 5, 2, 1, 1, 1, 2],        /* ( */
    [3, 0, 2, 5, 1, 2, 2, 2, 1],        /* ) */
    [4, 0, 3, 5, 0, 2, 7, 2, 0],        /* + */
    [3, 0, 2, 5, 0, 0, 0, 2, 1],        /* , */
    [4, 0, 3, 5, 0, 0, 7, 0, 0],        /* - */
    [2, 0, 1, 5, 0, 0, 0, 0, 1],        /* . */
    [5, 0, 4, 5, 8, 8, 6, 1, 1],        /* / */
    [2, 0, 1, 5, 0, 1, 0, 1, 0],        /* : */

    [5, 0, 4, 5, 6, 9, 9, 9, 6],        /* 0 */
    [4, 0, 3, 5, 2, 3, 2, 2, 7],        /* 1 */
    [5, 0, 4, 5, 7, 8, 6, 1, 15],       /* 2 */
    [5, 0, 4, 5, 7, 8, 6, 8, 7],        /* 3 */
    [5, 0, 4, 5, 9, 9, 15, 8, 8],       /* 4 */
    [5, 0, 4, 5, 15, 1, 7, 8, 7],       /* 5 */
    [5, 0, 4, 5, 14, 1, 7, 9, 6],       /* 6 */
    [5, 0, 4, 5, 15, 8, 4, 2, 1],       /* 7 */
    [5, 0, 4, 5, 6, 9, 6, 9, 6],        /* 8 */
    [5, 0, 4, 5, 6, 9, 14, 8, 7],       /* 9 */

    [5, 0, 4, 5, 6, 9, 15, 9, 9],       /* A  (Elektron) */
    [5, 0, 4, 5, 7, 9, 7, 9, 7],        /* B */
    [5, 0, 4, 5, 14, 1, 1, 1, 14],      /* C */
    [5, 0, 4, 5, 7, 9, 9, 9, 7],        /* D  (Elektron) */
    [5, 0, 4, 5, 15, 1, 7, 1, 15],      /* E  (Elektron) */
    [5, 0, 4, 5, 15, 1, 7, 1, 1],       /* F */
    [5, 0, 4, 5, 14, 1, 13, 9, 14],     /* G */
    [5, 0, 4, 5, 9, 9, 15, 9, 9],       /* H */
    [2, 0, 1, 5, 1, 1, 1, 1, 1],        /* I  (Elektron) */
    [5, 0, 4, 5, 12, 8, 8, 9, 6],       /* J */
    [5, 0, 4, 5, 9, 5, 3, 5, 9],        /* K */
    [5, 0, 4, 5, 1, 1, 1, 1, 15],       /* L  (Elektron) */
    [6, 0, 5, 5, 17, 27, 21, 17, 17],   /* M  (Elektron) */
    [5, 0, 4, 5, 9, 11, 13, 9, 9],      /* N */
    [5, 0, 4, 5, 6, 9, 9, 9, 6],        /* O */
    [5, 0, 4, 5, 7, 9, 7, 1, 1],        /* P  (Elektron) */
    [5, 0, 4, 5, 6, 9, 9, 5, 10],       /* Q */
    [5, 0, 4, 5, 7, 9, 7, 5, 9],        /* R */
    [5, 0, 4, 5, 14, 1, 6, 8, 7],       /* S */
    [4, 0, 3, 5, 7, 2, 2, 2, 2],        /* T  (Elektron) */
    [5, 0, 4, 5, 9, 9, 9, 9, 6],        /* U  (Elektron) */
    [6, 0, 5, 5, 17, 17, 17, 10, 4],    /* V */
    [6, 0, 5, 5, 17, 17, 21, 21, 10],   /* W */
    [5, 0, 4, 5, 9, 9, 6, 9, 9],        /* X */
    [4, 0, 3, 5, 5, 5, 2, 2, 2],        /* Y */
    [5, 0, 4, 5, 15, 8, 6, 1, 15],      /* Z */

    [5, 0, 4, 5, 9, 8, 6, 1, 9],        /* % */
    [4, 0, 3, 5, 4, 2, 1, 2, 4],        /* < */
    [4, 0, 3, 5, 1, 2, 4, 2, 1],        /* > */
    [4, 0, 3, 5, 0, 7, 0, 7, 0],        /* = */
    [5, 0, 4, 5, 7, 8, 6, 0, 2],        /* ? */
    [4, 0, 3, 5, 5, 2, 5, 0, 0],        /* * */
    [5, 0, 4, 5, 10, 15, 10, 15, 10],   /* # */
    [5, 0, 4, 5, 6, 9, 6, 5, 10],       /* & */
    [5, 0, 4, 5, 0, 0, 0, 0, 15],       /* _ */
    [5, 0, 4, 5, 1, 1, 6, 8, 8],        /* \ */
    [4, 0, 3, 5, 2, 5, 0, 0, 0],        /* ^ */
];
function glyph4(ch) { const i = CHARS.indexOf(ch); return i >= 0 ? G[i] : null; }
function width4(str) {
    let w = 0;
    for (const ch of String(str)) { const g = glyph4(ch); w += g ? g[0] : 5; }
    return w > 0 ? w - 1 : 0;
}
function print4(ctx, x, y, str, color) {
    let cx = x;
    for (const ch of String(str)) {
        const g = glyph4(ch);
        if (!g) { cx += 5; continue; }
        for (let row = 0; row < g[3]; row++) {
            const bits = g[4 + row];
            for (let col = 0; col < g[2]; col++)
                if (bits & (1 << col)) ctx.fillRect(cx + col, y + g[1] + row, 1, 1, color);
        }
        cx += g[0];
    }
}

/* ---- the knob rows: four 32px cells, a label over a horizontal fader ---- */
const CELL_W = 32, FADER_X = 3, FADER_W = 26;
function drawKnob(ctx, x, y, k, v) {
    print4(ctx, x + Math.round((CELL_W - width4(k.label)) / 2), y, k.label, 1);
    const fy = y + 8, fx = x + FADER_X;
    /* the track: a dotted line, as the host draws a scrollbar track */
    for (let i = 0; i < FADER_W; i += 2) ctx.fillRect(fx + i, fy + 1, 1, 1, 1);
    const val = typeof v === 'number' && isFinite(v) ? v : 0;
    if (k.bipolar) {
        /* bipolar: a centre mark for the default, the bar grows either way */
        const mid = fx + (FADER_W >> 1);
        ctx.fillRect(mid, fy - 1, 1, 5, 1);
        const len = Math.round(Math.abs(val) * (FADER_W >> 1));
        if (len > 0) ctx.fillRect(val > 0 ? mid + 1 : mid - len, fy, len, 3, 1);
    } else {
        const len = Math.round(Math.max(0, Math.min(1, val)) * FADER_W);
        ctx.fillRect(fx - 1, fy - 1, 1, 5, 1);   /* the zero end */
        if (len > 0) ctx.fillRect(fx, fy, len, 3, 1);
    }
}

/* ---- the generator line ---- */
const LINE_Y = 17, LINE_H = 11;
/* The host face's glyph widths, space (32) to '~' (126), 1px spacing. */
const GLYPH_W = '51355552335525255355555555224545555555555355555555555555555353553554555553443555555555555553135';
function textW(t) {
    const s = String(t);
    let w = 0;
    for (let i = 0; i < s.length; i++) {
        const c = s.charCodeAt(i);
        w += (c >= 32 && c <= 126 ? Number(GLYPH_W[c - 32]) : 5) + 1;
    }
    return w > 0 ? w - 1 : 0;
}
const FONT_H = 7;

/* dAVEBOx's dialog buttons (menu_layout drawDialogButtonRow), in this band. */
const BTN_Y = 33, BTN_H = 13, BTN_X0 = 6, BTN_X1 = 122, BTN_PAD = 6, BTN_GAP = 4;

function outline(ctx, x, y, w, h) {
    ctx.fillRect(x, y, w, 1, 1);
    ctx.fillRect(x, y + h - 1, w, 1, 1);
    ctx.fillRect(x, y, 1, h, 1);
    ctx.fillRect(x + w - 1, y, 1, h, 1);
}

function drawButtonRow(ctx, buttons) {
    const n = buttons.length;
    const widths = buttons.map((b) => textW(b.label) + BTN_PAD * 2);
    const span = BTN_X1 - BTN_X0 - BTN_GAP * (n - 1);
    const used = widths.reduce((a, b) => a + b, 0);
    if (used < span) {
        const extra = Math.floor((span - used) / n);
        for (let i = 0; i < n; i++) widths[i] += extra;
    }
    let x = BTN_X0;
    for (let i = 0; i < n; i++) {
        const b = buttons[i], w = widths[i];
        const lx = x + Math.round((w - textW(b.label)) / 2);
        const ly = BTN_Y + Math.max(1, Math.round((BTN_H - FONT_H) / 2));
        if (b.sel) { ctx.fillRect(x, BTN_Y, w, BTN_H, 1); ctx.print(lx, ly, b.label, 0); }
        else { outline(ctx, x, BTN_Y, w, BTN_H); ctx.print(lx, ly, b.label, 1); }
        x += w + BTN_GAP;
    }
}

/* Body lines, centred, 10px apart, above the buttons; wrapped by width. */
function drawLines(ctx, lines) {
    const out = [];
    for (const raw of lines) {
        if (!raw) continue;
        let cur = '';
        for (const word of String(raw).split(' ')) {
            const next = cur ? cur + ' ' + word : word;
            if (cur && textW(next) > 124) { out.push(cur); cur = word; }
            else cur = next;
        }
        if (cur) out.push(cur);
    }
    const pitch = 10;
    const block = out.length * pitch - (pitch - FONT_H);
    const top = Math.max(0, Math.floor((BTN_Y - 2 - block) / 2));
    for (let i = 0; i < out.length; i++)
        ctx.print(Math.max(0, Math.floor((SCREEN_W - textW(out[i])) / 2)), top + i * pitch, out[i], 1);
}

const SCREEN_W = 128;

/* The brackets of an un-entered door, around the generator line */
const FRAME_X = 4, FRAME_W = 120, ARM = 4;
function drawBrackets(ctx) {
    const x = FRAME_X, y = LINE_Y - 2, w = FRAME_W, h = LINE_H + 3;
    for (let i = 0; i < ARM; i++) {
        ctx.fillRect(x + i, y, 1, 1, 1);
        ctx.fillRect(x + w - 1 - i, y, 1, 1, 1);
        ctx.fillRect(x + i, y + h - 1, 1, 1, 1);
        ctx.fillRect(x + w - 1 - i, y + h - 1, 1, 1, 1);
    }
    for (let i = 0; i < ARM - 1; i++) {
        ctx.fillRect(x, y + i, 1, 1, 1);
        ctx.fillRect(x + w - 1, y + i, 1, 1, 1);
        ctx.fillRect(x, y + h - 1 - i, 1, 1, 1);
        ctx.fillRect(x + w - 1, y + h - 1 - i, 1, 1, 1);
    }
}

/* ------------------------------------------------------------ state */

const S = {
    entered: false,   /* inferred -- see the header */
    view: 'page',     /* page | ask */
    type: 0,          /* TYPES index */
    sel: 0,           /* dialog button: 0 Keep, 1 Revert */
    nonce: 1 + Math.floor(Math.random() * 100000),
    status: null,
    lastDraw: -1,
};

function parseStatus(raw) {
    if (raw && typeof raw === 'object') return raw;
    if (typeof raw !== 'string' || raw.charAt(0) !== '{') return null;
    try { return JSON.parse(raw); } catch (e) { return null; }
}

/* Take a status only if it is not older than the one already held. */
function takeStatus(st) {
    if (!st) return S.status;
    if (S.status && typeof st.seq === 'number' && typeof S.status.seq === 'number' && st.seq < S.status.seq)
        return S.status;
    S.status = st;
    return st;
}

function readStatus(ctx) {
    return takeStatus(parseStatus(ctx.getParam('rnd_status')));
}

function send(ctx, op) {
    S.nonce = (S.nonce + 1) % 20000000;
    ctx.setParam('rnd_cmd', String(S.nonce * 100 + op));
    return readStatus(ctx);
}

function arrived() {
    S.entered = false;
    S.view = 'page';
    S.sel = 0;
}

/* A knob's value: the host's own reading when the draw is handed one, else
 * the plugin's status */
function knobValue(info, st, i) {
    const vals = info && info.values;
    if (vals && vals[KNOBS[i].key] !== undefined && vals[KNOBS[i].key] !== null && vals[KNOBS[i].key] !== '') {
        const n = Number(vals[KNOBS[i].key]);
        if (isFinite(n)) return n;
    }
    return st && Array.isArray(st.rk) ? Number(st.rk[i]) : 0;
}

globalThis.canvas_overlay = {
    onMidi(ctx, msg) {
        const d = msg && msg.data;
        if (!d || d.length < 3 || (d[0] & 0xF0) !== 0xB0) return;
        S.entered = true;   /* only an entered door is handed gestures */
        if (d[1] === CC_JOG) {
            const v = d[2];
            const delta = v === 0 ? 0 : (v <= 63 ? v : -(128 - v));
            if (!delta) return;
            if (S.view === 'page')
                S.type = Math.max(0, Math.min(TYPES.length - 1, S.type + (delta > 0 ? 1 : -1)));
            else
                S.sel = delta > 0 ? 1 : 0;
            return;
        }
        if (d[1] !== CC_CLICK || d[2] === 0) return;
        if (S.view === 'page') {
            send(ctx, S.type === 0 ? OP_VARY : OP_ROLL + (S.type - 1));
            return;
        }
        /* the Keep / Revert question: answer it, and leave */
        send(ctx, S.sel === 0 ? OP_KEEP : OP_REVERT);
        arrived();
        ctx.close();
    },

    /* true = stay on this page; anything else = the host leaves it. */
    handleBack(ctx) {
        if (S.view === 'ask') { S.view = 'page'; return true; }
        const st = readStatus(ctx);
        if (st && st.pending) {
            S.view = 'ask';
            S.sel = 0;
            return true;
        }
        S.entered = false;
        return false;
    },

    drawPage(ctx, info) {
        const now = info && typeof info.nowMs === 'number' ? info.nowMs : Date.now();
        if (S.lastDraw < 0 || now - S.lastDraw > AWAY_MS) arrived();
        S.lastDraw = now;
        const st = takeStatus(parseStatus(info && info.values ? info.values.rnd_status : null));

        if (S.view === 'ask') {
            const name = st && st.name ? st.name : 'this sound';
            const back = st && st.anchor ? st.anchor : 'the last sound';
            drawLines(ctx, ['Keep ' + name + '?', 'Revert goes back to ' + back]);
            drawButtonRow(ctx, [{ label: 'Keep', sel: S.sel === 0 }, { label: 'Revert', sel: S.sel === 1 }]);
            return;
        }

        for (let i = 0; i < 4; i++) drawKnob(ctx, i * CELL_W, 1, KNOBS[i], knobValue(info, st, i));
        for (let i = 4; i < 8; i++) drawKnob(ctx, (i - 4) * CELL_W, 33, KNOBS[i], knobValue(info, st, i));

        const label = TYPES[S.type];
        const on = S.entered;
        if (on) ctx.fillRect(0, LINE_Y, SCREEN_W, LINE_H, 1);
        const ty = LINE_Y + Math.floor((LINE_H - FONT_H) / 2);
        if (S.type > 0) ctx.print(8, ty, '<', on ? 0 : 1);
        if (S.type < TYPES.length - 1) ctx.print(SCREEN_W - 8 - textW('>'), ty, '>', on ? 0 : 1);
        ctx.print(Math.round((SCREEN_W - textW(label)) / 2), ty, label, on ? 0 : 1);
        if (!on) drawBrackets(ctx);
    },
};
