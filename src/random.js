/*
 * random.js -- the Random page: a list you click into, drawn by the module.
 *
 *   Vary Current      a sibling of the sound you started from
 *   Bass ... FX       a new sound of that kind
 *
 * Scroll, click to make a sound, click again for another. Nothing else is
 * drawn on the list. Play as much as you like. Back, while a roll is playing
 * that you have not kept, asks Keep or Revert: Revert puts back exactly the
 * sound you had before the first roll.
 *
 * The plugin does the work: this page sends one command key and reads one
 * status key (see tenebrae.c, "The Random page's one command key").
 *
 * WHAT THE HOST GIVES THIS PAGE (an `as_page` + `enterable` canvas):
 *   - un-entered, it is a DOOR: the jog pages past it and a click enters.
 *     Entered, jog and click arrive here as CC 14 / CC 3, Back asks
 *     handleBack (true = stay), and ctx.close() leaves.
 *   - the ENTERING click is the host's and nothing reports it, so "entered"
 *     is inferred: any gesture that reaches onMidi proves it. Until then the
 *     page is drawn un-entered, with the host's corner brackets.
 *   - Shift+jog leaves without asking. The page is then not drawn, so a gap
 *     in draws means you came back; a roll left playing that way is still
 *     not kept, and Back next time still offers Revert.
 *   - the draw cannot read params on every host: the status arrives as this
 *     page's `extra_keys` value, and onMidi reads it directly after a write.
 *   - a host may re-send a write it did not see confirmed. A command carries
 *     a nonce, so a re-sent one is the same value, which the plugin ignores.
 */

const CC_JOG = 14;
const CC_CLICK = 3;

/* The host's list, in this page's band (it starts at screen row 9): rows 9
 * apart from y 1, labels at x 9, the highlight one row above its text, the
 * bracket frame at x 4 across rows 0..44 with arms 4 long. */
const ROW_H = 9, LIST_Y = 1, LIST_X = 9, HI_OFF = 1, VISIBLE = 5;
const FRAME_X = 4, FRAME_Y = 0, FRAME_W = 120, FRAME_H = 45, ARM = 4;
const SCREEN_W = 128, BAR_GUTTER = 3;

const CATS = ['Bass', 'Lead', 'Pad', 'Keys', 'Pluck', 'Brass', 'Voice', 'Stab', 'Sequence', 'FX'];
const ROWS = ['Vary Current'].concat(CATS);
const OP_KEEP = 0, OP_REVERT = 1, OP_VARY = 2, OP_ROLL = 10;

/* A page that has not been drawn for this long was not on screen. */
const AWAY_MS = 250;

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

function drawBrackets(ctx) {
    const x = FRAME_X, y = FRAME_Y, w = FRAME_W, h = FRAME_H;
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

/* The host's scrollbar (menu_layout drawScrollbar): a dotted track two
 * pixels from the right edge, covering the rows, and a solid thumb. */
function drawScrollbar(ctx, start, total) {
    if (!(start > 0 || start + VISIBLE < total)) return;
    const x = SCREEN_W - 2, top = LIST_Y, bottom = LIST_Y + (VISIBLE - 1) * ROW_H + FONT_H;
    const h = bottom - top;
    for (let y = top; y < bottom; y += 2) ctx.fillRect(x, y, 1, 1, 1);
    const th = Math.max(2, Math.round((VISIBLE / total) * h));
    const max = total - VISIBLE;
    const ty = top + (max > 0 ? Math.round((start / max) * (h - th)) : 0);
    ctx.fillRect(x, ty, 1, th, 1);
}

/* ------------------------------------------------------------ state */

const S = {
    entered: false,   /* inferred -- see the header */
    view: 'list',     /* list | ask */
    cursor: 0,
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
    S.view = 'list';
    S.sel = 0;
}

function windowStart() {
    /* the cursor stays off the last visible row while there is more below,
     * as the host's lists do */
    const maxRow = VISIBLE - 2;
    let start = S.cursor > maxRow ? S.cursor - maxRow : 0;
    if (start > ROWS.length - VISIBLE) start = Math.max(0, ROWS.length - VISIBLE);
    return start;
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
            if (S.view === 'list')
                S.cursor = Math.max(0, Math.min(ROWS.length - 1, S.cursor + (delta > 0 ? 1 : -1)));
            else
                S.sel = delta > 0 ? 1 : 0;
            return;
        }
        if (d[1] !== CC_CLICK || d[2] === 0) return;
        if (S.view === 'list') {
            send(ctx, S.cursor === 0 ? OP_VARY : OP_ROLL + (S.cursor - 1));
            return;
        }
        /* the Keep / Revert question: answer it, and leave */
        send(ctx, S.sel === 0 ? OP_KEEP : OP_REVERT);
        arrived();
        ctx.close();
    },

    /* true = stay on this page; anything else = the host leaves it. */
    handleBack(ctx) {
        if (S.view === 'ask') { S.view = 'list'; return true; }
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

        const start = windowStart();
        const scrolls = start > 0 || start + VISIBLE < ROWS.length;
        for (let i = 0; i < VISIBLE && start + i < ROWS.length; i++) {
            const row = start + i;
            const y = LIST_Y + i * ROW_H;
            const on = S.entered && row === S.cursor;
            if (on) ctx.fillRect(0, y - HI_OFF, SCREEN_W - (scrolls ? BAR_GUTTER : 0), ROW_H, 1);
            ctx.print(LIST_X, y, ROWS[row], on ? 0 : 1);
        }
        drawScrollbar(ctx, start, ROWS.length);
        if (!S.entered) drawBrackets(ctx);
    },
};
